/*
 * libvirt-domain.c: The PHP bindings to libvirt domain API
 *
 * See COPYING for the license of this software
 */

#include <config.h>
#include <stdio.h>
#include <libvirt/libvirt.h>

#include "libvirt-php.h"
#include "libvirt-domain.h"
#include "sockets.h"
#include "vncfunc.h"

DEBUG_INIT("domain");

void
php_libvirt_domain_dtor(virt_resource *rsrc TSRMLS_DC)
{
    php_libvirt_domain *domain = (php_libvirt_domain *)rsrc->ptr;
    int rv = 0;

    if (domain != NULL) {
        if (domain->domain != NULL) {
            if (!check_resource_allocation(domain->conn->conn, INT_RESOURCE_DOMAIN, domain->domain TSRMLS_CC)) {
                domain->domain = NULL;
                efree(domain);
                return;
            }

            rv = virDomainFree(domain->domain);
            if (rv != 0) {
                DPRINTF("%s: virDomainFree(%p) returned %d (%s)\n", __FUNCTION__, domain->domain, rv, LIBVIRT_G(last_error));
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "virDomainFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virDomainFree(%p) completed successfully\n", __FUNCTION__, domain->domain);
                resource_change_counter(INT_RESOURCE_DOMAIN, domain->conn->conn, domain->domain, 0 TSRMLS_CC);
            }
            domain->domain = NULL;
        }
        efree(domain);
    }
}

/*
 * Function name:   libvirt_domain_new
 * Since version:   0.4.5
 * Description:     Function is used to install a new virtual machine to the machine
 * Arguments:       @conn [resource]: libvirt connection resource
 *                  @name [string]: name of the new domain
 *                  @arch [string]: optional architecture string, can be NULL to get default (or false)
 *                  @memMB [int]: number of megabytes of RAM to be allocated for domain
 *                  @maxmemMB [int]: maximum number of megabytes of RAM to be allocated for domain
 *                  @vcpus [int]: number of VCPUs to be allocated to domain
 *                  @iso_image [string]: installation ISO image for domain
 *                  @disks [array]: array of disk devices for domain, consist of keys as 'path' (storage location), 'driver' (image type, e.g. 'raw' or 'qcow2'), 'bus' (e.g. 'ide', 'scsi'), 'dev' (device to be presented to the guest - e.g. 'hda'), 'size' (with 'M' or 'G' suffixes, like '10G' for 10 gigabytes image etc.) and 'flags' (VIR_DOMAIN_DISK_FILE or VIR_DOMAIN_DISK_BLOCK, optionally VIR_DOMAIN_DISK_ACCESS_ALL to allow access to the disk for all users on the host system)
 *                  @networks [array]: array of network devices for domain, consists of keys as 'mac' (for MAC address), 'network' (for network name) and optional 'model' for model of NIC device
 *                  @flags [int]: bit array of flags
 * Returns:     a new domain resource
 */
PHP_FUNCTION(libvirt_domain_new)
{
    php_libvirt_connection *conn = NULL;
    php_libvirt_domain *res_domain = NULL;
    virDomainPtr domain = NULL;
    virDomainPtr domainUpdated = NULL;
    zval *zconn;
    char *arch = NULL;
    strsize_t arch_len;
    char *tmp;
    char *name;
    strsize_t name_len = 0;
    // char *emulator;
    char *iso_image = NULL;
    strsize_t iso_image_len;
    zend_long vcpus = -1;
    zend_long memMB = -1;
    zval *disks, *networks;
    tVMDisk *vmDisks = NULL;
    tVMNetwork *vmNetworks = NULL;
    zend_long maxmemMB = -1;
    HashTable *arr_hash;
    int numDisks, numNets, i;
    zval *data;
    HashPosition pointer;
    char vncl[2048] = { 0 };
    char *xml = NULL;
    char *hostname = NULL;
    int retval = 0;
    char uuid[VIR_UUID_STRING_BUFLEN] = { 0 };
    zend_long flags = 0;
    int fd = -1;

    GET_CONNECTION_FROM_ARGS("rsslllsaa|l", &zconn, &name, &name_len, &arch, &arch_len, &memMB, &maxmemMB, &vcpus, &iso_image, &iso_image_len, &disks, &networks, &flags);

    if (iso_image == NULL) {
        DPRINTF("%s: Iso image is not defined\n", PHPFUNC);
        RETURN_FALSE;
    }

    if ((arch == NULL) || (arch_len == 0))
        arch = NULL;

    //DPRINTF("%s: name: %s, arch: %s, memMB: %d, maxmemMB: %d, vcpus: %d, iso_image: %s\n", PHPFUNC, name, arch, memMB, maxmemMB, vcpus, iso_image);
    if (memMB == 0)
        memMB = maxmemMB;

    /* Parse all disks from array */
    arr_hash = Z_ARRVAL_P(disks);
    numDisks = zend_hash_num_elements(arr_hash);
    vmDisks = (tVMDisk *)malloc(numDisks * sizeof(tVMDisk));
    memset(vmDisks, 0, numDisks * sizeof(tVMDisk));
    i = 0;
    VIRT_FOREACH(arr_hash, pointer, data) {
        if (Z_TYPE_P(data) == IS_ARRAY) {
            tVMDisk disk;
            parse_array(data, &disk, NULL);
            if (disk.path != NULL) {
                //DPRINTF("Disk => path = '%s', driver = '%s', bus = '%s', dev = '%s', size = %ld MB, flags = %d\n",
                //  disk.path, disk.driver, disk.bus, disk.dev, disk.size, disk.flags);
                vmDisks[i++] = disk;
            }
        }
    } VIRT_FOREACH_END();
    numDisks = i;

    /* Parse all networks from array */
    arr_hash = Z_ARRVAL_P(networks);
    numNets = zend_hash_num_elements(arr_hash);
    vmNetworks = (tVMNetwork *)malloc(numNets * sizeof(tVMNetwork));
    memset(vmNetworks, 0, numNets * sizeof(tVMNetwork));
    i = 0;
    VIRT_FOREACH(arr_hash, pointer, data) {
        if (Z_TYPE_P(data) == IS_ARRAY) {
            tVMNetwork network;
            parse_array(data, NULL, &network);
            if (network.mac != NULL) {
                //DPRINTF("Network => mac = '%s', network = '%s', model = '%s'\n", network.mac, network.network, network.model);
                vmNetworks[i++] = network;
            }
        }
    } VIRT_FOREACH_END();
    numNets = i;

    tmp = installation_get_xml(conn->conn, name, memMB, maxmemMB,
                               NULL, NULL, vcpus, iso_image,
                               vmDisks, numDisks, vmNetworks, numNets,
                               flags TSRMLS_CC);
    if (tmp == NULL) {
        DPRINTF("%s: Cannot get installation XML\n", PHPFUNC);
        set_error("Cannot get installation XML" TSRMLS_CC);
        goto error;
    }

    domain = virDomainDefineXML(conn->conn, tmp);
    if (domain == NULL) {
        set_error_if_unset("Cannot define domain from the XML description" TSRMLS_CC);
        DPRINTF("%s: Cannot define domain from the XML description (%s): %s\n", PHPFUNC, LIBVIRT_G(last_error), tmp);
        goto error;
    }

    if (virDomainCreate(domain) < 0) {
        DPRINTF("%s: Cannot create domain: %s\n", PHPFUNC, LIBVIRT_G(last_error));
        set_error_if_unset("Cannot create domain" TSRMLS_CC);
        goto error;
    }

    xml = virDomainGetXMLDesc(domain, 0);
    if (!xml) {
        DPRINTF("%s: Cannot get the XML description: %s\n", PHPFUNC, LIBVIRT_G(last_error));
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        goto error;
    }

    if (virDomainGetUUIDString(domain, uuid) < 0) {
        DPRINTF("%s: Cannot get domain UUID: %s\n", PHPFUNC, LIBVIRT_G(last_error));
        set_error_if_unset("Cannot get domain UUID" TSRMLS_CC);
        goto error;
    }

    VIR_FREE(tmp);
    tmp = get_string_from_xpath(xml, "//domain/devices/graphics[@type='vnc']/@port", NULL, &retval);
    if (retval < 0) {
        DPRINTF("%s: Cannot get port from XML description\n", PHPFUNC);
        set_error_if_unset("Cannot get port from XML description" TSRMLS_CC);
        goto error;
    }
    VIR_FREE(xml);

    hostname = virConnectGetHostname(conn->conn);
    if (!hostname) {
        DPRINTF("%s: Cannot get hostname\n", PHPFUNC);
        set_error_if_unset("Cannot get hostname" TSRMLS_CC);
        goto error;
    }

    snprintf(vncl, sizeof(vncl), "%s:%s", hostname, tmp);
    DPRINTF("%s: Trying to connect to '%s'\n", PHPFUNC, vncl);

#ifndef EXTWIN
    if ((fd = connect_socket(hostname, tmp, 0, 0, flags & DOMAIN_FLAG_TEST_LOCAL_VNC)) < 0) {
        DPRINTF("%s: Cannot connect to '%s'\n", PHPFUNC, vncl);
        snprintf(vncl, sizeof(vncl), "Connection failed, port %s is most likely forbidden on firewall (iptables) on the host (%s)"
                 " or the emulator VNC server is bound to localhost address only.",
                 tmp, hostname);
    } else {
        close(fd);
        DPRINTF("%s: Connection to '%s' successfull (%s local connection)\n", PHPFUNC, vncl,
                (flags & DOMAIN_FLAG_TEST_LOCAL_VNC) ? "using" : "not using");
    }
#endif

    set_vnc_location(vncl TSRMLS_CC);

    VIR_FREE(tmp);
    tmp = installation_get_xml(conn->conn, name, memMB, maxmemMB,
                               NULL, uuid, vcpus, NULL,
                               vmDisks, numDisks, vmNetworks, numNets,
                               flags TSRMLS_CC);
    if (tmp == NULL) {
        DPRINTF("%s: Cannot get installation XML\n", PHPFUNC);
        set_error("Cannot get installation XML" TSRMLS_CC);
        goto error;
    }

    domainUpdated = virDomainDefineXML(conn->conn, tmp);
    if (domainUpdated == NULL) {
        set_error_if_unset("Cannot update domain definition" TSRMLS_CC);
        DPRINTF("%s: Cannot update domain definition "
                "(name = '%s', uuid = '%s', error = '%s')\n",
                PHPFUNC, name, uuid, LIBVIRT_G(last_error));
        goto error;
    }
    virDomainFree(domainUpdated);
    domainUpdated = NULL;

    res_domain = (php_libvirt_domain *)emalloc(sizeof(php_libvirt_domain));
    res_domain->domain = domain;
    res_domain->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_domain->domain);
    resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, res_domain->domain, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_domain, le_libvirt_domain);
    VIR_FREE(vmDisks);
    VIR_FREE(vmNetworks);
    VIR_FREE(tmp);
    VIR_FREE(hostname);
    return;

 error:
    if (domain) {
        if (virDomainIsActive(domain) > 0)
            virDomainDestroy(domain);
        virDomainUndefine(domain);
        virDomainFree(domain);
    }
    if (domainUpdated)
        virDomainFree(domainUpdated);
    VIR_FREE(vmDisks);
    VIR_FREE(vmNetworks);
    VIR_FREE(tmp);
    VIR_FREE(hostname);
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_domain_new_get_vnc
 * Since version:   0.4.5
 * Description:     Function is used to get the VNC server location for the newly created domain (newly started installation)
 * Arguments:       None
 * Returns:         a VNC server for a newly created domain resource (if any)
 */
PHP_FUNCTION(libvirt_domain_new_get_vnc)
{
    if (LIBVIRT_G(vnc_location))
        VIRT_RETURN_STRING(LIBVIRT_G(vnc_location));

    RETURN_NULL();
}

/*
 * Function name:   libvirt_domain_get_counts
 * Since version:   0.4.1(-1)
 * Description:     Function is getting domain counts for all, active and inactive domains
 * Arguments:       @conn [resource]: libvirt connection resource from libvirt_connect()
 * Returns:         array of total, active and inactive (but defined) domain counts
 */
PHP_FUNCTION(libvirt_domain_get_counts)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int count_defined;
    int count_active;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    count_defined = virConnectNumOfDefinedDomains(conn->conn);
    count_active  = virConnectNumOfDomains(conn->conn);

    array_init(return_value);
    add_assoc_long(return_value, "total", (long)(count_defined + count_active));
    add_assoc_long(return_value, "active", (long)count_active);
    add_assoc_long(return_value, "inactive", (long)count_defined);
}

/*
 * Function name:   libvirt_domain_is_persistent
 * Since version:   0.4.9
 * Description:     Function to get information whether domain is persistent or not
 * Arguments:       @res [resource]: libvirt domain resource
 * Returns:         TRUE for persistent, FALSE for not persistent, -1 on error
 */
PHP_FUNCTION(libvirt_domain_is_persistent)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int p;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    if ((p = virDomainIsPersistent(domain->domain)) < 0)
        RETURN_LONG(-1);

    if (p == 1)
        RETURN_TRUE;

    RETURN_FALSE;
}

/*
 * Function name:   libvirt_domain_lookup_by_name
 * Since version:   0.4.1(-1)
 * Description:     Function is used to lookup for domain by it's name
 * Arguments:       @res [resource]: libvirt connection resource from libvirt_connect()
 *                  @name [string]: domain name to look for
 * Returns:         libvirt domain resource
 */
PHP_FUNCTION(libvirt_domain_lookup_by_name)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    strsize_t name_len;
    char *name = NULL;
    virDomainPtr domain = NULL;
    php_libvirt_domain *res_domain;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &name, &name_len);
    if ((name == NULL) || (name_len < 1))
        RETURN_FALSE;
    domain = virDomainLookupByName(conn->conn, name);
    if (domain == NULL)
        RETURN_FALSE;

    res_domain = (php_libvirt_domain *)emalloc(sizeof(php_libvirt_domain));
    res_domain->domain = domain;
    res_domain->conn = conn;

    DPRINTF("%s: domain name = '%s', returning %p\n", PHPFUNC, name, res_domain->domain);
    resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, res_domain->domain, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_domain, le_libvirt_domain);
}

/*
 * Function name:   libvirt_domain_get_xml_desc
 * Since version:   0.4.1(-1), changed 0.4.2
 * Description:     Function is used to get the domain's XML description
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @xpath [string]: optional xPath expression string to get just this entry, can be NULL
 * Returns:         domain XML description string or result of xPath expression
 */
PHP_FUNCTION(libvirt_domain_get_xml_desc)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    char *tmp = NULL;
    char *xml;
    char *xpath = NULL;
    strsize_t xpath_len;
    zend_long flags = 0;
    int retval = -1;

    GET_DOMAIN_FROM_ARGS("rs|l", &zdomain, &xpath, &xpath_len, &flags);
    if (xpath_len < 1)
        xpath = NULL;

    DPRINTF("%s: Getting the XML for domain %p (xPath = %s)\n", PHPFUNC, domain->domain, xpath);

    xml = virDomainGetXMLDesc(domain->domain, flags);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        RETURN_FALSE;
    }

    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if ((tmp == NULL) || (retval < 0)) {
        VIRT_RETVAL_STRING(xml);
    } else {
        VIRT_RETVAL_STRING(tmp);
    }

    VIR_FREE(tmp);
    VIR_FREE(xml);
}

/*
 * Function name:   libvirt_domain_get_disk_devices
 * Since version:   0.4.4
 * Description:     Function is used to get disk devices for the domain
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         list of domain disk devices
 */
PHP_FUNCTION(libvirt_domain_get_disk_devices)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    char *xml;
    char *tmp;
    int retval = -1;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    DPRINTF("%s: Getting disk device list for domain %p\n", PHPFUNC, domain->domain);

    xml = virDomainGetXMLDesc(domain->domain, 0);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        RETURN_FALSE;
    }

    array_init(return_value);

    tmp = get_string_from_xpath(xml, "//domain/devices/disk/target/@dev", &return_value, &retval);
    VIR_FREE(tmp);
    VIR_FREE(xml);

    if (retval < 0)
        add_assoc_long(return_value, "error_code", (long)retval);
    else
        add_assoc_long(return_value, "num", (long)retval);
}

/*
 * Function name:   libvirt_domain_get_interface_devices
 * Since version:   0.4.4
 * Description:     Function is used to get network interface devices for the domain
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         list of domain interface devices
 */
PHP_FUNCTION(libvirt_domain_get_interface_devices)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    char *xml;
    char *tmp;
    int retval = -1;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    DPRINTF("%s: Getting interface device list for domain %p\n", PHPFUNC, domain->domain);

    xml = virDomainGetXMLDesc(domain->domain, 0);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        RETURN_FALSE;
    }

    array_init(return_value);

    tmp = get_string_from_xpath(xml, "//domain/devices/interface/target/@dev", &return_value, &retval);
    VIR_FREE(tmp);
    VIR_FREE(xml);

    if (retval < 0)
        add_assoc_long(return_value, "error_code", (long)retval);
    else
        add_assoc_long(return_value, "num", (long)retval);
}

/*
 * Function name:   libvirt_domain_change_vcpus
 * Since version:   0.4.2
 * Description:     Function is used to change the VCPU count for the domain
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @numCpus [int]: number of VCPUs to be set for the guest
 *                  @flags [int]: flags for virDomainSetVcpusFlags (available at http://libvirt.org/html/libvirt-libvirt.html#virDomainVcpuFlags)
 * Returns:         true on success, false on error
 */
PHP_FUNCTION(libvirt_domain_change_vcpus)
{
    zend_long numCpus, flags = 0;
    php_libvirt_domain *domain = NULL;
    zval *zdomain;

    GET_DOMAIN_FROM_ARGS("rl|l", &zdomain, &numCpus, &flags);

    if (virDomainSetVcpusFlags(domain->domain, numCpus, flags) == 0) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

/*
 * Function name:   libvirt_domain_change_memory
 * Since version:   0.4.2
 * Description:     Function is used to change the domain memory allocation
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @allocMem [int]: number of MiBs to be set as immediate memory value
 *                  @allocMax [int]: number of MiBs to be set as the maximum allocation
 *                  @flags [int]: flags
 * Returns:         new domain resource
 */
PHP_FUNCTION(libvirt_domain_change_memory)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    char *tmpA = NULL;
    char *tmp1 = NULL;
    char *tmp2 = NULL;
    char *xml;
    char *new_xml = NULL;
    int new_len;
    char newXml[512] = { 0 };
    zend_long xflags = 0;
    zend_long allocMem = 0;
    zend_long allocMax = 0;
    // int pos = -1;
    int len = 0;
    php_libvirt_domain *res_domain = NULL;
    php_libvirt_connection *conn   = NULL;
    virDomainPtr dom = NULL;

    GET_DOMAIN_FROM_ARGS("rll|l", &zdomain, &allocMem, &allocMax, &xflags);

    DPRINTF("%s: Changing domain memory count to %d MiB current/%d MiB max, domain = %p\n",
            PHPFUNC, (int)allocMem, (int)allocMax, domain->domain);

    allocMem *= 1024;
    allocMax *= 1024;

    if (allocMem > allocMax)
        allocMem = allocMax;

    xml = virDomainGetXMLDesc(domain->domain, xflags);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        RETURN_FALSE;
    }

    snprintf(newXml, sizeof(newXml), "  <memory>%d</memory>\n  <currentMemory>%d</currentMemory>\n", allocMax, allocMem);
    tmpA = strstr(xml, "<memory>");
    tmp1 = strstr(xml, "</currentMemory>") + strlen("</currentMemory>");
    // pos = strlen(xml) - strlen(tmp1);
    if (!tmpA || !tmp1) {
        set_error_if_unset("Cannot parse domain XML" TSRMLS_CC);
        RETURN_FALSE;
    }
    len = strlen(xml) - strlen(tmpA);

    tmp2 = (char *)emalloc((len + 1) * sizeof(char));
    memset(tmp2, 0, len + 1);
    memcpy(tmp2, xml, len);

    new_len = strlen(tmp1) + strlen(tmp2) + strlen(newXml) + 2;
    new_xml = (char *)emalloc(new_len * sizeof(char));
    snprintf(new_xml, new_len, "%s\n%s%s", tmp2, newXml, tmp1);

    conn = domain->conn;

    dom = virDomainDefineXML(conn->conn, new_xml);
    if (dom == NULL) {
        VIR_FREE(xml);
        efree(new_xml);
        RETURN_FALSE;
    }
    VIR_FREE(xml);
    efree(new_xml);

    res_domain = (php_libvirt_domain *)emalloc(sizeof(php_libvirt_domain));
    res_domain->domain = dom;
    res_domain->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_domain->domain);
    resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, res_domain->domain, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_domain, le_libvirt_domain);
}

/*
 * Function name:   libvirt_domain_change_boot_devices
 * Since version:   0.4.2
 * Description:     Function is used to change the domain boot devices
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @first [string]: first boot device to be set
 *                  @second [string]: second boot device to be set
 *                  @flags [int]: flags
 * Returns:         new domain resource
 */
PHP_FUNCTION(libvirt_domain_change_boot_devices)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    char *tmpA = NULL;
    char *tmp1 = NULL;
    char *tmp2 = NULL;
    char *xml;
    char *new_xml = NULL;
    int new_len;
    char newXml[4096] = { 0 };
    zend_long xflags = 0;
    char *first = NULL;
    strsize_t first_len;
    char *second = NULL;
    strsize_t second_len;
    // int pos = -1;
    int len = 0;
    php_libvirt_domain *res_domain = NULL;
    php_libvirt_connection *conn   = NULL;
    virDomainPtr dom = NULL;

    GET_DOMAIN_FROM_ARGS("rss|l", &zdomain, &first, &first_len, &second, &second_len, &xflags);

    xml = virDomainGetXMLDesc(domain->domain, xflags);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        RETURN_FALSE;
    }

    DPRINTF("%s: Changing domain boot order, domain = %p\n", PHPFUNC, domain->domain);

    if (!second || (strcmp(second, "-") == 0))
        snprintf(newXml, sizeof(newXml), "    <boot dev='%s'/>\n", first);
    else
        snprintf(newXml, sizeof(newXml), "    <boot dev='%s'/>\n    <boot dev='%s'/>\n", first, second);

    tmpA = strstr(xml, "</type>") + strlen("</type>");
    tmp1 = strstr(xml, "</os>");
    // pos = strlen(xml) - strlen(tmp1);
    len = strlen(xml) - strlen(tmpA);

    tmp2 = (char *)emalloc((len + 1) * sizeof(char));
    memset(tmp2, 0, len + 1);
    memcpy(tmp2, xml, len);

    new_len = strlen(tmp1) + strlen(tmp2) + strlen(newXml) + 2;
    new_xml = (char *)emalloc(new_len * sizeof(char));
    snprintf(new_xml, new_len, "%s\n%s%s", tmp2, newXml, tmp1);

    conn = domain->conn;

    dom = virDomainDefineXML(conn->conn, new_xml);
    if (dom == NULL) {
        DPRINTF("%s: Function failed, restoring original XML\n", PHPFUNC);
        VIR_FREE(xml);
        efree(newXml);
        RETURN_FALSE;
    }
    VIR_FREE(xml);
    efree(newXml);

    res_domain = (php_libvirt_domain *)emalloc(sizeof(php_libvirt_domain));
    res_domain->domain = dom;
    res_domain->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_domain->domain);
    resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, res_domain->domain, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_domain, le_libvirt_domain);
}

/*
 * Function name:   libvirt_domain_disk_add
 * Since version:   0.4.2
 * Description:     Function is used to add the disk to the virtual machine using set of API functions to make it as simple as possible for the user
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @img [string]: string for the image file on the host system
 *                  @dev [string]: string for the device to be presented to the guest (e.g. hda)
 *                  @typ [string]: bus type for the device in the guest, usually 'ide' or 'scsi'
 *                  @driver [string]: driver type to be specified, like 'raw' or 'qcow2'
 *                  @flags [int]: flags for getting the XML description
 * Returns:         new domain resource
 */
PHP_FUNCTION(libvirt_domain_disk_add)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    char *xml;
    char *img = NULL;
    strsize_t img_len;
    char *dev = NULL;
    strsize_t dev_len;
    char *driver = NULL;
    strsize_t driver_len;
    char *typ = NULL;
    strsize_t typ_len;
    char *newXml = NULL;
    zend_long xflags = 0;
    int retval = -1;
    char *xpath = NULL;
    char *tmp = NULL;

    GET_DOMAIN_FROM_ARGS("rssss|l", &zdomain, &img, &img_len, &dev, &dev_len, &typ, &typ_len, &driver, &driver_len, &xflags);

    DPRINTF("%s: Domain %p, device = %s, image = %s, type = %s, driver = %s\n", PHPFUNC,
            domain->domain, dev, img, typ, driver);

    xml = virDomainGetXMLDesc(domain->domain, xflags);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        goto error;
    }

    if (asprintf(&xpath, "//domain/devices/disk/source[@file=\"%s\"]/./@file", img) < 0) {
        set_error("Out of memory" TSRMLS_CC);
        goto error;
    }
    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if (tmp != NULL) {
        VIR_FREE(tmp);
        if (asprintf(&tmp, "Domain already has image <i>%s</i> connected", img) < 0)
            set_error("Out of memory" TSRMLS_CC);
        else
            set_error(tmp TSRMLS_CC);
        goto error;
    }

    VIR_FREE(xpath);
    if (asprintf(&xpath, "//domain/devices/disk/target[@dev='%s']/./@dev", dev) < 0) {
        set_error("Out of memory" TSRMLS_CC);
        goto error;
    }
    tmp = get_string_from_xpath(xml, newXml, NULL, &retval);
    if (tmp != NULL) {
        VIR_FREE(tmp);
        if (asprintf(&tmp, "Domain already has device <i>%s</i> connected", dev) < 0)
            set_error("Out of memory" TSRMLS_CC);
        else
            set_error(tmp TSRMLS_CC);
        goto error;
    }

    if (asprintf(&newXml,
                 "    <disk type='file' device='disk'>\n"
                 "      <driver name='qemu' type='%s'/>\n"
                 "      <source file='%s'/>\n"
                 "      <target dev='%s' bus='%s'/>\n"
                 "    </disk>", driver, img, dev, typ) < 0) {
        set_error("Out of memory" TSRMLS_CC);
        goto error;
    }

    if (virDomainAttachDeviceFlags(domain->domain,
                                   newXml, VIR_DOMAIN_AFFECT_CONFIG) < 0) {
        set_error("Unable to attach disk" TSRMLS_CC);
        goto error;
    }

    VIR_FREE(tmp);
    VIR_FREE(xpath);
    VIR_FREE(xml);
    RETURN_TRUE;

 error:
    VIR_FREE(tmp);
    VIR_FREE(xpath);
    VIR_FREE(xml);
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_domain_disk_remove
 * Since version:   0.4.2
 * Description:     Function is used to remove the disk from the virtual machine using set of API functions to make it as simple as possible
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @dev [string]: string for the device to be removed from the guest (e.g. 'hdb')
 *                  @flags [int]: flags for getting the XML description
 * Returns:         new domain resource
 */
PHP_FUNCTION(libvirt_domain_disk_remove)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    char *xml;
    char *dev = NULL;
    strsize_t dev_len;
    char *newXml = NULL;
    zend_long xflags = 0;
    int retval = -1;
    char *xpath = NULL;
    char *tmp = NULL;

    GET_DOMAIN_FROM_ARGS("rs|l", &zdomain, &dev, &dev_len, &xflags);

    DPRINTF("%s: Trying to remove %s from domain %p\n", PHPFUNC, dev, domain->domain);

    xml = virDomainGetXMLDesc(domain->domain, xflags);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        RETURN_FALSE;
    }

    if (asprintf(&xpath, "/domain/devices/disk[target/@dev='%s']", dev) < 0) {
        set_error("Out of memory" TSRMLS_CC);
        goto error;
    }
    newXml = get_node_string_from_xpath(xml, xpath);
    if (!newXml) {
        if (asprintf(&tmp, "Device <i>%s</i> is not connected to the guest", dev) < 0)
            set_error("Out of memory" TSRMLS_CC);
        else
            set_error(tmp TSRMLS_CC);
        goto error;
    }

    if (virDomainDetachDeviceFlags(domain->domain,
                                   newXml, VIR_DOMAIN_AFFECT_CONFIG) < 0) {
        set_error("Unable to detach disk" TSRMLS_CC);
        goto error;
    }

    VIR_FREE(tmp);
    VIR_FREE(xpath);
    VIR_FREE(xml);
    RETURN_TRUE;

 error:
    VIR_FREE(tmp);
    VIR_FREE(xpath);
    VIR_FREE(xml);
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_domain_nic_add
 * Since version:   0.4.2
 * Description:     Function is used to add the NIC card to the virtual machine using set of API functions to make it as simple as possible for the user
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @mac [string]: MAC string interpretation to be used for the NIC device
 *                  @network [string]: network name where to connect this NIC
 *                  @model [string]: string of the NIC model
 *                  @flags [int]: flags for getting the XML description
 * Returns:         new domain resource
 */
PHP_FUNCTION(libvirt_domain_nic_add)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    char *xml;
    char *mac = NULL;
    strsize_t mac_len;
    char *net = NULL;
    strsize_t net_len;
    char *model = NULL;
    strsize_t model_len;
    char *newXml = NULL;
    zend_long xflags = 0;
    int retval = -1;
    char *xpath = NULL;
    char *tmp = NULL;

    DPRINTF("%s: Entering\n", PHPFUNC);

    GET_DOMAIN_FROM_ARGS("rsss|l", &zdomain, &mac, &mac_len, &net, &net_len, &model, &model_len, &xflags);
    if (model_len < 1)
        model = NULL;

    DPRINTF("%s: domain = %p, mac = %s, net = %s, model = %s\n", PHPFUNC, domain->domain, mac, net, model);

    xml = virDomainGetXMLDesc(domain->domain, xflags);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        RETURN_FALSE;
    }

    if (asprintf(&xpath, "//domain/devices/interface[@type='network']/mac[@address='%s']/./@mac", mac) < 0) {
        set_error("Out of memory" TSRMLS_CC);
        goto error;
    }
    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if (tmp) {
        VIR_FREE(tmp);
        if (asprintf(&tmp, "Domain already has NIC device with MAC address <i>%s</i> connected", mac) < 0)
            set_error("Out of memory" TSRMLS_CC);
        else
            set_error(tmp TSRMLS_CC);
        goto error;
    }

    if (model) {
        if (asprintf(&newXml,
                     "   <interface type='network'>\n"
                     "       <mac address='%s' />\n"
                     "       <source network='%s' />\n"
                     "       <model type='%s' />\n"
                     "   </interface>", mac, net, model) < 0) {
            set_error("Out of memory" TSRMLS_CC);
            goto error;
        }
    } else {
        if (asprintf(&newXml,
                     "   <interface type='network'>\n"
                     "       <mac address='%s' />\n"
                     "       <source network='%s' />\n"
                     "   </interface>", mac, net) < 0) {
            set_error("Out of memory" TSRMLS_CC);
            goto error;
        }
    }

    if (virDomainAttachDeviceFlags(domain->domain,
                                   newXml, VIR_DOMAIN_AFFECT_CONFIG) < 0) {
        set_error("Unable to attach interface" TSRMLS_CC);
        goto error;
    }

    VIR_FREE(tmp);
    VIR_FREE(xpath);
    VIR_FREE(xml);
    RETURN_TRUE;

 error:
    VIR_FREE(tmp);
    VIR_FREE(xpath);
    VIR_FREE(xml);
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_domain_nic_remove
 * Since version:   0.4.2
 * Description:     Function is used to remove the NIC from the virtual machine using set of API functions to make it as simple as possible
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @dev [string]: string representation of the IP address to be removed (e.g. 54:52:00:xx:yy:zz)
 *                  @flags [int]: optional flags for getting the XML description
 * Returns:         new domain resource
 */
PHP_FUNCTION(libvirt_domain_nic_remove)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    char *xml;
    char *mac = NULL;
    strsize_t mac_len;
    char *newXml = NULL;
    zend_long xflags = 0;
    int retval = -1;
    char *xpath = NULL;
    char *tmp = NULL;

    GET_DOMAIN_FROM_ARGS("rs|l", &zdomain, &mac, &mac_len, &xflags);

    DPRINTF("%s: Trying to remove NIC device with MAC address %s from domain %p\n", PHPFUNC, mac, domain->domain);

    xml = virDomainGetXMLDesc(domain->domain, xflags);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        RETURN_FALSE;
    }
    if (asprintf(&xpath, "//domain/devices/interface[@type='network']/mac[@address='%s']/./@mac", mac) < 0) {
        set_error("Out of memory" TSRMLS_CC);
        goto error;
    }
    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if (!tmp) {
        VIR_FREE(tmp);
        if (asprintf(&tmp, "Domain has no such interface with mac %s", mac) < 0)
            set_error("Out of memory" TSRMLS_CC);
        else
            set_error(tmp TSRMLS_CC);
        goto error;
    }

    if (asprintf(&newXml,
                 "   <interface type='network'>\n"
                 "       <mac address='%s' />\n"
                 "   </interface>", mac) < 0) {
        set_error("Out of memory" TSRMLS_CC);
        goto error;
    }

    if (virDomainDetachDeviceFlags(domain->domain,
                                   newXml, VIR_DOMAIN_AFFECT_CONFIG) < 0) {
        set_error("Unable to detach interface" TSRMLS_CC);
        goto error;
    }

    VIR_FREE(tmp);
    VIR_FREE(xpath);
    VIR_FREE(xml);
    RETURN_TRUE;

 error:
    VIR_FREE(tmp);
    VIR_FREE(xpath);
    VIR_FREE(xml);
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_domain_attach_device
 * Since version:   0.5.3
 * Description:     Function is used to attach a virtual device to a domain.
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @xml [string]: XML description of one device.
 *                  @flags [int]: optional flags to control how the device is attached. Defaults to VIR_DOMAIN_AFFECT_LIVE
 * Returns:         TRUE for success, FALSE on error.
 */
PHP_FUNCTION(libvirt_domain_attach_device)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain = NULL;
    char *xml = NULL;
    strsize_t xml_len = 0;
    zend_long flags = VIR_DOMAIN_AFFECT_LIVE;

    GET_DOMAIN_FROM_ARGS("rs|l", &zdomain, &xml, &xml_len, &flags);

    if (virDomainAttachDeviceFlags(domain->domain, xml, flags) < 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_detach_device
 * Since version:   0.5.3
 * Description:     Function is used to detach a virtual device from a domain.
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @xml [string]: XML description of one device.
 *                  @flags [int]: optional flags to control how the device is attached. Defaults to VIR_DOMAIN_AFFECT_LIVE
 * Returns:         TRUE for success, FALSE on error.
 */
PHP_FUNCTION(libvirt_domain_detach_device)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain = NULL;
    char *xml = NULL;
    strsize_t xml_len = 0;
    zend_long flags = VIR_DOMAIN_AFFECT_LIVE;

    GET_DOMAIN_FROM_ARGS("rs|l", &zdomain, &xml, &xml_len, &flags);

    if (virDomainDetachDeviceFlags(domain->domain, xml, flags) < 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_get_info
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the domain's information
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         domain information array
 */
PHP_FUNCTION(libvirt_domain_get_info)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    virDomainInfo domainInfo;
    int retval;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    retval = virDomainGetInfo(domain->domain, &domainInfo);
    DPRINTF("%s: virDomainGetInfo(%p) returned %d\n", PHPFUNC, domain->domain, retval);
    if (retval != 0)
        RETURN_FALSE;

    array_init(return_value);
    add_assoc_long(return_value, "maxMem", domainInfo.maxMem);
    add_assoc_long(return_value, "memory", domainInfo.memory);
    add_assoc_long(return_value, "state", (long)domainInfo.state);
    add_assoc_long(return_value, "nrVirtCpu", domainInfo.nrVirtCpu);
    add_assoc_double(return_value, "cpuUsed", (double)((double)domainInfo.cpuTime/1000000000.0));
}

/*
 * Function name:   libvirt_domain_get_name
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get domain name from it's resource
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         domain name string
 */
PHP_FUNCTION(libvirt_domain_get_name)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    const char *name = NULL;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    if (domain->domain == NULL)
        RETURN_FALSE;

    name = virDomainGetName(domain->domain);
    DPRINTF("%s: virDomainGetName(%p) returned %s\n", PHPFUNC, domain->domain, name);
    if (name == NULL)
        RETURN_FALSE;

    VIRT_RETURN_STRING(name);  //we can use the copy mechanism as we need not to free name (we even can not!)
}

/*
 * Function name:   libvirt_domain_get_uuid
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the domain's UUID in binary format
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         domain UUID in binary format
 */
PHP_FUNCTION(libvirt_domain_get_uuid)
{

    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    char *uuid;
    int retval;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    uuid = (char *)emalloc(VIR_UUID_BUFLEN);
    retval = virDomainGetUUID(domain->domain, (unsigned char *)uuid);
    DPRINTF("%s: virDomainGetUUID(%p, %p) returned %d\n", PHPFUNC, domain->domain, uuid, retval);
    if (retval != 0)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(uuid);
    efree(uuid);
}

/*
 * Function name:   libvirt_domain_get_uuid_string
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the domain's UUID in string format
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         domain UUID string
 */
PHP_FUNCTION(libvirt_domain_get_uuid_string)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    char *uuid;
    int retval;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    uuid = (char *)emalloc(VIR_UUID_STRING_BUFLEN);
    retval = virDomainGetUUIDString(domain->domain, uuid);
    DPRINTF("%s: virDomainGetUUIDString(%p) returned %d (%s)\n", PHPFUNC, domain->domain, retval, uuid);
    if (retval != 0)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(uuid);
    efree(uuid);
}

/*
 * Function name:   libvirt_domain_get_id
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the domain's ID, applicable to running guests only
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         running domain ID or -1 if not running
 */
PHP_FUNCTION(libvirt_domain_get_id)
{

    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    retval = virDomainGetID(domain->domain);
    DPRINTF("%s: virDomainGetID(%p) returned %d\n", PHPFUNC, domain->domain, retval);

    RETURN_LONG(retval);
}

/*
 * Function name:   libvirt_domain_lookup_by_uuid
 * Since version:   0.4.1(-1)
 * Description:     Function is used to lookup for domain by it's UUID in the binary format
 * Arguments:       @res [resource]: libvirt connection resource from libvirt_connect()
 *                  @uuid [string]: binary defined UUID to look for
 * Returns:         libvirt domain resource
 */
PHP_FUNCTION(libvirt_domain_lookup_by_uuid)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    strsize_t uuid_len;
    unsigned char *uuid = NULL;
    virDomainPtr domain = NULL;
    php_libvirt_domain *res_domain;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &uuid, &uuid_len);

    if ((uuid == NULL) || (uuid_len < 1))
        RETURN_FALSE;
    domain = virDomainLookupByUUID(conn->conn, uuid);
    if (domain == NULL)
        RETURN_FALSE;

    res_domain = (php_libvirt_domain *)emalloc(sizeof(php_libvirt_domain));
    res_domain->domain = domain;
    res_domain->conn = conn;

    DPRINTF("%s: domain UUID = '%s', returning %p\n", PHPFUNC, uuid, res_domain->domain);
    resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, res_domain->domain, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_domain, le_libvirt_domain);
}

/*
 * Function name:   libvirt_domain_lookup_by_uuid_string
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the domain by it's UUID that's accepted in string format
 * Arguments:       @res [resource]: libvirt connection resource from libvirt_connect()
 *                  @uuid [string]: domain UUID [in string format] to look for
 * Returns:         libvirt domain resource
 */
PHP_FUNCTION(libvirt_domain_lookup_by_uuid_string)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    strsize_t uuid_len;
    char *uuid = NULL;
    virDomainPtr domain = NULL;
    php_libvirt_domain *res_domain;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &uuid, &uuid_len);

    if ((uuid == NULL) || (uuid_len < 1))
        RETURN_FALSE;
    domain = virDomainLookupByUUIDString(conn->conn, uuid);
    if (domain == NULL)
        RETURN_FALSE;

    res_domain = (php_libvirt_domain *)emalloc(sizeof(php_libvirt_domain));
    res_domain->domain = domain;
    res_domain->conn = conn;

    DPRINTF("%s: domain UUID string = '%s', returning %p\n", PHPFUNC, uuid, res_domain->domain);
    resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, res_domain->domain, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_domain, le_libvirt_domain);
}

/*
 * Function name:   libvirt_domain_lookup_by_id
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get domain by it's ID, applicable only to running guests
 * Arguments:       @conn [resource]: libvirt connection resource from libvirt_connect()
 *                  @id   [string]: domain id to look for
 * Returns:         libvirt domain resource
 */
PHP_FUNCTION(libvirt_domain_lookup_by_id)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    zend_long id;
    virDomainPtr domain = NULL;
    php_libvirt_domain *res_domain;

    GET_CONNECTION_FROM_ARGS("rl", &zconn, &id);

    domain = virDomainLookupByID(conn->conn, (int)id);
    if (domain == NULL)
        RETURN_FALSE;

    res_domain = (php_libvirt_domain *)emalloc(sizeof(php_libvirt_domain));
    res_domain->domain = domain;
    res_domain->conn = conn;

    DPRINTF("%s: domain id = '%d', returning %p\n", PHPFUNC, (int)id, res_domain->domain);
    resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, res_domain->domain, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_domain, le_libvirt_domain);
}

/*
 * Function name:   libvirt_domain_create
 * Since version:   0.4.1(-1)
 * Description:     Function is used to create the domain identified by it's resource
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         result of domain creation (startup)
 */
PHP_FUNCTION(libvirt_domain_create)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    retval = virDomainCreate(domain->domain);
    DPRINTF("%s: virDomainCreate(%p) returned %d\n", PHPFUNC, domain->domain, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_destroy
 * Since version:   0.4.1(-1)
 * Description:     Function is used to destroy the domain identified by it's resource
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         result of domain destroy
 */
PHP_FUNCTION(libvirt_domain_destroy)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    retval = virDomainDestroy(domain->domain);
    DPRINTF("%s: virDomainDestroy(%p) returned %d\n", PHPFUNC, domain->domain, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_resume
 * Since version:   0.4.1(-1)
 * Description:     Function is used to resume the domain identified by it's resource
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         result of domain resume
 */
PHP_FUNCTION(libvirt_domain_resume)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    retval = virDomainResume(domain->domain);
    DPRINTF("%s: virDomainResume(%p) returned %d\n", PHPFUNC, domain->domain, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_core_dump
 * Since version:   0.4.1(-2)
 * Description:     Function is used to dump core of the domain identified by it's resource
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @to [string]: to
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_core_dump)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    strsize_t to_len;
    char *to;

    GET_DOMAIN_FROM_ARGS("rs", &zdomain, &to, &to_len);

    retval = virDomainCoreDump(domain->domain, to, 0);
    DPRINTF("%s: virDomainCoreDump(%p, %s, 0) returned %d\n", PHPFUNC, domain->domain, to, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_shutdown
 * Since version:   0.4.1(-1)
 * Description:     Function is used to shutdown the domain identified by it's resource
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_shutdown)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    retval = virDomainShutdown(domain->domain);
    DPRINTF("%s: virDomainShutdown(%p) returned %d\n", PHPFUNC, domain->domain, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_suspend
 * Since version:   0.4.1(-1)
 * Description:     Function is used to suspend the domain identified by it's resource
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_suspend)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    retval = virDomainSuspend(domain->domain);
    DPRINTF("%s: virDomainSuspend(%p) returned %d\n", PHPFUNC, domain->domain, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_managedsave
 * Since version:   0.4.1(-1)
 * Description:     Function is used to managed save the domain (domain was unloaded from memory and it state saved to disk) identified by it's resource
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_managedsave)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);
    retval = virDomainManagedSave(domain->domain, 0);
    DPRINTF("%s: virDomainManagedSave(%p) returned %d\n", PHPFUNC, domain->domain, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_undefine
 * Since version:   0.4.1(-1)
 * Description:     Function is used to undefine the domain identified by it's resource
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_undefine)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    retval = virDomainUndefine(domain->domain);
    DPRINTF("%s: virDomainUndefine(%p) returned %d\n", PHPFUNC, domain->domain, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_reboot
 * Since version:   0.4.1(-1)
 * Description:     Function is used to reboot the domain identified by it's resource
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @flags [int]: optional flags
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_reboot)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    zend_long flags = 0;

    GET_DOMAIN_FROM_ARGS("r|l", &zdomain, &flags);

    retval = virDomainReboot(domain->domain, flags);
    DPRINTF("%s: virDomainReboot(%p) returned %d\n", PHPFUNC, domain->domain, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_define_xml
 * Since version:   0.4.1(-1)
 * Description:     Function is used to define the domain from XML string
 * Arguments:       @conn [resource]: libvirt connection resource
 *                  @xml [string]: XML string to define guest from
 * Returns:         newly defined domain resource
 */
PHP_FUNCTION(libvirt_domain_define_xml)
{
    php_libvirt_domain *res_domain = NULL;
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    virDomainPtr domain = NULL;
    char *xml;
    strsize_t xml_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &xml, &xml_len);

    domain = virDomainDefineXML(conn->conn, xml);
    if (domain == NULL)
        RETURN_FALSE;

    res_domain = (php_libvirt_domain *)emalloc(sizeof(php_libvirt_domain));
    res_domain->domain = domain;
    res_domain->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_domain->domain);
    resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, res_domain->domain, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_domain, le_libvirt_domain);
}

/*
 * Function name:   libvirt_domain_create_xml
 * Since version:   0.4.1(-1)
 * Description:     Function is used to create the domain identified by it's resource
 * Arguments:       @conn [resource]: libvirt connection resource
 *                  @xml [string]: XML string to create guest from
 * Returns:         newly started/created domain resource
 */
PHP_FUNCTION(libvirt_domain_create_xml)
{
    php_libvirt_domain *res_domain = NULL;
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    virDomainPtr domain = NULL;
    char *xml;
    strsize_t xml_len;
    zend_long flags = 0;

    GET_CONNECTION_FROM_ARGS("rs|l", &zconn, &xml, &xml_len, &flags);

    domain = virDomainCreateXML(conn->conn, xml, flags);
    DPRINTF("%s: virDomainCreateXML(%p, <xml>, 0) returned %p\n", PHPFUNC, conn->conn, domain);
    if (domain == NULL)
        RETURN_FALSE;

    res_domain = (php_libvirt_domain *)emalloc(sizeof(php_libvirt_domain));
    res_domain->domain = domain;
    res_domain->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_domain->domain);
    resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, res_domain->domain, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_domain, le_libvirt_domain);
}

/*
 * Function name:   libvirt_domain_xml_from_native
 * Since version:   0.5.3
 * Description:     Function is used to convert native configuration data to libvirt domain XML
 * Arguments:       @conn [resource]: libvirt connection resource
 *                  @format [string]: configuration format converting from
 *                  @config_data [string]: content of the native config file
 * Returns:         libvirt domain XML, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_xml_from_native)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    char *config_data = NULL;
    char *format = NULL;
    char *xml = NULL;
    strsize_t config_data_len;
    strsize_t format_len;
    unsigned int flags = 0;

    GET_CONNECTION_FROM_ARGS("rss", &zconn, &format, &format_len, &config_data, &config_data_len);

    xml = virConnectDomainXMLFromNative(conn->conn, format, config_data, flags);

    if (xml == NULL) {
        set_error_if_unset("Cannot convert native format to XML" TSRMLS_CC);
        RETURN_FALSE;
    }

    VIRT_RETVAL_STRING(xml);
    VIR_FREE(xml);
}

/*
 * Function name:   libvirt_domain_xml_to_native
 * Since version:   0.5.3
 * Description:     Function is used to convert libvirt domain XML to native configuration
 * Arguments:       @conn [resource]: libvirt connection resource
 *                  @format [string]: configuration format converting to
 *                  @xml_data [string]: content of the libvirt domain xml file
 * Returns:         contents of the native data file, FALSE on error
*/
PHP_FUNCTION(libvirt_domain_xml_to_native)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    char *xml_data = NULL;
    char *format  = NULL;
    char *config_data = NULL;
    strsize_t xml_data_len;
    strsize_t format_len;
    unsigned int flags = 0;

    GET_CONNECTION_FROM_ARGS("rss", &zconn, &format, &format_len, &xml_data, &xml_data_len);

    config_data = virConnectDomainXMLToNative(conn->conn, format, xml_data, flags);

    if (config_data == NULL) {
        set_error_if_unset("Cannot convert to native format from XML" TSRMLS_CC);
        RETURN_FALSE;
    }

    VIRT_RETVAL_STRING(config_data);
    VIR_FREE(config_data);
}

/*
 * Function name:   libvirt_domain_set_max_memory
 * Since version:   0.5.1
 * Description:     Function to set max memory for domain
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @memory [int]: memory size in 1024 bytes (Kb)
 * Returns:         TRUE for success, FALSE for failure
 */
PHP_FUNCTION(libvirt_domain_set_max_memory)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    zend_long memory = 0;

    GET_DOMAIN_FROM_ARGS("rl", &zdomain, &memory);

    if (virDomainSetMaxMemory(domain->domain, memory) != 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_set_memory
 * Since version:   0.5.1
 * Description:     Function to set memory for domain
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @memory [int]: memory size in 1024 bytes (Kb)
 * Returns:         TRUE for success, FALSE for failure
 */
PHP_FUNCTION(libvirt_domain_set_memory)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    zend_long memory = 0;

    GET_DOMAIN_FROM_ARGS("rl", &zdomain, &memory);

    if (virDomainSetMemory(domain->domain, memory) != 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_set_memory_flags
 * Since version:   0.5.1
 * Description:     Function to set max memory for domain
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @memory [int]: memory size in 1024 bytes (Kb)
 *                  @flags [int]: bitwise-OR VIR_DOMAIN_MEM_* flags
 * Returns:         TRUE for success, FALSE for failure
 */
PHP_FUNCTION(libvirt_domain_set_memory_flags)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    zend_long memory = 0;
    zend_long flags = 0;

    GET_DOMAIN_FROM_ARGS("rl|l", &zdomain, &memory, &flags);

    if (virDomainSetMemoryFlags(domain->domain, memory, flags) != 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_memory_peek
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the domain's memory peek value
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @start [int]: start
 *                  @size [int]: size
 *                  @flags [int]: optional flags
 * Returns:         domain memory peek
 */
PHP_FUNCTION(libvirt_domain_memory_peek)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    zend_long flags = 0;
    zend_long start;
    zend_long size;
    char *buff;

    GET_DOMAIN_FROM_ARGS("rlll", &zdomain, &start, &size, &flags);
    if (start < 0) {
        set_error("Negative argument start" TSRMLS_CC);
        RETURN_FALSE;
    }
    buff = (char *)emalloc(size);
    retval = virDomainMemoryPeek(domain->domain, start, size, buff, flags);
    if (retval != 0)
        RETURN_FALSE;
    VIRT_RETVAL_STRINGL(buff, size);
    efree(buff);
}

/*
 * Function name:   libvirt_domain_memory_stats
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the domain's memory stats
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @flags [int]: optional flags
 * Returns:         domain memory stats array (same fields as virDomainMemoryStats, please see libvirt documentation)
 */
PHP_FUNCTION(libvirt_domain_memory_stats)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    zend_long flags = 0;
    int i;
    struct _virDomainMemoryStat stats[VIR_DOMAIN_MEMORY_STAT_NR];

    GET_DOMAIN_FROM_ARGS("r|l", &zdomain, &flags);

    retval = virDomainMemoryStats(domain->domain, stats, VIR_DOMAIN_MEMORY_STAT_NR, flags);
    DPRINTF("%s: virDomainMemoryStats(%p...) returned %d\n", PHPFUNC, domain->domain, retval);

    if (retval == -1)
        RETURN_FALSE;
    LONGLONG_INIT;
    array_init(return_value);
    for (i = 0; i < retval; i++)
        LONGLONG_INDEX(return_value, stats[i].tag, stats[i].val)
}

/*
 * Function name:   libvirt_domain_block_commit
 * Since version:   0.5.2(-1)
 * Description:     Function is used to commit block job
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @disk [string]: path to the block device, or device shorthand
 *                  @base [string]: path to backing file to merge into, or device shorthand, or NULL for default
 *                  @top [string]: path to file within backing chain that contains data to be merged, or device shorthand, or NULL to merge all possible data
 *                  @bandwidth [int]: (optional) specify bandwidth limit; flags determine the unit
 *                  @flags [int]: bitwise-OR of VIR_DOMAIN_BLOCK_COMMIT_*
 * Returns:         true on success fail on error
 */
PHP_FUNCTION(libvirt_domain_block_commit)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    char *disk = NULL;
    strsize_t disk_len;
    char *base = NULL;
    strsize_t base_len;
    char *top = NULL;
    strsize_t top_len;
    zend_long bandwidth = 0;
    zend_long flags = 0;

    GET_DOMAIN_FROM_ARGS("rs|ssll", &zdomain, &disk, &disk_len, &base, &base_len, &top, &top_len, &bandwidth, &flags);
    if (strcmp(disk, "") == 0)
        RETURN_FALSE;
    if (strcmp(base, "") == 0)
        base = NULL;
    if (strcmp(top, "") == 0)
        top = NULL;

    retval = virDomainBlockCommit(domain->domain, disk, base, top, bandwidth, flags);
    if (retval == -1)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_block_stats
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the domain's block stats
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @path [string]: device path to get statistics about
 * Returns:         domain block stats array, fields are rd_req, rd_bytes, wr_req, wr_bytes and errs
 */
PHP_FUNCTION(libvirt_domain_block_stats)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    char *path;
    strsize_t path_len;

    struct _virDomainBlockStats stats;

    GET_DOMAIN_FROM_ARGS("rs", &zdomain, &path, &path_len);

    retval = virDomainBlockStats(domain->domain, path, &stats, sizeof stats);
    DPRINTF("%s: virDomainBlockStats(%p, %s, <stats>, <size>) returned %d\n", PHPFUNC, domain->domain, path, retval);
    if (retval == -1)
        RETURN_FALSE;

    array_init(return_value);
    LONGLONG_INIT;
    LONGLONG_ASSOC(return_value, "rd_req", stats.rd_req);
    LONGLONG_ASSOC(return_value, "rd_bytes", stats.rd_bytes);
    LONGLONG_ASSOC(return_value, "wr_req", stats.wr_req);
    LONGLONG_ASSOC(return_value, "wr_bytes", stats.wr_bytes);
    LONGLONG_ASSOC(return_value, "errs", stats.errs);
}

/*
 * Function name:   libvirt_domain_block_resize
 * Since version:   0.5.1(-1)
 * Description:     Function is used to resize the domain's block device
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @path [string]: device path to resize
 *                  @size [int]: size of device
 *                  @flags [int]: bitwise-OR of VIR_DOMAIN_BLOCK_RESIZE_*
 * Returns:         true on success fail on error
 */
PHP_FUNCTION(libvirt_domain_block_resize)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    char *path;
    strsize_t path_len;
    zend_long size = 0;
    zend_long flags = 0;

    GET_DOMAIN_FROM_ARGS("rsl|l", &zdomain, &path, &path_len, &size, &flags);

    retval = virDomainBlockResize(domain->domain, path, size, flags);
    if (retval == -1)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_block_job_info
 * Since version:   0.5.2(-1)
 * Description:     Function is used to request block job information for the given disk
 * Arguments:       @dom [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @disk [string]: path to the block device, or device shorthand
 *                  @flags [int]: bitwise-OR of VIR_DOMAIN_BLOCK_COMMIT_*
 * Returns:         Array with status virDomainGetBlockJobInfo and blockjob information.
 */
PHP_FUNCTION(libvirt_domain_block_job_info)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    char *disk;
    int disk_len;
    long flags = 0;
    virDomainBlockJobInfo info;

    GET_DOMAIN_FROM_ARGS("rs|l", &zdomain, &disk, &disk_len, &flags);

    retval = virDomainGetBlockJobInfo(domain->domain, disk, &info, flags);

    array_init(return_value);
    add_assoc_long(return_value, "status", (int)retval);
    add_assoc_long(return_value, "type", (int)info.type);
    add_assoc_long(return_value, "bandwidth", (unsigned long)info.bandwidth);
    add_assoc_long(return_value, "cur", (unsigned long long)info.cur);
    add_assoc_long(return_value, "end", (unsigned long long)info.end);
}


/*
 * Function name:   libvirt_domain_block_job_abort
 * Since version:   0.5.1(-1)
 * Description:     Function is used to abort block job
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @path [string]: device path to resize
 *                  @flags [int]: bitwise-OR of VIR_DOMAIN_BLOCK_JOB_ABORT_*
 * Returns:         true on success fail on error
 */
PHP_FUNCTION(libvirt_domain_block_job_abort)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    char *path;
    strsize_t path_len;
    zend_long flags = 0;

    GET_DOMAIN_FROM_ARGS("rs|l", &zdomain, &path, &path_len, &flags);

    retval = virDomainBlockJobAbort(domain->domain, path, flags);
    if (retval == -1)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_block_job_set_speed
 * Since version:   0.5.1(-1)
 * Description:     Function is used to set speed of block job
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @path [string]: device path to resize
 *                  @bandwidth [int]: bandwidth
 *                  @flags [int]: bitwise-OR of VIR_DOMAIN_BLOCK_JOB_SPEED_BANDWIDTH_*
 * Returns:         true on success fail on error
 */
PHP_FUNCTION(libvirt_domain_block_job_set_speed)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    char *path;
    strsize_t path_len;
    zend_long bandwidth = 0;
    zend_long flags = 0;

    GET_DOMAIN_FROM_ARGS("rsl|l", &zdomain, &path, &path_len, &bandwidth, &flags);

    retval = virDomainBlockJobSetSpeed(domain->domain, path, bandwidth, flags);
    if (retval == -1)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_interface_addresses
 * Since version:   0.5.5
 * Description:     Function is used to get network interface addresses for the domain
 * Arguments:       @domain [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @source [int]: one of the VIR_DOMAIN_ADDRESSES_SRC_* flags.
 * Returns:         interface array of a domain holding information about addresses resembling the virDomainInterface structure, false on error
 */
PHP_FUNCTION(libvirt_domain_interface_addresses)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    zend_long source = 0;

    virDomainInterfacePtr *ifaces = NULL;
    int count = 0;
    size_t i, j;

    GET_DOMAIN_FROM_ARGS("rl", &zdomain, &source);

    if ((count = virDomainInterfaceAddresses(domain->domain, &ifaces, source, 0)) < 0) {
        RETURN_FALSE
        goto cleanup;
    }

    array_init(return_value);

    for (i = 0; i < count; i++) {
        zval *iface;
        VIRT_ARRAY_INIT(iface);
        VIRT_ADD_ASSOC_STRING(iface, "name", ifaces[i]->name);
        VIRT_ADD_ASSOC_STRING(iface, "hwaddr", ifaces[i]->hwaddr);
        add_assoc_long(iface, "naddrs", ifaces[i]->naddrs);

        for (j = 0; j < ifaces[i]->naddrs; j++) {
            zval *ifaddr;
            VIRT_ARRAY_INIT(ifaddr);
            VIRT_ADD_ASSOC_STRING(ifaddr, "addr", ifaces[i]->addrs[j].addr);
            add_assoc_long(ifaddr, "prefix", ifaces[i]->addrs[j].prefix);
            add_assoc_long(ifaddr, "type", ifaces[i]->addrs[j].type);

            add_assoc_zval(iface, "addrs", ifaddr);
        }

        add_index_zval(return_value, i, iface);
    }

 cleanup:
    if (ifaces && count > 0) {
        for (i = 0; i < count; i++)
            virDomainInterfaceFree(ifaces[i]);
    }
    VIR_FREE(ifaces);
}

/*
 * Function name:   libvirt_domain_interface_stats
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the domain's interface stats
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @path [string]: path to interface device
 * Returns:         interface stats array of {tx|rx}_{bytes|packets|errs|drop} fields
 */
PHP_FUNCTION(libvirt_domain_interface_stats)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    char *path;
    strsize_t path_len;

    struct _virDomainInterfaceStats stats;

    GET_DOMAIN_FROM_ARGS("rs", &zdomain, &path, &path_len);

    retval = virDomainInterfaceStats(domain->domain, path, &stats, sizeof stats);
    DPRINTF("%s: virDomainInterfaceStats(%p, %s, <stats>, <size>) returned %d\n", PHPFUNC, domain->domain, path, retval);
    if (retval == -1)
        RETURN_FALSE;

    array_init(return_value);
    LONGLONG_INIT;
    LONGLONG_ASSOC(return_value, "rx_bytes", stats.rx_bytes);
    LONGLONG_ASSOC(return_value, "rx_packets", stats.rx_packets);
    LONGLONG_ASSOC(return_value, "rx_errs", stats.rx_errs);
    LONGLONG_ASSOC(return_value, "rx_drop", stats.rx_drop);
    LONGLONG_ASSOC(return_value, "tx_bytes", stats.tx_bytes);
    LONGLONG_ASSOC(return_value, "tx_packets", stats.tx_packets);
    LONGLONG_ASSOC(return_value, "tx_errs", stats.tx_errs);
    LONGLONG_ASSOC(return_value, "tx_drop", stats.tx_drop);
}

/*
 * Function name:   libvirt_domain_get_connect
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the domain's connection resource. This function should *not* be used!
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         libvirt connection resource
 */
PHP_FUNCTION(libvirt_domain_get_connect)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    php_libvirt_connection *conn;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    conn = domain->conn;
    if (conn->conn == NULL)
        RETURN_FALSE;

    VIRT_RETURN_RESOURCE(conn->resource);
    /* since we're returning already registered resource, bump refcount */
    Z_ADDREF_P(return_value);
}

/*
 * Function name:   libvirt_domain_migrate
 * Since version:   0.4.1(-1)
 * Description:     Function is used migrate domain to another domain
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @dest_conn [string]: destination host connection object
 *                  @flags [int]: migration flags
 *                  @dname [string]: domain name to rename domain to on destination side
 *                  @bandwidth [int]: migration bandwidth in Mbps
 * Returns:         libvirt domain resource for migrated domain
 */
PHP_FUNCTION(libvirt_domain_migrate)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    php_libvirt_connection *dconn = NULL;
    zval *zdconn;
    virDomainPtr destdomain = NULL;
    php_libvirt_domain *res_domain;

    zend_long flags = 0;
    char *dname;
    strsize_t dname_len;
    zend_long bandwidth;

    dname = NULL;
    dname_len = 0;
    bandwidth = 0;

    GET_DOMAIN_FROM_ARGS("rrl|sl", &zdomain, &zdconn, &flags, &dname, &dname_len, &bandwidth);

    if ((domain->domain == NULL) || (domain->conn->conn == NULL)) {
        set_error("Domain object is not valid" TSRMLS_CC);
        RETURN_FALSE;
    }

    VIRT_FETCH_RESOURCE(dconn, php_libvirt_connection*, &zdconn, PHP_LIBVIRT_CONNECTION_RES_NAME, le_libvirt_connection);
    if ((dconn == NULL) || (dconn->conn == NULL)) {
        set_error("Destination connection object is not valid" TSRMLS_CC);
        RETURN_FALSE;
    }

    destdomain = virDomainMigrate(domain->domain, dconn->conn, flags, dname, NULL, bandwidth);
    if (destdomain == NULL)
        RETURN_FALSE;

    res_domain = (php_libvirt_domain *)emalloc(sizeof(php_libvirt_domain));
    res_domain->domain = destdomain;
    res_domain->conn = dconn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_domain->domain);
    resource_change_counter(INT_RESOURCE_DOMAIN, dconn->conn, res_domain->domain, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_domain, le_libvirt_domain);
}

/*
 * Function name:   libvirt_domain_migrate_to_uri
 * Since version:   0.4.1(-1)
 * Description:     Function is used migrate domain to another libvirt daemon specified by it's URI
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @dest_uri [string]: destination URI to migrate to
 *                  @flags [int]: migration flags
 *                  @dname [string]: domain name to rename domain to on destination side
 *                  @bandwidth [int]: migration bandwidth in Mbps
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_migrate_to_uri)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    long flags = 0;
    char *duri;
    strsize_t duri_len;
    char *dname;
    strsize_t dname_len;
    zend_long bandwidth;

    dname = NULL;
    dname_len = 0;
    bandwidth = 0;
    GET_DOMAIN_FROM_ARGS("rsl|sl", &zdomain, &duri, &duri_len, &flags, &dname, &dname_len, &bandwidth);

    retval = virDomainMigrateToURI(domain->domain, duri, flags, dname, bandwidth);
    DPRINTF("%s: virDomainMigrateToURI() returned %d\n", PHPFUNC, retval);

    if (retval == 0)
        RETURN_TRUE;
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_domain_migrate_to_uri2
 * Since version:   0.4.6(-1)
 * Description:     Function is used migrate domain to another libvirt daemon specified by it's URI
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @dconnuri [string]: URI for target libvirtd
 *                  @miguri [string]: URI for invoking the migration
 *                  @dxml [string]: XML config for launching guest on target
 *                  @flags [int]: migration flags
 *                  @dname [string]: domain name to rename domain to on destination side
 *                  @bandwidth [int]: migration bandwidth in Mbps
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_migrate_to_uri2)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    char *dconnuri;
    strsize_t dconnuri_len;
    char *miguri;
    strsize_t miguri_len;
    char *dxml;
    strsize_t dxml_len;
    zend_long flags = 0;
    char *dname;
    strsize_t dname_len;
    zend_long bandwidth;

    dconnuri = NULL;
    dconnuri_len = 0;
    miguri = NULL;
    miguri_len = 0;
    dxml = NULL;
    dxml_len = 0;
    dname = NULL;
    dname_len = 0;
    bandwidth = 0;
    GET_DOMAIN_FROM_ARGS("r|ssslsl", &zdomain, &dconnuri, &dconnuri_len, &miguri, &miguri_len, &dxml, &dxml_len, &flags, &dname, &dname_len, &bandwidth);

    // set to NULL if empty string
    if (dconnuri_len == 0)
        dconnuri=NULL;
    if (miguri_len == 0)
        miguri=NULL;
    if (dxml_len == 0)
        dxml=NULL;
    if (dname_len == 0)
        dname=NULL;

    retval = virDomainMigrateToURI2(domain->domain, dconnuri, miguri, dxml, flags, dname, bandwidth);
    DPRINTF("%s: virDomainMigrateToURI2() returned %d\n", PHPFUNC, retval);

    if (retval == 0)
        RETURN_TRUE;
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_domain_get_job_info
 * Since version:   0.4.1(-1)
 * Description:     Function is used get job information for the domain
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         job information array of type, time, data, mem and file fields
 */
PHP_FUNCTION(libvirt_domain_get_job_info)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;

    struct _virDomainJobInfo jobinfo;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    retval = virDomainGetJobInfo(domain->domain, &jobinfo);
    if (retval == -1)
        RETURN_FALSE;

    array_init(return_value);
    LONGLONG_INIT;
    add_assoc_long(return_value, "type", jobinfo.type);
    LONGLONG_ASSOC(return_value, "time_elapsed", jobinfo.timeElapsed);
    LONGLONG_ASSOC(return_value, "time_remaining", jobinfo.timeRemaining);
    LONGLONG_ASSOC(return_value, "data_total", jobinfo.dataTotal);
    LONGLONG_ASSOC(return_value, "data_processed", jobinfo.dataProcessed);
    LONGLONG_ASSOC(return_value, "data_remaining", jobinfo.dataRemaining);
    LONGLONG_ASSOC(return_value, "mem_total", jobinfo.memTotal);
    LONGLONG_ASSOC(return_value, "mem_processed", jobinfo.memProcessed);
    LONGLONG_ASSOC(return_value, "mem_remaining", jobinfo.memRemaining);
    LONGLONG_ASSOC(return_value, "file_total", jobinfo.fileTotal);
    LONGLONG_ASSOC(return_value, "file_processed", jobinfo.fileProcessed);
    LONGLONG_ASSOC(return_value, "file_remaining", jobinfo.fileRemaining);
}

/*
 * Function name:   libvirt_domain_xml_xpath
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the result of xPath expression that's run against the domain
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @xpath [string]: xPath expression to parse against the domain
 *                  @flags [int]: optional flags
 * Returns:         result of the expression in an array
 */
PHP_FUNCTION(libvirt_domain_xml_xpath)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    zval *zpath;
    char *xml;
    char *tmp = NULL;
    strsize_t path_len = -1;
    zend_long flags = 0;
    int rc = 0;

    GET_DOMAIN_FROM_ARGS("rs|l", &zdomain, &zpath, &path_len, &flags);

    xml = virDomainGetXMLDesc(domain->domain, flags);
    if (!xml)
        RETURN_FALSE;

    array_init(return_value);

    tmp = get_string_from_xpath(xml, (char *)zpath, &return_value, &rc);
    if (return_value < 0) {
        VIR_FREE(xml);
        RETURN_FALSE;
    }

    VIR_FREE(tmp);
    VIR_FREE(xml);

    if (rc == 0)
        RETURN_FALSE;

    VIRT_ADD_ASSOC_STRING(return_value, "xpath", (char *)zpath);
    if (rc < 0)
        add_assoc_long(return_value, "error_code", (long)rc);
}

/*
 * Function name:   libvirt_domain_get_block_info
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the domain's block device information
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @dev [string]: device to get block information about
 * Returns:         domain block device information array of device, file or partition, capacity, allocation and physical size
 */
PHP_FUNCTION(libvirt_domain_get_block_info)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    char *dev;
    char *xml;
    char *tmp = NULL;
    strsize_t dev_len;
    int isFile;
    char *xpath = NULL;

    struct _virDomainBlockInfo info;

    GET_DOMAIN_FROM_ARGS("rs", &zdomain, &dev, &dev_len);

    /* Get XML for the domain */
    xml = virDomainGetXMLDesc(domain->domain, VIR_DOMAIN_XML_INACTIVE);
    if (!xml) {
        set_error("Cannot get domain XML" TSRMLS_CC);
        RETURN_FALSE;
    }

    isFile = 0;

    if (asprintf(&xpath, "//domain/devices/disk/target[@dev='%s']/../source/@dev", dev) < 0) {
        set_error("Out of memory" TSRMLS_CC);
        goto error;
    }
    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if (retval < 0) {
        set_error("Cannot get XPath expression result for device storage" TSRMLS_CC);
        goto error;
    }

    if (retval == 0) {
        VIR_FREE(xpath);
        if (asprintf(&xpath, "//domain/devices/disk/target[@dev='%s']/../source/@file", dev) < 0) {
            set_error("Out of memory" TSRMLS_CC);
            goto error;
        }
        VIR_FREE(tmp);
        tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
        if (retval < 0) {
            set_error("Cannot get XPath expression result for file storage" TSRMLS_CC);
            goto error;
        }
        isFile = 1;
    }

    if (retval == 0) {
        set_error("No relevant node found" TSRMLS_CC);
        goto error;
    }

    retval = virDomainGetBlockInfo(domain->domain, tmp, &info, 0);
    if (retval == -1) {
        set_error("Cannot get domain block information" TSRMLS_CC);
        goto error;
    }

    array_init(return_value);
    LONGLONG_INIT;
    VIRT_ADD_ASSOC_STRING(return_value, "device", dev);

    if (isFile)
        VIRT_ADD_ASSOC_STRING(return_value, "file", tmp);
    else
        VIRT_ADD_ASSOC_STRING(return_value, "partition", tmp);

    VIR_FREE(xpath);
    if (asprintf(&xpath, "//domain/devices/disk/target[@dev='%s']/../driver/@type", dev) < 0) {
        set_error("Out of memory" TSRMLS_CC);
        goto error;
    }
    VIR_FREE(tmp);
    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if (tmp != NULL)
        VIRT_ADD_ASSOC_STRING(return_value, "type", tmp);

    LONGLONG_ASSOC(return_value, "capacity", info.capacity);
    LONGLONG_ASSOC(return_value, "allocation", info.allocation);
    LONGLONG_ASSOC(return_value, "physical", info.physical);

    VIR_FREE(xpath);
    VIR_FREE(tmp);
    VIR_FREE(xml);
    return;

 error:
    VIR_FREE(xpath);
    VIR_FREE(tmp);
    VIR_FREE(xml);
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_domain_get_network_info
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the domain's network information
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @mac [string]: mac address of the network device
 * Returns:         domain network info array of MAC address, network name and type of NIC card
 */
PHP_FUNCTION(libvirt_domain_get_network_info)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    char *mac;
    char *xml;
    char *tmp = NULL;
    strsize_t mac_len;
    char *xpath = NULL;

    GET_DOMAIN_FROM_ARGS("rs", &zdomain, &mac, &mac_len);

    /* Get XML for the domain */
    xml = virDomainGetXMLDesc(domain->domain, VIR_DOMAIN_XML_INACTIVE);
    if (!xml) {
        set_error("Cannot get domain XML" TSRMLS_CC);
        RETURN_FALSE;
    }

    DPRINTF("%s: Getting network information for NIC with MAC address '%s'\n", PHPFUNC, mac);
    if (asprintf(&xpath, "//domain/devices/interface[@type='network']/mac[@address='%s']/../source/@network", mac) < 0) {
        set_error("Out of memory" TSRMLS_CC);
        goto error;
    }
    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if (tmp == NULL) {
        set_error("Invalid XPath node for source network" TSRMLS_CC);
        goto error;
    }

    if (retval < 0) {
        set_error("Cannot get XPath expression result for network source" TSRMLS_CC);
        goto error;
    }

    array_init(return_value);
    VIRT_ADD_ASSOC_STRING(return_value, "mac", mac);
    VIRT_ADD_ASSOC_STRING(return_value, "network", tmp);

    VIR_FREE(tmp);
    VIR_FREE(xpath);

    if (asprintf(&xpath, "//domain/devices/interface[@type='network']/mac[@address='%s']/../model/@type", mac) < 0) {
        set_error("Out of memory" TSRMLS_CC);
        goto error;
    }
    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "nic_type", tmp);
    else
        VIRT_ADD_ASSOC_STRING(return_value, "nic_type", "default");

    VIR_FREE(xml);
    VIR_FREE(xpath);
    VIR_FREE(tmp);
    return;

 error:
    VIR_FREE(xml);
    VIR_FREE(xpath);
    VIR_FREE(tmp);
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_domain_get_autostart
 * Since version:   0.4.1(-1)
 * Description:     Function is getting the autostart value for the domain
 * Arguments:       @res [resource]: libvirt domain resource
 * Returns:         autostart value or -1
 */
PHP_FUNCTION(libvirt_domain_get_autostart)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int flags = 0;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    if (virDomainGetAutostart (domain->domain, &flags) != 0) {
        RETURN_LONG(-1);
    }
    RETURN_LONG((long)flags);
}

/*
 * Function name:   libvirt_domain_set_autostart
 * Since version:   0.4.1(-1)
 * Description:     Function is setting the autostart value for the domain
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @flags [int]: flag to enable/disable autostart
 * Returns:         TRUE on success, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_set_autostart)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    zend_bool flags = 0;

    GET_DOMAIN_FROM_ARGS("rb", &zdomain, &flags);

    if (virDomainSetAutostart (domain->domain, flags) != 0) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_get_metadata
 * Since version:   0.4.9
 * Description:     Function retrieve appropriate domain element given by @type.
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @type [int]: virDomainMetadataType type of description
 *                  @uri [string]: XML namespace identifier
 *                  @flags [int]: bitwise-OR of virDomainModificationImpact
 * Returns:         metadata string, NULL on error or FALSE on API not supported
 */
PHP_FUNCTION(libvirt_domain_get_metadata)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    zend_long type = 0;
    zend_long flags = 0;
    char *uri = NULL;
    strsize_t uri_len;
    char *ret = NULL;

    GET_DOMAIN_FROM_ARGS("rlsl", &zdomain, &type, &uri, &uri_len, &flags);

    if ((uri != NULL) && (strlen(uri) == 0))
        uri = NULL;

    ret = virDomainGetMetadata(domain->domain, type, uri, flags);
    if (ret == NULL) {
        if (strstr(LIBVIRT_G(last_error), "not supported") != NULL)
            RETURN_FALSE;
        RETURN_NULL();
    } else {
        VIRT_RETVAL_STRING(ret);
        VIR_FREE(ret);
    }
}

/*
 * Function name:   libvirt_domain_set_metadata
 * Since version:   0.4.9
 * Description:     Function sets the appropriate domain element given by @type to the value of @description. No new lines are permitted.
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @type [int]: virDomainMetadataType type of description
 *                  @metadata [string]: new metadata text
 *                  @key [string]: XML namespace key or empty string (alias of NULL)
 *                  @uri [string]: XML namespace identifier or empty string (alias of NULL)
 *                  @flags [int]: bitwise-OR of virDomainModificationImpact
 * Returns:         -1 on error, 0 on success
 */
PHP_FUNCTION(libvirt_domain_set_metadata)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    strsize_t metadata_len, key_len, uri_len;
    char *metadata = NULL;
    char *key = NULL;
    char *uri = NULL;
    zend_long type = 0;
    zend_long flags = 0;
    int rc;

    GET_DOMAIN_FROM_ARGS("rlsssl", &zdomain, &type, &metadata, &metadata_len, &key, &key_len, &uri, &uri_len, &flags);

    if ((key != NULL) && (strlen(key) == 0))
        key = NULL;
    if ((uri != NULL) && (strlen(uri) == 0))
        uri = NULL;

    rc = virDomainSetMetadata(domain->domain, type, metadata, key, uri, flags);
    RETURN_LONG(rc);
}

/*
 * Function name:   libvirt_domain_is_active
 * Since version:   0.4.1(-1)
 * Description:     Function is getting information whether domain identified by resource is active or not
 * Arguments:       @res [resource]: libvirt domain resource
 * Returns:         virDomainIsActive() result on the domain
 */
PHP_FUNCTION(libvirt_domain_is_active)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    RETURN_LONG(virDomainIsActive(domain->domain));
}

/*
 * Function name:   libvirt_domain_get_next_dev_ids
 * Since version:   0.4.2
 * Description:     This functions can be used to get the next free slot if you intend to add a new device identified by slot to the domain, e.g. NIC device
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 * Returns:         next free slot number for the domain
 */
PHP_FUNCTION(libvirt_domain_get_next_dev_ids)
{
    long dom, bus, slot, func;
    php_libvirt_domain *domain = NULL;
    zval *zdomain;

    GET_DOMAIN_FROM_ARGS("r", &zdomain);

    DPRINTF("%s: Getting the next dev ids for domain %p\n", PHPFUNC, domain->domain);

    dom = get_next_free_numeric_value(domain->domain, "//@domain");
    bus = get_next_free_numeric_value(domain->domain, "//@bus");
    slot = get_next_free_numeric_value(domain->domain, "//@slot");
    func = get_next_free_numeric_value(domain->domain, "//@func");

    array_init(return_value);
    add_assoc_long(return_value, "next_domain", dom);
    add_assoc_long(return_value, "next_bus", bus);
    add_assoc_long(return_value, "next_slot", slot);
    add_assoc_long(return_value, "next_func", func);
}

/*
 * Function name:   libvirt_domain_get_screenshot
 * Since version:   0.4.2
 * Description:     Function uses gvnccapture (if available) to get the screenshot of the running domain
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @server [string]: server string for the host machine
 *                  @scancode [int]: integer value of the scancode to be send to refresh screen
 * Returns:         PNG image binary data
 */
PHP_FUNCTION(libvirt_domain_get_screenshot)
{
#ifndef EXTWIN
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    pid_t childpid = -1;
    pid_t w = -1;
    int retval = -1;
    int fd = -1, fsize = -1;
    char file[] = "/tmp/libvirt-php-tmp-XXXXXX";
    char *buf = NULL;
    char *tmp = NULL;
    char *xml = NULL;
    int port = -1;
    char *hostname = NULL;
    strsize_t hostname_len;
    zend_long scancode = 10;
    const char *path;
    char *pathDup = NULL;
    char name[1024] = { 0 };
    int use_builtin = 0;

    path = get_feature_binary("screenshot");
    DPRINTF("%s: get_feature_binary('screenshot') returned %s\n", PHPFUNC, path);

    if ((path == NULL) || (access(path, X_OK) != 0)) {
        use_builtin = 1;
    } else {
        if (!(pathDup = strdup(path))) {
            set_error("Out of memory" TSRMLS_CC);
            goto error;
        }
    }

    GET_DOMAIN_FROM_ARGS("rs|l", &zdomain, &hostname, &hostname_len, &scancode);

    xml = virDomainGetXMLDesc(domain->domain, 0);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        goto error;
    }

    tmp = get_string_from_xpath(xml, "//domain/devices/graphics/@port", NULL, &retval);
    if ((tmp == NULL) || (retval < 0)) {
        set_error("Cannot get the VNC port" TSRMLS_CC);
        goto error;
    }

    if (mkstemp(file) == 0)
        goto error;

    /* Get the current hostname and override to localhost if local machine */
    gethostname(name, 1024);
    if (strcmp(name, hostname) == 0)
        hostname = strdup("localhost");

    vnc_refresh_screen(hostname, tmp, scancode);

    if (use_builtin == 1) {
        DPRINTF("%s: Binary not found, using builtin approach to %s:%s, tmp file = %s\n", PHPFUNC, hostname, tmp, file);

        if (vnc_get_bitmap(hostname, tmp, file) != 0) {
            set_error("Cannot use builtin approach to get VNC window contents" TSRMLS_CC);
            goto error;
        }
    } else {
        port = atoi(tmp)-5900;

        DPRINTF("%s: Getting screenshot of %s:%d to temporary file %s\n", PHPFUNC, hostname, port, file);

        childpid = fork();
        if (childpid == -1)
            goto error;

        if (childpid == 0) {
            char tmpp[64] = { 0 };

            snprintf(tmpp, sizeof(tmpp), "%s:%d", hostname, port);
            retval = execlp(path, basename(pathDup), tmpp, file, NULL);
            _exit(retval);
        } else {
            do {
                w = waitpid(childpid, &retval, 0);
                if (w == -1)
                    goto error;
            } while (!WIFEXITED(retval) && !WIFSIGNALED(retval));
        }

        if (WEXITSTATUS(retval) != 0) {
            set_error("Cannot spawn utility to get screenshot" TSRMLS_CC);
            goto error;
        }
    }

    fd = open(file, O_RDONLY);
    fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    buf = emalloc((fsize + 1) * sizeof(char));
    memset(buf, 0, fsize + 1);
    if (read(fd, buf, fsize) < 0) {
        close(fd);
        unlink(file);
        goto error;
    }
    close(fd);

    if (access(file, F_OK) == 0) {
        DPRINTF("%s: Temporary file %s deleted\n", PHPFUNC, file);
        unlink(file);
    }

    /* This is necessary to make the output binary safe */
    VIRT_ZVAL_STRINGL(return_value, buf, fsize);

    efree(buf);
    VIR_FREE(tmp);
    VIR_FREE(xml);
    VIR_FREE(pathDup);
    return;

 error:
    efree(buf);
    VIR_FREE(tmp);
    VIR_FREE(xml);
    VIR_FREE(pathDup);
    RETURN_FALSE;
#else
    set_error("Function is not supported on Windows systems" TSRMLS_CC);
    RETURN_FALSE;
#endif
}

/*
 * Function name:   libvirt_domain_get_screenshot_api
 * Since version:   0.4.5
 * Description:     Function is trying to get domain screenshot using libvirt virGetDomainScreenshot() API if available.
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_get_by_*()
 *                  @screenID [int]: monitor ID from where to take screenshot
 * Returns:         array of filename and mime type as type is hypervisor specific, caller is responsible for temporary file deletion
 */
PHP_FUNCTION(libvirt_domain_get_screenshot_api)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    zend_long screen = 0;
    int fd = -1;
    char file[] = "/tmp/libvirt-php-tmp-XXXXXX";
    virStreamPtr st = NULL;
    char *mime = NULL;
    const char *bin = get_feature_binary("screenshot-convert");

#ifdef EXTWIN
    set_error_if_unset("Cannot get domain screenshot on Windows systems" TSRMLS_CC);
    RETURN_FALSE;
#endif

    GET_DOMAIN_FROM_ARGS("r|l", &zdomain, &screen);

    if (!(st = virStreamNew(domain->conn->conn, 0))) {
        set_error("Cannot create new stream" TSRMLS_CC);
        goto error;
    }

    mime = virDomainScreenshot(domain->domain, st, screen, 0);
    if (!mime) {
        set_error_if_unset("Cannot get domain screenshot" TSRMLS_CC);
        goto error;
    }

    if (!(fd = mkstemp(file))) {
        virStreamAbort(st);
        set_error_if_unset("Cannot get create file to save domain screenshot" TSRMLS_CC);
        goto error;
    }

    if (virStreamRecvAll(st, streamSink, &fd) < 0) {
        set_error_if_unset("Cannot receive screenshot data" TSRMLS_CC);
        virStreamAbort(st);
        goto error;
    }

    if (virStreamFinish(st) < 0) {
        set_error_if_unset("Cannot close stream for domain" TSRMLS_CC);
        goto error;
    }

    virStreamFree(st);
    st = NULL;

    array_init(return_value);
    if (bin) {
        char tmp[4096] = { 0 };
        char fileNew[1024] = { 0 };
        int exitStatus;

        snprintf(fileNew, sizeof(fileNew), "%s.png", file);
        snprintf(tmp, sizeof(tmp), "%s %s %s > /dev/null 2> /dev/null", bin, file, fileNew);
        exitStatus = system(tmp);
        if (WEXITSTATUS(exitStatus) != 0)
            goto error;

        unlink(file);
        close(fd);
        fd = -1;
        VIRT_ADD_ASSOC_STRING(return_value, "file", fileNew);
        VIRT_ADD_ASSOC_STRING(return_value, "mime", "image/png");
    } else {
        close(fd);
        fd = -1;
        VIRT_ADD_ASSOC_STRING(return_value, "file", file);
        VIRT_ADD_ASSOC_STRING(return_value, "mime", mime);
    }

    VIR_FREE(mime);
    return;

 error:
    VIR_FREE(mime);
    if (fd != -1) {
        unlink(file);
        close(fd);
    }
    if (st)
        virStreamFree(st);
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_domain_get_screen_dimensions
 * Since version:   0.4.3
 * Description:     Function get screen dimensions of the VNC window
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @server [string]: server string of the host machine
 * Returns:         array of height and width on success, FALSE otherwise
 */
PHP_FUNCTION(libvirt_domain_get_screen_dimensions)
{
#ifndef EXTWIN
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval = -1;
    char *tmp = NULL;
    char *xml = NULL;
    char *hostname = NULL;
    strsize_t hostname_len;
    int ret;
    int width;
    int height;

    GET_DOMAIN_FROM_ARGS("rs", &zdomain, &hostname, &hostname_len);

    xml = virDomainGetXMLDesc(domain->domain, 0);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        goto error;
    }

    tmp = get_string_from_xpath(xml, "//domain/devices/graphics/@port", NULL, &retval);
    if ((tmp == NULL) || (retval < 0)) {
        set_error("Cannot get the VNC port" TSRMLS_CC);
        goto error;
    }

    DPRINTF("%s: hostname = %s, port = %s\n", PHPFUNC, hostname, tmp);
    ret = vnc_get_dimensions(hostname, tmp, &width, &height);
    VIR_FREE(tmp);
    if (ret != 0) {
        char error[1024] = { 0 };
        if (ret == -9)
            snprintf(error, sizeof(error), "Cannot connect to VNC server. Please make sure domain is running and VNC graphics are set");
        else
            snprintf(error, sizeof(error), "Cannot get screen dimensions, error code = %d (%s)", ret, strerror(-ret));

        set_error(error TSRMLS_CC);
        goto error;
    }

    array_init(return_value);
    add_assoc_long(return_value, "width", (long)width);
    add_assoc_long(return_value, "height", (long)height);

    VIR_FREE(tmp);
    VIR_FREE(xml);
    return;

 error:
    VIR_FREE(tmp);
    VIR_FREE(xml);
    RETURN_FALSE;
#else
    set_error("Function is not supported on Windows systems" TSRMLS_CC);
    RETURN_FALSE;
#endif
}

/*
 * Function name:   libvirt_domain_send_keys
 * Since version:   0.4.2
 * Description:     Function sends keys to the domain's VNC window
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @server [string]: server string of the host machine
 *                  @scancode [int]: integer scancode to be sent to VNC window
 * Returns:         TRUE on success, FALSE otherwise
 */
PHP_FUNCTION(libvirt_domain_send_keys)
{
#ifndef EXTWIN
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval = -1;
    char *tmp = NULL;
    char *xml = NULL;
    char *hostname = NULL;
    strsize_t hostname_len;
    char *keys = NULL;
    strsize_t keys_len;
    int ret = 0;

    GET_DOMAIN_FROM_ARGS("rss", &zdomain, &hostname, &hostname_len, &keys, &keys_len);

    DPRINTF("%s: Sending %d VNC keys to %s...\n", PHPFUNC, (int)strlen((const char *)keys), hostname);

    xml = virDomainGetXMLDesc(domain->domain, 0);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        goto error;
    }

    tmp = get_string_from_xpath(xml, "//domain/devices/graphics/@port", NULL, &retval);
    if ((tmp == NULL) || (retval < 0)) {
        set_error("Cannot get the VNC port" TSRMLS_CC);
        goto error;
    }

    DPRINTF("%s: About to send string '%s' (%d keys) to %s:%s\n", PHPFUNC, keys, (int)strlen((const char *)keys), hostname, tmp);

    ret = vnc_send_keys(hostname, tmp, keys);
    DPRINTF("%s: Sequence sending result is %d\n", PHPFUNC, ret);

    if (!ret) {
        char tmpp[64] = { 0 };
        snprintf(tmpp, sizeof(tmpp), "Cannot send keys, error code %d", ret);
        set_error(tmpp TSRMLS_CC);
        goto error;
    }

    VIR_FREE(tmp);
    VIR_FREE(xml);
    RETURN_TRUE;

 error:
    VIR_FREE(tmp);
    VIR_FREE(xml);
    RETURN_FALSE;
#else
    set_error("Function is not supported on Windows systems" TSRMLS_CC);
    RETURN_FALSE;
#endif
}

/*
 * Function name:   libvirt_domain_send_key_api
 * Since version:   0.5.3
 * Description:     Function sends keys to domain via libvirt API
 * Arguments:       @res[resource]: libvirt domain resource, e.g. from libvirt_domaing_lookup_by_*()
 *                  @codeset [int]: the codeset of keycodes, from virKeycodeSet
 *                  @holdtime [int]: the duration (in miliseconds) that the keys will be held
 *                  @keycodes [array]: array of keycodes
 *                  @flags [int]: extra flags; not used yet so callers should alway pass 0
 * Returns:         TRUE for success, FALSE for failure
 */
PHP_FUNCTION(libvirt_domain_send_key_api)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    zend_long codeset;
    zend_long holdtime = 0;
    zend_long flags = 0;
    zval *zkeycodes, *data = NULL;
    HashTable *arr_hash = NULL;
    HashPosition pointer;
    int count, i;
    uint *keycodes = NULL;

    GET_DOMAIN_FROM_ARGS("rlla|l", &zdomain, &codeset, &holdtime, &zkeycodes,
                         &flags);

    arr_hash = Z_ARRVAL_P(zkeycodes);
    count = zend_hash_num_elements(arr_hash);

    keycodes = (uint *) emalloc(count * sizeof(uint));

    i = 0;
    VIRT_FOREACH(arr_hash, pointer, data) {
        if (Z_TYPE_P(data) == IS_LONG) {
            keycodes[i++] = (uint) Z_LVAL_P(data);
        }
    } VIRT_FOREACH_END();

    if (virDomainSendKey(domain->domain, codeset, holdtime, keycodes, count,
                         flags) != 0) {
        efree(keycodes);
        RETURN_FALSE;
    }

    efree(keycodes);
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_send_pointer_event
 * Since version:   0.4.2
 * Description:     Function sends keys to the domain's VNC window
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @server [string]: server string of the host machine
 *                  @pos_x [int]: position on x-axis
 *                  @pos_y [int]: position on y-axis
 *                  @clicked [int]: mask of clicked buttons (0 for none, bit 1 for button #1, bit 8 for button #8)
 *                  @release [int]: boolean value (0 or 1) whether to release the buttons automatically once pressed
 * Returns:         TRUE on success, FALSE otherwise
 */
PHP_FUNCTION(libvirt_domain_send_pointer_event)
{
#ifndef EXTWIN
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval = -1;
    char *tmp = NULL;
    char *xml = NULL;
    char *hostname = NULL;
    strsize_t hostname_len;
    zend_long pos_x = 0;
    zend_long pos_y = 0;
    zend_long clicked = 0;
    zend_bool release = 1;
    int ret;

    GET_DOMAIN_FROM_ARGS("rslll|b", &zdomain, &hostname, &hostname_len, &pos_x, &pos_y, &clicked, &release);

    xml = virDomainGetXMLDesc(domain->domain, 0);
    if (!xml) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        goto error;
    }

    tmp = get_string_from_xpath(xml, "//domain/devices/graphics/@port", NULL, &retval);
    if ((tmp == NULL) || (retval < 0)) {
        set_error("Cannot get the VNC port" TSRMLS_CC);
        goto error;
    }

    DPRINTF("%s: x = %d, y = %d, clicked = %d, release = %d, hostname = %s...\n", PHPFUNC, (int) pos_x, (int) pos_y, (int) clicked, release, hostname);
    ret = vnc_send_pointer_event(hostname, tmp, pos_x, pos_y, clicked, release);
    if (!ret) {
        char error[1024] = { 0 };
        if (ret == -9)
            snprintf(error, sizeof(error), "Cannot connect to VNC server. Please make sure domain is running and VNC graphics are set");
        else
            snprintf(error, sizeof(error), "Cannot send pointer event, error code = %d (%s)", ret, strerror(-ret));

        set_error(error TSRMLS_CC);
        goto error;
    }

    VIR_FREE(tmp);
    VIR_FREE(xml);
    RETURN_TRUE;

 error:
    VIR_FREE(tmp);
    VIR_FREE(xml);
    RETURN_FALSE;
#else
    set_error("Function is not supported on Windows systems" TSRMLS_CC);
    RETURN_FALSE;
#endif
}

/*
 * Function name:   libvirt_domain_update_device
 * Since version:   0.4.1(-1)
 * Description:     Function is used to update the domain's devices from the XML string
 * Arguments:       @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                  @xml [string]: XML string for the update
 *                  @flags [int]: Flags to update the device (VIR_DOMAIN_DEVICE_MODIFY_CURRENT, VIR_DOMAIN_DEVICE_MODIFY_LIVE, VIR_DOMAIN_DEVICE_MODIFY_CONFIG, VIR_DOMAIN_DEVICE_MODIFY_FORCE)
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_update_device)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    char *xml;
    strsize_t xml_len;
    zend_long flags;
    int res;

    GET_DOMAIN_FROM_ARGS("rsl", &zdomain, &xml, &xml_len, &flags);

    res = virDomainUpdateDeviceFlags(domain->domain, xml, flags);
    DPRINTF("%s: virDomainUpdateDeviceFlags(%p) returned %d\n", PHPFUNC, domain->domain, res);
    if (res != 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:  libvirt_domain_qemu_agent_command
 * Since version:  0.5.2(-1)
 * Description:    Function is used to send qemu-ga command
 * Arguments:      @res [resource]: libvirt domain resource, e.g. from libvirt_domain_lookup_by_*()
 *                 @cmd [string]: command
 *                 @timeout [int] timeout for waiting (-2 block, -1 default, 0 no wait, >0 wait specific time
 *                 @flags [int]: unknown
 * Returns:        String on success and FALSE on error
 */
PHP_FUNCTION(libvirt_domain_qemu_agent_command)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    const char *cmd;
    strsize_t cmd_len;
    char *ret;
    zend_long timeout = -1;
    zend_long flags = 0;

    GET_DOMAIN_FROM_ARGS("rs|ll", &zdomain, &cmd, &cmd_len, &timeout, &flags);

    ret = virDomainQemuAgentCommand(domain->domain, cmd, timeout, flags);
    if (ret == NULL)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(ret);
    VIR_FREE(ret);
}

/*
 * Function name:   libvirt_list_domains
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list domains on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt domain names array for the connection
 */
PHP_FUNCTION(libvirt_list_domains)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int count = -1;
    int expectedcount = -1;
    int *ids;
    char **names;
    const char *name;
    int i, rv;
    virDomainPtr domain = NULL;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((expectedcount = virConnectNumOfDomains(conn->conn)) < 0)
        RETURN_FALSE;

    DPRINTF("%s: Found %d domains\n", PHPFUNC, expectedcount);

    ids = (int *)emalloc(sizeof(int) * expectedcount);
    count = virConnectListDomains(conn->conn, ids, expectedcount);
    DPRINTF("%s: virConnectListDomains returned %d domains\n", PHPFUNC, count);

    array_init(return_value);
    for (i = 0; i < count; i++) {
        domain = virDomainLookupByID(conn->conn, ids[i]);
        resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, domain, 1 TSRMLS_CC);
        if (domain != NULL) {
            name = virDomainGetName(domain);
            if (name != NULL) {
                DPRINTF("%s: Found running domain %s with ID = %d\n", PHPFUNC, name, ids[i]);
                VIRT_ADD_NEXT_INDEX_STRING(return_value, name);
            } else {
                DPRINTF("%s: Cannot get ID for running domain %d\n", PHPFUNC, ids[i]);
            }
        }
        rv = virDomainFree(domain);
        if (rv != 0) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "virDomainFree failed with %i on list_domain: %s",
                             rv, LIBVIRT_G(last_error));
        } else {
            resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, domain, 0 TSRMLS_CC);
        }
        domain = NULL;
    }
    efree(ids);

    expectedcount = virConnectNumOfDefinedDomains(conn->conn);
    DPRINTF("%s: virConnectNumOfDefinedDomains returned %d domains\n", PHPFUNC, expectedcount);
    if (expectedcount < 0) {
        DPRINTF("%s: virConnectNumOfDefinedDomains failed with error code %d\n", PHPFUNC, expectedcount);
        RETURN_FALSE;
    }

    names = (char **)emalloc(expectedcount*sizeof(char *));
    count = virConnectListDefinedDomains(conn->conn, names, expectedcount);
    DPRINTF("%s: virConnectListDefinedDomains returned %d domains\n", PHPFUNC, count);
    if (count < 0) {
        DPRINTF("%s: virConnectListDefinedDomains failed with error code %d\n", PHPFUNC, count);
        RETURN_FALSE;
    }

    for (i = 0; i < count; i++) {
        VIRT_ADD_NEXT_INDEX_STRING(return_value, names[i]);
        DPRINTF("%s: Found inactive domain %s\n", PHPFUNC, names[i]);
        VIR_FREE(names[i]);
    }
    efree(names);
}

/*
 * Function name:   libvirt_list_domain_resources
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list domain resources on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt domain resources array for the connection
 */
PHP_FUNCTION(libvirt_list_domain_resources)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int count = -1;
    int expectedcount = -1;
    int *ids;
    char **names;
    int i;

    virDomainPtr domain = NULL;
    php_libvirt_domain *res_domain;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((expectedcount = virConnectNumOfDomains(conn->conn)) < 0)
        RETURN_FALSE;

    ids = (int *)emalloc(sizeof(int) * expectedcount);
    count = virConnectListDomains(conn->conn, ids, expectedcount);
    if ((count != expectedcount) || (count < 0)) {
        efree(ids);
        RETURN_FALSE;
    }
    array_init(return_value);
    for (i = 0; i < count; i++) {
        domain = virDomainLookupByID(conn->conn, ids[i]);
        if (domain != NULL) {
            res_domain = (php_libvirt_domain *)emalloc(sizeof(php_libvirt_domain));
            res_domain->domain = domain;

            res_domain->conn = conn;

            VIRT_REGISTER_LIST_RESOURCE(domain);
            resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, res_domain->domain, 1 TSRMLS_CC);
        }
    }
    efree(ids);

    if ((expectedcount = virConnectNumOfDefinedDomains(conn->conn)) < 0)
        RETURN_FALSE;

    names = (char **)emalloc(expectedcount*sizeof(char *));
    count = virConnectListDefinedDomains(conn->conn, names, expectedcount);
    if ((count != expectedcount) || (count < 0)) {
        efree(names);
        RETURN_FALSE;
    }
    for (i = 0; i < count; i++) {
        domain = virDomainLookupByName(conn->conn, names[i]);
        if (domain != NULL) {
            res_domain = (php_libvirt_domain *)emalloc(sizeof(php_libvirt_domain));
            res_domain->domain = domain;

            res_domain->conn = conn;

            VIRT_REGISTER_LIST_RESOURCE(domain);
            resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, res_domain->domain, 1 TSRMLS_CC);
        }
        VIR_FREE(names[i]);
    }
    efree(names);
}

/*
 * Function name:   libvirt_list_active_domain_ids
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list active domain IDs on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt active domain ids array for the connection
 */
PHP_FUNCTION(libvirt_list_active_domain_ids)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int count = -1;
    int expectedcount = -1;
    int *ids;
    int i;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((expectedcount = virConnectNumOfDomains(conn->conn)) < 0)
        RETURN_FALSE;

    ids = (int *)emalloc(sizeof(int) * expectedcount);
    count = virConnectListDomains(conn->conn, ids, expectedcount);
    if ((count != expectedcount) || (count < 0)) {
        efree(ids);
        RETURN_FALSE;
    }
    array_init(return_value);
    for (i = 0; i < count; i++)
        add_next_index_long(return_value,  ids[i]);
    efree(ids);
}

/*
 * Function name:   libvirt_list_active_domains
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list active domain names on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt active domain names array for the connection
 */
PHP_FUNCTION(libvirt_list_active_domains)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int count = -1;
    int expectedcount = -1;
    int *ids;
    int i;
    virDomainPtr domain = NULL;
    const char *name;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((expectedcount = virConnectNumOfDomains(conn->conn)) < 0)
        RETURN_FALSE;

    ids = (int *)emalloc(sizeof(int) * expectedcount);
    count = virConnectListDomains(conn->conn, ids, expectedcount);
    if ((count != expectedcount) || (count < 0)) {
        efree(ids);
        RETURN_FALSE;
    }

    array_init(return_value);
    for (i = 0; i < count; i++) {
        domain = virDomainLookupByID(conn->conn, ids[i]);
        if (domain != NULL) {
            if (!(name = virDomainGetName(domain))) {
                efree(ids);
                RETURN_FALSE;
            }

            VIRT_ADD_NEXT_INDEX_STRING(return_value, name);

            if (virDomainFree(domain))
                resource_change_counter(INT_RESOURCE_DOMAIN, conn->conn, domain, 0 TSRMLS_CC);
        }
    }
    efree(ids);
}

/*
 * Function name:   libvirt_list_inactive_domains
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list inactive domain names on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt inactive domain names array for the connection
 */
PHP_FUNCTION(libvirt_list_inactive_domains)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int count = -1;
    int expectedcount = -1;
    char **names;
    int i;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((expectedcount = virConnectNumOfDefinedDomains(conn->conn)) < 0)
        RETURN_FALSE;

    names = (char **)emalloc(expectedcount*sizeof(char *));
    count = virConnectListDefinedDomains(conn->conn, names, expectedcount);
    if ((count != expectedcount) || (count < 0)) {
        efree(names);
        RETURN_FALSE;
    }

    array_init(return_value);
    for (i = 0; i < count; i++) {
        VIRT_ADD_NEXT_INDEX_STRING(return_value,  names[i]);
        VIR_FREE(names[i]);
    }
    efree(names);
}

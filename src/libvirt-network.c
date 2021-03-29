/*
 * libvirt-network.c: The PHP bindings to libvirt network API
 *
 * See COPYING for the license of this software
 */

#include <libvirt/libvirt.h>

#include "libvirt-php.h"
#include "libvirt-network.h"

DEBUG_INIT("network");

int le_libvirt_network;

void
php_libvirt_network_dtor(virt_resource *rsrc TSRMLS_DC)
{
    php_libvirt_network *network = (php_libvirt_network *)rsrc->ptr;
    int rv = 0;

    if (network != NULL) {
        if (network->network != NULL) {
            if (!check_resource_allocation(network->conn->conn, INT_RESOURCE_NETWORK, network->network TSRMLS_CC)) {
                network->network = NULL;
                efree(network);
                return;
            }
            rv = virNetworkFree(network->network);
            if (rv != 0) {
                DPRINTF("%s: virNetworkFree(%p) returned %d (%s)\n", __FUNCTION__, network->network, rv, LIBVIRT_G(last_error));
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "virStorageVolFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virNetworkFree(%p) completed successfully\n", __FUNCTION__, network->network);
                resource_change_counter(INT_RESOURCE_NETWORK, NULL, network->network, 0 TSRMLS_CC);
            }
            network->network = NULL;
        }
        efree(network);
    }
}

/*
 * Function name:   libvirt_network_define_xml
 * Since version:   0.4.2
 * Description:     Function is used to define a new virtual network based on the XML description
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @xml [string]: XML string definition of network to be defined
 * Returns:         libvirt network resource of newly defined network
 */
PHP_FUNCTION(libvirt_network_define_xml)
{
    php_libvirt_connection *conn = NULL;
    php_libvirt_network *res_net = NULL;
    virNetwork *net;
    zval *zconn;
    char *xml = NULL;
    strsize_t xml_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &xml, &xml_len);

    if ((net = virNetworkDefineXML(conn->conn, xml)) == NULL) {
        set_error_if_unset("Cannot define a new network" TSRMLS_CC);
        RETURN_FALSE;
    }

    res_net = (php_libvirt_network *)emalloc(sizeof(php_libvirt_network));
    res_net->network = net;
    res_net->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_net->network);
    resource_change_counter(INT_RESOURCE_NETWORK, conn->conn, res_net->network, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_net, le_libvirt_network);
}

/*
 * Function name:   libvirt_network_get_xml_desc
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the XML description for the network
 * Arguments:       @res [resource]: libvirt network resource
 *                  @xpath [string]: optional xPath expression string to get just this entry, can be NULL
 * Returns:         network XML string or result of xPath expression
 */
PHP_FUNCTION(libvirt_network_get_xml_desc)
{
    php_libvirt_network *network;
    zval *znetwork;
    char *xml = NULL;
    char *xpath = NULL;
    char *tmp;
    strsize_t xpath_len;
    int retval = -1;

    GET_NETWORK_FROM_ARGS("r|s", &znetwork, &xpath, &xpath_len);
    if (xpath_len < 1)
        xpath = NULL;

    xml = virNetworkGetXMLDesc(network->network, 0);

    if (xml == NULL) {
        set_error_if_unset("Cannot get network XML" TSRMLS_CC);
        RETURN_FALSE;
    }

    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if ((tmp == NULL) || (retval < 0)) {
        VIRT_RETVAL_STRING(xml);
    } else {
        VIRT_RETVAL_STRING(tmp);
    }

    VIR_FREE(xml);
    VIR_FREE(tmp);
}

/*
 * Function name:   libvirt_network_undefine
 * Since version:   0.4.2
 * Description:     Function is used to undefine already defined network
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_network_undefine)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    if (virNetworkUndefine(network->network) != 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_network_get
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the network resource from name
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @name [string]: network name string
 * Returns:         libvirt network resource
 */
PHP_FUNCTION(libvirt_network_get)
{
    php_libvirt_connection *conn = NULL;
    php_libvirt_network *res_net = NULL;
    virNetwork *net;
    zval *zconn;
    char *name;
    strsize_t name_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &name, &name_len);

    if ((net = virNetworkLookupByName(conn->conn, name)) == NULL) {
        set_error_if_unset("Cannot get find requested network" TSRMLS_CC);
        RETURN_FALSE;
    }

    res_net = (php_libvirt_network *)emalloc(sizeof(php_libvirt_network));
    res_net->network = net;
    res_net->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_net->network);
    resource_change_counter(INT_RESOURCE_NETWORK, conn->conn, res_net->network, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_net, le_libvirt_network);
}

/*
 * Function name:   libvirt_network_get_bridge
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the bridge associated with the network
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         bridge name string
 */
PHP_FUNCTION(libvirt_network_get_bridge)
{
    php_libvirt_network *network;
    zval *znetwork;
    char *name;

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    name = virNetworkGetBridgeName(network->network);

    if (name == NULL) {
        set_error_if_unset("Cannot get network bridge name" TSRMLS_CC);
        RETURN_FALSE;
    }

    VIRT_RETVAL_STRING(name);
    VIR_FREE(name);
}

/*
 * Function name:   libvirt_network_get_active
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the activity state of the network
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         1 when active, 0 when inactive, FALSE on error
 */
PHP_FUNCTION(libvirt_network_get_active)
{
    php_libvirt_network *network;
    zval *znetwork;
    int res;

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    res = virNetworkIsActive(network->network);

    if (res == -1) {
        set_error_if_unset("Error getting virtual network state" TSRMLS_CC);
        RETURN_FALSE;
    }

    RETURN_LONG(res);
}

/*
 * Function name:   libvirt_network_set_active
 * Since version:   0.4.1(-1)
 * Description:     Function is used to set the activity state of the network
 * Arguments:       @res [resource]: libvirt network resource
 *                  @flags [int]: active
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_network_set_active)
{
    php_libvirt_network *network;
    zval *znetwork;
    zend_long act = 0;

    DPRINTF("%s: Setting network activity...\n", PHPFUNC);

    GET_NETWORK_FROM_ARGS("rl", &znetwork, &act);

    if ((act != 0) && (act != 1)) {
        set_error("Invalid network activity state" TSRMLS_CC);
        RETURN_FALSE;
    }

    DPRINTF("%s: %sabling network...\n", PHPFUNC, (act == 1) ? "En" : "Dis");

    if (act == 1) {
        if (virNetworkCreate(network->network) == 0) {
            // Network is up and running
            RETURN_TRUE;
        } else {
            // We don't have to set error since it's caught by libvirt error handler itself
            RETURN_FALSE;
        }
    }

    if (virNetworkDestroy(network->network) == 0) {
        // Network is down
        RETURN_TRUE;
    } else {
        // Caught by libvirt error handler too
        RETURN_FALSE;
    }
}

/*
 * Function name:   libvirt_network_get_information
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the network information
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         network information array
 */
PHP_FUNCTION(libvirt_network_get_information)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    int retval = 0;
    char *xml  = NULL;
    char *name = NULL;
    char *ipaddr = NULL;
    char *netmask = NULL;
    char *mode = NULL;
    char *dev = NULL;
    char *dhcp_start = NULL;
    char *dhcp_end = NULL;
    char fixedtemp[32] = { 0 };

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    xml = virNetworkGetXMLDesc(network->network, 0);

    if (xml == NULL) {
        set_error_if_unset("Cannot get network XML" TSRMLS_CC);
        RETURN_FALSE;
    }

    array_init(return_value);

    /* Get name */
    name = get_string_from_xpath(xml, "//network/name", NULL, &retval);
    if (name == NULL) {
        set_error("Invalid XPath node for network name" TSRMLS_CC);
        RETURN_FALSE;
    }

    if (retval < 0) {
        set_error("Cannot get XPath expression result for network name" TSRMLS_CC);
        RETURN_FALSE;
    }

    VIRT_ADD_ASSOC_STRING(return_value, "name", name);

    /* Get gateway IP address */
    ipaddr = get_string_from_xpath(xml, "//network/ip/@address", NULL, &retval);
    if (ipaddr && retval > 0)
        VIRT_ADD_ASSOC_STRING(return_value, "ip", ipaddr);

    /* Get netmask */
    netmask = get_string_from_xpath(xml, "//network/ip/@netmask", NULL, &retval);
    if (netmask && retval > 0) {
        int subnet_bits = get_subnet_bits(netmask);
        VIRT_ADD_ASSOC_STRING(return_value, "netmask", netmask);
        add_assoc_long(return_value, "netmask_bits", (long) subnet_bits);

        /* Format CIDR address representation */
        ipaddr[strlen(ipaddr) - 1] = ipaddr[strlen(ipaddr) - 1] - 1;
        snprintf(fixedtemp, sizeof(fixedtemp), "%s/%d", ipaddr, subnet_bits);
        VIRT_ADD_ASSOC_STRING(return_value, "ip_range", fixedtemp);
    }

    /* Get forwarding settings */
    mode = get_string_from_xpath(xml, "//network/forward/@mode", NULL, &retval);
    if (mode && retval > 0)
        VIRT_ADD_ASSOC_STRING(return_value, "forwarding", mode);

    /* Get forwarding settings */
    dev = get_string_from_xpath(xml, "//network/forward/@dev", NULL, &retval);
    if (dev && retval > 0)
        VIRT_ADD_ASSOC_STRING(return_value, "forward_dev", dev);

    /* Get DHCP values */
    dhcp_start = get_string_from_xpath(xml, "//network/ip/dhcp/range/@start", NULL, &retval);
    dhcp_end = get_string_from_xpath(xml, "//network/ip/dhcp/range/@end", NULL, &retval);
    if (dhcp_start && dhcp_end && retval > 0) {
        VIRT_ADD_ASSOC_STRING(return_value, "dhcp_start", dhcp_start);
        VIRT_ADD_ASSOC_STRING(return_value, "dhcp_end", dhcp_end);
    }

    VIR_FREE(dhcp_end);
    VIR_FREE(dhcp_start);
    VIR_FREE(dev);
    VIR_FREE(mode);
    VIR_FREE(netmask);
    VIR_FREE(ipaddr);
    VIR_FREE(name);
    VIR_FREE(xml);
}

/*
 * Function name:   libvirt_network_get_uuid_string
 * Since version:   0.5.3
 * Description:     Function is used to get network's UUID in string format
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         network UUID string or FALSE on failure
 */
PHP_FUNCTION(libvirt_network_get_uuid_string)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    char *uuid = NULL;
    int ret = -1;

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    uuid = (char *) emalloc(VIR_UUID_STRING_BUFLEN);
    ret = virNetworkGetUUIDString(network->network, uuid);

    DPRINTF("%s: virNetworkGetUUIDString(%p) returned %d (%s)\n", PHPFUNC,
            network->network, ret, uuid);

    if (ret != 0)
        RETURN_FALSE;

    VIRT_RETURN_STRING(uuid);
    efree(uuid);
}

/*
 * Function name:   libvirt_network_get_uuid
 * Since version:   0.5.3
 * Description:     Function is used to get network's UUID in binary format
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         network UUID in binary format or FALSE on failure
 */
PHP_FUNCTION(libvirt_network_get_uuid)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    char *uuid = NULL;
    int ret = -1;

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    uuid = (char *) emalloc(VIR_UUID_BUFLEN);
    ret = virNetworkGetUUID(network->network, (unsigned char *)uuid);

    DPRINTF("%s: virNetworkGetUUID(%p, %p) returned %d\n", PHPFUNC,
            network->network, uuid, ret);

    if (ret != 0)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(uuid);
    efree(uuid);
}

/*
 * Function name:   libvirt_network_get_name
 * Since version:   0.5.3
 * Description:     Function is used to get network's name
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         network name string or FALSE on failure
 */
PHP_FUNCTION(libvirt_network_get_name)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    const char *name = NULL;

    GET_NETWORK_FROM_ARGS("r", &znetwork);
    name = virNetworkGetName(network->network);

    DPRINTF("%s: virNetworkGetName(%p) returned %s\n", PHPFUNC,
            network->network, name);

    if (name == NULL)
        RETURN_FALSE;

    /* name should not be freed as its lifetime is the same as network resource */
    VIRT_RETURN_STRING(name);
}

/*
 * Function name:   libvirt_network_get_autostart
 * Since version:   0.5.4
 * Description:     Function is getting the autostart value for the network
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         autostart value or -1 on error
 */
PHP_FUNCTION(libvirt_network_get_autostart)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    int autostart;

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    if (virNetworkGetAutostart(network->network, &autostart) != 0)
        RETURN_LONG(-1);

    RETURN_LONG((long) autostart);
}

/*
 * Function name:   libvirt_network_set_autostart
 * Since version:   0.5.4
 * Description:     Function is setting the autostart value for the network
 * Arguments:       @res [resource]: libvirt network resource
 *                  @flags [int]: flag to enable/disable autostart
 * Returns:         TRUE on success, FALSE on error
 */
PHP_FUNCTION(libvirt_network_set_autostart)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    zend_long autostart = 0;

    GET_NETWORK_FROM_ARGS("rl", &znetwork, &autostart);

    if (virNetworkSetAutostart(network->network, autostart) < 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_list_all_networks
 * Since version:   0.5.3
 * Description:     Function is used to list networks on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @flags [int]: optional flags to filter the results for a smaller list of targeted networks (bitwise-OR VIR_CONNECT_LIST_NETWORKS_* constants)
 * Returns:         libvirt network resources array for the connection
 */
PHP_FUNCTION(libvirt_list_all_networks)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    zend_long flags = VIR_CONNECT_LIST_NETWORKS_ACTIVE |
                      VIR_CONNECT_LIST_NETWORKS_INACTIVE;
    int count = -1;
    size_t i = 0;
    virNetworkPtr *nets = NULL;
    virNetworkPtr network = NULL;
    php_libvirt_network *res_network;

    GET_CONNECTION_FROM_ARGS("r|l", &zconn, &flags);

    if ((count = virConnectListAllNetworks(conn->conn, &nets, flags)) < 0)
        RETURN_FALSE;

    DPRINTF("%s: Found %d networks\n", PHPFUNC, count);

    array_init(return_value);

    for (i = 0; i < count; i++) {
        network = nets[i];
        res_network = (php_libvirt_network *) emalloc(sizeof(php_libvirt_network));
        res_network->network = network;
        res_network->conn = conn;

        VIRT_REGISTER_LIST_RESOURCE(network);
        resource_change_counter(INT_RESOURCE_NETWORK, conn->conn,
                                res_network->network, 1 TSRMLS_CC);
    }
}

/*
 * Function name:   libvirt_list_networks
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list networks on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @flags [int]: flags whether to list active, inactive or all networks (VIR_NETWORKS_{ACTIVE|INACTIVE|ALL} constants)
 * Returns:         libvirt network names array for the connection
 */
PHP_FUNCTION(libvirt_list_networks)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int i;
    virNetworkPtr *nets = NULL;
    int nnets = 0;
    unsigned int flags = 0;

    GET_CONNECTION_FROM_ARGS("r|l", &zconn, &flags);

    if ((nnets = virConnectListAllNetworks(conn->conn, &nets, flags)) < 0)
        RETURN_FALSE;

    DPRINTF("%s: Found %d networks\n", PHPFUNC, nnets);

    array_init(return_value);
    for (i = 0; i < nnets; i++) {
        virNetworkPtr net = nets[i];
        const char *name;

        if (!(name = virNetworkGetName(net)))
            goto error;

        VIRT_ADD_NEXT_INDEX_STRING(return_value, name);
    }

    for (i = 0; i < nnets; i++)
        virNetworkFree(nets[i]);
    free(nets);

    return;

 error:
    for (i = 0; i < nnets; i++)
        virNetworkFree(nets[i]);
    free(nets);
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_network_get_dhcp_leases
 * Description:     Function is fetching leases info of guests in the specified network
 * Arguments:       @res [resource]: libvirt network resource
 *                  @mac [string]: Optional ASCII formatted MAC address of an interface
 *                  @flags [int]: Extra flags, not used yet, so callers should always pass 0
 * Returns:         dhcp leases info array on success, FALSE on error
 */
PHP_FUNCTION(libvirt_network_get_dhcp_leases)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    char *mac = NULL;
    strsize_t mac_len;
    zend_long flags = 0;

    virNetworkDHCPLeasePtr *leases = NULL;
    int nleases = 0;

    int i;

    int done = 0;

    GET_NETWORK_FROM_ARGS("r|sl", &znetwork, &mac, &mac_len, &flags);

    if ((nleases = virNetworkGetDHCPLeases(network->network, mac, &leases, flags)) < 0) {
        set_error_if_unset("Failed to get leases info" TSRMLS_CC);
        goto cleanup;
    }

    array_init(return_value);

    for (i = 0; i < nleases; i++) {
        virNetworkDHCPLeasePtr lease;
        lease = leases[i];
        zval *arr;
        VIRT_ARRAY_INIT(arr);
        add_assoc_long(arr, "time", (long) lease->expirytime);
        VIRT_ADD_ASSOC_STRING_WITH_NULL_POINTER_CHECK(arr, "mac", lease->mac);
        VIRT_ADD_ASSOC_STRING_WITH_NULL_POINTER_CHECK(arr, "ipaddr", lease->ipaddr);
        VIRT_ADD_ASSOC_STRING_WITH_NULL_POINTER_CHECK(arr, "hostname", lease->hostname);
        VIRT_ADD_ASSOC_STRING_WITH_NULL_POINTER_CHECK(arr, "clientid", lease->clientid);
        add_index_zval(return_value, i, arr);
    }

    done = 1;

    cleanup:
    if (leases) {
        for (i = 0; i < nleases; i++)
            virNetworkDHCPLeaseFree(leases[i]);
        VIR_FREE(leases);
    }
    if (!done) {
        RETURN_FALSE
    }
}
/*
 * libvirt-nodedev.c: The PHP bindings to libvirt nodedev API
 *
 * See COPYING for the license of this software
 */

#include <libvirt/libvirt.h>

#include "libvirt-php.h"
#include "libvirt-nodedev.h"

DEBUG_INIT("nodedev");

void
php_libvirt_nodedev_dtor(virt_resource *rsrc TSRMLS_DC)
{
    php_libvirt_nodedev *nodedev = (php_libvirt_nodedev *)rsrc->ptr;
    int rv = 0;

    if (nodedev != NULL) {
        if (nodedev->device != NULL) {
            if (!check_resource_allocation(nodedev->conn->conn, INT_RESOURCE_NODEDEV, nodedev->device TSRMLS_CC)) {
                nodedev->device = NULL;
                efree(nodedev);
                return;
            }
            rv = virNodeDeviceFree(nodedev->device);
            if (rv != 0) {
                DPRINTF("%s: virNodeDeviceFree(%p) returned %d (%s)\n", __FUNCTION__, nodedev->device, rv, LIBVIRT_G(last_error));
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "virStorageVolFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virNodeDeviceFree(%p) completed successfully\n", __FUNCTION__, nodedev->device);
                resource_change_counter(INT_RESOURCE_NODEDEV, nodedev->conn->conn, nodedev->device, 0 TSRMLS_CC);
            }
            nodedev->device = NULL;
        }
        efree(nodedev);
    }
}

/*
 * Function name:   libvirt_nodedev_get
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the node device by it's name
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @name [string]: name of the nodedev to get resource
 * Returns:         libvirt nodedev resource
 */
PHP_FUNCTION(libvirt_nodedev_get)
{
    php_libvirt_connection *conn = NULL;
    php_libvirt_nodedev *res_dev = NULL;
    virNodeDevice *dev;
    zval *zconn;
    char *name;
    strsize_t name_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &name, &name_len);

    if ((dev = virNodeDeviceLookupByName(conn->conn, name)) == NULL) {
        set_error("Cannot get find requested node device" TSRMLS_CC);
        RETURN_FALSE;
    }

    res_dev = (php_libvirt_nodedev *)emalloc(sizeof(php_libvirt_nodedev));
    res_dev->device = dev;
    res_dev->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_dev->device);
    resource_change_counter(INT_RESOURCE_NODEDEV, conn->conn, res_dev->device, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_dev, le_libvirt_nodedev);
}

/*
 * Function name:   libvirt_nodedev_capabilities
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list node devices by capabilities
 * Arguments:       @res [resource]: libvirt nodedev resource
 * Returns:         nodedev capabilities array
 */
PHP_FUNCTION(libvirt_nodedev_capabilities)
{
    php_libvirt_nodedev *nodedev = NULL;
    zval *znodedev;
    int count = -1;
    int expectedcount = -1;
    char **names;
    int i;

    GET_NODEDEV_FROM_ARGS("r", &znodedev);

    if ((expectedcount = virNodeDeviceNumOfCaps(nodedev->device)) < 0)
        RETURN_FALSE;
    names = (char **)emalloc(expectedcount*sizeof(char *));
    count = virNodeDeviceListCaps(nodedev->device, names, expectedcount);
    if ((count != expectedcount) || (count < 0))
        RETURN_FALSE;

    array_init(return_value);
    for (i = 0; i < count; i++) {
        VIRT_ADD_NEXT_INDEX_STRING(return_value, names[i]);
        VIR_FREE(names[i]);
    }

    efree(names);
}

/*
 * Function name:   libvirt_nodedev_get_xml_desc
 * Since version:   0.4.1(-1), changed 0.4.2
 * Description:     Function is used to get the node device's XML description
 * Arguments:       @res [resource]: libvirt nodedev resource
 *                  @xpath [string]: optional xPath expression string to get just this entry, can be NULL
 * Returns:         nodedev XML description string or result of xPath expression
 */
PHP_FUNCTION(libvirt_nodedev_get_xml_desc)
{
    php_libvirt_nodedev *nodedev = NULL;
    zval *znodedev;
    char *tmp = NULL;
    char *xml = NULL;
    char *xpath = NULL;
    strsize_t xpath_len;
    int retval = -1;

    GET_NODEDEV_FROM_ARGS("r|s", &znodedev, &xpath, &xpath_len);
    if (xpath_len < 1)
        xpath = NULL;

    xml = virNodeDeviceGetXMLDesc(nodedev->device, 0);
    if (!xml) {
        set_error("Cannot get the device XML information" TSRMLS_CC);
        RETURN_FALSE;
    }

    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if ((tmp == NULL) || (retval < 0))
        VIRT_RETVAL_STRING(xml);
    else
        VIRT_RETVAL_STRING(tmp);

    VIR_FREE(xml);
    VIR_FREE(tmp);
}

/*
 * Function name:   libvirt_nodedev_get_information
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the node device's information
 * Arguments:       @res [resource]: libvirt nodedev resource
 * Returns:         nodedev information array
 */
PHP_FUNCTION(libvirt_nodedev_get_information)
{
    php_libvirt_nodedev *nodedev = NULL;
    zval *znodedev;
    int retval = -1;
    char *xml = NULL;
    char *tmp = NULL;
    char *cap = NULL;

    GET_NODEDEV_FROM_ARGS("r", &znodedev);

    xml = virNodeDeviceGetXMLDesc(nodedev->device, 0);
    if (!xml) {
        set_error("Cannot get the device XML information" TSRMLS_CC);
        RETURN_FALSE;
    }

    array_init(return_value);

    /* Get name */
    tmp = get_string_from_xpath(xml, "//device/name", NULL, &retval);
    if (tmp == NULL) {
        set_error("Invalid XPath node for device name" TSRMLS_CC);
        goto error;
    }

    if (retval < 0) {
        set_error("Cannot get XPath expression result for device name" TSRMLS_CC);
        goto error;
    }

    VIRT_ADD_ASSOC_STRING(return_value, "name", tmp);

    /* Get parent name */
    VIR_FREE(tmp);
    tmp = get_string_from_xpath(xml, "//device/parent", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "parent", tmp);

    /* Get capability */
    cap = get_string_from_xpath(xml, "//device/capability/@type", NULL, &retval);
    if ((cap != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "capability", cap);

    /* System capability is having hardware and firmware sub-blocks */
    if (strcmp(cap, "system") == 0) {
        /* Get hardware vendor */
        VIR_FREE(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/hardware/vendor", NULL, &retval);
        if ((tmp != NULL) && (retval > 0))
            VIRT_ADD_ASSOC_STRING(return_value, "hardware_vendor", tmp);

        /* Get hardware version */
        VIR_FREE(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/hardware/version", NULL, &retval);
        if ((tmp != NULL) && (retval > 0))
            VIRT_ADD_ASSOC_STRING(return_value, "hardware_version", tmp);

        /* Get hardware serial */
        VIR_FREE(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/hardware/serial", NULL, &retval);
        if ((tmp != NULL) && (retval > 0))
            VIRT_ADD_ASSOC_STRING(return_value, "hardware_serial", tmp);

        /* Get hardware UUID */
        VIR_FREE(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/hardware/uuid", NULL, &retval);
        if (tmp != NULL)
            VIRT_ADD_ASSOC_STRING(return_value, "hardware_uuid", tmp);

        /* Get firmware vendor */
        VIR_FREE(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/firmware/vendor", NULL, &retval);
        if ((tmp != NULL) && (retval > 0))
            VIRT_ADD_ASSOC_STRING(return_value, "firmware_vendor", tmp);

        /* Get firmware version */
        VIR_FREE(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/firmware/version", NULL, &retval);
        if ((tmp != NULL) && (retval > 0))
            VIRT_ADD_ASSOC_STRING(return_value, "firmware_version", tmp);

        /* Get firmware release date */
        VIR_FREE(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/firmware/release_date", NULL, &retval);
        if ((tmp != NULL) && (retval > 0))
            VIRT_ADD_ASSOC_STRING(return_value, "firmware_release_date", tmp);
    }

    /* Get product_id */
    VIR_FREE(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/product/@id", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "product_id", tmp);

    /* Get product_name */
    VIR_FREE(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/product", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "product_name", tmp);

    /* Get vendor_id */
    VIR_FREE(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/vendor/@id", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "vendor_id", tmp);

    /* Get vendor_name */
    VIR_FREE(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/vendor", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "vendor_name", tmp);

    /* Get driver name */
    VIR_FREE(tmp);
    tmp = get_string_from_xpath(xml, "//device/driver/name", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "driver_name", tmp);

    /* Get driver name */
    VIR_FREE(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/interface", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "interface_name", tmp);

    /* Get driver name */
    VIR_FREE(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/address", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "address", tmp);

    /* Get driver name */
    VIR_FREE(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/capability/@type", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "capabilities", tmp);

    VIR_FREE(cap);
    VIR_FREE(tmp);
    VIR_FREE(xml);
    return;

 error:
    VIR_FREE(cap);
    VIR_FREE(tmp);
    VIR_FREE(xml);
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_list_nodedevs
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list node devices on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @cap [string]: optional capability string
 * Returns:         libvirt nodedev names array for the connection
 */
PHP_FUNCTION(libvirt_list_nodedevs)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int count = -1;
    int expectedcount = -1;
    char *cap = NULL;
    char **names;
    int i;
    strsize_t cap_len;

    GET_CONNECTION_FROM_ARGS("r|s", &zconn, &cap, &cap_len);

    if ((expectedcount = virNodeNumOfDevices(conn->conn, cap, 0)) < 0)
        RETURN_FALSE;
    names = (char **)emalloc(expectedcount*sizeof(char *));
    count = virNodeListDevices(conn->conn, cap, names, expectedcount, 0);
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

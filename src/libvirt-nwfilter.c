/*
 * libvirt-nwfilter.c: The PHP bindings to libvirt NWFilter API
 *
 * See COPYING for the license of this software
 */

#include <config.h>

#include <libvirt/libvirt.h>

#include "libvirt-php.h"
#include "libvirt-nwfilter.h"

DEBUG_INIT("nwfilter");

int le_libvirt_nwfilter;

void
php_libvirt_nwfilter_dtor(zend_resource *rsrc)
{
    php_libvirt_nwfilter *nwfilter = (php_libvirt_nwfilter *) rsrc->ptr;
    int rv = 0;

    if (nwfilter != NULL) {
        if (nwfilter->nwfilter != NULL) {
            if (!check_resource_allocation(NULL, INT_RESOURCE_NWFILTER, nwfilter->nwfilter)) {
                nwfilter->nwfilter = NULL;
                efree(nwfilter);

                return;
            }
            rv = virNWFilterFree(nwfilter->nwfilter);
            if (rv != 0) {
                DPRINTF("%s: virNWFilterFree(%p) returned %d\n", __FUNCTION__, nwfilter->nwfilter, rv);
                php_error_docref(NULL, E_WARNING, "virNWFilterFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virNWFilterFree(%p) completed successfully\n", __FUNCTION__, nwfilter->nwfilter);
                resource_change_counter(INT_RESOURCE_NWFILTER, nwfilter->conn->conn, nwfilter->nwfilter, 0);
            }
            nwfilter->nwfilter = NULL;
        }
        efree(nwfilter);
    }
}

/*
 * Function name:   libvirt_nwfilter_define_xml
 * Since version:   0.5.4
 * Description:     Function is used to define a new nwfilter based on the XML description
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @xml [string]: XML string definition of nwfilter to be defined
 * Returns:         libvirt nwfilter resource of newly defined nwfilter
 */
PHP_FUNCTION(libvirt_nwfilter_define_xml)
{
    php_libvirt_connection *conn = NULL;
    php_libvirt_nwfilter *res_nwfilter = NULL;
    virNWFilter *nwfilter;
    zval *zconn;
    char *xml = NULL;
    size_t xml_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &xml, &xml_len);

    if ((nwfilter = virNWFilterDefineXML(conn->conn, xml)) == NULL) {
        set_error_if_unset("Cannot define a new NWFilter");
        RETURN_FALSE;
    }

    res_nwfilter = (php_libvirt_nwfilter *) emalloc(sizeof(php_libvirt_nwfilter));
    res_nwfilter->nwfilter = nwfilter;
    res_nwfilter->conn = conn;

    resource_change_counter(INT_RESOURCE_NWFILTER, conn->conn,
                            res_nwfilter->nwfilter, 1);

    VIRT_REGISTER_RESOURCE(res_nwfilter, le_libvirt_nwfilter);
}

/*
 * Function name:   libvirt_nwfilter_undefine
 * Since version:   0.5.4
 * Description:     Function is used to undefine already defined nwfilter
 * Arguments:       @res [resource]: libvirt nwfilter resource
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_nwfilter_undefine)
{
    php_libvirt_nwfilter *nwfilter = NULL;
    zval *znwfilter;

    GET_NWFILTER_FROM_ARGS("r", &znwfilter);

    if (virNWFilterUndefine(nwfilter->nwfilter) != 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_nwfilter_get_xml_desc
 * Since version:   0.5.4
 * Description:     Function is used to get the XML description for the nwfilter
 * Arguments:       @res [resource]: libvirt nwfilter resource
 *                  @xpath [string]: optional xPath expression string to get just this entry, can be NULL
 * Returns:         nwfilter XML string or result of xPath expression
 */
PHP_FUNCTION(libvirt_nwfilter_get_xml_desc)
{
    php_libvirt_nwfilter *nwfilter = NULL;
    zval *znwfilter;
    char *xml = NULL;
    char *xpath = NULL;
    char *tmp;
    size_t xpath_len = 0;
    int retval = -1;

    GET_NWFILTER_FROM_ARGS("r|s", &znwfilter, &xpath, &xpath_len);

    if (xpath_len < 1)
        xpath = NULL;

    xml = virNWFilterGetXMLDesc(nwfilter->nwfilter, 0);

    if (xml == NULL) {
        set_error_if_unset("Cannot get nwfilter XML");
        RETURN_FALSE;
    }

    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);

    if (tmp == NULL || retval < 0)
        VIRT_RETVAL_STRING(xml);
    else
        VIRT_RETVAL_STRING(tmp);

    VIR_FREE(xml);
    VIR_FREE(tmp);
}

/*
 * Function name:   libvirt_nwfilter_get_uuid_string
 * Since version:   0.5.4
 * Description:     Function is used to get nwfilter's UUID in string format
 * Arguments:       @res [resource]: libvirt nwfilter resource
 * Returns:         nwfilter UUID string or FALSE on failure
 */
PHP_FUNCTION(libvirt_nwfilter_get_uuid_string)
{
    php_libvirt_nwfilter *nwfilter = NULL;
    zval *znwfilter;
    char uuid[VIR_UUID_STRING_BUFLEN];
    int ret = -1;

    GET_NWFILTER_FROM_ARGS("r", &znwfilter);

    ret = virNWFilterGetUUIDString(nwfilter->nwfilter, uuid);

    DPRINTF("%s: virNWFilterGetUUIDString(%p) returned %d (%s)\n", PHPFUNC,
            nwfilter->nwfilter, ret, uuid);

    if (ret != 0)
        RETURN_FALSE;

    VIRT_RETURN_STRING(uuid);
}

/*
 * Function name:   libvirt_nwfilter_get_uuid
 * Since version:   0.5.3
 * Description:     Function is used to get nwfilter's UUID in binary format
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         nwfilter UUID in binary format or FALSE on failure
 */
PHP_FUNCTION(libvirt_nwfilter_get_uuid)
{
    php_libvirt_nwfilter *nwfilter = NULL;
    zval *znwfilter;
    char *uuid = NULL;
    int ret = -1;

    GET_NWFILTER_FROM_ARGS("r", &znwfilter);

    uuid = (char *) emalloc(VIR_UUID_BUFLEN);
    ret = virNWFilterGetUUID(nwfilter->nwfilter, (unsigned char *) uuid);

    DPRINTF("%s: virNWFilterUUID(%p, %p) returned %d\n", PHPFUNC,
            nwfilter->nwfilter, uuid, ret);

    if (ret != 0)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(uuid);
    efree(uuid);
}

/*
 * Function name:   libvirt_nwfilter_get_name
 * Since version:   0.5.4
 * Description:     Function is used to get nwfilter's name
 * Arguments:       @res [resource]: libvirt nwfilter resource
 * Returns:         nwfilter name string or FALSE on failure
 */
PHP_FUNCTION(libvirt_nwfilter_get_name)
{
    php_libvirt_nwfilter *nwfilter = NULL;
    zval *znwfilter;
    const char *name = NULL;

    GET_NWFILTER_FROM_ARGS("r", &znwfilter);
    name = virNWFilterGetName(nwfilter->nwfilter);

    DPRINTF("%s: virNWFilterGetName(%p) returned %s\n", PHPFUNC,
            nwfilter->nwfilter, name);

    if (name == NULL)
        RETURN_FALSE;

    /* name should not be freed as its lifetime is the same as nwfilter resource */
    VIRT_RETURN_STRING(name);
}

/*
 * Function name:   libvirt_nwfilter_lookup_by_name
 * Since version:   0.5.4
 * Description:     This functions is used to lookup for the nwfilter by it's name
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @name [string]: name of the nwfilter to get the resource
 * Returns:         libvirt nwfilter resource
 */
PHP_FUNCTION(libvirt_nwfilter_lookup_by_name)
{
    php_libvirt_nwfilter *res_nwfilter = NULL;
    php_libvirt_connection *conn = NULL;
    virNWFilterPtr nwfilter = NULL;
    zval *zconn;
    size_t name_len;
    char *name = NULL;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &name, &name_len);

    if (name == NULL || name_len < 1)
        RETURN_FALSE;

    nwfilter = virNWFilterLookupByName(conn->conn, name);

    if (nwfilter == NULL)
        RETURN_FALSE;

    res_nwfilter = (php_libvirt_nwfilter *) emalloc(sizeof(php_libvirt_nwfilter));
    res_nwfilter->conn = conn;
    res_nwfilter->nwfilter = nwfilter;

    resource_change_counter(INT_RESOURCE_NWFILTER, conn->conn,
                            res_nwfilter->nwfilter, 1);

    VIRT_REGISTER_RESOURCE(res_nwfilter, le_libvirt_nwfilter);
}

/*
 * Function name:   libvirt_nwfilter_lookup_by_uuid_string
 * Since version:   0.5.4
 * Description:     Function is used to lookup for nwfilter identified by UUID string
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @uuid [string]: UUID string to look for nwfilter
 * Returns:         libvirt nwfilter resource
 */
PHP_FUNCTION(libvirt_nwfilter_lookup_by_uuid_string)
{
    php_libvirt_nwfilter *res_nwfilter = NULL;
    php_libvirt_connection *conn = NULL;
    virNWFilterPtr nwfilter = NULL;
    zval *zconn;
    char *uuid = NULL;
    size_t uuid_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &uuid, &uuid_len);

    if (uuid == NULL || uuid_len < 1)
        RETURN_FALSE;

    nwfilter = virNWFilterLookupByUUIDString(conn->conn, uuid);

    if (nwfilter == NULL)
        RETURN_FALSE;

    res_nwfilter = (php_libvirt_nwfilter *) emalloc(sizeof(php_libvirt_nwfilter));
    res_nwfilter->conn = conn;
    res_nwfilter->nwfilter = nwfilter;

    resource_change_counter(INT_RESOURCE_NWFILTER, conn->conn,
                            res_nwfilter->nwfilter, 1);

    VIRT_REGISTER_RESOURCE(res_nwfilter, le_libvirt_nwfilter);
}

/*
 * Function name:   libvirt_nwfilter_lookup_by_uuid
 * Since version:   0.5.4
 * Description:     Function is used to lookup for nwfilter by it's UUID in the binary format
 * Arguments:       @res [resource]: libvirt connection resource from libvirt_connect()
 *                  @uuid [string]: binary defined UUID to look for
 * Returns:         libvirt nwfilter resource
 */
PHP_FUNCTION(libvirt_nwfilter_lookup_by_uuid)
{
    php_libvirt_nwfilter *res_nwfilter = NULL;
    php_libvirt_connection *conn = NULL;
    virNWFilterPtr nwfilter = NULL;
    zval *zconn;
    size_t uuid_len;
    char *uuid = NULL;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &uuid, &uuid_len);

    if ((uuid == NULL) || (uuid_len < 1))
        RETURN_FALSE;

    nwfilter = virNWFilterLookupByUUID(conn->conn, uuid);

    if (nwfilter == NULL)
        RETURN_FALSE;

    res_nwfilter = (php_libvirt_nwfilter *) emalloc(sizeof(php_libvirt_nwfilter));
    res_nwfilter->conn = conn;
    res_nwfilter->nwfilter = nwfilter;

    resource_change_counter(INT_RESOURCE_NWFILTER, conn->conn,
                            res_nwfilter->nwfilter, 1);

    VIRT_REGISTER_RESOURCE(res_nwfilter, le_libvirt_nwfilter);
}

/*
 * Function name:   libvirt_list_all_nwfilters
 * Since version:   0.5.4
 * Description:     Function is used to list nwfilters on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt nwfilter resources array for the connection
 */
PHP_FUNCTION(libvirt_list_all_nwfilters)
{
    php_libvirt_nwfilter *res_nwfilter;
    php_libvirt_connection *conn = NULL;
    virNWFilterPtr *filters = NULL;
    virNWFilterPtr nwfilter = NULL;
    zval *zconn;
    int count = -1;
    size_t i = 0;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    /* in current libvirt version, flags are not used for this, so passing 0 */
    if ((count = virConnectListAllNWFilters(conn->conn, &filters, 0)) < 0)
        RETURN_FALSE;

    DPRINTF("%s: Found %d nwfilters\n", PHPFUNC, count);

    array_init(return_value);

    for (i = 0; i < count; i++) {
        nwfilter = filters[i];
        res_nwfilter = (php_libvirt_nwfilter *) emalloc(sizeof(php_libvirt_nwfilter));
        res_nwfilter->nwfilter = nwfilter;
        res_nwfilter->conn = conn;

        resource_change_counter(INT_RESOURCE_NWFILTER, conn->conn,
                                res_nwfilter->nwfilter, 1);
        VIRT_REGISTER_LIST_RESOURCE(nwfilter);
    }
    VIR_FREE(filters);
}

/*
 * Function name:   libvirt_list_nwfilters
 * Since version:   0.5.4
 * Description:     Function is used to list nwfilters on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt nwfilter names array for the connection
 */
PHP_FUNCTION(libvirt_list_nwfilters)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int i;
    virNWFilterPtr *nwfilters = NULL;
    int nnwfilters = 0;
    const unsigned int flags = 0;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((nnwfilters = virConnectListAllNWFilters(conn->conn, &nwfilters, flags)) < 0)
        RETURN_FALSE;

    DPRINTF("%s: Found %d nwfilters\n", PHPFUNC, nnwfilters);

    array_init(return_value);
    for (i = 0; i < nnwfilters; i++) {
        virNWFilterPtr nwfilter = nwfilters[i];
        const char *name;

        if (!(name = virNWFilterGetName(nwfilter)))
            goto error;

        VIRT_ADD_NEXT_INDEX_STRING(return_value, name);
    }

    for (i = 0; i < nnwfilters; i++)
        virNWFilterFree(nwfilters[i]);
    free(nwfilters);

    return;

 error:
    for (i = 0; i < nnwfilters; i++)
        virNWFilterFree(nwfilters[i]);
    free(nwfilters);
    RETURN_FALSE;
}

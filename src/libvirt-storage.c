/*
 * libvirt-storage.c: The PHP bindings to libvirt storage API
 *
 * See COPYING for the license of this software
 */

#include <libvirt/libvirt.h>

#include "libvirt-php.h"
#include "libvirt-stream.h"
#include "libvirt-storage.h"

DEBUG_INIT("storage");

int le_libvirt_storagepool;
int le_libvirt_volume;

void
php_libvirt_storagepool_dtor(virt_resource *rsrc TSRMLS_DC)
{
    php_libvirt_storagepool *pool = (php_libvirt_storagepool *)rsrc->ptr;
    int rv = 0;

    if (pool != NULL) {
        if (pool->pool != NULL) {
            if (!check_resource_allocation(NULL, INT_RESOURCE_STORAGEPOOL, pool->pool TSRMLS_CC)) {
                pool->pool = NULL;
                efree(pool);
                return;
            }
            rv = virStoragePoolFree(pool->pool);
            if (rv != 0) {
                DPRINTF("%s: virStoragePoolFree(%p) returned %d (%s)\n", __FUNCTION__, pool->pool, rv, LIBVIRT_G(last_error));
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "virStoragePoolFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virStoragePoolFree(%p) completed successfully\n", __FUNCTION__, pool->pool);
                resource_change_counter(INT_RESOURCE_STORAGEPOOL, pool->conn->conn, pool->pool, 0 TSRMLS_CC);
            }
            pool->pool = NULL;
        }
        efree(pool);
    }
}

void
php_libvirt_volume_dtor(virt_resource *rsrc TSRMLS_DC)
{
    php_libvirt_volume *volume = (php_libvirt_volume *)rsrc->ptr;
    int rv = 0;

    if (volume != NULL) {
        if (volume->volume != NULL) {
            if (!check_resource_allocation(NULL, INT_RESOURCE_VOLUME, volume->volume TSRMLS_CC)) {
                volume->volume = NULL;
                efree(volume);
                return;
            }
            rv = virStorageVolFree(volume->volume);
            if (rv != 0) {
                DPRINTF("%s: virStorageVolFree(%p) returned %d (%s)\n", __FUNCTION__, volume->volume, rv, LIBVIRT_G(last_error));
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "virStorageVolFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virStorageVolFree(%p) completed successfully\n", __FUNCTION__, volume->volume);
                resource_change_counter(INT_RESOURCE_VOLUME, volume->conn->conn, volume->volume, 0 TSRMLS_CC);
            }
            volume->volume = NULL;
        }
        efree(volume);
    }
}

/*
 * Function name:   libvirt_storagepool_lookup_by_name
 * Since version:   0.4.1(-1)
 * Description:     Function is used to lookup for storage pool by it's name
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @name [string]: storage pool name
 * Returns:         libvirt storagepool resource
 */
PHP_FUNCTION(libvirt_storagepool_lookup_by_name)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    strsize_t name_len;
    char *name = NULL;
    virStoragePoolPtr pool = NULL;
    php_libvirt_storagepool *res_pool;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &name, &name_len);

    if ((name == NULL) || (name_len < 1))
        RETURN_FALSE;
    pool = virStoragePoolLookupByName(conn->conn, name);
    DPRINTF("%s: virStoragePoolLookupByName(%p, %s) returned %p\n", PHPFUNC, conn->conn, name, pool);
    if (pool == NULL)
        RETURN_FALSE;

    res_pool = (php_libvirt_storagepool *)emalloc(sizeof(php_libvirt_storagepool));
    res_pool->pool = pool;
    res_pool->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_pool->pool);
    resource_change_counter(INT_RESOURCE_STORAGEPOOL, conn->conn, res_pool->pool, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_pool, le_libvirt_storagepool);
}

/*
 * Function name:   libvirt_storagepool_lookup_by_volume
 * Since version:   0.4.1(-1)
 * Description:     Function is used to lookup for storage pool by a volume
 * Arguments:       @res [volume]: volume resource of storage pool
 * Returns:         libvirt storagepool resource
 */
PHP_FUNCTION(libvirt_storagepool_lookup_by_volume)
{
    php_libvirt_volume *volume;
    zval *zvolume;
    virStoragePoolPtr pool = NULL;
    php_libvirt_storagepool *res_pool;

    GET_VOLUME_FROM_ARGS("r", &zvolume);

    pool = virStoragePoolLookupByVolume(volume->volume);
    DPRINTF("%s: virStoragePoolLookupByVolume(%p) returned %p\n", PHPFUNC, volume->volume, pool);
    if (pool == NULL)
        RETURN_FALSE;

    res_pool = (php_libvirt_storagepool *)emalloc(sizeof(php_libvirt_storagepool));
    res_pool->pool = pool;
    res_pool->conn = volume->conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_pool->pool);
    resource_change_counter(INT_RESOURCE_STORAGEPOOL, res_pool->conn->conn, res_pool->pool, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_pool, le_libvirt_storagepool);
}

/*
 * Function name:   libvirt_storagepool_list_volumes
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list volumes in the specified storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         list of storage volume names in the storage pool in an array using default keys (indexes)
 */
PHP_FUNCTION(libvirt_storagepool_list_volumes)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    char **names = NULL;
    int expectedcount = -1;
    int i;
    int count = -1;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    if ((expectedcount = virStoragePoolNumOfVolumes(pool->pool)) < 0)
        RETURN_FALSE;

    DPRINTF("%s: virStoragePoolNumOfVolumes(%p) returned %d\n", PHPFUNC, pool->pool, expectedcount);
    names = (char **)emalloc(expectedcount*sizeof(char *));

    count = virStoragePoolListVolumes(pool->pool, names, expectedcount);
    DPRINTF("%s: virStoragePoolListVolumes(%p, %p, %d) returned %d\n", PHPFUNC, pool->pool, names, expectedcount, count);
    array_init(return_value);

    if ((count != expectedcount) || (count < 0))
        RETURN_FALSE;
    for (i = 0; i < count; i++) {
        VIRT_ADD_NEXT_INDEX_STRING(return_value,  names[i]);
        VIR_FREE(names[i]);
    }

    efree(names);
}

/*
 * Function name:   libvirt_storagepool_get_info
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get information about the storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         storage pool information array of state, capacity, allocation and available space
 */
PHP_FUNCTION(libvirt_storagepool_get_info)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    virStoragePoolInfo poolInfo;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    retval = virStoragePoolGetInfo(pool->pool, &poolInfo);
    DPRINTF("%s: virStoragePoolGetInfo(%p, <info>) returned %d\n", PHPFUNC, pool->pool, retval);
    if (retval != 0)
        RETURN_FALSE;

    array_init(return_value);

    // @todo: fix the long long returns
    LONGLONG_INIT;
    add_assoc_long(return_value, "state", (long)poolInfo.state);
    LONGLONG_ASSOC(return_value, "capacity", poolInfo.capacity);
    LONGLONG_ASSOC(return_value, "allocation", poolInfo.allocation);
    LONGLONG_ASSOC(return_value, "available", poolInfo.available);
}

/*
 * Function name:   libvirt_storagepool_get_uuid_string
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get storage pool by UUID string
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         storagepool UUID string
 */
PHP_FUNCTION(libvirt_storagepool_get_uuid_string)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    char *uuid;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    uuid = (char *)emalloc(VIR_UUID_STRING_BUFLEN);
    retval = virStoragePoolGetUUIDString(pool->pool, uuid);
    DPRINTF("%s: virStoragePoolGetUUIDString(%p, %p) returned %d (%s)\n", PHPFUNC, pool->pool, uuid, retval, uuid);
    if (retval != 0)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(uuid);
    efree(uuid);
}

/*
 * Function name:   libvirt_storagepool_get_name
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get storage pool name from the storage pool resource
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         storagepool name string
 */
PHP_FUNCTION(libvirt_storagepool_get_name)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    const char *name = NULL;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    name = virStoragePoolGetName(pool->pool);
    DPRINTF("%s: virStoragePoolGetName(%p) returned %s\n", PHPFUNC, pool->pool, name);
    if (name == NULL)
        RETURN_FALSE;

    VIRT_RETURN_STRING(name);
}

/*
 * Function name:   libvirt_storagepool_lookup_by_uuid_string
 * Since version:   0.4.1(-1)
 * Description:     Function is used to lookup for storage pool identified by UUID string
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @uuid [string]: UUID string to look for storagepool
 * Returns:         libvirt storagepool resource
 */
PHP_FUNCTION(libvirt_storagepool_lookup_by_uuid_string)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    char *uuid = NULL;
    strsize_t uuid_len;
    virStoragePoolPtr storage = NULL;
    php_libvirt_storagepool *res_pool;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &uuid, &uuid_len);

    if ((uuid == NULL) || (uuid_len < 1))
        RETURN_FALSE;

    storage = virStoragePoolLookupByUUIDString(conn->conn, uuid);
    DPRINTF("%s: virStoragePoolLookupByUUIDString(%p, %s) returned %p\n", PHPFUNC, conn->conn, uuid, storage);
    if (storage == NULL)
        RETURN_FALSE;

    res_pool = (php_libvirt_storagepool *)emalloc(sizeof(php_libvirt_storagepool));
    res_pool->pool = storage;
    res_pool->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_pool->pool);
    resource_change_counter(INT_RESOURCE_STORAGEPOOL, conn->conn, res_pool->pool, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_pool, le_libvirt_storagepool);
}

/*
 * Function name:   libvirt_storagepool_get_xml_desc
 * Since version:   0.4.1(-1), changed 0.4.2
 * Description:     Function is used to get the XML description for the storage pool identified by res
 * Arguments:       @res [resource]: libvirt storagepool resource
 *                  @xpath [string]: optional xPath expression string to get just this entry, can be NULL
 * Returns:         storagepool XML description string or result of xPath expression
 */
PHP_FUNCTION(libvirt_storagepool_get_xml_desc)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    char *xml;
    char *xpath = NULL;
    char *tmp = NULL;
    zend_long flags = 0;
    strsize_t xpath_len;
    int retval = -1;

    GET_STORAGEPOOL_FROM_ARGS("r|s", &zpool, &xpath, &xpath_len, &flags);
    if (xpath_len < 1)
        xpath = NULL;

    DPRINTF("%s: pool = %p, flags = %ld, xpath = %s\n", PHPFUNC, pool->pool, flags, xpath);

    xml = virStoragePoolGetXMLDesc(pool->pool, flags);
    if (xml == NULL) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
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
 * Function name:   libvirt_storagepool_define_xml
 * Since version:   0.4.1(-1)
 * Description:     Function is used to define the storage pool from XML string and return it's resource
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @xml [string]: XML string definition of storagepool
 *                  @flags [int]: flags to define XML
 * Returns:         libvirt storagepool resource
 */
PHP_FUNCTION(libvirt_storagepool_define_xml)
{
    php_libvirt_storagepool *res_pool = NULL;
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    virStoragePoolPtr pool = NULL;
    char *xml;
    strsize_t xml_len;
    zend_long flags = 0;


    GET_CONNECTION_FROM_ARGS("rs|l", &zconn, &xml, &xml_len, &flags);

    pool = virStoragePoolDefineXML(conn->conn, xml, (unsigned int)flags);
    DPRINTF("%s: virStoragePoolDefineXML(%p, <xml>) returned %p\n", PHPFUNC, conn->conn, pool);
    if (pool == NULL)
        RETURN_FALSE;

    res_pool = (php_libvirt_storagepool *)emalloc(sizeof(php_libvirt_storagepool));
    res_pool->pool = pool;
    res_pool->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_pool->pool);
    resource_change_counter(INT_RESOURCE_STORAGEPOOL, conn->conn, res_pool->pool, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_pool, le_libvirt_storagepool);
}

/*
 * Function name:   libvirt_storagepool_undefine
 * Since version:   0.4.1(-1)
 * Description:     Function is used to undefine the storage pool identified by it's resource
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagepool_undefine)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    int retval = 0;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    retval = virStoragePoolUndefine(pool->pool);
    DPRINTF("%s: virStoragePoolUndefine(%p) returned %d\n", PHPFUNC, pool->pool, retval);
    if (retval != 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_storagepool_create
 * Since version:   0.4.1(-1)
 * Description:     Function is used to create/start the storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagepool_create)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    retval = virStoragePoolCreate(pool->pool, 0);
    DPRINTF("%s: virStoragePoolCreate(%p, 0) returned %d\n", PHPFUNC, pool->pool, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_storagepool_destroy
 * Since version:   0.4.1(-1)
 * Description:     Function is used to destroy the storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagepool_destroy)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    retval = virStoragePoolDestroy(pool->pool);
    DPRINTF("%s: virStoragePoolDestroy(%p) returned %d\n", PHPFUNC, pool->pool, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_storagepool_is_active
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get information whether storage pool is active or not
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         result of virStoragePoolIsActive
 */
PHP_FUNCTION(libvirt_storagepool_is_active)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    RETURN_LONG(virStoragePoolIsActive(pool->pool));
}

/*
 * Function name:   libvirt_storagepool_get_volume_count
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get storage volume count in the storage pool
 * Arguments:           @res [resource]: libvirt storagepool resource
 * Returns:             number of volumes in the pool
 */
PHP_FUNCTION(libvirt_storagepool_get_volume_count)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    RETURN_LONG(virStoragePoolNumOfVolumes(pool->pool));
}

/*
 * Function name:   libvirt_storagepool_refresh
 * Since version:   0.4.1(-1)
 * Description:     Function is used to refresh the storage pool information
 * Arguments:       @res [resource]: libvirt storagepool resource
 *                  @flags [int]: refresh flags
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagepool_refresh)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    zend_long flags = 0;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r|l", &zpool, &flags);

    retval = virStoragePoolRefresh(pool->pool, flags);
    DPRINTF("%s: virStoragePoolRefresh(%p, %ld) returned %d\n", PHPFUNC, pool->pool, flags, retval);
    if (retval < 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_storagepool_set_autostart
 * Since version:   0.4.1(-1)
 * Description:     Function is used to set autostart of the storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 *                  @flags [int]: flags to set autostart
 * Returns:         result on setting storagepool autostart value
 */
PHP_FUNCTION(libvirt_storagepool_set_autostart)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    zend_bool flags = 0;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("rb", &zpool, &flags);

    retval = virStoragePoolSetAutostart(pool->pool, flags);
    DPRINTF("%s: virStoragePoolSetAutostart(%p, %d) returned %d\n", PHPFUNC, pool->pool, flags, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_storagepool_get_autostart
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get autostart of the storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         TRUE for autostart enabled, FALSE for autostart disabled, FALSE with last_error set for error
 */
PHP_FUNCTION(libvirt_storagepool_get_autostart)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    int autostart;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    if (virStoragePoolGetAutostart(pool->pool, &autostart) == 0 &&
        autostart != 0)
        RETURN_TRUE;

    RETURN_FALSE;
}

/*
 * Function name:   libvirt_storagepool_build
 * Since version:   0.4.2
 * Description:     Function is used to Build the underlying storage pool, e.g. create the destination directory for NFS
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagepool_build)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    int flags = 0;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    retval = virStoragePoolBuild(pool->pool, flags);
    DPRINTF("%s: virStoragePoolBuild(%p, %d) returned %d\n", PHPFUNC, pool->pool, flags, retval);
    if (retval == 0)
        RETURN_TRUE;

    RETURN_FALSE;
}

/*
 * Function name:   libvirt_storagepool_delete
 * Since version:   0.4.6
 * Description:     Function is used to Delete the underlying storage pool, e.g. remove the destination directory for NFS
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagepool_delete)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    int flags = 0;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    retval = virStoragePoolDelete(pool->pool, flags);
    DPRINTF("%s: virStoragePoolDelete(%p, %d) returned %d\n", PHPFUNC, pool->pool, flags, retval);
    if (retval == 0)
        RETURN_TRUE;

    RETURN_FALSE;
}

/*
 * Function name:   libvirt_storagevolume_lookup_by_name
 * Since version:   0.4.1(-1)
 * Description:     Function is used to lookup for storage volume by it's name
 * Arguments:       @res [resource]: libvirt storagepool resource
 *                  @name [string]: name of the storage volume to look for
 * Returns:         libvirt storagevolume resource
 */
PHP_FUNCTION(libvirt_storagevolume_lookup_by_name)
{
    php_libvirt_storagepool *pool = NULL;
    php_libvirt_volume *res_volume;
    zval *zpool;
    strsize_t name_len;
    char *name = NULL;
    virStorageVolPtr volume = NULL;

    GET_STORAGEPOOL_FROM_ARGS("rs", &zpool, &name, &name_len);
    if ((name == NULL) || (name_len < 1))
        RETURN_FALSE;

    volume = virStorageVolLookupByName(pool->pool, name);
    DPRINTF("%s: virStorageVolLookupByName(%p, %s) returned %p\n", PHPFUNC, pool->pool, name, volume);
    if (volume == NULL)
        RETURN_FALSE;

    res_volume = (php_libvirt_volume *)emalloc(sizeof(php_libvirt_volume));
    res_volume->volume = volume;
    res_volume->conn   = pool->conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_volume->volume);
    resource_change_counter(INT_RESOURCE_VOLUME, pool->conn->conn, res_volume->volume, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_volume, le_libvirt_volume);
}

/*
 * Function name:   libvirt_storagevolume_lookup_by_path
 * Since version:   0.4.1(-2)
 * Description:     Function is used to lookup for storage volume by it's path
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @path [string]: path of the storage volume to look for
 * Returns:         libvirt storagevolume resource
 */
PHP_FUNCTION(libvirt_storagevolume_lookup_by_path)
{
    php_libvirt_connection *conn = NULL;
    php_libvirt_volume *res_volume;
    zval *zconn;
    strsize_t name_len;
    char *name = NULL;
    virStorageVolPtr volume = NULL;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &name, &name_len);
    if ((name == NULL) || (name_len < 1))
        RETURN_FALSE;

    volume = virStorageVolLookupByPath(conn->conn, name);
    DPRINTF("%s: virStorageVolLookupByPath(%p, %s) returned %p\n", PHPFUNC, conn->conn, name, volume);
    if (volume == NULL) {
        set_error_if_unset("Cannot find storage volume on requested path" TSRMLS_CC);
        RETURN_FALSE;
    }

    res_volume = (php_libvirt_volume *)emalloc(sizeof(php_libvirt_volume));
    res_volume->volume = volume;
    res_volume->conn   = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_volume->volume);
    resource_change_counter(INT_RESOURCE_VOLUME, conn->conn, res_volume->volume, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_volume, le_libvirt_volume);
}

/*
 * Function name:   libvirt_storagevolume_get_name
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get the storage volume name
 * Arguments:       @res [resource]: libvirt storagevolume resource
 * Returns:         storagevolume name
 */
PHP_FUNCTION(libvirt_storagevolume_get_name)
{
    php_libvirt_volume *volume = NULL;
    zval *zvolume;
    const char *retval;

    GET_VOLUME_FROM_ARGS("r", &zvolume);

    retval = virStorageVolGetName(volume->volume);
    DPRINTF("%s: virStorageVolGetName(%p) returned %s\n", PHPFUNC, volume->volume, retval);
    if (retval == NULL)
        RETURN_FALSE;

    VIRT_RETURN_STRING(retval);
}

/*
 * Function name:   libvirt_storagevolume_get_path
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get the  storage volume path
 * Arguments:       @res [resource]: libvirt storagevolume resource
 * Returns:         storagevolume path
 */
PHP_FUNCTION(libvirt_storagevolume_get_path)
{
    php_libvirt_volume *volume = NULL;
    zval *zvolume;
    char *retval;

    GET_VOLUME_FROM_ARGS("r", &zvolume);

    retval = virStorageVolGetPath(volume->volume);
    DPRINTF("%s: virStorageVolGetPath(%p) returned %s\n", PHPFUNC, volume->volume, retval);
    if (retval == NULL)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(retval);
    VIR_FREE(retval);
}

/*
 * Function name:   libvirt_storagevolume_get_info
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the storage volume information
 * Arguments:       @res [resource]: libvirt storagevolume resource
 * Returns:         storage volume information array of type, allocation and capacity
 */
PHP_FUNCTION(libvirt_storagevolume_get_info)
{
    php_libvirt_volume *volume = NULL;
    zval *zvolume;
    virStorageVolInfo volumeInfo;
    int retval;

    GET_VOLUME_FROM_ARGS("r", &zvolume);

    retval = virStorageVolGetInfo(volume->volume, &volumeInfo);
    DPRINTF("%s: virStorageVolGetInfo(%p, <info>) returned %d\n", PHPFUNC, volume->volume, retval);
    if (retval != 0)
        RETURN_FALSE;

    array_init(return_value);
    LONGLONG_INIT;
    add_assoc_long(return_value, "type", (long)volumeInfo.type);
    LONGLONG_ASSOC(return_value, "capacity", volumeInfo.capacity);
    LONGLONG_ASSOC(return_value, "allocation", volumeInfo.allocation);
}

/*
 * Function name:   libvirt_storagevolume_get_xml_desc
 * Since version:   0.4.1(-1), changed 0.4.2
 * Description:     Function is used to get the storage volume XML description
 * Arguments:       @res [resource]: libvirt storagevolume resource
 *                  @xpath [string]: optional xPath expression string to get just this entry, can be NULL
 *                  @flags [int]: optional flags
 * Returns:         storagevolume XML description or result of xPath expression
 */
PHP_FUNCTION(libvirt_storagevolume_get_xml_desc)
{
    php_libvirt_volume *volume = NULL;
    zval *zvolume;
    char *tmp = NULL;
    char *xml;
    char *xpath = NULL;
    strsize_t xpath_len;
    zend_long flags = 0;
    int retval = -1;

    GET_VOLUME_FROM_ARGS("rs|l", &zvolume, &xpath, &xpath_len, &flags);
    if (xpath_len < 1)
        xpath = NULL;

    DPRINTF("%s: volume = %p, xpath = %s, flags = %ld\n", PHPFUNC, volume->volume, xpath, flags);

    xml = virStorageVolGetXMLDesc(volume->volume, flags);
    if (xml == NULL) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
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
 * Function name:   libvirt_storagevolume_create_xml
 * Since version:   0.4.1(-1)
 * Description:     Function is used to create the new storage pool and return the handle to new storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 *                  @xml [string]: XML string to create the storage volume in the storage pool
 *                  @flags [int]: virStorageVolCreateXML flags
 * Returns:         libvirt storagevolume resource
 */
PHP_FUNCTION(libvirt_storagevolume_create_xml)
{
    php_libvirt_volume *res_volume = NULL;
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    virStorageVolPtr volume = NULL;
    char *xml;
    zend_long flags = 0;
    strsize_t xml_len;

    GET_STORAGEPOOL_FROM_ARGS("rs|l", &zpool, &xml, &xml_len, &flags);

    volume = virStorageVolCreateXML(pool->pool, xml, flags);
    DPRINTF("%s: virStorageVolCreateXML(%p, <xml>, 0) returned %p\n", PHPFUNC, pool->pool, volume);
    if (volume == NULL)
        RETURN_FALSE;

    res_volume = (php_libvirt_volume *)emalloc(sizeof(php_libvirt_volume));
    res_volume->volume = volume;
    res_volume->conn   = pool->conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_volume->volume);
    resource_change_counter(INT_RESOURCE_VOLUME, pool->conn->conn, res_volume->volume, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_volume, le_libvirt_volume);
}

/*
 * Function name:   libvirt_storagevolume_create_xml_from
 * Since version:   0.4.1(-2)
 * Description:     Function is used to clone the new storage volume into pool from the original volume
 * Arguments:       @pool [resource]: libvirt storagepool resource
 *                  @xml [string]: XML string to create the storage volume in the storage pool
 *                  @original_volume [resource]: libvirt storagevolume resource
 * Returns:         libvirt storagevolume resource
 */
PHP_FUNCTION(libvirt_storagevolume_create_xml_from)
{
    php_libvirt_volume *res_volume = NULL;
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;

    php_libvirt_volume *pl_volume = NULL;
    zval *zvolume;

    virStorageVolPtr volume = NULL;
    char *xml;
    strsize_t xml_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsr", &zpool, &xml, &xml_len, &zvolume) == FAILURE) {
        set_error("Invalid pool resource, XML or volume resource" TSRMLS_CC);
        RETURN_FALSE;
    }

    VIRT_FETCH_RESOURCE(pool, php_libvirt_storagepool*, &zpool, PHP_LIBVIRT_STORAGEPOOL_RES_NAME, le_libvirt_storagepool);
    if ((pool == NULL) || (pool->pool == NULL))
        RETURN_FALSE;
    VIRT_FETCH_RESOURCE(pl_volume, php_libvirt_volume*, &zvolume, PHP_LIBVIRT_VOLUME_RES_NAME, le_libvirt_volume);
    if ((pl_volume == NULL) || (pl_volume->volume == NULL))
        RETURN_FALSE;
    resource_change_counter(INT_RESOURCE_VOLUME, pl_volume->conn->conn, pl_volume->volume, 1 TSRMLS_CC);

    volume = virStorageVolCreateXMLFrom(pool->pool, xml, pl_volume->volume, 0);
    DPRINTF("%s: virStorageVolCreateXMLFrom(%p, <xml>, %p, 0) returned %p\n", PHPFUNC, pool->pool, pl_volume->volume, volume);
    if (volume == NULL)
        RETURN_FALSE;

    res_volume = (php_libvirt_volume *)emalloc(sizeof(php_libvirt_volume));
    res_volume->volume = volume;
    res_volume->conn   = pool->conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_volume->volume);
    resource_change_counter(INT_RESOURCE_VOLUME, pool->conn->conn, res_volume->volume, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_volume, le_libvirt_volume);
}

/*
 * Function name:   libvirt_storagevolume_delete
 * Since version:   0.4.2
 * Description:     Function is used to delete to volume identified by it's resource
 * Arguments:       @res [resource]: libvirt storagevolume resource
 *                  @flags [int]: optional flags for the storage volume deletion for virStorageVolDelete()
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagevolume_delete)
{
    php_libvirt_volume *volume = NULL;
    zval *zvolume;
    zend_long flags = 0;
    int retval = 0;

    GET_VOLUME_FROM_ARGS("r|l", &zvolume, &flags);

    retval = virStorageVolDelete(volume->volume, flags);
    DPRINTF("%s: virStorageVolDelete(%p, %d) returned %d\n", PHPFUNC, volume->volume, (int) flags, retval);
    if (retval != 0) {
        set_error_if_unset("Cannot delete storage volume" TSRMLS_CC);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_storagevolume_download
 * Since version:   0.5.0
 * Description:     Function is used to download volume identified by it's resource
 * Arguments:       @res [resource]: libvirt storagevolume resource
 *                  @stream [resource]: stream to use as output
 *                  @offset [int]: position to start reading from
 *                  @length [int] : limit on amount of data to download
 *                  @flags [int]: optional flags for the storage volume download for virStorageVolDownload()
 * Returns:         int
 */
PHP_FUNCTION(libvirt_storagevolume_download)
{
    php_libvirt_volume *volume = NULL;
    php_libvirt_stream *stream = NULL;
    zval *zvolume;
    zval *zstream;
    zend_long flags = 0;
    zend_long offset = 0;
    zend_long length = 0;
    int retval = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rr|l|l|l", &zvolume, &zstream, &offset, &length, &flags) == FAILURE)
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(volume, php_libvirt_volume*, &zvolume, PHP_LIBVIRT_VOLUME_RES_NAME, le_libvirt_volume);
    if ((volume == NULL) || (volume->volume == NULL))
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(stream, php_libvirt_stream*, &zstream, PHP_LIBVIRT_STREAM_RES_NAME, le_libvirt_stream);
    if ((stream == NULL) || (stream->stream == NULL))
        RETURN_LONG(retval);

    retval = virStorageVolDownload(volume->volume, stream->stream, offset, length, flags);
    DPRINTF("%s: virStorageVolDownload(%p, %p, %d, %d, %d) returned %d\n", PHPFUNC, volume->volume, stream->stream, (int) offset, (int) length, (int) flags, retval);

    if (retval == -1) {
        set_error("Cannot download from stream" TSRMLS_CC);
        RETURN_LONG(retval);
    }

    RETURN_LONG(retval);
}

/*
 * Function name:   libvirt_storagevolume_upload
 * Since version:   0.5.0
 * Description:     Function is used to upload volume identified by it's resource
 * Arguments:       @res [resource]: libvirt storagevolume resource
 *                  @stream [resource]: stream to use as input
 *                  @offset [int]: position to start writing to
 *                  @length [int] : limit on amount of data to upload
 *                  @flags [int]: optional flags for the storage volume upload for virStorageVolUpload()
 * Returns:         int
 */
PHP_FUNCTION(libvirt_storagevolume_upload)
{
    php_libvirt_volume *volume = NULL;
    php_libvirt_stream *stream = NULL;
    zval *zvolume;
    zval *zstream;
    zend_long flags = 0;
    zend_long offset = 0;
    zend_long length = 0;
    int retval = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rr|l|l|l", &zvolume, &zstream, &offset, &length, &flags) == FAILURE)
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(volume, php_libvirt_volume*, &zvolume, PHP_LIBVIRT_VOLUME_RES_NAME, le_libvirt_volume);
    if ((volume == NULL) || (volume->volume == NULL))
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(stream, php_libvirt_stream*, &zstream, PHP_LIBVIRT_STREAM_RES_NAME, le_libvirt_stream);
    if ((stream == NULL) || (stream->stream == NULL))
        RETURN_LONG(retval);

    retval = virStorageVolUpload(volume->volume, stream->stream, offset, length, flags);
    DPRINTF("%s: virStorageVolUpload(%p, %p, %d, %d, %d) returned %d\n", PHPFUNC, volume->volume, stream->stream, (int) offset, (int) length, (int) flags, retval);

    if (retval == -1) {
        set_error_if_unset("Cannot upload storage volume" TSRMLS_CC);
        RETURN_LONG(retval);
    }

    RETURN_LONG(retval);
}

/*
 * Function name:   libvirt_storagevolume_resize
 * Since version:   0.5.0
 * Description:     Function is used to resize volume identified by it's resource
 * Arguments:       @res [resource]: libvirt storagevolume resource
 *                  @capacity [int]: capacity for the storage volume
 *                  @flags [int]: optional flags for the storage volume resize for virStorageVolResize()
 * Returns:         int
 */
PHP_FUNCTION(libvirt_storagevolume_resize)
{
    php_libvirt_volume *volume = NULL;
    zval *zvolume;
    zend_long flags = 0;
    zend_long capacity = 0;
    int retval = -1;

    GET_VOLUME_FROM_ARGS("rl|l", &zvolume, &capacity, &flags);

    retval = virStorageVolResize(volume->volume, capacity, flags);
    DPRINTF("%s: virStorageVolResize(%p, %d, %d) returned %d\n", PHPFUNC, volume->volume, (int) capacity, (int) flags, retval);
    if (retval != 0) {
        set_error_if_unset("Cannot resize storage volume" TSRMLS_CC);
        RETURN_LONG(retval);
    }

    RETURN_LONG(retval);
}

/*
 * Function name:   libvirt_list_storagepools
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list storage pools on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt storagepool names array for the connection
 */
PHP_FUNCTION(libvirt_list_storagepools)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int i;
    virStoragePoolPtr *pools = NULL;
    int npools = 0;
    const unsigned int flags = 0;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((npools = virConnectListAllStoragePools(conn->conn, &pools, flags)) < 0)
        RETURN_FALSE;

    DPRINTF("%s: Found %d pools\n", PHPFUNC, npools);

    array_init(return_value);
    for (i = 0; i < npools; i++) {
        virStoragePoolPtr pool = pools[i];
        const char *name;

        if (!(name = virStoragePoolGetName(pools[i])))
            goto error;

        VIRT_ADD_NEXT_INDEX_STRING(return_value,  name);
    }

    for (i = 0; i < npools; i++)
        virStoragePoolFree(pools[i]);
    free(pools);

    return;

 error:
    for (i = 0; i < npools; i++)
        virStoragePoolFree(pools[i]);
    free(pools);
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_list_active_storagepools
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list active storage pools on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt storagepool names array for the connection
 */
PHP_FUNCTION(libvirt_list_active_storagepools)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int i;
    virStoragePoolPtr *pools = NULL;
    int npools = 0;
    const unsigned int flags = VIR_CONNECT_LIST_STORAGE_POOLS_ACTIVE;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((npools = virConnectListAllStoragePools(conn->conn, &pools, flags)) < 0)
        RETURN_FALSE;

    DPRINTF("%s: Found %d pools\n", PHPFUNC, npools);

    array_init(return_value);
    for (i = 0; i < npools; i++) {
        virStoragePoolPtr pool = pools[i];
        const char *name;

        if (!(name = virStoragePoolGetName(pools[i])))
            goto error;

        VIRT_ADD_NEXT_INDEX_STRING(return_value,  name);
    }

    for (i = 0; i < npools; i++)
        virStoragePoolFree(pools[i]);
    free(pools);

    return;

 error:
    for (i = 0; i < npools; i++)
        virStoragePoolFree(pools[i]);
    free(pools);
    RETURN_FALSE;
}

/*
 * Function name:   libvirt_list_inactive_storagepools
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list inactive storage pools on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt storagepool names array for the connection
 */
PHP_FUNCTION(libvirt_list_inactive_storagepools)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int i;
    virStoragePoolPtr *pools = NULL;
    int npools = 0;
    const unsigned int flags = VIR_CONNECT_LIST_STORAGE_POOLS_INACTIVE;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((npools = virConnectListAllStoragePools(conn->conn, &pools, flags)) < 0)
        RETURN_FALSE;

    DPRINTF("%s: Found %d pools\n", PHPFUNC, npools);

    array_init(return_value);
    for (i = 0; i < npools; i++) {
        virStoragePoolPtr pool = pools[i];
        const char *name;

        if (!(name = virStoragePoolGetName(pools[i])))
            goto error;

        VIRT_ADD_NEXT_INDEX_STRING(return_value,  name);
    }

    for (i = 0; i < npools; i++)
        virStoragePoolFree(pools[i]);
    free(pools);

    return;

 error:
    for (i = 0; i < npools; i++)
        virStoragePoolFree(pools[i]);
    free(pools);
    RETURN_FALSE;
}

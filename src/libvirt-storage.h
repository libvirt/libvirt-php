/*
 * libvirt-storage.h: The PHP bindings to libvirt storage API
 *
 * See COPYING for the license of this software
 */

#ifndef __LIBVIRT_STORAGE_H__
# define __LIBVIRT_STORAGE_H__

# include "libvirt-connection.h"

# define PHP_LIBVIRT_STORAGEPOOL_RES_NAME "Libvirt storagepool"
# define PHP_LIBVIRT_VOLUME_RES_NAME "Libvirt volume"
# define INT_RESOURCE_STORAGEPOOL 0x10
# define INT_RESOURCE_VOLUME 0x20

# define PHP_FE_LIBVIRT_STORAGE                                                                      \
    PHP_FE(libvirt_storagepool_lookup_by_name,        arginfo_libvirt_conn_name)                     \
    PHP_FE(libvirt_storagepool_lookup_by_volume,      arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagepool_list_volumes,          arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagepool_get_info,              arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagepool_get_uuid_string,       arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagepool_get_name,              arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagepool_lookup_by_uuid_string, arginfo_libvirt_conn_uuid)                     \
    PHP_FE(libvirt_storagepool_get_xml_desc,          arginfo_libvirt_conn_xpath)                    \
    PHP_FE(libvirt_storagepool_define_xml,            arginfo_libvirt_storagepool_define_xml)        \
    PHP_FE(libvirt_storagepool_undefine,              arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagepool_create,                arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagepool_destroy,               arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagepool_is_active,             arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagepool_get_volume_count,      arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagepool_refresh,               arginfo_libvirt_conn_optflags)                 \
    PHP_FE(libvirt_storagepool_set_autostart,         arginfo_libvirt_conn_flags)                    \
    PHP_FE(libvirt_storagepool_get_autostart,         arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagepool_build,                 arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagepool_delete,                arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagevolume_lookup_by_name,      arginfo_libvirt_conn_name)                     \
    PHP_FE(libvirt_storagevolume_lookup_by_path,      arginfo_libvirt_storagevolume_lookup_by_path)  \
    PHP_FE(libvirt_storagevolume_get_name,            arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagevolume_get_path,            arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagevolume_get_info,            arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_storagevolume_get_xml_desc,        arginfo_libvirt_storagevolume_get_xml_desc)    \
    PHP_FE(libvirt_storagevolume_create_xml,          arginfo_libvirt_conn_xml)                      \
    PHP_FE(libvirt_storagevolume_create_xml_from,     arginfo_libvirt_storagevolume_create_xml_from) \
    PHP_FE(libvirt_storagevolume_delete,              arginfo_libvirt_conn_optflags)                 \
    PHP_FE(libvirt_storagevolume_download,            arginfo_libvirt_storagevolume_download)        \
    PHP_FE(libvirt_storagevolume_upload,              arginfo_libvirt_storagevolume_download)        \
    PHP_FE(libvirt_storagevolume_resize,              arginfo_libvirt_storagevolume_resize)          \
    PHP_FE(libvirt_list_storagepools,                 arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_list_active_storagepools,          arginfo_libvirt_conn)                          \
    PHP_FE(libvirt_list_inactive_storagepools,        arginfo_libvirt_conn)

# define GET_STORAGEPOOL_FROM_ARGS(args, ...)                                  \
    do {                                                                       \
        reset_error();                                                         \
        if (zend_parse_parameters(ZEND_NUM_ARGS(),                             \
                                  args,                                        \
                                  __VA_ARGS__) == FAILURE) {                   \
            set_error("Invalid arguments");                                    \
            RETURN_FALSE;                                                      \
        }                                                                      \
                                                                               \
        VIRT_FETCH_RESOURCE(pool, php_libvirt_storagepool*, &zpool,            \
                            PHP_LIBVIRT_STORAGEPOOL_RES_NAME,                  \
                            le_libvirt_storagepool);                           \
        if ((pool == NULL) || (pool->pool == NULL))                            \
            RETURN_FALSE;                                                      \
    } while (0)                                                                \

# define GET_VOLUME_FROM_ARGS(args, ...)                                       \
    do {                                                                       \
        reset_error();                                                         \
        if (zend_parse_parameters(ZEND_NUM_ARGS(),                             \
                                  args,                                        \
                                  __VA_ARGS__) == FAILURE) {                   \
            set_error("Invalid arguments");                                    \
            RETURN_FALSE;                                                      \
        }                                                                      \
                                                                               \
        VIRT_FETCH_RESOURCE(volume, php_libvirt_volume*, &zvolume,             \
                            PHP_LIBVIRT_VOLUME_RES_NAME, le_libvirt_volume);   \
        if ((volume == NULL) || (volume->volume == NULL))                      \
            RETURN_FALSE;                                                      \
    } while (0)                                                                \

extern int le_libvirt_storagepool;
extern int le_libvirt_volume;

typedef struct _php_libvirt_storagepool {
    virStoragePoolPtr pool;
    php_libvirt_connection* conn;
} php_libvirt_storagepool;

typedef struct _php_libvirt_volume {
    virStorageVolPtr volume;
    php_libvirt_connection* conn;
} php_libvirt_volume;

void php_libvirt_storagepool_dtor(zend_resource *rsrc);
void php_libvirt_volume_dtor(zend_resource *rsrc);

PHP_FUNCTION(libvirt_storagepool_lookup_by_name);
PHP_FUNCTION(libvirt_storagepool_lookup_by_volume);
PHP_FUNCTION(libvirt_storagepool_list_volumes);
PHP_FUNCTION(libvirt_storagepool_get_info);
PHP_FUNCTION(libvirt_storagepool_get_uuid_string);
PHP_FUNCTION(libvirt_storagepool_get_name);
PHP_FUNCTION(libvirt_storagepool_lookup_by_uuid_string);
PHP_FUNCTION(libvirt_storagepool_get_xml_desc);
PHP_FUNCTION(libvirt_storagepool_define_xml);
PHP_FUNCTION(libvirt_storagepool_undefine);
PHP_FUNCTION(libvirt_storagepool_create);
PHP_FUNCTION(libvirt_storagepool_destroy);
PHP_FUNCTION(libvirt_storagepool_is_active);
PHP_FUNCTION(libvirt_storagepool_get_volume_count);
PHP_FUNCTION(libvirt_storagepool_refresh);
PHP_FUNCTION(libvirt_storagepool_set_autostart);
PHP_FUNCTION(libvirt_storagepool_get_autostart);
PHP_FUNCTION(libvirt_storagepool_build);
PHP_FUNCTION(libvirt_storagepool_delete);
PHP_FUNCTION(libvirt_storagevolume_lookup_by_name);
PHP_FUNCTION(libvirt_storagevolume_lookup_by_path);
PHP_FUNCTION(libvirt_storagevolume_get_name);
PHP_FUNCTION(libvirt_storagevolume_get_path);
PHP_FUNCTION(libvirt_storagevolume_get_info);
PHP_FUNCTION(libvirt_storagevolume_get_xml_desc);
PHP_FUNCTION(libvirt_storagevolume_create_xml);
PHP_FUNCTION(libvirt_storagevolume_create_xml_from);
PHP_FUNCTION(libvirt_storagevolume_delete);
PHP_FUNCTION(libvirt_storagevolume_download);
PHP_FUNCTION(libvirt_storagevolume_upload);
PHP_FUNCTION(libvirt_storagevolume_resize);
PHP_FUNCTION(libvirt_list_storagepools);
PHP_FUNCTION(libvirt_list_active_storagepools);
PHP_FUNCTION(libvirt_list_inactive_storagepools);

#endif

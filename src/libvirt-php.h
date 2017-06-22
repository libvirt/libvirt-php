/*
 * libvirt-php.h: libvirt PHP bindings header file
 *
 * See COPYING for the license of this software
 *
 * Written by:
 *   Radek Hladik <r.hladik@cybersales.cz>
 *   Michal Novotny <minovotn@redhat.com>
 *   David King
 *   Jan-Paul van Burgsteden
 *   Lyre <liyong@skybility.com> (or <4179e1@gmail.com>)
 *   Daniel P. Berrange <berrange@redhat.com>
 *   Tiziano Mueller <dev-zero@gentoo.org>
 *   Yukihiro Kawada <warp.kawada@gmail.com>
 */

#ifndef PHP_LIBVIRT_H
#define PHP_LIBVIRT_H 1


/* Network constants */
#define VIR_NETWORKS_ACTIVE     1
#define VIR_NETWORKS_INACTIVE       2

/* Version constants */
#define VIR_VERSION_BINDING     1
#define VIR_VERSION_LIBVIRT     2

#ifdef _MSC_VER
#define EXTWIN
#endif

#ifdef EXTWIN
#define COMPILE_DL_LIBVIRT
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef COMPILE_DL_LIBVIRT
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_URL
#undef PACKAGE_VERSION
#include "php.h"

#ifdef ZTS
#include "TSRM.h"
#endif

#include "php_ini.h"
#ifdef EXTWIN
#include "ext/standard/info.h"
#else
#include "standard/info.h"
#endif
#endif

#ifndef VERSION
#define VERSION "0.5.1"
#define VERSION_MAJOR 0
#define VERSION_MINOR 5
#define VERSION_MICRO 1
#endif

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <libvirt/libvirt-qemu.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <fcntl.h>
#include <sys/types.h>

#ifndef EXTWIN
#include <inttypes.h>
#include <dirent.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <stdint.h>
#include <libgen.h>

#else

#define PRIx32       "I32x"
#define PRIx64       "I64x"

#ifdef EXTWIN
#if (_MSC_VER < 1300)
typedef signed char       int8_t;
typedef signed short      int16_t;
typedef signed int        int32_t;
typedef unsigned char     uint8_t;
typedef unsigned short    uint16_t;
typedef unsigned int      uint32_t;
#else
typedef signed __int8     int8_t;
typedef signed __int16    int16_t;
typedef signed __int32    int32_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
#endif
typedef signed __int64       int64_t;
typedef unsigned __int64     uint64_t;
#endif

#endif

#ifdef __i386__
typedef uint32_t arch_uint;
#define UINTx PRIx32
#else
typedef uint64_t arch_uint;
#define UINTx PRIx64
#endif

#if PHP_MAJOR_VERSION >= 7
typedef size_t strsize_t;
typedef zend_resource virt_resource;
typedef virt_resource *virt_resource_handle;

#define VIRT_RETURN_RESOURCE(_resource) \
    RETVAL_RES(_resource)

#define VIRT_REGISTER_RESOURCE(_resource, _le_resource)          \
    VIRT_RETURN_RESOURCE(zend_register_resource(_resource, _le_resource))

#define VIRT_REGISTER_LIST_RESOURCE(_name) do { \
    zval zret; \
    ZVAL_RES(&zret, zend_register_resource(res_##_name, le_libvirt_##_name)); \
    add_next_index_zval(return_value, &zret); \
    } while(0)

#define VIRT_RESOURCE_HANDLE(_resource) \
    Z_RES_P(_resource)

#define VIRT_FETCH_RESOURCE(_state, _type, _zval, _name, _le) \
    if ((_state = (_type)zend_fetch_resource(Z_RES_P(*_zval), _name, _le)) == NULL) { \
        RETURN_FALSE; \
    }

#define VIRT_RETVAL_STRING(_str)    \
    RETVAL_STRING(_str)
#define VIRT_RETVAL_STRINGL(_str, _len) \
    RETVAL_STRINGL(_str, _len)
#define VIRT_RETURN_STRING(_str)    \
    RETURN_STRING(_str)
#define VIRT_RETURN_STRINGL(_str, _len) \
    RETURN_STRINGL(_str, _len)
#define VIRT_ZVAL_STRINGL(_zv, _str, _len)  \
    ZVAL_STRINGL(_zv, _str, _len)
#define VIRT_ADD_INDEX_STRING(_arg, _idx, _str)  \
    add_index_string(_arg, _idx, _str)
#define VIRT_ADD_NEXT_INDEX_STRING(_arg, _str)  \
    add_next_index_string(_arg, _str)
#define VIRT_ADD_ASSOC_STRING(_arg, _key, _str) \
    add_assoc_string(_arg, _key, _str)
#define VIRT_ADD_ASSOC_STRING_EX(_arg, _key, _key_len, _value) \
    add_assoc_string_ex(_arg, _key, _key_len, _value)

#define VIRT_FOREACH(_ht, _pos, _zv) \
    for (zend_hash_internal_pointer_reset_ex(_ht, &_pos); \
         (_zv = zend_hash_get_current_data_ex(_ht, &_pos)) != NULL; \
         zend_hash_move_forward_ex(_ht, &_pos)) \

#define VIRT_FOREACH_END(_dummy)

#define VIRT_HASH_CURRENT_KEY_INFO(_ht, _pos, _idx, _info) \
    do { \
    zend_string *tmp_key_info; \
    _info.type = zend_hash_get_current_key_ex(_ht, &tmp_key_info, &_idx, &_pos); \
    _info.name = ZSTR_VAL(tmp_key_info); \
    _info.length = ZSTR_LEN(tmp_key_info); \
    } while(0)

#define VIRT_ARRAY_INIT(_name) do { \
    zval z##_name; \
    _name = &z##_name; \
    array_init(_name); \
    } while(0)

#else /* PHP_MAJOR_VERSION < 7 */
typedef int strsize_t;
typedef long zend_long;
typedef unsigned long zend_ulong;
typedef zend_rsrc_list_entry virt_resource;
typedef long virt_resource_handle;

#define VIRT_RETURN_RESOURCE(_resource) \
    RETVAL_RESOURCE((long) _resource)

#define VIRT_REGISTER_RESOURCE(_resource, _le_resource) \
    ZEND_REGISTER_RESOURCE(return_value, _resource, _le_resource)

#define VIRT_REGISTER_LIST_RESOURCE(_name) do { \
    zval *zret; \
    ALLOC_INIT_ZVAL(zret); \
    ZEND_REGISTER_RESOURCE(zret, res_##_name, le_libvirt_##_name); \
    add_next_index_zval(return_value, zret); \
    } while(0)

#define VIRT_RESOURCE_HANDLE(_resource) \
    Z_LVAL_P(_resource)

#define VIRT_FETCH_RESOURCE(_state, _type, _zval, _name, _le) \
    ZEND_FETCH_RESOURCE(_state, _type, _zval, -1, _name, _le);

#define VIRT_RETVAL_STRING(_str)    \
    RETVAL_STRING(_str, 1)
#define VIRT_RETVAL_STRINGL(_str, _len) \
    RETVAL_STRINGL(_str, _len, 1)
#define VIRT_RETURN_STRING(_str)    \
    RETURN_STRING(_str, 1)
#define VIRT_RETURN_STRINGL(_str, _len) \
    RETURN_STRINGL(_str, _len, 1)
#define VIRT_ZVAL_STRINGL(_zv, _str, _len)  \
    ZVAL_STRINGL(_zv, _str, _len, 1)
#define VIRT_ADD_INDEX_STRING(_arg, _idx, _str)  \
    add_index_string(_arg, _idx, _str, 1)
#define VIRT_ADD_NEXT_INDEX_STRING(_arg, _str)  \
    add_next_index_string(_arg, _str, 1)
#define VIRT_ADD_ASSOC_STRING(_arg, _key, _str) \
    add_assoc_string(_arg, _key, _str, 1)
#define VIRT_ADD_ASSOC_STRING_EX(_arg, _key, _key_len, _value) \
    add_assoc_string_ex(_arg, _key, _key_len, _value, 1)

#define VIRT_FOREACH(_ht, _pos, _zv) \
    { \
    zval **pzv = &_zv; \
    for (zend_hash_internal_pointer_reset_ex(_ht, &_pos); \
         zend_hash_get_current_data_ex(_ht, (void **) &pzv, &_pos) == SUCCESS; \
         zend_hash_move_forward_ex(_ht, &_pos)) { \
        _zv = *pzv;

#define VIRT_FOREACH_END(_dummy) \
    }}

#define VIRT_HASH_CURRENT_KEY_INFO(_ht, _pos, _idx, _info) \
    do { \
    _info.type = zend_hash_get_current_key_ex(_ht, &_info.name, &_info.length, &_idx, 0, &_pos); \
    } while(0)

#define VIRT_ARRAY_INIT(_name) do {\
    ALLOC_INIT_ZVAL(_name); \
    array_init(_name); \
    } while(0)

#endif /* PHP_MAJOR_VERSION < 7 */

typedef struct tTokenizer {
    char **tokens;
    int numTokens;
} tTokenizer;

typedef struct _resource_info {
    int type;
    virConnectPtr conn;
    void *mem;
    int overwrite;
} resource_info;

#ifdef ZTS
#define LIBVIRT_G(v) TSRMG(libvirt_globals_id, zend_libvirt_globals *, v)
#else
#define LIBVIRT_G(v) (libvirt_globals.v)
#endif

#define PHP_LIBVIRT_WORLD_VERSION VERSION
#define PHP_LIBVIRT_WORLD_EXTNAME "libvirt"

/* Connect flags */
#define CONNECT_FLAG_SOUNDHW_GET_NAMES  0x01

/* Domain flags */
#define DOMAIN_FLAG_FEATURE_ACPI    0x01
#define DOMAIN_FLAG_FEATURE_APIC    0x02
#define DOMAIN_FLAG_FEATURE_PAE     0x04
#define DOMAIN_FLAG_CLOCK_LOCALTIME 0x08
#define DOMAIN_FLAG_TEST_LOCAL_VNC  0x10
#define DOMAIN_FLAG_SOUND_AC97      0x20

/* Domain disk flags */
#define DOMAIN_DISK_FILE            0x01
#define DOMAIN_DISK_BLOCK           0x02
#define DOMAIN_DISK_ACCESS_ALL      0x04

/* Internal resource identifier objects */
#define INT_RESOURCE_CONNECTION     0x01
#define INT_RESOURCE_DOMAIN         0x02
#define INT_RESOURCE_NETWORK        0x04
#define INT_RESOURCE_NODEDEV        0x08
#define INT_RESOURCE_STORAGEPOOL    0x10
#define INT_RESOURCE_VOLUME         0x20
#define INT_RESOURCE_SNAPSHOT       0x40
#define INT_RESOURCE_STREAM         0x50

typedef struct tVMDisk {
    char *path;
    char *driver;
    char *bus;
    char *dev;
    unsigned long long size;
    int flags;
} tVMDisk;

typedef struct tVMNetwork {
    char *mac;
    char *network;
    char *model;
} tVMNetwork;

/* Libvirt-php types */
typedef struct _php_libvirt_connection {
    virConnectPtr conn;
    virt_resource_handle resource;
} php_libvirt_connection;

typedef struct _php_libvirt_stream {
    virStreamPtr stream;
    php_libvirt_connection* conn;
} php_libvirt_stream;

typedef struct _php_libvirt_domain {
    virDomainPtr domain;
    php_libvirt_connection* conn;
} php_libvirt_domain;

typedef struct _php_libvirt_snapshot {
    virDomainSnapshotPtr snapshot;
    php_libvirt_domain* domain;
} php_libvirt_snapshot;

typedef struct _php_libvirt_network {
    virNetworkPtr network;
    php_libvirt_connection* conn;
} php_libvirt_network;

typedef struct _php_libvirt_nodedev {
    virNodeDevicePtr device;
    php_libvirt_connection* conn;
} php_libvirt_nodedev;

typedef struct _php_libvirt_storagepool {
    virStoragePoolPtr pool;
    php_libvirt_connection* conn;
} php_libvirt_storagepool;

typedef struct _php_libvirt_volume {
    virStorageVolPtr volume;
    php_libvirt_connection* conn;
} php_libvirt_volume;

typedef struct _php_libvirt_cred_value {
    int count;
    int type;
    char *result;
    unsigned int    resultlen;
} php_libvirt_cred_value;

typedef struct _php_libvirt_hash_key_info {
    char *name;
    unsigned int length;
    unsigned int type;
} php_libvirt_hash_key_info;

/* Private definitions */
int set_logfile(char *filename, long maxsize TSRMLS_DC);
char *get_datetime(void);
char *get_string_from_xpath(char *xml, char *xpath, zval **val, int *retVal);
char **get_array_from_xpath(char *xml, char *xpath, int *num);

#define PHP_LIBVIRT_CONNECTION_RES_NAME "Libvirt connection"
#define PHP_LIBVIRT_DOMAIN_RES_NAME "Libvirt domain"
#define PHP_LIBVIRT_STREAM_RES_NAME "Libvirt stream"
#define PHP_LIBVIRT_STORAGEPOOL_RES_NAME "Libvirt storagepool"
#define PHP_LIBVIRT_VOLUME_RES_NAME "Libvirt volume"
#define PHP_LIBVIRT_NETWORK_RES_NAME "Libvirt virtual network"
#define PHP_LIBVIRT_NODEDEV_RES_NAME "Libvirt node device"
#define PHP_LIBVIRT_SNAPSHOT_RES_NAME "Libvirt domain snapshot"

PHP_MINIT_FUNCTION(libvirt);
PHP_MSHUTDOWN_FUNCTION(libvirt);
PHP_RINIT_FUNCTION(libvirt);
PHP_RSHUTDOWN_FUNCTION(libvirt);
PHP_MINFO_FUNCTION(libvirt);

/* Common functions */
PHP_FUNCTION(libvirt_get_last_error);
/* Connect functions */
PHP_FUNCTION(libvirt_connect);
PHP_FUNCTION(libvirt_connect_get_uri);
PHP_FUNCTION(libvirt_connect_get_hostname);
PHP_FUNCTION(libvirt_connect_get_hypervisor);
PHP_FUNCTION(libvirt_connect_get_capabilities);
PHP_FUNCTION(libvirt_connect_get_emulator);
PHP_FUNCTION(libvirt_connect_get_nic_models);
PHP_FUNCTION(libvirt_connect_get_soundhw_models);
PHP_FUNCTION(libvirt_connect_get_maxvcpus);
PHP_FUNCTION(libvirt_connect_get_sysinfo);
PHP_FUNCTION(libvirt_connect_get_encrypted);
PHP_FUNCTION(libvirt_connect_get_secure);
PHP_FUNCTION(libvirt_connect_get_information);
PHP_FUNCTION(libvirt_connect_get_machine_types);
PHP_FUNCTION(libvirt_connect_get_all_domain_stats);
/* Node functions */
PHP_FUNCTION(libvirt_node_get_info);
PHP_FUNCTION(libvirt_node_get_cpu_stats);
PHP_FUNCTION(libvirt_node_get_cpu_stats_for_each_cpu);
PHP_FUNCTION(libvirt_node_get_mem_stats);
PHP_FUNCTION(libvirt_node_get_free_memory);
/* Stream functions */
PHP_FUNCTION(libvirt_stream_create);
PHP_FUNCTION(libvirt_stream_close);
PHP_FUNCTION(libvirt_stream_abort);
PHP_FUNCTION(libvirt_stream_finish);
PHP_FUNCTION(libvirt_stream_recv);
PHP_FUNCTION(libvirt_stream_send);
/* Domain functions */
PHP_FUNCTION(libvirt_domain_new);
PHP_FUNCTION(libvirt_domain_new_get_vnc);
PHP_FUNCTION(libvirt_domain_get_counts);
PHP_FUNCTION(libvirt_domain_is_persistent);
PHP_FUNCTION(libvirt_domain_lookup_by_name);
PHP_FUNCTION(libvirt_domain_get_xml_desc);
PHP_FUNCTION(libvirt_domain_get_disk_devices);
PHP_FUNCTION(libvirt_domain_get_interface_devices);
PHP_FUNCTION(libvirt_domain_get_screenshot);
PHP_FUNCTION(libvirt_domain_get_screenshot_api);
PHP_FUNCTION(libvirt_domain_get_screen_dimensions);
PHP_FUNCTION(libvirt_domain_change_vcpus);
PHP_FUNCTION(libvirt_domain_change_memory);
PHP_FUNCTION(libvirt_domain_change_boot_devices);
PHP_FUNCTION(libvirt_domain_disk_add);
PHP_FUNCTION(libvirt_domain_disk_remove);
PHP_FUNCTION(libvirt_domain_nic_add);
PHP_FUNCTION(libvirt_domain_nic_remove);
PHP_FUNCTION(libvirt_domain_attach_device);
PHP_FUNCTION(libvirt_domain_detach_device);
PHP_FUNCTION(libvirt_domain_get_info);
PHP_FUNCTION(libvirt_domain_get_uuid);
PHP_FUNCTION(libvirt_domain_get_uuid_string);
PHP_FUNCTION(libvirt_domain_get_name);
PHP_FUNCTION(libvirt_domain_get_id);
PHP_FUNCTION(libvirt_domain_lookup_by_id);
PHP_FUNCTION(libvirt_domain_lookup_by_uuid);
PHP_FUNCTION(libvirt_domain_lookup_by_uuid_string);
PHP_FUNCTION(libvirt_domain_destroy);
PHP_FUNCTION(libvirt_domain_create);
PHP_FUNCTION(libvirt_domain_resume);
PHP_FUNCTION(libvirt_domain_core_dump);
PHP_FUNCTION(libvirt_domain_shutdown);
PHP_FUNCTION(libvirt_domain_suspend);
PHP_FUNCTION(libvirt_domain_managedsave);
PHP_FUNCTION(libvirt_domain_undefine);
PHP_FUNCTION(libvirt_domain_reboot);
PHP_FUNCTION(libvirt_domain_define_xml);
PHP_FUNCTION(libvirt_domain_create_xml);
PHP_FUNCTION(libvirt_domain_xml_from_native);
PHP_FUNCTION(libvirt_domain_xml_to_native);
PHP_FUNCTION(libvirt_domain_set_max_memory);
PHP_FUNCTION(libvirt_domain_set_memory);
PHP_FUNCTION(libvirt_domain_set_memory_flags);
PHP_FUNCTION(libvirt_domain_memory_peek);
PHP_FUNCTION(libvirt_domain_memory_stats);
PHP_FUNCTION(libvirt_domain_update_device);
PHP_FUNCTION(libvirt_domain_block_commit);
PHP_FUNCTION(libvirt_domain_block_stats);
PHP_FUNCTION(libvirt_domain_block_resize);
PHP_FUNCTION(libvirt_domain_block_job_abort);
PHP_FUNCTION(libvirt_domain_block_job_set_speed);
PHP_FUNCTION(libvirt_domain_block_job_info);
PHP_FUNCTION(libvirt_domain_interface_stats);
PHP_FUNCTION(libvirt_domain_get_connect);
PHP_FUNCTION(libvirt_domain_migrate);
PHP_FUNCTION(libvirt_domain_get_job_info);
PHP_FUNCTION(libvirt_domain_xml_xpath);
PHP_FUNCTION(libvirt_domain_get_block_info);
PHP_FUNCTION(libvirt_domain_get_network_info);
PHP_FUNCTION(libvirt_domain_migrate_to_uri);
PHP_FUNCTION(libvirt_domain_migrate_to_uri2);
PHP_FUNCTION(libvirt_domain_get_autostart);
PHP_FUNCTION(libvirt_domain_set_autostart);
PHP_FUNCTION(libvirt_domain_is_active);
PHP_FUNCTION(libvirt_domain_get_next_dev_ids);
PHP_FUNCTION(libvirt_domain_send_keys);
PHP_FUNCTION(libvirt_domain_send_key_api);
PHP_FUNCTION(libvirt_domain_send_pointer_event);
PHP_FUNCTION(libvirt_domain_get_metadata);
PHP_FUNCTION(libvirt_domain_set_metadata);
PHP_FUNCTION(libvirt_domain_qemu_agent_command);
/* Domain snapshot functions */
PHP_FUNCTION(libvirt_domain_has_current_snapshot);
PHP_FUNCTION(libvirt_domain_snapshot_create);
PHP_FUNCTION(libvirt_domain_snapshot_lookup_by_name);
PHP_FUNCTION(libvirt_domain_snapshot_get_xml);
PHP_FUNCTION(libvirt_domain_snapshot_revert);
PHP_FUNCTION(libvirt_domain_snapshot_delete);
/* Storage pool and storage volume functions */
PHP_FUNCTION(libvirt_storagepool_lookup_by_name);
PHP_FUNCTION(libvirt_storagepool_lookup_by_volume);
PHP_FUNCTION(libvirt_storagepool_list_volumes);
PHP_FUNCTION(libvirt_storagepool_get_info);
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
/* Network functions */
PHP_FUNCTION(libvirt_network_define_xml);
PHP_FUNCTION(libvirt_network_undefine);
PHP_FUNCTION(libvirt_network_get);
PHP_FUNCTION(libvirt_network_get_xml_desc);
PHP_FUNCTION(libvirt_network_get_bridge);
PHP_FUNCTION(libvirt_network_get_information);
PHP_FUNCTION(libvirt_network_get_active);
PHP_FUNCTION(libvirt_network_set_active);
PHP_FUNCTION(libvirt_network_get_uuid_string);
PHP_FUNCTION(libvirt_network_get_uuid);
PHP_FUNCTION(libvirt_network_get_name);
PHP_FUNCTION(libvirt_network_get_autostart);
PHP_FUNCTION(libvirt_network_set_autostart);
/* Nodedev functions */
PHP_FUNCTION(libvirt_nodedev_get);
PHP_FUNCTION(libvirt_nodedev_capabilities);
PHP_FUNCTION(libvirt_nodedev_get_xml_desc);
PHP_FUNCTION(libvirt_nodedev_get_information);
/* Listing functions */
PHP_FUNCTION(libvirt_list_nodedevs);
PHP_FUNCTION(libvirt_list_all_networks);
PHP_FUNCTION(libvirt_list_networks);
PHP_FUNCTION(libvirt_list_domains);
PHP_FUNCTION(libvirt_list_domain_snapshots);
PHP_FUNCTION(libvirt_list_domain_resources);
PHP_FUNCTION(libvirt_list_active_domains);
PHP_FUNCTION(libvirt_list_active_domain_ids);
PHP_FUNCTION(libvirt_list_inactive_domains);
PHP_FUNCTION(libvirt_list_storagepools);
PHP_FUNCTION(libvirt_list_active_storagepools);
PHP_FUNCTION(libvirt_list_inactive_storagepools);
/* Common functions */
PHP_FUNCTION(libvirt_version);
PHP_FUNCTION(libvirt_check_version);
PHP_FUNCTION(libvirt_has_feature);
PHP_FUNCTION(libvirt_get_iso_images);
PHP_FUNCTION(libvirt_image_create);
PHP_FUNCTION(libvirt_image_remove);
/* Debugging functions */
PHP_FUNCTION(libvirt_logfile_set);
PHP_FUNCTION(libvirt_print_binding_resources);

extern zend_module_entry libvirt_module_entry;
#define phpext_libvirt_ptr &libvirt_module_entry

#endif

#ifndef PHP_LIBVIRT_H
#define PHP_LIBVIRT_H 1

#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_URL
#undef PACKAGE_VERSION

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include <libvirt/libvirt.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

ZEND_BEGIN_MODULE_GLOBALS(libvirt)
	char *last_error;
	zend_bool longlong_to_string_ini;
ZEND_END_MODULE_GLOBALS(libvirt)

#ifdef ZTS
#define LIBVIRT_G(v) TSRMG(libvirt_globals_id, zend_libvirt_globals *, v)
#else
#define LIBVIRT_G(v) (libvirt_globals.v)
#endif

#define PHP_LIBVIRT_WORLD_VERSION "0.4.1"
#define PHP_LIBVIRT_WORLD_EXTNAME "libvirt"

typedef struct _php_libvirt_connection {
	virConnectPtr conn;
	long resource_id;
} php_libvirt_connection;

typedef struct _php_libvirt_domain {
	virDomainPtr domain;
	php_libvirt_connection* conn;
} php_libvirt_domain;

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
} php_libvirt_storagepool;

typedef struct _php_libvirt_volume {
	virStorageVolPtr volume;
} php_libvirt_volume;


typedef struct _php_libvirt_cred_value {
	int count;
	int	type;
	char *result;
	unsigned int	resultlen;
} php_libvirt_cred_value;


#define PHP_LIBVIRT_CONNECTION_RES_NAME "Libvirt connection"
#define PHP_LIBVIRT_DOMAIN_RES_NAME "Libvirt domain"
#define PHP_LIBVIRT_STORAGEPOOL_RES_NAME "Libvirt storagepool"
#define PHP_LIBVIRT_VOLUME_RES_NAME "Libvirt volume"
#define PHP_LIBVIRT_NETWORK_RES_NAME "Libvirt virtual network"
#define PHP_LIBVIRT_NODEDEV_RES_NAME "Libvirt node device"

PHP_MINIT_FUNCTION(libvirt);
PHP_MSHUTDOWN_FUNCTION(libvirt);
PHP_RINIT_FUNCTION(libvirt);
PHP_RSHUTDOWN_FUNCTION(libvirt);
PHP_MINFO_FUNCTION(libvirt);

/* Common functions */
PHP_FUNCTION(libvirt_get_last_error);
PHP_FUNCTION(libvirt_connect);
PHP_FUNCTION(libvirt_get_uri);
PHP_FUNCTION(libvirt_get_hostname);
PHP_FUNCTION(libvirt_node_get_info);
/*  Domain functions */
PHP_FUNCTION(libvirt_domain_get_counts);
PHP_FUNCTION(libvirt_domain_lookup_by_name);
PHP_FUNCTION(libvirt_domain_get_xml_desc);
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
PHP_FUNCTION(libvirt_domain_shutdown);
PHP_FUNCTION(libvirt_domain_suspend);
PHP_FUNCTION(libvirt_domain_undefine);
PHP_FUNCTION(libvirt_domain_reboot);
PHP_FUNCTION(libvirt_domain_define_xml);
PHP_FUNCTION(libvirt_domain_create_xml);
PHP_FUNCTION(libvirt_domain_memory_peek);
PHP_FUNCTION(libvirt_domain_memory_stats);
PHP_FUNCTION(libvirt_domain_block_stats);
PHP_FUNCTION(libvirt_domain_interface_stats);
PHP_FUNCTION(libvirt_domain_get_connect);
PHP_FUNCTION(libvirt_domain_migrate);
PHP_FUNCTION(libvirt_domain_get_job_info);
PHP_FUNCTION(libvirt_domain_xml_xpath);
PHP_FUNCTION(libvirt_domain_get_block_info);
PHP_FUNCTION(libvirt_domain_get_network_info);
PHP_FUNCTION(libvirt_domain_migrate_to_uri);
PHP_FUNCTION(libvirt_domain_get_autostart);
PHP_FUNCTION(libvirt_domain_set_autostart);
PHP_FUNCTION(libvirt_domain_is_active);
/* Storagepool functions */
PHP_FUNCTION(libvirt_storagepool_lookup_by_name);
PHP_FUNCTION(libvirt_storagepool_list_volumes);
PHP_FUNCTION(libvirt_storagepool_get_info);
PHP_FUNCTION(libvirt_storagevolume_lookup_by_name);
PHP_FUNCTION(libvirt_storagevolume_get_info);
PHP_FUNCTION(libvirt_storagevolume_get_xml_desc);
PHP_FUNCTION(libvirt_storagevolume_create_xml);
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
/* Network functions */
PHP_FUNCTION(libvirt_network_get);
PHP_FUNCTION(libvirt_network_get_xml_desc);
PHP_FUNCTION(libvirt_network_get_bridge);
PHP_FUNCTION(libvirt_network_get_information);
PHP_FUNCTION(libvirt_network_get_active);
PHP_FUNCTION(libvirt_network_set_active);
/* Nodedev functions */
PHP_FUNCTION(libvirt_nodedev_get);
PHP_FUNCTION(libvirt_nodedev_capabilities);
PHP_FUNCTION(libvirt_nodedev_get_xml_desc);
PHP_FUNCTION(libvirt_nodedev_get_information);
/* Listing functions */
PHP_FUNCTION(libvirt_list_nodedevs);
PHP_FUNCTION(libvirt_list_networks);
PHP_FUNCTION(libvirt_list_domains);
PHP_FUNCTION(libvirt_list_domain_resources);
PHP_FUNCTION(libvirt_list_active_domains);
PHP_FUNCTION(libvirt_list_active_domain_ids);
PHP_FUNCTION(libvirt_list_inactive_domains);
PHP_FUNCTION(libvirt_list_storagepools);
PHP_FUNCTION(libvirt_version);
PHP_FUNCTION(libvirt_check_version);

extern zend_module_entry libvirt_module_entry;
#define phpext_libvirt_ptr &libvirt_module_entry

#endif

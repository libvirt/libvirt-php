/*
 * libvirt-network.h: The PHP bindings to libvirt network API
 *
 * See COPYING for the license of this software
 */

#ifndef __LIBVIRT_NETWORK_H__
# define __LIBVIRT_NETWORK_H__

# include "libvirt-connection.h"

# define PHP_LIBVIRT_NETWORK_RES_NAME "Libvirt virtual network"
# define INT_RESOURCE_NETWORK 0x04

# define GET_NETWORK_FROM_ARGS(args, ...)                                      \
    do {                                                                       \
        reset_error();                                                         \
        if (zend_parse_parameters(ZEND_NUM_ARGS(),                             \
                                  args,                                        \
                                  __VA_ARGS__) == FAILURE) {                   \
            set_error("Invalid arguments");                                    \
            RETURN_FALSE;                                                      \
        }                                                                      \
                                                                               \
        VIRT_FETCH_RESOURCE(network, php_libvirt_network*, &znetwork,          \
                            PHP_LIBVIRT_NETWORK_RES_NAME, le_libvirt_network); \
        if (network == NULL || network->network == NULL)                       \
            RETURN_FALSE;                                                      \
    } while (0)                                                                \

# define PHP_FE_LIBVIRT_NETWORK                                                \
    PHP_FE(libvirt_network_define_xml,      arginfo_libvirt_conn_xml)          \
    PHP_FE(libvirt_network_get_xml_desc,    arginfo_libvirt_conn_xpath)        \
    PHP_FE(libvirt_network_undefine,        arginfo_libvirt_conn)              \
    PHP_FE(libvirt_network_get,             arginfo_libvirt_conn_name)         \
    PHP_FE(libvirt_network_get_active,      arginfo_libvirt_conn)              \
    PHP_FE(libvirt_network_set_active,      arginfo_libvirt_conn_flags)        \
    PHP_FE(libvirt_network_get_bridge,      arginfo_libvirt_conn)              \
    PHP_FE(libvirt_network_get_information, arginfo_libvirt_conn)              \
    PHP_FE(libvirt_network_get_uuid_string, arginfo_libvirt_conn)              \
    PHP_FE(libvirt_network_get_uuid,        arginfo_libvirt_conn)              \
    PHP_FE(libvirt_network_get_name,        arginfo_libvirt_conn)              \
    PHP_FE(libvirt_network_get_autostart,   arginfo_libvirt_conn)              \
    PHP_FE(libvirt_network_set_autostart,   arginfo_libvirt_conn_flags)        \
    PHP_FE(libvirt_list_all_networks,       arginfo_libvirt_conn_optflags)     \
    PHP_FE(libvirt_list_networks,           arginfo_libvirt_conn_optflags)     \
    PHP_FE(libvirt_network_get_dhcp_leases, arginfo_libvirt_network_get_dhcp_leases)

extern int le_libvirt_network;

typedef struct _php_libvirt_network {
    virNetworkPtr network;
    php_libvirt_connection* conn;
} php_libvirt_network;

void php_libvirt_network_dtor(zend_resource *rsrc);

PHP_FUNCTION(libvirt_network_define_xml);
PHP_FUNCTION(libvirt_network_get_xml_desc);
PHP_FUNCTION(libvirt_network_undefine);
PHP_FUNCTION(libvirt_network_get);
PHP_FUNCTION(libvirt_network_get_active);
PHP_FUNCTION(libvirt_network_set_active);
PHP_FUNCTION(libvirt_network_get_bridge);
PHP_FUNCTION(libvirt_network_get_information);
PHP_FUNCTION(libvirt_network_get_uuid_string);
PHP_FUNCTION(libvirt_network_get_uuid);
PHP_FUNCTION(libvirt_network_get_name);
PHP_FUNCTION(libvirt_network_get_autostart);
PHP_FUNCTION(libvirt_network_set_autostart);
PHP_FUNCTION(libvirt_list_all_networks);
PHP_FUNCTION(libvirt_list_networks);
PHP_FUNCTION(libvirt_network_get_dhcp_leases);

#endif

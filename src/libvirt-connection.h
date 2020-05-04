/*
 * libvirt-connection.h: The PHP bindings to libvirt connection API
 *
 * See COPYING for the license of this software
 */

#ifndef __LIBVIRT_CONNECTION_H__
# define __LIBVIRT_CONNECTION_H__

# define PHP_LIBVIRT_CONNECTION_RES_NAME "Libvirt connection"
# define INT_RESOURCE_CONNECTION 0x01
# define CONNECT_FLAG_SOUNDHW_GET_NAMES  0x01

# define PHP_FE_LIBVIRT_CONNECTION                                                             \
    PHP_FE(libvirt_connect,                      arginfo_libvirt_connect)                      \
    PHP_FE(libvirt_connect_get_uri,              arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_connect_get_hostname,         arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_connect_get_hypervisor,       arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_connect_get_capabilities,     arginfo_libvirt_conn_xpath)                   \
    PHP_FE(libvirt_connect_get_emulator,         arginfo_libvirt_connect_get_emulator)         \
    PHP_FE(libvirt_connect_get_nic_models,       arginfo_libvirt_connect_get_emulator)         \
    PHP_FE(libvirt_connect_get_soundhw_models,   arginfo_libvirt_connect_get_soundhw_models)   \
    PHP_FE(libvirt_connect_get_maxvcpus,         arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_connect_get_sysinfo,          arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_connect_get_encrypted,        arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_connect_get_secure,           arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_connect_get_information,      arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_connect_get_machine_types,    arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_connect_get_all_domain_stats, arginfo_libvirt_connect_get_all_domain_stats)

# define GET_CONNECTION_FROM_ARGS(args, ...)                                   \
    do {                                                                       \
        reset_error(TSRMLS_C);                                                 \
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,                   \
                                  args,                                        \
                                  __VA_ARGS__) == FAILURE) {                   \
           set_error("Invalid arguments" TSRMLS_CC);                           \
           RETURN_FALSE;                                                       \
        }                                                                      \
                                                                               \
        VIRT_FETCH_RESOURCE(conn, php_libvirt_connection*, &zconn,             \
                            PHP_LIBVIRT_CONNECTION_RES_NAME,                   \
                            le_libvirt_connection);                            \
                                                                               \
        if (conn == NULL || conn->conn == NULL)                                \
            RETURN_FALSE;                                                      \
    } while (0)

extern int le_libvirt_connection;

typedef struct _php_libvirt_connection {
    virConnectPtr conn;
    virt_resource_handle resource;
} php_libvirt_connection;

typedef struct _php_libvirt_cred_value {
    int count;
    int type;
    char *result;
    unsigned int    resultlen;
} php_libvirt_cred_value;

void php_libvirt_connection_dtor(virt_resource *rsrc TSRMLS_DC);

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

#endif

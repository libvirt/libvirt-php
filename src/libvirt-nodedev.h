/*
 * libvirt-nodedev.h: The PHP bindings to libvirt nodedev API
 *
 * See COPYING for the license of this software
 */

#ifndef __LIBVIRT_NODEDEV_H__
# define __LIBVIRT_NODEDEV_H__

# include "libvirt-connection.h"

# define PHP_LIBVIRT_NODEDEV_RES_NAME "Libvirt node device"
# define INT_RESOURCE_NODEDEV 0x08

# define PHP_FE_LIBVIRT_NODEDEV                                                \
    PHP_FE(libvirt_nodedev_get,             arginfo_libvirt_conn)              \
    PHP_FE(libvirt_nodedev_capabilities,    arginfo_libvirt_conn)              \
    PHP_FE(libvirt_nodedev_get_xml_desc,    arginfo_libvirt_conn_xpath)        \
    PHP_FE(libvirt_nodedev_get_information, arginfo_libvirt_conn)              \
    PHP_FE(libvirt_list_nodedevs,           arginfo_libvirt_conn_optcap)

# define GET_NODEDEV_FROM_ARGS(args, ...)                                      \
    do {                                                                       \
        reset_error(TSRMLS_C);                                                 \
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,                   \
                                  args,                                        \
                                  __VA_ARGS__) == FAILURE) {                   \
            set_error("Invalid arguments" TSRMLS_CC);                          \
            RETURN_FALSE;                                                      \
        }                                                                      \
                                                                               \
        VIRT_FETCH_RESOURCE(nodedev, php_libvirt_nodedev*, &znodedev,          \
                            PHP_LIBVIRT_NODEDEV_RES_NAME, le_libvirt_nodedev); \
        if (nodedev == NULL || nodedev->device == NULL)                        \
            RETURN_FALSE;                                                      \
    } while (0)

extern int le_libvirt_nodedev;

typedef struct _php_libvirt_nodedev {
    virNodeDevicePtr device;
    php_libvirt_connection* conn;
} php_libvirt_nodedev;


void php_libvirt_nodedev_dtor(virt_resource *rsrc TSRMLS_DC);

PHP_FUNCTION(libvirt_nodedev_get);
PHP_FUNCTION(libvirt_nodedev_capabilities);
PHP_FUNCTION(libvirt_nodedev_get_xml_desc);
PHP_FUNCTION(libvirt_nodedev_get_information);
PHP_FUNCTION(libvirt_list_nodedevs);

#endif

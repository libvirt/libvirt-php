/*
 * libvirt-nwfilter.h: libvirt PHP binding for the NWFilter driver
 *
 * See COPYING for the license of this software
 */

#ifndef __LIBVIRT_NWFILTER_H__
# define __LIBVIRT_NWFILTER_H__

# include "libvirt-connection.h"

# define PHP_LIBVIRT_NWFILTER_RES_NAME "Libvirt nwfilter"
# define INT_RESOURCE_NWFILTER 0x60

# define PHP_FE_LIBVIRT_NWFILTER                                               \
    PHP_FE(libvirt_nwfilter_define_xml,            arginfo_libvirt_conn_xml)   \
    PHP_FE(libvirt_nwfilter_undefine,              arginfo_libvirt_conn)       \
    PHP_FE(libvirt_nwfilter_get_xml_desc,          arginfo_libvirt_conn_xpath) \
    PHP_FE(libvirt_nwfilter_get_uuid_string,       arginfo_libvirt_conn)       \
    PHP_FE(libvirt_nwfilter_get_uuid,              arginfo_libvirt_conn)       \
    PHP_FE(libvirt_nwfilter_get_name,              arginfo_libvirt_conn)       \
    PHP_FE(libvirt_nwfilter_lookup_by_name,        arginfo_libvirt_conn_name)  \
    PHP_FE(libvirt_nwfilter_lookup_by_uuid_string, arginfo_libvirt_conn_uuid)  \
    PHP_FE(libvirt_nwfilter_lookup_by_uuid,        arginfo_libvirt_conn_uuid)  \
    PHP_FE(libvirt_list_all_nwfilters,             arginfo_libvirt_conn)       \
    PHP_FE(libvirt_list_nwfilters,                 arginfo_libvirt_conn)

# define GET_NWFILTER_FROM_ARGS(args, ...)                                     \
    do {                                                                       \
        reset_error();                                                         \
        if (zend_parse_parameters(ZEND_NUM_ARGS(),                             \
                                  args,                                        \
                                  __VA_ARGS__) == FAILURE) {                   \
            set_error("Invalid arguments");                                    \
            RETURN_FALSE;                                                      \
        }                                                                      \
                                                                               \
        VIRT_FETCH_RESOURCE(nwfilter, php_libvirt_nwfilter *, &znwfilter,      \
                            PHP_LIBVIRT_NWFILTER_RES_NAME,                     \
                            le_libvirt_nwfilter);                              \
        if ((nwfilter == NULL) || (nwfilter->nwfilter == NULL))                \
            RETURN_FALSE;                                                      \
    } while (0)                                                                \

extern int le_libvirt_nwfilter;

typedef struct _php_libvirt_nwfilter {
    virNWFilterPtr nwfilter;
    php_libvirt_connection* conn;
} php_libvirt_nwfilter;

void php_libvirt_nwfilter_dtor(zend_resource *rsrc);

PHP_FUNCTION(libvirt_nwfilter_define_xml);
PHP_FUNCTION(libvirt_nwfilter_undefine);
PHP_FUNCTION(libvirt_nwfilter_get_xml_desc);
PHP_FUNCTION(libvirt_nwfilter_get_name);
PHP_FUNCTION(libvirt_nwfilter_get_uuid_string);
PHP_FUNCTION(libvirt_nwfilter_get_uuid);
PHP_FUNCTION(libvirt_nwfilter_lookup_by_name);
PHP_FUNCTION(libvirt_nwfilter_lookup_by_uuid_string);
PHP_FUNCTION(libvirt_nwfilter_lookup_by_uuid);
PHP_FUNCTION(libvirt_list_all_nwfilters);
PHP_FUNCTION(libvirt_list_nwfilters);

#endif

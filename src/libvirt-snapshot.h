/*
 * libvirt-snapshot.h: The PHP bindings to libvirt domain snapshot API
 *
 * See COPYING for the license of this software
 */

#ifndef __LIBVIRT_SNAPSHOT_H__
# define __LIBVIRT_SNAPSHOT_H__

# include "libvirt-domain.h"

# define PHP_LIBVIRT_SNAPSHOT_RES_NAME "Libvirt domain snapshot"
# define INT_RESOURCE_SNAPSHOT 0x40

# define PHP_FE_LIBVIRT_SNAPSHOT                                                                   \
    PHP_FE(libvirt_domain_has_current_snapshot,    arginfo_libvirt_conn_optflags)                  \
    PHP_FE(libvirt_domain_snapshot_lookup_by_name, arginfo_libvirt_domain_snapshot_lookup_by_name) \
    PHP_FE(libvirt_domain_snapshot_create,         arginfo_libvirt_conn_optflags)                  \
    PHP_FE(libvirt_domain_snapshot_create_xml,     arginfo_libvirt_domain_snapshot_create_xml)     \
    PHP_FE(libvirt_domain_snapshot_current,        arginfo_libvirt_conn_optflags)                  \
    PHP_FE(libvirt_domain_snapshot_get_xml,        arginfo_libvirt_conn_optflags)                  \
    PHP_FE(libvirt_domain_snapshot_revert,         arginfo_libvirt_conn_optflags)                  \
    PHP_FE(libvirt_domain_snapshot_delete,         arginfo_libvirt_conn_optflags)                  \
    PHP_FE(libvirt_list_domain_snapshots,          arginfo_libvirt_conn_optflags)

# define GET_SNAPSHOT_FROM_ARGS(args, ...)                                     \
    do {                                                                       \
        reset_error();                                                         \
        if (zend_parse_parameters(ZEND_NUM_ARGS(),                             \
                                  args,                                        \
                                  __VA_ARGS__) == FAILURE) {                   \
            set_error("Invalid arguments");                                    \
            RETURN_FALSE;                                                      \
        }                                                                      \
                                                                               \
        VIRT_FETCH_RESOURCE(snapshot, php_libvirt_snapshot*, &zsnapshot,       \
                            PHP_LIBVIRT_SNAPSHOT_RES_NAME,                     \
                            le_libvirt_snapshot);                              \
        if (snapshot == NULL || snapshot->snapshot == NULL)                    \
            RETURN_FALSE;                                                      \
    } while (0)                                                                \

extern int le_libvirt_snapshot;

typedef struct _php_libvirt_snapshot {
    virDomainSnapshotPtr snapshot;
    php_libvirt_domain* domain;
} php_libvirt_snapshot;

void php_libvirt_snapshot_dtor(zend_resource *rsrc);

PHP_FUNCTION(libvirt_domain_has_current_snapshot);
PHP_FUNCTION(libvirt_domain_snapshot_lookup_by_name);
PHP_FUNCTION(libvirt_domain_snapshot_create);
PHP_FUNCTION(libvirt_domain_snapshot_create_xml);
PHP_FUNCTION(libvirt_domain_snapshot_current);
PHP_FUNCTION(libvirt_domain_snapshot_get_xml);
PHP_FUNCTION(libvirt_domain_snapshot_revert);
PHP_FUNCTION(libvirt_domain_snapshot_delete);
PHP_FUNCTION(libvirt_list_domain_snapshots);

#endif

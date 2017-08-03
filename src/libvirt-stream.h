/*
 * libvirt-stream.h: The PHP bindings to libvirt stream API
 *
 * See COPYING for the license of this software
 */

#ifndef __LIBVIRT_STREAM_H__
# define __LIBVIRT_STREAM_H__

# include "libvirt-connection.h"

# define PHP_LIBVIRT_STREAM_RES_NAME "Libvirt stream"
# define INT_RESOURCE_STREAM 0x50

# define PHP_FE_LIBVIRT_STREAM                                                 \
    PHP_FE(libvirt_stream_create, arginfo_libvirt_conn)                        \
    PHP_FE(libvirt_stream_close,  arginfo_libvirt_conn)                        \
    PHP_FE(libvirt_stream_abort,  arginfo_libvirt_conn)                        \
    PHP_FE(libvirt_stream_finish, arginfo_libvirt_conn)                        \
    PHP_FE(libvirt_stream_send,   arginfo_libvirt_stream_send)                 \
    PHP_FE(libvirt_stream_recv,   arginfo_libvirt_stream_recv)

int le_libvirt_stream;

typedef struct _php_libvirt_stream {
    virStreamPtr stream;
    php_libvirt_connection* conn;
} php_libvirt_stream;

void php_libvirt_stream_dtor(virt_resource *rsrc TSRMLS_DC);

PHP_FUNCTION(libvirt_stream_create);
PHP_FUNCTION(libvirt_stream_close);
PHP_FUNCTION(libvirt_stream_abort);
PHP_FUNCTION(libvirt_stream_finish);
PHP_FUNCTION(libvirt_stream_recv);
PHP_FUNCTION(libvirt_stream_send);

#endif

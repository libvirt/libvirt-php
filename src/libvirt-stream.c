/*
 * libvirt-stream.c: The PHP bindings to libvirt stream API
 *
 * See COPYING for the license of this software
 */

#include <config.h>

#include <libvirt/libvirt.h>

#include "libvirt-php.h"
#include "libvirt-stream.h"

DEBUG_INIT("stream");

int le_libvirt_stream;

void
php_libvirt_stream_dtor(virt_resource *rsrc)
{
    php_libvirt_stream *stream = (php_libvirt_stream *)rsrc->ptr;
    int rv = 0;

    if (stream != NULL) {
        if (stream->stream != NULL) {
            if (!check_resource_allocation(NULL, INT_RESOURCE_STREAM, stream->stream)) {
                stream->stream = NULL;
                efree(stream);
                return;
            }
            rv = virStreamFree(stream->stream);
            if (rv != 0) {
                DPRINTF("%s: virStreamFree(%p) returned %d (%s)\n", __FUNCTION__, stream->stream, rv, LIBVIRT_G(last_error));
                php_error_docref(NULL, E_WARNING, "virStreamFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virStreamFree(%p) completed successfully\n", __FUNCTION__, stream->stream);
                resource_change_counter(INT_RESOURCE_STREAM, stream->conn->conn, stream->stream, 0);
            }
            stream->stream = NULL;
        }
        efree(stream);
    }
}

/*
 * Function name:   libvirt_stream_create
 * Since version:   0.5.0
 * Description:     Function is used to create new stream from libvirt conn
 * Arguments:       @res [resource]: libvirt connection resource from libvirt_connect()
 * Returns:         resource libvirt stream resource
 */
PHP_FUNCTION(libvirt_stream_create)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    virStreamPtr stream = NULL;
    php_libvirt_stream *res_stream;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &zconn) == FAILURE)
        RETURN_FALSE;
    VIRT_FETCH_RESOURCE(conn, php_libvirt_connection*, &zconn, PHP_LIBVIRT_CONNECTION_RES_NAME, le_libvirt_connection);
    if ((conn == NULL) || (conn->conn == NULL))
        RETURN_FALSE;

    stream = virStreamNew(conn->conn, 0);
    if (stream == NULL) {
        set_error("Cannot create new stream");
        RETURN_FALSE;
    }

    res_stream = (php_libvirt_stream *)emalloc(sizeof(php_libvirt_stream));
    res_stream->stream = stream;
    res_stream->conn = conn;

    resource_change_counter(INT_RESOURCE_STREAM, conn->conn, res_stream->stream, 1);

    VIRT_REGISTER_RESOURCE(res_stream, le_libvirt_stream);
}

/*
 * Function name:   libvirt_stream_close
 * Since version:   0.5.0
 * Description:     Function is used to close stream
 * Arguments:       @res [resource]: libvirt stream resource from libvirt_stream_create()
 * Returns:         int
 */
PHP_FUNCTION(libvirt_stream_close)
{
    zval *zstream;
    php_libvirt_stream *stream = NULL;
    int retval = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &zstream) == FAILURE)
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(stream, php_libvirt_stream*, &zstream, PHP_LIBVIRT_STREAM_RES_NAME, le_libvirt_stream);
    if ((stream == NULL) || (stream->stream == NULL))
        RETURN_LONG(retval);

    retval = virStreamFree(stream->stream);
    if (retval != 0) {
        set_error("Cannot free stream");
        RETURN_LONG(retval);
    }

    resource_change_counter(INT_RESOURCE_STREAM, stream->conn->conn, stream->stream, 0);
    RETURN_LONG(retval);
}

/*
 * Function name:   libvirt_stream_abort
 * Since version:   0.5.0
 * Description:     Function is used to abort transfer
 * Arguments:       @res [resource]: libvirt stream resource from libvirt_stream_create()
 * Returns:         int
 */
PHP_FUNCTION(libvirt_stream_abort)
{
    zval *zstream;
    php_libvirt_stream *stream = NULL;
    int retval = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &zstream) == FAILURE)
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(stream, php_libvirt_stream*, &zstream, PHP_LIBVIRT_STREAM_RES_NAME, le_libvirt_stream);
    if ((stream == NULL) || (stream->stream == NULL))
        RETURN_LONG(retval);

    retval = virStreamAbort(stream->stream);
    if (retval != 0) {
        set_error("Cannot abort stream");
        RETURN_LONG(retval);
    }
    RETURN_LONG(retval);
}

/*
 * Function name:   libvirt_stream_finish
 * Since version:   0.5.0
 * Description:     Function is used to finish transfer
 * Arguments:       @res [resource]: libvirt stream resource from libvirt_stream_create()
 * Returns:         int
 */
PHP_FUNCTION(libvirt_stream_finish)
{
    zval *zstream;
    php_libvirt_stream *stream = NULL;
    int retval = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &zstream) == FAILURE)
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(stream, php_libvirt_stream*, &zstream, PHP_LIBVIRT_STREAM_RES_NAME, le_libvirt_stream);
    if ((stream == NULL) || (stream->stream == NULL))
        RETURN_LONG(retval);

    retval = virStreamFinish(stream->stream);
    if (retval != 0) {
        set_error("Cannot finish stream");
        RETURN_LONG(retval);
    }
    RETURN_LONG(retval);
}

/*
 * Function name:   libvirt_stream_recv
 * Since version:   0.5.0
 * Description:     Function is used to recv from stream via libvirt conn
 * Arguments:       @res [resource]: libvirt stream resource from libvirt_stream_create()
 *                  @data [string]: buffer
 *                  @len [int]: amout of data to recieve
 * Returns:         int
 */
PHP_FUNCTION(libvirt_stream_recv)
{
    zval *zstream, *zbuf;
    php_libvirt_stream *stream = NULL;
    char *recv_buf = NULL;
    int retval = -1;
    zend_long length = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rz|l", &zstream, &zbuf, &length) == FAILURE)
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(stream, php_libvirt_stream*, &zstream, PHP_LIBVIRT_STREAM_RES_NAME, le_libvirt_stream);
    if ((stream == NULL) || (stream->stream == NULL))
        RETURN_LONG(retval);

    recv_buf = emalloc(length + 1);

    retval = virStreamRecv(stream->stream, recv_buf, length);
    if (retval < 0) {
        zval_dtor(zbuf);
        ZVAL_NULL(zbuf);
    } else {
        recv_buf[retval] = '\0';
        VIRT_ZVAL_STRINGL(zbuf, recv_buf, retval);
    }

    if (retval == -1)
        set_error("Cannot recv from stream");

    efree(recv_buf);
    RETURN_LONG(retval);
}

/*
 * Function name:   libvirt_stream_send
 * Since version:   0.5.0
 * Description:     Function is used to send to stream via libvirt conn
 * Arguments:       @res [resource]: libvirt stream resource from libvirt_stream_create()
 *                  @data [string]: buffer
 *                  @length [int]: amout of data to send
 * Returns:         int
 */
PHP_FUNCTION(libvirt_stream_send)
{
    zval *zstream, *zbuf;
    php_libvirt_stream *stream = NULL;
    int retval = -1;
    zend_long length = 0;
    char *cstr;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rz|l", &zstream, &zbuf, &length) == FAILURE)
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(stream, php_libvirt_stream*, &zstream, PHP_LIBVIRT_STREAM_RES_NAME, le_libvirt_stream);
    if ((stream == NULL) || (stream->stream == NULL))
        RETURN_LONG(retval);

    cstr = Z_STRVAL_P(zbuf);

    retval = virStreamSend(stream->stream, cstr, length);
    if (retval == -1)
        set_error("Cannot send to stream");

    RETURN_LONG(retval);
}

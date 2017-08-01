/*
 * vncfunc.c: VNC Client functions to be used for the graphical VNC console of libvirt-php
 *
 * See COPYING for the license of this software
 */

#include <config.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "vncfunc.h"
#include "util.h"
#include "sockets.h"

DEBUG_INIT("vncfunc");

/* Function macro */
#define VNCFUNC __FUNCTION__

#define UC(a) (unsigned char)a
#define CALC_UINT32(a, b, c, d) (uint32_t)((a >> 24) + (b >> 16) + (c >> 8) + d)

typedef struct tServerFBParams {
    int width;
    int height;
    int bpp;
    int depth;
    int bigEndian;
    int trueColor;
    int maxRed;
    int maxGreen;
    int maxBlue;
    int shiftRed;
    int shiftGreen;
    int shiftBlue;
    int desktopNameLen;
    unsigned char *desktopName;
} tServerFBParams;

typedef struct tBMPFile {
    uint32_t filesz;
    uint16_t creator1;
    uint16_t creator2;
    uint32_t bmp_offset;

    uint32_t header_sz;
    int32_t height;
    int32_t width;
    uint16_t nplanes;
    uint16_t bitspp;
    uint32_t compress_type;
    uint32_t bmp_bytesz;
    int32_t hres;
    int32_t vres;
    uint32_t ncolors;
    uint32_t nimpcolors;
} tBMPFile;

/*
 * Private function name:   vnc_write_client_version
 * Since version:           0.4.3
 * Description:             Writes the client version header for version 3.8
 * Arguments:               @sfd [int]: socket descriptor connected to the VNC server
 * Returns:                 0 on success, -errno on error
 */
static int
vnc_write_client_version(int sfd)
{
    unsigned char buf[12];

    memset(buf, 0, sizeof(buf));
    buf[0] = 0x52;
    buf[1] = 0x46;
    buf[2] = 0x42;
    buf[3] = 0x20;
    buf[4] = 0x30;
    buf[5] = 0x30;
    buf[6] = 0x33;
    buf[7] = 0x2e;
    buf[8] = 0x30;
    buf[9] = 0x30;
    buf[10] = 0x38;
    buf[11] = 0x0a;

    if (write(sfd, buf, 12) < 0) {
        int err = errno;
        close(sfd);
        DPRINTF("%s: Write of client version failed\n", VNCFUNC);
        return -err;
    }

    DPRINTF("%s: VNC Client version packet sent\n", VNCFUNC);
    return 0;
}

/* Maximum number of authentication attempts */
# define VNC_MAX_AUTH_ATTEMPTS  10

/*
 * Private function name:   vnc_authorize
 * Since version:           0.4.3
 * Description:             Authorize the VNC client with server
 * Arguments:               @sfd [int]: socket descriptor connected to the VNC server
 * Returns:                 0 on success, -errno on error (incl. -EIO on invalid authorization)
 */
static int
vnc_authorize(int sfd)
{
    unsigned char buf[4] = { 0 };
    unsigned char buf2[32] = { 0 };
    int i, ok, num = -1;

    /* Read number security types supported */
    if ((num = read(sfd, buf, 1)) < 0) {
        int err = errno;
        DPRINTF("%s: Cannot read number of security types, error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        return -err;
    }

    /* Read all the security types */
    if (read(sfd, buf2, num) < 0) {
        int err = errno;
        DPRINTF("%s: Read function failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        return -err;
    }

    /* Check whether there's a security type None supported */
    ok = 0;
    for (i = 0; i < num; i++) {
        if (buf2[i] == 0x01)
            ok = 1;
    }

    /* Bail if security type None is not supported */
    if (ok == 0) {
        close(sfd);
        DPRINTF("%s: Security type None is not supported\n", VNCFUNC);
        return -ENOTSUP;
    }

    /* Just security type None is supported */
    buf[0] = 0x01;
    if (write(sfd, buf, 1) < 0) {
        int err = errno;
        close(sfd);
        return -err;
    }

    DPRINTF("%s: Security None selected\n", VNCFUNC);

    /* Just bogus to wait for proper auth response */
    i = 0;
    buf[0] = 0x01;
    while (buf[0] + buf[1] + buf[2] + buf[3] != 0) {
        if (read(sfd, buf, 4) < 0) {
            int err = errno;
            DPRINTF("%s: Read function failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
            close(sfd);
            return -err;
        }
        if (i++ > VNC_MAX_AUTH_ATTEMPTS) {
            close(sfd);
            return -EIO;
        }
    }

    DPRINTF("%s: VNC Client authorized\n", VNCFUNC);
    return 0;
}

/*
 * Private function name:   vnc_parse_fb_params
 * Since version:           0.4.3
 * Description:             Function to parse the framebuffer parameters from the VNC server
 * Arguments:               @buf [char *]: buffer to be parsed
 *                          @len [int]: length of the buffer
 * Returns:                 parameters structure of tServerFBParams
 */
static tServerFBParams
vnc_parse_fb_params(unsigned char *buf, int len)
{
    int nlen, little_endian = 0;
    int w1, w2, h1, h2, h, w;
    tServerFBParams params;

    w1 = buf[0];
    w2 = buf[1];
    h1 = buf[2];
    h2 = buf[3];

    little_endian = (buf[6] == 0);

    DPRINTF("%s: Read dimension bytes: width = { 0x%02x, 0x%02x }, height = { 0x%02x, 0x%02x }, %s endian\n", VNCFUNC,
            w1, w2, h1, h2, little_endian ? "little" : "big");

    w = SWAP2_BY_ENDIAN(little_endian, w1, w2);
    h = SWAP2_BY_ENDIAN(little_endian, h1, h2);

    DPRINTF("%s: Filling the parameters structure with width = %d, height = %d\n", VNCFUNC, w, h);

    params.width = w;
    params.height = h;

    /* Pixel format */
    params.bpp = buf[4];
    params.depth = buf[5];
    params.bigEndian = buf[6];
    params.trueColor = buf[7];
    params.maxRed = SWAP2_BY_ENDIAN(little_endian, buf[8], buf[9]);
    params.maxGreen = SWAP2_BY_ENDIAN(little_endian, buf[10], buf[11]);
    params.maxBlue = SWAP2_BY_ENDIAN(little_endian, buf[12], buf[13]);
    params.shiftRed = buf[14];
    params.shiftGreen = buf[15];
    params.shiftBlue = buf[16];

    /* Positions buf[17] up to buf[19] are positions for padding only */

    nlen = (int)CALC_UINT32(buf[20], buf[21], buf[22], buf[23]);
    params.desktopNameLen = nlen;
    params.desktopName =  (unsigned char*) strdup((char *)buf + 24);

    DPRINTF("%s: Desktop name set to '%s'\n", VNCFUNC, params.desktopName);

    DPRINTF("%s: width = %d, height = %d, bpp = %d, depth = %d, bigEndian = %d, trueColor = %d\n",
            VNCFUNC, params.width, params.height, params.bpp, params.depth, params.bigEndian, params.trueColor);
    DPRINTF("%s: maxColors = { %d, %d, %d }, shifts = { %d, %d, %d }\n", VNCFUNC, params.maxRed,
            params.maxGreen, params.maxBlue, params.shiftRed, params.shiftGreen, params.shiftBlue);

    DPRINTF("%s: Desktop name is '%s' (%d bytes)\n", VNCFUNC, params.desktopName, params.desktopNameLen);

    return params;
}

/*
 * Private function name:   vnc_send_key
 * Since version:           0.4.3
 * Description:             Function to send key to VNC server
 * Arguments:               @sfd [int]: socket descriptor for existing VNC client socket
 *                          @key [char]: key to send to the VNC server
 *                          @modifier [bool]: flag whether this is a modifier key
 *                          @release [bool]: flag to release the key immediately
 * Returns:                 0 on success, -errno otherwise
 */
static int
vnc_send_key(int sfd, unsigned char key, int modifier, int release)
{
    unsigned char buf[8];

    memset(buf, 0, 8);
    buf[0] = 0x04;  // KeyEvent
    buf[1] = (release ? 0x00 : 0x01);
    buf[2] = 0x00;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = modifier ? 0xff : 0x00;
    buf[7] = key;

    DPRINTF("%s: %s key %d [0x%02x], modifier: %s\n", VNCFUNC, (release ? "Releasing" : "Pressing"),
            key, key, modifier ? "true" : "false");

    if (write(sfd, buf, 8) < 0) {
        int err = errno;
        DPRINTF("%s: Error occured while writing to socket descriptor #%d: %d (%s)\n",
                VNCFUNC, sfd, err, strerror(err));
        close(sfd);
        return -err;
    }

    DPRINTF("%s: Write of 8 bytes successful\n", VNCFUNC);

    return 0;
}

/*
 * Private function name:   vnc_send_client_pointer
 * Since version:           0.4.3
 * Description:             Function to set the VNC client pointer event
 * Arguments:               @sfd [int]: socket descriptor for existing VNC client socket
 *                          @clicked [int]: bitmask of clicked mouse buttons
 *                          @pos_x [int]: X position of mouse cursor
 *                          @pos_y [int]: Y position of mouse cursor
 * Returns:                 0 on success, -errno otherwise
 */
static int
vnc_send_client_pointer(int sfd, int clicked, int pos_x, int pos_y)
{
    unsigned char buf[6] = { 0 };

    if (sfd < 0) {
        DPRINTF("%s: Socket is not opened!\n", VNCFUNC);
        return -EINVAL;
    }

    memset(buf, 0, 6);
    buf[0] = 0x05;
    buf[1] = clicked;

    PUT2_BYTE_ENDIAN(1, pos_x, buf[2], buf[3]);
    PUT2_BYTE_ENDIAN(1, pos_y, buf[4], buf[5]);

    if (write(sfd, buf, 6) < 0) {
        int err = errno;
        DPRINTF("%s: Write function failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        return -err;
    }


    DPRINTF("%s: Wrote 6 bytes of client pointer event, clicked = %d, x = { 0x%02x, 0x%02x }, y = { 0x%02x, 0x%02x }\n",
            VNCFUNC, buf[1], buf[2], buf[3], buf[4], buf[5]);
    return 0;
}

/*
 * Private function name:   vnc_set_pixel_format
 * Since version:           0.4.3
 * Description:             Function to set the VNC client pixel format
 * Arguments:               @sfd [int]: socket descriptor for existing VNC client socket
 *                          @params [struct]: structure of parameters to set the data
 * Returns:                 0 on success, -errno otherwise
 */
static int
vnc_set_pixel_format(int sfd, tServerFBParams params)
{
    unsigned char buf[20];

    if (sfd < 0) {
        DPRINTF("%s: Socket is not opened!\n", VNCFUNC);
        return -EINVAL;
    }

    DPRINTF("%s: Setting up pixel format\n", VNCFUNC);

    memset(buf, 0, 20);
    /* Message type 0 is SetPixelFormat message */
    buf[0] = 0x00;
    /* Next 3 bytes are just padding bytes */
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    /* This is for the future use only, those values are default if SetPixelFormat not sent at all */
    buf[4] = params.bpp;
    buf[5] = params.depth;
    buf[6] = 0;         // big endian bit, we try to disable big endian
    buf[7] = params.trueColor;  // inherit true color bit from the ServerInit message
    buf[8] = 0;
    buf[9] = 0xff;
    buf[10] = 0;
    buf[11] = 0xff;
    buf[12] = 0;
    buf[13] = 0xff;
    buf[14] = params.shiftRed;
    buf[15] = params.shiftGreen;
    buf[16] = params.shiftBlue;
    /* Next 3 bytes are padding bytes */
    buf[17] = 0;
    buf[18] = 0;
    buf[19] = 0;

    if (write(sfd, buf, 20) < 0) {
        int err = errno;
        DPRINTF("%s: Write function failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        return -err;
    }

    DPRINTF("%s: Pixel format set\n", VNCFUNC);

    return 0;
}

/*
 * Private function name:   vnc_set_encoding
 * Since version:           0.4.3
 * Description:             Function to set the VNC client encoding to Tight encoding
 * Arguments:               @sfd [int]: socket descriptor for existing VNC client socket
 * Returns:                 0 on success, -errno otherwise
 */
static int
vnc_set_encoding(int sfd)
{
    unsigned char buf[8];

    if (sfd < 0) {
        DPRINTF("%s: Socket is not opened!\n", VNCFUNC);
        return -EINVAL;
    }

    DPRINTF("%s: Setting up encoding\n", VNCFUNC);

    memset(buf, 0, 8);
    buf[0] = 0x02;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 1;
    /* Raw encoding */
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x00;

    if (write(sfd, buf, 8) < 0) {
        int err = errno;
        DPRINTF("%s: Write function failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        return -err;
    }

    DPRINTF("%s: Client encoding set\n", VNCFUNC);
    return 0;
}

/*
 * Private function name:   vnc_send_framebuffer_update
 * Since version:           0.4.3
 * Description:             Function to request update from the server
 * Arguments:               @sfd [int]: socket descriptor for existing VNC client socket
 *                          @incrementalUpdate [bool]: flag to determine whether we need incremental update or not
 *                          @x [int]: x position for update start
 *                          @y [int]: y position for update start
 *                          @w [int]: width of frame
 *                          @h [int]: height of frame
 * Returns:                 0 on success, -errno otherwise
 */
static int
vnc_send_framebuffer_update(int sfd, int incrementalUpdate, int x, int y, int w, int h)
{
    unsigned char buf[10];

    if (sfd < 0) {
        DPRINTF("%s: Socket is not opened!\n", VNCFUNC);
        return -EINVAL;
    }

    DPRINTF("%s: Sending %s update request\n", VNCFUNC, (incrementalUpdate ? "standard" : "incremental"));
    memset(buf, 0, 10);
    buf[0] = 0x03;
    buf[1] = incrementalUpdate;

    PUT2_BYTE_ENDIAN(1, x, buf[2], buf[3]);
    PUT2_BYTE_ENDIAN(1, y, buf[4], buf[5]);
    PUT2_BYTE_ENDIAN(1, w, buf[6], buf[7]);
    PUT2_BYTE_ENDIAN(1, h, buf[8], buf[9]);

    if (write(sfd, buf, 10) < 0) {
        int err = errno;
        DPRINTF("%s: Write function failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        return -err;
    }

    DPRINTF("%s: Request sent\n", VNCFUNC);

    return 0;
}

/*
 * Private function name:   vnc_send_framebuffer_update_request
 * Since version:           0.4.3
 * Description:             Function to request update from the server
 * Arguments:               @sfd [int]: socket descriptor for existing VNC client socket
 *                          @incrementalUpdate [bool]: flag to determine whether we need incremental update or not
 *                          @params [struct]: structure of parameters to request the update for full screen
 * Returns:                 0 on success, -errno otherwise
 */
static int
vnc_send_framebuffer_update_request(int sfd, int incrementalUpdate, tServerFBParams params)
{
    return vnc_send_framebuffer_update(sfd, incrementalUpdate, 0, 0, params.width, params.height);
}

/*
 * Private function name:   vnc_connect
 * Since version:           0.4.3
 * Description:             Function to connect to VNC server
 * Arguments:               @server [string]: server string to specify VNC server
 *                          @port [string]: string version of port value to connect to
 *                          @share [bool]: flag whether to share desktop or not
 * Returns:                 socket descriptor on success, -errno otherwise
 */
static int
vnc_connect(char *server, char *port, int share)
{
    int sfd, err;
    unsigned char buf[1024] = { 0 };

    sfd = connect_socket(server, port, 1, 1, 1);
    if (sfd < 0)
        return sfd;

    DPRINTF("%s: Opened socket with descriptor #%d\n", VNCFUNC, sfd);

    if (read(sfd, buf, 1024) < 0) {
        int err = errno;
        DPRINTF("%s: Read function failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        return -err;
    }

    if ((err = vnc_write_client_version(sfd)) < 0)
        return err;

    if ((err = vnc_authorize(sfd)) < 0)
        return err;

    /* ClientInit phase - Set 'share desktop' bit */
    buf[0] = share;
    if (write(sfd, buf, 1) < 0) {
        int err = errno;
        close(sfd);
        return -err;
    }

    DPRINTF("%s: Share desktop flag sent (%d)\n", VNCFUNC, buf[0]);

    return sfd;
}

/*
 * Private function name:   vnc_read_server_init
 * Since version:           0.4.5
 * Description:             Function to read the server initialization array
 * Arguments:               @sfd [int]: socket file descriptor acquired by vnc_connect() call
 * Returns:                 parameters block
 */
static tServerFBParams
vnc_read_server_init(int sfd)
{
    unsigned char *buf = NULL;
    unsigned char tmpbuf[25] = { 0 };
    tServerFBParams params = { 0 };
    int len = 0, namelen = 0;

    DPRINTF("%s: Server init - reading framebuffer parameters\n", VNCFUNC);
    if (read(sfd, tmpbuf, 24) < 0) {
        int err = errno;
        DPRINTF("%s: Read function failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        goto cleanup;
    }

    namelen = (int)CALC_UINT32(tmpbuf[20], tmpbuf[21], tmpbuf[22], tmpbuf[23]);
    DPRINTF("%s: Name length is %d\n", VNCFUNC, namelen);

    buf = (unsigned char *)malloc(namelen + 25);
    memset(buf, 0, namelen + 25);
    memcpy(buf, tmpbuf, sizeof(tmpbuf));

    if ((len = read(sfd, buf + 24, namelen)) < 0) {
        int err = errno;
        DPRINTF("%s: Read function failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        goto cleanup;
    }

    params = vnc_parse_fb_params(buf, len + 24);
 cleanup:
    free(buf);
    buf = NULL;
    return params;
}

/*
 * Private function name:   vnc_get_dimensions
 * Since version:           0.4.3
 * Description:             Function to get the dimensions of VNC window
 * Arguments:               @server [string]: server string to specify VNC server
 *                          @port [string]: string version of port value to connect to
 *                          @width [out int]: pointer to integer to carry width argument
 *                          @height [out int]: pointer to integer to carry height argument
 * Returns:                 0 on success, -errno otherwise
 */
int vnc_get_dimensions(char *server, char *port, int *width, int *height)
{
    int sfd;
    tServerFBParams params;

    if (!width && !height) {
        DPRINTF("%s: Neither width or height output value not defined\n", VNCFUNC);
        return -EINVAL;
    }

    DPRINTF("%s: server is %s, port is %s\n", VNCFUNC, server, port);

    sfd = vnc_connect(server, port, 1);
    if (sfd < 0) {
        int err = errno;
        DPRINTF("%s: VNC Connection failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        return -err;
    }

    params = vnc_read_server_init(sfd);

    if (width) {
        *width = params.width;

        DPRINTF("%s: Output parameter of width set to %d\n", VNCFUNC, *width);
    }

    if (height) {
        *height = params.height;

        DPRINTF("%s: Output parameter of height set to %d\n", VNCFUNC, *height);
    }

    while (socket_has_data(sfd, 500000, 0) == 1)
        socket_read(sfd, -1);

    shutdown(sfd, SHUT_RDWR);
    close(sfd);
    DPRINTF("%s: Closed descriptor #%d\n", VNCFUNC, sfd);
    return 0;
}

/*
 * Private function name:   vnc_raw_to_bitmap
 * Since version:           0.4.5
 * Description:             Function to get the bitmap from raw encoding
 * Arguments:               @infile [string]: input file
 *                          @outfile [string]: output file
 *                          @width [int]: width of the output image (for bitmap headers)
 *                          @height [int]: height of the output image (for bitmap headers)
 * Returns:                 0 on success, -errno otherwise
 */
static int
vnc_raw_to_bmp(char *infile, char *outfile, int width, int height)
{
    int i, ix, fd, fd2;
    tBMPFile fBMP = { 0 };
    long size = -1;
    long len, hsize = 0;
    uint32_t *pixels = NULL;
    unsigned char buf[8192] = { 0 };
    unsigned char tbuf[4] = { 0 };
    long total = 0;
    int start, end;

    fd = open(infile, O_RDONLY);
    if (fd == -1)
        return -EACCES;

    size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    hsize = sizeof(tBMPFile);
    fBMP.filesz = size + hsize + 2;
    fBMP.bmp_offset = hsize + 2;
    fBMP.header_sz = 40;
    fBMP.height = width;
    fBMP.width = height;
    fBMP.nplanes = 1;
    fBMP.bitspp = 32;
    fBMP.compress_type = 0;
    fBMP.bmp_bytesz = 32;
    fBMP.hres = 2835;
    fBMP.vres = 2835;
    fBMP.ncolors = 0;
    fBMP.nimpcolors = 0;

    fd2 = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd2 == -1)
        return -EPERM;

    if (write(fd2, "BM", 2) < 0 ||
        write(fd2, &fBMP, hsize) < 0)
        perror("Error on write");

    ix = 0;
    pixels = malloc(width * height * sizeof(uint32_t));
    if (pixels == NULL)
        return -ENOMEM;

    total = 0;
    while ((len = read(fd, buf, sizeof(buf))) > 0) {
        for (i = 0; i < len; i += 4) {
            tbuf[0] = buf[i];
            tbuf[1] = buf[i + 1];
            tbuf[2] = buf[i + 2];
            tbuf[3] = buf[i + 3];
            pixels[ix++] = GETUINT32(tbuf);

            total++;
        }

        memset(buf, 0, sizeof(buf));
    }

    /* Flip the image to get the real image */
    for (i = height - 1; i >= 0; i--) {
        start = (i * width) + 1;
        end = ((i + 1) * width) + 1;

        for (ix = start; ix < end; ix++) {
            UINT32STR(tbuf, pixels[ix]);
            if (write(fd2, tbuf, 4) < 0)
                perror("Error on write");
        }
    }

    free(pixels);
    close(fd2);
    close(fd);
    return 0;
}

/*
 * Private function name:   vnc_get_bitmap
 * Since version:           0.4.5
 * Description:             Function to get the bitmap from the VNC window
 * Arguments:               @server [string]: server string to specify VNC server
 *                          @port [string]: string version of port value to connect to
 *                          @fn [string]: string version of filename
 * Returns:                 0 on success, -errno otherwise
 */
int vnc_get_bitmap(char *server, char *port, char *fn)
{
    int sfd;
    long pattern_size;
    tServerFBParams params;
    char file[] = "/tmp/libvirt-php-tmp-XXXXXX";

    if (mkstemp(file) == 0)
        return -ENOENT;

    if (fn == NULL)
        return -ENOENT;

    sfd = vnc_connect(server, port, 1);
    if (sfd < 0) {
        int err = errno;
        DPRINTF("%s: VNC Connection failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        return -err;
    }

    params = vnc_read_server_init(sfd);

    pattern_size = params.width * params.height * (params.bpp / 8);
    DPRINTF("%s: %ld\n", VNCFUNC, pattern_size);

    vnc_set_pixel_format(sfd, params);
    vnc_set_encoding(sfd);

    DPRINTF("%s: Requesting framebuffer update\n", VNCFUNC);
    vnc_send_framebuffer_update_request(sfd, 1, params);

    while (socket_has_data(sfd, 50000, 0) == 0) {
        continue;
    }
    socket_read_and_save(sfd, file, pattern_size);

    shutdown(sfd, SHUT_RDWR);
    close(sfd);

    vnc_raw_to_bmp(file, fn, params.width, params.height);
    unlink(file);
    DPRINTF("%s: Closed descriptor #%d\n", VNCFUNC, sfd);
    return 0;
}

/*
 * Private function name:   vnc_send_keys
 * Since version:           0.4.2
 * Description:             Function to send the key to VNC window
 * Arguments:               @server [string]: server string to specify VNC server
 *                          @port [string]: string version of port value to connect to
 *                          @keys [string]: string to be send to the guest's VNC window
 * Returns:                 0 on success, -errno otherwise
 */
int vnc_send_keys(char *server, char *port, char *keys)
{
    int sfd;
    int i, skip_next;
    tServerFBParams params;

    DPRINTF("%s: server is %s, port is %s, keys are '%s'\n", VNCFUNC, server, port, keys);

    sfd = vnc_connect(server, port, 1);
    if (sfd < 0) {
        int err = errno;
        DPRINTF("%s: VNC Connection failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        return -err;
    }

    params = vnc_read_server_init(sfd);

    skip_next = 0;
    DPRINTF("%s: About to process key sequence '%s' (%d keys)\n", VNCFUNC, keys, (int)strlen(keys));
    for (i = 0; i < strlen(keys); i++) {
        DPRINTF("%s: Processing key %d: %d [0x%02x]\n", VNCFUNC, i, keys[i], keys[i]);
        if (skip_next) {
            skip_next = 0;
            continue;
        }
        /* Handling for escape characters */
        if ((keys[i] == '\\') && (strlen(keys) > i + 1)) {
            if (keys[i + 1] == 'n')
                keys[i] = 13;
            if (keys[i + 1] == 'r')
                keys[i] = 10;

            skip_next = 1;
        }

        DPRINTF("%s: Sending key press emulation for key %d\n", VNCFUNC, keys[i]);
        vnc_send_key(sfd, keys[i], skip_next, 0);
        DPRINTF("%s: Requesting framebuffer update\n", VNCFUNC);
        vnc_send_framebuffer_update_request(sfd, 1, params);
        DPRINTF("%s: Sending key release emulation for key %d\n", VNCFUNC, keys[i]);
        vnc_send_key(sfd, keys[i], skip_next, 1);

        /* Sleep for 50 ms, required to make VNC server accept the keystroke emulation */
        usleep(50000);
    }

    DPRINTF("%s: All %d keys sent\n", VNCFUNC, (int)strlen(keys));

    while (socket_has_data(sfd, 500000, 0) == 1)
        socket_read(sfd, -1);

    shutdown(sfd, SHUT_RDWR);
    close(sfd);
    DPRINTF("%s: Closed descriptor #%d\n", VNCFUNC, sfd);
    return 0;
}

/*
 * Private function name:   vnc_send_pointer_event
 * Since version:           0.4.2
 * Description:             Function to send the mouse pointer event/click to VNC window
 * Arguments:               @server [string]: server string to specify VNC server
 *                          @port [string]: string version of port value to connect to
 *                          @pos_x [int]: position on x-axis of the VNC window
 *                          @pos_y [int]: position on y-axis of the VNC window
 *                          @clicked [int]: mask of what buttons were clicked
 *                          @release [boolean]: release the buttons once clicked
 * Returns:                 0 on success, -errno otherwise
 */
int vnc_send_pointer_event(char *server, char *port, int pos_x, int pos_y, int clicked, int release)
{
    int sfd;
    tServerFBParams params;

    DPRINTF("%s: Server is %s, port is %s, x is %d, y is %d, clicked is %d, release is %d\n", VNCFUNC,
            server, port, pos_x, pos_y, clicked, release);

    sfd = vnc_connect(server, port, 0);
    if (sfd < 0) {
        int err = errno;
        DPRINTF("%s: VNC Connection failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        return -err;
    }

    params = vnc_read_server_init(sfd);

    if (((pos_x > params.width) || (pos_y < 0))
        || ((pos_y > params.height) || (pos_y < 0))) {
        DPRINTF("%s: Required positions out of range (width = %d, height = %d, x = %d, y = %d) for '%s'\n",
                VNCFUNC, params.width, params.height, pos_x, pos_y, params.desktopName);
        return -EINVAL;
    }

    socket_read(sfd, -1);
    vnc_set_pixel_format(sfd, params);
    vnc_set_encoding(sfd);
    socket_read(sfd, -1);
    usleep(50000);

    /* Following paragraph moves mouse pointer to [0, 0] (left upper corner) */
    vnc_send_client_pointer(sfd, 0, 0x7FFF, 0x7FFF);
    socket_read(sfd, -1);
    vnc_send_client_pointer(sfd, 0, 0, 0);
    socket_read(sfd, -1);

    vnc_send_client_pointer(sfd, clicked, (pos_x / 2), (params.height - pos_y) / 2);
    socket_read(sfd, -1);
    vnc_send_client_pointer(sfd, 0, (pos_x / 2), ((params.height - pos_y) / 2));
    socket_read(sfd, -1);

    if (release) {
        vnc_send_client_pointer(sfd, clicked, (pos_x / 2), (params.height - pos_y) / 2);
        socket_read(sfd, -1);
        vnc_send_client_pointer(sfd, 0, (pos_x / 2), (params.height - pos_y) / 2);
        socket_read(sfd, -1);
    }

    shutdown(sfd, SHUT_RDWR);
    close(sfd);

    DPRINTF("%s: Closed descriptor #%d\n", VNCFUNC, sfd);

    return 0;
}

/*
 * Private function name:   vnc_refresh_screen
 * Since version:           0.4.1(-3)
 * Description:             Function to send the key to VNC window to refresh the screen
 * Arguments:               @server [string]: server string to connect to
 *                          @port [string]: string version of port value to connect to
 *                          @scancode [int]: scancode to be sent to the guest's VNC window
 * Returns:                 0 on success, -errno otherwise
 */
int vnc_refresh_screen(char *server, char *port, int scancode)
{
    int sfd;
    tServerFBParams params;

    DPRINTF("%s: Server is %s, port is %s, scancode is %d\n", VNCFUNC, server, port, scancode);

    DPRINTF("%s: server is %s, port is %s\n", VNCFUNC, server, port);

    sfd = vnc_connect(server, port, 1);
    if (sfd < 0) {
        int err = errno;
        DPRINTF("%s: VNC Connection failed with error code %d (%s)\n", VNCFUNC, err, strerror(err));
        close(sfd);
        return -err;
    }

    params = vnc_read_server_init(sfd);

    DPRINTF("%s: Sending key press emulation for key %d\n", VNCFUNC, scancode);
    if (scancode > -1)
        vnc_send_key(sfd, scancode, 1, 0);

    DPRINTF("%s: Requesting framebuffer update\n", VNCFUNC);
    vnc_send_framebuffer_update_request(sfd, 1, params);

    shutdown(sfd, SHUT_RDWR);
    close(sfd);
    DPRINTF("%s: Closed descriptor #%d\n", VNCFUNC, sfd);
    return 0;
}

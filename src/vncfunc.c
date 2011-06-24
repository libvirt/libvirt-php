/*
* vnc.c: VNC Client functions to be used for the graphical VNC console of libvirt-php
*
* See COPYING for the license of this software
*
* Written by:
*   Michal Novotny <minovotn@redhat.com>
*/

#include "libvirt-php.h"

#ifdef DEBUG_VNC
#define DPRINTF(fmt, ...) \
if (gdebug)	\
do { fprintf(stderr, "[%s ", get_datetime()); fprintf(stderr, "libvirt-php/vnc ]: " fmt , ## __VA_ARGS__); fflush(stderr); } while (0)
#else
#define DPRINTF(fmt, ...) \
do {} while(0)
#endif

/* Function macro */
#define	PHPFUNC	__FUNCTION__

/*
	Private function name:	connect_socket
	Since version:		0.4.2
	Description:		Function is used to connect the socket and return the socket descriptor or error
	Arguments:		@server [string]: server string to connect to
				@port [string]: string version of port value to connect to
	Returns:		socket descriptor on success, -errno otherwise
*/
int connect_socket(char *server, char *port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;
	char name[1024] = { 0 };

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	/* Get the current hostname and override to localhost if local machine */
	gethostname(name, 1024);
	if (strcmp(name, server) == 0)
		server = strdup("localhost");

	DPRINTF("%s: Connecting to %s:%s\n", PHPFUNC, server, port);

	s = getaddrinfo(server, port, &hints, &result);
	if (s != 0)
		return -errno;

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;

		close(sfd);
	}

	if (rp == NULL)
		return -errno;

	freeaddrinfo(result);
	DPRINTF("%s: Socket descriptor #%d opened\n", PHPFUNC, sfd);

	return sfd;
}

/*
	Private function name:	vnc_refresh_screen
	Since version:		0.4.1(-3)
	Description:		Function to send the key to VNC window to refresh the screen
	Arguments:		@server [string]: server string to connect to
				@port [string]: string version of port value to connect to
				@scancode [int]: scancode to be sent to the guest's VNC window
	Returns:		0 on success, -errno otherwise
*/
int vnc_refresh_screen(char *server, char *port, int scancode)
{
	int sfd;
	char buf[1024] = { 0 };

	sfd = connect_socket(server, port);
	if (sfd < 0)
		return sfd;

	read(sfd, buf, 10);
	memset(buf, 0, 1024);
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
		close(sfd);
		return -errno;
	}

	read(sfd, buf, 1);
	buf[0] = 0x01;

	if (write(sfd, buf, 1) < 0) {
		close(sfd);
		return -errno;
	}

	read(sfd, buf, 1);
	buf[1] = 0x00;

	if (write(sfd, buf, 1) < 0) {
		close(sfd);
		return -errno;
	}
	
	read(sfd, buf, 1);
	memset(buf, 0, 1024);
	
	buf[0] = 0x04;
	buf[1] = 0x01;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = 0x00;
	buf[7] = scancode;

	if (write(sfd, buf, 8) < 0) {
		close(sfd);
		return -errno;
	}
	
	read(sfd, buf, 2);
	memset(buf, 0, 1024);
	buf[0] = 0x04;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = 0x00;
	buf[7] = scancode;
    
	if (write(sfd, buf, 8) < 0) {
		close(sfd);
		return -errno;
	}
	
	close(sfd);
	return 0;
}

/*
	Private function name:	vnc_send_keys
	Since version:		0.4.2
	Description:		Function to send the key to VNC window
	Arguments:		@server [string]: server string to specify VNC server
				@port [string]: string version of port value to connect to
				@keys [string]: string to be send to the guest's VNC window
	Returns:		0 on success, -errno otherwise
*/
int vnc_send_keys(char *server, char *port, char *keys)
{
	int sfd;
	char buf[1024] = { 0 };
	int i, skip_next;

	DPRINTF("%s: server = %s, port = %s, keys = %s\n", PHPFUNC, server, port, keys);

	sfd = connect_socket(server, port);
	if (sfd < 0)
		return sfd;

	DPRINTF("%s: Preparing to send keys '%s' (%d keys) to %s:%s\n", PHPFUNC, keys, strlen(keys), server, port);

	if (read(sfd, buf, 10) < 0) {
		int err = errno;
		DPRINTF("%s: Read function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
		close(sfd);
		return -err;
	}

	memset(buf, 0, 1024);
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
		close(sfd);
		return -errno;
	}

	DPRINTF("%s: VNC Client version packet sent\n", PHPFUNC);

	if (read(sfd, buf, 2) < 0) {
		int err = errno;
		DPRINTF("%s: Read function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
		close(sfd);
		return -err;
	}

	/* Just security type None is supported */
	buf[0] = 0x01;
	if (write(sfd, buf, 1) < 0) {
		close(sfd);
		return -errno;
	}

	DPRINTF("%s: Security None selected\n", PHPFUNC);

	/* Just bogus to wait for proper auth response */
	i = 0;
	buf[0] = 0x01;
	while (buf[0] + buf[1] + buf[2] + buf[3] != 0) {
		if (read(sfd, buf, 4) < 0) {
			int err = errno;
			DPRINTF("%s: Read function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
			close(sfd);
			return -err;
		}
		if (i++ > VNC_MAX_AUTH_ATTEMPTS) {
			close(sfd);
			return -EIO;
		}
	}

	/* Share desktop */
	buf[0] = 0x01;
	if (write(sfd, buf, 1) < 0) {
		close(sfd);
		return -errno;
	}

	DPRINTF("%s: Share desktop flag sent (%d)\n", PHPFUNC, buf[0]);

	skip_next = 0;
	for (i = 0; i < strlen(keys); i++) {
		DPRINTF("%s: Processing key %d: %d [0x%02x]\n", PHPFUNC, i, keys[i], keys[i]);
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

		if (read(sfd, buf, 10) < 0) {
			int err = errno;
			DPRINTF("%s: Read function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
			close(sfd);
			return -err;
		}
		memset(buf, 0, 1024);
		buf[0] = 0x04;
		buf[1] = 0x01;
		buf[2] = 0x00;
		buf[3] = 0x00;
		buf[4] = 0x00;
		buf[5] = 0x00;
		buf[6] = skip_next ? 0xff : 0x00;
		buf[7] = (unsigned char)keys[i];
		DPRINTF("%s: Pressing key %d: %d [0x%02x], skip next: %s\n", PHPFUNC, i, keys[i], keys[i],
			skip_next ? "true" : "false");

		if (write(sfd, buf, 8) < 0) {
			int err = errno;
			DPRINTF("%s: Error occured while writing to socket descriptor #%d: %d (%s)\n",
				PHPFUNC, sfd, err, strerror(err));
			close(sfd);
			return -err;
		}

		if (read(sfd, buf, 1) < 0) {
			int err = errno;
			DPRINTF("%s: Read function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
			close(sfd);
			return -err;
		}
		/* Client update framebuffer request */
		memset(buf, 0, 1024);
		buf[0] = 0x03;
		buf[1] = 0x01;
		buf[2] = 0x00;
		buf[3] = 0x00;
		buf[4] = 0x00;
		buf[5] = 0x00;
		buf[6] = 0x02;
		buf[7] = 0xd0;
		buf[8] = 0x01;
		buf[9] = 0x90;

		DPRINTF("%s: Sending client update framebuffer request (10 bytes)\n", PHPFUNC);

		if (write(sfd, buf, 10) < 0) {
			int err = errno;
			DPRINTF("%s: Error occured while writing to socket descriptor #%d: %d (%s)\n",
				PHPFUNC, sfd, err, strerror(err));
			close(sfd);
			return -err;
		}

		memset(buf, 0, 1024);
	
		buf[0] = 0x04;
		buf[1] = 0x00;
		buf[2] = 0x00;
		buf[3] = 0x00;
		buf[4] = 0x00;
		buf[5] = 0x00;
		buf[6] = skip_next ? 0xff : 0x00;
		buf[7] = (unsigned char)keys[i];
		DPRINTF("%s: Releasing key %d: %d [0x%02x], skip next: %s\n", PHPFUNC, i, keys[i], keys[i],
			skip_next ? "true" : "false");

		if (write(sfd, buf, 8) < 0) {
			int err = errno;
			DPRINTF("%s: Error occured while writing to socket descriptor #%d: %d (%s)\n",
				PHPFUNC, sfd, err, strerror(err));
			close(sfd);
			return -err;
		}

		/* Sleep for 50 ms, required to make VNC server accept the keystroke emulation */
		usleep(50000);
	}

	DPRINTF("%s: All %d keys sent\n", PHPFUNC, strlen(keys));

	close(sfd);
	DPRINTF("%s: Closed descriptor #%d\n", PHPFUNC, sfd);
	return 0;
}

/*
	Private function name:	vnc_send_pointer_event
	Since version:		0.4.2
	Description:		Function to send the mouse pointer event/click to VNC window
	Arguments:		@server [string]: server string to specify VNC server
				@port [string]: string version of port value to connect to
				@pos_x [int]: position on x-axis of the VNC window
				@pos_y [int]: position on y-axis of the VNC window
				@clicked [int]: mask of what buttons were clicked
				@release [boolean]: release the buttons once clicked
	Returns:		0 on success, -errno otherwise
*/
int vnc_send_pointer_event(char *server, char *port, int pos_x, int pos_y, int clicked, int release)
{
	int sfd, j;
	char buf[1024] = { 0 };
	int i, ok, h1, h2, w1, w2, h, w;

	DPRINTF("%s: server = %s, port = %s, x = %d, y = %d, clicked = %d, release = %d\n", PHPFUNC,
		server, port, pos_x, pos_y, clicked, release);

	sfd = connect_socket(server, port);
	if (sfd < 0)
		return sfd;

	DPRINTF("%s: Opened socket with descriptor #%d\n", PHPFUNC, sfd);

	/* Compose and send VNC client protocol version */
	if (read(sfd, buf, 1024) < 0) {
		int err = errno;
		DPRINTF("%s: Read function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
		close(sfd);
		return -err;
	}

	memset(buf, 0, 1024);
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
		DPRINTF("%s: Write function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
		close(sfd);
		return -err;
	}

	DPRINTF("%s: VNC Client version packet sent\n", PHPFUNC);

	/* Security types supported */
	if (read(sfd, buf, 1) < 0) {
		int err = errno;
		DPRINTF("%s: Read function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
		close(sfd);
		return -err;
	}

	j = read(sfd, buf, buf[0]);
	if (j < 0) {
		int err = errno;
		DPRINTF("%s: Read function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
		close(sfd);
		return -errno;
	}

	DPRINTF("%s: Got list of security types\n", PHPFUNC);

	/* Check for None type supported by server */
	ok = 0;
	for (i = 0; i < j; i++)
		if (buf[i] == 0x01)
			ok = 1;

	if (!ok) {
		DPRINTF("%s: Security type None is not supported!\n", PHPFUNC);
		return -EPERM;
	}

	memset(buf, 0, 1024);

	/* Request security type of None */
	buf[0] = 0x01;

	if (write(sfd, buf, 1) < 0) {
		int err = errno;
		DPRINTF("%s: Write function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
		close(sfd);
		return -err;
	}

	DPRINTF("%s: Security type None requested\n", PHPFUNC);

	/* Just bogus to wait for proper auth response */
	i = 0;
	buf[0] = 0x01;
	while (buf[0] + buf[1] + buf[2] + buf[3] != 0) {
		if (read(sfd, buf, 4) < 0) {
			int err = errno;
			DPRINTF("%s: Read function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
			close(sfd);
			return -err;
		}
		if (i++ > VNC_MAX_AUTH_ATTEMPTS) {
			close(sfd);
			return -EIO;
		}
	}

	DPRINTF("%s: Authentication was successful\n", PHPFUNC);

	/* Share desktop */
	buf[0] = 0x01;

	if (write(sfd, buf, 1) < 0) {
		int err = errno;
		DPRINTF("%s: Write function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
		close(sfd);
		return -err;
	}

	DPRINTF("%s: Sending share desktop flag (%d)\n", PHPFUNC, buf[0]);

	/* Read server framebuffer parameters */
	DPRINTF("%s: Reading framebuffer parameters\n", PHPFUNC);
	if (read(sfd, buf, 36) < 0) {
		int err = errno;
		DPRINTF("%s: Read function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
		close(sfd);
		return -err;
	}

	w1 = (unsigned char)buf[0];
	w2 = (unsigned char)buf[1];
	h1 = (unsigned char)buf[2];
	h2 = (unsigned char)buf[3];

	DPRINTF("%s: Read dimension bytes: width = { 0x%02x, 0x%02x }, height = { 0x%02x, 0x%02x }, %s endian\n", PHPFUNC,
		w1, w2, h1, h2, (buf[6] == 0) ? "little" : "big");

	if (buf[6] != 0) {
		h = (h1 << 8) + h2;
		w = (w1 << 8) + w2;
	}
	else {
		h = (h2 << 8) + h1;
		w = (w2 << 8) + w1;
	}

	/* Client pointer event */
	memset(buf, 0, 1024);
	buf[0] = 0x05;
	buf[1] = clicked;
	buf[2] = (unsigned char)(pos_x / 256);
	buf[3] = (unsigned char)(pos_x % 256);
	buf[4] = (unsigned char)(pos_y / 256);
	buf[5] = (unsigned char)(pos_y % 256);

	if (write(sfd, buf, 6) < 0) {
		int err = errno;
		DPRINTF("%s: Read function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
		close(sfd);
		return -err;
	}

	DPRINTF("%s: Wrote 6 bytes of client pointer event, clicked = %d, x = { 0x%02x, 0x%02x}, y = { 0x%02x, 0x%02x }\n",
		PHPFUNC, buf[1], buf[2], buf[3], buf[4], buf[5]);

	/* Client update framebuffer request */
	memset(buf, 0, 1024);
	buf[0] = 0x03;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = w1;
	buf[7] = w2;
	buf[8] = h1;
	buf[9] = h2;

	DPRINTF("%s: Client update framebuffer request sent\n", PHPFUNC);

	if (write(sfd, buf, 10) < 0) {
		int err = errno;
		DPRINTF("%s: Write function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
		close(sfd);
		return -err;
	}

	if (release) {
		usleep(50000);
		memset(buf, 0, 1024);
		buf[0] = 0x05;
		buf[1] = 0x00;
		buf[2] = (unsigned char)(pos_x / 256);
		buf[3] = (unsigned char)(pos_x % 256);
		buf[4] = (unsigned char)(pos_y / 256);
		buf[5] = (unsigned char)(pos_y % 256);

		if (write(sfd, buf, 6) < 0) {
			int err = errno;
			DPRINTF("%s: Write function failed with error code %d (%s)\n", PHPFUNC, err, strerror(err));
			close(sfd);
			return -err;
		}

		DPRINTF("%s: Wrote 6 bytes of client pointer event for the second time, clicked = 0 (release all), "
			"x = { 0x%02x, 0x%02x}, y = { 0x%02x, 0x%02x }\n", PHPFUNC, buf[1], buf[2], buf[3], buf[4]);
	}
	
	close(sfd);

	DPRINTF("%s: Closed descriptor #%d\n", PHPFUNC, sfd);

	return 0;
}


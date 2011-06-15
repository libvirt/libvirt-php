/*
* vnc.c: VNC Client functions to be used for the graphical VNC console of libvirt-php
*
* See COPYING for the license of this software
*
* Written by:
*   Michal Novotny <minovotn@redhat.com>
*/

#include "libvirt-php.h"

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
	ssize_t nread;
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
	
	return sfd;
}

/*
	Private function name:	vnc_refresh_screen
	Since version:		0.4.1(-3)
	Description:		Function to send the key to VNC window to refresh the screen, accepts both port and key scancode
	Arguments:		@server [string]: server string to connect to
				@port [string]: string version of port value to connect to
				@scancode [int]: key scancode
	Returns:		0 on success, -errno otherwise
*/
int vnc_refresh_screen(char *server, int *port, int scancode)
{
	vnc_send_pointer_event(server, port, 0, 0, 0, 0);
	vnc_send_pointer_event(server, port, 1, 1, 0, 1);
}

// Obsolete version
int vnc_refresh_screen_old(char *server, char *port, int scancode)
{
	int sfd, j;
	size_t len;
	ssize_t nread;
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
	int sfd, j;
	size_t len;
	ssize_t nread;
	char buf[1024] = { 0 };
	int i, skip_next;

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

	read(sfd, buf, 2);

	/* Just security type None is supported */
	buf[0] = 0x01;
	if (write(sfd, buf, 1) < 0) {
		close(sfd);
		return -errno;
	}

	/* Just bogus to wait for proper auth response */
	i = 0;
	buf[0] = 0x01;
	while (buf[0] + buf[1] + buf[2] + buf[3] != 0) {
		read(sfd, buf, 4);
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

	for (i = 0; i < strlen(keys); i++) {
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

		read(sfd, buf, 10);
		memset(buf, 0, 1024);
		buf[0] = 0x04;
		buf[1] = 0x01;
		buf[2] = 0x00;
		buf[3] = 0x00;
		buf[4] = 0x00;
		buf[5] = 0x00;
		buf[6] = skip_next ? 0xff : 0x00;
		buf[7] = keys[i];

		if (write(sfd, buf, 8) < 0) {
			close(sfd);
			return -errno;
		}

		read(sfd, buf, 1);
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

		if (write(sfd, buf, 10) < 0) {
			close(sfd);
			return -errno;
		}

		memset(buf, 0, 1024);
	
		buf[0] = 0x04;
		buf[1] = 0x00;
		buf[2] = 0x00;
		buf[3] = 0x00;
		buf[4] = 0x00;
		buf[5] = 0x00;
		buf[6] = skip_next ? 0xff : 0x00;
		buf[7] = keys[i];

		if (write(sfd, buf, 8) < 0) {
			close(sfd);
			return -errno;
		}

		/* Sleep for 50 ms, required to make VNC server accept the keystroke emulation */
		usleep(50000);
	}
	
	close(sfd);
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
	size_t len;
	ssize_t nread;
	char buf[1024] = { 0 };
	int i, ok, h1, h2, w1, w2, h, w;

	sfd = connect_socket(server, port);
	if (sfd < 0)
		return sfd;

	/* Compose and send VNC client protocol version */
	read(sfd, buf, 1024);

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

	/* Security types supported */
	read(sfd, buf, 1);
	j = read(sfd, buf, buf[0]);

	/* Check for None type supported by server */
	ok = 0;
	for (i = 0; i < j; i++)
		if (buf[i] == 0x01)
			ok = 1;

	if (!ok)
		return -EPERM;

	memset(buf, 0, 1024);

	/* Request security type of None */
	buf[0] = 0x01;

	if (write(sfd, buf, 1) < 0) {
		close(sfd);
		return -errno;
	}

	/* Just bogus to wait for proper auth response */
	i = 0;
	buf[0] = 0x01;
	while (buf[0] + buf[1] + buf[2] + buf[3] != 0) {
		read(sfd, buf, 4);
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

	/* Read server framebuffer parameters */
	read(sfd, buf, 36);
	w1 = buf[0];
	w2 = buf[1];
	h1 = buf[2];
	h2 = buf[3];

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
	buf[2] = pos_x / 256;
	buf[3] = pos_x % 256;
	buf[4] = pos_y / 256;
	buf[5] = pos_y % 256;

	if (write(sfd, buf, 6) < 0) {
		close(sfd);
		return -errno;
	}

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

	if (write(sfd, buf, 10) < 0) {
		close(sfd);
		return -errno;
	}

	if (release) {
		usleep(50000);
		memset(buf, 0, 1024);
		buf[0] = 0x05;
		buf[1] = 0x00;
		buf[2] = pos_x / 256;
		buf[3] = pos_x % 256;
		buf[4] = pos_y / 256;
		buf[5] = pos_y % 256;

		if (write(sfd, buf, 6) < 0) {
			close(sfd);
			return -errno;
		}
	}
	
	close(sfd);
	return 0;
}


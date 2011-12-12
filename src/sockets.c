/*
* sockets.c: Socket functions for libvirt-php
*
* See COPYING for the license of this software
*
* Written by:
*   Michal Novotny <minovotn@redhat.com>
*/

#include "libvirt-php.h"

#ifdef DEBUG_SOCKETS
#define DPRINTF(fmt, ...) \
if (gdebug)	\
do { fprintf(stderr, "[%s ", get_datetime()); fprintf(stderr, "libvirt-php/sockets]: " fmt , ## __VA_ARGS__); fflush(stderr); } while (0)
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
				@keepalive [bool]: determines whether to create keepalive socket or not
				@nodelay [bool]: determines whether to set no-delay option on socket
				@allow_server_override [bool]: allows function to override server to localhost if server equals local hostname
	Returns:		socket descriptor on success, -errno otherwise
*/
int connect_socket(char *server, char *port, int keepalive, int nodelay, int allow_server_override)
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

	if (allow_server_override) {
		/* Get the current hostname and override to localhost if local machine */
		gethostname(name, 1024);
		if (strcmp(name, server) == 0)
			server = strdup("localhost");
	}

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

	if (keepalive) {
		int on = 1;
		if (setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)) < 0) {
			int err = errno;
			close(sfd);
			DPRINTF("%s: Cannot set keep alive option on socket\n", PHPFUNC);
			return -err;
		}

		DPRINTF("%s: Socket #%d set as keepalive socket\n", PHPFUNC, sfd);
	}

	if (nodelay) {
		int on = 1;
		if (setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) < 0) {
			int err = errno;
			close(sfd);
			DPRINTF("%s: Cannot set no delay option on socket\n", PHPFUNC);
			return -err;
		}

		DPRINTF("%s: Socket #%d set as no delay socket\n", PHPFUNC, sfd);
	}

	return sfd;
}

/*
	Private function name:	socket_has_data
	Since version:		0.4.3
	Description:		Function to check socket for data in queue
	Arguments:		@sfd [int]: socket descriptor for existing VNC client socket
				@maxtime [bool]: maximum wait time in milliseconds
				@ignoremsg [bool]: flag to print messages if debug log enabled (useful to set to 1 for recursion)
	Returns:		0 on success, -errno otherwise
*/
int socket_has_data(int sfd, long maxtime, int ignoremsg)
{
	fd_set fds;
	struct timeval timeout;
	int rc;

	if (maxtime > 0) {
		timeout.tv_sec = maxtime / 1000000;
		timeout.tv_usec = (maxtime % 1000000);
	}

	if (!ignoremsg)
		DPRINTF("%s: Checking data on socket %d, timeout = { %ld, %ld }\n", PHPFUNC, sfd,
			(long)timeout.tv_sec, (long)timeout.tv_usec);

	FD_ZERO(&fds);
	FD_SET(sfd, &fds);
	if (maxtime > 0)
		rc = select( sizeof(fds), &fds, NULL, NULL, &timeout);
	else
		rc = select( sizeof(fds), &fds, NULL, NULL, NULL);

	if (rc==-1) {
		DPRINTF("%s: Select with error %d (%s)\n", PHPFUNC, errno, strerror(-errno));
		return -errno;
	}

	if (!ignoremsg)
		DPRINTF("%s: Select returned %d\n", PHPFUNC, rc);
		
	return (rc == 1);
}

/*
	Private function name:	socket_read
	Since version:		0.4.3
	Description:		Function to read the data from socket
	Arguments:		@sfd [int]: socket descriptor for existing VNC client socket
				@length [bool]: length of the data to be read or -1 for all the data
	Returns:		0 on success, -errno otherwise
*/
void socket_read(int sfd, long length)
{
	long len = 0;
	unsigned char bigbuf[1048576];

	if (socket_has_data(sfd, 50000, 0) != 1) {
		DPRINTF("%s: No data appears to be available\n", PHPFUNC);
		return;
	}
	
	if (length == -1) {
		DPRINTF("%s: Reading all the data from socket\n", PHPFUNC);
		while (socket_has_data(sfd, 50000, 1) == 1)
			while ((len = read(sfd, bigbuf, sizeof(bigbuf))) == sizeof(bigbuf)) ;
		DPRINTF("%s: Read done ...\n", PHPFUNC);
		return;
	}

	DPRINTF("%s: Reading %ld bytes\n", PHPFUNC, length);
	while (length > 0) {
		len = read(sfd, bigbuf, sizeof(bigbuf));

		length -= len;
		if (length < 0)
			length = 0;
	}

	if (length)
		read(sfd, bigbuf, length);

	DPRINTF("%s: All bytes read\n", PHPFUNC);
}

/*
	Private function name:	socket_read_and_save
	Since version:		0.4.5
	Description:		Function to read the data from socket and save them into a file identified by fn
	Arguments:		@sfd [int]: socket descriptor for existing VNC client socket
				@fn [string]: filename to save data to
				@length [bool]: length of the data to be read or -1 for all the data
	Returns:		0 on success, -errno otherwise
*/
int socket_read_and_save(int sfd, char *fn, long length)
{
	int fd, i;
        long len = 0;
	long orig_len = length;
        unsigned char bigbuf[1048576];

	if (fn == NULL)
		return -ENOENT;

	orig_len = length;
	fd = open(fn, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		return -EPERM;

        if (socket_has_data(sfd, 50000, 0) != 1) {
                DPRINTF("%s: No data appears to be available\n", PHPFUNC);
                return -ENOENT;
        }

        DPRINTF("%s: Reading %ld bytes\n", PHPFUNC, length);
        while (length > 0) {
                len = read(sfd, bigbuf, sizeof(bigbuf));

		for (i = 0; i < len; i += 4)
			SWAP2_BYTES_ENDIAN(1, bigbuf[i+1], bigbuf[i+2]);

		write(fd, bigbuf, len);

                length -= len;
                if (length < 0)
                        length = 0;
        }

        if (length) {
                len = read(sfd, bigbuf, length);

		for (i = 0; i < len; i += 4)
			SWAP2_BYTES_ENDIAN(1, bigbuf[i+1], bigbuf[i+2]);

		write(fd, bigbuf, len);
	}

	ftruncate(fd, orig_len);

	close(fd);

        DPRINTF("%s: All bytes read\n", PHPFUNC);
	return 0;
}


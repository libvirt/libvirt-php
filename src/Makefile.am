WL=@WL@
SHLIB_FLAGS=@SHLIB_FLAGS@

AM_CFLAGS = \
	$(PHP_CFLAGS) $(LIBXML_CFLAGS) \
	$(LIBVIRT_CFLAGS) $(QEMU_CFLAGS) \
	-I$(top_srcdir)/winsrc

AM_LDFLAGS = \
	$(SHLIB_LDFLAGS) \
	$(PHP_LDFLAGS) \
	$(LIBXML_LIBS) \
	$(LIBVIRT_LIBS) \
	$(QEMU_LIBS)

LIBVIRT_PHP_SYMBOL_FILE = \
	$(srcdir)/libvirt_php.syms
php_plugindir = $(extensiondir)
php_plugin_LTLIBRARIES = libvirt-php.la
libvirt_php_la_SOURCES = \
	util.c util.h	\
	vncfunc.c vncfunc.h	\
	sockets.c sockets.h \
	libvirt-php.c libvirt-php.h \
	libvirt-connection.c libvirt-connection.h \
	libvirt-domain.c libvirt-domain.h \
	libvirt-network.c libvirt-network.h \
	libvirt-node.c libvirt-node.h \
	libvirt-nodedev.c libvirt-nodedev.h \
	libvirt-nwfilter.c libvirt-nwfilter.h \
	libvirt-snapshot.c libvirt-snapshot.h \
	libvirt-storage.c libvirt-storage.h \
	libvirt-stream.c libvirt-stream.h
libvirt_php_la_CFLAGS = \
	$(AM_CFLAGS) \
	-DCOMPILE_DL_LIBVIRT=1
libvirt_php_la_LDFLAGS = \
	$(VERSION_SCRIPT_FLAGS)$(LIBVIRT_PHP_SYMBOL_FILE)	\
	$(AM_LDFLAGS)	\
	-module	\
	-avoid-version	\
	-export-dynamic	\
	-shared

php_confdir = $(confdir)
php_conf_DATA = libvirt-php.ini

EXTRA_DIST = $(php_conf_DATA) \
	config.m4	\
	$(LIBVIRT_PHP_SYMBOL_FILE)

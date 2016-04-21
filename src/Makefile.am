DEFINES=-DHAVE_CONFIG_H
WL=@WL@
SHLIB_FLAGS=@SHLIB_FLAGS@

AM_CFLAGS = \
	$(PHP_CFLAGS) $(LIBXML_CFLAGS) \
	$(LIBVIRT_CFLAGS) $(QEMU_CFLAGS) $(DEFINES) \
	-I$(top_srcdir)/winsrc

AM_LDFLAGS = \
	$(SHLIB_LDFLAGS) \
	$(PHP_LDFLAGS) \
	$(LIBXML_LIBS) \
	$(LIBVIRT_LIBS) \
	$(QEMU_LIBS)

php_plugindir = $(extensiondir)
php_plugin_LTLIBRARIES = libvirt-php.la
libvirt_php_la_SOURCES = \
	vncfunc.c \
	sockets.c \
	libvirt-php.c libvirt-php.h
libvirt_php_la_CFLAGS = \
	$(AM_CFLAGS) \
	-DCOMPILE_DL_LIBVIRT=1

php_confdir = $(confdir)
php_conf_DATA = libvirt-php.ini

EXTRA_DIST = $(php_conf_DATA)

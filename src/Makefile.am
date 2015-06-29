PHPINC=$(shell $(PHPCONFIG) --includes)
PHPEDIR=$(shell $(PHPCONFIG) --extension-dir)
PHPCDIR=$(shell $(PHPCONFIG) --configure-options | sed -n 's|.*--with-config-file-scan-dir=\([^ ]*\).*|\1|p')
DEFINES=-DHAVE_CONFIG_H
WL=@WL@
SHLIB_FLAGS=@SHLIB_FLAGS@

install-exec-local:
	$(MKDIR_P) $(DESTDIR)$(PHPEDIR)
	$(MKDIR_P) $(DESTDIR)$(PHPCDIR)
	$(INSTALL) -m 644 -D .libs/$(PACKAGE).so $(DESTDIR)$(PHPEDIR)/$(PACKAGE).so
	$(INSTALL) -m 755 -d $(DESTDIR)$(PHPCDIR)
	$(ECHO) "extension=$(PACKAGE).so" > libvirt-php.ini
	$(ECHO) >> libvirt-php.ini
	$(ECHO) "[libvirt]" >> libvirt-php.ini
	$(ECHO) "; Path to ISO images for VM installations" >> libvirt-php.ini
	$(ECHO) "libvirt.iso_path=/var/lib/libvirt/images" >> libvirt-php.ini
	$(ECHO) "; Path where disk images for new VMs should be created" >> libvirt-php.ini
	$(ECHO) "libvirt.image_path=/var/lib/libvirt/images" >> libvirt-php.ini
	$(ECHO) "; Limit maximum number of libvirt connections" >> libvirt-php.ini
	$(ECHO) "libvirt.max_connections=5" >> libvirt-php.ini
	$(INSTALL) -m 644 -D libvirt-php.ini $(DESTDIR)$(PHPCDIR)/$(PACKAGE).ini

uninstall-local:
	$(RM) -f $(DESTDIR)$(PHPCDIR)/$(PACKAGE).ini
	$(RM) -f $(DESTDIR)$(PHPEDIR)/$(PACKAGE).so

AM_CFLAGS = \
	$(PHPINC) $(LIBXML_CFLAGS) \
	$(LIBVIRT_CFLAGS) $(QEMU_CFLAGS) $(DEFINES) \
	-I$(top_srcdir)/winsrc

AM_LDFLAGS = \
	$(SHLIB_LDFLAGS) \
	$(LIBXML_LIBS) \
	$(LIBVIRT_LIBS) \
	$(QEMU_LIBS)

lib_LTLIBRARIES = libvirt-php.la

libvirt_php_la_SOURCES = \
	vncfunc.c \
	sockets.c \
	libvirt-php.c libvirt-php.h
libvirt_php_la_CFLAGS = \
	$(AM_CFLAGS) \
	-DCOMPILE_DL_LIBVIRT=1

CLEANFILES = libvirt-php.ini

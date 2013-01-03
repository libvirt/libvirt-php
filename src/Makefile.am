PHPINC=$(shell $(PHPCONFIG) --includes)
PHPEDIR=$(shell $(PHPCONFIG) --extension-dir)
PHPCDIR=$(shell $(PHPCONFIG) --configure-options | sed -n 's|.*--with-config-file-scan-dir=\([^ ]*\).*|\1|p')
DEFINES=-DHAVE_CONFIG_H
WL=@WL@
SHLIB_FLAGS=@SHLIB_FLAGS@

EXTRA_DIST = libvirt-php.c sockets.c vncfunc.c libvirt-php.h


all-am: build

build:
	$(CC) $(CFLAGS) -Wall -fpic -DCOMPILE_DL_LIBVIRT=1 $(PHPINC) -c -o libvirt-php.o libvirt-php.c $(LIBXML_CFLAGS) $(LIBVIRT_CFLAGS) $(DEFINES)
	$(CC) $(CFLAGS) -Wall -fpic -DCOMPILE_DL_LIBVIRT=1 -c -o vncfunc.o vncfunc.c $(PHPINC) $(LIBXML_CFLAGS) $(LIBVIRT_CFLAGS) $(DEFINES)
	$(CC) $(CFLAGS) -Wall -fpic -DCOMPILE_DL_LIBVIRT=1 -c -o sockets.o sockets.c $(PHPINC) $(LIBXML_CFLAGS) $(LIBVIRT_CFLAGS) $(DEFINES)
	$(CC) $(CFLAGS) -Wall $(SHLIB_FLAGS) $(LIBS) -o $(PACKAGE).so vncfunc.o sockets.o libvirt-php.o -ldl -lvirt $(LIBXML_LIBS) $(LIBVIRT_LIBS)
	$(ECHO) "Extension compiled as $(PACKAGE).so"

install-exec-local:
	$(INSTALL) -m 644 -D $(PACKAGE).so $(DESTDIR)$(PHPEDIR)/$(PACKAGE).so
	$(INSTALL) -m 755 -d $(DESTDIR)$(PHPCDIR)
	$(ECHO) "extension=$(PACKAGE).so" > $(DESTDIR)$(PHPCDIR)/libvirt-php.ini
	$(ECHO) >> $(DESTDIR)$(PHPCDIR)/libvirt-php.ini
	$(ECHO) "[libvirt]" >> $(DESTDIR)$(PHPCDIR)/libvirt-php.ini
	$(ECHO) "; Path to ISO images for VM installations" >> $(DESTDIR)$(PHPCDIR)/libvirt-php.ini
	$(ECHO) "libvirt.iso_path=/var/lib/libvirt/images" >> $(DESTDIR)$(PHPCDIR)/libvirt-php.ini
	$(ECHO) "; Path where disk images for new VMs should be created" >> $(DESTDIR)$(PHPCDIR)/libvirt-php.ini
	$(ECHO) "libvirt.image_path=/var/lib/libvirt/images" >> $(DESTDIR)$(PHPCDIR)/libvirt-php.ini
	$(ECHO) "; Limit maximum number of libvirt connections" >> $(DESTDIR)$(PHPCDIR)/libvirt-php.ini
	$(ECHO) "libvirt.max_connections=5" >> $(DESTDIR)$(PHPCDIR)/libvirt-php.ini

uninstall-local:
	$(RM) -f $(DESTDIR)$(PHPCDIR)/$(PACKAGE).ini
	$(RM) -f $(DESTDIR)$(PHPEDIR)/$(PACKAGE).so

clean-local:
	$(RM) -f *.o

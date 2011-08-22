PHPINC=$(shell $(PHPCONFIG) --includes)
PHPEDIR=$(shell $(PHPCONFIG) --extension-dir)
PHPCDIR=$(shell $(PHPCONFIG) --configure-options | sed -n 's|.*--with-config-file-scan-dir=\([^ ]*\).*|\1|p')
DEFINES=-DHAVE_CONFIG_H

EXTRA_DIST = libvirt-php.c sockets.c vncfunc.c libvirt-php.h

all-am: build clean-temp

build:
	$(CC) $(CFLAGS) -fpic -DCOMPILE_DL_LIBVIRT=1 $(PHPINC) -c -o libvirt-php.o libvirt-php.c $(LIBXML_CFLAGS) $(DEFINES)
	$(CC) $(CFLAGS) -fpic -DCOMPILE_DL_LIBVIRT=1 -c -o vncfunc.o vncfunc.c $(PHPINC) $(LIBXML_CFLAGS) $(DEFINES)
	$(CC) $(CFLAGS) -fpic -DCOMPILE_DL_LIBVIRT=1 -c -o sockets.o sockets.c $(PHPINC) $(LIBXML_CFLAGS) $(DEFINES)
	$(CC) $(CFLAGS) -shared $(LIBS) -rdynamic -o $(PACKAGE).so vncfunc.o sockets.o libvirt-php.o -ldl -lvirt $(LIBXML_LIBS)
	$(ECHO) "Extension compiled as $(PACKAGE).so"

install-exec-local:
	$(INSTALL) -m 644 -D $(PACKAGE).so $(DESTDIR)$(PHPEDIR)/$(PACKAGE).so
	$(INSTALL) -m 755 -d $(DESTDIR)$(PHPCDIR)
	$(ECHO) "extension=$(PACKAGE).so" > $(DESTDIR)$(PHPCDIR)/$(PACKAGE).ini

uninstall-local:
	$(RM) -f $(DESTDIR)$(PHPCDIR)/$(PACKAGE).ini
	$(RM) -f $(DESTDIR)$(PHPEDIR)/$(PACKAGE).so

clean-temp:
	$(RM) -f *.o

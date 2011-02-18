PHPINC=$(shell php-config --includes)
PHPEDIR=$(shell php-config --extension-dir)
PHPCDIR=$(shell php-config --configure-options | sed -n 's|.*--with-config-file-scan-dir=\([^ ]*\).*|\1|p')

EXTRA_DIST = libvirt.c php_libvirt.h

all-am: build clean-temp

build:
	$(CC) -Wall -fpic -DCOMPILE_DL_LIBVIRT=1 $(PHPINC) -c -o $(PACKAGE).o libvirt.c $(XML_CPPFLAGS)
	$(CC) -Wall -shared $(LIBS) -rdynamic -o $(PACKAGE).so $(PACKAGE).o -ldl -lvirt $(XML_LIBS)
	$(ECHO) "Extension compiled as $(PACKAGE).so"

install-exec-local:
	$(INSTALL) -m 644 -D --strip $(PACKAGE).so $(DESTDIR)$(PHPEDIR)/$(PACKAGE).so
	$(INSTALL) -m 755 -d $(DESTDIR)$(PHPCDIR)
	$(ECHO) "extension=$(PACKAGE).so" > $(DESTDIR)$(PHPCDIR)/libvirt-php.ini

uninstall-local:
	$(RM) -f $(DESTDIR)$(PHPCDIR)/$(PACKAGE).ini
	$(RM) -f $(DESTDIR)$(PHPEDIR)/$(PACKAGE).so

clean-temp:
	$(RM) -f *.o

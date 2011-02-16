NAME=libvirt-php
CC=cc
CP=cp
RM=rm
ECHO=echo
PHPINC=`php-config --includes`
PHPEDIR=`php-config --extension-dir`
PHPCDIR=`php-config --configure-options | sed -n 's|.*--with-config-file-scan-dir=\([^ ]*\).*|\1|p'`
XMLCFLAGS=`pkg-config libxml-2.0 --cflags`
XMLLIBS=`pkg-config libxml-2.0 --libs`

EXTRA_DIST = libvirt.c php_libvirt.h

all: build clean-temp

build:
	$(CC) -Wall -fpic -DCOMPILE_DL_LIBVIRT=1 $(PHPINC) -c -o $(NAME).o libvirt.c $(XMLCFLAGS)
	$(CC) -Wall -shared $(LIBS) -rdynamic -o $(NAME).so $(NAME).o -ldl -lvirt $(XMLLIBS)
	$(ECHO) "Extension compiled as $(NAME).so"

install-exec-local:
	mkdir -p $(DESTDIR)$(PHPEDIR)
	$(CP) $(NAME).so $(DESTDIR)$(PHPEDIR)
	mkdir -p $(DESTDIR)$(PHPCDIR)
	$(ECHO) "extension=$(NAME).so" > $(DESTDIR)$(PHPCDIR)/libvirt-php.ini


uninstall-local:
	$(RM) -f $(DESTDIR)$(PHPCDIR)/$(NAME).ini
	$(RM) -f $(DESTDIR)$(PHPEDIR)/$(NAME).so

clean-temp:
	$(RM) -f *.o

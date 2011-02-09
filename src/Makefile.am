NAME=libvirt-php
CC=cc
CP=cp
RM=rm
ECHO=echo
PHPINC=`php-config --includes`
PHPEDIR=`php-config --extension-dir`
XMLCFLAGS=`pkg-config libxml-2.0 --cflags`
XMLLIBS=`pkg-config libxml-2.0 --libs`

all: compile clean-temp

compile:
	$(CC) -fpic -DCOMPILE_DL_LIBVIRT=1 $(PHPINC) -c -o $(NAME).o libvirt.c $(XMLCFLAGS)
	$(CC) -shared $(LIBS) -rdynamic -o $(NAME).so $(NAME).o -ldl -lvirt $(XMLLIBS)
	$(ECHO) "Extension compiled as $(NAME).so"

install-exec-local:
	$(CP) $(NAME).so $(PHPEDIR)
	$(ECHO) "extension=$(NAME)" > /etc/php.d/libvirt-php.ini

uninstall-local:
	$(RM) -f /etc/php.d/$(NAME).ini
	$(RM) -f $(PHPEDIR)/$(NAME).so

clean-temp:
	$(RM) -f *.o

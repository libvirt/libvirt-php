NAME=libvirt-php
CC=cc
RM=rm
PHPINC=`php-config --includes`
PHPEDIR=`php-config --extension-dir`
XMLCFLAGS=`pkg-config libxml-2.0 --cflags`
XMLLIBS=`pkg-config libxml-2.0 --libs`

all: compile clean-temp

compile:
	$(CC) -fpic -DCOMPILE_DL_LIBVIRT=1 $(PHPINC) -c -o $(NAME).o libvirt.c $(XMLCFLAGS)
	$(CC) -shared $(LIBS) -rdynamic -o $(NAME).so $(NAME).o -ldl -lvirt $(XMLLIBS)
	echo "Extension compiled as $(NAME).so"

install-module:
	cp $(NAME).so $(PHPEDIR)
	echo "extension=php_libvirt.so" > /etc/php.d/libvirt-php.ini

clean-temp:
	$(RM) -f *.o

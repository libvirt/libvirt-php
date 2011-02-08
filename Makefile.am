CC=cc
PHPINC=`php-config --includes`

all: clean-so compile

compile:
	$(CC) -fpic -DCOMPILE_DL_LIBVIRT=1 $(PHPINC) -c -o php_libvirt.o libvirt.c `pkg-config libxml-2.0 --cflags`
	$(CC) -shared $(LIBS) -rdynamic -o php_libvirt.so php_libvirt.o -ldl -lvirt `pkg-config libxml-2.0 --libs`
	echo "Extension compiled as php_libvirt.so"

clean-so:
	rm -f php_libvirt.o php_libvirt.so

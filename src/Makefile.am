CC=cc
RM=rm
PHPINC=`php-config --includes`

all: compile clean-temp

compile:
	$(CC) -fpic -DCOMPILE_DL_LIBVIRT=1 $(PHPINC) -c -o php_libvirt.o libvirt.c `pkg-config libxml-2.0 --cflags`
	$(CC) -shared $(LIBS) -rdynamic -o php_libvirt.so php_libvirt.o -ldl -lvirt `pkg-config libxml-2.0 --libs`
	echo "Extension compiled as php_libvirt.so"

clean-temp:
	$(RM) -f *.o

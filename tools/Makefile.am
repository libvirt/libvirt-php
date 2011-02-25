EXTRA_DIST = generate-api-docs.c

all-am:
	$(CC) -Wall -o generate-api-docs generate-api-docs.c
	./generate-api-docs ../src/libvirt.c ../docs/api-reference.html.in

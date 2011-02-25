EXTRA_DIST = generate-api-docs.c

all-am:
	$(CC) -Wall -o generate-api-docs generate-api-docs.c
	./generate-api-docs ../src/libvirt.c ../docs/api-reference.html.in
	./generate-api-docs --private ../src/libvirt.c ../docs/dev-api-reference.html.in

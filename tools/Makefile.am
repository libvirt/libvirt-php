noinst_PROGRAMS=generate-api-docs

generate_api_docs_SOURCES=generate-api-docs.c

APIBUILD_STAMP=$(srcdir)/.stamp
EXTRA_DIST = $(APIBUILD_STAMP)

doc_generated_files = \
		$(top_builddir)/docs/api-reference.html.in \
		$(top_builddir)/docs/dev-api-reference.html.in
		$(NULL)

$(doc_generated_files): $(APIBUILD_STAMP)

API_FILES = \
	libvirt-php.c \
	libvirt-connection.c \
	libvirt-domain.c \
	libvirt-network.c \
	libvirt-node.c \
	libvirt-nodedev.c \
	libvirt-nwfilter.c \
	libvirt-snapshot.c \
	libvirt-storage.c \
	libvirt-stream.c

$(APIBUILD_STAMP): generate-api-docs
	./generate-api-docs -o ../docs/api-reference.html.in $(addprefix $(top_srcdir)/src/,$(API_FILES))
	./generate-api-docs --private -o ../docs/dev-api-reference.html.in $(addprefix $(top_srcdir)/src/,$(API_FILES))
	touch $@

CLEANFILES = $(doc_generated_files)

maintainer-clean-local:
	rm -rf $(APIBUILD_STAMP)

all-am: $(APIBUILD_STAMP)

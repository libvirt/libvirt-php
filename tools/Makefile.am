noinst_PROGRAMS=generate-api-docs

generate_api_docs_SOURCES=generate-api-docs.c

APIBUILD_STAMP=$(srcdir)/.stamp
EXTRA_DIST = $(APIBUILD_STAMP)

doc_generated_files = \
		$(top_builddir)/docs/api-reference.html.in \
		$(top_builddir)/docs/dev-api-reference.html.in
		$(NULL)

$(doc_generated_files): $(APIBUILD_STAMP)

$(APIBUILD_STAMP): generate-api-docs
	./generate-api-docs -o ../docs/api-reference.html.in $(top_srcdir)/src/libvirt-php.c
	./generate-api-docs --private -o ../docs/dev-api-reference.html.in $(top_srcdir)/src/libvirt-php.c
	touch $@

CLEANFILES = $(doc_generated_files)

maintainer-clean-local:
	rm -rf $(APIBUILD_STAMP)

all-am: $(APIBUILD_STAMP)

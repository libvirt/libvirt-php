## Process this file with automake to produce Makefile.in
PERL = perl

css =         \
  generic.css \
  libvirt.css \
  main.css

png = \
  graphics/32favicon.png \
  graphics/footer_corner.png \
  graphics/footer_pattern.png \
  graphics/libvirt-header-bg.png \
  graphics/libvirt-php-header-logo.png

dot_html_in = $(notdir $(wildcard $(srcdir)/*.html.in))
dot_html = $(dot_html_in:%.html.in=%.html)

EXTRA_DIST=					\
  site.xsl news.xsl page.xsl \
  $(dot_html) $(dot_html_in) $(png) $(css) \
  sitemap.html.in

MAINTAINERCLEANFILES = $(dot_html)

all-am: web

web: $(dot_html)

%.png: %.fig
	convert -rotate 90 $< $@

%.html.tmp: %.html.in site.xsl page.xsl sitemap.html.in
	@if [ -x $(XSLTPROC) ] ; then \
	  echo "Generating $@"; \
	  name=`echo $@ | sed -e 's/.tmp//'`; \
	  $(XSLTPROC) --stringparam pagename $$name --nonet --html \
	    $(top_srcdir)/docs/site.xsl $< > $@ \
	    || { rm $@ && exit 1; }; fi

%.html: %.html.tmp
	@if test -x $(XMLLINT) && test -x $(XMLCATALOG) ; then \
	  if $(XMLCATALOG) '$(XML_CATALOG_FILE)' \
	    "-//W3C//DTD XHTML 1.0 Strict//EN" > /dev/null ; then \
	    echo "Validating $@" ; \
	    SGML_CATALOG_FILES='$(XML_CATALOG_FILE)' \
	    $(XMLLINT) --catalogs --nonet --format --valid $< > $@ \
	    || { rm $@ && exit 1; }; \
      else cp $< $@; \
      fi ; \
    fi
# Hack it as build is failing on my F-17 x86_64 box

check-local: all

clean-local:
	rm -f *.html

maintainer-clean-local: clean-local

rebuild: all

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(HTML_DIR)
	for f in $(css) $(dot_html) $(png); do \
	  $(INSTALL) -m 0644 $(srcdir)/$$f $(DESTDIR)$(HTML_DIR); \
	done

uninstall-local:
	for f in $(css) $(dot_html) $(png); do \
	  rm -f $(DESTDIR)$(HTML_DIR)/$$(basename $$f); \
	done

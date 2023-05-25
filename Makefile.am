SUBDIRS = tools src docs tests

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = libvirt-php.spec libvirt-php.spec.in AUTHORS.in

AM_DISTCHECK_CONFIGURE_FLAGS = --with-distcheck --with-php=$(PHPEXECUTABLE) --with-php-config=$(PHPCONFIG)

rpm: clean
	@(unset CDPATH ; $(MAKE) dist && rpmbuild -ta $(distdir).tar.xz)

dist-hook: gen-AUTHORS

.PHONY: gen-AUTHORS
gen-AUTHORS:
	$(AM_V_GEN)if test -d $(srcdir)/.git; then \
	    out="`cd $(srcdir) && git log --pretty=format:'%aN <%aE>' | sort -u | sed 's/^/\t/'`" && \
	    perl -p -e "s/#authorslist#// and print '$$out'" \
	      < $(srcdir)/AUTHORS.in > $(distdir)/AUTHORS-tmp && \
	    mv -f $(distdir)/AUTHORS-tmp $(distdir)/AUTHORS ; \
	fi

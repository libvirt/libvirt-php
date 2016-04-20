SUBDIRS = tools src docs tests

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = libvirt-php.spec libvirt-php.spec.in winsrc

AM_DISTCHECK_CONFIGURE_FLAGS = --with-distcheck

rpm: clean
	@(unset CDPATH ; $(MAKE) dist && rpmbuild -ta $(distdir).tar.gz)

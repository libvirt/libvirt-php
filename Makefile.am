SUBDIRS = tools src docs tests

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = libvirt-php.spec libvirt-php.spec.in winsrc

rpm: clean
	@(unset CDPATH ; $(MAKE) dist && rpmbuild -ta $(distdir).tar.gz)

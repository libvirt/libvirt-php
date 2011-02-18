AC_INIT([libvirt-php], [0.4], [http://libvirt.org])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_FILES([Makefile src/Makefile docs/Makefile])
AC_CHECK_LIB([virt], [virConnectOpen], [], AC_MSG_ERROR([You need libvirt to compile this module]))
AM_INIT_AUTOMAKE([-Wno-portability])
AM_MAINTAINER_MODE([enable])

dnl Checks for programs.
AC_PROG_CC
AC_LANG([C])
AC_PROG_INSTALL
AC_PROG_SED
AC_CHECK_TOOL([STRIP], [strip])
AC_CHECK_TOOL([ECHO], [echo])
AC_CHECK_TOOL([RM], [rm])

dnl Check for libxml2
AM_PATH_XML2([2.0.0])

dnl Specific XML catalog file for validation of generated html
AC_ARG_WITH([xml-catalog-file],
            [AC_HELP_STRING([--with-xml-catalog-file=path],
                [path to XML catalog file for validating
                 generated html, default /etc/xml/catalog])],
            [XML_CATALOG_FILE=$withval],
            [XML_CATALOG_FILE='/etc/xml/catalog'])
AC_SUBST([XML_CATALOG_FILE])

# External programs to generate documentation
AC_PATH_PROG([XSLTPROC], [xsltproc], [/usr/bin/xsltproc])
AC_PATH_PROG([XMLLINT], [xmllint], [/usr/bin/xmllint])
AC_PATH_PROG([XMLCATALOG], [xmlcatalog], [/usr/bin/xmlcatalog])

dnl Specific dir for HTML output ?
AC_ARG_WITH([html-dir], [AC_HELP_STRING([--with-html-dir=path],
            [path to base html directory, default $datadir/doc/html])],
            [HTML_DIR=$withval], [HTML_DIR='$(datadir)/doc'])

AC_ARG_WITH([html-subdir], [AC_HELP_STRING([--with-html-subdir=path],
            [directory used under html-dir, default $PACKAGE-$VERSION/html])],
            [test "x$withval" != "x" && HTML_DIR="$HTML_DIR/$withval"],
            [HTML_DIR="$HTML_DIR/\$(PACKAGE)-\$(VERSION)/html"])
AC_SUBST([HTML_DIR])

AC_PATH_PROG([PHPIZE], [phpize], [no])
if test "x$PHPIZE" = "xno"; then
    AC_MSG_ERROR([phpize not found; please install the PHP SDK])
fi

AC_PATH_PROG([PHPCONFIG], [php-config], [no])
if test "x$PHPCONFIG" = "xno"; then
    AC_MSG_ERROR([php-config not found; please install the PHP SDK])
fi

AC_SUBST([PHPIZE])
AC_SUBST([PHPCONFIG])
AC_OUTPUT

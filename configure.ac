AC_INIT([libvirt-php], [0.4.7], [http://libvirt.org])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_FILES([Makefile tools/Makefile src/Makefile tests/Makefile docs/Makefile])
AM_INIT_AUTOMAKE([-Wno-portability])
AM_MAINTAINER_MODE([enable])

dnl Checks for programs.
AC_PROG_CC
AC_LANG([C])
AC_PROG_INSTALL
AC_PROG_SED
AC_PROG_AWK
AC_CHECK_TOOL([ECHO], [echo])
AC_CHECK_TOOL([RM], [rm])

dnl Need to test if pkg-config exists
PKG_PROG_PKG_CONFIG

LIBVIRT_REQUIRED=0.6.2
PKG_CHECK_MODULES(LIBVIRT, libvirt >= $LIBVIRT_REQUIRED)

dnl ==========================================================================
dnl required minimum version of libxml2
dnl replace "2.0.0" with an appropriate one
dnl ==========================================================================
LIBXML_REQUIRED="2.0.0"

dnl ==========================================================================
dnl find libxml2 library, borrowed from xmlsec
dnl ==========================================================================
LIBXML_CONFIG="xml2-config"
LIBXML_CFLAGS=""
LIBXML_LIBS=""
LIBXML_FOUND="no"

AC_ARG_WITH([libxml], AC_HELP_STRING([--with-libxml=@<:@PFX@:>@], [libxml2 location]))
if test "x$with_libxml" = "xno" ; then
    AC_MSG_CHECKING(for libxml2 libraries >= $LIBXML_REQUIRED)
    AC_MSG_ERROR([libxml2 >= $LIBXML_REQUIRED is required for libvirt])
elif test "x$with_libxml" = "x" && test "x$PKG_CONFIG" != "x" ; then
    PKG_CHECK_MODULES(LIBXML, libxml-2.0 >= $LIBXML_REQUIRED, [LIBXML_FOUND=yes], [LIBXML_FOUND=no])
fi
if test "$LIBXML_FOUND" = "no" ; then
    if test "x$with_libxml" != "x" ; then
	LIBXML_CONFIG=$with_libxml/bin/$LIBXML_CONFIG
    fi
    AC_MSG_CHECKING(libxml2 $LIBXML_CONFIG >= $LIBXML_REQUIRED )
    if ! $LIBXML_CONFIG --version > /dev/null 2>&1 ; then
	AC_MSG_ERROR([Could not find libxml2 anywhere (see config.log for details).])
    fi
    vers=`$LIBXML_CONFIG --version | awk -F. '{ printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
    minvers=`echo $LIBXML_REQUIRED | awk -F. '{ printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
    if test "$vers" -ge "$minvers" ; then
        LIBXML_LIBS="`$LIBXML_CONFIG --libs`"
        LIBXML_CFLAGS="`$LIBXML_CONFIG --cflags`"
	LIBXML_FOUND="yes"
        AC_MSG_RESULT(yes)
    else
        AC_MSG_ERROR(
          [You need at least libxml2 $LIBXML_REQUIRED for this version of libvirt])
    fi
fi

AC_SUBST([LIBXML_CFLAGS])
AC_SUBST([LIBXML_LIBS])

dnl Get the version information at compile-time
VERSION_MAJOR=`echo $VERSION | awk -F. '{print $1}'`
VERSION_MINOR=`echo $VERSION | awk -F. '{print $2}'`
VERSION_MICRO=`echo $VERSION | awk -F. '{print $3}'`

AC_DEFINE_UNQUOTED(VERSION_MAJOR, $VERSION_MAJOR, [Major version number of package])
AC_DEFINE_UNQUOTED(VERSION_MINOR, $VERSION_MINOR, [Minor version number of package])
AC_DEFINE_UNQUOTED(VERSION_MICRO, $VERSION_MICRO, [Micro version number of package])

dnl Specific XML catalog file for validation of generated html
AC_ARG_WITH([xml-catalog-file],
            [AC_HELP_STRING([--with-xml-catalog-file=path],
                [path to XML catalog file for validating
                 generated html, default /etc/xml/catalog])],
            [XML_CATALOG_FILE=$withval],
            [XML_CATALOG_FILE='/etc/xml/catalog'])
AC_SUBST([XML_CATALOG_FILE])

# External programs to generate documentation
AC_PATH_PROG([XSLTPROC], [xsltproc], [no])
if test "x$XSLTPROC" = "xno"; then
    AC_MSG_ERROR([xsltproc not found])
fi

AC_PATH_PROG([XMLLINT], [xmllint], [no])
if test "x$XMLLINT" = "xno"; then
    AC_MSG_ERROR([xmllint not found])
fi

AC_PATH_PROG([XMLCATALOG], [xmlcatalog], [no])
if test "x$XMLCATALOG" = "xno"; then
    AC_MSG_ERROR([xmlcatalog not found])
fi


dnl Specific dir for HTML output ?
AC_ARG_WITH([html-dir], [AC_HELP_STRING([--with-html-dir=path],
            [path to base html directory, default $datadir/doc/html])],
            [HTML_DIR=$withval], [HTML_DIR='$(datadir)/doc'])

AC_ARG_WITH([html-subdir], [AC_HELP_STRING([--with-html-subdir=path],
            [directory used under html-dir, default $PACKAGE-$VERSION/html])],
            [test "x$withval" != "x" && HTML_DIR="$HTML_DIR/$withval"],
            [HTML_DIR="$HTML_DIR/\$(PACKAGE)-\$(VERSION)/html"])
AC_SUBST([HTML_DIR])

AC_ARG_WITH([php-config], [AC_HELP_STRING([--with-php-config=path],
            [path to php-config, default search all paths])])

if test "x$with_php_config" != "x" ; then
    AC_MSG_CHECKING(for php-config)
    PHPCONFIG=$with_php_config
    AC_MSG_RESULT($with_php_config)
else
    AC_PATH_PROG([PHPCONFIG], [php-config], [no])
fi

if test "x$PHPCONFIG" = "xno"; then
    AC_MSG_ERROR([php-config not found; please install the PHP SDK])
fi

AC_PATH_PROG([PHPIZE], [phpize], [no], [`php-config --prefix`/bin$PATH_SEPARATOR$PATH])
if test "x$PHPIZE" = "xno"; then
    AC_MSG_ERROR([phpize not found; please install the PHP SDK])
fi

OS=`uname`
if test "$OS" = "Darwin"; then
    WL=-Wl,
    AC_SUBST([WL])
    SHLIB_FLAGS="\${WL}-flat_namespace \${WL}-undefined \${WL}suppress -bundle"
else
    SHLIB_FLAGS="-shared -rdynamic"
fi
AC_SUBST([SHLIB_FLAGS])

AC_SUBST([PHPIZE])
AC_SUBST([PHPCONFIG])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([$PACKAGE.spec])
AC_OUTPUT

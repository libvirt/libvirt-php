AC_INIT([php-libvirt], [0.3a], [r.hladik@cybersales.cz])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_PREFIX_DEFAULT([LIBDIR/php/modules])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_SRCDIR([libvirt.c])
AC_CHECK_LIB([virt], [virConnectOpen], [], AC_MSG_ERROR([You need libvirt to compile this module]))

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

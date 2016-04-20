dnl The libvirt-php.so config
dnl
dnl Copyright (C) 2016 Red Hat, Inc.
dnl
dnl This library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Lesser General Public
dnl License as published by the Free Software Foundation; either
dnl version 2.1 of the License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with this library.  If not, see
dnl <http://www.gnu.org/licenses/>.
dnl

AC_DEFUN([LIBVIRT_CHECK_PHP_CONFDIR],[
  AC_ARG_WITH([php-confdir],
    [AS_HELP_STRING([--with-php-confdir],
      [location of php extenstion config files])],
      [], [with_php_confdir=check])

  dnl Check for system location of php configs
  if test "x$with_php_confdir" != "xno" ; then
    if test "x$with_php_confdir" = "xcheck" ; then
      confdir="$($PHPCONFIG --configure-options | sed -n 's/.*--with-config-file-scan-dir=\(\S*\).*/\1/p')"
    elif test "x$with_php_confdir" = "xno" || test "x$with_php_confdir" = "xyes"; then
      AC_MSG_ERROR([php-confdir must be used only with valid path])
    else
      confdir=$with_php_confdir
    fi
  fi

  AC_SUBST([confdir])
])

AC_DEFUN([LIBVIRT_RESULT_PHP_CONFDIR],[
  AC_MSG_NOTICE([php-confdir: $confdir])
])

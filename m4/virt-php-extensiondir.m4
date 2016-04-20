dnl The libvirt-php.so php extension
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

AC_DEFUN([LIBVIRT_CHECK_PHP_EXTENSIONDIR],[
  AC_ARG_WITH([php-extensiondir],
    [AS_HELP_STRING([--with-php-extensiondir],
      [location of php extensions])],
      [], [with_php_extensiondir=check])

  dnl Check for system location of php extensions
  if test "x$with_php_extensiondir" != "xno" ; then
    if test "x$with_php_extensiondir" = "xcheck" ; then
      extensiondir="$($PHPCONFIG --extension-dir)"
    elif test "x$with_php_extensiondir" = "xno" || test "x$with_php_extensiondir" = "xyes"; then
      AC_MSG_ERROR([php-extensiondir must be used only with valid path])
    else
      extensiondir=$with_php_extensiondir
    fi
  fi

  if test "x$with_distcheck" == "xyes" ; then
     extensiondir=${prefix}${extensiondir}
  fi

  AC_SUBST([extensiondir])
])

AC_DEFUN([LIBVIRT_RESULT_PHP_EXTENSIONDIR],[
  AC_MSG_NOTICE([php-extensiondir: $extensiondir])
])

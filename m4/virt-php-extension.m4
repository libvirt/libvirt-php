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

dnl
dnl Check whether php module exists
dnl
dnl LIBVIRT_CHECK_PHP_EXTENSION([EXTENSION])
dnl
AC_DEFUN([LIBVIRT_CHECK_PHP_EXTENSION],[
  AC_MSG_CHECKING([for php module $1])

  phpbinary="$($PHPCONFIG --php-binary)"
  if test "x$phpbinary" = "x" || test ! -x "$phpbinary" ; then
    phpbinary="$($PHPCONFIG --prefix)/bin/php"
  fi

  if test ! -x "$phpbinary"; then
    AC_MSG_ERROR([php binary not found])
  fi

  AC_SUBST([phpbinary])

  module="$($phpbinary -m | grep $1)"

  if test "x$module" = "x"; then
    AC_MSG_ERROR([php module $1 not found])
  else
    AC_MSG_RESULT([found])
  fi
])

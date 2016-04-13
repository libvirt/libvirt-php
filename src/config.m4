PHP_ARG_WITH(libvirt, for libvirt support,
[  --with-libvirt             Include libvirt support])

if test "$PHP_LIBVIRT" != "no"; then

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

  LIBVIRT_REQUIRED=1.2.8

  if test -x "$PKG_CONFIG" && $PKG_CONFIG libvirt --exists ; then
    AC_MSG_CHECKING(libvirt version)
    if $PKG_CONFIG libvirt --atleast-version=$LIBVIRT_REQUIRED ; then
      LIBVIRT_INCLUDE=`$PKG_CONFIG libvirt --cflags`
      LIBVIRT_LIBRARY=`$PKG_CONFIG libvirt --libs`
      LIBVIRT_VERSION=`$PKG_CONFIG libvirt --modversion`
      AC_MSG_RESULT($LIBVIRT_VERSION)
    else
      AC_MSG_ERROR(version too old)
    fi
    PHP_EVAL_INCLINE($LIBVIRT_INCLUDE)
    PHP_EVAL_LIBLINE($LIBVIRT_LIBRARY, LIBVIRT_SHARED_LIBADD)

    AC_MSG_CHECKING(libvirt-qemu version)
    if $PKG_CONFIG libvirt-qemu --atleast-version=$LIBVIRT_REQUIRED ; then
      QEMU_INCLUDE=`$PKG_CONFIG libvirt-qemu --cflags`
      QEMU_LIBRARY=`$PKG_CONFIG libvirt-qemu --libs`
      QEMU_VERSION=`$PKG_CONFIG libvirt-qemu --modversion`
      AC_MSG_RESULT($QEMU_VERSION)
    else
      AC_MSG_ERROR(version too old)
    fi
    PHP_EVAL_INCLINE($QEMU_INCLUDE)
    PHP_EVAL_LIBLINE($QEMU_LIBRARY, LIBVIRT_SHARED_LIBADD)

    AC_MSG_CHECKING(libxml version)
    if $PKG_CONFIG libxml-2.0 --exists ; then
      LIBXML_INCLUDE=`$PKG_CONFIG libxml-2.0 --cflags`
      LIBXML_LIBRARY=`$PKG_CONFIG libxml-2.0 --libs`
      LIBXML_VERSION=`$PKG_CONFIG libxml-2.0 --modversion`
      AC_MSG_RESULT($LIBXML_VERSION)
    else
      AC_MSG_ERROR(version too old)
    fi
    PHP_EVAL_INCLINE($LIBXML_INCLUDE)
    PHP_EVAL_LIBLINE($LIBXML_LIBRARY, LIBVIRT_SHARED_LIBADD)

    PHP_SUBST(LIBVIRT_SHARED_LIBADD)
    PHP_NEW_EXTENSION(libvirt, libvirt-php.c sockets.c vncfunc.c, $ext_shared)
  else
    AC_MSG_ERROR(pkg-config not found)
  fi
fi

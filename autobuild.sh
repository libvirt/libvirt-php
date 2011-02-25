#!/bin/sh

set -e
set -v

make distclean || :

aclocal
autoreconf -i -f
#phpize

PHPEDIR=`php-config --extension-dir | sed -s "s,/usr,$AUTOBUILD_INSTALL_ROOT,"`
PHPCDIR=`php-config --configure-options |
         sed -n 's|.*--with-config-file-scan-dir=\([^ ]*\).*|\1|p' |
         sed -s "s,/etc,$AUTOBUILD_INSTALL_ROOT/etc,"`

./configure --prefix=$AUTOBUILD_INSTALL_ROOT

make
make install PHPEDIR=$PHPEDIR PHPCDIR=$PHPCDIR

rm -f *.tar.gz
make dist

if [ -n "$AUTOBUILD_COUNTER" ]; then
  EXTRA_RELEASE=".auto$AUTOBUILD_COUNTER"
else
  NOW=`date +"%s"`
  EXTRA_RELEASE=".$USER$NOW"
fi

if [ -x /usr/bin/rpmbuild ]
then
  rpmbuild --nodeps \
     --define "extra_release $EXTRA_RELEASE" \
     --define "_sourcedir `pwd`" \
     -ba --clean libvirt-php.spec
fi

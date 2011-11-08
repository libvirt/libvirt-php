#!/bin/bash

AUTHOR_MAIL=minovotn@redhat.com
MAIL_SUBJECT="[libvirt-php bug report] libvirt-php check error"

./runtests.sh
if [ "x$?" == "x0" ]; then
	exit 0
fi

read -p "Some (or all) of the tests have failed. Please enter your e-mail address to submit to the author (or Enter for exit): " mail
if [ "x$mail" == "x" ]; then
	exit 1
fi

system=$(lsb_release -d | awk '/Description:/ { split($0, a, ":"); x=a[2]; gsub(/[[:space:]] */," ",x); print x }')
phpver=$(php -v | head -n 1)
phpvernum=$(php -v | head -n 1 | awk '/PHP / { split($0, a, " "); print a[2]; }')
arch=$(uname -m)
echo "Appending system information: $system on $arch"
echo "Appending PHP version: $phpver"
echo "Generating report. This may take a while ..."

TMPFILE=$(mktemp)
echo "E-mail address: $mail" > $TMPFILE
echo "System: $system on $arch" >> $TMPFILE
echo "System PHP: $phpver" >> $TMPFILE
echo "Results:" >> $TMPFILE
echo >> $TMPFILE
./runtests.sh 1 >> $TMPFILE
cat $TMPFILE | mail -s "$MAIL_SUBJECT - PHP $phpvernum on $arch" "$AUTHOR_MAIL"; ret=$?

if [ "x$ret" == "x1" ]; then
	echo "Cannot send e-mail. Please send file $TMPFILE manually to $AUTHOR_MAIL"
	exit 1
fi

rm -f $TMPFILE
echo "E-mail has been sent to author's e-mail ($AUTHOR_MAIL)"

exit 1

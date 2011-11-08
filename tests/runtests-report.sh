#!/bin/bash

AUTHOR_MAIL=minovotn@redhat.com

./runtests.sh
if [ "x$?" == "x0" ]; then
	exit 0
fi

read -p "Some of the tests have failed. Please enter your e-mail address to submit to the author (or Enter for exit): " mail
if [ "x$mail" == "x" ]; then
	exit 1
fi

system="unknown"
if [ -f "/etc/redhat-release" ]; then
	system=$(cat /etc/redhat-release)
fi

phpver=$(php -v | head -n 1)
echo "Appending system information: $system"
echo "Appending PHP version: $phpver"
echo "Generating report. This may take a while ..."

TMPFILE=$(mktemp)
echo "E-mail address: $mail on $system" > $TMPFILE
echo "System PHP: $phpver" >> $TMPFILE
echo "Results:" >> $TMPFILE
echo >> $TMPFILE
./runtests.sh 1 >> $TMPFILE
cat $TMPFILE | mail -s "libvirt-php check error" $AUTHOR_MAIL
rm -f $TMPFILE

echo "E-mail has been sent to author's e-mail ($AUTHOR_MAIL)"

exit 1

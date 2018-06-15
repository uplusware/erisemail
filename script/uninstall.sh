#!/bin/bash
#
#	Copyright (c) openheap, uplusware
#	uplusware@gmail.com
#
if [ `id -u` -ne 0 ]; then
        echo "You need root privileges to run this script"
        exit 1
fi

#
# uninstall the eRisemail System
#

echo "Sure to remove eRisemail from you computer? [yes/no]"
read uack
if [ ${uack} = "yes" ]
then
	echo "Remove the erisemail...."
else
	exit 1
fi

/etc/init.d/erisemail stop

test -x /var/erisemail/html && rm -rf /var/erisemail/html

echo "Sure to remove database? All the mails&users data will be deleted and non-recoverable.[yes/no]"
read uack
if [ ${uack} = "yes" ]
then
	echo "Remove the erisemail's database...."
	/usr/bin/eriseutil --uninstall
	test -x /var/erisemail/private/ && rm -rf /var/erisemail/private/
fi

test -x /usr/bin/erisemaild && rm -rf /usr/bin/erisemaild
test -x /usr/bin/eriseutil && rm -rf /usr/bin/eriseutil

test -x /usr/lib/liberisestorage.so && rm -rf /usr/lib/liberisestorage.so
test -x /usr/lib/liberiseantispam.so && rm -rf /usr/lib/liberiseantispam.so
test -x /usr/lib/libldapasn1.so && rm -rf /usr/lib/libldapasn1.so

test -x /usr/lib64/liberisestorage.so && rm -rf /usr/lib64/liberisestorage.so
test -x /usr/lib64/liberiseantispam.so && rm -rf /usr/lib64/liberiseantispam.so
test -x /usr/lib64/libldapasn1.so && rm -rf /usr/lib64/libldapasn1.so

test -x /etc/init.d/erisemail && rm -rf /etc/init.d/erisemail
test -x /etc/erisemail && rm -rf /etc/erisemail

test -x /etc/rc0.d/K60erisemail && rm -f /etc/rc0.d/K60erisemail
test -x /etc/rc1.d/S60erisemail && rm -f /etc/rc1.d/S60erisemail
test -x /etc/rc2.d/S60erisemail && rm -f /etc/rc2.d/S60erisemail
test -x /etc/rc3.d/S60erisemail && rm -f /etc/rc3.d/S60erisemail
test -x /etc/rc4.d/S60erisemail && rm -f /etc/rc4.d/S60erisemail
test -x /etc/rc5.d/S60erisemail && rm -f /etc/rc5.d/S60erisemail
test -x /etc/rc6.d/K60erisemail && rm -f /etc/rc6.d/K60erisemail

echo "Done"

#!/bin/bash

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

rm -rf /var/erisemail/html

echo "Sure to remove database? All the mails&users data will be deleted and non-recoverable.[yes/no]"
read uack
if [ ${uack} = "yes" ]
then
	echo "Remove the erisemail's database...."
	/usr/bin/eriseutil --uninstall
	rm -rf /var/erisemail/private/
fi

rm -rf /usr/bin/erisemaild
rm -rf /usr/bin/eriseutil
rm -rf /usr/lib/liberisestorage.so
rm -rf /usr/lib/liberiseantijunk.so
rm -rf /usr/bin/eriseaddgroup
rm -rf /usr/bin/eriseadduser
rm -rf /usr/bin/erisedeluser
rm -rf /usr/bin/erisepasswd
rm -rf /etc/init.d/erisemail
rm -rf /etc/erisemail

rm -f /etc/rc0.d/K60erisemail
rm -f /etc/rc1.d/S60erisemail
rm -f /etc/rc2.d/S60erisemail
rm -f /etc/rc3.d/S60erisemail
rm -f /etc/rc4.d/S60erisemail
rm -f /etc/rc5.d/S60erisemail
rm -f /etc/rc6.d/K60erisemail

echo "Done"

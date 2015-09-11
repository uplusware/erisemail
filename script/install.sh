#!/bin/bash

if [ `id -u` -ne 0 ]; then
        echo "You need root privileges to run this script"
        exit 1
fi

#
# install the eRisemail System
#
test -x /etc/init.d/erisemail && /etc/init.d/erisemail stop
killall erisemail 2> /dev/null
sleep 1

path=$(dirname $0)
oldpwd=$(pwd)
cd ${path}
path=$(pwd)

echo "Copy the files to your system."

test -x /etc/erisemail || mkdir /etc/erisemail
test -x /var/erisemail || mkdir /var/erisemail
test -x /var/erisemail/html || mkdir /var/erisemail/html
test -x /var/erisemail/private || mkdir /var/erisemail/private
test -x /var/erisemail/private/eml || mkdir /var/erisemail/private/eml
test -x /var/erisemail/private/tmp || mkdir /var/erisemail/private/tmp
test -x /var/erisemail/private/cal || mkdir /var/erisemail/private/cal
test -x /var/erisemail/private/box || mkdir /var/erisemail/private/box

cp -rf ${path}/html/* /var/erisemail/html/ 

cp -f ${path}/erisemail.conf /etc/erisemail/erisemail.conf
chmod 600 /etc/erisemail/erisemail.conf

cp -f ${path}/erise-root.crt /etc/erisemail/erise-root.crt
chmod 600 /etc/erisemail/erise-root.crt

cp -f ${path}/erise-server.key /etc/erisemail/erise-server.key
chmod 600 /etc/erisemail/erise-server.key

cp -f ${path}/erise-server.crt /etc/erisemail/erise-server.crt
chmod 600 /etc/erisemail/erise-server.crt

cp -f ${path}/erise-server.key /etc/erisemail/erise-client.key
chmod 600 /etc/erisemail/erise-client.key
 
cp -f ${path}/erise-server.crt /etc/erisemail/erise-client.crt
chmod 600 /etc/erisemail/erise-client.crt

cp -f ${path}/domain.list /etc/erisemail/domain.list
chmod a-x /etc/erisemail/domain.list
cp -f ${path}/permit.list /etc/erisemail/permit.list
chmod a-x /etc/erisemail/permit.list
cp -f ${path}/reject.list /etc/erisemail/reject.list
chmod a-x /etc/erisemail/reject.list
cp -f ${path}/webadmin.list /etc/erisemail/webadmin.list
chmod a-x /etc/erisemail/webadmin.list
cp -f ${path}/mfilter.xml /etc/erisemail/mfilter.xml
chmod a-x /etc/erisemail/mfilter.xml

if uname -o | grep -i cygwin;
then
  cp -f ${path}/liberisestorage.so /usr/bin/liberisestorage.so
  cp -f ${path}/liberiseantijunk.so /usr/bin/liberiseantijunk.so
else
  cp -f ${path}/liberisestorage.so /usr/lib64/liberisestorage.so
  cp -f ${path}/liberiseantijunk.so /usr/lib64/liberiseantijunk.so
fi
cp -f ${path}/erisemaild /usr/bin/erisemaild
chmod a+x /usr/bin/erisemaild

cp -f ${path}/eriseaddgroup /usr/bin/eriseaddgroup
chmod a+x /usr/bin/eriseaddgroup

cp -f ${path}/erisedelgroup /usr/bin/erisedelgroup
chmod a+x /usr/bin/erisedelgroup

cp -f ${path}/eriseadduser /usr/bin/eriseadduser
chmod a+x /usr/bin/eriseadduser

cp -f ${path}/erisedeluser /usr/bin/erisedeluser
chmod a+x /usr/bin/erisedeluser

cp -f ${path}/eriseaddusertogroup /usr/bin/eriseaddusertogroup
chmod a+x /usr/bin/eriseaddusertogroup

cp -f ${path}/erisedeluserfromgroup /usr/bin/erisedeluserfromgroup
chmod a+x /usr/bin/erisedeluserfromgroup

cp -f ${path}/eriseutil /usr/bin/eriseutil
chmod a+x /usr/bin/eriseutil

cp -f ${path}/erisepasswd /usr/bin/erisepasswd
chmod a+x /usr/bin/erisepasswd

cp -f ${path}/eriseenableuser /usr/bin/eriseenableuser
chmod a+x /usr/bin/eriseenableuser

cp -f ${path}/erisedisableuser /usr/bin/erisedisableuser
chmod a+x /usr/bin/erisedisableuser

cp -f ${path}/erisemail.sh  /etc/init.d/erisemail
chmod a+x /etc/init.d/erisemail

cp -f ${path}/uninstall.sh  /var/erisemail/uninstall.sh
chmod a-x /var/erisemail/uninstall.sh

ln -s /etc/init.d/erisemail /etc/rc0.d/K60erisemail 2> /dev/null
ln -s /etc/init.d/erisemail /etc/rc1.d/S60erisemail 2> /dev/null
ln -s /etc/init.d/erisemail /etc/rc2.d/S60erisemail 2> /dev/null
ln -s /etc/init.d/erisemail /etc/rc3.d/S60erisemail 2> /dev/null
ln -s /etc/init.d/erisemail /etc/rc4.d/S60erisemail 2> /dev/null
ln -s /etc/init.d/erisemail /etc/rc5.d/S60erisemail 2> /dev/null
ln -s /etc/init.d/erisemail /etc/rc6.d/K60erisemail 2> /dev/null

echo "Done."
echo "Please reference the document named INSTALL to go ahead."
cd ${oldpwd}

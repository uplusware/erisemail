#!/bin/bash

if [ $# = 3 ]
then
	echo $2
else
	echo "mkdeb 1.5.02 utf8/gb2312 ubuntu"
	exit 1	
fi

path=$(dirname $0)
oldpwd=$(pwd)
cd ${path}
path=$(pwd)

cd ${path}

#############################################################################
# Platform
m=`uname -m`
if uname -o | grep -i linux;
then
	o=linux
	cd src/
	make clean
	make
	cd ..
elif uname -o | grep -i solaris;
then
	o=solaris
	cd src/
	gmake clean
	gmake SOLARIS=1
	cd ..
elif uname -o | grep -i freebsd;
then
	o=freebsd
	cd src/
	make clean
	make FREEBSD=1
	cd ..
fi

##############################################################################
# Simplfied Chinese
rm -rf $3-erisemail-bin-cn-$2-${m}
mkdir $3-erisemail-bin-cn-$2-${m}

mkdir $3-erisemail-bin-cn-$2-${m}/DEBIAN
cp DEBIAN/control-${m} $3-erisemail-bin-cn-$2-${m}/DEBIAN/control

mkdir $3-erisemail-bin-cn-$2-${m}/usr

mkdir $3-erisemail-bin-cn-$2-${m}/var
mkdir $3-erisemail-bin-cn-$2-${m}/var/erisemail
mkdir $3-erisemail-bin-cn-$2-${m}/var/erisemail/html
mkdir $3-erisemail-bin-cn-$2-${m}/var/erisemail/private
mkdir $3-erisemail-bin-cn-$2-${m}/var/erisemail/private/eml
mkdir $3-erisemail-bin-cn-$2-${m}/var/erisemail/private/tmp

mkdir $3-erisemail-bin-cn-$2-${m}/usr/bin
mkdir $3-erisemail-bin-cn-$2-${m}/usr/lib

mkdir $3-erisemail-bin-cn-$2-${m}/etc
mkdir $3-erisemail-bin-cn-$2-${m}/etc/erisemail/
mkdir $3-erisemail-bin-cn-$2-${m}/etc/init.d/
mkdir $3-erisemail-bin-cn-$2-${m}/etc/rc0.d/
mkdir $3-erisemail-bin-cn-$2-${m}/etc/rc1.d/
mkdir $3-erisemail-bin-cn-$2-${m}/etc/rc2.d/
mkdir $3-erisemail-bin-cn-$2-${m}/etc/rc3.d/
mkdir $3-erisemail-bin-cn-$2-${m}/etc/rc4.d/
mkdir $3-erisemail-bin-cn-$2-${m}/etc/rc5.d/
mkdir $3-erisemail-bin-cn-$2-${m}/etc/rc6.d/

cp cn-html-$2/*.js $3-erisemail-bin-cn-$2-${m}/var/erisemail/html
cp cn-html-$2/*.jpg $3-erisemail-bin-cn-$2-${m}/var/erisemail/html
cp cn-html-$2/*.gif $3-erisemail-bin-cn-$2-${m}/var/erisemail/html
cp cn-html-$2/*.ico $3-erisemail-bin-cn-$2-${m}/var/erisemail/html
cp cn-html-$2/*.html $3-erisemail-bin-cn-$2-${m}/var/erisemail/html
cp cn-html-$2/*.css $3-erisemail-bin-cn-$2-${m}/var/erisemail/html
cp cn-html-$2/*.xml $3-erisemail-bin-cn-$2-${m}/var/erisemail/html
chmod 400 $3-erisemail-bin-cn-$2-${m}/var/erisemail/html/*

cp src/erisemaild $3-erisemail-bin-cn-$2-${m}/usr/bin/erisemaild
cp src/eriseutil $3-erisemail-bin-cn-$2-${m}/usr/bin/eriseutil
cp script/eriseaddgroup $3-erisemail-bin-cn-$2-${m}/usr/bin/eriseaddgroup
cp script/eriseadduser $3-erisemail-bin-cn-$2-${m}/usr/bin/eriseadduser
cp script/erisedelgroup $3-erisemail-bin-cn-$2-${m}/usr/bin/erisedelgroup
cp script/erisedeluser $3-erisemail-bin-cn-$2-${m}/usr/bin/erisedeluser
cp script/eriseaddusertogroup $3-erisemail-bin-cn-$2-${m}/usr/bin/eriseaddusertogroup
cp script/erisedeluserfromgroup $3-erisemail-bin-cn-$2-${m}/usr/bin/erisedeluserfromgroup
cp script/erisepasswd $3-erisemail-bin-cn-$2-${m}/usr/bin/erisepasswd
cp script/eriseenableuser $3-erisemail-bin-cn-$2-${m}/usr/bin/eriseenableuser
cp script/erisedisableuser $3-erisemail-bin-cn-$2-${m}/usr/bin/erisedisableuser

chmod 700 $3-erisemail-bin-cn-$2-${m}/usr/bin/erisemaild
chmod 700 $3-erisemail-bin-cn-$2-${m}/usr/bin/eriseutil
chmod 700 $3-erisemail-bin-cn-$2-${m}/usr/bin/eriseaddgroup
chmod 700 $3-erisemail-bin-cn-$2-${m}/usr/bin/eriseadduser
chmod 700 $3-erisemail-bin-cn-$2-${m}/usr/bin/erisedelgroup
chmod 700 $3-erisemail-bin-cn-$2-${m}/usr/bin/erisedeluser
chmod 700 $3-erisemail-bin-cn-$2-${m}/usr/bin/eriseaddusertogroup
chmod 700 $3-erisemail-bin-cn-$2-${m}/usr/bin/erisedeluserfromgroup
chmod 700 $3-erisemail-bin-cn-$2-${m}/usr/bin/erisepasswd
chmod 700 $3-erisemail-bin-cn-$2-${m}/usr/bin/eriseenableuser
chmod 700 $3-erisemail-bin-cn-$2-${m}/usr/bin/erisedisableuser


cp src/liberisestorage.so $3-erisemail-bin-cn-$2-${m}/usr/lib/liberisestorage.so
cp src/liberiseantijunk.so $3-erisemail-bin-cn-$2-${m}/usr/lib/liberiseantijunk.so
chmod 644 $3-erisemail-bin-cn-$2-${m}/usr/lib/liberisestorage.so
chmod 644 $3-erisemail-bin-cn-$2-${m}/usr/lib/liberiseantijunk.so

cp script/erisemail.sh $3-erisemail-bin-cn-$2-${m}/etc/init.d/erisemail
chmod 700 $3-erisemail-bin-cn-$2-${m}/etc/init.d/erisemail

cp script/erisemail.sh $3-erisemail-bin-cn-$2-${m}/etc/rc0.d/K60erisemail
chmod 700 $3-erisemail-bin-cn-$2-${m}/etc/rc0.d/K60erisemail
cp script/erisemail.sh $3-erisemail-bin-cn-$2-${m}/etc/rc1.d/S60erisemail
chmod 700 $3-erisemail-bin-cn-$2-${m}/etc/rc1.d/S60erisemail
cp script/erisemail.sh $3-erisemail-bin-cn-$2-${m}/etc/rc2.d/S60erisemail
chmod 700 $3-erisemail-bin-cn-$2-${m}/etc/rc2.d/S60erisemail
cp script/erisemail.sh $3-erisemail-bin-cn-$2-${m}/etc/rc3.d/S60erisemail
chmod 700 $3-erisemail-bin-cn-$2-${m}/etc/rc3.d/S60erisemail
cp script/erisemail.sh $3-erisemail-bin-cn-$2-${m}/etc/rc4.d/S60erisemail
chmod 700 $3-erisemail-bin-cn-$2-${m}/etc/rc4.d/S60erisemail
cp script/erisemail.sh $3-erisemail-bin-cn-$2-${m}/etc/rc5.d/S60erisemail
chmod 700 $3-erisemail-bin-cn-$2-${m}/etc/rc5.d/S60erisemail
cp script/erisemail.sh $3-erisemail-bin-cn-$2-${m}/etc/rc6.d/K60erisemail
chmod 700 $3-erisemail-bin-cn-$2-${m}/etc/rc6.d/K60erisemail

cp script/erisemail-$2.conf $3-erisemail-bin-cn-$2-${m}/etc/erisemail/erisemail.conf
cp script/permit.list $3-erisemail-bin-cn-$2-${m}/etc/erisemail/
cp script/reject.list $3-erisemail-bin-cn-$2-${m}/etc/erisemail/
cp script/mfilter.xml $3-erisemail-bin-cn-$2-${m}/etc/erisemail/
cp script/domain.list $3-erisemail-bin-cn-$2-${m}/etc/erisemail/
cp script/webadmin.list $3-erisemail-bin-cn-$2-${m}/etc/erisemail/
cp ca/erise-root.crt $3-erisemail-bin-cn-$2-${m}/etc/erisemail/erise-root.crt
cp ca/erise-server.crt $3-erisemail-bin-cn-$2-${m}/etc/erisemail/erise-server.crt
cp ca/erise-server.key $3-erisemail-bin-cn-$2-${m}/etc/erisemail/erise-server.key
cp ca/erise-client.crt $3-erisemail-bin-cn-$2-${m}/etc/erisemail/erise-client.crt
cp ca/erise-client.key $3-erisemail-bin-cn-$2-${m}/etc/erisemail/erise-client.key
chmod 600 $3-erisemail-bin-cn-$2-${m}/etc/erisemail/*.conf
chmod 600 $3-erisemail-bin-cn-$2-${m}/etc/erisemail/*.list
chmod 600 $3-erisemail-bin-cn-$2-${m}/etc/erisemail/*.xml
chmod 400 $3-erisemail-bin-cn-$2-${m}/etc/erisemail/*.crt
chmod 400 $3-erisemail-bin-cn-$2-${m}/etc/erisemail/*.key

cp script/README-ZH.html $3-erisemail-bin-cn-$2-${m}/var/erisemail/README.html

dpkg -b $3-erisemail-bin-cn-$2-${m} $3-erisemail-bin-cn-$2-${m}-$1.deb

##############################################################################
# English
rm -rf $3-erisemail-bin-en-$2-${m}
mkdir $3-erisemail-bin-en-$2-${m}

mkdir $3-erisemail-bin-en-$2-${m}/DEBIAN
cp DEBIAN/control-${m} $3-erisemail-bin-en-$2-${m}/DEBIAN/control

mkdir $3-erisemail-bin-en-$2-${m}/usr

mkdir $3-erisemail-bin-en-$2-${m}/var
mkdir $3-erisemail-bin-en-$2-${m}/var/erisemail
mkdir $3-erisemail-bin-en-$2-${m}/var/erisemail/html
mkdir $3-erisemail-bin-en-$2-${m}/var/erisemail/private
mkdir $3-erisemail-bin-en-$2-${m}/var/erisemail/private/eml
mkdir $3-erisemail-bin-en-$2-${m}/var/erisemail/private/tmp

mkdir $3-erisemail-bin-en-$2-${m}/usr/bin
mkdir $3-erisemail-bin-en-$2-${m}/usr/lib

mkdir $3-erisemail-bin-en-$2-${m}/etc
mkdir $3-erisemail-bin-en-$2-${m}/etc/erisemail/
mkdir $3-erisemail-bin-en-$2-${m}/etc/init.d/
mkdir $3-erisemail-bin-en-$2-${m}/etc/rc0.d/
mkdir $3-erisemail-bin-en-$2-${m}/etc/rc1.d/
mkdir $3-erisemail-bin-en-$2-${m}/etc/rc2.d/
mkdir $3-erisemail-bin-en-$2-${m}/etc/rc3.d/
mkdir $3-erisemail-bin-en-$2-${m}/etc/rc4.d/
mkdir $3-erisemail-bin-en-$2-${m}/etc/rc5.d/
mkdir $3-erisemail-bin-en-$2-${m}/etc/rc6.d/

cp en-html-$2/*.js $3-erisemail-bin-en-$2-${m}/var/erisemail/html
cp en-html-$2/*.jpg $3-erisemail-bin-en-$2-${m}/var/erisemail/html
cp en-html-$2/*.gif $3-erisemail-bin-en-$2-${m}/var/erisemail/html
cp en-html-$2/*.ico $3-erisemail-bin-en-$2-${m}/var/erisemail/html
cp en-html-$2/*.html $3-erisemail-bin-en-$2-${m}/var/erisemail/html
cp en-html-$2/*.css $3-erisemail-bin-en-$2-${m}/var/erisemail/html
cp en-html-$2/*.xml $3-erisemail-bin-en-$2-${m}/var/erisemail/html
chmod 400 $3-erisemail-bin-en-$2-${m}/var/erisemail/html/*

cp src/erisemaild $3-erisemail-bin-en-$2-${m}/usr/bin/erisemaild
cp src/eriseutil $3-erisemail-bin-en-$2-${m}/usr/bin/eriseutil
cp script/eriseaddgroup $3-erisemail-bin-en-$2-${m}/usr/bin/eriseaddgroup
cp script/eriseadduser $3-erisemail-bin-en-$2-${m}/usr/bin/eriseadduser
cp script/erisedelgroup $3-erisemail-bin-en-$2-${m}/usr/bin/erisedelgroup
cp script/erisedeluser $3-erisemail-bin-en-$2-${m}/usr/bin/erisedeluser
cp script/eriseaddusertogroup $3-erisemail-bin-en-$2-${m}/usr/bin/eriseaddusertogroup
cp script/erisedeluserfromgroup $3-erisemail-bin-en-$2-${m}/usr/bin/erisedeluserfromgroup
cp script/erisepasswd $3-erisemail-bin-en-$2-${m}/usr/bin/erisepasswd
cp script/eriseenableuser $3-erisemail-bin-en-$2-${m}/usr/bin/eriseenableuser
cp script/erisedisableuser $3-erisemail-bin-en-$2-${m}/usr/bin/erisedisableuser

chmod 700 $3-erisemail-bin-en-$2-${m}/usr/bin/erisemaild
chmod 700 $3-erisemail-bin-en-$2-${m}/usr/bin/eriseutil
chmod 700 $3-erisemail-bin-en-$2-${m}/usr/bin/eriseaddgroup
chmod 700 $3-erisemail-bin-en-$2-${m}/usr/bin/eriseadduser
chmod 700 $3-erisemail-bin-en-$2-${m}/usr/bin/erisedelgroup
chmod 700 $3-erisemail-bin-en-$2-${m}/usr/bin/erisedeluser
chmod 700 $3-erisemail-bin-en-$2-${m}/usr/bin/eriseaddusertogroup
chmod 700 $3-erisemail-bin-en-$2-${m}/usr/bin/erisedeluserfromgroup
chmod 700 $3-erisemail-bin-en-$2-${m}/usr/bin/erisepasswd
chmod 700 $3-erisemail-bin-en-$2-${m}/usr/bin/eriseenableuser
chmod 700 $3-erisemail-bin-en-$2-${m}/usr/bin/erisedisableuser


cp src/liberisestorage.so $3-erisemail-bin-en-$2-${m}/usr/lib/liberisestorage.so
cp src/liberiseantijunk.so $3-erisemail-bin-en-$2-${m}/usr/lib/liberiseantijunk.so
chmod 644 $3-erisemail-bin-en-$2-${m}/usr/lib/liberisestorage.so
chmod 644 $3-erisemail-bin-en-$2-${m}/usr/lib/liberiseantijunk.so

cp script/erisemail.sh $3-erisemail-bin-en-$2-${m}/etc/init.d/erisemail
chmod 700 $3-erisemail-bin-en-$2-${m}/etc/init.d/erisemail

cp script/erisemail.sh $3-erisemail-bin-en-$2-${m}/etc/rc0.d/K60erisemail
chmod 700 $3-erisemail-bin-en-$2-${m}/etc/rc0.d/K60erisemail
cp script/erisemail.sh $3-erisemail-bin-en-$2-${m}/etc/rc1.d/S60erisemail
chmod 700 $3-erisemail-bin-en-$2-${m}/etc/rc1.d/S60erisemail
cp script/erisemail.sh $3-erisemail-bin-en-$2-${m}/etc/rc2.d/S60erisemail
chmod 700 $3-erisemail-bin-en-$2-${m}/etc/rc2.d/S60erisemail
cp script/erisemail.sh $3-erisemail-bin-en-$2-${m}/etc/rc3.d/S60erisemail
chmod 700 $3-erisemail-bin-en-$2-${m}/etc/rc3.d/S60erisemail
cp script/erisemail.sh $3-erisemail-bin-en-$2-${m}/etc/rc4.d/S60erisemail
chmod 700 $3-erisemail-bin-en-$2-${m}/etc/rc4.d/S60erisemail
cp script/erisemail.sh $3-erisemail-bin-en-$2-${m}/etc/rc5.d/S60erisemail
chmod 700 $3-erisemail-bin-en-$2-${m}/etc/rc5.d/S60erisemail
cp script/erisemail.sh $3-erisemail-bin-en-$2-${m}/etc/rc6.d/K60erisemail
chmod 700 $3-erisemail-bin-en-$2-${m}/etc/rc6.d/K60erisemail

cp script/erisemail-$2.conf $3-erisemail-bin-en-$2-${m}/etc/erisemail/erisemail.conf
cp script/permit.list $3-erisemail-bin-en-$2-${m}/etc/erisemail/
cp script/reject.list $3-erisemail-bin-en-$2-${m}/etc/erisemail/
cp script/mfilter.xml $3-erisemail-bin-en-$2-${m}/etc/erisemail/
cp script/domain.list $3-erisemail-bin-en-$2-${m}/etc/erisemail/
cp script/webadmin.list $3-erisemail-bin-en-$2-${m}/etc/erisemail/
cp ca/erise-root.crt $3-erisemail-bin-en-$2-${m}/etc/erisemail/erise-root.crt
cp ca/erise-server.crt $3-erisemail-bin-en-$2-${m}/etc/erisemail/erise-server.crt
cp ca/erise-server.key $3-erisemail-bin-en-$2-${m}/etc/erisemail/erise-server.key
cp ca/erise-client.crt $3-erisemail-bin-en-$2-${m}/etc/erisemail/erise-client.crt
cp ca/erise-client.key $3-erisemail-bin-en-$2-${m}/etc/erisemail/erise-client.key
chmod 600 $3-erisemail-bin-en-$2-${m}/etc/erisemail/*.conf
chmod 600 $3-erisemail-bin-en-$2-${m}/etc/erisemail/*.list
chmod 600 $3-erisemail-bin-en-$2-${m}/etc/erisemail/*.xml
chmod 400 $3-erisemail-bin-en-$2-${m}/etc/erisemail/*.crt
chmod 400 $3-erisemail-bin-en-$2-${m}/etc/erisemail/*.key

cp script/README-EN.html $3-erisemail-bin-en-$2-${m}/var/erisemail/README.html

dpkg -b $3-erisemail-bin-en-$2-${m} $3-erisemail-bin-en-$2-${m}-$1.deb


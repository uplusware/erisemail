#!/bin/bash

#
# eRisemail control script
#

test -x /usr/bin/erisemaild || exit 0

SELF=$(cd $(dirname $0); pwd -P)/$(basename $0)

cd /
umask 0

erisemail_start()
{
	/usr/bin/erisemaild start
}

erisemail_stop()
{
	/usr/bin/erisemaild stop
	sleep 3
	killall erisemaild 2> /dev/null
}

erisemail_status()
{
	/usr/bin/erisemaild status
}

erisemail_reload()
{
	/usr/bin/erisemaild reload
}

erisemail_version()
{
	/usr/bin/erisemaild version
}

erisemail_restart()
{
	/usr/bin/erisemaild stop
	sleep 3
	killall erisemaild 2> /dev/null
	sleep 1
	/usr/bin/erisemaild start
}

case "${1:-''}" in
	'start')
	erisemail_start
	;;
	
	'stop')
	erisemail_stop
	;;
	
	'restart')
	erisemail_restart
	;;
	
	'reload')
	erisemail_reload
	;;
	
	'status')
	erisemail_status
	;;
	
	'version')
	erisemail_version
	;;
	
	*)
	echo "Usage: $SELF Usage:erisemaild start | stop | status | reload | version [CONFIG FILE]"
	exit 1
	;;
esac


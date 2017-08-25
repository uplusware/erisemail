#!/bin/bash
_PASSWORD_=123456
_COUNTRY_="CN"
_ST_="BJ"
_ORG_="Uplusware"
_ORG_UNIT_="erisemail"

test -x demoCA || mkdir demoCA
test -x demoCA/newcerts || mkdir demoCA/newcerts

test -x demoCA/index.txt || touch demoCA/index.txt
test -x demoCA/serial || ( touch demoCA/serial && echo 01 > demoCA/serial )
test -x private || mkdir private
test -x certs || mkdir certs

#CA
_COMMON_NAME_="root"
openssl req -new -x509 -keyout private/ca.key -passout pass:${_PASSWORD_} -out certs/ca.crt -days 3650 -subj "/C="${_COUNTRY_}"/ST="${_ST_}"/L=/O="${_ORG_}"/OU="${_ORG_UNIT_}"/CN="${_COMMON_NAME_} -config openssl.cnf
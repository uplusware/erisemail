#!/bin/bash
_PASSWORD_=123456
_COUNTRY_="CN"
_ST_="BJ"
_ORG_="Uplusware"
_ORG_UNIT_="erisemail"

#server
_COMMON_NAME_="localhost"
openssl genrsa -des3 -passout pass:${_PASSWORD_} -out private/server.key 1024 -config openssl.cnf

openssl req -new -key private/server.key -passin pass:${_PASSWORD_} -passout pass:${_PASSWORD_} -out certs/server.csr -subj "/C="${_COUNTRY_}"/ST="${_ST_}"/L=/O="${_ORG_}"/OU="${_ORG_UNIT_}"/CN="${_COMMON_NAME_} -config openssl.cnf

openssl ca -in certs/server.csr -passin pass:${_PASSWORD_} -out certs/server.crt -cert certs/ca.crt -keyfile private/ca.key -config openssl.cnf

test -x demoCA/index.txt.old && rm demoCA/index.txt.old
test -x demoCA/serial.old && rm demoCA/serial.old
#!/bin/bash
_PASSWORD_=123456
_COUNTRY_="CN"
_ST_="BJ"
_COMMON_NAME_="localhost"
_ORG_="Uplusware"
_ORG_UNIT_="erisemail"

test -x demoCA || mkdir demoCA
test -x demoCA/newcerts || mkdir demoCA/newcerts

test -x demoCA/index.txt || touch demoCA/index.txt
test -x demoCA/serial || ( touch demoCA/serial && echo 01 > demoCA/serial )
test -x private || mkdir private
test -x certs || mkdir certs

#CA
openssl req -new -x509 -keyout private/ca.key -passout pass:${_PASSWORD_} -out certs/ca.crt -subj "/C="${_COUNTRY_}"/ST="${_ST_}"/L=/O="${_ORG_}"/OU="${_ORG_UNIT_}"/CN="${_COMMON_NAME_} -config openssl.cnf

#server
openssl genrsa -des3 -passout pass:${_PASSWORD_} -out private/server.key 1024 -config openssl.cnf

openssl req -new -key private/server.key -passin pass:${_PASSWORD_} -passout pass:${_PASSWORD_} -out certs/server.csr -subj "/C="${_COUNTRY_}"/ST="${_ST_}"/L=/O="${_ORG_}"/OU="${_ORG_UNIT_}"/CN="${_COMMON_NAME_} -config openssl.cnf

openssl ca -in certs/server.csr -passin pass:${_PASSWORD_} -out certs/server.crt -cert certs/ca.crt -keyfile private/ca.key -config openssl.cnf

test -x demoCA/index.txt.old && rm demoCA/index.txt.old
test -x demoCA/serial.old && rm demoCA/serial.old

_COMMON_NAME_="127.0.0.1"

#client
openssl genrsa -des3 -passout pass:${_PASSWORD_} -out private/client.key 1024 -config openssl.cnf

openssl req -new -key private/client.key -passin pass:${_PASSWORD_} -passout pass:${_PASSWORD_} -out certs/client.csr -subj "/C="${_COUNTRY_}"/ST="${_ST_}"/L=/O="${_ORG_}"/OU="${_ORG_UNIT_}"/CN="${_COMMON_NAME_} -config openssl.cnf

openssl ca -in certs/client.csr -passin pass:${_PASSWORD_} -out certs/client.crt -cert certs/ca.crt -keyfile private/ca.key -config openssl.cnf


openssl pkcs12 -export -clcerts -inkey private/client.key -in certs/client.crt -passout pass:${_PASSWORD_} -passin pass:${_PASSWORD_} -out certs/client.p12

openssl pkcs12 -export -clcerts -inkey private/server.key -in certs/server.crt -passout pass:${_PASSWORD_} -passin pass:${_PASSWORD_} -out certs/server.p12

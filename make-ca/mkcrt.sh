#!/bin/bash
_PASSWORD_=123456
_COUNTRY_="CN"
_COMMON_NAME_="localhost"
_ORG_="Uplusware"
_ORG_UNIT_="erisemail"

test -x index.txt || touch index.txt
test -x serial || ( touch serial && echo 01 > serial )
test -x private || mkdir private
test -x certs || mkdir certs
openssl genrsa -aes256 -passout pass:${_PASSWORD_} -out private/cakey.pem 1024

openssl req -new -key private/cakey.pem  -passin pass:${_PASSWORD_} -passout pass:${_PASSWORD_} -out private/ca.csr -subj "/C="${_COUNTRY_}"/ST=/L=/O="${_ORG_}"/OU="${_ORG_UNIT_}"/CN="${_COMMON_NAME_}
openssl x509 -req -days 365 -sha1 -extensions v3_ca -signkey private/cakey.pem -in private/ca.csr -out certs/ca.cer -passin pass:${_PASSWORD_}

openssl genrsa -aes256 -passout pass:${_PASSWORD_} -out private/server-key.pem 1024
openssl req -new -key private/server-key.pem -passin pass:${_PASSWORD_} -passout pass:${_PASSWORD_} -out private/server.csr -subj "/C="${_COUNTRY_}"/ST=/L=/O="${_ORG_}"/OU="${_ORG_UNIT_}"/CN="${_COMMON_NAME_}

openssl x509 -req -days 365 -sha1 -extensions v3_req -CA certs/ca.cer -CAkey private/cakey.pem -CAserial ca.srl -CAcreateserial -in private/server.csr -passin pass:${_PASSWORD_} -out certs/server.cer

openssl genrsa -aes256 -passout pass:${_PASSWORD_} -out private/client-key.pem 1024
openssl req -new -key private/client-key.pem -passin pass:${_PASSWORD_} -passout pass:${_PASSWORD_} -out private/client.csr -subj "/C="${_COUNTRY_}"/ST=/L=/O="${_ORG_}"/OU="${_ORG_UNIT_}"/CN="${_COMMON_NAME_}

openssl x509 -req -days 365 -sha1 -extensions v3_req -CA certs/ca.cer -CAkey private/cakey.pem -CAserial ca.srl -CAcreateserial -in private/client.csr -passin pass:${_PASSWORD_} -out certs/client.cer

openssl pkcs12 -export -clcerts -inkey private/client-key.pem -in certs/client.cer -passout pass:${_PASSWORD_} -passin pass:${_PASSWORD_} -out certs/client.p12
openssl pkcs12 -export -clcerts -inkey private/server-key.pem -in certs/server.cer -passout pass:${_PASSWORD_} -passin pass:${_PASSWORD_} -out certs/server.p12
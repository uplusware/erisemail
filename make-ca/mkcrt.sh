#!/bin/bash
touch index.txt serial
echo 01 > serial
mkdir private
mkdir certs
openssl genrsa -aes256 -out private/cakey.pem 1024
openssl req -new -key private/cakey.pem -out private/ca.csr -subj "/C=CN/ST=BJ/L=BJ/O=uplusware/OU=uplusware/CN=uplusware"
openssl x509 -req -days 365 -sha1 -extensions v3_ca -signkey private/cakey.pem -in private/ca.csr -out certs/ca.cer

openssl genrsa -aes256 -out private/server-key.pem 1024
openssl req -new -key private/server-key.pem -out private/server.csr -subj "/C=CN/ST=BJ/L=BJ/O=uplusware/OU=uplusware/CN=uplusware"
openssl x509 -req -days 365 -sha1 -extensions v3_req -CA certs/ca.cer -CAkey private/cakey.pem -CAserial ca.srl -CAcreateserial -in private/server.csr -out certs/server.cer

openssl genrsa -aes256 -out private/client-key.pem 1024
openssl req -new -key private/client-key.pem -out private/client.csr -subj "/C=CN/ST=BJ/L=BJ/O=uplusware/OU=uplusware/CN=uplusware"
openssl x509 -req -days 365 -sha1 -extensions v3_req -CA certs/ca.cer -CAkey private/cakey.pem -CAserial ca.srl -in private/client.csr -out certs/client.cer

openssl pkcs12 -export -clcerts -name myclient -inkey private/client-key.pem -in certs/client.cer -out certs/client.p12
openssl pkcs12 -export -clcerts -name myserver -inkey private/server-key.pem -in certs/server.cer -out certs/server.p12

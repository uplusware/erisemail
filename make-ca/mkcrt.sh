#!/bin/bash
rm *.key *.csr *.crt
openssl genrsa -des3 -out ca.key 1024
#chmod 400 ca.key
#openssl rsa -noout -text -in ca.key

openssl req -new -x509 -days 3650 -key ca.key -out ca.crt
#chmod 400 ca.crt 
#openssl x509 -noout -text -in ca.crt

rm ca.db* -rf

openssl genrsa -des3 -out server.key 1024 
#openssl rsa -in server.key -out server.key
#chmod 400 server.key 
#openssl rsa -noout -text -in server.key 
openssl req -new -key server.key -out server.csr 
#openssl req -noout -text -in server.csr 
./sign.sh server.csr 
#chmod 400 server.crt 
rm ca.db* -rf

openssl genrsa -des3 -out client.key 1024
#openssl rsa -in client.key -out client.key
#chmod 400 client.key
#openssl rsa -noout -text -in client.key
openssl req -new -key client.key -out client.csr
#openssl req -noout -text -in client.csr
./sign.sh client.csr
#chmod 400 client.crt
rm ca.db* -rf

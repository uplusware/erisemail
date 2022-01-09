#!/bin/bash
VERSION=1.7
ENCODING=utf8
OS=centos8

SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
cd ${SCRIPT_DIR}/src/
make clean
make
cd ${SCRIPT_DIR}
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh ${VERSION} ${ENCODING} ${OS}

cd ${SCRIPT_DIR}/src/
make clean
make DIST=1
cd ${SCRIPT_DIR}
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh ${VERSION}-dist ${ENCODING} ${OS}
cd ${SCRIPT_DIR}

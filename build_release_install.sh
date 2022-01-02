#!/bin/bash
VERSION=1.6.13
ENCODING=utf8
OS=centos8
SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
cd ${SCRIPT_DIR}/src/
make clean
make
cd ${SCRIPT_DIR}/
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh ${VERSION} ${ENCODING} ${OS}
chmod a+x ${SCRIPT_DIR}/${OS}-erisemail-bin-en-${ENCODING}-x86_64-linux/install.sh
sudo ${SCRIPT_DIR}//${OS}-erisemail-bin-en-${ENCODING}-x86_64-linux/install.sh
sudo /etc/init.d/erisemail restart
cd  ${SCRIPT_DIR}

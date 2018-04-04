#!/bin/bash
SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
cd ${SCRIPT_DIR}/src/
#make clean
make HDFS=1
cd ${SCRIPT_DIR}
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh 1.6.10 utf8 ubuntu
sudo /etc/init.d/erisemail stop
sudo ${SCRIPT_DIR}/ubuntu-erisemail-bin-en-utf8-x86_64-linux/install.sh
sudo /etc/init.d/erisemail start

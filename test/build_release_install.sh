#!/bin/bash
SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
cd ${SCRIPT_DIR}/src/
make clean
make
cd ${SCRIPT_DIR}
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh 1.6.10 utf8 ubuntu18lts
chmod a+x ${SCRIPT_DIR}/ubuntu18lts-erisemail-bin-cn-utf8-x86_64-linux/install.sh
sudo ${SCRIPT_DIR}/ubuntu18lts-erisemail-bin-cn-utf8-x86_64-linux/install.sh
sudo /etc/init.d/erisemail restart

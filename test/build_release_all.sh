#!/bin/bash
SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
cd ${SCRIPT_DIR}/src/

make clean
make
cd ${SCRIPT_DIR}
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh 1.6.10 utf8 ubuntu18lts

cd ${SCRIPT_DIR}/src/
make clean
make DIST=1
cd ${SCRIPT_DIR}
chmod a+x ${SCRIPT_DIR}/release.sh
${SCRIPT_DIR}/release.sh 1.6.10-dist utf8 ubuntu18lts

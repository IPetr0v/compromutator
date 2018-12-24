#!/bin/sh -e

cd $(dirname $0)

git submodule init
git submodule update

cd libfluid_base
./autogen.sh || true
./autogen.sh # Work around configure.ac bug
./configure
cd ..

cd libfluid_msg
./autogen.sh
./configure
cd ..

#git clone https://github.com/osrg/ryu.git

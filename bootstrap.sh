#!/bin/sh

git submodule init
git submodule update

sudo apt-get -y install build-essential libtool autoconf cmake
sudo apt-get -y install python python-pip mininet

sudo pip install ryu
pip install --user pandas retry tqdm

#sudo apt-get install libevent-dev
# TODO: Add libevent version check to cmake
# libevent version should be >= 2.1
git clone https://github.com/libevent/libevent.git
cd libevent
./autogen.sh
./configure
make -j4
sudo make install

cd third_party
./bootstrap.sh
cd ..

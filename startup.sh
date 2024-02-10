#!/bin/sh

apt -y upgrade && apt -y update
apt install -y git

apt install -y cmake
apt install -y build-essential autoconf libtool pkg-config
apt-get install -y zlib1g-dev
apt install -y tar curl python3 systemd

apt-get install gnupg curl
curl -fsSL https://www.mongodb.org/static/pgp/server-7.0.asc | \
   gpg -o /usr/share/keyrings/mongodb-server-7.0.gpg \
   --dearmor

echo "deb [ arch=amd64,arm64 signed-by=/usr/share/keyrings/mongodb-server-7.0.gpg ] https://repo.mongodb.org/apt/ubuntu jammy/mongodb-org/7.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-7.0.list
apt-get update
apt-get install -y mongodb-org
systemctl start mongod




curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.9.0/mongo-cxx-driver-r3.9.0.tar.gz
tar -xzf mongo-cxx-driver-r3.9.0.tar.gz
cd mongo-cxx-driver-r3.9.0/build

cmake ..                                \
    -DCMAKE_BUILD_TYPE=Release          \
    -DMONGOCXX_OVERRIDE_DEFAULT_INSTALL_PREFIX=OFF

cmake --build .
cmake --build . --target install

/bin/bash

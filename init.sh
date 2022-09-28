#!/bin/bash
set -e -x -o pipefail
mkdir /etc/cube

ln -fs $PWD/*.service /etc/systemd/system
apt-get install -y git libgraphicsmagick++-dev libwebp-dev libavahi-compat-libdnssd-dev libffi-dev libssl-dev 
wget --no-clobber https://static.rust-lang.org/rustup/dist/arm-unknown-linux-gnueabihf/rustup-init
if ! command -v rust &> /dev/null
then
    chmod +x ./rustup-init
    ./rustup-init -y
fi
source "$HOME/.cargo/env"

wget --no-clobber https://bootstrap.pypa.io/get-pip.py
[[ $(type -P "pip") ]] || python3 get-pip.py
grep -qxF '.local/bin' .bashrc || echo 'export PATH="$HOME/.local/bin:$PATH"' >> .bashrc && hash -r
pip3 install  'HAP-python[QRCode]'

chmod +x rgb-matrix.sh
#./rgb-matrix.sh
grep -qxF 'isolcpus=3' /boot/cmdline.txt || echo 'isolcpus=3' >> /boot/cmdline.txt



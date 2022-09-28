#!/bin/bash
set -e -x -o pipefail

ln -sf $PWD/*.glsl /etc/cube
ln -sf $PWD/as-light.py /usr/bin/
ln -sf $PWD/cpu-stats-gl /usr/bin/

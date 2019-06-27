#!/usr/bin/env bash

set -e
set -o pipefail

./bootstrap
./configure
make

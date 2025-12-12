#!/usr/bin/env bash

set -e

FLAGS="-std=c99 -lX11 -I ."

gcc bluewm.c -o bluewm $FLAGS
gcc bluewmbar.c -o bluewmbar $FLAGS

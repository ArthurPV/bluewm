#!/usr/bin/env bash

set -e

FLAGS="-Wall -std=c99 -lX11 -I ."

gcc bluewm.c -o bluewm $FLAGS
gcc bluewmbar.c -o bluewmbar $FLAGS
gcc bluewmbg.c -o bluewmbg -lspng -ljpeg $FLAGS
gcc bluewmlauncher.c -o bluewmlauncher $FLAGS
gcc bluewmclock.c -o bluewmclock $FLAGS -lm

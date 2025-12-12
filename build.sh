#!/usr/bin/env bash

set -e

gcc bluewm.c -std=c99 -o bluewm -lX11

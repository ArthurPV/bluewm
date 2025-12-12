#!/usr/bin/env bash

set -e

XEPHYR=$(command -v Xephyr)

xinit ./scripts/xinitrc -- \
    "$XEPHYR" \
        :100 \
        -ac \
        -screen 1280x720 \
        -host-cursor

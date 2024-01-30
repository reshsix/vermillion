#!/bin/bash

DIR="$(mktemp -d)"

for x in $(seq 0 255); do
    if [ "$x" -gt 32 ] && [ "$x" -ne 92 ]; then
        c="label:$(printf "\x$(printf %x "$x")")"
    elif [ "$x" -eq 92 ]; then
        c='label:\\'
    else
        c='xc:none'
    fi

    convert -background transparent -fill white \
        -antialias -font Liberation-Mono -size 8x16 -pointsize 12 \
        -gravity center "$c" "$DIR/$(printf '%03d' "$x").png"
done

montage "$DIR"/*.png +set label -tile 16x16 \
        -geometry +0+0 -background none font.png

#!/bin/bash

rm -f codecount *~ core.*
if [ "x$1" = "xrelease" ]; then
    gcc -O2 codecount.cpp -o codecount

    chmod +x codecount

    ./codecount
else
    # gcc -g3 -D_DEBUG codecount.cpp -o codecount
    gcc -g3  codecount.cpp -o codecount

    chmod +x codecount

    ./codecount
fi





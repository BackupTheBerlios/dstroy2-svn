#!/bin/sh

aclocal
automake -a -v -c --foreign
autoconf
./configure $1

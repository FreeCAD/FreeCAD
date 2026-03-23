#!/bin/sh

sed -e '/^\/\* for setup.h \*\//, /^$/ d' <discard.h.in >config.h.in

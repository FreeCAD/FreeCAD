#!/bin/sh

set -e

case "$1" in
    remove|upgrade|deconfigure)
    downloaddir="/usr/share/freecad/Mod/Robot/Lib/Kuka"
    rm -f $downloaddir/kr16.wrl
    rm -f $downloaddir/kr125_3.wrl
    rm -f $downloaddir/kr210.WRL
    rm -f $downloaddir/kr500_1.csv
    rm -f $downloaddir/kr500_1.wrl
    rm -f $downloaddir/kr_16.csv
    rm -f $downloaddir/kr_125.csv
    rm -f $downloaddir/kr_210_2.csv
    rm -f $downloaddir/testprog.dat
    rm -f $downloaddir/testprog.src
    ;;

    failed-upgrade)
    ;;

    *)
        echo "prerm called with unknown argument \`$1'" >&2
        exit 1
    ;;
esac

#DEBHELPER#

exit 0



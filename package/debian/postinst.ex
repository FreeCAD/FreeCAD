#!/bin/sh

set -e

case "$1" in
    configure)
    if [ -d /usr/share/freecad/Mod/Robot/Lib/Kuka ]; then
        echo "resource files installed..."
    else
        echo "getting  resource files ..."
        wgetoptions="--tries=2 --timeout=60"
        downloadurl="http://free-cad.svn.sourceforge.net/svnroot/free-cad/trunk"
        downloaddir="--directory-prefix=/usr/share/freecad/Mod/Robot/Lib/Kuka"
        wget $wgetoptions  $downloaddir $downloadurl/src/Mod/Robot/Lib/Kuka/kr16.wrl
        wget $wgetoptions  $downloaddir $downloadurl/src/Mod/Robot/Lib/Kuka/kr125_3.wrl
        wget $wgetoptions  $downloaddir $downloadurl/src/Mod/Robot/Lib/Kuka/kr210.WRL
        wget $wgetoptions  $downloaddir $downloadurl/src/Mod/Robot/Lib/Kuka/kr500_1.csv
        wget $wgetoptions  $downloaddir $downloadurl/src/Mod/Robot/Lib/Kuka/kr500_1.wrl
        wget $wgetoptions  $downloaddir $downloadurl/src/Mod/Robot/Lib/Kuka/kr_16.csv
        wget $wgetoptions  $downloaddir $downloadurl/src/Mod/Robot/Lib/Kuka/kr_125.csv
        wget $wgetoptions  $downloaddir $downloadurl/src/Mod/Robot/Lib/Kuka/kr_210_2.csv
        wget $wgetoptions  $downloaddir $downloadurl/src/Mod/Robot/Lib/Kuka/testprog.dat
        wget $wgetoptions  $downloaddir $downloadurl/src/Mod/Robot/Lib/Kuka/testprog.src
    fi
    ;;

    abort-upgrade|abort-remove|abort-deconfigure)
    ;;

    *)
        echo "postinst called with unknown argument \`$1'" >&2
        exit 1
    ;;
esac

#DEBHELPER#

exit 0



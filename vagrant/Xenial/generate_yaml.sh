#!/bin/bash
# snapcraft yaml automatic generation tool for FreeCAD 0.17
# (c) 2017 vejmarie 
# Distributed under LGPL v2.0
# Please report bugs at vejmarie@ruggedpod.qyshare.com

rm snapcraft.yaml
lib_list=`ls /opt/local/FreeCAD-0.17/lib/*.so`
mod_list=`ls -ltd $(find /opt/local/FreeCAD-0.17/Mod) | awk '{ print $9}'`
bin_list=`ls -ltd $(find /opt/local/FreeCAD-0.17/bin) | awk '{ print $9}'`
data_list=`ls -ltd $(find /opt/local/FreeCAD-0.17/data) | awk '{ print $9}'`
python_list=`ls -ltd $(find /usr/lib/python2.7) | awk '{ print $9}'`
pyside_list=`ls -ltd $(find  /usr/share/PySide/typesystems) | awk '{ print $9}'`
python_binary=`ls /usr/bin/python /usr/bin/py*2.7`
syslib=`ls -ltd $(find /usr/lib/x86_64-linux-gnu/dri) | awk '{ print $9}'`
syslib2=`ls -ltd $(find  /usr/lib/x86_64-linux-gnu/gtk-2.0) | awk '{ print $9}'`
syslib3=`ls -ltd $(find  /usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0) | awk '{ print $9}'`
syslib4=`ls -ltd $(find  /usr/lib/locale) | awk '{ print $9}'`
syslib5=`ls -ltd $(find  /usr/share/X11/locale) | awk '{ print $9}'`
syslib6=`ls -ltd $(find  /usr/lib/x86_64-linux-gnu/*gtk*) | awk '{ print $9}'`
syslib7=`ls -ltd $(find  /usr/share/glib-2.0) | awk '{ print $9}'`
syslib8=`ls -ltd $(find  /usr/share/i18n/locales) | awk '{ print $9}'`
echo "
name: freecad
version: 0.17
summary: development version 
description: development version This version is far from being bug free but integrates the latest FreeCAD technologies
parts:
 example-part:
    plugin: copy
    files:
      bin/launcher : bin/launcher" >> snapcraft.yaml
#      bin/FreeCAD : opt/local/FreeCAD-0.17/bin/FreeCAD" >> snapcraft.yaml
for i in $bin_list
do
        if [ ! -d "$i" ]
        then
                second_name=`echo $i | sed 's/\///'`
                echo "      $i : $second_name" >> snapcraft.yaml
        fi
done
for i in $lib_list
do
        second_name=`echo $i | sed 's/\///'`
	echo "      $i : $second_name" >> snapcraft.yaml
done
for i in $mod_list
do
        if [ ! -d "$i" ]
	then
		second_name=`echo $i | sed 's/\///'`
       		echo "      $i : $second_name" >> snapcraft.yaml
	fi
done
for i in $data_list
do
        if [ ! -d "$i" ]
        then
                second_name=`echo $i | sed 's/\///'`
                echo "      $i : $second_name" >> snapcraft.yaml
        fi
done
for i in $python_list $pyside_list
do
        if [ ! -d "$i" ]
        then
                second_name=`echo $i | sed 's/\///'`
                echo "      $i : $second_name" >> snapcraft.yaml
        fi
done
for i in $python_binary
do
        if [ ! -d "$i" ]
        then
                second_name=`echo $i | sed 's/\///'`
                echo "      $i : $second_name" >> snapcraft.yaml
        fi
done
for i in  $syslib $syslib2 $syslib3 $syslib4 $syslib5 $syslib6 $syslib7 $syslib8
do
        if [ ! -d "$i" ]
        then
                second_name=`echo $i | sed 's/\///'`
                echo "      $i : $second_name" >> snapcraft.yaml
        fi
done
# python:
#    plugin: python2
echo "
    snap:
     - bin/launcher
     - opt/local/FreeCAD-0.17/bin/FreeCAD
     - opt
     - usr" >> snapcraft.yaml
for i in $lib_list
do
        second_name=`echo $i | sed 's/\///'`
        echo "     - $second_name" >> snapcraft.yaml
done
echo "
apps:
 FreeCAD:
  command: bin/launcher
  plugs: [ locale-control,x11,opengl,network-bind,home,unity7 ]
" >> snapcraft.yaml

#!/usr/bin/python3

# -*- coding: utf-8 -*-
# (c) 2010 Werner Mayer LGPL

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2010 Werner Mayer <wmayer@users.sourceforge.net>        *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Library General Public License (LGPL)   *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

# Changelog:
# 0.1 Initial Release

from __future__ import print_function

#Usage = """updateqm - run lrelease for all .ts files found in the source directories

#Usage:
#   updateqm

#Authors:
#  (c) 2022 FreeCAD Volunteers
#  Licence: LGPL

#Version:
#  0.1
#"""

import os, sys

directories = [
        {"tsname":"FreeCAD", "workingdir":"./src/Gui/Language"},
        {"tsname":"AddonManager", "workingdir":"./src/Mod/AddonManager/Resources/translations"},
        {"tsname":"Arch", "workingdir":"./src/Mod/Arch/Resources/translations"},
        {"tsname":"Draft", "workingdir":"./src/Mod/Draft/Resources/translations"},
        {"tsname":"Drawing", "workingdir":"./src/Mod/Drawing/Gui/Resources/translations"},
        {"tsname":"Fem", "workingdir":"./src/Mod/Fem/Gui/Resources/translations"},
        {"tsname":"Image", "workingdir":"./src/Mod/Image/Gui/Resources/translations"},
        {"tsname":"Mesh", "workingdir":"./src/Mod/Mesh/Gui/Resources/translations"},
        {"tsname":"MeshPart", "workingdir":"./src/Mod/MeshPart/Gui/Resources/translations"},
        {"tsname":"OpenSCAD", "workingdir":"./src/Mod/OpenSCAD/Resources/translations"},
        {"tsname":"PartDesign", "workingdir":"./src/Mod/PartDesign/Gui/Resources/translations"},
        {"tsname":"Part", "workingdir":"./src/Mod/Part/Gui/Resources/translations"},
        {"tsname":"Path", "workingdir":"./src/Mod/Path/Gui/Resources/translations"},
        {"tsname":"Points", "workingdir":"./src/Mod/Points/Gui/Resources/translations"},
        {"tsname":"Raytracing", "workingdir":"./src/Mod/Raytracing/Gui/Resources/translations"},
        {"tsname":"ReverseEngineering", "workingdir":"./src/Mod/ReverseEngineering/Gui/Resources/translations"},
        {"tsname":"Robot", "workingdir":"./src/Mod/Robot/Gui/Resources/translations"},
        {"tsname":"Sketcher", "workingdir":"./src/Mod/Sketcher/Gui/Resources/translations"},
        {"tsname":"Spreadsheet", "workingdir":"./src/Mod/Spreadsheet/Gui/Resources/translations"},
        {"tsname":"Start", "workingdir":"./src/Mod/Start/Gui/Resources/translations"},
        {"tsname":"TechDraw", "workingdir":"./src/Mod/TechDraw/Gui/Resources/translations"},
        {"tsname":"Test", "workingdir":"./src/Mod/Test/Gui/Resources/translations"},
        {"tsname":"Tux", "workingdir":"./src/Mod/Tux/Resources/translations"},
        {"tsname":"Web", "workingdir":"./src/Mod/Web/Gui/Resources/translations"}
        ]

LRELEASE = ""

def find_tools():

    print("First, lets find lrelese tool on your system")
    global LRELEASE
    if (os.system("lrelease -version") == 0):
        LRELEASE = "lrelease"
    elif (os.system("lrelease-qt5 -version") == 0):
        LRELEASE = "lrelease-qt5"
    else:
        raise Exception("Cannot find lrelease")
    print("\nQt tool lrelease have been found!",
          "\t" + LRELEASE   + "\n")
#    print("==============================================\n")

def make_qmfiles(entry):

    global LRELEASE
#    print ("\n\n=============================================")
    print (f"MAKING QM files FOR {entry['tsname']}")
#    print ("=============================================")
    cur = os.getcwd()
    log_redirect = f" 2>> {cur}/qm_create_stderr.log 1>> {cur}/qm_create_stdout.log"
    os.chdir(entry["workingdir"])

    execline = []
    execline.append (f"{LRELEASE} *.ts {log_redirect}")
#    print(f"Executing commands in {entry['workingdir']}:")
    for line in execline:
#        print (line)
        os.system(line)
#    print()
    os.chdir(cur)

def main(mod=None):

    find_tools()
    path = os.path.realpath(__file__)
    path = os.path.dirname(path)
    os.chdir(path)
    os.chdir("..")
    os.chdir("..")
    print(" BEGINNING MAKING .qm FILES")
    if mod:
        for i in directories:
            if i["tsname"] == mod:
                print("WARNING - Running lrelease for ",mod,"ONLY")
                make_qmfiles(i)
                break
    else:
        for i in directories:
            make_qmfiles(i)
    print("Updateqm.py run finish")
    print("stderr output from lrelease can be found in qm_create_stderr.log")
    print("stdout output from lrelease can be found in qm_create_stdout.log")

if __name__ == "__main__":
    if len(sys.argv[1:]) > 0:
        main(sys.argv[1])
    else:
        main()

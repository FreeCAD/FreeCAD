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
# 0.5 Add support for Qt 6 lupdate (which fixes MANY bugs and should be preferred)
# 0.4 Refactor to always try both C++ and Python translations
# 0.3 User-friendly output
#     Corrections
#     Added Changelog
# 0.2 Add Qt5 support
#     Add "no obsolete" flags in order to fix 'ghost strings' in Crowdin
# 0.1 Initial Release

from __future__ import print_function

Usage = """updatets - update all .ts files found in the source directories

Usage:
   updatets

Authors:
  (c) 2010 Werner Mayer
  (c) 2019 FreeCAD Volunteers
  Licence: LGPL

Version:
  0.5
"""

import os, sys
import subprocess
import re
import pathlib

directories = [
        {"tsname":"FreeCAD", "workingdir":"./src/Gui", "tsdir":"Language"},
        {"tsname":"AddonManager", "workingdir":"./src/Mod/AddonManager/", "tsdir":"Resources/translations"},
        {"tsname":"Arch", "workingdir":"./src/Mod/Arch/", "tsdir":"Resources/translations"},
        #{"tsname":"Assembly", "workingdir":"./src/Mod/Assembly/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Draft", "workingdir":"./src/Mod/Draft/", "tsdir":"Resources/translations"},
        {"tsname":"Drawing", "workingdir":"./src/Mod/Drawing/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Fem", "workingdir":"./src/Mod/Fem/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Image", "workingdir":"./src/Mod/Image/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Mesh", "workingdir":"./src/Mod/Mesh/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"MeshPart", "workingdir":"./src/Mod/MeshPart/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"OpenSCAD", "workingdir":"./src/Mod/OpenSCAD/", "tsdir":"Resources/translations"},
        {"tsname":"PartDesign", "workingdir":"./src/Mod/PartDesign/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Part", "workingdir":"./src/Mod/Part/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Path", "workingdir":"./src/Mod/Path/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Points", "workingdir":"./src/Mod/Points/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Raytracing", "workingdir":"./src/Mod/Raytracing/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"ReverseEngineering", "workingdir":"./src/Mod/ReverseEngineering/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Robot", "workingdir":"./src/Mod/Robot/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Sketcher", "workingdir":"./src/Mod/Sketcher/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Spreadsheet", "workingdir":"./src/Mod/Spreadsheet/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Start", "workingdir":"./src/Mod/Start/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"TechDraw", "workingdir":"./src/Mod/TechDraw/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Test", "workingdir":"./src/Mod/Test/", "tsdir":"Gui/Resources/translations"},
        {"tsname":"Tux", "workingdir":"./src/Mod/Tux/", "tsdir":"Resources/translations"},
        {"tsname":"Web", "workingdir":"./src/Mod/Web/", "tsdir":"Gui/Resources/translations"}
        ]

QMAKE = ""
LUPDATE = ""
PYLUPDATE = ""
LCONVERT = ""
QT_VERSION_MAJOR = ""

def find_tools(noobsolete=True):

    print(Usage + "\nFirst, lets find all necessary tools on your system")
    global QMAKE, LUPDATE, PYLUPDATE, LCONVERT, QT_VERSION_MAJOR
    
    p = subprocess.run(["lupdate","-version"],check=True,stdout=subprocess.PIPE)
    lupdate_version = p.stdout.decode()
    result = re.search(r'.* ([456])\.([\d]+)\.([\d]+)', lupdate_version)
    if not result:
        print (f"Failed to parse version from {lupdate_version}")
    QT_VERSION_MAJOR = int(result.group(1))
    QT_VERSION_MINOR = int(result.group(2))
    QT_VERSION_PATCH = int(result.group(3))
    QT_VERSION = f"{QT_VERSION_MAJOR}.{QT_VERSION_MINOR}.{QT_VERSION_PATCH}"
    print (f"Found Qt {QT_VERSION}")

    if QT_VERSION_MAJOR < 6:
        if (os.system("lupdate -version") == 0):
            LUPDATE = "lupdate"
            # TODO: we suppose lupdate is a symlink to lupdate-qt4 for now
            if noobsolete:
                LUPDATE += " -no-obsolete"
        elif (os.system("lupdate-qt5 -version") == 0):
            LUPDATE = "lupdate-qt5"
            if noobsolete:
                LUPDATE += " -no-obsolete"
        else:
            raise Exception("Cannot find lupdate")
    else:
        LUPDATE = "lupdate"
    
    if QT_VERSION_MAJOR < 6:
        if (os.system("qmake -version") == 0):
            QMAKE = "qmake"
        elif (os.system("qmake-qt5 -version") == 0):
            QMAKE = "qmake-qt5"
        else:
            raise Exception("Cannot find qmake")
        if (os.system("pylupdate -version") == 0):
            PYLUPDATE = "pylupdate"
        elif (os.system("pylupdate5 -version") == 0):
            PYLUPDATE = "pylupdate5"
            if noobsolete:
                PYLUPDATE += " -noobsolete"
        elif (os.system("pylupdate4 -version") == 0):
            PYLUPDATE = "pylupdate4"
            if noobsolete:
                PYLUPDATE += " -noobsolete"
        elif (os.system("pyside2-lupdate -version") == 0):
            PYLUPDATE = "pyside2-lupdate"
            raise Exception("Please do not use pyside2-lupdate at the moment, as it shows encoding problems. Please use pylupdate5 instead.")
        else:
            raise Exception("Cannot find pylupdate")
    else:
        QMAKE = "(qmake not needed for Qt 6 and later)"
        PYLUPDATE = "(pylupdate not needed for Qt 6 and later)"
    if (os.system("lconvert -h") == 0):
        LCONVERT = "lconvert"
        if noobsolete and QT_VERSION_MAJOR < 6:
            LCONVERT += " -no-obsolete"
    else:
        raise Exception("Cannot find lconvert")
    print("\nAll Qt tools have been found!\n",
          "\t" + QMAKE     + "\n",
          "\t" + LUPDATE   + "\n",
          "\t" + PYLUPDATE + "\n",
          "\t" + LCONVERT  + "\n")
    print("==============================================\n")

def update_translation(entry):

    global QMAKE, LUPDATE, LCONVERT, QT_VERSION_MAJOR
    cur = os.getcwd()
    log_redirect = f" 2>> {cur}/tsupdate_stderr.log 1>> {cur}/tsupdate_stdout.log"
    os.chdir(entry["workingdir"])
    existingjsons = [f for f in os.listdir(".") if f.endswith(".json")]
    project_filename = entry["tsname"] + ".pro"
    tsBasename = os.path.join(entry["tsdir"],entry["tsname"])

    if QT_VERSION_MAJOR < 6:
        print ("\n\n=============================================")
        print (f"EXTRACTING STRINGS FOR {entry['tsname']}")
        print ("=============================================",flush=True)
        execline = []
        execline.append (f"touch dummy_cpp_file_for_lupdate.cpp") #lupdate 5.x requires at least one source file to process the UI files
        execline.append (f"{QMAKE} -project -o {project_filename} -r")
        execline.append (f"{LUPDATE} {project_filename} -ts {tsBasename}.ts {log_redirect}")
        execline.append (f"sed 's/<translation.*>.*<\/translation>/<translation type=\"unfinished\"><\/translation>/g' {tsBasename}.ts > {tsBasename}.ts.temp")
        execline.append (f"mv {tsBasename}.ts.temp {tsBasename}.ts")
        execline.append (f"{PYLUPDATE} `find ./ -name \"*.py\"` -ts {tsBasename}py.ts {log_redirect}")
        execline.append (f"{LCONVERT} -i {tsBasename}py.ts {tsBasename}.ts -o {tsBasename}.ts {log_redirect}")
        execline.append (f"rm {tsBasename}py.ts")
        execline.append (f"rm dummy_cpp_file_for_lupdate.cpp")

        print(f"Executing commands in {entry['workingdir']}:")
        for line in execline:
            print (line)
            os.system(line)
        print()

        os.remove(project_filename)
        # lupdate creates json files since Qt5.something. Remove them here too
        for jsonfile in [f for f in os.listdir(".") if f.endswith(".json")]:
            if not jsonfile in existingjsons:
                os.remove(jsonfile)

    elif QT_VERSION_MAJOR == 6:
        # In Qt6, QMake project files are deprecated, and lupdate directly scans for source files. The same executable is
        # used for all supported programming languages, so it's just a single function call

        # For Windows compatibility, do most of the work in Python:
        extensions = ["java","jui","ui","c","c++","cc","cpp","cxx","ch","h","h++","hh","hpp","hxx","js","qs","qml","qrc","py"]
        with open ("files_to_translate.txt","w") as file_list:
            for root, dirs, files in os.walk("./"):
                for f in files:
                    if pathlib.Path(f).suffix[1:] in extensions:
                        file_list.write(os.path.join(root,f) + "\n")

        try:
            p = subprocess.run([LUPDATE, "@files_to_translate.txt","-I","./", "-recursive", "-ts", f"{tsBasename}.ts"], capture_output=True, timeout=60)
        except Exception as e:
            print(str(e))
            os.chdir(cur)
            return
         
        with open (f"{cur}/tsupdate_stdout.log","a") as f:
            f.write(p.stdout.decode())
            print(p.stdout.decode())
        with open (f"{cur}/tsupdate_stderr.log","a") as f:
            f.write(p.stderr.decode())

        # Strip out obsolete strings, and make sure there are no translations in the main *.ts file
        subprocess.run([LCONVERT, "-drop-translations", "-i", f"{tsBasename}.ts", "-o", f"{tsBasename}.ts"])

    else:
        print("ERROR: unrecognized version of lupdate -- found Qt {QT_VERSION_MAJOR}, we only support 4, 5 and 6")
        exit(1)

    os.chdir(cur)

def main(mod=None):

    find_tools()
    path = os.path.realpath(__file__)
    path = os.path.dirname(path)
    os.chdir(path)
    os.chdir("..")
    os.chdir("..")
    print("\n\n\n BEGINNING TRANSLATION EXTRACTION \n\n")
    if mod:
        for i in directories:
            if i["tsname"] == mod:
                print("WARNING - Updating",mod,"ONLY")
                update_translation(i)
                break
    else:
        for i in directories:
            update_translation(i)
    print("\nIf updatets.py was run successfully, the next step is to run ./src/Tools/updatecrowdin.py")
    print("stderr output from lupdate can be found in tsupdate_stderr.log")
    print("stdout output from lupdate can be found in tsupdate_stdout.log")

if __name__ == "__main__":
    if len(sys.argv[1:]) > 0:
        main(sys.argv[1])
    else:
        main()

#!/usr/bin/python

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
  0.3
"""

import os, re

# folders that should not be treated by standard Qt tools
DirFilter = ["^Attic$",
             "^CVS$",
             "^\\.svn$",
             "^\\.deps$",
             "^\\.libs$",
             "src/Mod/Cam",
             "src/Mod/Import",
             "src/Mod/JtReader",
             "src/Mod/Sandbox",
             "src/Mod/TemplatePyMod"]

# python folders that need a special pylupdate command
PyCommands = [["src/Mod/Draft",
               'pylupdate `find ./ -name "*.py"` Resources/ui/*.ui -ts Resources/translations/Draft.ts'],
              ["src/Mod/Arch",
               'pylupdate `find ./ -name "*.py"` Resources/ui/*.ui -ts Resources/translations/Arch.ts'],
              ["src/Mod/OpenSCAD",
               "pylupdate *.py Resources/ui/*.ui -ts Resources/translations/OpenSCAD.ts"],
              ["src/Mod/Start",
               "pylupdate StartPage/*.py -ts Gui/Resources/translations/StartPagepy.ts"],
              ["src/Mod/Start",
               'lconvert -i Gui/Resources/translations/StartPagepy.ts Gui/Resources/translations/StartPage.ts -o Gui/Resources/translations/StartPage.ts'],
              ["src/Mod/Start",
               'rm Gui/Resources/translations/StartPagepy.ts'],
              ["src/Mod/Ship",
               'pylupdate `find ./ -name "*.py"` -ts resources/translations/Ship.ts'],
              ["src/Mod/Plot",
               'pylupdate `find ./ -name "*.py"` -ts resources/translations/Plot.ts'],
              ["src/Mod/Path",
               'pylupdate `find ./ -name "*.py"` -ts Gui/Resources/translations/Pathpy.ts'],
              ["src/Mod/Path",
               'lconvert -i Gui/Resources/translations/Pathpy.ts Gui/Resources/translations/Path.ts -o Gui/Resources/translations/Path.ts'],
              ["src/Mod/Path",
               'rm Gui/Resources/translations/Pathpy.ts'],
              ["src/Mod/Fem",
               'pylupdate `find ./ -name "*.py"` -ts Gui/Resources/translations/Fempy.ts'],
              ["src/Mod/Fem",
               'lconvert -i Gui/Resources/translations/Fempy.ts Gui/Resources/translations/Fem.ts -o Gui/Resources/translations/Fem.ts'],
              ["src/Mod/Fem",
               'rm Gui/Resources/translations/Fempy.ts'],
              ["src/Mod/Tux",
               'pylupdate `find ./ -name "*.py"` -ts Resources/translations/Tux.ts'],
              ["src/Mod/Part",
               'pylupdate `find ./ -name "*.py"` -ts Gui/Resources/translations/Partpy.ts'],
              ["src/Mod/Part",
               'lconvert -i Gui/Resources/translations/Partpy.ts Gui/Resources/translations/Part.ts -o Gui/Resources/translations/Part.ts'],
              ["src/Mod/Part",
               'rm Gui/Resources/translations/Partpy.ts'],
              ["src/Mod/Image",
               'pylupdate `find ./ -name "*.py"` -ts Gui/Resources/translations/Imagepy.ts'],
              ["src/Mod/Image",
               'lconvert -i Gui/Resources/translations/Imagepy.ts Gui/Resources/translations/Image.ts -o Gui/Resources/translations/Image.ts'],
              ["src/Mod/Image",
               'rm Gui/Resources/translations/Imagepy.ts'],
              ["src/Mod/AddonManager",
               "pylupdate *.py *.ui -ts Resources/translations/AddonManager.ts"],
               ]

# add python folders to exclude list
for c in PyCommands:
    DirFilter.append(c[0]+"$")

QMAKE = ""
LUPDATE = ""
PYLUPDATE = ""
LCONVERT = ""

def find_tools(noobsolete=True):

    print(Usage + "\nFirst, lets find all necessary tools on your system")
    global QMAKE, LUPDATE, PYLUPDATE, LCONVERT
    if (os.system("qmake -version") == 0):
        QMAKE = "qmake"
    elif (os.system("qmake-qt5 -version") == 0):
        QMAKE = "qmake-qt5"
    elif (os.system("qmake-qt4 -version") == 0):
        QMAKE = "qmake-qt4"
    else:
        raise Exception("Cannot find qmake")
    if (os.system("lupdate -version") == 0):
        LUPDATE = "lupdate"
        # TODO: we suppose lupdate is a symlink to lupdate-qt4 for now
        if noobsolete:
            LUPDATE += " -no-obsolete"
    elif (os.system("lupdate-qt5 -version") == 0):
        LUPDATE = "lupdate-qt5"
        if noobsolete:
            LUPDATE += " -no-obsolete"
    elif (os.system("lupdate-qt4 -version") == 0):
        LUPDATE = "lupdate-qt4"
        if noobsolete:
            LUPDATE += " -noobsolete"
    else:
        raise Exception("Cannot find lupdate")
    if (os.system("pyside2-lupdate -version") == 0):
        PYLUPDATE = "pyside2-lupdate"
    elif (os.system("pylupdate -version") == 0):
        PYLUPDATE = "pylupdate"
    elif (os.system("pylupdate5 -version") == 0):
        PYLUPDATE = "pylupdate5"
        if noobsolete:
            PYLUPDATE += " -noobsolete"
    elif (os.system("pylupdate4 -version") == 0):
        PYLUPDATE = "pylupdate4"
        if noobsolete:
            PYLUPDATE += " -noobsolete"
    else:
        raise Exception("Cannot find pylupdate")
    if (os.system("lconvert -h") == 0):
        LCONVERT = "lconvert"
        if noobsolete:
            LCONVERT += " -no-obsolete"
    else:
        raise Exception("Cannot find lconvert")
    print("\nAll Qt tools have been found!\n",
          "\t" + QMAKE     + "\n",
          "\t" + LUPDATE   + "\n",
          "\t" + PYLUPDATE + "\n",
          "\t" + LCONVERT  + "\n")
    print("==============================================\n")

def filter_dirs(item):

    global DirFilter
    if not os.path.isdir(item):
        return False
    for regexp in DirFilter:
        a = re.compile(regexp)
        if (re.match(a, item)):
            return False
    return True

def update_translation(path):

    global QMAKE, LUPDATE
    cur = os.getcwd()
    os.chdir(path)
    existingjsons = [f for f in os.listdir(".") if f.endswith(".json")]
    filename = os.path.basename(path) + ".pro"
    os.system(QMAKE + " -project")
    #os.system(LUPDATE + " " + filename)
    #only update the master ts file
    tsname = ""
    if "Mod" in path:
        tsname = " -ts "+os.path.join("Gui","Resources","translations",os.path.basename(path) + ".ts")
    elif "src/Gui" in path:
        tsname = " -ts "+os.path.join("Language", "FreeCAD.ts")
    os.system(LUPDATE + " " + filename + tsname)
    os.remove(filename)
    # lupdate creates json files since Qt5.something. Remove them here too
    for jsonfile in [f for f in os.listdir(".") if f.endswith(".json")]:
        if not jsonfile in existingjsons:
            os.remove(jsonfile)
    os.chdir(cur)

def update_python_translation(item):

    global PYLUPDATE, LCONVERT
    cur = os.getcwd()
    os.chdir(item[0])
    execline = item[1].replace("pylupdate",PYLUPDATE)
    execline = execline.replace("lconvert",LCONVERT)
    print("Executing special command in ",item[0],": ",execline)
    os.system(execline)
    os.chdir(cur)

def main():

    find_tools()
    path = os.path.realpath(__file__)
    path = os.path.dirname(path)
    os.chdir(path)
    os.chdir("..")
    os.chdir("..")
    dirs=os.listdir("src/Mod")
    for i in range(len(dirs)):
        dirs[i] = "src/Mod/" + dirs[i]
    dirs.append("src/Base")
    dirs.append("src/App")
    dirs.append("src/Gui")
    dirs = filter(filter_dirs, dirs)
    for i in dirs:
        update_translation(i)
    for j in PyCommands:
        update_python_translation(j)
    print("\nIf updatets.py was run successfully, the next step is to run ./src/Tools/updatecrowdin.py")

if __name__ == "__main__":
    main()

#! python
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


Usage = """updatets - update all .ts files found in the source directories

Usage:
   updatets 
   
Autor:
  (c) 2010 Werner Mayer
  Licence: GPL

Version:
  0.1
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
               "pylupdate *.py draftlibs/*.py Resources/ui/*.ui -ts Resources/translations/Draft.ts"],
              ["src/Mod/Arch",
               "pylupdate *.py Resources/ui/*.ui -ts Resources/translations/Arch.ts"],
              ["src/Mod/OpenSCAD",
               "pylupdate *.py Resources/ui/*.ui -ts Resources/translations/OpenSCAD.ts"],
              ["src/Mod/Start",
               "pylupdate StartPage/*.py -ts Gui/Resources/translations/StartPage.ts"],
              ["src/Mod/Ship",
               'pylupdate `find ./ -name "*.py"` -ts resources/translations/Ship.ts'],
              ["src/Mod/Plot",
               'pylupdate `find ./ -name "*.py"` -ts resources/translations/Plot.ts']]

# add python folders to exclude list
for c in PyCommands:
    DirFilter.append(c[0])
             
QMAKE = ""
LUPDATE = ""
PYLUPDATE = ""

def find_tools():
    global QMAKE, LUPDATE, PYLUPDATE
    if (os.system("qmake-qt4 -version") == 0):
        QMAKE = "qmake-qt4"
    elif (os.system("qmake -version") == 0):
        QMAKE = "qmake"
    else:
        raise Exception("Cannot find qmake")
    if (os.system("lupdate-qt4 -version") == 0):
        LUPDATE = "lupdate-qt4"
    elif (os.system("lupdate -version") == 0):
        LUPDATE = "lupdate"
    else:
        raise Exception("Cannot find lupdate")
    if (os.system("pylupdate -version") == 0):
        PYLUPDATE = "pylupdate"
    elif (os.system("pylupdate4 -version") == 0):
        PYLUPDATE = "pylupdate4"
    else:
        raise Exception("Cannot find pylupdate")
    print "Qt tools:", QMAKE, LUPDATE, PYLUPDATE

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
    filename = os.path.basename(path) + ".pro"
    os.system(QMAKE + " -project")
    os.system(LUPDATE + " " + filename)
    os.remove(filename)
    os.chdir(cur)

def update_python_translation(item):
    global PYLUPDATE
    cur = os.getcwd()
    os.chdir(item[0])
    execline = item[1].replace("pylupdate",PYLUPDATE)
    print "Executing special command in ",item[0],": ",execline
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

if __name__ == "__main__":
    main()


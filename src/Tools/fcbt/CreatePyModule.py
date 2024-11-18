# ***************************************************************************
# *   Copyright (c) 2003 Werner Mayer <werner.wm.mayer@gmx.de>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

# FreeCAD MakeNewBuildNbr script
# Creates a new application

import os, sys
import MakeAppTools
import re

FilFilter = [
    "^.*\\.o$",
    "^.*Makefile$",
    "^.*\\.la$",
    "^.*\\.lo$",
    "^.*\\.positions$",
    "^.*\\.aux$",
    "^.*\\.bsc$",
    "^.*\\.exp$",
    "^.*\\.ilg$",
    "^.*\\.ilk$",
    "^.*\\.in$",
    "^.*\\.mak$",
    "^.*\\.ncb$",
    "^.*\\.opt$",
    "^.*\\.pyc$",
    "^.*\\.pyd$",
    "^.*\\.pdb$",
    "^.*\\.plg$",
]

DirFilter = [
    "^.*\\.o$",
    "^Debug$",
    "^DebugCmd$",
    "^DebugPy$",
    "^Release$",
    "^ReleaseCmd$",
    "^ReleasePy$",
    "^Attic$",
    "^CVS$",
    "^\\.svn$",
    "^\\.deps$",
    "^\\.libs$",
]


def SetupFilter(MatchList):
    RegList = []
    for regexp in MatchList:
        a = re.compile(regexp)
        RegList.append(a)
    return RegList


def createApp(Application):
    """
    Create a new application by copying the template
    """
    # create directory ../Mod/<Application>
    if not os.path.isdir("../Mod/" + Application):
        os.mkdir("../Mod/" + Application)
    else:
        sys.stdout.write(Application + " already exists. Please enter another name.\n")
        sys.exit()

    # copying files from _TEMPLATEPY_ to ../Mod/<Application>
    sys.stdout.write("Copying files...")
    MakeAppTools.copyTemplate(
        "_TEMPLATEPY_",
        "../Mod/" + Application,
        "_TEMPLATEPY_",
        Application,
        SetupFilter(FilFilter),
        SetupFilter(DirFilter),
    )
    sys.stdout.write("Ok\n")

    # replace the _TEMPLATEPY_ string by <Application>
    sys.stdout.write("Modifying files...\n")
    MakeAppTools.replaceTemplate("../Mod/" + Application, "_TEMPLATEPY_", Application)
    MakeAppTools.replaceTemplate(
        "../Mod/" + Application,
        "${CMAKE_SOURCE_DIR}/src/Tools/",
        "${CMAKE_SOURCE_DIR}/src/Mod/",
    )
    # make the configure script executable
    # os.chmod("../Mod/" + Application + "/configure", 0777);
    sys.stdout.write("Modifying files done.\n")

    sys.stdout.write(Application + " module created successfully.\n")


def validateApp(AppName):
    """
    Validates the class name
    """
    if len(AppName) < 2:
        sys.stdout.write("Too short name: '" + AppName + "'\n")
        sys.exit()
    # name is long enough
    clName = "class " + AppName + ": self=0"
    try:
        exec(clName)
    except Exception:
        # Invalid class name
        sys.stdout.write("Invalid name: '" + AppName + "'\n")
        sys.exit()


sys.stdout.write("Please enter a name for your application:")
sys.stdout.flush()
AppName = sys.stdin.readline()[:-1]
validateApp(AppName)
createApp(AppName)

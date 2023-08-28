# ***************************************************************************
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM solver object Z88"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package SolverZ88
#  \ingroup FEM

import glob
import os

import FreeCAD

from . import tasks
from .. import run
from .. import solverbase
from femtools import femutils

if FreeCAD.GuiUp:
    import FemGui

ANALYSIS_TYPES = ["static"]


def create(doc, name="SolverZ88"):
    return femutils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(solverbase.Proxy):
    """The Fem::FemSolver's Proxy python type, add solver specific properties
    """

    Type = "Fem::SolverZ88"

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)
        obj.Proxy = self

        # z88_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Z88")

        obj.addProperty("App::PropertyEnumeration", "AnalysisType", "Fem", "Type of the analysis")
        obj.AnalysisType = ANALYSIS_TYPES
        obj.AnalysisType = ANALYSIS_TYPES[0]

    def createMachine(self, obj, directory, testmode=False):
        return run.Machine(
            solver=obj, directory=directory,
            check=tasks.Check(),
            prepare=tasks.Prepare(),
            solve=tasks.Solve(),
            results=tasks.Results(),
            testmode=testmode)

    def editSupported(self):
        return True

    def edit(self, directory):
        pattern = os.path.join(directory, "z88i1.txt")
        FreeCAD.Console.PrintMessage("{}\n".format(pattern))
        f = glob.glob(pattern)[0]
        FemGui.open(f)
        pattern = os.path.join(directory, "z88i2.txt")
        FreeCAD.Console.PrintMessage("{}\n".format(pattern))
        f = glob.glob(pattern)[0]
        FemGui.open(f)
        pattern = os.path.join(directory, "z88i5.txt")
        FreeCAD.Console.PrintMessage("{}\n".format(pattern))
        f = glob.glob(pattern)[0]
        FemGui.open(f)
        pattern = os.path.join(directory, "z88man.txt")
        FreeCAD.Console.PrintMessage("{}\n".format(pattern))
        f = glob.glob(pattern)[0]
        FemGui.open(f)
        pattern = os.path.join(directory, "z88mat.txt")
        FreeCAD.Console.PrintMessage("{}\n".format(pattern))
        f = glob.glob(pattern)[0]
        FemGui.open(f)
        pattern = os.path.join(directory, "z88elp.txt")
        FreeCAD.Console.PrintMessage("{}\n".format(pattern))
        f = glob.glob(pattern)[0]
        FemGui.open(f)

    def execute(self, obj):
        return


class ViewProxy(solverbase.ViewProxy):

    def getIcon(self):
        return ":/icons/FEM_SolverZ88.svg"

##  @}

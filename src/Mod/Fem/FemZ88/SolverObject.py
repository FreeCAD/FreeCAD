# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "Z88 SolverObject"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package SolverZ88
#  \ingroup FEM

import os
import glob

import FreeCAD
import FemSolverObject
import FemMisc
import FemRun

import Tasks

if FreeCAD.GuiUp:
    import FemGui

ANALYSIS_TYPES = ["static"]


def create(doc, name="SolverZ88"):
    return FemMisc.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(FemSolverObject.Proxy):
    """The Fem::FemSolver's Proxy python type, add solver specific properties
    """

    Type = "Fem::FemSolverObjectZ88"

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)
        obj.Proxy = self

        # z88_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Z88")

        obj.addProperty("App::PropertyEnumeration", "AnalysisType", "Fem", "Type of the analysis")
        obj.AnalysisType = ANALYSIS_TYPES
        obj.AnalysisType = ANALYSIS_TYPES[0]

    def createMachine(self, obj, directory):
        return FemRun.Machine(
            solver=obj, directory=directory,
            check=Tasks.Check(),
            prepare=Tasks.Prepare(),
            solve=Tasks.Solve(),
            results=Tasks.Results())

    def editSupported(self):
        return True

    def edit(self, directory):
        pattern = os.path.join(directory, "*.txt")
        print(pattern)
        f = glob.glob(pattern)[0]
        FemGui.open(f)

    def execute(self, obj):
        return


class ViewProxy(FemSolverObject.ViewProxy):
    pass

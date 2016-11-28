# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "_FemSolverZ88"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package FemSolverZ88
#  \ingroup FEM

# import FreeCAD
import FemToolsZ88


class _FemSolverZ88():
    """The Fem::FemSolver's Proxy python type, add solver specific properties
    """
    def __init__(self, obj):
        self.Type = "FemSolverZ88"
        self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to  this object

        obj.addProperty("App::PropertyString", "SolverType", "Base", "Type of the solver", 1)  # the 1 set the property to ReadOnly
        obj.SolverType = str(self.Type)

        # fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")  # not needed ATM

        obj.addProperty("App::PropertyPath", "WorkingDir", "Fem", "Working directory for calculations, will only be used it is left blank in preferences")
        # the working directory is not set, the solver working directory is only used if the preferences working directory is left blank

        obj.addProperty("App::PropertyEnumeration", "AnalysisType", "Fem", "Type of the analysis")
        known_analysis_types = FemToolsZ88.FemToolsZ88.known_analysis_types
        obj.AnalysisType = known_analysis_types
        obj.AnalysisType = known_analysis_types[0]

    def execute(self, obj):
        return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state

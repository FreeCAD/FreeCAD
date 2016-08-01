# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "_FemSolverCalculix"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import FreeCAD
import FemToolsCcx


class _FemSolverCalculix():
    """The Fem::FemSolver's Proxy python type, add solver specific properties
    """
    def __init__(self, obj):
        self.Type = "FemSolverCalculix"
        self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to  this object

        obj.addProperty("App::PropertyString", "SolverType", "Base", "Type of the solver", 1)  # the 1 set the property to ReadOnly
        obj.SolverType = str(self.Type)

        fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        ccx_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx")

        obj.addProperty("App::PropertyPath", "WorkingDir", "Fem", "Working directory for calculations")
        obj.WorkingDir = fem_prefs.GetString("WorkingDir", "")

        obj.addProperty("App::PropertyEnumeration", "AnalysisType", "Fem", "Type of the analysis")
        obj.AnalysisType = FemToolsCcx.FemToolsCcx.known_analysis_types
        analysis_type = ccx_prefs.GetInt("AnalysisType", 0)
        obj.AnalysisType = FemToolsCcx.FemToolsCcx.known_analysis_types[analysis_type]

        known_geom_nonlinear_types = ["linear", "nonlinear"]
        obj.addProperty("App::PropertyEnumeration", "GeometricalNonlinearity", "Fem", "Type of geometrical nonlinearity")
        obj.GeometricalNonlinearity = known_geom_nonlinear_types
        geom = ccx_prefs.GetBool("NonlinearGeometry", False)
        if geom is True:
            obj.GeometricalNonlinearity = known_geom_nonlinear_types[1]  # nonlinear
        else:
            obj.GeometricalNonlinearity = known_geom_nonlinear_types[0]  # linear

        obj.addProperty("App::PropertyIntegerConstraint", "NumberOfEigenmodes", "Fem", "Number of modes for frequency calculations")
        noe = ccx_prefs.GetInt("NumberOfEigenmodes", 10)
        obj.NumberOfEigenmodes = (noe, 1, 100, 1)

        obj.addProperty("App::PropertyFloatConstraint", "EigenmodeLowLimit", "Fem", "Low frequency limit for eigenmode calculations")
        # Not yet in prefs, so it will always default to 0.0
        ell = ccx_prefs.GetFloat("EigenmodeLowLimit", 0.0)
        obj.EigenmodeLowLimit = (ell, 0.0, 1000000.0, 10000.0)

        obj.addProperty("App::PropertyFloatConstraint", "EigenmodeHighLimit", "Fem", "High frequency limit for eigenmode calculations")
        ehl = ccx_prefs.GetFloat("EigenmodeHighLimit", 1000000.0)
        obj.EigenmodeHighLimit = (ehl, 0.0, 1000000.0, 10000.0)

        obj.addProperty("App::PropertyIntegerConstraint", "Maxiterations", "Fem", "Number of iterations allowed before stopping jobs")
        niter = ccx_prefs.GetInt("AnalysisMaxIterations", 200)
        obj.Maxiterations = (niter)

        obj.addProperty("App::PropertyFloatConstraint", "InitialTimeStep", "Fem", "Initial time steps")
        ini = ccx_prefs.GetFloat("AnalysisInitialTimeStep", 1.0)
        obj.InitialTimeStep = (ini)

        obj.addProperty("App::PropertyFloatConstraint", "EndTime", "Fem", "Initial time steps")
        eni = ccx_prefs.GetFloat("AnalysisTime", 1.0)
        obj.EndTime = (eni)

        obj.addProperty("App::PropertyBool", "SteadyState", "Fem", "Run steady state or transient analysis")
        sted = ccx_prefs.GetBool("StaticAnalysis", True)
        obj.SteadyState = (sted)

        known_ccx_solver_types = ["default", "spooles", "iterativescaling", "iterativecholesky"]
        obj.addProperty("App::PropertyEnumeration", "MatrixSolverType", "Fem", "Type of solver to use")
        obj.MatrixSolverType = known_ccx_solver_types
        solver_type = ccx_prefs.GetInt("Solver", 0)
        obj.MatrixSolverType = known_ccx_solver_types[solver_type]

    def execute(self, obj):
        return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state

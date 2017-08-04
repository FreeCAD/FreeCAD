# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
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


__title__ = "Elmer Solver Object"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


import FemDefsElmer
import FemElmerTasks
import FemSolve


class _FemSolverElmer(object):
    """Proxy for FemSolverElmers Document Object."""

    Type = "FemSolverElmer"

    def __init__(self, obj):
        obj.Proxy = self

        # Prop_None     = 0
        # Prop_ReadOnly = 1
        # Prop_Transient= 2
        # Prop_Hidden   = 4
        # Prop_Output   = 8

        obj.addProperty(
                "App::PropertyPythonObject", "SupportedTypes",
                "Base", "", 4)
        obj.SupportedTypes = FemDefsElmer.SUPPORTED

        obj.addProperty(
                "App::PropertyLink", "ElmerResult",
                "Base", "", 4 | 8)
        obj.addProperty(
                "App::PropertyLink", "ElmerOutput",
                "Base", "", 4 | 8)

        obj.addProperty(
                "App::PropertyString", "SolverType",
                "Base", "Type of the solver.", 1)
        obj.addProperty(
                "App::PropertyEnumeration", "AnalysisType",
                "Fem", "Type of the analysis.")
        obj.addProperty(
                "App::PropertyInteger", "EigenmodesCount",
                "Fem", "Number of modes for frequency calculations")
        obj.addProperty(
                "App::PropertyInteger", "LinMaxIterations",
                "Fem", "Maximum iterations for iterative methods")
        obj.addProperty(
                "App::PropertyFloatConstraint", "LinConvergenceTolerance",
                "Fem", "Stopping criterion for iterative methods")
        obj.addProperty(
                "App::PropertyInteger", "TermoNLinMaxIterations",
                "Fem", "Maximum iterations for heat equations (non linear)")
        obj.addProperty(
                "App::PropertyFloatConstraint",
                "TermoNLinConvergenceTolerance",
                "Fem", "Stopping criterion for heat equations (linear)")
        obj.addProperty(
                "App::PropertyInteger", "TermoLinMaxIterations",
                "Fem", "Maximum iterations for heat equations (non linear)")
        obj.addProperty(
                "App::PropertyFloatConstraint", "TermoLinConvergenceTolerance",
                "Fem", "Stopping criterion for heat equations (linear)")
        obj.AnalysisType = FemDefsElmer.SUPPORTED

        # Set default values for properties.
        obj.SolverType = self.Type
        obj.AnalysisType = FemDefsElmer.STATIC
        obj.EigenmodesCount = 10
        obj.LinMaxIterations = 500
        obj.LinConvergenceTolerance = 1e-10
        obj.TermoLinMaxIterations = 500
        obj.TermoLinConvergenceTolerance = 1e-10
        obj.TermoNLinMaxIterations = 20
        obj.TermoNLinConvergenceTolerance = 1e-7

    def buildMachine(self, obj, directory):
        return FemSolve.Machine(
            solver=obj, directory=directory,
            check=FemElmerTasks.Check(),
            prepare=FemElmerTasks.Prepare(),
            solve=FemElmerTasks.Solve(),
            results=FemElmerTasks.Results())

    def execute(self, obj):
        return True

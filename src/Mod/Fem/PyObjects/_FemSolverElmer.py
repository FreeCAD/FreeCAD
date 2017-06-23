# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Markus Hovorka <m.hovorka@live.de>               *
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
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import FemDefsElmer


class _FemSolverElmer(object):
    """Proxy for FemSolverElmers Document Object."""

    solverType = "FemSolverElmer"
    Type = ""

    def __init__(self, obj):
        obj.Proxy = self

        # Prop_None     = 0
        # Prop_ReadOnly = 1
        # Prop_Transient= 2
        # Prop_Hidden   = 4
        # Prop_Output   = 8
        attr = 1

        obj.addProperty(
                "App::PropertyString", "SolverType",
                "Base", "Type of the solver.", attr)
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
                "App::PropertyFloatConstraint", "TermoNLinConvergenceTolerance",
                "Fem", "Stopping criterion for heat equations (linear)")
        obj.addProperty(
                "App::PropertyInteger", "TermoLinMaxIterations",
                "Fem", "Maximum iterations for heat equations (non linear)")
        obj.addProperty(
                "App::PropertyFloatConstraint", "TermoLinConvergenceTolerance",
                "Fem", "Stopping criterion for heat equations (linear)")
        obj.AnalysisType = FemDefsElmer.SUPPORTED

        # Set default values for properties.
        obj.SolverType = self.solverType
        obj.AnalysisType = FemDefsElmer.STATIC
        obj.EigenmodesCount = 10
        obj.LinMaxIterations = 500
        obj.LinConvergenceTolerance = 1e-10
        obj.TermoLinMaxIterations = 500
        obj.TermoLinConvergenceTolerance = 1e-10
        obj.TermoNLinMaxIterations = 20
        obj.TermoNLinConvergenceTolerance = 1e-7

    def execute(self, obj):
        return True

# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2022 Uwe Stöhr <uwestoehr@lyx.org>                      *
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

__title__ = "FreeCAD FEM solver Elmer equation object _Linear"
__author__ = "Markus Hovorka, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from . import equation


# the linear equation object defines some attributes for some various elmer equations
# these various elmer equations are based on the linear equation object
# thus in ObjectsFem module is no method to add a linear equation object


LINEAR_SOLVER = ["Direct", "Iterative"]
LINEAR_DIRECT = ["Banded", "MUMPS", "Umfpack"]
LINEAR_ITERATIVE = [
    "BiCGStab",
    "BiCGStabl",
    "CG",
    "GCR",
    "CGS",
    "GMRES",
    "Idrs",
    "TFQMR"
]
LINEAR_PRECONDITIONING = [
    "None",
    "Diagonal",
    "ILU0",
    "ILU1",
    "ILU2",
    "ILU3",
    "ILU4",
    "ILUT"
]


class Proxy(equation.Proxy):

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)

        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "BiCGstablDegree",
            "Linear System",
            "Polynom degree for iterative method 'BiCGstabl'"
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "IdrsParameter",
            "Linear System",
            "Parameter for iterative method 'Idrs'"
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "LinearDirectMethod",
            "Linear System",
            ""
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "LinearIterations",
            "Linear System",
            ""
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "LinearIterativeMethod",
            "Linear System",
            ""
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "LinearPreconditioning",
            "Linear System",
            ""
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "LinearSolverType",
            "Linear System",
            ""
        )
        obj.addProperty(
            "App::PropertyBool",
            "LinearSystemSolverDisabled",
            "Linear System",
            (
                "Disable the linear system.\n"
                "Only use for special cases\n"
                "and consult the Elmer docs."
            )
        )
        obj.addProperty(
            "App::PropertyFloat",
            "LinearTolerance",
            "Linear System",
            "Linear preconditioning method"
        )
        obj.addProperty(
            "App::PropertyBool",
            "Stabilize",
            "Base",
            ""
        )
        obj.addProperty(
            "App::PropertyFloat",
            "SteadyStateTolerance",
            "Steady State",
            ""
        )

        obj.BiCGstablDegree = (2, 2, 10, 1)
        obj.IdrsParameter = (2, 1, 10, 1)
        obj.LinearDirectMethod = LINEAR_DIRECT
        obj.LinearIterations = (500, 1, int(1e6), 50)
        obj.LinearIterativeMethod = LINEAR_ITERATIVE
        obj.LinearIterativeMethod = "BiCGStab"
        obj.LinearPreconditioning = LINEAR_PRECONDITIONING
        obj.LinearPreconditioning = "ILU0"
        # we must set an expression because we don't have a UI, the user has to
        # view and edit the tolerance via the property editor and this does not
        # yet allow to view and edit small numbers in scientific notation
        # forum thread: https://forum.freecad.org/viewtopic.php?p=613897#p613897
        obj.setExpression("LinearTolerance", "1e-10")
        obj.LinearSolverType = LINEAR_SOLVER
        obj.LinearSolverType = "Iterative"
        # same reason to setup an expression as with LinearTolerance
        obj.setExpression("SteadyStateTolerance", "1e-5")
        obj.Stabilize = True


class ViewProxy(equation.ViewProxy):
    pass

##  @}

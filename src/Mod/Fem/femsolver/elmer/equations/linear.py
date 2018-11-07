# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

from . import equation


# the linear equation object defines some attributes for some various elmer equations
# these various elmer equations are based on the linear equation object
# thus in ObjectsFem module is no method to add a linear equation object


LINEAR_SOLVER = ["Direct", "Iterative"]
LINEAR_DIRECT = ["Banded", "umfpack"]
LINEAR_ITERATIVE = [
    "CG",
    "CGS",
    "BiCGStab",
    "BiCGStabl",
    "TFQMR",
    "GMRES",
    "GCR",
]
LINEAR_PRECONDITIONING = [
    "None",
    "Diagonal",
    "ILU0",
    "ILU1",
    "ILU2",
    "ILU3",
    "ILU4",
]


class Proxy(equation.Proxy):

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)
        obj.addProperty(
            "App::PropertyEnumeration", "LinearSolverType",
            "Linear System", "Select type of solver for linear system")
        obj.LinearSolverType = LINEAR_SOLVER
        obj.LinearSolverType = "Iterative"
        obj.addProperty(
            "App::PropertyEnumeration", "LinearDirectMethod",
            "Linear System", "Select type of solver for linear system")
        obj.LinearDirectMethod = LINEAR_DIRECT
        obj.addProperty(
            "App::PropertyEnumeration", "LinearIterativeMethod",
            "Linear System", "Select type of solver for linear system")
        obj.LinearIterativeMethod = LINEAR_ITERATIVE
        obj.LinearIterativeMethod = "BiCGStab"
        obj.addProperty(
            "App::PropertyInteger", "BiCGstablDegree",
            "Linear System", "Select type of solver for linear system")
        obj.addProperty(
            "App::PropertyEnumeration", "LinearPreconditioning",
            "Linear System", "Select type of solver for linear system")
        obj.LinearPreconditioning = LINEAR_PRECONDITIONING
        obj.LinearPreconditioning = "ILU0"
        obj.addProperty(
            "App::PropertyFloat", "LinearTolerance",
            "Linear System", "Select type of solver for linear system")
        obj.LinearTolerance = 1e-8
        obj.addProperty(
            "App::PropertyInteger", "LinearIterations",
            "Linear System", "Select type of solver for linear system")
        obj.LinearIterations = 500
        obj.addProperty(
            "App::PropertyFloat", "SteadyStateTolerance",
            "Steady State", "Select type of solver for linear system")
        obj.SteadyStateTolerance = 1e-5
        obj.addProperty(
            "App::PropertyBool", "Stabilize",
            "Base", "Select type of solver for linear system")
        obj.Stabilize = True
        obj.addProperty(
            "App::PropertyBool", "Bubbles",
            "Base", "Select type of solver for linear system")
        obj.Bubbles = False


class ViewProxy(equation.ViewProxy):
    pass

##  @}

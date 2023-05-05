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

__title__ = "FreeCAD FEM solver Elmer equation object _NonLinear"
__author__ = "Markus Hovorka, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from . import linear


# the linear equation object defines some attributes for some various elmer equations
# these various elmer equations are based on the linear equation object
# thus in ObjectsFem module is no method to add a linear equation object


class Proxy(linear.Proxy):

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)

        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "NonlinearIterations",
            "Nonlinear System",
            "Maximum number of iterations"
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "NonlinearNewtonAfterIterations",
            "Nonlinear System",
            ""
        )
        obj.addProperty(
            "App::PropertyFloat",
            "NonlinearNewtonAfterTolerance",
            "Nonlinear System",
            ""
        )
        obj.addProperty(
            "App::PropertyFloat",
            "NonlinearTolerance",
            "Nonlinear System",
            ""
        )
        obj.addProperty(
            "App::PropertyFloatConstraint",
            "RelaxationFactor",
            "Nonlinear System",
            (
                "Value below 1.0 might be necessary to achieve convergence\n"
                "Typical values are in the range [0.3, 1.0]"
            )
        )

        obj.NonlinearIterations = (20, 1, int(1e6), 10)
        obj.NonlinearNewtonAfterIterations = (3, 1, 500, 1)
        # for small numbers we must set an expression because we don't have a UI,
        # the user has to view and edit the tolerance via the property editor and
        # this does not yet allow to view and edit small numbers in scientific notation
        # forum thread: https://forum.freecad.org/viewtopic.php?p=613897#p613897
        obj.setExpression("NonlinearTolerance", "1e-7")
        obj.setExpression("NonlinearNewtonAfterTolerance", "1e-3")
        obj.RelaxationFactor = (1.0, 0.0001, 2.0, 0.1)


class ViewProxy(linear.ViewProxy):
    pass

##  @}

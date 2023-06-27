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

__title__ = "FreeCAD FEM solver Elmer equation object Elasticity"
__author__ = "Markus Hovorka, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from femtools import femutils
from ... import equationbase
from . import linear

EIGEN_SYSTEM_SELECT = [
    "Smallest Magnitude",
    "Largest Magnitude",
    "Smallest Real Part",
    "Largest Real Part",
    "Smallest Imag Part",
    "Largest Imag Part"
]


def create(doc, name="Elasticity"):
    return femutils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(linear.Proxy, equationbase.ElasticityProxy):

    Type = "Fem::EquationElmerElasticity"

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)

        obj.addProperty(
            "App::PropertyBool",
            "CalculatePangle",
            "Elasticity",
            "Compute principal stress angles"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculatePrincipal",
            "Elasticity",
            "Compute principal stress components"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateStrains",
            "Elasticity",
            "Compute the strain tensor"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateStresses",
            "Elasticity",
            "Compute stress tensor and vanMises"
        )
        obj.addProperty(
            "App::PropertyBool",
            "ConstantBulkSystem",
            "Elasticity",
            "See Elmer manual for info"
        )
        obj.addProperty(
            "App::PropertyBool",
            "DisplaceMesh",
            "Elasticity",
            (
                "If mesh is deformed by displacement field.\n"
                "Set to False for 'Eigen Analysis'."
            )
        )
        obj.addProperty(
            "App::PropertyBool",
            "EigenAnalysis",
            "Eigen Values",
            "If true, modal analysis"
        )
        obj.addProperty(
            "App::PropertyBool",
            "EigenSystemComplex",
            "Eigen Values",
            (
                "Should be true if eigen system is complex\n"
                "Must be false for a damped eigen value analysis."
            )
        )
        obj.addProperty(
            "App::PropertyBool",
            "EigenSystemComputeResiduals",
            "Eigen Values",
            "Computes residuals of eigen value system"
        )
        obj.addProperty(
            "App::PropertyBool",
            "EigenSystemDamped",
            "Eigen Values",
            (
                "Set a damped eigen analysis. Can only be\n"
                "used if 'Linear Solver Type' is 'Iterative'."
            )
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "EigenSystemMaxIterations",
            "Eigen Values",
            "Max iterations for iterative eigensystem solver"
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "EigenSystemSelect",
            "Eigen Values",
            "Which eigenvalues are computed"
        )
        obj.addProperty(
            "App::PropertyFloat",
            "EigenSystemTolerance",
            "Eigen Values",
            (
                "Convergence tolerance for iterative eigensystem solve\n"
                "Default is 100 times the 'Linear Tolerance'"
            )
        )
        obj.addProperty(
            "App::PropertyInteger",
            "EigenSystemValues",
            "Eigen Values",
            "Number of lowest eigen modes"
        )
        obj.addProperty(
            "App::PropertyBool",
            "FixDisplacement",
            "Elasticity",
            "If displacements or forces are set,\nthereby model lumping is used"
        )
        obj.addProperty(
            "App::PropertyBool",
            "GeometricStiffness",
            "Elasticity",
            "Consider geometric stiffness"
        )
        obj.addProperty(
            "App::PropertyBool",
            "Incompressible",
            "Elasticity",
            (
                "Computation of incompressible material in connection\n"
                "with viscoelastic Maxwell material and a custom 'Variable'"
            )
        )
        obj.addProperty(
            "App::PropertyBool",
            "MaxwellMaterial",
            "Elasticity",
            "Compute viscoelastic material model"
        )
        obj.addProperty(
            "App::PropertyBool",
            "ModelLumping",
            "Elasticity",
            "Use model lumping"
        )
        obj.addProperty(
            "App::PropertyFile",
            "ModelLumpingFilename",
            "Elasticity",
            "File to save results from model lumping to"
        )
        obj.addProperty(
            "App::PropertyBool",
            "StabilityAnalysis",
            "Elasticity",
            (
                "If true, 'Eigen Analysis' is stability analysis.\n"
                "Otherwise modal analysis is performed."
            )
        )
        obj.addProperty(
            "App::PropertyBool",
            "UpdateTransientSystem",
            "Elasticity",
            "See Elmer manual for info"
        )
        obj.addProperty(
            "App::PropertyString",
            "Variable",
            "Elasticity",
            (
                "Only change this if 'Incompressible' is set to true\n"
                "according to the Elmer manual."
            )
        )

        obj.addProperty(
            "App::PropertyBool",
            "PlaneStress",
            "Equation",
            (
                "Computes solution according to plane\nstress situation.\n"
                "Applies only for 2D geometry."
            )
        )

        obj.EigenSystemValues = 5
        obj.Priority = 10
        obj.CalculatePrincipal = True
        obj.DisplaceMesh = True
        # according to Elmer tutorial and forum, for stresses direct solving
        # is recommended -> tests showed 10 times faster and even more accurate
        obj.LinearSolverType = "Direct"
        obj.LinearDirectMethod = "Umfpack"
        # according to Elmer manual should be true except there is damping
        # we set damping by default to false
        obj.EigenSystemComplex = True
        # according to Elmer manual the default is 300
        obj.EigenSystemMaxIterations = (300, 1, int(1e8), 1)
        # according to Elmer manual the default is Smallest magnitude
        obj.EigenSystemSelect = EIGEN_SYSTEM_SELECT
        obj.EigenSystemSelect = "Smallest Magnitude"
        # according to Elmer manual default is 100 times the Linear Tolerance
        # since this is a small value we must set an expression, see linear.py for background
        for (prop, expr) in obj.ExpressionEngine:
            if (prop == "LinearTolerance"):
                obj.setExpression("EigenSystemTolerance", str(100 * obj.evalExpression(expr)))
        obj.Variable = "Displacement"


class ViewProxy(linear.ViewProxy, equationbase.ElasticityViewProxy):
    pass

##  @}

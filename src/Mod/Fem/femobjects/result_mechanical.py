# ***************************************************************************
# *   Copyright (c) 2016 Qingfeng Xia <qingfeng.xia()eng.ox.ac.uk>          *
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM result mechanical document object"
__author__ = "Qingfeng Xia, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package result_mechanical
#  \ingroup FEM
#  \brief mechanical result object

from . import base_fempythonobject


class ResultMechanical(base_fempythonobject.BaseFemPythonObject):
    """
    The Fem::ResultMechanical's Proxy python type, add result specific properties
    """

    Type = "Fem::ResultMechanical"

    def __init__(self, obj):
        super().__init__(obj)

        obj.addLockedProperty(
            "App::PropertyString",
            "ResultType",
            "Base",
            "Type of the result",
            1,  # the 1 set the property to ReadOnly
        )

        obj.ResultType = str(self.Type)

        # for frequency analysis
        obj.addLockedProperty("App::PropertyInteger", "Eigenmode", "Data", "", True)

        obj.addLockedProperty(
            "App::PropertyFloat", "EigenmodeFrequency", "Data", "User Defined Results", True
        )

        # node results
        # set read only or hide a property:
        # https://forum.freecad.org/viewtopic.php?f=18&t=13460&start=10#p108072
        # do not show up in propertyEditor of comboView
        obj.addLockedProperty(
            "App::PropertyVectorList",
            "DisplacementVectors",
            "NodeData",
            "List of displacement vectors",
            True,
        )

        obj.addLockedProperty(
            "App::PropertyFloatList",
            "Peeq",
            "NodeData",
            "List of equivalent plastic strain values",
            True,
        )

        obj.addLockedProperty(
            "App::PropertyFloatList",
            "MohrCoulomb",
            "NodeData",
            "List of Mohr Coulomb stress values",
            True,
        )

        obj.addLockedProperty(
            "App::PropertyFloatList",
            "ReinforcementRatio_x",
            "NodeData",
            "Reinforcement ratio x-direction",
            True,
        )

        obj.addLockedProperty(
            "App::PropertyFloatList",
            "ReinforcementRatio_y",
            "NodeData",
            "Reinforcement ratio y-direction",
            True,
        )

        obj.addLockedProperty(
            "App::PropertyFloatList",
            "ReinforcementRatio_z",
            "NodeData",
            "Reinforcement ratio z-direction",
            True,
        )

        # these three principal vectors are used only if there is a reinforced mat obj
        # https://forum.freecad.org/viewtopic.php?f=18&t=33106&p=416006#p416006
        obj.addLockedProperty(
            "App::PropertyVectorList",
            "PS1Vector",
            "NodeData",
            "List of 1st Principal Stress Vectors",
            True,
        )

        obj.addLockedProperty(
            "App::PropertyVectorList",
            "PS2Vector",
            "NodeData",
            "List of 2nd Principal Stress Vectors",
            True,
        )

        obj.addLockedProperty(
            "App::PropertyVectorList",
            "PS3Vector",
            "NodeData",
            "List of 3rd Principal Stress Vectors",
            True,
        )

        # readonly in propertyEditor of comboView
        obj.addLockedProperty(
            "App::PropertyFloatList",
            "DisplacementLengths",
            "NodeData",
            "List of displacement lengths",
            True,
        )

        obj.addLockedProperty(
            "App::PropertyFloatList",
            "vonMises",
            "NodeData",
            "List of von Mises equivalent stresses",
            True,
        )

        obj.addLockedProperty("App::PropertyFloatList", "PrincipalMax", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "PrincipalMed", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "PrincipalMin", "NodeData", "", True)

        obj.addLockedProperty(
            "App::PropertyFloatList",
            "MaxShear",
            "NodeData",
            "List of Maximum Shear stress values",
            True,
        )

        obj.addLockedProperty(
            "App::PropertyFloatList",
            "MassFlowRate",
            "NodeData",
            "List of mass flow rate values",
            True,
        )

        obj.addLockedProperty(
            "App::PropertyFloatList",
            "NetworkPressure",
            "NodeData",
            "List of network pressure values",
            True,
        )

        obj.addLockedProperty(
            "App::PropertyFloatList", "UserDefined", "NodeData", "User Defined Results", True
        )

        obj.addLockedProperty(
            "App::PropertyFloatList", "Temperature", "NodeData", "Temperature field", True
        )
        obj.addLockedProperty(
            "App::PropertyVectorList", "HeatFlux", "NodeData", "List of heat flux vectors", True
        )

        obj.addLockedProperty("App::PropertyFloatList", "NodeStressXX", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "NodeStressYY", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "NodeStressZZ", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "NodeStressXY", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "NodeStressXZ", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "NodeStressYZ", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "NodeStrainXX", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "NodeStrainYY", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "NodeStrainZZ", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "NodeStrainXY", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "NodeStrainXZ", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "NodeStrainYZ", "NodeData", "", True)

        obj.addLockedProperty("App::PropertyFloatList", "CriticalStrainRatio", "NodeData", "", True)

        # initialize the Stats with the appropriate count of items
        # see fill_femresult_stats in femresult/resulttools.py
        zero_list = 26 * [0]
        obj.Stats = zero_list

    def onDocumentRestored(self, obj):
        # migrate old result objects, because property "StressValues"
        # was renamed to "vonMises" in commit 8b68ab7
        if hasattr(obj, "StressValues") is True:
            obj.addLockedProperty(
                "App::PropertyFloatList",
                "vonMises",
                "NodeData",
                "List of von Mises equivalent stresses",
                True,
            )

            obj.vonMises = obj.StressValues
            obj.setPropertyStatus("StressValues", "-LockDynamic")
            obj.removeProperty("StressValues")

        # migrate old result objects, because property "Stats"
        # consisting of min, avg, max values was reduced to min, max in commit c2a57b3e
        if len(obj.Stats) == 39:
            temp = obj.Stats
            for i in range(12, -1, -1):
                del temp[3 * i + 1]
            obj.Stats = temp

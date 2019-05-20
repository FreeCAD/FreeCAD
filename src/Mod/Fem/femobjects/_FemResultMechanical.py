# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 Qingfeng Xia <qingfeng.xia()eng.ox.ac.uk>          *
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
__url__ = "http://www.freecadweb.org"

## @package FemResultMechanical
#  \ingroup FEM
#  \brief FreeCAD DocumentObject class to hold mechanical results in FEM workbench


class _FemResultMechanical():
    """The Fem::_FemResultMechanical's Proxy python type, add result specific properties
    """
    def __init__(self, obj):
        self.Type = "Fem::FemResultMechanical"
        self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to this object

        obj.addProperty(
            "App::PropertyString",
            "ResultType",
            "Base",
            "Type of the result",
            1  # the 1 set the property to ReadOnly
        )
        obj.ResultType = str(self.Type)

        # for frequency analysis
        obj.addProperty(
            "App::PropertyInteger",
            "Eigenmode",
            "Data",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloat",
            "EigenmodeFrequency",
            "Data",
            "User Defined Results",
            True
        )

        # node results
        # set read only or hide a property:
        # https://forum.freecadweb.org/viewtopic.php?f=18&t=13460&start=10#p108072
        # do not show up in propertyEditor of comboView
        obj.addProperty(
            "App::PropertyVectorList",
            "DisplacementVectors",
            "NodeData",
            "List of displacement vectors",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "Peeq",
            "NodeData",
            "List of equivalent plastic strain values",
            True
        )
        # readonly in propertyEditor of comboView
        obj.addProperty(
            "App::PropertyFloatList",
            "DisplacementLengths",
            "NodeData",
            "List of displacement lengths",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "StressValues",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "PrincipalMax",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "PrincipalMed",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "PrincipalMin",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "MaxShear",
            "NodeData",
            "List of Maximum Shear stress values",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "MassFlowRate",
            "NodeData",
            "List of mass flow rate values",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NetworkPressure",
            "NodeData",
            "List of network pressure values",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "UserDefined",
            "NodeData",
            "User Defined Results",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "Temperature",
            "NodeData",
            "Temperature field",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NodeStressXX",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NodeStressYY",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NodeStressZZ",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NodeStressXY",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NodeStressXZ",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NodeStressYZ",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NodeStrainXX",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NodeStrainYY",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NodeStrainZZ",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NodeStrainXY", "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NodeStrainXZ",
            "NodeData",
            "",
            True
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "NodeStrainYZ",
            "NodeData",
            "",
            True
        )

        # initialize the Stats with the appropriate count of items
        # see fill_femresult_stats in femresult/resulttools.py
        zero_list = 39 * [0]
        obj.Stats = zero_list

    # standard Feature methods
    def execute(self, obj):
        """"this method is executed on object creation and
        whenever the document is recomputed"
        update Part or Mesh should NOT lead to recomputation
        of the analysis automatically, time consuming
        """
        return

    def onChanged(self, obj, prop):
        return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state

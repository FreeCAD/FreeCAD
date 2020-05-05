# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD individual reinforcement object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import FreeCAD
from FreeCAD import Vector as vec

from .reinforcement_generic import ReinforcementGeneric

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP


class ReinforcementIndividual(ReinforcementGeneric):

    """
    A individual reinforcement object based on a rebar object

    """

    def __init__(
        self,
        obj
    ):
        super(ReinforcementIndividual, self).__init__(obj)
        self.Type = "ReinforcementIndividual"

        pl = obj.PropertiesList

        # New properties

        # linked vertieces
        if "Vertieces" not in pl:
            obj.addProperty(
                "App::PropertyLinkList",
                "Vertieces",
                "ArrayOfRebars",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The linked vertex objects to place the rebars."
                )
            )

    def execute(
        self,
        obj  # why obj? self is the obj?
    ):

        # what should be used to access an attribute
        # self.Attribute
        # obj.Attribute
        # same in onChanged()

        if self.clone(obj):
            return
        if not obj.BaseRebar:
            return
        if not obj.Vertieces:
            return

        pl_list = []
        rot = FreeCAD.Rotation()
        for v in obj.Vertieces:
            # Placment is not set for Part Vertex
            # built placement out of the coordinates attributes
            vertex_vec = vec(v.X, v.Y, v.Z)
            # print(FreeCAD.Placement(vertex_vec, rot))
            pl_list.append(FreeCAD.Placement(vertex_vec, rot))
        obj.RebarPlacements = pl_list

        self.build_shape(obj)
        obj.Amount = len(obj.RebarPlacements)
        obj.TotalLength = obj.Amount * obj.BaseRebar.Length

        # set Visibility of BaseRebar
        # this should be done in the Gui Command,
        # but this dos not yet exist TODO
        # set view of base rebar to off
        # if reinforcement shape is not a null shape
        # TODO may be use another color for base rebar
        if FreeCAD.GuiUp:
            if obj.Shape.isNull() is not True:
                obj.BaseRebar.ViewObject.Visibility = False

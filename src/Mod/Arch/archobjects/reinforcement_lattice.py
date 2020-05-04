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

__title__ = "FreeCAD reinforcement object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import FreeCAD

from . reinforcement_generic import ReinforcementGeneric

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP


class ReinforcementLattice(ReinforcementGeneric):

    """A reinforcement bar (rebar) object
    for a reinforcement based on a lattic2 placement"""

    def __init__(
        self,
        obj
    ):
        super(ReinforcementLattice, self).__init__(obj)

        # self.setPropertiesLattice(obj)
        # why the reinforcement properties should have been added ...
        # move to __init__ :-)

        self.Type = "ReinforcementLattice"

        # LatticePlacement
        # can a reinforcement have multiple lattice placement
        # example stirrups of a column
        # but this will be difficault
        # to automatically create text for bar space etc
        # means not used ATM
        # further more dangerous because collisons,
        # cause double placements
        # same if one reinforcement in more rebar shapes
        # such is cool but corner cases could make problems
        if "LatticePlacement" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLink",
                "LatticePlacement",
                "Reinforcement",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Lattice placement obj for this reinforcement"
                )
            )

    """
    # exact the same as in super class, thus not needed
    def onDocumentRestored(
        self,
        obj
    ):
        ReinforcementGeneric.onDocumentRestored(obj)
        self.setProperties(obj)
    """

    def execute(
        self,
        obj
    ):
        if self.clone(obj):
            return
        if not obj.LatticePlacement:
            return

        from lattice2BaseFeature import isObjectLattice as islattice
        if islattice(obj.LatticePlacement) is True:
            from lattice2BaseFeature import getPlacementsList as getpl
            obj.RebarPlacements = getpl(obj.LatticePlacement)
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
        else:
            FreeCAD.Console.PrintError(
                "The object provided: {} is not a Lattice2 object\n"
                .format(obj.Name)
            )

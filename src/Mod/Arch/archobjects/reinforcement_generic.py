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

__title__ = "FreeCAD generic reinforcement object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD

import ArchComponent
import Part


class ReinforcementGeneric(ArchComponent.Component):

    """
    The generic reinforcement object based on a rebar object

    Information
    -----------
    Placement list with the placements of each rebar is calculated.
    A compound from all rebars is created. The compound it the reinforcement.
    TODO: Create a reinforcement class especially for point reinforcement.
    A list of vertieces will be given and the reinforcement will be created.

    Who is child of who?
    --------------------
    Should the base rebar know all its reinforcements?
    or
    Should every reinforcement know its base rebar?
    Use case: User would like to change the diameter of a reinforcement.
    Diameter of the base rebar will be changed. All reinforcement changes.
    If this is not wanted, a copy of the base rebar is made, the diameter
    will be changed and the reinforcement is moved to the new base rebar.
    In TreeView a base rebar has all reinforcements as children.
    A reinforcement could be moved from one base rebar into another.
    That makes sense but it does not make sense, but it does not the
    other way around. If every reinforcement would have the rebar shape as
    child it would be confusing. Really? Why not? It just would need some
    list or group or whatever with all base rebars. TODO find out.

    Module separation?
    -----------------_
    Should ReinforcementLattice in a separate module?

    Additional Attributes
    ---------------------
    BaseRebar : App::PropertyLink
        the rebar base object
    RebarPlacements : App::PropertyPlacementList
        Placement of each rebar of the reinforcement
    BasePlacement : App::PropertyPlacement
        on base rebar could be used in many reinforcements, but the rotations
        might be different in the reinforcements. This placement is applied
        relative to each placement in RebarPlacements. Thus it is important
        for rotations only, because each rebar is rotatated instead of the
        whole reinforcement if applied in Placement attribute. A translation
        could be applied eitheron BasePlacement or Placement attribute.
    """

    def __init__(
        self,
        obj
    ):
        super(ReinforcementGeneric, self).__init__(obj)
        self.setProperties(obj)
        obj.IfcType = "Reinforcing Bar"

    def setProperties(
        self,
        obj
    ):
        self.Type = "ReinforcementGeneric"
        pl = obj.PropertiesList

        # ArchComponent properties will be inherited
        # lots of them are not needed, hide them in editor ATM
        obj.setEditorMode("Additions", 2)
        obj.setEditorMode("Axis", 2)
        obj.setEditorMode("Base", 2)
        obj.setEditorMode("HorizontalArea", 2)
        obj.setEditorMode("Material", 2)
        obj.setEditorMode("MoveBase", 2)
        obj.setEditorMode("MoveWithHost", 2)
        obj.setEditorMode("PerimeterLength", 2)
        obj.setEditorMode("StandardCode", 2)  # useful, but not used ATM
        obj.setEditorMode("Subtractions", 2)
        obj.setEditorMode("VerticalArea", 2)
        # CloneOf makes sense if two rebars are identical
        # but need different MarkNumber

        # New properties

        # BaseRebar
        if "BaseRebar" not in pl:
            obj.addProperty(
                "App::PropertyLink",
                "BaseRebar",
                "Reinforcement",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Base rebar for this reinforcement"
                )
            )

        # RebarPlacements
        if "RebarPlacements" not in pl:
            obj.addProperty(
                "App::PropertyPlacementList",
                "RebarPlacements",
                "Reinforcement",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Placement for each rebar of the reinforcement"
                )
            )
        # TODO: Why ist this property not shown in PropertyEditor

        # BasePlacement
        if "BasePlacement" not in pl:
            obj.addProperty(
                "App::PropertyPlacement",
                "BasePlacement",
                "Reinforcement",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    (
                        "Rotations of the first rebar in "
                        "the reinforcement (Yaw-Pitch-Roll)"
                    )
                )
            )

        # Host
        if "Host" not in pl:
            obj.addProperty(
                "App::PropertyLink",
                "Host",
                "Reinforcement",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The structure object that hosts this rebar"
                    )
            )

        # Amount
        if "Amount" not in pl:
            obj.addProperty(
                "App::PropertyInteger",
                "Amount",
                "ArrayOfRebars",
                QT_TRANSLATE_NOOP("App::Property", ("The amount of rebars")),
            )
            obj.setEditorMode("Amount", 1)

        # TotalLength
        if "TotalLength" not in pl:
            obj.addProperty(
                "App::PropertyLength",
                "TotalLength",
                "Reinforcement",
                QT_TRANSLATE_NOOP(
                    "App::Property", ("The total length of all rebars")
                ),
            )
            obj.setEditorMode("TotalLength", 1)

    def onDocumentRestored(
        self,
        obj
    ):
        ArchComponent.Component.onDocumentRestored(self, obj)
        self.setProperties(obj)

    def execute(
        self,
        obj  # why obj? self is the obj?
    ):
        if self.clone(obj):
            return
        if not obj.BaseRebar:
            return
        if not obj.RebarPlacements:
            return
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

    def build_shape(
        self,
        obj
    ):
        """
        if obj has no attribute Shape
        a empty Shape can not be assigned :-)
        if not hasattr(obj, 'Shape'):
            print('{} has no Shape'.format(obj.Label))
            obj.Shape = Part.Shape()
            return
        """
        if hasattr(obj, "BaseRebar") and obj.BaseRebar is None:
            FreeCAD.Console.PrintMessage(
                "BaseRebar property is not set for reinforcement: {}. "
                "Shape of the reinforcement will be an empty shape.\n"
                .format(obj.Label)
            )
            obj.Shape = Part.Shape()
            return

        # build compound shape with base rebar
        # and reinforcement placements and BasePlacement
        shapes = []
        for pl in obj.RebarPlacements:
            bar = obj.BaseRebar.Shape.copy()
            # ATM there is no check
            # if translation vector of BasePlacement is 0, 0, 0
            bar.Placement = pl.multiply(obj.BasePlacement)
            shapes.append(bar)
        if shapes:
            obj.Shape = Part.makeCompound(shapes)

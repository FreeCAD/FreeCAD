# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides the viewprovider code for the Wire (polyline) object.

This viewprovider is also used by simple lines, B-splines, bezier curves,
and similar objects.
"""
## @package view_wire
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for the Wire (polyline) object.

## \addtogroup draftviewproviders
# @{
import pivy.coin as coin
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import DraftVecUtils
import WorkingPlane
from draftgeoutils import wires
from draftutils import gui_utils
from draftutils import params
from draftutils import utils
from draftutils.translate import translate
from draftviewproviders.view_base import ViewProviderDraft


class ViewProviderWire(ViewProviderDraft):
    """A base View Provider for the Wire object."""

    def __init__(self, vobj):
        super(ViewProviderWire, self).__init__(vobj)
        self._set_properties(vobj)

    def _set_properties(self, vobj):
        """Set the properties of objects if they don't exist."""
        super(ViewProviderWire, self)._set_properties(vobj)

        if not hasattr(vobj, "ArrowSizeStart"):
            _tip = QT_TRANSLATE_NOOP("App::Property", "Arrow size")
            vobj.addProperty("App::PropertyLength", "ArrowSizeStart", "Draft", _tip, locked=True)
            vobj.ArrowSizeStart = params.get_param("arrowsizestart")

        if not hasattr(vobj, "ArrowTypeStart"):
            _tip = QT_TRANSLATE_NOOP("App::Property", "Arrow type")
            vobj.addProperty(
                "App::PropertyEnumeration", "ArrowTypeStart", "Draft", _tip, locked=True
            )
            vobj.ArrowTypeStart = utils.ARROW_TYPES
            vobj.ArrowTypeStart = "None"

        if not hasattr(vobj, "ArrowSizeEnd"):
            _tip = QT_TRANSLATE_NOOP("App::Property", "Arrow size")
            vobj.addProperty("App::PropertyLength", "ArrowSizeEnd", "Draft", _tip, locked=True)
            vobj.ArrowSizeEnd = params.get_param("arrowsizeend")

        if not hasattr(vobj, "ArrowTypeEnd"):
            _tip = QT_TRANSLATE_NOOP("App::Property", "Arrow type")
            vobj.addProperty("App::PropertyEnumeration", "ArrowTypeEnd", "Draft", _tip, locked=True)
            vobj.ArrowTypeEnd = utils.ARROW_TYPES
            vobj.ArrowTypeEnd = "None"

    def attach(self, vobj):
        self.Object = vobj.Object

        self.color = coin.SoBaseColor()
        self.color.rgb.setValue(*vobj.LineColor[:3])

        self.startSymbol = gui_utils.dim_symbol()
        self.coords1 = coin.SoTransform()
        self.pt1 = coin.SoSeparator()
        self.pt1.addChild(self.color)
        self.pt1.addChild(self.coords1)
        self.pt1.addChild(self.startSymbol)

        self.endSymbol = gui_utils.dim_symbol()
        self.coords2 = coin.SoTransform()
        self.pt2 = coin.SoSeparator()
        self.pt2.addChild(self.color)
        self.pt2.addChild(self.coords2)
        self.pt2.addChild(self.endSymbol)

        super(ViewProviderWire, self).attach(vobj)
        self.onChanged(vobj, "ArrowSizeStart")

    def updateData(self, obj, prop):
        if (
            prop == "Points"
            and hasattr(obj, "Points")
            and len(obj.Points) >= 2
            and hasattr(self, "coords1")
            and hasattr(self, "coords2")
        ):
            self.coords1.translation.setValue(*obj.Points[0])
            v1 = obj.Points[1].sub(obj.Points[0])
            if not DraftVecUtils.isNull(v1):
                v1.normalize()
                rot1 = coin.SbRotation()
                rot1.setValue(coin.SbVec3f(1, 0, 0), coin.SbVec3f(*v1))
                self.coords1.rotation.setValue(rot1)
            self.coords2.translation.setValue(*obj.Points[-1])
            v2 = obj.Points[-2].sub(obj.Points[-1])
            if not DraftVecUtils.isNull(v2):
                v2.normalize()
                rot2 = coin.SbRotation()
                rot2.setValue(coin.SbVec3f(1, 0, 0), coin.SbVec3f(*v2))
                self.coords2.rotation.setValue(rot2)

    def onChanged(self, vobj, prop):
        if (
            prop
            in ["ArrowSizeStart", "ArrowSizeEnd", "ArrowTypeStart", "ArrowTypeEnd", "Visibility"]
            and hasattr(vobj, "ArrowSizeStart")
            and hasattr(vobj, "ArrowSizeEnd")
            and hasattr(vobj, "ArrowTypeStart")
            and hasattr(vobj, "ArrowTypeEnd")
            and hasattr(vobj, "Visibility")
            and hasattr(self, "pt1")
            and hasattr(self, "pt2")
        ):
            rn = vobj.RootNode
            rn.removeChild(self.pt1)
            rn.removeChild(self.pt2)
            if vobj.Visibility:
                self.pt1.removeChild(self.startSymbol)
                self.startSymbol = gui_utils.dim_symbol(
                    utils.ARROW_TYPES.index(vobj.ArrowTypeStart)
                )
                self.pt1.addChild(self.startSymbol)
                self.coords1.scaleFactor.setValue([vobj.ArrowSizeStart] * 3)

                self.pt2.removeChild(self.endSymbol)
                self.endSymbol = gui_utils.dim_symbol(utils.ARROW_TYPES.index(vobj.ArrowTypeEnd))
                self.pt2.addChild(self.endSymbol)
                self.coords2.scaleFactor.setValue([vobj.ArrowSizeEnd] * 3)

                self.updateData(vobj.Object, "Points")
                rn.addChild(self.pt1)
                rn.addChild(self.pt2)

        if (
            prop == "LineColor"
            and hasattr(vobj, "LineColor")
            and hasattr(self, "pt1")
            and hasattr(self, "pt2")
        ):
            self.color.rgb.setValue(*vobj.LineColor[:3])

        super().onChanged(vobj, prop)
        return

    def claimChildren(self):
        if hasattr(self.Object, "Base"):
            return [self.Object.Base, self.Object.Tool]
        return []

    def flatten(self):  # Only to be used for Draft_Wires.
        if not hasattr(self, "Object"):
            return

        wp = WorkingPlane.get_working_plane()
        flat_wire = wires.flattenWire(
            self.Object.Shape.Wires[0], origin=wp.position, normal=wp.axis
        )

        doc = App.ActiveDocument
        doc.openTransaction(translate("draft", "Flatten"))

        self.Object.Placement = App.Placement()
        self.Object.Points = [vert.Point for vert in flat_wire.Vertexes]
        self.Object.Closed = flat_wire.isClosed()

        doc.recompute()
        doc.commitTransaction()


# Alias for compatibility with v0.18 and earlier
_ViewProviderWire = ViewProviderWire

## @}

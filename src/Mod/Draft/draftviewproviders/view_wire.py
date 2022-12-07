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
import draftgeoutils.wires as wires
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftutils.messages import _msg
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

        if not hasattr(vobj, "EndArrow"):
            _tip = "Displays a Dimension symbol at the end of the wire."
            vobj.addProperty("App::PropertyBool",
                             "EndArrow",
                             "Draft",
                             QT_TRANSLATE_NOOP("App::Property", _tip))
            vobj.EndArrow = False

        if not hasattr(vobj, "ArrowSize"):
            _tip = "Arrow size"
            vobj.addProperty("App::PropertyLength",
                             "ArrowSize",
                             "Draft",
                             QT_TRANSLATE_NOOP("App::Property", _tip))
            vobj.ArrowSize = utils.get_param("arrowsize", 0.1)

        if not hasattr(vobj, "ArrowType"):
            _tip = "Arrow type"
            vobj.addProperty("App::PropertyEnumeration",
                             "ArrowType",
                             "Draft",
                             QT_TRANSLATE_NOOP("App::Property", _tip))
            vobj.ArrowType = utils.ARROW_TYPES
            vobj.ArrowType = utils.ARROW_TYPES[utils.get_param("dimsymbol", 0)]

    def attach(self, vobj):
        self.Object = vobj.Object
        col = coin.SoBaseColor()
        col.rgb.setValue(vobj.LineColor[0],vobj.LineColor[1],vobj.LineColor[2])
        self.coords = coin.SoTransform()
        self.pt = coin.SoSeparator()
        self.pt.addChild(col)
        self.pt.addChild(self.coords)
        self.symbol = gui_utils.dim_symbol()
        self.pt.addChild(self.symbol)
        super(ViewProviderWire, self).attach(vobj)
        self.onChanged(vobj,"EndArrow")

    def updateData(self, obj, prop):
        if prop == "Points":
            if obj.Points:
                p = obj.Points[-1]
                if hasattr(self,"coords"):
                    self.coords.translation.setValue((p.x,p.y,p.z))
                    if len(obj.Points) >= 2:
                        v1 = obj.Points[-2].sub(obj.Points[-1])
                        if not DraftVecUtils.isNull(v1):
                            v1.normalize()
                            _rot = coin.SbRotation()
                            _rot.setValue(coin.SbVec3f(1, 0, 0), coin.SbVec3f(v1[0], v1[1], v1[2]))
                            self.coords.rotation.setValue(_rot)
        return

    def onChanged(self, vobj, prop):
        if prop in ["EndArrow","ArrowSize","ArrowType","Visibility"]:
            rn = vobj.RootNode
            if hasattr(self,"pt") and hasattr(vobj,"EndArrow"):
                if vobj.EndArrow and vobj.Visibility:
                    self.pt.removeChild(self.symbol)
                    s = utils.ARROW_TYPES.index(vobj.ArrowType)
                    self.symbol = gui_utils.dim_symbol(s)
                    self.pt.addChild(self.symbol)
                    self.updateData(vobj.Object,"Points")
                    if hasattr(vobj,"ArrowSize"):
                        s = vobj.ArrowSize
                    else:
                        s = utils.get_param("arrowsize",0.1)
                    self.coords.scaleFactor.setValue((s,s,s))
                    rn.addChild(self.pt)
                else:
                    if self.symbol:
                        if self.pt.findChild(self.symbol) != -1:
                            self.pt.removeChild(self.symbol)
                        if rn.findChild(self.pt) != -1:
                            rn.removeChild(self.pt)

        if prop in ["LineColor"]:
            if hasattr(self, "pt"):
                self.pt[0].rgb.setValue(vobj.LineColor[0],vobj.LineColor[1],vobj.LineColor[2])

        super(ViewProviderWire, self).onChanged(vobj, prop)
        return

    def claimChildren(self):
        if hasattr(self.Object,"Base"):
            return [self.Object.Base,self.Object.Tool]
        return []

    def flatten(self): # Only to be used for Draft_Wires.
        if not hasattr(self, "Object"):
            return

        if hasattr(App, "DraftWorkingPlane"):
            App.DraftWorkingPlane.setup()
            origin = App.DraftWorkingPlane.position
            normal = App.DraftWorkingPlane.axis
            # Align the grid for visual feedback:
            if hasattr(Gui, "Snapper"):
                Gui.Snapper.setTrackers()
        else:
            origin = App.Vector(0, 0, 0)
            normal = App.Vector(0, 0, 1)

        flat_wire = wires.flattenWire(self.Object.Shape.Wires[0],
                                      origin=origin,
                                      normal=normal)

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

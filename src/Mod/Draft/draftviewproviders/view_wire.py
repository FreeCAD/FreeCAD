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
"""This module provides the view provider code for Draft wire related objects.
"""
## @package view_base
# \ingroup DRAFT
# \brief This module provides the view provider code for Draft objects like\
# Line, Polyline, BSpline, BezCurve.


from pivy import coin
from PySide import QtCore
from PySide import QtGui

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui

import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
import DraftVecUtils
import DraftGeomUtils

from draftviewproviders.view_base import ViewProviderDraft


class ViewProviderWire(ViewProviderDraft):
    """A base View Provider for the Wire object"""
    def __init__(self, vobj):
        super(ViewProviderWire, self).__init__(vobj)

        _tip = "Displays a Dimension symbol at the end of the wire"
        vobj.addProperty("App::PropertyBool", "EndArrow",
                         "Draft", QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Arrow size"
        vobj.addProperty("App::PropertyLength", "ArrowSize",
                         "Draft", QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Arrow type"
        vobj.addProperty("App::PropertyEnumeration", "ArrowType",
                         "Draft", QT_TRANSLATE_NOOP("App::Property", _tip))

        vobj.ArrowSize = utils.get_param("arrowsize",0.1)
        vobj.ArrowType = utils.ARROW_TYPES
        vobj.ArrowType = utils.ARROW_TYPES[utils.get_param("dimsymbol",0)]

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

    def setupContextMenu(self,vobj,menu):
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_Edit.svg"),
                                "Flatten this wire",
                                menu)
        QtCore.QObject.connect(action1,
                               QtCore.SIGNAL("triggered()"),
                               self.flatten)
        menu.addAction(action1)

    def flatten(self):
        if hasattr(self,"Object"):
            if len(self.Object.Shape.Wires) == 1:
                fw = DraftGeomUtils.flattenWire(self.Object.Shape.Wires[0])
                points = [v.Point for v in fw.Vertexes]
                if len(points) == len(self.Object.Points):
                    if points != self.Object.Points:
                        App.ActiveDocument.openTransaction("Flatten wire")
                        Gui.doCommand("FreeCAD.ActiveDocument." + 
                                      self.Object.Name + 
                                      ".Points=" + 
                                      str(points).replace("Vector", "FreeCAD.Vector").replace(" " , ""))
                        App.ActiveDocument.commitTransaction()

                    else:
                        _msg = "This Wire is already flat"
                        App.Console.PrintMessage(QT_TRANSLATE_NOOP("Draft", _msg) + "\n")


_ViewProviderWire = ViewProviderWire
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

__title__ = "FreeCAD generic rebar ViewProvider"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import re
from pivy import coin

import FreeCAD

import Arch
import ArchCommands
import ArchComponent
import Part

from PySide.QtCore import QT_TRANSLATE_NOOP


# ****************************************************************************
# generic rebar and reinforcement ViewProvider
class ViewProviderRebarCommon(Arch.ArchComponent.ViewProviderComponent):

    """A View Provider for the rebar and reinforcement object"""
    # inherite this class and only use a different icon
    # color may be not brown, may be depending on diameter

    def __init__(
        self,
        vobj
    ):
        super(ViewProviderRebarCommon, self).__init__(vobj)

        pl = vobj.PropertiesList
        if "RebarShape" not in pl:
            vobj.addProperty(
                "App::PropertyString",
                "RebarShape",
                "Rebar Shape",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Shape of rebar"
                )
            ).RebarShape
            vobj.setEditorMode("RebarShape", 2)

        vobj.ShapeColor = ArchCommands.getDefaultColor("Rebar")

    def onDocumentRestored(
        self,
        vobj
    ):
        self.setProperties(vobj)

    def setEdit(
        self,
        vobj,
        mode
    ):
        if mode == 0:
            if vobj.RebarShape:
                try:
                    # Import module of RebarShape
                    module = __import__(vobj.RebarShape)
                except ImportError:
                    FreeCAD.Console.PrintError(
                        "Unable to import RebarShape module\n"
                    )
                    return
                module.editDialog(vobj)

    def updateData(
        self,
        obj,
        prop
    ):
        if prop == "Shape":
            if hasattr(self, "centerline"):
                if self.centerline:
                    self.centerlinegroup.removeChild(self.centerline)
            if hasattr(obj.Proxy, "wires"):
                if obj.Proxy.wires:
                    self.centerline = coin.SoSeparator()
                    comp = Part.makeCompound(obj.Proxy.wires)
                    pts = re.findall(
                        "point \[(.*?)\]",
                        comp.writeInventor().replace("\n", "")
                    )
                    pts = [p.split(",") for p in pts]
                    for pt in pts:
                        ps = coin.SoSeparator()
                        plist = []
                        for p in pt:
                            c = []
                            for pstr in p.split(" "):
                                if pstr:
                                    c.append(float(pstr))
                            plist.append(c)
                        coords = coin.SoCoordinate3()
                        coords.point.setValues(plist)
                        ps.addChild(coords)
                        ls = coin.SoLineSet()
                        ls.numVertices = -1
                        ps.addChild(ls)
                        self.centerline.addChild(ps)
                    self.centerlinegroup.addChild(self.centerline)
        ArchComponent.ViewProviderComponent.updateData(self, obj, prop)  # ???

    def attach(
        self,
        vobj
    ):
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.centerlinegroup = coin.SoSeparator()
        self.centerlinegroup.setName("Centerline")
        self.centerlinecolor = coin.SoBaseColor()
        self.centerlinestyle = coin.SoDrawStyle()
        self.centerlinegroup.addChild(self.centerlinecolor)
        self.centerlinegroup.addChild(self.centerlinestyle)
        vobj.addDisplayMode(self.centerlinegroup, "Centerline")
        ArchComponent.ViewProviderComponent.attach(self, vobj)  # ???

    def onChanged(
        self,
        vobj,
        prop
    ):
        if (prop == "LineColor") and hasattr(vobj, "LineColor"):
            if hasattr(self, "centerlinecolor"):
                c = vobj.LineColor
                self.centerlinecolor.rgb.setValue(c[0], c[1], c[2])
        elif (prop == "LineWidth") and hasattr(vobj, "LineWidth"):
            if hasattr(self, "centerlinestyle"):
                self.centerlinestyle.lineWidth = vobj.LineWidth
        ArchComponent.ViewProviderComponent.onChanged(self, vobj, prop)  # ???

    def getDisplayModes(
        self,
        vobj
    ):
        modes = ArchComponent.ViewProviderComponent.getDisplayModes(
            self, vobj
        )
        modes.append("Centerline")
        return modes

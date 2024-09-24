# -*- coding: utf8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

"""The BIM SetWPFront command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_SetWPFront:

    def GetResources(self):
        return {
            "Pixmap": "view-front.svg",
            "MenuText": QT_TRANSLATE_NOOP("BIM_SetWPFront", "Working Plane Front"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_SetWPFront", "Set the working plane to Front"
            ),
            "Accel": "W,P,1",
        }

    def Activated(self):
        FreeCADGui.addModule("WorkingPlane")
        FreeCADGui.doCommand("WorkingPlane.get_working_plane(update=False).set_to_front()")


class BIM_SetWPSide:

    def GetResources(self):
        return {
            "Pixmap": "view-right.svg",
            "MenuText": QT_TRANSLATE_NOOP("BIM_SetWPSide", "Working Plane Side"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_SetWPSide", "Set the working plane to Side"
            ),
            "Accel": "W,P,3",
        }

    def Activated(self):
        FreeCADGui.addModule("WorkingPlane")
        FreeCADGui.doCommand("WorkingPlane.get_working_plane(update=False).set_to_side()")


class BIM_SetWPTop:

    def GetResources(self):
        return {
            "Pixmap": "view-top.svg",
            "MenuText": QT_TRANSLATE_NOOP("BIM_SetWPTop", "Working Plane Top"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_SetWPTop", "Set the working plane to Top"
            ),
            "Accel": "W,P,2",
        }

    def Activated(self):
        FreeCADGui.addModule("WorkingPlane")
        FreeCADGui.doCommand("WorkingPlane.get_working_plane(update=False).set_to_top()")


class BIM_WPView:

    def GetResources(self):
        return {
            "Pixmap": "BIM_WPView",
            "MenuText": QT_TRANSLATE_NOOP("BIM_WPView", "Working Plane View"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_WPView",
                "Aligns the view on the current item in BIM Views window or on the current working plane",
            ),
            "Accel": "9",
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        done = False
        try:
            import BimViews
        except ImportError:
            pass
        else:
            v = BimViews.findWidget()
            if v:
                i = v.tree.currentItem()
                if i:
                    # Aligning on current widget item
                    BimViews.show(i)
                    done = True
                elif hasattr(v, "lastSelected"):
                    BimViews.show(v.lastSelected)
                    # Aligning on stored widget item
                    done = True
            elif hasattr(FreeCAD, "DraftWorkingPlane"):
                if hasattr(FreeCAD.DraftWorkingPlane, "lastBuildingPart"):
                    BimViews.show(FreeCAD.DraftWorkingPlane.lastBuildingPart)
                    done = True
        if not done:
            # Aligning on current working plane
            c = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
            r = FreeCAD.DraftWorkingPlane.getRotation().Rotation.Q
            c.orientation.setValue(r)


FreeCADGui.addCommand("BIM_WPView", BIM_WPView())
FreeCADGui.addCommand("BIM_SetWPTop", BIM_SetWPTop())
FreeCADGui.addCommand("BIM_SetWPSide", BIM_SetWPSide())
FreeCADGui.addCommand("BIM_SetWPFront", BIM_SetWPFront())

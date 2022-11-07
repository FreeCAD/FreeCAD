# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides GUI tools to activate the different snapping methods."""
## @package gui_snaps
# \ingroup draftguitools
# \brief Provides GUI tools to activate the different snapping methods.

## \addtogroup draftguitools
# @{
from PySide import QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import draftguitools.gui_base as gui_base

from draftutils.messages import _log
from draftutils.translate import translate


class Draft_Snap_Base():
    """Base Class inherited by all Draft Snap commands."""

    def __init__(self, command_name="None"):
        self.command_name = command_name

    def Activated(self, status=0):
        # _log("GuiCommand: {}".format(self.command_name))
        if hasattr(Gui, "Snapper"):
            Gui.Snapper.toggle_snap(self.command_name[11:], bool(status))

    def IsActive(self):
        return hasattr(Gui, "Snapper") and Gui.Snapper.isEnabled("Lock")

    def isChecked(self):
        """Return true if the given snap is active in Snapper."""
        return hasattr(Gui, "Snapper") and self.command_name[11:] in Gui.Snapper.active_snaps


class Draft_Snap_Lock(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Lock tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Lock")

    def GetResources(self):
        return {"Pixmap":    "Snap_Lock",
                "Accel":     "Shift+S",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Lock", "Snap Lock"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Lock", "Enables or disables snapping globally."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}

    def IsActive(self): return True


Gui.addCommand("Draft_Snap_Lock", Draft_Snap_Lock())


class Draft_Snap_Midpoint(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Midpoint tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Midpoint")

    def GetResources(self):
        return {"Pixmap":    "Snap_Midpoint",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Midpoint", "Snap Midpoint"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Midpoint", "Snaps to the midpoint of edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Midpoint", Draft_Snap_Midpoint())


class Draft_Snap_Perpendicular(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Perpendicular tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Perpendicular")

    def GetResources(self):
        return {"Pixmap":    "Snap_Perpendicular",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Perpendicular", "Snap Perpendicular"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Perpendicular", "Snaps to the perpendicular points on faces and edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Perpendicular", Draft_Snap_Perpendicular())


class Draft_Snap_Grid(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Grid tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Grid")

    def GetResources(self):
        return {"Pixmap":    "Snap_Grid",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Grid", "Snap Grid"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Grid", "Snaps to the intersections of grid lines."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Grid", Draft_Snap_Grid())


class Draft_Snap_Intersection(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Intersection tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Intersection")

    def GetResources(self):
        return {"Pixmap":    "Snap_Intersection",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Intersection", "Snap Intersection"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Intersection", "Snaps to the intersection of two edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Intersection", Draft_Snap_Intersection())


class Draft_Snap_Parallel(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Parallel tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Parallel")

    def GetResources(self):
        return {"Pixmap":    "Snap_Parallel",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Parallel", "Snap Parallel"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Parallel", "Snaps to an imaginary line parallel to straight edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Parallel", Draft_Snap_Parallel())


class Draft_Snap_Endpoint(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Endpoint tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Endpoint")

    def GetResources(self):
        return {"Pixmap":    "Snap_Endpoint",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Endpoint", "Snap Endpoint"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Endpoint", "Snaps to the endpoints of edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Endpoint", Draft_Snap_Endpoint())


class Draft_Snap_Angle(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Angle tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Angle")

    def GetResources(self):
        return {"Pixmap":    "Snap_Angle",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Angle", "Snap Angle"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Angle", "Snaps to the special cardinal points on circular edges, at multiples of 30° and 45°."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Angle", Draft_Snap_Angle())


class Draft_Snap_Center(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Center tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Center")

    def GetResources(self):
        return {"Pixmap":    "Snap_Center",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Center", "Snap Center"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Center", "Snaps to the center point of faces and circular edges, and to the Placement point of Working Plane Proxies and Building Parts."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Center", Draft_Snap_Center())


class Draft_Snap_Extension(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Extension tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Extension")

    def GetResources(self):
        return {"Pixmap":    "Snap_Extension",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Extension", "Snap Extension"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Extension", "Snaps to an imaginary line that extends beyond the endpoints of straight edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Extension", Draft_Snap_Extension())


class Draft_Snap_Near(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Near tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Near")

    def GetResources(self):
        return {"Pixmap":    "Snap_Near",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Near", "Snap Near"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Near", "Snaps to the nearest point on faces and edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Near", Draft_Snap_Near())


class Draft_Snap_Ortho(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Ortho tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Ortho")

    def GetResources(self):
        return {"Pixmap":    "Snap_Ortho",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Ortho", "Snap Ortho"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Ortho", "Snaps to imaginary lines that cross the previous point at multiples of 45°."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Ortho", Draft_Snap_Ortho())


class Draft_Snap_Special(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Special tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Special")

    def GetResources(self):
        return {"Pixmap":    "Snap_Special",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Special", "Snap Special"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Special", "Snaps to special points defined by the object."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Special", Draft_Snap_Special())


class Draft_Snap_Dimensions(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Dimensions tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_Dimensions")

    def GetResources(self):
        return {"Pixmap":    "Snap_Dimensions",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Dimensions", "Snap Dimensions"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Dimensions", "Shows temporary X and Y dimensions."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Dimensions", Draft_Snap_Dimensions())


class Draft_Snap_WorkingPlane(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_WorkingPlane tool."""

    def __init__(self):
        super().__init__(command_name="Draft_Snap_WorkingPlane")

    def GetResources(self):
        return {"Pixmap":    "Snap_WorkingPlane",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_WorkingPlane", "Snap Working Plane"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_WorkingPlane", "Projects snap points onto the current working plane."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_WorkingPlane", Draft_Snap_WorkingPlane())


class ShowSnapBar(Draft_Snap_Base):
    """GuiCommand for the Draft_ShowSnapBar tool."""

    def __init__(self):
        super().__init__(command_name="Draft_ShowSnapBar")

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_ShowSnapBar", "Show snap toolbar"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_ShowSnapBar", "Shows the snap toolbar if it is hidden."),
                "CmdType":   "NoTransaction"}

    def Activated(self):
        """Execute when the command is called."""
        if hasattr(Gui, "Snapper"):
            Gui.Snapper.show()


Gui.addCommand('Draft_ShowSnapBar', ShowSnapBar())

## @}

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
import math
import FreeCAD
import FreeCADGui as Gui
from PySide import QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import draftguitools.gui_base as gui_base
from draftutils.messages import _log
from draftutils.translate import translate
from draftguitools.gui_trackers import orthoTracker
from pivy import coin

class Draft_Snap_Base():
    """Base Class inherited by all Draft Snap commands."""

    def Activated(self, status=0):
        # _log("GuiCommand: {}".format(self.__class__.__name__))

        if hasattr(Gui, "Snapper"):
            Gui.Snapper.toggle_snap(self.__class__.__name__[11:], bool(status))

    def IsActive(self):
        return hasattr(Gui, "Snapper") and Gui.Snapper.isEnabled("Lock")

    def isChecked(self):
        """Return true if the given snap is active in Snapper."""
        return hasattr(Gui, "Snapper") and self.__class__.__name__[11:] in Gui.Snapper.active_snaps


class Draft_Snap_Lock(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Lock tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Lock",
                "Accel":     "Shift+S",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Lock", "Snap lock"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Lock", "Enables or disables snapping globally."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}

    def IsActive(self): return True


Gui.addCommand("Draft_Snap_Lock", Draft_Snap_Lock())


class Draft_Snap_Midpoint(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Midpoint tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Midpoint",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Midpoint", "Snap midpoint"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Midpoint", "Snaps to the midpoint of edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Midpoint", Draft_Snap_Midpoint())


class Draft_Snap_Perpendicular(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Perpendicular tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Perpendicular",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Perpendicular", "Snap perpendicular"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Perpendicular", "Snaps to the perpendicular points on faces and edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Perpendicular", Draft_Snap_Perpendicular())


class Draft_Snap_Grid(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Grid tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Grid",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Grid", "Snap grid"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Grid", "Snaps to the intersections of grid lines."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Grid", Draft_Snap_Grid())


class Draft_Snap_Intersection(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Intersection tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Intersection",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Intersection", "Snap intersection"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Intersection", "Snaps to the intersection of two edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Intersection", Draft_Snap_Intersection())


class Draft_Snap_Parallel(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Parallel tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Parallel",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Parallel", "Snap parallel"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Parallel", "Snaps to an imaginary line parallel to straight edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Parallel", Draft_Snap_Parallel())


class Draft_Snap_Endpoint(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Endpoint tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Endpoint",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Endpoint", "Snap endpoint"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Endpoint", "Snaps to the endpoints of edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Endpoint", Draft_Snap_Endpoint())


class Draft_Snap_Angle(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Angle tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Angle",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Angle", "Snap angle"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Angle", "Snaps to the special cardinal points on circular edges, at multiples of 30° and 45°."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Angle", Draft_Snap_Angle())


class Draft_Snap_Center(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Center tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Center",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Center", "Snap center"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Center", "Snaps to the center point of faces and circular edges, and to the Placement point of Working Plane Proxies and Building Parts."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Center", Draft_Snap_Center())


class Draft_Snap_Extension(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Extension tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Extension",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Extension", "Snap extension"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Extension", "Snaps to an imaginary line that extends beyond the endpoints of straight edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Extension", Draft_Snap_Extension())


class Draft_Snap_Near(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Near tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Near",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Near", "Snap near"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Near", "Snaps to the nearest point on faces and edges."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Near", Draft_Snap_Near())


class Draft_Snap_Ortho(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Ortho tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Ortho",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Ortho", "Snap ortho"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Ortho", "Snaps to imaginary lines that cross the previous point at multiples of 45°."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Ortho", Draft_Snap_Ortho())


class Draft_Snap_Special(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Special tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Special",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Special", "Snap special"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Special", "Snaps to special points defined by the object."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Special", Draft_Snap_Special())


class Draft_Snap_Dimensions(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Dimensions tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_Dimensions",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_Dimensions", "Snap dimensions"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_Dimensions", "Shows temporary X and Y dimensions."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_Dimensions", Draft_Snap_Dimensions())


class Draft_Snap_WorkingPlane(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_WorkingPlane tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap_WorkingPlane",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_Snap_WorkingPlane", "Snap working plane"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_Snap_WorkingPlane", "Projects snap points onto the current working plane."),
                "CmdType":   "NoTransaction",
                "Checkable": self.isChecked()}


Gui.addCommand("Draft_Snap_WorkingPlane", Draft_Snap_WorkingPlane())


class ShowSnapBar(Draft_Snap_Base):
    """GuiCommand for the Draft_ShowSnapBar tool."""

    def GetResources(self):
        return {"Pixmap":    "Draft_Snap",
                "MenuText":  QT_TRANSLATE_NOOP("Draft_ShowSnapBar", "Show snap toolbar"),
                "ToolTip":   QT_TRANSLATE_NOOP("Draft_ShowSnapBar", "Shows the snap toolbar if it is hidden."),
                "CmdType":   "NoTransaction"}

    def Activated(self):
        """Execute when the command is called."""
        if hasattr(Gui, "Snapper"):
            toolbar = Gui.Snapper.get_snap_toolbar()
            if toolbar is not None:
                toolbar.show()


Gui.addCommand('Draft_ShowSnapBar', ShowSnapBar())

class Draft_Snap_Ortho_Extension():
    """Extends orthogonal snap functionality."""
    
    def __init__(self):
        self.active = False
        self.base_point = None
        self.distance = None
        self.tracker = None
        self.last_cursor_pos = None
        self.dimension_display = None
        self.keyboard_buffer = ""
        self.keyboard_input_active = False
        
    def activate(self):
        """Activate ortho tracking."""
        if not self.active:
            self.active = True
            if not self.tracker:
                from draftguitools.gui_trackers import orthoTracker
                self.tracker = orthoTracker()
                
            # callbacks only when activating
            from pivy import coin
            self.keyboard_cb = Gui.ActiveDocument.ActiveView.addEventCallbackPivy(
                coin.SoKeyboardEvent.getClassTypeId(), self.keyboard_event)
            self.mouse_cb = Gui.ActiveDocument.ActiveView.addEventCallbackPivy(
                coin.SoLocation2Event.getClassTypeId(), self.mouse_event)
                
            self.tracker.on()

    def deactivate(self):
        """Deactivate ortho tracking."""
        self.active = False
        if self.tracker:
            self.tracker.off()
            self.tracker.finalize()
            self.tracker = None
        self.base_point = None
        self.distance = None

    def get_ortho_points(self, point):
        """Calculate orthogonal points from base point."""
        if not self.base_point:
            return []
            
        # Calculating points at 0, 90, 180, 270 degrees
        points = []
        if self.distance:
            # Use specified distance
            for angle in [0, math.pi/2, math.pi, 3*math.pi/2]:
                vec = FreeCAD.Vector(math.cos(angle), math.sin(angle), 0)
                vec.scale(self.distance, self.distance, self.distance)
                points.append(self.base_point.add(vec))
        else:
            # Use the cursor distance
            dx = point.x - self.base_point.x
            dy = point.y - self.base_point.y
            dist = math.sqrt(dx*dx + dy*dy)
            
            for angle in [0, math.pi/2, math.pi, 3*math.pi/2]:
                vec = FreeCAD.Vector(math.cos(angle), math.sin(angle), 0)
                vec.scale(dist, dist, dist)
                points.append(self.base_point.add(vec))
                
        return points

    def set_base_point(self, point):
        """Set new base point for orthogonal tracking."""
        self.base_point = point
        if self.tracker:
            self.tracker.set_base_point(point)

    def set_distance(self, dist):
        """Set fixed tracking distance."""
        self.distance = dist
        if self.tracker:
            self.tracker.set_distance(dist)

    def keyboard_event(self, event):
        """Handle keyboard events for ortho tracking.
        
        Parameters
        ----------
        event: coin.SoKeyboardEvent
            The keyboard event from the 3D view
        """
        # Q key activates held point
        if event.getKey() == ord('Q'):
            if event.getState() == coin.SoKeyboardEvent.DOWN:
                if self.last_cursor_pos:
                    self.set_base_point(self.last_cursor_pos)
                    
        # Enter key for numeric input
        elif event.getKey() == coin.SoKeyboardEvent.RETURN:
            if self.keyboard_input_active:
                try:
                    dist = float(self.keyboard_buffer)
                    self.set_distance(dist)
                except ValueError:
                    pass
                self.keyboard_input_active = False
                self.keyboard_buffer = ""
                
        # Numeric input
        elif event.getKey() in range(ord('0'), ord('9')+1) or event.getKey() == ord('.'):
            if not self.keyboard_input_active:
                self.keyboard_input_active = True
                self.keyboard_buffer = ""
            self.keyboard_buffer += chr(event.getKey())

    def mouse_event(self, event):
        """Handle mouse movement for ortho tracking.
        
        Parameters
        ----------
        event: coin.SoLocation2Event  
            The mouse movement event from the 3D view
        """
        pos = event.getPosition()
        view = Gui.ActiveDocument.ActiveView
        point = view.getPoint(pos)
        self.last_cursor_pos = point
        
        if self.tracker:
            self.tracker.update(point)

class Draft_Ortho_Track(Draft_Snap_Base):
    """GuiCommand for the Draft_Ortho_Track tool."""

    def GetResources(self):
        shortcut = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetString("Draft_Ortho_Tracking_Shortcut")
        return {"Pixmap": "Draft_Snap_Ortho",
                "Accel": shortcut,
                "MenuText": QT_TRANSLATE_NOOP("Draft_Ortho_Track", "Orthogonal tracking"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_Ortho_Track", "Helps you draw orthogonal lines from the previous point"),
                "CmdType": "ForEdit"}

    def Activated(self):
        if hasattr(Gui, "Snapper"):
            if not hasattr(Gui.Snapper, "ortho_tracking"):
                from draftguitools.gui_tool_utils import init_ortho_tracking
                init_ortho_tracking()
            if Gui.Snapper.ortho_tracking.active:
                disable_ortho_tracking()
            else:
                enable_ortho_tracking()

Gui.addCommand('Draft_Ortho_Track', Draft_Ortho_Track())

## @}

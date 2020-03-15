# ***************************************************************************
# *   (c) 2009 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2010 Ken Cline <cline@frii.com>                                   *
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *   (c) 2020 Carlo Pavan                                                  *
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

"""This module provides the Draft Snap Gui commands.
"""
## @package gui_base
# \ingroup DRAFT
# \brief This module provides the Draft Snap Gui commands.

import FreeCADGui as Gui
from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtGui


# UTILITIES -----------------------------------------------------------------

def get_snap_toolbar():
    mw = Gui.getMainWindow()
    if mw:
        toolbar = mw.findChild(QtGui.QToolBar,"Draft Snap")
        if toolbar:
            return toolbar
    return None

def sync_snap_toolbar_button(button, status):
    snap_toolbar = get_snap_toolbar()
    if snap_toolbar:
        for a in snap_toolbar.actions():
            if a.objectName() == button:
                a.setChecked(status)
                if a.isChecked():
                    a.setToolTip(a.toolTip().replace("OFF","ON"))
                else:
                    a.setToolTip(a.toolTip().replace("ON","OFF"))

def sync_snap_statusbar_button(button, status):
    mw = Gui.getMainWindow()
    if mw:
        sb = mw.statusBar()
        if sb:
            ortho_button = sb.findChild(QtGui.QAction,button)
            if ortho_button:
                ortho_button.setChecked(status)

# SNAP TOOLS ----------------------------------------------------------------

class Draft_Snap_Lock():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Lock',
                'Accel' : "Shift+S",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Lock", "Toggle On/Off"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Lock", "Activates/deactivates all snap tools at once")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Lock')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Lock"+"_Button", status)
            snap_toolbar = get_snap_toolbar()
            if snap_toolbar:
                snap_toolbar.actions()[0].setChecked(status)
                for a in snap_toolbar.actions()[1:]:
                    a.setEnabled(status)

class Draft_Snap_Near():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Near',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Near", "Nearest"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Near", "Snaps to nearest point on edges")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Near')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Near"+"_Button", status)


class Draft_Snap_Midpoint():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Midpoint',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Midpoint", "Midpoint"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Midpoint", "Snaps to midpoints of edges")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Midpoint')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Midpoint"+"_Button", status)

class Draft_Snap_Perpendicular():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Perpendicular',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Perpendicular", "Perpendicular"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Perpendicular", "Snaps to perpendicular points on edges")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Perpendicular')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Perpendicular"+"_Button", status)

class Draft_Snap_Grid():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Grid',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Grid", "Grid"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Grid", "Snaps to grid points")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Grid')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Grid"+"_Button", status)

class Draft_Snap_Intersection():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Intersection',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Intersection", "Intersection"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Intersection", "Snaps to edges intersections")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Intersection')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Intersection"+"_Button", status)

class Draft_Snap_Parallel():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Parallel',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Parallel", "Parallel"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Parallel", "Snaps to parallel directions of edges")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Parallel')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Parallel"+"_Button", status)

class Draft_Snap_Endpoint():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Endpoint',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Endpoint", "Endpoint"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Endpoint", "Snaps to endpoints of edges")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Endpoint')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Endpoint"+"_Button", status)

class Draft_Snap_Angle():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Angle',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Angle", "Angles"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Angle", "Snaps to 45 and 90 degrees points on arcs and circles")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Angle')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Angle"+"_Button", status)

class Draft_Snap_Center():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Center',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Center", "Center"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Center", "Snaps to center of circles and arcs")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Center')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Center"+"_Button", status)

class Draft_Snap_Extension():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Extension',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Extension", "Extension"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Extension", "Snaps to extension of edges")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Extension')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Extension"+"_Button", status)

class Draft_Snap_Ortho():
    """
    Toggle Snap Ortho (snap_index = 10)
    """
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Ortho',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Ortho", "Ortho"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Ortho", "Snaps to orthogonal and 45 degrees directions")}
    
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Ortho')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Ortho"+"_Button", status)
            sync_snap_statusbar_button("OrthoButton", status)

class Draft_Snap_Special():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Special',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Special", "Special"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Special", "Snaps to special locations of objects")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Special')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Special"+"_Button", status)

class Draft_Snap_Dimensions():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Dimensions',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Dimensions", "Dimensions"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_Dimensions", "Shows temporary dimensions when snapping to Arch objects")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('Dimensions')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Dimensions"+"_Button", status)
            sync_snap_statusbar_button("DimButton", status)

class Draft_Snap_WorkingPlane():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_WorkingPlane',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_WorkingPlane", "Working Plane"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Snap_WorkingPlane", "Restricts the snapped point to the current working plane")}
    def Activated(self):
        if hasattr(Gui,"Snapper"):
            # toggle the corresponding snap_index in Preferences/Mod/Draft/snapModes
            status = Gui.Snapper.toggle_snap('WorkingPlane')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_WorkingPlane"+"_Button", status)


# REGISTER SNAP GUI COMMANDS ------------------------------------------------
Gui.addCommand('Draft_Snap_Lock',Draft_Snap_Lock())
Gui.addCommand('Draft_Snap_Near',Draft_Snap_Near())
Gui.addCommand('Draft_Snap_Midpoint',Draft_Snap_Midpoint())
Gui.addCommand('Draft_Snap_Perpendicular',Draft_Snap_Perpendicular())
Gui.addCommand('Draft_Snap_Grid',Draft_Snap_Grid())
Gui.addCommand('Draft_Snap_Intersection',Draft_Snap_Intersection())
Gui.addCommand('Draft_Snap_Parallel',Draft_Snap_Parallel())
Gui.addCommand('Draft_Snap_Endpoint',Draft_Snap_Endpoint())
Gui.addCommand('Draft_Snap_Angle',Draft_Snap_Angle())
Gui.addCommand('Draft_Snap_Center',Draft_Snap_Center())
Gui.addCommand('Draft_Snap_Extension',Draft_Snap_Extension())
Gui.addCommand('Draft_Snap_Ortho',Draft_Snap_Ortho())
Gui.addCommand('Draft_Snap_Special',Draft_Snap_Special())
Gui.addCommand('Draft_Snap_Dimensions',Draft_Snap_Dimensions())
Gui.addCommand('Draft_Snap_WorkingPlane',Draft_Snap_WorkingPlane())

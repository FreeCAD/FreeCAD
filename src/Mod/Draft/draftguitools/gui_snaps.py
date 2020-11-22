# ***************************************************************************
# *   (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>                  *
# *   (c) 2009, 2010 Ken Cline <cline@frii.com>                             *
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
import PySide.QtGui as QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import draftguitools.gui_base as gui_base

from draftutils.translate import _tr


# UTILITIES -----------------------------------------------------------------


def get_snap_statusbar_widget():
    """Return snap statusbar button."""
    mw = Gui.getMainWindow()
    if mw:
        sb = mw.statusBar()
        if sb:
            return sb.findChild(QtGui.QToolBar,"draft_snap_widget")
    return None


def sync_snap_toolbar_button(button, status):
    """Set snap toolbar button to given state."""
    snap_toolbar = Gui.Snapper.get_snap_toolbar()
    if not snap_toolbar:
        return
    for a in snap_toolbar.actions():
        if a.objectName() == button:
            if button == "Draft_Snap_Lock_Button":
                # for lock button
                snap_toolbar.actions()[0].setChecked(status)
                for a in snap_toolbar.actions()[1:]:
                    if a.objectName()[:10] == "Draft_Snap":
                        a.setEnabled(status)
            else:
                # for every other button
                a.setChecked(status)
                if a.isChecked():
                    a.setToolTip(a.toolTip().replace("OFF","ON"))
                else:
                    a.setToolTip(a.toolTip().replace("ON","OFF"))


def sync_snap_statusbar_button(button, status):
    """Set snap statusbar button to given state."""
    ssw = get_snap_statusbar_widget()
    if not ssw:
        return
    for child in ssw.children():
        if child.objectName() == "Snap_Statusbutton":
            ssb = child
    actions = []
    for a in ssb.menu().actions() + ssw.children()[-6:]:
        actions.append(a)

    if button == "Draft_Snap_Lock_Statusbutton":
        ssb.setChecked(status)
        for a in actions[1:]:
            if a.objectName()[:10] == "Draft_Snap":
                a.setEnabled(status)
    else:
        for a in actions:
            if a.objectName() == button:
                a.setChecked(status)


# SNAP GUI TOOLS ------------------------------------------------------------


class Draft_Snap_Lock(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Lock tool.

    Activate or deactivate all snap methods at once.
    """

    def __init__(self):
        super(Draft_Snap_Lock, self).__init__(name=_tr("Main toggle snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Main snapping toggle On/Off"
        _tip = ("Activates or deactivates "
                "all snap methods at once.")

        return {'Pixmap': 'Snap_Lock',
                'Accel': "Shift+S",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Lock", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Lock", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Lock, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Lock')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Lock"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Lock"+"_Statusbutton", status)


Gui.addCommand('Draft_Snap_Lock', Draft_Snap_Lock())


class Draft_Snap_Midpoint(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Midpoint tool.

    Set snapping to the midpoint of an edge.
    """

    def __init__(self):
        super(Draft_Snap_Midpoint, self).__init__(name=_tr("Midpoint snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Midpoint"
        _tip = "Set snapping to the midpoint of an edge."

        return {'Pixmap': 'Snap_Midpoint',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Midpoint", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Midpoint", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Midpoint, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Midpoint')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Midpoint"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Midpoint_Statusbutton", status)


Gui.addCommand('Draft_Snap_Midpoint', Draft_Snap_Midpoint())


class Draft_Snap_Perpendicular(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Perpendicular tool.

    Set snapping to a direction that is perpendicular to an edge.
    """

    def __init__(self):
        super(Draft_Snap_Perpendicular, self).__init__(name=_tr("Perpendicular snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Perpendicular"
        _tip = "Set snapping to a direction that is perpendicular to an edge."

        return {'Pixmap': 'Snap_Perpendicular',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Perpendicular",
                                              _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Perpendicular",
                                             _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Perpendicular, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Perpendicular')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Perpendicular"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Perpendicular_Statusbutton", status)


Gui.addCommand('Draft_Snap_Perpendicular', Draft_Snap_Perpendicular())


class Draft_Snap_Grid(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Grid tool.

    Set snapping to the intersection of grid lines.
    """

    def __init__(self):
        super(Draft_Snap_Grid, self).__init__(name=_tr("Grid snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = "Set snapping to the intersection of grid lines."

        return {'Pixmap': 'Snap_Grid',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Grid", "Grid"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Grid", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Grid, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Grid')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Grid"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Grid_Statusbutton", status)


Gui.addCommand('Draft_Snap_Grid', Draft_Snap_Grid())


class Draft_Snap_Intersection(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Intersection tool.

    Set snapping to the intersection of edges.
    """

    def __init__(self):
        super(Draft_Snap_Intersection, self).__init__(name=_tr("Intersection snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Intersection"
        _tip = "Set snapping to the intersection of edges."

        return {'Pixmap': 'Snap_Intersection',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Intersection",
                                              _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Intersection",
                                             _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Intersection, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Intersection')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Intersection"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Intersection_Statusbutton", status)


Gui.addCommand('Draft_Snap_Intersection', Draft_Snap_Intersection())


class Draft_Snap_Parallel(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Parallel tool.

    Set snapping to a direction that is parallel to an edge.
    """

    def __init__(self):
        super(Draft_Snap_Parallel, self).__init__(name=_tr("Parallel snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Parallel"
        _tip = "Set snapping to a direction that is parallel to an edge."

        return {'Pixmap': 'Snap_Parallel',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Parallel", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Parallel", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Parallel, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Parallel')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Parallel"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Parallel_Statusbutton", status)


Gui.addCommand('Draft_Snap_Parallel', Draft_Snap_Parallel())


class Draft_Snap_Endpoint(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Endpoint tool.

    Set snapping to endpoints of an edge.
    """

    def __init__(self):
        super(Draft_Snap_Endpoint, self).__init__(name=_tr("Endpoint snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Endpoint"
        _tip = "Set snapping to endpoints of an edge."

        return {'Pixmap': 'Snap_Endpoint',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Endpoint", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Endpoint", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Endpoint, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Endpoint')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Endpoint"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Endpoint_Statusbutton", status)


Gui.addCommand('Draft_Snap_Endpoint', Draft_Snap_Endpoint())


class Draft_Snap_Angle(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Angle tool.

    Set snapping to points in a circular arc located at multiples
    of 30 and 45 degree angles.
    """

    def __init__(self):
        super(Draft_Snap_Angle, self).__init__(name=_tr("Angle snap (30 and 45 degrees)"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Angles (30 and 45 degrees)"
        _tip = ("Set snapping to points in a circular arc located "
                "at multiples of 30 and 45 degree angles.")

        return {'Pixmap': 'Snap_Angle',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Angle", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Angle", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Angle, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Angle')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Angle"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Angle_Statusbutton", status)


Gui.addCommand('Draft_Snap_Angle', Draft_Snap_Angle())


class Draft_Snap_Center(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Center tool.

    Set snapping to the center of a circular arc.
    """

    def __init__(self):
        super(Draft_Snap_Center, self).__init__(name=_tr("Arc center snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = "Set snapping to the center of a circular arc."

        return {'Pixmap': 'Snap_Center',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Center", "Center"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Center", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Center, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Center')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Center"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Center_Statusbutton", status)


Gui.addCommand('Draft_Snap_Center', Draft_Snap_Center())


class Draft_Snap_Extension(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Extension tool.

    Set snapping to the extension of an edge.
    """

    def __init__(self):
        super(Draft_Snap_Extension, self).__init__(name=_tr("Edge extension snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Extension"
        _tip = "Set snapping to the extension of an edge."

        return {'Pixmap': 'Snap_Extension',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Extension", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Extension", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Extension, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Extension')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Extension"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Extension_Statusbutton", status)


Gui.addCommand('Draft_Snap_Extension', Draft_Snap_Extension())


class Draft_Snap_Near(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Near tool.

    Set snapping to the nearest point of an edge.
    """

    def __init__(self):
        super(Draft_Snap_Near, self).__init__(name=_tr("Near snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = "Set snapping to the nearest point of an edge."

        return {'Pixmap': 'Snap_Near',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Near", "Nearest"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Near", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Near, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Near')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Near"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Near_Statusbutton", status)


Gui.addCommand('Draft_Snap_Near', Draft_Snap_Near())


class Draft_Snap_Ortho(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Ortho tool.

    Set snapping to a direction that is a multiple of 45 degrees
    from a point.
    """

    def __init__(self):
        super(Draft_Snap_Ortho, self).__init__(name=_tr("Orthogonal snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Orthogonal angles (45 degrees)"
        _tip = ("Set snapping to a direction that is a multiple "
                "of 45 degrees from a point.")

        return {'Pixmap': 'Snap_Ortho',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Ortho", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Ortho", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Ortho, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Ortho')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Ortho"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Ortho"+"_Statusbutton", status)


Gui.addCommand('Draft_Snap_Ortho', Draft_Snap_Ortho())


class Draft_Snap_Special(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Special tool.

    Set snapping to the special points defined inside an object.
    """

    def __init__(self):
        super(Draft_Snap_Special, self).__init__(name=_tr("Special point snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Special"
        _tip = "Set snapping to the special points defined inside an object."

        return {'Pixmap': 'Snap_Special',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Special", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Special", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Special, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Special')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Special"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Special_Statusbutton", status)


Gui.addCommand('Draft_Snap_Special', Draft_Snap_Special())


class Draft_Snap_Dimensions(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_Dimensions tool.

    Show temporary linear dimensions when editing an object
    and using other snapping methods.
    """

    def __init__(self):
        super(Draft_Snap_Dimensions, self).__init__(name=_tr("Dimension display"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Show dimensions"
        _tip = ("Show temporary linear dimensions when editing an object "
                "and using other snapping methods.")

        return {'Pixmap': 'Snap_Dimensions',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Dimensions", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Dimensions", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Dimensions, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Dimensions')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Dimensions"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Dimensions"+"_Statusbutton", status)


Gui.addCommand('Draft_Snap_Dimensions', Draft_Snap_Dimensions())


class Draft_Snap_WorkingPlane(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Snap_WorkingPlane tool.

    Restricts snapping to a point in the current working plane.
    If you select a point outside the working plane, for example,
    by using other snapping methods, it will snap to that point's
    projection in the current working plane.
    """

    def __init__(self):
        super(Draft_Snap_WorkingPlane, self).__init__(name=_tr("Working plane snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Working plane"
        _tip = ("Restricts snapping to a point in the current "
                "working plane.\n"
                "If you select a point outside the working plane, "
                "for example, by using other snapping methods,\n"
                "it will snap to that point's projection "
                "in the current working plane.")

        return {'Pixmap': 'Snap_WorkingPlane',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_WorkingPlane",
                                              _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_WorkingPlane",
                                             _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_WorkingPlane, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('WorkingPlane')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_WorkingPlane"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_WorkingPlane_Statusbutton", status)


Gui.addCommand('Draft_Snap_WorkingPlane', Draft_Snap_WorkingPlane())


class ShowSnapBar(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_ShowSnapBar tool.

    Show the snap toolbar if it is hidden.
    """

    def __init__(self):
        super(ShowSnapBar, self).__init__(name=_tr("Show snap toolbar"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = "Show the snap toolbar if it is hidden."

        return {'Pixmap': 'Draft_Snap',
                'MenuText': QT_TRANSLATE_NOOP("Draft_ShowSnapBar",
                                              "Show snap toolbar"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_ShowSnapBar",
                                             _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(ShowSnapBar, self).Activated()

        if hasattr(Gui, "Snapper"):
            Gui.Snapper.show()


Gui.addCommand('Draft_ShowSnapBar', ShowSnapBar())

## @}

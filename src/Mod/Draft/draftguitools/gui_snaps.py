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

from draftutils.messages import _msg, _log
from draftutils.translate import translate


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

    # Setting the snap lock button enables or disables all of the other snap actions:
    if button == "Draft_Snap_Lock_Button":
        snap_toolbar.actions()[0].setChecked(status) # Snap lock must be the first button
        for action in snap_toolbar.actions()[1:]:
            if action.objectName()[:10] == "Draft_Snap":
                action.setEnabled(status)

    # All other buttons only affect themselves
    else:
        for action in snap_toolbar.actions():
            if action.objectName() == button:
                action.setChecked(status)
                if action.isChecked():
                    action.setToolTip(action.toolTip().replace("OFF","ON"))
                else:
                    action.setToolTip(action.toolTip().replace("ON","OFF"))
                return


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
                return


# SNAP GUI TOOLS ------------------------------------------------------------

class Draft_Snap_Base():
    def __init__(self, name="None"):
        self.command_name = name

    def IsActive(self):
        return True

    def Activated(self):
        _log("GuiCommand: {}".format(self.command_name))
        #_msg("{}".format(16*"-"))
        #_msg("GuiCommand: {}".format(self.command_name))


class Draft_Snap_Lock(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Lock tool.

    Activate or deactivate all snap methods at once.
    """

    def __init__(self):
        super(Draft_Snap_Lock, self).__init__(name=translate("draft","Main toggle snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Lock',
                'Accel': "Shift+S",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Lock", "Main snapping toggle On/Off"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Lock", "Activates or deactivates all snap methods at once.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Lock, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Lock')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Lock"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Lock"+"_Statusbutton", status)


Gui.addCommand('Draft_Snap_Lock', Draft_Snap_Lock())


class Draft_Snap_Midpoint(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Midpoint tool.

    Set snapping to the midpoint of an edge.
    """

    def __init__(self):
        super(Draft_Snap_Midpoint, self).__init__(name=translate("draft","Midpoint snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Midpoint',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Midpoint", "Midpoint"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Midpoint", "Set snapping to the midpoint of an edge.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Midpoint, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Midpoint')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Midpoint"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Midpoint_Statusbutton", status)


Gui.addCommand('Draft_Snap_Midpoint', Draft_Snap_Midpoint())


class Draft_Snap_Perpendicular(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Perpendicular tool.

    Set snapping to a direction that is perpendicular to an edge.
    """

    def __init__(self):
        super(Draft_Snap_Perpendicular, self).__init__(name=translate("draft","Perpendicular snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Perpendicular',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Perpendicular", "Perpendicular"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Perpendicular", "Set snapping to a direction that is perpendicular to an edge.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Perpendicular, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Perpendicular')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Perpendicular"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Perpendicular_Statusbutton", status)


Gui.addCommand('Draft_Snap_Perpendicular', Draft_Snap_Perpendicular())


class Draft_Snap_Grid(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Grid tool.

    Set snapping to the intersection of grid lines.
    """

    def __init__(self):
        super(Draft_Snap_Grid, self).__init__(name=translate("draft","Grid snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Grid',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Grid", "Grid"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Grid", "Set snapping to the intersection of grid lines.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Grid, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Grid')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Grid"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Grid_Statusbutton", status)


Gui.addCommand('Draft_Snap_Grid', Draft_Snap_Grid())


class Draft_Snap_Intersection(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Intersection tool.

    Set snapping to the intersection of edges.
    """

    def __init__(self):
        super(Draft_Snap_Intersection, self).__init__(name=translate("draft","Intersection snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Intersection',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Intersection","Intersection"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Intersection","Set snapping to the intersection of edges.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Intersection, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Intersection')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Intersection"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Intersection_Statusbutton", status)


Gui.addCommand('Draft_Snap_Intersection', Draft_Snap_Intersection())


class Draft_Snap_Parallel(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Parallel tool.

    Set snapping to a direction that is parallel to an edge.
    """

    def __init__(self):
        super(Draft_Snap_Parallel, self).__init__(name=translate("draft","Parallel snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Parallel',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Parallel", "Parallel"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Parallel", "Set snapping to a direction that is parallel to an edge.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Parallel, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Parallel')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Parallel"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Parallel_Statusbutton", status)


Gui.addCommand('Draft_Snap_Parallel', Draft_Snap_Parallel())


class Draft_Snap_Endpoint(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Endpoint tool.

    Set snapping to endpoints of an edge.
    """

    def __init__(self):
        super(Draft_Snap_Endpoint, self).__init__(name=translate("draft","Endpoint snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Endpoint',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Endpoint", "Endpoint"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Endpoint", "Set snapping to endpoints of an edge.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Endpoint, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Endpoint')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Endpoint"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Endpoint_Statusbutton", status)


Gui.addCommand('Draft_Snap_Endpoint', Draft_Snap_Endpoint())


class Draft_Snap_Angle(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Angle tool.

    Set snapping to points in a circular arc located at multiples
    of 30 and 45 degree angles.
    """

    def __init__(self):
        super(Draft_Snap_Angle, self).__init__(name=translate("draft","Angle snap (30 and 45 degrees)"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Angle',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Angle", "Angle"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Angle", "Set snapping to points in a circular arc located at multiples of 30 and 45 degree angles.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Angle, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Angle')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Angle"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Angle_Statusbutton", status)


Gui.addCommand('Draft_Snap_Angle', Draft_Snap_Angle())


class Draft_Snap_Center(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Center tool.

    Set snapping to the center of a circular arc.
    """

    def __init__(self):
        super(Draft_Snap_Center, self).__init__(name=translate("draft","Arc center snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Center',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Center", "Center"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Center", "Set snapping to the center of a circular arc.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Center, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Center')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Center"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Center_Statusbutton", status)


Gui.addCommand('Draft_Snap_Center', Draft_Snap_Center())


class Draft_Snap_Extension(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Extension tool.

    Set snapping to the extension of an edge.
    """

    def __init__(self):
        super(Draft_Snap_Extension, self).__init__(name=translate("draft","Edge extension snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Extension',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Extension", "Extension"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Extension", "Set snapping to the extension of an edge.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Extension, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Extension')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Extension"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Extension_Statusbutton", status)


Gui.addCommand('Draft_Snap_Extension', Draft_Snap_Extension())


class Draft_Snap_Near(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Near tool.

    Set snapping to the nearest point of an edge.
    """

    def __init__(self):
        super(Draft_Snap_Near, self).__init__(name=translate("draft","Near snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Near',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Near", "Nearest"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Near", "Set snapping to the nearest point of an edge.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Near, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Near')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Near"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Near_Statusbutton", status)


Gui.addCommand('Draft_Snap_Near', Draft_Snap_Near())


class Draft_Snap_Ortho(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Ortho tool.

    Set snapping to a direction that is a multiple of 45 degrees
    from a point.
    """

    def __init__(self):
        super(Draft_Snap_Ortho, self).__init__(name=translate("draft","Orthogonal snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Ortho',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Ortho", "Orthogonal"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Ortho", "Set snapping to a direction that is a multiple of 45 degrees from a point.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Ortho, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Ortho')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Ortho"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Ortho"+"_Statusbutton", status)


Gui.addCommand('Draft_Snap_Ortho', Draft_Snap_Ortho())


class Draft_Snap_Special(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Special tool.

    Set snapping to the special points defined inside an object.
    """

    def __init__(self):
        super(Draft_Snap_Special, self).__init__(name=translate("draft","Special point snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Special',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Special", "Special"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Special", "Set snapping to the special points defined inside an object.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Special, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Special')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Special"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Special_Statusbutton", status)


Gui.addCommand('Draft_Snap_Special', Draft_Snap_Special())


class Draft_Snap_Dimensions(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_Dimensions tool.

    Show temporary linear dimensions when editing an object
    and using other snapping methods.
    """

    def __init__(self):
        super(Draft_Snap_Dimensions, self).__init__(name=translate("draft","Dimension display"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_Dimensions',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_Dimensions", "Show dimensions"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_Dimensions", "Show temporary linear dimensions when editing an object and using other snapping methods.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_Dimensions, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('Dimensions')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_Dimensions"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_Dimensions"+"_Statusbutton", status)


Gui.addCommand('Draft_Snap_Dimensions', Draft_Snap_Dimensions())


class Draft_Snap_WorkingPlane(Draft_Snap_Base):
    """GuiCommand for the Draft_Snap_WorkingPlane tool.

    Restricts snapping to a point in the current working plane.
    If you select a point outside the working plane, for example,
    by using other snapping methods, it will snap to that point's
    projection in the current working plane.
    """

    def __init__(self):
        super(Draft_Snap_WorkingPlane, self).__init__(name=translate("draft","Working plane snap"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Snap_WorkingPlane',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Snap_WorkingPlane","Working plane"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Snap_WorkingPlane","Restricts snapping to a point in the current working plane.\nIf you select a point outside the working plane, for example, by using other snapping methods,\nit will snap to that point's projection in the current working plane.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_Snap_WorkingPlane, self).Activated()

        if hasattr(Gui, "Snapper"):
            status = Gui.Snapper.toggle_snap('WorkingPlane')
            # change interface consistently
            sync_snap_toolbar_button("Draft_Snap_WorkingPlane"+"_Button", status)
            sync_snap_statusbar_button("Draft_Snap_WorkingPlane_Statusbutton", status)


Gui.addCommand('Draft_Snap_WorkingPlane', Draft_Snap_WorkingPlane())


class ShowSnapBar(Draft_Snap_Base):
    """GuiCommand for the Draft_ShowSnapBar tool.

    Show the snap toolbar if it is hidden.
    """

    def __init__(self):
        super(ShowSnapBar, self).__init__(name=translate("draft","Show snap toolbar"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Snap',
                'MenuText': QT_TRANSLATE_NOOP("Draft_ShowSnapBar","Show snap toolbar"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_ShowSnapBar","Show the snap toolbar if it is hidden.")}

    def Activated(self):
        """Execute when the command is called."""
        super(ShowSnapBar, self).Activated()

        if hasattr(Gui, "Snapper"):
            Gui.Snapper.show()


Gui.addCommand('Draft_ShowSnapBar', ShowSnapBar())

## @}

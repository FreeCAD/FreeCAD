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
"""Provides GUI tools to set different modes of other tools.

For example, a construction mode, a continue mode to repeat commands,
and to toggle the appearance of certain shapes to wireframe.
"""
## @package gui_togglemodes
# \ingroup draftguitools
# \brief Provides GUI tools to set different modes of other tools.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_base as gui_base

from draftutils.messages import _msg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class BaseMode(gui_base.GuiCommandSimplest):
    """Base class for mode context GuiCommands.

    This is inherited by the other GuiCommand classes to run
    a set of similar actions when changing modes.

    It inherits `GuiCommandSimplest` to set up the document
    and other behavior. See this class for more information.
    """

    def Activated(self, mode="None"):
        """Execute when the command is called.

        Parameters
        ----------
        action: str
            Indicates the type of mode to switch to.
            It can be `'construction'` or `'continue'`.
        """
        super(BaseMode, self).Activated()

        if hasattr(Gui, "draftToolBar"):
            _ui = Gui.draftToolBar
        else:
            _msg(translate("draft","No active Draft Toolbar."))
            return

        if _ui is not None:
            if mode == "construction" and hasattr(_ui, "constrButton"):
                _ui.constrButton.toggle()
            elif mode == "continue":
                _ui.toggleContinue()


class ToggleConstructionMode(BaseMode):
    """GuiCommand for the Draft_ToggleConstructionMode tool.

    When construction mode is active, the following objects created
    will be included in the construction group, and will be drawn
    with the specified color and properties.
    """

    def __init__(self):
        super(ToggleConstructionMode,
              self).__init__(name=translate("draft","Construction mode"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        d = {'Pixmap': 'Draft_Construction',
             'MenuText': QT_TRANSLATE_NOOP("Draft_ToggleConstructionMode","Toggle construction mode"),
             'Accel': "C, M",
             'ToolTip': QT_TRANSLATE_NOOP("Draft_ToggleConstructionMode","Toggles the Construction mode.\nWhen this is active, the following objects created will be included in the construction group, and will be drawn with the specified color and properties.")}
        return d

    def Activated(self):
        """Execute when the command is called.

        It calls the `toggle()` method of the construction button
        in the `DraftToolbar` class.
        """
        super(ToggleConstructionMode, self).Activated(mode="construction")


Gui.addCommand('Draft_ToggleConstructionMode', ToggleConstructionMode())


class ToggleContinueMode(BaseMode):
    """GuiCommand for the Draft_ToggleContinueMode tool.

    When continue mode is active, any drawing tool that is terminated
    will automatically start again. This can be used to draw several
    objects one after the other in succession.
    """

    def __init__(self):
        super(ToggleContinueMode, self).__init__(name=translate("draft","Continue mode"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        d = {'Pixmap': 'Draft_Continue',
             'MenuText': QT_TRANSLATE_NOOP("Draft_ToggleContinueMode","Toggle continue mode"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_ToggleContinueMode","Toggles the Continue mode.\nWhen this is active, any drawing tool that is terminated will automatically start again.\nThis can be used to draw several objects one after the other in succession.")}
        return d

    def Activated(self):
        """Execute when the command is called.

        It calls the `toggleContinue()` method of the `DraftToolbar` class.
        """
        super(ToggleContinueMode, self).Activated(mode="continue")


Gui.addCommand('Draft_ToggleContinueMode', ToggleContinueMode())


class ToggleDisplayMode(gui_base.GuiCommandNeedsSelection):
    """GuiCommand for the Draft_ToggleDisplayMode tool.

    Switches the display mode of selected objects from flatlines
    to wireframe and back.

    It inherits `GuiCommandNeedsSelection` to only be available
    when there is a document and a selection.
    See this class for more information.
    """

    def __init__(self):
        super(ToggleDisplayMode,
              self).__init__(name=translate("draft","Toggle display mode"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        d = {'Pixmap': 'Draft_SwitchMode',
             'Accel': "Shift+Space",
             'MenuText': QT_TRANSLATE_NOOP("Draft_ToggleDisplayMode","Toggle normal/wireframe display"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_ToggleDisplayMode","Switches the display mode of selected objects from flatlines to wireframe and back.\nThis is helpful to quickly visualize objects that are hidden by other objects.\nThis is intended to be used with closed shapes and solids, and doesn't affect open wires.")}
        return d

    def Activated(self):
        """Execute when the command is called.

        It tests the view provider of the selected objects
        and changes their `DisplayMode` from `'Wireframe'`
        to `'Flat Lines'`, and the other way around, if possible.
        """
        super(ToggleDisplayMode, self).Activated()

        for obj in Gui.Selection.getSelection():
            if obj.ViewObject.DisplayMode == "Flat Lines":
                if "Wireframe" in obj.ViewObject.listDisplayModes():
                    obj.ViewObject.DisplayMode = "Wireframe"
            elif obj.ViewObject.DisplayMode == "Wireframe":
                if "Flat Lines" in obj.ViewObject.listDisplayModes():
                    obj.ViewObject.DisplayMode = "Flat Lines"


Gui.addCommand('Draft_ToggleDisplayMode', ToggleDisplayMode())

## @}

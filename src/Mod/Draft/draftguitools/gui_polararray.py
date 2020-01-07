"""This module provides the Draft PolarArray tool.
"""
## @package gui_polararray
# \ingroup DRAFT
# \brief This module provides the Draft PolarArray tool.

# ***************************************************************************
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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

import FreeCAD as App
import FreeCADGui as Gui
import Draft
import DraftGui
import Draft_rc
from . import gui_base
from drafttaskpanels import task_polararray


if App.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
    # import DraftTools
    from DraftGui import translate
    # from DraftGui import displayExternal
    from pivy import coin
else:
    def QT_TRANSLATE_NOOP(context, text):
        return text

    def translate(context, text):
        return text


def _tr(text):
    """Function to translate with the context set"""
    return translate("Draft", text)


# So the resource file doesn't trigger errors from code checkers (flake8)
True if Draft_rc.__name__ else False


class GuiCommandPolarArray(gui_base.GuiCommandBase):
    """Gui command for the PolarArray tool"""

    def __init__(self):
        super().__init__()
        self.command_name = "PolarArray"
        self.location = None
        self.mouse_event = None
        self.view = None
        self.callback_move = None
        self.callback_click = None
        self.ui = None
        self.point = App.Vector()

    def GetResources(self):
        _msg = ("Creates copies of a selected object, "
                "and places the copies in a polar pattern.\n"
                "The properties of the array can be further modified after "
                "the new object is created, including turning it into "
                "a different type of array.")
        d = {'Pixmap': 'Draft_PolarArray',
             'MenuText': QT_TRANSLATE_NOOP("Draft", "Polar array"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft", _msg)}
        return d

    def Activated(self):
        """This is called when the command is executed.

        We add callbacks that connect the 3D view with
        the widgets of the task panel.
        """
        self.location = coin.SoLocation2Event.getClassTypeId()
        self.mouse_event = coin.SoMouseButtonEvent.getClassTypeId()
        self.view = Draft.get3DView()
        self.callback_move = \
            self.view.addEventCallbackPivy(self.location, self.move)
        self.callback_click = \
            self.view.addEventCallbackPivy(self.mouse_event, self.click)

        self.ui = task_polararray.TaskPanelPolarArray()
        # The calling class (this one) is saved in the object
        # of the interface, to be able to call a function from within it.
        self.ui.source_command = self
        # Gui.Control.showDialog(self.ui)
        DraftGui.todo.delay(Gui.Control.showDialog, self.ui)

    def move(self, event_cb):
        """This is a callback for when the mouse pointer moves in the 3D view.

        It should automatically update the coordinates in the widgets
        of the task panel.
        """
        event = event_cb.getEvent()
        mousepos = event.getPosition().getValue()
        ctrl = event.wasCtrlDown()
        self.point = Gui.Snapper.snap(mousepos, active=ctrl)
        if self.ui:
            self.ui.display_point(self.point)

    def click(self, event_cb=None):
        """This is a callback for when the mouse pointer clicks on the 3D view.

        It should act as if the Enter key was pressed, or the OK button
        was pressed in the task panel.
        """
        if event_cb:
            event = event_cb.getEvent()
            if (event.getState() != coin.SoMouseButtonEvent.DOWN
                    or event.getButton() != coin.SoMouseButtonEvent.BUTTON1):
                return
        if self.ui and self.point:
            # The accept function of the interface
            # should call the completed function
            # of the calling class (this one).
            self.ui.accept()

    def completed(self):
        """This is called when the command is terminated.

        We should remove the callbacks that were added to the 3D view
        and then close the task panel.
        """
        self.view.removeEventCallbackPivy(self.location,
                                          self.callback_move)
        self.view.removeEventCallbackPivy(self.mouse_event,
                                          self.callback_click)
        if Gui.Control.activeDialog():
            Gui.Snapper.off()
            Gui.Control.closeDialog()
            super().finish()


if App.GuiUp:
    Gui.addCommand('Draft_PolarArray', GuiCommandPolarArray())

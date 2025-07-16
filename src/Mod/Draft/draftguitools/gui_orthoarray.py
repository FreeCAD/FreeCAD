# ***************************************************************************
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
"""Provides GUI tools to create orthogonal Array objects."""
## @package gui_orthoarray
# \ingroup draftguitools
# \brief Provides GUI tools to create orthogonal Array objects.

## \addtogroup draftguitools
# @{
from pivy import coin
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
from draftguitools import gui_base
from draftutils import gui_utils
from drafttaskpanels import task_orthoarray


class OrthoArray(gui_base.GuiCommandBase):
    """Gui command for the OrthoArray tool."""

    def __init__(self):
        super().__init__(name="OrthoArray")
        # self.location = None
        self.mouse_event = None
        # self.callback_move = None
        self.callback_click = None
        self.ui = None
        self.point = App.Vector()

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"Pixmap": "Draft_Array",
                "MenuText": QT_TRANSLATE_NOOP("Draft_OrthoArray", "Array"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_OrthoArray", "Creates copies of the selected object, and places the copies in an orthogonal pattern,\nmeaning the copies follow the specified direction in the X, Y, Z axes.\n\nThe array can be turned into a polar or a circular array by changing its type.")}

    def Activated(self):
        """Execute when the command is called.

        We add callbacks that connect the 3D view with
        the widgets of the task panel.
        """
        super().Activated()

        # self.location = coin.SoLocation2Event.getClassTypeId()
        self.mouse_event = coin.SoMouseButtonEvent.getClassTypeId()
        # self.callback_move = \
        #     self.view.addEventCallbackPivy(self.location, self.move)
        self.callback_click = \
            self.view.addEventCallbackPivy(self.mouse_event, self.click)

        self.ui = task_orthoarray.TaskPanelOrthoArray()
        # The calling class (this one) is saved in the object
        # of the interface, to be able to call a function from within it.
        self.ui.source_command = self
        task = Gui.Control.showDialog(self.ui)
        task.setDocumentName(Gui.ActiveDocument.Document.Name)
        task.setAutoCloseOnDeletedDocument(True)

    def click(self, event_cb=None):
        """Execute as a callback when the pointer clicks on the 3D view.

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
        """Execute when the command is terminated.

        We should remove the callbacks that were added to the 3D view
        and then close the task panel.
        """
        try:
            self.view.removeEventCallbackPivy(self.mouse_event, self.callback_click)
            gui_utils.end_all_events()
        except RuntimeError:
            # the view has been deleted already
            pass
        self.callback_click = None
        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()
        self.finish()


Gui.addCommand('Draft_OrthoArray', OrthoArray())

## @}

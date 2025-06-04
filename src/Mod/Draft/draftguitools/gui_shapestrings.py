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
"""Provides GUI tools to create text shapes with a particular font.

These text shapes are made of various edges and closed faces, and therefore
can be extruded to create solid bodies that can be used in boolean
operations. That is, these text shapes can be used for engraving text
into solid bodies.

They are more complex that simple text annotations.
"""
## @package gui_shapestrings
# \ingroup draftguitools
# \brief Provides GUI tools to create text shapes with a particular font.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
from draftguitools import gui_base
from draftutils import gui_utils
from draftutils.messages import _toolmsg
from draftutils.translate import translate
from drafttaskpanels import task_shapestring


class ShapeString(gui_base.GuiCommandBase):
    """Gui command for the ShapeString tool."""

    def __init__(self):
        super().__init__(name="ShapeString")

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {"Pixmap": "Draft_ShapeString",
                "MenuText": QT_TRANSLATE_NOOP("Draft_ShapeString", "Shape from text"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_ShapeString", "Creates a shape from a text string by choosing a specific font and a placement.\nThe closed shapes can be used for extrusions and boolean operations.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated()
        self.ui = task_shapestring.ShapeStringTaskPanelCmd(self)
        self.call = self.view.addEventCallback("SoEvent", self.ui.action)
        _toolmsg(translate("draft", "Pick ShapeString location point"))
        task = Gui.Control.showDialog(self.ui)
        task.setDocumentName(Gui.ActiveDocument.Document.Name)
        task.setAutoCloseOnDeletedDocument(True)

    def finish(self):
        try:
            self.view.removeEventCallback("SoEvent", self.call)
            gui_utils.end_all_events()
        except RuntimeError:
            # the view has been deleted already
            pass
        self.call = None
        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()
        super().finish()


Gui.addCommand('Draft_ShapeString', ShapeString())

## @}

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
"""Provides GUI tools to create Clone objects.

The clone is basically a simple copy of the `Shape` of another object,
whether that is a Draft object or any other 3D object.

The Clone's `Shape` can be scaled in size in any direction.

This implementation was developed before the `App::Link` object was created.
In many cases using `App::Link` makes more sense, as this object is
more memory efficient as it reuses the same internal `Shape`
instead of creating a copy of it.
"""
## @package gui_clone
# \ingroup draftguitools
# \brief Provides GUI tools to create Clone objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftutils.todo as todo
from draftutils.messages import _msg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Clone(gui_base_original.Modifier):
    """Gui Command for the Clone tool."""

    def __init__(self):
        super(Clone, self).__init__()
        self.moveAfterCloning = False

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Clone',
                'Accel': "C,L",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Clone", "Clone"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Clone", "Creates a clone of the selected objects.\nThe resulting clone can be scaled in each of its three directions.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Clone, self).Activated(name="Clone")
        if not Gui.Selection.getSelection():
            if self.ui:
                self.ui.selectUi(on_close_call=self.finish)
                _msg(translate("draft", "Select an object to clone"))
                self.call = self.view.addEventCallback(
                    "SoEvent",
                    gui_tool_utils.selectObject)
        else:
            self.proceed()

    def proceed(self):
        """Proceed with the command if one object was selected."""
        if Gui.Selection.getSelection():
            sels = len(Gui.Selection.getSelection())
            Gui.addModule("Draft")
            App.ActiveDocument.openTransaction(translate("Draft", "Clone"))
            nonRepeatList = []
            n = 0
            for obj in Gui.Selection.getSelection():
                if obj not in nonRepeatList:
                    _cmd = "Draft.make_clone"
                    _cmd += "("
                    _cmd += "FreeCAD.ActiveDocument."
                    _cmd += 'getObject("' + obj.Name + '")'
                    _cmd += ")"
                    Gui.doCommand("c" + str(n) + " = " + _cmd)
                    nonRepeatList.append(obj)
                    n += 1
            App.ActiveDocument.commitTransaction()
            App.ActiveDocument.recompute()
            Gui.Selection.clearSelection()

            objects = App.ActiveDocument.Objects
            for i in range(sels):
                Gui.Selection.addSelection(objects[-(1 + i)])
        self.finish()

    def finish(self):
        """Terminate the operation of the tool."""
        super(Clone, self).finish()
        if self.moveAfterCloning:
            todo.ToDo.delay(Gui.runCommand, "Draft_Move")


Draft_Clone = Clone
Gui.addCommand('Draft_Clone', Clone())

## @}

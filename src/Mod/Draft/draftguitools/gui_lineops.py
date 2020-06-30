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
"""Provides GUI tools to do certain line operations.

These GuiCommands aren't really used anymore, as the same actions
are called from the task panel interface by other methods.
"""
## @package gui_lineops
# \ingroup draftguitools
# \brief Provides GUI tools to do certain line operations.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_base as gui_base

from draftutils.messages import _msg
from draftutils.translate import _tr

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class LineAction(gui_base.GuiCommandSimplest):
    """Base class for Line context GuiCommands.

    This is inherited by the other GuiCommand classes to run
    a set of similar actions when editing a line, wire, spline,
    or bezier curve.

    It inherits `GuiCommandSimplest` to set up the document
    and other behavior. See this class for more information.
    """

    def Activated(self, action="None"):
        """Execute when the command is called.

        Parameters
        ----------
        action: str
            Indicates the type of action to perform with the line object.
            It can be `'finish'`, `'close'`, or `'undo'`.
        """
        if hasattr(App, "activeDraftCommand"):
            _command = App.activeDraftCommand
        else:
            _msg(_tr("No active command."))
            return

        if (_command is not None
                and _command.featureName in ("Line", "Polyline",
                                             "BSpline", "BezCurve",
                                             "CubicBezCurve")):
            if action == "finish":
                _command.finish(False)
            elif action == "close":
                _command.finish(True)
            elif action == "undo":
                _command.undolast()


class FinishLine(LineAction):
    """GuiCommand to finish any running line drawing operation."""

    def __init__(self):
        super(FinishLine, self).__init__(name=_tr("Finish line"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = "Finishes a line without closing it."

        d = {'Pixmap': 'Draft_Finish',
             'MenuText': QT_TRANSLATE_NOOP("Draft_FinishLine", "Finish line"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_FinishLine", _tip),
             'CmdType': 'ForEdit'}
        return d

    def Activated(self):
        """Execute when the command is called.

        It calls the `finish(False)` method of the active Draft command.
        """
        super(FinishLine, self).Activated(action="finish")


Gui.addCommand('Draft_FinishLine', FinishLine())


class CloseLine(LineAction):
    """GuiCommand to close the line being drawn and finish the operation."""

    def __init__(self):
        super(CloseLine, self).__init__(name=_tr("Close line"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = "Closes the line being drawn, and finishes the operation."

        d = {'Pixmap': 'Draft_Lock',
             'MenuText': QT_TRANSLATE_NOOP("Draft_CloseLine", "Close Line"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_CloseLine", _tip),
             'CmdType': 'ForEdit'}
        return d

    def Activated(self):
        """Execute when the command is called.

        It calls the `finish(True)` method of the active Draft command.
        """
        super(CloseLine, self).Activated(action="close")


Gui.addCommand('Draft_CloseLine', CloseLine())


class UndoLine(LineAction):
    """GuiCommand to undo the last drawn segment of a line."""

    def __init__(self):
        super(UndoLine, self).__init__(name=_tr("Undo line"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = "Undoes the last drawn segment of the line being drawn."

        d = {'Pixmap': 'Draft_Rotate',
             'MenuText': QT_TRANSLATE_NOOP("Draft_UndoLine",
                                           "Undo last segment"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_UndoLine", _tip),
             'CmdType': 'ForEdit'}
        return d

    def Activated(self):
        """Execute when the command is called.

        It calls the `undolast` method of the active Draft command.
        """
        super(UndoLine, self).Activated(action="undo")


Gui.addCommand('Draft_UndoLine', UndoLine())

## @}

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
"""Provides GUI tools to create simple Point objects.

A point is just a simple vertex with a position in 3D space.

Its visual properties can be changed, like display size on screen
and color.
"""
## @package gui_points
# \ingroup draftguitools
# \brief Provides GUI tools to create simple Point objects.

## \addtogroup draftguitools
# @{
import pivy.coin as coin
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
import draftguitools.gui_base_original as gui_base_original
import draftutils.todo as todo

from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Point(gui_base_original.Creator):
    """Gui Command for the Point tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Point',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Point", "Point"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Point", "Creates a point object. Click anywhere on the 3D view.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated(name="Point")
        self.stack = []
        if self.ui:
            self.ui.pointUi(title=translate("draft", self.featureName), icon="Draft_Point")
            self.ui.isRelative.hide()
            self.ui.continueCmd.show()
        # adding 2 callback functions
        self.callbackClick = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), self.click)
        self.callbackMove = self.view.addEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), self.move)

    def move(self, event_cb):
        """Execute as a callback when the pointer moves in the 3D view.

        It should automatically update the coordinates in the widgets
        of the task panel.
        """
        event = event_cb.getEvent()
        mousepos = event.getPosition().getValue()
        ctrl = event.wasCtrlDown()
        self.point = Gui.Snapper.snap(mousepos, active=ctrl)
        if self.ui:
            self.ui.displayPoint(self.point)

    def numericInput(self, numx, numy, numz):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.point = App.Vector(numx, numy, numz)
        self.click()

    def click(self, event_cb=None):
        """Execute as a callback when the pointer clicks on the 3D view.

        It should act as if the Enter key was pressed, or the OK button
        was pressed in the task panel.
        """
        if event_cb:
            event = event_cb.getEvent()
            if (event.getState() != coin.SoMouseButtonEvent.DOWN or
                event.getButton() != event.BUTTON1):
                return
        if self.point:
            self.stack.append(self.point)
            if len(self.stack) == 1:
                # The command to run is built as a series of text strings
                # to be committed through the `draftutils.todo.ToDo` class.
                commitlist = []
                Gui.addModule("Draft")
                if utils.getParam("UsePartPrimitives", False):
                    # Insert a Part::Primitive object
                    _cmd = 'FreeCAD.ActiveDocument.'
                    _cmd += 'addObject("Part::Vertex", "Point")'
                    _cmd_list = ['point = ' + _cmd,
                                 'point.X = ' + str(self.stack[0][0]),
                                 'point.Y = ' + str(self.stack[0][1]),
                                 'point.Z = ' + str(self.stack[0][2]),
                                 'Draft.autogroup(point)',
                                 'FreeCAD.ActiveDocument.recompute()']
                    commitlist.append((translate("draft", "Create Point"),
                                       _cmd_list))
                else:
                    # Insert a Draft point
                    _cmd = 'Draft.make_point'
                    _cmd += '('
                    _cmd += str(self.stack[0][0]) + ', '
                    _cmd += str(self.stack[0][1]) + ', '
                    _cmd += str(self.stack[0][2])
                    _cmd += ')'
                    _cmd_list = ['point = ' + _cmd,
                                 'Draft.autogroup(point)',
                                 'FreeCAD.ActiveDocument.recompute()']
                    commitlist.append((translate("draft", "Create Point"),
                                       _cmd_list))
                todo.ToDo.delayCommit(commitlist)
                Gui.Snapper.off()
            self.finish(cont=None)

    def finish(self, cont=False):
        """Terminate the operation.

        Parameters
        ----------
        cont: bool or None, optional
            Restart (continue) the command if `True`, or if `None` and
            `ui.continueMode` is `True`.
        """
        super().finish()
        if self.callbackClick:
            self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), self.callbackClick)
        if self.callbackMove:
            self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), self.callbackMove)
        if cont or (cont is None and self.ui and self.ui.continueMode):
            self.Activated()


Gui.addCommand('Draft_Point', Point())

## @}

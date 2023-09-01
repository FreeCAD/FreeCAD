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
"""Provides GUI tools to create mirrored objects.

The mirror tool creates a `Part::Mirroring` object, which is the same
as the one created by the Part module.

Perhaps in the future a specific Draft `Mirror` object can be defined.
"""
## @package gui_mirror
# \ingroup draftguitools
# \brief Provides GUI tools to create mirrored objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import DraftGeomUtils
import DraftVecUtils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_trackers as trackers
import draftguitools.gui_tool_utils as gui_tool_utils

from draftutils.messages import _msg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Mirror(gui_base_original.Modifier):
    """Gui Command for the Mirror tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Mirror',
                'Accel': "M, I",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Mirror", "Mirror"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Mirror", "Mirrors the selected objects along a line defined by two points.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Mirror, self).Activated(name="Mirror")
        self.ghost = None
        if self.ui:
            if not Gui.Selection.getSelection():
                self.ui.selectUi(on_close_call=self.finish)
                _msg(translate("draft", "Select an object to mirror"))
                self.call = \
                    self.view.addEventCallback("SoEvent",
                                               gui_tool_utils.selectObject)
            else:
                self.proceed()

    def proceed(self):
        """Proceed with the command if one object was selected."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)

        self.sel = Gui.Selection.getSelection()
        self.ui.pointUi(title=translate("draft", self.featureName), icon="Draft_Mirror")
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.ghost = trackers.ghostTracker(self.sel, mirror=True)
        self.call = self.view.addEventCallback("SoEvent", self.action)
        _msg(translate("draft", "Pick start point of mirror line"))
        self.ui.isCopy.hide()

    def finish(self, cont=False):
        """Terminate the operation of the tool."""
        if self.ghost:
            self.ghost.finalize()
        super(Mirror, self).finish()

    def mirror(self, p1, p2, copy=False):
        """Mirror the real shapes."""
        sel = '['
        for o in self.sel:
            if len(sel) > 1:
                sel += ', '
            sel += 'FreeCAD.ActiveDocument.' + o.Name
        sel += ']'
        Gui.addModule("Draft")
        _cmd = 'Draft.mirror'
        _cmd += '('
        _cmd += sel + ', '
        _cmd += DraftVecUtils.toString(p1) + ', '
        _cmd += DraftVecUtils.toString(p2)
        _cmd += ')'
        _cmd_list = ['m = ' + _cmd,
                     'FreeCAD.ActiveDocument.recompute()']
        self.commit(translate("draft", "Mirror"),
                    _cmd_list)

    def action(self, arg):
        """Handle the 3D scene events.

        This is installed as an EventCallback in the Inventor view.

        Parameters
        ----------
        arg: dict
            Dictionary with strings that indicates the type of event received
            from the 3D view.
        """
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":  # mouse movement detection
            self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
            if len(self.node) > 0:
                last = self.node[-1]
                if self.ghost:
                    tol = 1e-7
                    if self.point.sub(last).Length > tol:
                        # The normal of the mirror plane must be same as in mirror.py.
                        nor = self.point.sub(last).cross(self.wp.axis)
                        if nor.Length > tol:
                            nor.normalize()
                            mtx = DraftGeomUtils.mirror_matrix(App.Matrix(), last, nor)
                            self.ghost.setMatrix(mtx) # Ignores the position of the matrix.
                            self.ghost.move(App.Vector(mtx.col(3)[:3]))
            if self.extendedCopy:
                if not gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT):
                    self.finish()
            gui_tool_utils.redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    self.ui.redraw()
                    if (self.node == []):
                        self.node.append(self.point)
                        self.ui.isRelative.show()
                        if self.ghost:
                            self.ghost.on()
                        _msg(translate("draft",
                                       "Pick end point of mirror line"))
                        if self.planetrack:
                            self.planetrack.set(self.point)
                    else:
                        last = self.node[0]
                        if (self.ui.isCopy.isChecked()
                                or gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT)):
                            self.mirror(last, self.point, True)
                        else:
                            self.mirror(last, self.point)
                        if gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT):
                            self.extendedCopy = True
                        else:
                            self.finish()

    def numericInput(self, numx, numy, numz):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.point = App.Vector(numx, numy, numz)
        if not self.node:
            self.node.append(self.point)
            if self.ghost:
                self.ghost.on()
            _msg(translate("draft", "Pick end point of mirror line"))
        else:
            last = self.node[-1]
            if self.ui.isCopy.isChecked():
                self.mirror(last, self.point, True)
            else:
                self.mirror(last, self.point)
            self.finish()


Gui.addCommand('Draft_Mirror', Mirror())

## @}

# ***************************************************************************
# *   (c) 2009 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2010 Ken Cline <cline@frii.com>                                   *
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
"""Provides GUI tools to create Rectangle objects."""
## @package gui_rectangles
# \ingroup draftguitools
# \brief Provides GUI tools to create Rectangle objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import DraftVecUtils
import draftutils.utils as utils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers

from draftutils.messages import _msg, _err
from draftutils.translate import translate


class Rectangle(gui_base_original.Creator):
    """Gui command for the Rectangle tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Rectangle',
                'Accel': "R, E",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Rectangle", "Rectangle"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Rectangle", "Creates a 2-point rectangle. CTRL to snap.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated(name="Rectangle")
        if self.ui:
            self.refpoint = None
            self.ui.pointUi(title=translate("draft", "Rectangle"), icon="Draft_Rectangle")
            self.ui.extUi()
            if utils.getParam("UsePartPrimitives", False):
                self.fillstate = self.ui.hasFill.isChecked()
                self.ui.hasFill.setChecked(True)
            self.call = self.view.addEventCallback("SoEvent", self.action)
            self.rect = trackers.rectangleTracker()
            _msg(translate("draft", "Pick first point"))

    def finish(self, cont=False):
        """Terminate the operation.

        Parameters
        ----------
        cont: bool or None, optional
            Restart (continue) the command if `True`, or if `None` and
            `ui.continueMode` is `True`.
        """
        super().finish()
        if self.ui:
            if hasattr(self, "fillstate"):
                self.ui.hasFill.setChecked(self.fillstate)
                del self.fillstate
            self.rect.off()
            self.rect.finalize()
        if cont or (cont is None and self.ui and self.ui.continueMode):
            self.Activated()

    def createObject(self):
        """Create the final object in the current document."""
        p1 = self.node[0]
        p3 = self.node[-1]
        diagonal = p3.sub(p1)
        p2 = p1.add(DraftVecUtils.project(diagonal, self.wp.v))
        p4 = p1.add(DraftVecUtils.project(diagonal, self.wp.u))
        length = p4.sub(p1).Length
        if abs(DraftVecUtils.angle(p4.sub(p1), self.wp.u, self.wp.axis)) > 1:
            length = -length
        height = p2.sub(p1).Length
        if abs(DraftVecUtils.angle(p2.sub(p1), self.wp.v, self.wp.axis)) > 1:
            height = -height
        try:
            # The command to run is built as a series of text strings
            # to be committed through the `draftutils.todo.ToDo` class.
            rot, sup, pts, fil = self.getStrings()
            base = p1
            if length < 0:
                length = -length
                base = base.add((p1.sub(p4)).negative())
            if height < 0:
                height = -height
                base = base.add((p1.sub(p2)).negative())
            Gui.addModule("Draft")
            if utils.getParam("UsePartPrimitives", False):
                # Insert a Part::Primitive object
                _cmd = 'FreeCAD.ActiveDocument.'
                _cmd += 'addObject("Part::Plane", "Plane")'
                _cmd_list = ['plane = ' + _cmd,
                             'plane.Length = ' + str(length),
                             'plane.Width = ' + str(height),
                             'pl = FreeCAD.Placement()',
                             'pl.Rotation.Q=' + rot,
                             'pl.Base = ' + DraftVecUtils.toString(base),
                             'plane.Placement = pl',
                             'Draft.autogroup(plane)',
                             'FreeCAD.ActiveDocument.recompute()']
                self.commit(translate("draft", "Create Plane"),
                            _cmd_list)
            else:
                _cmd = 'Draft.make_rectangle'
                _cmd += '('
                _cmd += 'length=' + str(length) + ', '
                _cmd += 'height=' + str(height) + ', '
                _cmd += 'placement=pl, '
                _cmd += 'face=' + fil + ', '
                _cmd += 'support=' + sup
                _cmd += ')'
                _cmd_list = ['pl = FreeCAD.Placement()',
                             'pl.Rotation.Q = ' + rot,
                             'pl.Base = ' + DraftVecUtils.toString(base),
                             'rec = ' + _cmd,
                             'Draft.autogroup(rec)',
                             'FreeCAD.ActiveDocument.recompute()']
                self.commit(translate("draft", "Create Rectangle"),
                            _cmd_list)
        except Exception:
            _err("Draft: error delaying commit")
        self.finish(cont=None)

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
            self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg, noTracker=True)
            self.rect.update(self.point)
            gui_tool_utils.redraw3DView()
        elif (arg["Type"] == "SoMouseButtonEvent"
              and arg["State"] == "DOWN"
              and arg["Button"] == "BUTTON1"):

            if arg["Position"] == self.pos:
                self.finish(cont=None)
                return

            if (not self.node) and (not self.support):
                gui_tool_utils.getSupport(arg)
                self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg, noTracker=True)
            if self.point:
                self.ui.redraw()
                self.pos = arg["Position"]
                self.appendPoint(self.point)

    def numericInput(self, numx, numy, numz):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.point = App.Vector(numx, numy, numz)
        self.appendPoint(self.point)

    def appendPoint(self, point):
        """Append a point to the list of nodes."""
        self.node.append(point)
        if len(self.node) > 1:
            self.rect.update(point)
            self.createObject()
        else:
            _msg(translate("draft", "Pick opposite point"))
            self.ui.setRelative()
            self.rect.setorigin(point)
            self.rect.on()
            if self.planetrack:
                self.planetrack.set(point)


Gui.addCommand('Draft_Rectangle', Rectangle())

## @}

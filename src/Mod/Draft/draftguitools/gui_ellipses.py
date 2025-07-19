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
"""Provides GUI tools to create Ellipse objects."""
## @package gui_ellipses
# \ingroup draftguitools
# \brief Provides GUI tools to create Ellipse objects.

## \addtogroup draftguitools
# @{
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import DraftVecUtils
from draftguitools import gui_base_original
from draftguitools import gui_tool_utils
from draftguitools import gui_trackers as trackers
from draftutils import params
from draftutils import utils
from draftutils.messages import _err, _toolmsg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Ellipse(gui_base_original.Creator):
    """Gui command for the Ellipse tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Ellipse',
                'Accel': "E, L",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Ellipse", "Ellipse"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Ellipse", "Creates an ellipse.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated(name="Ellipse")
        if self.ui:
            self.refpoint = None
            self.ui.pointUi(title=translate("draft", "Ellipse"), icon="Draft_Ellipse")
            self.ui.extUi()
            self.call = self.view.addEventCallback("SoEvent", self.action)
            self.rect = trackers.rectangleTracker()
            _toolmsg(translate("draft", "Pick first point"))

    def finish(self, cont=False):
        """Terminate the operation.

        Parameters
        ----------
        cont: bool or None, optional
            Restart (continue) the command if `True`, or if `None` and
            `ui.continueMode` is `True`.
        """
        self.end_callbacks(self.call)
        if self.ui:
            self.rect.off()
            self.rect.finalize()
        super().finish()
        if cont or (cont is None and self.ui and self.ui.continueMode):
            self.Activated()

    def createObject(self):
        """Create the actual object in the current document."""
        p1 = self.node[0]
        p3 = self.node[-1]
        diagonal = p3.sub(p1)
        halfdiag = App.Vector(diagonal).multiply(0.5)
        center = p1.add(halfdiag)
        p2 = p1.add(DraftVecUtils.project(diagonal, self.wp.v))
        p4 = p1.add(DraftVecUtils.project(diagonal, self.wp.u))
        r1 = (p4.sub(p1).Length)/2
        r2 = (p2.sub(p1).Length)/2
        try:
            # The command to run is built as a series of text strings
            # to be committed through the `draftutils.todo.ToDo` class.
            rot, sup, pts, fil = self.getStrings()
            if r2 > r1:
                r1, r2 = r2, r1
                m = App.Matrix()
                m.rotateZ(math.pi/2)
                rot1 = App.Rotation()
                rot1.Q = eval(rot)
                rot2 = App.Placement(m)
                rot2 = rot2.Rotation
                rot = str((rot1.multiply(rot2)).Q)
            Gui.addModule("Draft")
            if params.get_param("UsePartPrimitives"):
                # Insert a Part::Primitive object
                _cmd = 'FreeCAD.ActiveDocument.'
                _cmd += 'addObject("Part::Ellipse", "Ellipse")'
                _cmd_list = ['ellipse = ' + _cmd,
                             'ellipse.MajorRadius = ' + str(r1),
                             'ellipse.MinorRadius = ' + str(r2),
                             'pl = FreeCAD.Placement()',
                             'pl.Rotation.Q= ' + rot,
                             'pl.Base = ' + DraftVecUtils.toString(center),
                             'ellipse.Placement = pl',
                             'Draft.autogroup(ellipse)',
                             'Draft.select(ellipse)',
                             'FreeCAD.ActiveDocument.recompute()']
                self.commit(translate("draft", "Create Ellipse"),
                            _cmd_list)
            else:
                # Insert a Draft ellipse
                _cmd = 'Draft.make_ellipse'
                _cmd += '('
                _cmd += str(r1) + ', ' + str(r2) + ', '
                _cmd += 'placement=pl, '
                _cmd += 'face=' + fil + ', '
                _cmd += 'support=' + sup
                _cmd += ')'
                _cmd_list = ['pl = FreeCAD.Placement()',
                             'pl.Rotation.Q = ' + rot,
                             'pl.Base = ' + DraftVecUtils.toString(center),
                             'ellipse = ' + _cmd,
                             'Draft.autogroup(ellipse)',
                             'FreeCAD.ActiveDocument.recompute()']
                self.commit(translate("draft", "Create Ellipse"),
                            _cmd_list)
        except Exception:
            _err("Draft: Error: Unable to create object.")
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
        elif not self.ui.mouse:
            pass
        elif arg["Type"] == "SoLocation2Event":  # mouse movement detection
            self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg, noTracker=True)
            self.rect.update(self.point)
            gui_tool_utils.redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if arg["State"] == "DOWN" and arg["Button"] == "BUTTON1":

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
        """Append the point to the list of nodes."""
        self.node.append(point)
        if len(self.node) > 1:
            self.rect.update(point)
            self.createObject()
        else:
            _toolmsg(translate("draft", "Pick opposite point"))
            self.ui.setRelative()
            self.rect.setorigin(point)
            self.rect.on()
            if self.planetrack:
                self.planetrack.set(point)


Gui.addCommand('Draft_Ellipse', Ellipse())

## @}

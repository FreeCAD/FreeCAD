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
"""Provides GUI tools to create Label objects.

Labels are similar to text annotations but include a leader line
and an arrow in order to point to an object and indicate some of its
properties.
"""
## @package gui_labels
# \ingroup draftguitools
# \brief Provides GUI tools to create Label objects.

## \addtogroup draftguitools
# @{
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import DraftVecUtils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers
import draftutils.utils as utils

from draftutils.messages import _msg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Label(gui_base_original.Creator):
    """Gui Command for the Label tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Label',
                'Accel': "D, L",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Label", "Label"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Label", "Creates a label, optionally attached to a selected object or subelement.\n\nFirst select a vertex, an edge, or a face of an object, then call this command,\nand then set the position of the leader line and the textual label.\nThe label will be able to display information about this object, and about the selected subelement,\nif any.\n\nIf many objects or many subelements are selected, only the first one in each case\nwill be used to provide information to the label.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated(name="Label")
        self.ghost = None
        self.labeltype = utils.getParam("labeltype", "Custom")
        self.sel = Gui.Selection.getSelectionEx()
        if self.sel:
            self.sel = self.sel[0]
        self.ui.labelUi(title=translate("draft",self.featureName), callback=self.setmode)
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.ghost = trackers.lineTracker()
        self.call = self.view.addEventCallback("SoEvent", self.action)
        _msg(translate("draft", "Pick target point"))
        self.ui.isCopy.hide()

    def setmode(self, i):
        """Set the type of label, if it is associated to an object."""
        from draftobjects.label import get_label_types
        self.labeltype = get_label_types()[i]
        utils.setParam("labeltype", self.labeltype)

    def finish(self, cont=False):
        """Finish the command."""
        if self.ghost:
            self.ghost.finalize()
        super().finish()

    def create(self):
        """Create the actual object."""
        if len(self.node) == 3:
            targetpoint = self.node[0]
            basepoint = self.node[2]
            v = self.node[2].sub(self.node[1])
            dist = v.Length
            h = self.wp.u
            n = self.wp.axis
            r = self.wp.getRotation().Rotation

            if abs(DraftVecUtils.angle(v, h, n)) <= math.pi/4:
                direction = "Horizontal"
                dist = -dist
            elif abs(DraftVecUtils.angle(v, h, n)) >= math.pi*3/4:
                direction = "Horizontal"
            elif DraftVecUtils.angle(v, h, n) > 0:
                direction = "Vertical"
            else:
                direction = "Vertical"
                dist = -dist

            tp = DraftVecUtils.toString(targetpoint)
            sel = None
            if self.sel:
                sel = "FreeCAD.ActiveDocument." + self.sel.Object.Name

                if self.sel.SubElementNames:
                    sub = "'" + self.sel.SubElementNames[0] + "'"
                else:
                    sub = "None"

            pl = "FreeCAD.Placement"
            pl += "("
            pl += DraftVecUtils.toString(basepoint) + ", "
            pl += "FreeCAD.Rotation" + str(r.Q)
            pl += ")"

            Gui.addModule("Draft")
            _cmd = "Draft.make_label"
            _cmd += "("
            _cmd += "target_point=" + tp + ", "
            _cmd += "placement=" + pl + ", "
            if sel:
                _cmd += "target_object=" + sel + ", "
                _cmd += "subelements=" + sub + ", "
            _cmd += "label_type=" + "'" + self.labeltype + "'" + ", "
            # _cmd += "custom_text=" + "'Label'" + ", "
            _cmd += "direction=" + "'" + direction + "'" + ", "
            _cmd += "distance=" + str(dist)
            _cmd += ")"

            # Commit the creation instructions through the parent class,
            # the Creator class
            _cmd_list = ['_label_ = ' + _cmd,
                         'Draft.autogroup(_label_)',
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Create Label"),
                        _cmd_list)
        self.finish()

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
        elif arg["Type"] == "SoLocation2Event":
            if hasattr(Gui, "Snapper"):
                Gui.Snapper.affinity = None  # don't keep affinity
            if len(self.node) == 2:
                gui_tool_utils.setMod(arg, gui_tool_utils.MODCONSTRAIN, True)
            self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
            gui_tool_utils.redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    self.ui.redraw()
                    if not self.node:
                        # first click
                        self.node.append(self.point)
                        self.ui.isRelative.show()
                        _msg(translate("draft",
                                       "Pick endpoint of leader line"))
                        if self.planetrack:
                            self.planetrack.set(self.point)
                    elif len(self.node) == 1:
                        # second click
                        self.node.append(self.point)
                        if self.ghost:
                            self.ghost.p1(self.node[0])
                            self.ghost.p2(self.node[1])
                            self.ghost.on()
                        _msg(translate("draft", "Pick text position"))
                    else:
                        # third click
                        self.node.append(self.point)
                        self.create()

    def numericInput(self, numx, numy, numz):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.point = App.Vector(numx, numy, numz)
        if not self.node:
            # first click
            self.node.append(self.point)
            self.ui.isRelative.show()
            _msg(translate("draft", "Pick endpoint of leader line"))
            if self.planetrack:
                self.planetrack.set(self.point)
        elif len(self.node) == 1:
            # second click
            self.node.append(self.point)
            if self.ghost:
                self.ghost.p1(self.node[0])
                self.ghost.p2(self.node[1])
                self.ghost.on()
            _msg(translate("draft", "Pick text position"))
        else:
            # third click
            self.node.append(self.point)
            self.create()


Draft_Label = Label
Gui.addCommand('Draft_Label', Label())

## @}

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

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import DraftVecUtils
import draftutils.utils as utils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftutils.todo as todo

from drafttaskpanels.task_shapestring import ShapeStringTaskPanelCmd
from draftutils.translate import translate
from draftutils.messages import _msg, _err

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class ShapeString(gui_base_original.Creator):
    """Gui command for the ShapeString tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        d = {'Pixmap': 'Draft_ShapeString',
             'MenuText': QT_TRANSLATE_NOOP("Draft_ShapeString", "Shape from text"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_ShapeString", "Creates a shape from a text string by choosing a specific font and a placement.\nThe closed shapes can be used for extrusions and boolean operations.")}
        return d

    def Activated(self):
        """Execute when the command is called."""
        super(ShapeString, self).Activated(name="ShapeString")
        if self.ui:
            self.ui.sourceCmd = self
            self.taskmode = utils.getParam("UiMode", 1)
            if self.taskmode:
                # This doesn't use the task panel defined in DraftGui
                # so it is deleted and a new task panel is installed
                try:
                    del self.task
                except AttributeError:
                    pass
                self.task = ShapeStringTaskPanelCmd(self)
                self.call = self.view.addEventCallback("SoEvent", self.task.action)
                _msg(translate("draft", "Pick ShapeString location point"))
                todo.ToDo.delay(Gui.Control.showDialog, self.task)
            else:
                self.dialog = None
                self.text = ''
                self.ui.sourceCmd = self
                self.ui.pointUi(title=translate("draft",self.featureName),
                                icon="Draft_ShapeString")
                self.active = True
                self.call = self.view.addEventCallback("SoEvent", self.action)
                self.ssBase = None
                self.ui.xValue.setFocus()
                self.ui.xValue.selectAll()
                _msg(translate("draft", "Pick ShapeString location point"))
                Gui.draftToolBar.show()

    def createObject(self):
        """Create the actual object in the current document."""
        # print("debug: D_T ShapeString.createObject type(self.SString):"
        #       + str(type(self.SString)))

        dquote = '"'
        String = dquote + self.SString + dquote

        # Size and tracking are numbers;
        # they are ASCII so this conversion should always work
        Size = str(self.SSSize)
        Tracking = str(self.SSTrack)
        FFile = dquote + self.FFile + dquote

        try:
            qr, sup, points, fil = self.getStrings()
            Gui.addModule("Draft")
            _cmd = 'Draft.make_shapestring'
            _cmd += '('
            _cmd += 'String=' + String + ', '
            _cmd += 'FontFile=' + FFile + ', '
            _cmd += 'Size=' + Size + ', '
            _cmd += 'Tracking=' + Tracking
            _cmd += ')'
            _cmd_list = ['ss = ' + _cmd,
                         'plm = FreeCAD.Placement()',
                         'plm.Base = ' + DraftVecUtils.toString(self.ssBase),
                         'plm.Rotation.Q = ' + qr,
                         'ss.Placement = plm',
                         'ss.Support = ' + sup,
                         'Draft.autogroup(ss)',
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Create ShapeString"),
                        _cmd_list)
        except Exception:
            _err("Draft_ShapeString: error delaying commit")
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
        elif arg["Type"] == "SoLocation2Event":  # mouse movement detection
            if self.active:
                (self.point,
                 ctrlPoint, info) = gui_tool_utils.getPoint(self, arg,
                                                            noTracker=True)
            gui_tool_utils.redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if not self.ssBase:
                    self.ssBase = self.point
                    self.active = False
                    Gui.Snapper.off()
                    self.ui.SSUi()

    def numericInput(self, numx, numy, numz):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.ssBase = App.Vector(numx, numy, numz)
        self.ui.SSUi()  # move on to next step in parameter entry

    def numericSSize(self, ssize):
        """Validate the size in the user interface.

        This function is called by the toolbar or taskpanel interface
        when a valid size parameter has been entered in the input field.
        """
        self.SSSize = ssize
        self.ui.STrackUi()

    def numericSTrack(self, strack):
        """Validate the tracking value in the user interface.

        This function is called by the toolbar or taskpanel interface
        when a valid tracking value has been entered in the input field.
        """
        self.SSTrack = strack
        self.ui.SFileUi()

    def validSString(self, sstring):
        """Validate the string value in the user interface.

        This function is called by the toolbar or taskpanel interface
        when a valid string value has been entered in the input field.
        """
        self.SString = sstring
        self.ui.SSizeUi()

    def validFFile(self, FFile):
        """Validate the font file value in the user interface.

        This function is called by the toolbar or taskpanel interface
        when a valid font file value has been entered in the input field.
        """
        self.FFile = FFile
        # last step in ShapeString parm capture, create object
        self.createObject()

    def finish(self, finishbool=False):
        """Terminate the operation."""
        super(ShapeString, self).finish()
        if self.ui:
            # del self.dialog  # what does this do??
            if self.ui.continueMode:
                self.Activated()


Gui.addCommand('Draft_ShapeString', ShapeString())

## @}

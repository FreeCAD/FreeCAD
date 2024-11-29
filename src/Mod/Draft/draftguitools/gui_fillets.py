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
"""Provides GUI tools to create Fillet objects between two lines.

TODO: Currently this tool uses the DraftGui widgets. We want to avoid using
this big module because it creates manually the interface.
Instead we should provide its own .ui file and task panel,
similar to the OrthoArray tool.
"""
## @package gui_fillet
# \ingroup draftguitools
# \brief Provides GUI tools to create Fillet objects between two lines.

## \addtogroup draftguitools
# @{
import PySide.QtCore as QtCore
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft
import Draft_rc
from draftguitools import gui_base_original
from draftguitools import gui_tool_utils
from draftmake import make_fillet
from draftutils import utils
from draftutils.messages import _err, _toolmsg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Fillet(gui_base_original.Creator):
    """Gui command for the Fillet tool."""

    def __init__(self):
        super().__init__()
        self.featureName = "Fillet"

    def IsActive(self):
        """Return True when this command should be available."""
        return bool(Gui.Selection.getSelection())

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"Pixmap": "Draft_Fillet",
                "Accel": "F,I",
                "MenuText": QT_TRANSLATE_NOOP("Draft_Fillet", "Fillet"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_Fillet", "Creates a fillet between two selected wires or edges.")}

    def Activated(self, name="Fillet"):
        """Execute when the command is called."""
        super().Activated(name=name)

        if self.ui:
            self.rad = 100
            self.chamfer = False
            self.delete = False
            label = translate("draft", "Fillet radius")
            tooltip = translate("draft", "Radius of fillet")

            # Call the task panel defined in DraftGui to enter a radius.
            self.ui.taskUi(title=translate("Draft", "Fillet"), icon="Draft_Fillet")
            self.ui.radiusUi()
            self.ui.sourceCmd = self
            self.ui.labelRadius.setText(label)
            self.ui.radiusValue.setToolTip(tooltip)
            self.ui.setRadiusValue(self.rad, "Length")
            self.ui.check_delete = self.ui._checkbox("isdelete",
                                                     self.ui.layout,
                                                     checked=self.delete)
            self.ui.check_delete.setText(translate("Draft",
                                                   "Delete original objects"))
            self.ui.check_delete.show()
            self.ui.check_chamfer = self.ui._checkbox("ischamfer",
                                                      self.ui.layout,
                                                      checked=self.chamfer)
            self.ui.check_chamfer.setText(translate("Draft",
                                                    "Create chamfer"))
            self.ui.check_chamfer.show()

            self.ui.check_delete.stateChanged.connect(self.set_delete)
            self.ui.check_chamfer.stateChanged.connect(self.set_chamfer)

            # TODO: somehow we need to set up the trackers
            # to show a preview of the fillet.

            # self.linetrack = trackers.lineTracker(dotted=True)
            # self.arctrack = trackers.arcTracker()
            # self.call = self.view.addEventCallback("SoEvent", self.action)
            _toolmsg(translate("draft", "Enter radius."))

    def action(self, arg):
        """Scene event handler. CURRENTLY NOT USED.

        Here the displaying of the trackers (previews)
        should be implemented by considering the current value of the
        `ui.radiusValue`.
        """
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
            gui_tool_utils.redraw3DView()

    def set_delete(self):
        """Execute as a callback when the delete checkbox changes."""
        self.delete = self.ui.check_delete.isChecked()

    def set_chamfer(self):
        """Execute as a callback when the chamfer checkbox changes."""
        self.chamfer = self.ui.check_chamfer.isChecked()

    def numericRadius(self, rad):
        """Validate the entry radius in the user interface.

        This function is called by the toolbar or taskpanel interface
        when a valid radius has been entered in the input field.
        """
        self.rad = rad
        self.draw_arc(rad, self.chamfer, self.delete)

    def draw_arc(self, rad, chamfer, delete):
        """Process the selection and draw the actual object."""
        sels = Gui.Selection.getSelectionEx("", 0)
        edges, _ = make_fillet._preprocess(sels, rad, chamfer)
        if edges is None:
            _err(translate("draft", "Fillet cannot be created"))
            self.finish()
            return

        Gui.addModule("Draft")

        cmd = "Draft.make_fillet(sels, radius=" + str(rad)
        if chamfer:
            cmd += ", chamfer=True"
        if delete:
            cmd += ", delete=True"
        cmd += ")"
        cmd_list = ["sels = FreeCADGui.Selection.getSelectionEx('', 0)",
                    "fillet = " + cmd,
                    "Draft.autogroup(fillet)",
                    "FreeCAD.ActiveDocument.recompute()"]

        self.commit(translate("draft", "Create fillet"), cmd_list)
        self.finish()


Gui.addCommand('Draft_Fillet', Fillet())

## @}

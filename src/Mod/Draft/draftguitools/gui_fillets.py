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
import draftutils.utils as utils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils

from draftutils.messages import _msg, _err
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Fillet(gui_base_original.Creator):
    """Gui command for the Fillet tool."""

    def __init__(self):
        super(Fillet, self).__init__()
        self.featureName = "Fillet"

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {'Pixmap': 'Draft_Fillet',
                'Accel':'F,I',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Fillet", "Fillet"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Fillet", "Creates a fillet between two selected wires or edges.")}

    def Activated(self, name="Fillet"):
        """Execute when the command is called."""
        super(Fillet, self).Activated(name=name)

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
            _msg(translate("draft","Enter radius."))

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
        _msg(translate("draft","Delete original objects:") + " " + str(self.delete))

    def set_chamfer(self):
        """Execute as a callback when the chamfer checkbox changes."""
        self.chamfer = self.ui.check_chamfer.isChecked()
        _msg(translate("draft","Chamfer mode:") + " " + str(self.chamfer))

    def numericRadius(self, rad):
        """Validate the entry radius in the user interface.

        This function is called by the toolbar or taskpanel interface
        when a valid radius has been entered in the input field.
        """
        self.rad = rad
        self.draw_arc(rad, self.chamfer, self.delete)
        self.finish()

    def draw_arc(self, rad, chamfer, delete):
        """Process the selection and draw the actual object."""
        wires = Gui.Selection.getSelection()

        if not wires or len(wires) != 2:
            _err(translate("draft","Two elements needed."))
            return

        for o in wires:
            _msg(utils.get_type(o))

        _test = translate("draft", "Test object")
        _test_off = translate("draft", "Test object removed")
        _cant = translate("draft", "Fillet cannot be created")

        _msg(4*"=" + _test)
        arc = Draft.make_fillet(wires, rad)
        if not arc:
            _err(_cant)
            return
        self.doc.removeObject(arc.Name)
        _msg(4*"=" + _test_off)

        _doc = 'FreeCAD.ActiveDocument.'

        _wires = '['
        _wires += _doc + wires[0].Name + ', '
        _wires += _doc + wires[1].Name
        _wires += ']'

        Gui.addModule("Draft")

        _cmd = 'Draft.make_fillet'
        _cmd += '('
        _cmd += _wires + ', '
        _cmd += 'radius=' + str(rad)
        if chamfer:
            _cmd += ', chamfer=' + str(chamfer)
        if delete:
            _cmd += ', delete=' + str(delete)
        _cmd += ')'
        _cmd_list = ['arc = ' + _cmd,
                     'Draft.autogroup(arc)',
                     'FreeCAD.ActiveDocument.recompute()']

        self.commit(translate("draft", "Create fillet"),
                    _cmd_list)

    def finish(self, cont=False):
        """Terminate the operation."""
        super(Fillet, self).finish()
        if self.ui:
            # self.linetrack.finalize()
            # self.arctrack.finalize()
            self.doc.recompute()


Gui.addCommand('Draft_Fillet', Fillet())

## @}

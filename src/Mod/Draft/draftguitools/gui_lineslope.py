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
"""Provides GUI tools to change the slope of a line.

It currently only works for a line in the XY plane, it changes the height
of one of its points in the Z direction to create a sloped line.
"""
## @package gui_lineslope
# \ingroup draftguitools
# \brief Provides GUI tools to change the slope of a line.

## \addtogroup draftguitools
# @{
import PySide.QtGui as QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import draftutils.utils as utils
import draftguitools.gui_base as gui_base

from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class LineSlope(gui_base.GuiCommandNeedsSelection):
    """Gui Command for the Line slope tool.

    For a line in the XY plane, it changes the height of one of its points
    to create a sloped line.

    To Do
    -----
    Make it work also with lines lying on the YZ and XZ planes,
    or in an arbitrary plane, for which the normal is known.
    """

    def __init__(self):
        super(LineSlope, self).__init__(name=translate("draft","Change slope"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Slope',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Slope", "Set slope"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Slope", "Sets the slope of the selected line by changing the value of the Z value of one of its points.\nIf a polyline is selected, it will apply the slope transformation to each of its segments.\n\nThe slope will always change the Z value, therefore this command only works well for\nstraight Draft lines that are drawn in the XY plane. Selected objects that aren't single lines will be ignored.")}

    def Activated(self):
        """Execute when the command is called."""
        super(LineSlope, self).Activated()

        # for obj in Gui.Selection.getSelection():
        #     if utils.get_type(obj) != "Wire":
        #         _msg(translate("draft",
        #                        "This tool only works with "
        #                        "Draft Lines and Wires"))
        #         return

        # TODO: create a .ui file with QtCreator and import it here
        # instead of creating the interface programmatically,
        # see the `gui_othoarray` module for an example.
        w = QtGui.QWidget()
        w.setWindowTitle(translate("Draft", "Slope"))
        layout = QtGui.QHBoxLayout(w)
        label = QtGui.QLabel(w)
        label.setText(translate("Draft", "Slope")+":")
        layout.addWidget(label)
        self.spinbox = QtGui.QDoubleSpinBox(w)
        self.spinbox.setMinimum(-9999.99)
        self.spinbox.setMaximum(9999.99)
        self.spinbox.setSingleStep(0.01)
        _tip = ("New slope of the selected lines.\n"
                "This is the tangent of the horizontal angle:\n"
                "0 = horizontal\n"
                "1 = 45 deg up\n"
                "-1 = 45deg down\n")
        label.setToolTip(translate("Draft", _tip))
        self.spinbox.setToolTip(translate("Draft", _tip))
        layout.addWidget(self.spinbox)

        # In order to display our interface inside the task panel
        # we must contain our interface inside a parent widget.
        # Then our interface must be installed in this parent widget
        # inside the attribute called "form".
        taskwidget = QtGui.QWidget()
        taskwidget.form = w

        # The "accept" attribute of the parent widget
        # should also contain a reference to a function that will be called
        # when we press the "OK" button.
        # Then we must show the container widget.
        taskwidget.accept = self.accept
        Gui.Control.showDialog(taskwidget)

    def accept(self):
        """Execute when clicking the OK button or pressing Enter key.

        It changes the slope of the line that lies on the XY plane.

        TODO: make it work also with lines lying on the YZ and XZ planes.
        """
        if hasattr(self, "spinbox"):
            pc = self.spinbox.value()
            self.doc.openTransaction("Change slope")
            for obj in Gui.Selection.getSelection():
                if utils.get_type(obj) == "Wire":
                    if len(obj.Points) > 1:
                        lp = None
                        np = []
                        for p in obj.Points:
                            if not lp:
                                lp = p
                            else:
                                v = p.sub(lp)
                                z = pc * App.Vector(v.x, v.y, 0).Length
                                lp = App.Vector(p.x, p.y, lp.z + z)
                            np.append(lp)
                        obj.Points = np
            self.doc.commitTransaction()
        Gui.Control.closeDialog()
        self.doc.recompute()


Draft_Slope = LineSlope
Gui.addCommand('Draft_Slope', LineSlope())

## @}

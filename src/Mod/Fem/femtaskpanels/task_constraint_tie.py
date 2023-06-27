# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM constraint tie task panel for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package task_constraint_tie
#  \ingroup FEM
#  \brief task panel for constraint tie object

from PySide import QtCore
from PySide import QtGui

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets


class _TaskPanel:
    """
    The TaskPanel for editing References property of FemConstraintTie objects
    """

    def __init__(self, obj):

        self.obj = obj

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ConstraintTie.ui"
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_tolerance,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.tolerance_changed
        )
        self.init_parameter_widget()

        # geometry selection widget
        self.selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References,
            ["Face"],
            False,
            False
        )

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

    def accept(self):
        # check values
        items = len(self.selectionWidget.references)
        FreeCAD.Console.PrintMessage(
            "Task panel: found references: {}\n{}\n"
            .format(items, self.selectionWidget.references)
        )

        if items != 2:
            msgBox = QtGui.QMessageBox()
            msgBox.setIcon(QtGui.QMessageBox.Question)
            msgBox.setText(
                "Constraint Tie requires exactly two faces\n\nfound references: {}"
                .format(items)
            )
            msgBox.setWindowTitle("FreeCAD FEM Constraint Tie")
            retryButton = msgBox.addButton(QtGui.QMessageBox.Retry)
            ignoreButton = msgBox.addButton(QtGui.QMessageBox.Ignore)
            msgBox.exec_()

            if msgBox.clickedButton() == retryButton:
                return False
            elif msgBox.clickedButton() == ignoreButton:
                pass
        self.obj.Tolerance = self.tolerance
        self.obj.References = self.selectionWidget.references
        self.recompute_and_set_back_all()
        return True

    def reject(self):
        self.recompute_and_set_back_all()
        return True

    def recompute_and_set_back_all(self):
        doc = FreeCADGui.getDocument(self.obj.Document)
        doc.Document.recompute()
        self.selectionWidget.setback_listobj_visibility()
        if self.selectionWidget.sel_server:
            FreeCADGui.Selection.removeObserver(self.selectionWidget.sel_server)
        doc.resetEdit()

    def init_parameter_widget(self):
        self.tolerance = self.obj.Tolerance
        self.parameterWidget.if_tolerance.setText(self.tolerance.UserString)

    def tolerance_changed(self, base_quantity_value):
        self.tolerance = base_quantity_value

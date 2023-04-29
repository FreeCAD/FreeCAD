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

__title__ = "FreeCAD FEM constraint centrif task panel for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package task_constraint_centrif
#  \ingroup FEM
#  \brief task panel for constraint centrif object

from PySide import QtCore
from PySide import QtGui

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets


class _TaskPanel:
    """
    The TaskPanel for editing References property of FemConstraintCentrif objects
    """

    def __init__(self, obj):

        self.obj = obj

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ConstraintCentrif.ui"
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_rotation_frequency,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.rotation_frequency_changed
        )
        self.init_parameter_widget()

        # axis of rotation selection widget
        self.AxisSelectionWidget = selection_widgets.GeometryElementsSelection(
            obj.RotationAxis,
            ["Edge"],
            False,
            False
        )

        # loaded body selection widget
        self.BodySelectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References,
            ["Solid"],
            False,
            False
        )

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.BodySelectionWidget, self.AxisSelectionWidget]

    def accept(self):
        # check values RotationAxis
        items = len(self.AxisSelectionWidget.references)
        FreeCAD.Console.PrintMessage(
            "Task panel: found axis references: {}\n{}\n"
            .format(items, self.AxisSelectionWidget.references)
        )

        if items != 1:
            msgBox = QtGui.QMessageBox()
            msgBox.setIcon(QtGui.QMessageBox.Question)
            msgBox.setText(
                "Constraint Centrif requires exactly one line\n\nfound references: {}"
                .format(items)
            )
            msgBox.setWindowTitle("FreeCAD FEM Constraint Centrif - Axis selection")
            retryButton = msgBox.addButton(QtGui.QMessageBox.Retry)
            ignoreButton = msgBox.addButton(QtGui.QMessageBox.Ignore)
            msgBox.exec_()

            if msgBox.clickedButton() == retryButton:
                return False
            elif msgBox.clickedButton() == ignoreButton:
                pass

        # check values BodyReference
        items = len(self.BodySelectionWidget.references)
        FreeCAD.Console.PrintMessage(
            "Task panel: found body references: {}\n{}\n"
            .format(items, self.BodySelectionWidget.references)
        )

        # if no solid is added as reference all volume elements are used
        """
        if items == 0:
            msgBox = QtGui.QMessageBox()
            msgBox.setIcon(QtGui.QMessageBox.Question)
            msgBox.setText("Constraint Centrif requires at least one solid")
            msgBox.setWindowTitle("FreeCAD FEM Constraint Centrif - Body selection")
            retryButton = msgBox.addButton(QtGui.QMessageBox.Retry)
            ignoreButton = msgBox.addButton(QtGui.QMessageBox.Ignore)
            msgBox.exec_()

            if msgBox.clickedButton() == retryButton:
                return False
            elif msgBox.clickedButton() == ignoreButton:
                pass
        """

        # check value RotationFrequency
        if self.rotation_frequency == 0:
            msgBox = QtGui.QMessageBox()
            msgBox.setIcon(QtGui.QMessageBox.Question)
            msgBox.setText("Rotational speed is zero")
            msgBox.setWindowTitle("FreeCAD FEM Constraint Centrif - Rotational speed setting")
            retryButton = msgBox.addButton(QtGui.QMessageBox.Retry)
            ignoreButton = msgBox.addButton(QtGui.QMessageBox.Ignore)
            msgBox.exec_()

            if msgBox.clickedButton() == retryButton:
                return False
            elif msgBox.clickedButton() == ignoreButton:
                pass

        self.obj.RotationFrequency = self.rotation_frequency
        self.obj.RotationAxis = self.AxisSelectionWidget.references
        self.obj.References = self.BodySelectionWidget.references
        self.recompute_and_set_back_all()
        return True

    def reject(self):
        self.recompute_and_set_back_all()
        return True

    def recompute_and_set_back_all(self):
        doc = FreeCADGui.getDocument(self.obj.Document)
        doc.Document.recompute()

        self.AxisSelectionWidget.setback_listobj_visibility()
        if self.AxisSelectionWidget.sel_server:
            FreeCADGui.Selection.removeObserver(self.AxisSelectionWidget.sel_server)

        self.BodySelectionWidget.setback_listobj_visibility()
        if self.BodySelectionWidget.sel_server:
            FreeCADGui.Selection.removeObserver(self.BodySelectionWidget.sel_server)

        doc.resetEdit()

    def init_parameter_widget(self):
        self.rotation_frequency = self.obj.RotationFrequency
        self.parameterWidget.if_rotation_frequency.setText(self.rotation_frequency.UserString)

    def rotation_frequency_changed(self, base_quantity_value):
        self.rotation_frequency = base_quantity_value

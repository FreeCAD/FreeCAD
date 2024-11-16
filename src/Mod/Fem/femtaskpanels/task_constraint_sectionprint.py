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

__title__ = "FreeCAD FEM constraint section print task panel for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package task_constraint_sectionprint
#  \ingroup FEM
#  \brief task panel for constraint section print object

from PySide import QtCore
from PySide import QtGui

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from . import base_femtaskpanel


# TODO uses the old style Add button, move to the new style. See constraint fixed
class _TaskPanel(base_femtaskpanel._BaseTaskPanel):
    """
    The TaskPanel for editing References property of ConstraintSectionPrint objects
    """

    def __init__(self, obj):
        super().__init__(obj)

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ConstraintSectionPrint.ui"
        )

        self.init_parameter_widget()

        QtCore.QObject.connect(
            self.parameterWidget.cb_variable,
            QtCore.SIGNAL("currentIndexChanged(int)"),
            self.variable_changed,
        )

        # geometry selection widget
        self.selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Face"], False, False
        )

        # form made from param and selection widget
        self.form = [self.selectionWidget, self.parameterWidget]

    def accept(self):
        # check values
        items = len(self.selectionWidget.references)

        if items != 1:
            msgBox = QtGui.QMessageBox()
            msgBox.setIcon(QtGui.QMessageBox.Question)
            msgBox.setText(
                "Constraint SectionPrint requires exactly one face\n\nfound references: {}".format(
                    items
                )
            )
            msgBox.setWindowTitle("FreeCAD FEM Constraint SectionPrint")
            retryButton = msgBox.addButton(QtGui.QMessageBox.Retry)
            ignoreButton = msgBox.addButton(QtGui.QMessageBox.Ignore)
            msgBox.exec_()

            if msgBox.clickedButton() == retryButton:
                return False
            elif msgBox.clickedButton() == ignoreButton:
                pass

        self.obj.Variable = self.variable
        self.obj.References = self.selectionWidget.references
        self.selectionWidget.finish_selection()
        return super().accept()

    def reject(self):
        self.selectionWidget.finish_selection()
        return super().reject()

    def init_parameter_widget(self):
        self.variable = self.obj.Variable
        self.variable_enum = self.obj.getEnumerationsOfProperty("Variable")
        self.parameterWidget.cb_variable.addItems(self.variable_enum)
        index = self.variable_enum.index(self.variable)
        self.parameterWidget.cb_variable.setCurrentIndex(index)

    def variable_changed(self, index):
        self.variable = self.variable_enum[index]

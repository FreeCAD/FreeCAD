# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
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

"""
https://forum.freecadweb.org/viewtopic.php?f=10&t=48427
"""

import os

from importlib import import_module

from PySide import QtCore
from PySide import QtGui

import FreeCADGui


class FemExamples(QtGui.QWidget):
    def __init__(self):
        super(FemExamples, self).__init__()
        self.init_ui()

    def __del__(self,):
        # need as fix for qt event error
        # --> see http://forum.freecadweb.org/viewtopic.php?f=18&t=10732&start=10#p86493
        return

    def init_ui(self):

        # init widgets
        self.view = QtGui.QTreeWidget()

        path = os.path.dirname(os.path.realpath(__file__))
        files = [f for f in os.listdir(str(path))]
        not_files = [
            "examplesgui.py",
            "manager.py",
            "meshes",
            "__init__.py",
            "__pycache__",
        ]

        files = [str(f) for f in files if f not in not_files]
        # Slicing the .py from every file
        files = [f[:-3] for f in files if f.endswith(".py")]
        files.sort()
        files_info = {}
        self.files_name = {}
        constraints = set()
        meshes = set()
        solvers = set()
        equations = set()
        materials = set()

        for f in files:
            module = import_module("femexamples." + f)
            if hasattr(module, "get_information"):
                info = getattr(module, "get_information")()
                files_info[f] = info
                self.files_name[info["name"]] = f
                meshes.add(info["meshelement"])
                equations.add(info["equation"])
                materials.add(info["material"])
                file_solvers = info["solvers"]
                for solver in file_solvers:
                    solvers.add(solver)
                file_constraints = info["constraints"]
                for constraint in file_constraints:
                    constraints.add(constraint)

        all_examples = QtGui.QTreeWidgetItem(self.view, ["All"])
        for example, info in files_info.items():
            QtGui.QTreeWidgetItem(all_examples, [info["name"]])

        self.view.addTopLevelItem(all_examples)
        all_constraints = QtGui.QTreeWidgetItem(self.view, ["Constraints"])
        for constraint in constraints:
            constraint_item = QtGui.QTreeWidgetItem(all_constraints, [constraint])
            for example, info in files_info.items():
                file_constraints = info["constraints"]
                if constraint in file_constraints:
                    QtGui.QTreeWidgetItem(constraint_item, [info["name"]])

        self.view.addTopLevelItem(all_constraints)
        all_solvers = QtGui.QTreeWidgetItem(self.view, ["Solvers"])
        for solver in solvers:
            solver_item = QtGui.QTreeWidgetItem(all_solvers, [solver])
            for example, info in files_info.items():
                file_solvers = info["solvers"]
                if solver in file_solvers:
                    QtGui.QTreeWidgetItem(solver_item, [info["name"]])

        self.view.addTopLevelItem(all_solvers)

        all_meshes = QtGui.QTreeWidgetItem(self.view, ["Meshes"])
        for mesh in meshes:
            mesh_item = QtGui.QTreeWidgetItem(all_meshes, [mesh])
            for example, info in files_info.items():
                if info["meshelement"] == mesh:
                    QtGui.QTreeWidgetItem(mesh_item, [info["name"]])

        self.view.addTopLevelItem(all_meshes)
        all_equations = QtGui.QTreeWidgetItem(self.view, ["Equations"])
        for equation in equations:
            equation_item = QtGui.QTreeWidgetItem(all_equations, [equation])
            for example, info in files_info.items():
                if info["equation"] == equation:
                    QtGui.QTreeWidgetItem(equation_item, [info["name"]])

        self.view.addTopLevelItem(all_equations)
        all_materials = QtGui.QTreeWidgetItem(self.view, ["Materials"])
        for material in materials:
            material_item = QtGui.QTreeWidgetItem(all_materials, [material])
            for example, info in files_info.items():
                if info["material"] == material:
                    QtGui.QTreeWidgetItem(material_item, [info["name"]])

        self.view.addTopLevelItem(all_materials)

        self.view.setHeaderHidden(True)
        self.view.itemClicked.connect(self.enable_buttons)

        # Ok buttons:
        self.button_box = QtGui.QDialogButtonBox(self)
        self.button_box.setOrientation(QtCore.Qt.Horizontal)
        self.button_box.setStandardButtons(
            QtGui.QDialogButtonBox.Cancel
        )
        self.setup_button = QtGui.QPushButton(QtGui.QIcon.fromTheme("document-new"), "Setup")
        self.setup_button.setEnabled(False)
        self.button_box.addButton(self.setup_button, QtGui.QDialogButtonBox.AcceptRole)
        self.run_button = QtGui.QPushButton(QtGui.QIcon.fromTheme("system-run"), "Run")
        self.run_button.setEnabled(False)
        self.button_box.addButton(self.run_button, QtGui.QDialogButtonBox.ApplyRole)
        self.button_box.clicked.connect(self.clicked)

        # Layout:
        layout = QtGui.QGridLayout()
        layout.addWidget(self.view, 2, 0, 1, 2)
        layout.addWidget(self.button_box, 3, 1)
        self.setLayout(layout)

    def clicked(self, button):
        if self.button_box.buttonRole(button) == QtGui.QDialogButtonBox.AcceptRole:
            self.accept()
        elif self.button_box.buttonRole(button) == QtGui.QDialogButtonBox.ApplyRole:
            self.run()
        elif self.button_box.buttonRole(button) == QtGui.QDialogButtonBox.RejectRole:
            self.reject()

    def accept(self):
        item = self.view.selectedItems()[0]
        name = item.text(0)
        example = self.files_name[name]
        # if done this way the Python commands are printed in Python console
        FreeCADGui.doCommand("from femexamples." + str(example) + "  import setup")
        FreeCADGui.doCommand("setup()")

    def reject(self):
        self.close()

    def closeEvent(self, ev):
        pw = self.parentWidget()
        if pw and pw.inherits("QDockWidget"):
            pw.deleteLater()

    def run(self):
        item = self.view.selectedItems()[0]
        name = item.text(0)
        example = self.files_name[name]
        # if done this way the Python commands are printed in Python console
        FreeCADGui.doCommand("from femexamples.manager import run_example")
        FreeCADGui.doCommand("run_example(\"" + str(example) + "\")")

    def enable_buttons(self):
        # only enable buttons if a example is selected
        sel_item_text = self.view.selectedItems()[0].text(0)
        if sel_item_text in self.files_name:
            self.run_button.setEnabled(True)
            self.setup_button.setEnabled(True)
        else:
            self.run_button.setEnabled(False)
            self.setup_button.setEnabled(False)


def show_examplegui():
    mw = FreeCADGui.getMainWindow()
    example_widget = QtGui.QDockWidget("FEM Examples", mw)
    example_widget.setWidget(FemExamples())
    mw.addDockWidget(QtCore.Qt.RightDockWidgetArea, example_widget)

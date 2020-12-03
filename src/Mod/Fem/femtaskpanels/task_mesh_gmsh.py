# ***************************************************************************
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__  = "FreeCAD FEM mesh gmsh task panel for the document object"
__author__ = "Bernd Hahnebach"
__url__    = "https://www.freecadweb.org"

## @package task_mesh_gmsh
#  \ingroup FEM
#  \brief task panel for mesh gmsh object

import sys
import time

from PySide import QtCore
from PySide import QtGui
from PySide.QtCore import Qt
from PySide.QtGui import QApplication

import FreeCAD
import FreeCADGui

import FemGui
from femobjects import mesh_gmsh
from femtools.femutils import is_of_type


class _TaskPanel:
    """
    The TaskPanel for editing References property of
    MeshGmsh objects and creation of new FEM mesh
    """

    def __init__(self, obj):
        self.mesh_obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshGmsh.ui"
        )

        self.Timer = QtCore.QTimer()
        self.Timer.start(100)  # 100 milli seconds
        self.gmsh_runs = False
        self.console_message_gmsh = ""

        QtCore.QObject.connect(
            self.form.if_max,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.max_changed
        )
        QtCore.QObject.connect(
            self.form.if_min,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.min_changed
        )
        QtCore.QObject.connect(
            self.form.cb_dimension,
            QtCore.SIGNAL("activated(int)"),
            self.choose_dimension
        )
        QtCore.QObject.connect(
            self.Timer,
            QtCore.SIGNAL("timeout()"),
            self.update_timer_text
        )

        self.form.cb_dimension.addItems(
            mesh_gmsh.MeshGmsh.known_element_dimensions
        )

        self.get_mesh_params()
        self.get_active_analysis()
        self.update()

    def getStandardButtons(self):
        button_value = int(
            QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Cancel
        )
        return button_value
        # show a OK, a apply and a Cancel button
        # def reject() is called on Cancel button
        # def clicked(self, button) is needed, to access the apply button

    def accept(self):
        self.set_mesh_params()
        self.mesh_obj.ViewObject.Document.resetEdit()
        self.mesh_obj.Document.recompute()
        return True

    def reject(self):
        self.mesh_obj.ViewObject.Document.resetEdit()
        self.mesh_obj.Document.recompute()
        return True

    def clicked(self, button):
        if button == QtGui.QDialogButtonBox.Apply:
            self.set_mesh_params()
            self.run_gmsh()

    def get_mesh_params(self):
        self.clmax = self.mesh_obj.CharacteristicLengthMax
        self.clmin = self.mesh_obj.CharacteristicLengthMin
        self.dimension = self.mesh_obj.ElementDimension

    def set_mesh_params(self):
        self.mesh_obj.CharacteristicLengthMax = self.clmax
        self.mesh_obj.CharacteristicLengthMin = self.clmin
        self.mesh_obj.ElementDimension = self.dimension

    def update(self):
        "fills the widgets"
        self.form.if_max.setText(self.clmax.UserString)
        self.form.if_min.setText(self.clmin.UserString)
        index_dimension = self.form.cb_dimension.findText(self.dimension)
        self.form.cb_dimension.setCurrentIndex(index_dimension)

    def console_log(self, message="", color="#000000"):
        if (not isinstance(message, bytes)) and (sys.version_info.major < 3):
            message = message.encode("utf-8", "replace")
        self.console_message_gmsh = self.console_message_gmsh + (
            '<font color="#0000FF">{0:4.1f}:</font> <font color="{1}">{2}</font><br>'
            .format(time.time() - self.Start, color, message)
        )
        self.form.te_output.setText(self.console_message_gmsh)
        self.form.te_output.moveCursor(QtGui.QTextCursor.End)

    def update_timer_text(self):
        # FreeCAD.Console.PrintMessage("timer1\n")
        if self.gmsh_runs:
            FreeCAD.Console.PrintMessage("timer2\n")
            # FreeCAD.Console.PrintMessage("Time: {0:4.1f}: \n".format(time.time() - self.Start))
            self.form.l_time.setText("Time: {0:4.1f}: ".format(time.time() - self.Start))

    def max_changed(self, base_quantity_value):
        self.clmax = base_quantity_value

    def min_changed(self, base_quantity_value):
        self.clmin = base_quantity_value

    def choose_dimension(self, index):
        if index < 0:
            return
        self.form.cb_dimension.setCurrentIndex(index)
        self.dimension = str(self.form.cb_dimension.itemText(index))  # form returns unicode

    def run_gmsh(self):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        part = self.mesh_obj.Part
        if (
            self.mesh_obj.MeshRegionList and part.Shape.ShapeType == "Compound"
            and (
                is_of_type(part, "FeatureBooleanFragments")
                or is_of_type(part, "FeatureSlice")
                or is_of_type(part, "FeatureXOR")
            )
        ):
            error_message = (
                "The shape to mesh is a boolean split tools Compound "
                "and the mesh has mesh region list. "
                "Gmsh could return unexpected meshes in such circumstances. "
                "It is strongly recommended to extract the shape "
                "to mesh from the Compound and use this one."
            )
            qtbox_title = (
                "Shape to mesh is a BooleanFragmentsCompound "
                "and mesh regions are defined"
            )
            QtGui.QMessageBox.critical(
                None,
                qtbox_title,
                error_message
            )
        self.Start = time.time()
        self.form.l_time.setText("Time: {0:4.1f}: ".format(time.time() - self.Start))
        self.console_message_gmsh = ""
        self.gmsh_runs = True
        self.console_log("We are going to start ...")
        self.get_active_analysis()
        from femmesh import gmshtools
        gmsh_mesh = gmshtools.GmshTools(self.mesh_obj, self.analysis)
        self.console_log("Start Gmsh ...")
        error = ""
        try:
            error = gmsh_mesh.create_mesh()
        except Exception:
            import sys
            FreeCAD.Console.PrintMessage(
                "Unexpected error when creating mesh: {}\n"
                .format(sys.exc_info()[0])
            )
        if error:
            FreeCAD.Console.PrintMessage("Gmsh had warnings ...\n")
            FreeCAD.Console.PrintMessage("{}\n".format(error))
            self.console_log("Gmsh had warnings ...")
            self.console_log(error, "#FF0000")
        else:
            FreeCAD.Console.PrintMessage("Clean run of Gmsh\n")
            self.console_log("Clean run of Gmsh")
        self.console_log("Gmsh done!")
        self.form.l_time.setText("Time: {0:4.1f}: ".format(time.time() - self.Start))
        self.Timer.stop()
        self.update()
        QApplication.restoreOverrideCursor()

    def get_active_analysis(self):
        analysis = FemGui.getActiveAnalysis()
        if not analysis:
            FreeCAD.Console.PrintLog("No active analysis, means no group meshing.\n")
            self.analysis = None  # no group meshing
        else:
            for m in analysis.Group:
                if m.Name == self.mesh_obj.Name:
                    FreeCAD.Console.PrintMessage(
                        "Active analysis found: {}\n"
                        .format(analysis.Name)
                    )
                    self.analysis = analysis  # group meshing
                    break
            else:
                FreeCAD.Console.PrintLog(
                    "Mesh is not member of active analysis, means no group meshing.\n"
                )
                self.analysis = None  # no group meshing
        return

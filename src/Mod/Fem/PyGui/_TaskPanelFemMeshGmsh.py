# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "_TaskPanelFemMeshGmsh"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package TaskPanelFemMeshGmsh
#  \ingroup FEM

import FreeCAD
import time
import PyObjects._FemMeshGmsh
import FreeCADGui
from PySide import QtGui
from PySide import QtCore
from PySide.QtCore import Qt
from PySide.QtGui import QApplication


class _TaskPanelFemMeshGmsh:
    '''The TaskPanel for editing References property of FemMeshGmsh objects and creation of new FEM mesh'''
    def __init__(self, obj):
        self.mesh_obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/PyGui/TaskPanelFemMeshGmsh.ui")

        self.Timer = QtCore.QTimer()
        self.Timer.start(100)  # 100 milli seconds
        self.gmsh_runs = False
        self.console_message_gmsh = ''

        QtCore.QObject.connect(self.form.if_max, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.max_changed)
        QtCore.QObject.connect(self.form.if_min, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.min_changed)
        QtCore.QObject.connect(self.form.cb_dimension, QtCore.SIGNAL("activated(int)"), self.choose_dimension)
        QtCore.QObject.connect(self.Timer, QtCore.SIGNAL("timeout()"), self.update_timer_text)

        self.form.cb_dimension.addItems(PyObjects._FemMeshGmsh._FemMeshGmsh.known_element_dimensions)

        self.get_mesh_params()
        self.get_active_analysis()
        self.update()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Cancel)
        # show a OK, a apply and a Cancel button
        # def reject() is called on Cancel button
        # def clicked(self, button) is needed, to access the apply button

    def accept(self):
        self.set_mesh_params()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.recompute()
        return True

    def reject(self):
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.recompute()
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
        'fills the widgets'
        self.form.if_max.setText(self.clmax.UserString)
        self.form.if_min.setText(self.clmin.UserString)
        index_dimension = self.form.cb_dimension.findText(self.dimension)
        self.form.cb_dimension.setCurrentIndex(index_dimension)

    def console_log(self, message="", color="#000000"):
        self.console_message_gmsh = self.console_message_gmsh + '<font color="#0000FF">{0:4.1f}:</font> <font color="{1}">{2}</font><br>'.\
            format(time.time() - self.Start, color, message.encode('utf-8', 'replace'))
        self.form.te_output.setText(self.console_message_gmsh)
        self.form.te_output.moveCursor(QtGui.QTextCursor.End)

    def update_timer_text(self):
        # print('timer1')
        if self.gmsh_runs:
            print('timer2')
            # print('Time: {0:4.1f}: '.format(time.time() - self.Start))
            self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

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
        partsh = self.obj.Part
        if partsh.Shape.ShapeType == "Compound":
            error_message = "The mesh to shape is a Compound, GMSH could return unexpected meshes for Compounds. It is strongly recommended to extract the shape to mesh from the Compound and use this one."
            FreeCAD.Console.PrintError(error_message + "\n")
            if hasattr(partsh, "Proxy") and (partsh.Proxy.Type == "FeatureBooleanFragments" or partsh.Proxy.Type == "FeatureSlice" or partsh.Proxy.Type == "FeatureXOR"):  # other part obj might not have a Proxy
                error_message = "The mesh to shape is a boolean split tools Compound, GMSH could return unexpected meshes for a boolean split tools Compound. It is strongly recommended to extract the shape to mesh from the Compound and use this one."
                FreeCAD.Console.PrintError(error_message + "\n")
                QtGui.QMessageBox.critical(None, "Shape to mesh is a Compound", error_message)
        self.Start = time.time()
        self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        self.console_message_gmsh = ''
        self.gmsh_runs = True
        self.console_log("We gone start ...")
        self.get_active_analysis()
        import FemGmshTools
        gmsh_mesh = FemGmshTools.FemGmshTools(self.obj, self.analysis)
        self.console_log("Start GMSH ...")
        error = ''
        try:
            error = gmsh_mesh.create_mesh()
        except:
            import sys
            print("Unexpected error when creating mesh: ", sys.exc_info()[0])
        if error:
            print(error)
            self.console_log('GMSH had warnings ...')
            self.console_log(error, '#FF0000')
        else:
            self.console_log('Clean run of GMSH')
        self.console_log("GMSH done!")
        self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        self.Timer.stop()
        self.update()
        QApplication.restoreOverrideCursor()

    def get_active_analysis(self):
        import FemGui
        self.analysis = FemGui.getActiveAnalysis()
        if self.analysis:
            for m in FemGui.getActiveAnalysis().Member:
                if m.Name == self.mesh_obj.Name:
                    print('Active analysis found: ' + self.analysis.Name)
                    return
            else:
                # print('Mesh is not member of active analysis, means no group meshing')
                self.analysis = None  # no group meshing
        else:
            # print('No active analyis, means no group meshing')
            self.analysis = None  # no group meshing

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013-2015 - Juergen Riegel <FreeCAD@juergen-riegel.net> *
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

__title__ = "Result Control Task Panel"
__author__ = "Juergen Riegel, Michael Hindley"
__url__ = "http://www.freecadweb.org"

## @package TaskPanelShowResult
#  \ingroup FEM

import FreeCAD
import FemTools
import numpy as np

import FreeCADGui
import FemGui
from PySide import QtCore, QtGui
from PySide.QtCore import Qt
from PySide.QtGui import QApplication


class _TaskPanelShowResult:
    '''The task panel for the post-processing'''
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/TaskPanelShowResult.ui")
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
        self.restore_result_settings_in_dialog = self.fem_prefs.GetBool("RestoreResultDialog", True)

        # Connect Signals and Slots
        QtCore.QObject.connect(self.form.rb_none, QtCore.SIGNAL("toggled(bool)"), self.none_selected)
        QtCore.QObject.connect(self.form.rb_x_displacement, QtCore.SIGNAL("toggled(bool)"), self.x_displacement_selected)
        QtCore.QObject.connect(self.form.rb_y_displacement, QtCore.SIGNAL("toggled(bool)"), self.y_displacement_selected)
        QtCore.QObject.connect(self.form.rb_z_displacement, QtCore.SIGNAL("toggled(bool)"), self.z_displacement_selected)
        QtCore.QObject.connect(self.form.rb_abs_displacement, QtCore.SIGNAL("toggled(bool)"), self.abs_displacement_selected)
        QtCore.QObject.connect(self.form.rb_vm_stress, QtCore.SIGNAL("toggled(bool)"), self.vm_stress_selected)

        QtCore.QObject.connect(self.form.rb_max_shear_stress, QtCore.SIGNAL("toggled(bool)"), self.max_shear_selected)
        QtCore.QObject.connect(self.form.rb_maxprin, QtCore.SIGNAL("toggled(bool)"), self.max_prin_selected)
        QtCore.QObject.connect(self.form.rb_minprin, QtCore.SIGNAL("toggled(bool)"), self.min_prin_selected)
        QtCore.QObject.connect(self.form.rb_temperature, QtCore.SIGNAL("toggled(bool)"), self.temperature_selected)
        QtCore.QObject.connect(self.form.user_def_eq, QtCore.SIGNAL("textchanged()"), self.user_defined_text)
        QtCore.QObject.connect(self.form.calculate, QtCore.SIGNAL("clicked()"), self.calculate)

        QtCore.QObject.connect(self.form.cb_show_displacement, QtCore.SIGNAL("clicked(bool)"), self.show_displacement)
        QtCore.QObject.connect(self.form.hsb_displacement_factor, QtCore.SIGNAL("valueChanged(int)"), self.hsb_disp_factor_changed)
        QtCore.QObject.connect(self.form.sb_displacement_factor, QtCore.SIGNAL("valueChanged(int)"), self.sb_disp_factor_changed)
        QtCore.QObject.connect(self.form.sb_displacement_factor_max, QtCore.SIGNAL("valueChanged(int)"), self.sb_disp_factor_max_changed)

        self.update()
        if self.restore_result_settings_in_dialog:
            self.restore_result_dialog()
        else:
            self.restore_initial_result_dialog()

    def restore_result_dialog(self):
        try:
            rt = FreeCAD.FEM_dialog["results_type"]
            if rt == "None":
                self.form.rb_none.setChecked(True)
                self.none_selected(True)
            elif rt == "Uabs":
                self.form.rb_abs_displacement.setChecked(True)
                self.abs_displacement_selected(True)
            elif rt == "U1":
                self.form.rb_x_displacement.setChecked(True)
                self.x_displacement_selected(True)
            elif rt == "U2":
                self.form.rb_y_displacement.setChecked(True)
                self.y_displacement_selected(True)
            elif rt == "U3":
                self.form.rb_z_displacement.setChecked(True)
                self.z_displacement_selected(True)
            elif rt == "Sabs":
                self.form.rb_vm_stress.setChecked(True)
                self.vm_stress_selected(True)
            elif rt == "MaxShear":
                self.form.rb_max_shear.setChecked(True)
                self.rb_max_shear(True)
            elif rt == "MaxPrin":
                self.form.rb_maxprin.setChecked(True)
                self.rb_maxprin(True)
            elif rt == "Temp":
                self.form.rb_temperature.setChecked(True)
                self.rb_temperature(True)
            elif rt == "MinPrin":
                self.form.rb_minprin.setChecked(True)
                self.rb_minprin(True)

            sd = FreeCAD.FEM_dialog["show_disp"]
            self.form.cb_show_displacement.setChecked(sd)
            self.show_displacement(sd)

            df = FreeCAD.FEM_dialog["disp_factor"]
            dfm = FreeCAD.FEM_dialog["disp_factor_max"]
            self.form.hsb_displacement_factor.setMaximum(dfm)
            self.form.hsb_displacement_factor.setValue(df)
            self.form.sb_displacement_factor_max.setValue(dfm)
            self.form.sb_displacement_factor.setValue(df)
        except:
            self.restore_initial_result_dialog()

    def restore_initial_result_dialog(self):
        FreeCAD.FEM_dialog = {"results_type": "None", "show_disp": False,
                              "disp_factor": 0, "disp_factor_max": 100}
        fea = FemTools.FemTools()
        fea.reset_mesh_color()
        fea.reset_mesh_deformation()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def get_result_stats(self, type_name, analysis=None):
        if "Stats" in self.result_object.PropertiesList:
                Stats = self.result_object.Stats
                match_table = {"U1": (Stats[0], Stats[1], Stats[2]),
                               "U2": (Stats[3], Stats[4], Stats[5]),
                               "U3": (Stats[6], Stats[7], Stats[8]),
                               "Uabs": (Stats[9], Stats[10], Stats[11]),
                               "Sabs": (Stats[12], Stats[13], Stats[14]),
                               "MaxPrin": (Stats[15], Stats[16], Stats[17]),
                               "MidPrin": (Stats[18], Stats[19], Stats[20]),
                               "MinPrin": (Stats[21], Stats[22], Stats[23]),
                               "MaxShear": (Stats[24], Stats[25], Stats[26]),
                               "None": (0.0, 0.0, 0.0)}
                return match_table[type_name]
        return (0.0, 0.0, 0.0)

    def none_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "None"
        self.set_result_stats("mm", 0.0, 0.0, 0.0)
        fea = FemTools.FemTools()
        fea.reset_mesh_color()

    def abs_displacement_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "Uabs"
        self.select_displacement_type("Uabs")

    def x_displacement_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "U1"
        self.select_displacement_type("U1")

    def y_displacement_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "U2"
        self.select_displacement_type("U2")

    def z_displacement_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "U3"
        self.select_displacement_type("U3")

    def vm_stress_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "Sabs"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.MeshObject.ViewObject.setNodeColorByScalars(self.result_object.NodeNumbers, self.result_object.StressValues)
        (minm, avg, maxm) = self.get_result_stats("Sabs")
        self.set_result_stats("MPa", minm, avg, maxm)
        QtGui.qApp.restoreOverrideCursor()

    def max_shear_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "MaxShear"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.MeshObject.ViewObject.setNodeColorByScalars(self.result_object.NodeNumbers, self.result_object.MaxShear)
        (minm, avg, maxm) = self.get_result_stats("MaxShear")
        self.set_result_stats("MPa", minm, avg, maxm)
        QtGui.qApp.restoreOverrideCursor()

    def max_prin_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "MaxPrin"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.MeshObject.ViewObject.setNodeColorByScalars(self.result_object.NodeNumbers, self.result_object.PrincipalMax)
        (minm, avg, maxm) = self.get_result_stats("MaxPrin")
        self.set_result_stats("MPa", minm, avg, maxm)
        QtGui.qApp.restoreOverrideCursor()

    def temperature_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "Temp"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.MeshObject.ViewObject.setNodeColorByScalars(self.result_object.NodeNumbers, self.result_object.Temperature)
        minm = min(self.result_object.Temperature)
        avg = sum(self.result_object.Temperature) / len(self.result_object.Temperature)
        maxm = max(self.result_object.Temperature)
        self.set_result_stats("K", minm, avg, maxm)
        QtGui.qApp.restoreOverrideCursor()

    def min_prin_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "MinPrin"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.MeshObject.ViewObject.setNodeColorByScalars(self.result_object.NodeNumbers, self.result_object.PrincipalMin)
        (minm, avg, maxm) = self.get_result_stats("MinPrin")
        self.set_result_stats("MPa", minm, avg, maxm)
        QtGui.qApp.restoreOverrideCursor()

    def user_defined_text(self, equation):
        FreeCAD.FEM_dialog["results_type"] = "user"
        self.form.user_def_eq.toPlainText()

    def calculate(self):
        FreeCAD.FEM_dialog["results_type"] = "None"
        self.update()
        self.restore_result_dialog()
        # Convert existing values to numpy array
        P1 = np.array(self.result_object.PrincipalMax)
        P2 = np.array(self.result_object.PrincipalMed)
        P3 = np.array(self.result_object.PrincipalMin)
        Von = np.array(self.result_object.StressValues)
        T = np.array(self.result_object.Temperature)
        dispvectors = np.array(self.result_object.DisplacementVectors)
        x = np.array(dispvectors[:, 0])
        y = np.array(dispvectors[:, 1])
        z = np.array(dispvectors[:, 2])
        stressvectors = np.array(self.result_object.StressVectors)
        sx = np.array(stressvectors[:, 0])
        sy = np.array(stressvectors[:, 1])
        sz = np.array(stressvectors[:, 2])
        strainvectors = np.array(self.result_object.StrainVectors)
        ex = np.array(strainvectors[:, 0])
        ey = np.array(strainvectors[:, 1])
        ez = np.array(strainvectors[:, 2])
        userdefined_eq = x + y + z + T + Von + P1 + P2 + P3 + sx + sy + sz + ex + ey + ez  # Dummy equation to get around flake8, varibles not being used
        userdefined_eq = self.form.user_def_eq.toPlainText()  # Get equation to be used
        UserDefinedFormula = eval(userdefined_eq).tolist()
        self.result_object.UserDefined = UserDefinedFormula
        minm = min(UserDefinedFormula)
        avg = sum(UserDefinedFormula) / len(UserDefinedFormula)
        maxm = max(UserDefinedFormula)

        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.MeshObject.ViewObject.setNodeColorByScalars(self.result_object.NodeNumbers, UserDefinedFormula)
        self.set_result_stats("", minm, avg, maxm)
        QtGui.qApp.restoreOverrideCursor()

    def select_displacement_type(self, disp_type):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if disp_type == "Uabs":
            if self.suitable_results:
                self.MeshObject.ViewObject.setNodeColorByScalars(self.result_object.NodeNumbers, self.result_object.DisplacementLengths)
        else:
            match = {"U1": 0, "U2": 1, "U3": 2}
            d = zip(*self.result_object.DisplacementVectors)
            displacements = list(d[match[disp_type]])
            if self.suitable_results:
                self.MeshObject.ViewObject.setNodeColorByScalars(self.result_object.NodeNumbers, displacements)
        (minm, avg, maxm) = self.get_result_stats(disp_type)
        self.set_result_stats("mm", minm, avg, maxm)
        QtGui.qApp.restoreOverrideCursor()

    def set_result_stats(self, unit, minm, avg, maxm):
        self.form.le_min.setProperty("unit", unit)
        self.form.le_min.setText("{:.6} {}".format(minm, unit))
        self.form.le_avg.setProperty("unit", unit)
        self.form.le_avg.setText("{:.6} {}".format(avg, unit))
        self.form.le_max.setProperty("unit", unit)
        self.form.le_max.setText("{:.6} {}".format(maxm, unit))

    def update_displacement(self, factor=None):
        if factor is None:
            if FreeCAD.FEM_dialog["show_disp"]:
                factor = self.form.hsb_displacement_factor.value()
            else:
                factor = 0.0
        self.MeshObject.ViewObject.applyDisplacement(factor)

    def show_displacement(self, checked):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        FreeCAD.FEM_dialog["show_disp"] = checked
        if "result_object" in FreeCAD.FEM_dialog:
            if FreeCAD.FEM_dialog["result_object"] != self.result_object:
                self.update_displacement()
        FreeCAD.FEM_dialog["result_object"] = self.result_object
        if self.suitable_results:
            self.MeshObject.ViewObject.setNodeDisplacementByVectors(self.result_object.NodeNumbers, self.result_object.DisplacementVectors)
        self.update_displacement()
        QtGui.qApp.restoreOverrideCursor()

    def hsb_disp_factor_changed(self, value):
        self.form.sb_displacement_factor.setValue(value)
        self.update_displacement()

    def sb_disp_factor_max_changed(self, value):
        FreeCAD.FEM_dialog["disp_factor_max"] = value
        self.form.hsb_displacement_factor.setMaximum(value)

    def sb_disp_factor_changed(self, value):
        FreeCAD.FEM_dialog["disp_factor"] = value
        self.form.hsb_displacement_factor.setValue(value)

    def update(self):
        self.MeshObject = None
        self.result_object = get_results_object(FreeCADGui.Selection.getSelection())
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemMeshObject"):
                self.MeshObject = i
                break

        self.suitable_results = False
        if self.result_object:
            # Disable temperature radio button if it does ot exist in results
            if len(self.result_object.Temperature) == 1:
                self.form.rb_temperature.setEnabled(0)

            if (self.MeshObject.FemMesh.NodeCount == len(self.result_object.NodeNumbers)):
                self.suitable_results = True
            else:
                if not self.MeshObject.FemMesh.VolumeCount:
                    FreeCAD.Console.PrintError('FEM: Graphical bending stress output for beam or shell FEM Meshes not yet supported.\n')
                else:
                    FreeCAD.Console.PrintError('FEM: Result node numbers are not equal to FEM Mesh NodeCount.\n')
        else:
            error_message = 'FEM: Result task panel, no result object to display results for.\n'
            FreeCAD.Console.PrintError(error_message)
            QtGui.QMessageBox.critical(None, 'No result object', error_message)

    def accept(self):
        FreeCADGui.Control.closeDialog()

    def reject(self):
        FreeCADGui.Control.closeDialog()


# It's code duplication that should be removes wher we migrate to FemTools.py
def get_results_object(sel):
    if (len(sel) == 1):
        if sel[0].isDerivedFrom("Fem::FemResultObject"):
            return sel[0]

    for i in FemGui.getActiveAnalysis().Member:
        if(i.isDerivedFrom("Fem::FemResultObject")):
            return i
    return None

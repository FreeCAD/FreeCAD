# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Qingfeng Xia <qingfeng.xia()eng.ox.ac.uk>          *
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

__title__ = "FreeCAD result mechanical ViewProvider for the document object"
__author__ = "Qingfeng Xia, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package _ViewProviderFemResultMechanical
#  \ingroup FEM
#  \brief FreeCAD ViewProvider for mechanical ResultObjectPython in FEM workbench

import FreeCAD
import FreeCADGui
import FemGui  # needed to display the icons in TreeView
False if False else FemGui.__name__  # dummy usage of FemGui for flake8, just returns 'FemGui'

# for the panel
import FemGui
import femresult.resulttools as resulttools
from PySide import QtCore
from PySide import QtGui
from PySide.QtCore import Qt
from PySide.QtGui import QApplication
import numpy as np


class _ViewProviderFemResultMechanical:
    "A View Provider for the FemResultObject Python derived FemResult class"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        """after load from FCStd file, self.icon does not exist, return constant path instead"""
        return ":/icons/fem-post-result-show.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def doubleClicked(self, vobj):
        guidoc = FreeCADGui.getDocument(vobj.Object.Document)
        # check if another VP is in edit mode, https://forum.freecadweb.org/viewtopic.php?t=13077#p104702
        if not guidoc.getInEdit():
            guidoc.setEdit(vobj.Object.Name)
        else:
            from PySide.QtGui import QMessageBox
            message = 'Active Task Dialog found! Please close this one before opening  a new one!'
            QMessageBox.critical(None, "Error in tree view", message)
            FreeCAD.Console.PrintError(message + '\n')
        return True

    def setEdit(self, vobj, mode=0):
        if hasattr(self.Object, "Mesh") and self.Object.Mesh:
            hide_femmeshes_postpiplines()
            # only show the FEM result mesh
            self.Object.Mesh.ViewObject.show()
            taskd = _TaskPanelFemResultShow(self.Object)
            taskd.obj = vobj.Object
            FreeCADGui.Control.showDialog(taskd)
            return True
        else:
            error_message = 'FEM: Result object has no appropriate FEM mesh.\n'
            FreeCAD.Console.PrintError(error_message)
            from PySide import QtGui
            QtGui.QMessageBox.critical(None, 'No result object', error_message)
            return False

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        self.Object.Mesh.ViewObject.hide()  # hide the mesh after result viewing is finished, but do not reset the coloring
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def claimChildren(self):
        return [self.Object.Mesh]  # claimChildren needs to return a list !

    def onDelete(self, feature, subelements):
        try:
            for obj in self.claimChildren():
                obj.ViewObject.show()
        except Exception as err:
            FreeCAD.Console.PrintError("Error in onDelete: " + err.message)
        return True


class _TaskPanelFemResultShow:
    '''The task panel for the post-processing'''

    def __init__(self, obj):
        self.result_obj = obj
        self.mesh_obj = self.result_obj.Mesh
        # task panel should be started by use of setEdit of view provider
        # in view provider checks: Mesh, active analysis and if Mesh and result are in active analysis

        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ResultShow.ui")
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
        self.restore_result_settings_in_dialog = self.fem_prefs.GetBool("RestoreResultDialog", True)

        # Connect Signals and Slots
        # result type radio buttons
        QtCore.QObject.connect(self.form.rb_none, QtCore.SIGNAL("toggled(bool)"), self.none_selected)
        QtCore.QObject.connect(self.form.rb_abs_displacement, QtCore.SIGNAL("toggled(bool)"), self.abs_displacement_selected)
        QtCore.QObject.connect(self.form.rb_x_displacement, QtCore.SIGNAL("toggled(bool)"), self.x_displacement_selected)
        QtCore.QObject.connect(self.form.rb_y_displacement, QtCore.SIGNAL("toggled(bool)"), self.y_displacement_selected)
        QtCore.QObject.connect(self.form.rb_z_displacement, QtCore.SIGNAL("toggled(bool)"), self.z_displacement_selected)
        QtCore.QObject.connect(self.form.rb_temperature, QtCore.SIGNAL("toggled(bool)"), self.temperature_selected)
        QtCore.QObject.connect(self.form.rb_vm_stress, QtCore.SIGNAL("toggled(bool)"), self.vm_stress_selected)
        QtCore.QObject.connect(self.form.rb_maxprin, QtCore.SIGNAL("toggled(bool)"), self.max_prin_selected)
        QtCore.QObject.connect(self.form.rb_minprin, QtCore.SIGNAL("toggled(bool)"), self.min_prin_selected)
        QtCore.QObject.connect(self.form.rb_max_shear_stress, QtCore.SIGNAL("toggled(bool)"), self.max_shear_selected)
        QtCore.QObject.connect(self.form.rb_massflowrate, QtCore.SIGNAL("toggled(bool)"), self.massflowrate_selected)
        QtCore.QObject.connect(self.form.rb_networkpressure, QtCore.SIGNAL("toggled(bool)"), self.networkpressure_selected)
        QtCore.QObject.connect(self.form.rb_peeq, QtCore.SIGNAL("toggled(bool)"), self.peeq_selected)

        # displacement
        QtCore.QObject.connect(self.form.cb_show_displacement, QtCore.SIGNAL("clicked(bool)"), self.show_displacement)
        QtCore.QObject.connect(self.form.hsb_displacement_factor, QtCore.SIGNAL("valueChanged(int)"), self.hsb_disp_factor_changed)
        QtCore.QObject.connect(self.form.sb_displacement_factor, QtCore.SIGNAL("valueChanged(int)"), self.sb_disp_factor_changed)
        QtCore.QObject.connect(self.form.sb_displacement_factor_max, QtCore.SIGNAL("valueChanged(int)"), self.sb_disp_factor_max_changed)

        # user defined equation
        QtCore.QObject.connect(self.form.user_def_eq, QtCore.SIGNAL("textchanged()"), self.user_defined_text)
        QtCore.QObject.connect(self.form.calculate, QtCore.SIGNAL("clicked()"), self.calculate)

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
            elif rt == "Temp":
                self.form.rb_temperature.setChecked(True)
                self.temperature_selected(True)
            elif rt == "Sabs":
                self.form.rb_vm_stress.setChecked(True)
                self.vm_stress_selected(True)
            elif rt == "MaxPrin":
                self.form.rb_maxprin.setChecked(True)
                self.max_prin_selected(True)
            elif rt == "MinPrin":
                self.form.rb_minprin.setChecked(True)
                self.min_prin_selected(True)
            elif rt == "MaxShear":
                self.form.rb_max_shear_stress.setChecked(True)
                self.max_shear_selected(True)
            elif rt == "MFlow":
                self.form.rb_massflowrate.setChecked(True)
                self.massflowrate_selected(True)
            elif rt == "NPress":
                self.form.rb_networkpressure.setChecked(True)
                self.networkpressure_selected(True)
            elif rt == "Peeq":
                self.form.rb_peeq.setChecked(True)
                self.peeq_selected(True)

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
        self.reset_mesh_deformation()
        self.reset_mesh_color()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def get_result_stats(self, type_name):
        return resulttools.get_stats(self.result_obj, type_name)

    def none_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "None"
        self.set_result_stats("mm", 0.0, 0.0, 0.0)
        self.reset_mesh_color()

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
            self.mesh_obj.ViewObject.setNodeColorByScalars(self.result_obj.NodeNumbers, self.result_obj.StressValues)
        (minm, avg, maxm) = self.get_result_stats("Sabs")
        self.set_result_stats("MPa", minm, avg, maxm)
        QtGui.QApplication.restoreOverrideCursor()

    def max_shear_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "MaxShear"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.mesh_obj.ViewObject.setNodeColorByScalars(self.result_obj.NodeNumbers, self.result_obj.MaxShear)
        (minm, avg, maxm) = self.get_result_stats("MaxShear")
        self.set_result_stats("MPa", minm, avg, maxm)
        QtGui.QApplication.restoreOverrideCursor()

    def max_prin_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "MaxPrin"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.mesh_obj.ViewObject.setNodeColorByScalars(self.result_obj.NodeNumbers, self.result_obj.PrincipalMax)
        (minm, avg, maxm) = self.get_result_stats("MaxPrin")
        self.set_result_stats("MPa", minm, avg, maxm)
        QtGui.QApplication.restoreOverrideCursor()

    def temperature_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "Temp"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.mesh_obj.ViewObject.setNodeColorByScalars(self.result_obj.NodeNumbers, self.result_obj.Temperature)
        (minm, avg, maxm) = self.get_result_stats("Temp")
        self.set_result_stats("K", minm, avg, maxm)
        QtGui.QApplication.restoreOverrideCursor()

    def massflowrate_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "MFlow"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.mesh_obj.ViewObject.setNodeColorByScalars(self.result_obj.NodeNumbers, self.result_obj.MassFlowRate)
        (minm, avg, maxm) = self.get_result_stats("MFlow")
        self.set_result_stats("kg/s", minm, avg, maxm)
        QtGui.QApplication.restoreOverrideCursor()

    def networkpressure_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "NPress"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.mesh_obj.ViewObject.setNodeColorByScalars(self.result_obj.NodeNumbers, self.result_obj.NetworkPressure)
        (minm, avg, maxm) = self.get_result_stats("NPress")
        self.set_result_stats("MPa", minm, avg, maxm)
        QtGui.QApplication.restoreOverrideCursor()

    def min_prin_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "MinPrin"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.mesh_obj.ViewObject.setNodeColorByScalars(self.result_obj.NodeNumbers, self.result_obj.PrincipalMin)
        (minm, avg, maxm) = self.get_result_stats("MinPrin")
        self.set_result_stats("MPa", minm, avg, maxm)
        QtGui.QApplication.restoreOverrideCursor()

    def peeq_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "Peeq"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.mesh_obj.ViewObject.setNodeColorByScalars(self.result_obj.NodeNumbers, self.result_obj.Peeq)
        (minm, avg, maxm) = self.get_result_stats("Peeq")
        self.set_result_stats("", minm, avg, maxm)
        QtGui.QApplication.restoreOverrideCursor()

    def user_defined_text(self, equation):
        FreeCAD.FEM_dialog["results_type"] = "user"
        self.form.user_def_eq.toPlainText()

    def calculate(self):
        FreeCAD.FEM_dialog["results_type"] = "None"
        self.update()
        self.restore_result_dialog()
        # Convert existing values to numpy array
        P1 = np.array(self.result_obj.PrincipalMax)
        P2 = np.array(self.result_obj.PrincipalMed)
        P3 = np.array(self.result_obj.PrincipalMin)
        Von = np.array(self.result_obj.StressValues)
        Peeq = np.array(self.result_obj.Peeq)
        T = np.array(self.result_obj.Temperature)
        MF = np.array(self.result_obj.MassFlowRate)
        NP = np.array(self.result_obj.NetworkPressure)
        dispvectors = np.array(self.result_obj.DisplacementVectors)
        x = np.array(dispvectors[:, 0])
        y = np.array(dispvectors[:, 1])
        z = np.array(dispvectors[:, 2])
        stressvectors = np.array(self.result_obj.StressVectors)
        sx = np.array(stressvectors[:, 0])
        sy = np.array(stressvectors[:, 1])
        sz = np.array(stressvectors[:, 2])
        strainvectors = np.array(self.result_obj.StrainVectors)
        ex = np.array(strainvectors[:, 0])
        ey = np.array(strainvectors[:, 1])
        ez = np.array(strainvectors[:, 2])
        userdefined_eq = self.form.user_def_eq.toPlainText()  # Get equation to be used
        UserDefinedFormula = eval(userdefined_eq).tolist()
        self.result_obj.UserDefined = UserDefinedFormula
        minm = min(UserDefinedFormula)
        avg = sum(UserDefinedFormula) / len(UserDefinedFormula)
        maxm = max(UserDefinedFormula)

        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.mesh_obj.ViewObject.setNodeColorByScalars(self.result_obj.NodeNumbers, UserDefinedFormula)
        self.set_result_stats("", minm, avg, maxm)
        QtGui.QApplication.restoreOverrideCursor()
        del x, y, z, T, Von, Peeq, P1, P2, P3, sx, sy, sz, ex, ey, ez, MF, NP  # Dummy use of the variables to get around flake8 error

    def select_displacement_type(self, disp_type):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if disp_type == "Uabs":
            if self.suitable_results:
                self.mesh_obj.ViewObject.setNodeColorByScalars(self.result_obj.NodeNumbers, self.result_obj.DisplacementLengths)
        else:
            match = {"U1": 0, "U2": 1, "U3": 2}
            d = list(zip(*self.result_obj.DisplacementVectors))  # list is needed, as zib-object is not subscriptable in py3
            displacements = list(d[match[disp_type]])
            if self.suitable_results:
                self.mesh_obj.ViewObject.setNodeColorByScalars(self.result_obj.NodeNumbers, displacements)
        (minm, avg, maxm) = self.get_result_stats(disp_type)
        self.set_result_stats("mm", minm, avg, maxm)
        QtGui.QApplication.restoreOverrideCursor()

    def set_result_stats(self, unit, minm, avg, maxm):
        self.form.le_min.setProperty("unit", unit)
        self.form.le_min.setProperty("rawText", "{:.6} {}".format(minm, unit))
        self.form.le_avg.setProperty("unit", unit)
        self.form.le_avg.setProperty("rawText", "{:.6} {}".format(avg, unit))
        self.form.le_max.setProperty("unit", unit)
        self.form.le_max.setProperty("rawText", "{:.6} {}".format(maxm, unit))

    def update_displacement(self, factor=None):
        if factor is None:
            if FreeCAD.FEM_dialog["show_disp"]:
                factor = self.form.hsb_displacement_factor.value()
            else:
                factor = 0.0
        self.mesh_obj.ViewObject.applyDisplacement(factor)

    def show_displacement(self, checked):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        FreeCAD.FEM_dialog["show_disp"] = checked
        if "result_obj" in FreeCAD.FEM_dialog:
            if FreeCAD.FEM_dialog["result_obj"] != self.result_obj:
                self.update_displacement()
        FreeCAD.FEM_dialog["result_obj"] = self.result_obj
        if self.suitable_results:
            self.mesh_obj.ViewObject.setNodeDisplacementByVectors(self.result_obj.NodeNumbers, self.result_obj.DisplacementVectors)
        self.update_displacement()
        QtGui.QApplication.restoreOverrideCursor()

    def hsb_disp_factor_changed(self, value):
        self.form.sb_displacement_factor.setValue(value)
        self.update_displacement()

    def sb_disp_factor_max_changed(self, value):
        FreeCAD.FEM_dialog["disp_factor_max"] = value
        self.form.hsb_displacement_factor.setMaximum(value)

    def sb_disp_factor_changed(self, value):
        FreeCAD.FEM_dialog["disp_factor"] = value
        self.form.hsb_displacement_factor.setValue(value)

    def disable_empty_result_buttons(self):
        ''' disable radio buttons if result does not exists in result object'''
        '''assignments
        DisplacementLengths --> rb_abs_displacement
        DisplacementVectors --> rb_x_displacement, rb_y_displacement, rb_z_displacement
        Temperature         --> rb_temperature
        StressValues        --> rb_vm_stress
        PrincipalMax        --> rb_maxprin
        PrincipalMin        --> rb_minprin
        MaxShear            --> rb_max_shear_stress
        MassFlowRate        --> rb_massflowrate
        NetworkPressure     --> rb_networkpressure
        Peeq                --> rb_peeq'''
        if len(self.result_obj.DisplacementLengths) == 0:
            self.form.rb_abs_displacement.setEnabled(0)
        if len(self.result_obj.DisplacementVectors) == 0:
            self.form.rb_x_displacement.setEnabled(0)
            self.form.rb_y_displacement.setEnabled(0)
            self.form.rb_z_displacement.setEnabled(0)
        if len(self.result_obj.Temperature) == 0:
            self.form.rb_temperature.setEnabled(0)
        if len(self.result_obj.StressValues) == 0:
            self.form.rb_vm_stress.setEnabled(0)
        if len(self.result_obj.PrincipalMax) == 0:
            self.form.rb_maxprin.setEnabled(0)
        if len(self.result_obj.PrincipalMin) == 0:
            self.form.rb_minprin.setEnabled(0)
        if len(self.result_obj.MaxShear) == 0:
            self.form.rb_max_shear_stress.setEnabled(0)
        if len(self.result_obj.MassFlowRate) == 0:
            self.form.rb_massflowrate.setEnabled(0)
        if len(self.result_obj.NetworkPressure) == 0:
            self.form.rb_networkpressure.setEnabled(0)
        if len(self.result_obj.Peeq) == 0:
            self.form.rb_peeq.setEnabled(0)

    def update(self):
        self.suitable_results = False
        self.disable_empty_result_buttons()
        if (self.mesh_obj.FemMesh.NodeCount == len(self.result_obj.NodeNumbers)):
            self.suitable_results = True
            hide_parts_constraints()
        else:
            if not self.mesh_obj.FemMesh.VolumeCount:
                error_message = 'FEM: Graphical bending stress output for beam or shell FEM Meshes not yet supported.\n'
                FreeCAD.Console.PrintError(error_message)
                QtGui.QMessageBox.critical(None, 'No result object', error_message)
            else:
                error_message = 'FEM: Result node numbers are not equal to FEM Mesh NodeCount.\n'
                FreeCAD.Console.PrintError(error_message)
                QtGui.QMessageBox.critical(None, 'No result object', error_message)

    def reset_mesh_deformation(self):
        self.mesh_obj.ViewObject.applyDisplacement(0.0)

    def reset_mesh_color(self):
        self.mesh_obj.ViewObject.NodeColor = {}
        self.mesh_obj.ViewObject.ElementColor = {}
        node_numbers = list(self.mesh_obj.FemMesh.Nodes.keys())
        zero_values = [0] * len(node_numbers)
        self.mesh_obj.ViewObject.setNodeColorByScalars(node_numbers, zero_values)

    def reject(self):
        FreeCADGui.Control.closeDialog()  # if the tasks panel is called from Command obj is not in edit mode thus reset edit does not close the dialog, maybe don't call but set in edit instead
        FreeCADGui.ActiveDocument.resetEdit()


# helper
def hide_femmeshes_postpiplines():
    # hide all visible FEM mesh objects and VTK FemPostPipeline objects
    for o in FreeCAD.ActiveDocument.Objects:
        if o.isDerivedFrom("Fem::FemMeshObject") or o.isDerivedFrom("Fem::FemPostPipeline"):
            o.ViewObject.hide()


def hide_parts_constraints():
    fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
    hide_constraints = fem_prefs.GetBool("HideConstraint", False)
    if hide_constraints:
        for o in FreeCAD.ActiveDocument.Objects:
            if o.isDerivedFrom('Fem::FemAnalysis'):
                for acnstrmesh in FemGui.getActiveAnalysis().Group:
                    if "Constraint" in acnstrmesh.TypeId:
                        acnstrmesh.ViewObject.Visibility = False
                break

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

# for the panel
import femresult.resulttools as resulttools
from PySide import QtCore
from PySide import QtGui
from PySide.QtCore import Qt
from PySide.QtGui import QApplication
import numpy as np

False if FemGui.__name__ else True  # flake8, dummy FemGui usage


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
        # check if another VP is in edit mode
        # https://forum.freecadweb.org/viewtopic.php?t=13077#p104702
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
        # hide the mesh after result viewing is finished, but do not reset the coloring
        self.Object.Mesh.ViewObject.hide()
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
        # in view provider checks: Mesh, active analysis and
        # if Mesh and result are in active analysis

        self.form = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ResultShow.ui"
        )
        self.fem_prefs = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Fem/General"
        )
        self.restore_result_settings_in_dialog = self.fem_prefs.GetBool(
            "RestoreResultDialog", True
        )

        # Connect Signals and Slots
        # result type radio buttons
        # TODO: move to combo box, to be independent from result types and result types count
        QtCore.QObject.connect(
            self.form.rb_none, QtCore.SIGNAL("toggled(bool)"),
            self.none_selected
        )
        QtCore.QObject.connect(
            self.form.rb_abs_displacement,
            QtCore.SIGNAL("toggled(bool)"),
            self.abs_displacement_selected
        )
        QtCore.QObject.connect(
            self.form.rb_x_displacement,
            QtCore.SIGNAL("toggled(bool)"),
            self.x_displacement_selected
        )
        QtCore.QObject.connect(
            self.form.rb_y_displacement,
            QtCore.SIGNAL("toggled(bool)"),
            self.y_displacement_selected
        )
        QtCore.QObject.connect(
            self.form.rb_z_displacement,
            QtCore.SIGNAL("toggled(bool)"),
            self.z_displacement_selected
        )
        QtCore.QObject.connect(
            self.form.rb_temperature,
            QtCore.SIGNAL("toggled(bool)"),
            self.temperature_selected
        )
        QtCore.QObject.connect(
            self.form.rb_vm_stress,
            QtCore.SIGNAL("toggled(bool)"),
            self.vm_stress_selected
        )
        QtCore.QObject.connect(
            self.form.rb_maxprin,
            QtCore.SIGNAL("toggled(bool)"),
            self.max_prin_selected
        )
        QtCore.QObject.connect(
            self.form.rb_minprin,
            QtCore.SIGNAL("toggled(bool)"),
            self.min_prin_selected
        )
        QtCore.QObject.connect(
            self.form.rb_max_shear_stress,
            QtCore.SIGNAL("toggled(bool)"),
            self.max_shear_selected
        )
        QtCore.QObject.connect(
            self.form.rb_massflowrate,
            QtCore.SIGNAL("toggled(bool)"),
            self.massflowrate_selected
        )
        QtCore.QObject.connect(
            self.form.rb_networkpressure,
            QtCore.SIGNAL("toggled(bool)"),
            self.networkpressure_selected
        )
        QtCore.QObject.connect(
            self.form.rb_peeq,
            QtCore.SIGNAL("toggled(bool)"),
            self.peeq_selected
        )

        # displacement
        QtCore.QObject.connect(
            self.form.cb_show_displacement,
            QtCore.SIGNAL("clicked(bool)"),
            self.show_displacement
        )
        QtCore.QObject.connect(
            self.form.hsb_displacement_factor,
            QtCore.SIGNAL("valueChanged(int)"),
            self.hsb_disp_factor_changed
        )
        QtCore.QObject.connect(
            self.form.sb_displacement_factor,
            QtCore.SIGNAL("valueChanged(int)"),
            self.sb_disp_factor_changed
        )
        QtCore.QObject.connect(
            self.form.sb_displacement_factor_max,
            QtCore.SIGNAL("valueChanged(int)"),
            self.sb_disp_factor_max_changed
        )

        # user defined equation
        QtCore.QObject.connect(
            self.form.user_def_eq,
            QtCore.SIGNAL("textchanged()"),
            self.user_defined_text
        )
        QtCore.QObject.connect(
            self.form.calculate,
            QtCore.SIGNAL("clicked()"),
            self.calculate
        )

        self.update()
        if self.restore_result_settings_in_dialog:
            self.restore_result_dialog()
        else:
            self.restore_initial_result_dialog()
            # initialize scale factor for show displacement
            scale_factor = get_displacement_scale_factor(self.result_obj)
            self.form.sb_displacement_factor.setValue(scale_factor)
            self.form.hsb_displacement_factor.setValue(scale_factor)
            diggits_scale_factor = len(str(abs(int(scale_factor))))
            new_max_factor = 10 ** diggits_scale_factor
            self.form.sb_displacement_factor_max.setValue(new_max_factor)

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
        self.result_selected("Uabs", self.result_obj.DisplacementLengths, "mm")

    def x_displacement_selected(self, state):
        res_disp_u1 = self.get_scalar_disp_list(
            self.result_obj.DisplacementVectors, 0
        )
        self.result_selected("U1", res_disp_u1, "mm")

    def y_displacement_selected(self, state):
        res_disp_u2 = self.get_scalar_disp_list(
            self.result_obj.DisplacementVectors, 1
        )
        self.result_selected("U2", res_disp_u2, "mm")

    def z_displacement_selected(self, state):
        res_disp_u3 = self.get_scalar_disp_list(
            self.result_obj.DisplacementVectors, 2
        )
        self.result_selected("U3", res_disp_u3, "mm")

    def vm_stress_selected(self, state):
        self.result_selected("Sabs", self.result_obj.StressValues, "MPa")

    def max_shear_selected(self, state):
        self.result_selected("MaxShear", self.result_obj.MaxShear, "MPa")

    def max_prin_selected(self, state):
        self.result_selected("MaxPrin", self.result_obj.PrincipalMax, "MPa")

    def temperature_selected(self, state):
        self.result_selected("Temp", self.result_obj.Temperature, "K")

    def massflowrate_selected(self, state):
        self.result_selected("MFlow", self.result_obj.MassFlowRate, "kg/s")

    def networkpressure_selected(self, state):
        self.result_selected("NPress", self.result_obj.NetworkPressure, "MPa")

    def min_prin_selected(self, state):
        self.result_selected("MinPrin", self.result_obj.PrincipalMin, "MPa")

    def peeq_selected(self, state):
        self.result_selected("Peeq", self.result_obj.Peeq, "")

    def user_defined_text(self, equation):
        FreeCAD.FEM_dialog["results_type"] = "user"
        self.form.user_def_eq.toPlainText()

    def calculate(self):

        # Convert existing result values to numpy array
        # scalars
        P1 = np.array(self.result_obj.PrincipalMax)
        P2 = np.array(self.result_obj.PrincipalMed)
        P3 = np.array(self.result_obj.PrincipalMin)
        Von = np.array(self.result_obj.StressValues)
        Peeq = np.array(self.result_obj.Peeq)
        T = np.array(self.result_obj.Temperature)
        MF = np.array(self.result_obj.MassFlowRate)
        NP = np.array(self.result_obj.NetworkPressure)
        sxx = np.array(self.result_obj.NodeStressXX)
        syy = np.array(self.result_obj.NodeStressYY)
        szz = np.array(self.result_obj.NodeStressZZ)
        sxy = np.array(self.result_obj.NodeStressXY)
        sxz = np.array(self.result_obj.NodeStressXZ)
        syz = np.array(self.result_obj.NodeStressYZ)
        exx = np.array(self.result_obj.NodeStrainXX)
        eyy = np.array(self.result_obj.NodeStrainYY)
        ezz = np.array(self.result_obj.NodeStrainZZ)
        exy = np.array(self.result_obj.NodeStrainXY)
        exz = np.array(self.result_obj.NodeStrainXZ)
        eyz = np.array(self.result_obj.NodeStrainYZ)
        rx = np.array(self.result_obj.ReinforcementRatio_x)
        ry = np.array(self.result_obj.ReinforcementRatio_y)
        rz = np.array(self.result_obj.ReinforcementRatio_z)
        mc = np.array(self.result_obj.MohrCoulomb)
        # vectors
        dispvectors = np.array(self.result_obj.DisplacementVectors)
        x = np.array(dispvectors[:, 0])
        y = np.array(dispvectors[:, 1])
        z = np.array(dispvectors[:, 2])
        # If PSxVector is empty all UserDefined equation does not work
        if self.result_obj.PS1Vector:
            ps1vector = np.array(self.result_obj.PS1Vector)
            s1x = np.array(ps1vector[:, 0])
            s1y = np.array(ps1vector[:, 1])
            s1z = np.array(ps1vector[:, 2])
        if self.result_obj.PS2Vector:
            ps2vector = np.array(self.result_obj.PS2Vector)
            s2x = np.array(ps2vector[:, 0])
            s2y = np.array(ps2vector[:, 1])
            s2z = np.array(ps2vector[:, 2])
        if self.result_obj.PS3Vector:
            ps3vector = np.array(self.result_obj.PS1Vector)
            s3x = np.array(ps3vector[:, 0])
            s3y = np.array(ps3vector[:, 1])
            s3z = np.array(ps3vector[:, 2])

        FreeCAD.FEM_dialog["results_type"] = "None"
        self.update()
        self.restore_result_dialog()
        userdefined_eq = self.form.user_def_eq.toPlainText()  # Get equation to be used
        UserDefinedFormula = eval(userdefined_eq).tolist()
        self.result_obj.UserDefined = UserDefinedFormula
        minm = min(UserDefinedFormula)
        avg = sum(UserDefinedFormula) / len(UserDefinedFormula)
        maxm = max(UserDefinedFormula)
        self.update_colors_stats(UserDefinedFormula, "", minm, avg, maxm)

        # Dummy use of the variables to get around flake8 error
        del x, y, z, T, Von, Peeq, P1, P2, P3
        del sxx, syy, szz, sxy, sxz, syz
        del exx, eyy, ezz, exy, exz, eyz
        del MF, NP, rx, ry, rz, mc
        del s1x, s1y, s1z, s2x, s2y, s2z, s3x, s3y, s3z

    def get_scalar_disp_list(self, vector_list, axis):
        # list is needed, as zib-object is not subscriptable in py3
        d = list(zip(*self.result_obj.DisplacementVectors))
        scalar_list = list(d[axis])
        return scalar_list

    def result_selected(self, res_type, res_values, res_unit):
        # What is the FreeCAD.FEM_dialog for ???
        # Where is it initialized ?
        FreeCAD.FEM_dialog["results_type"] = res_type
        (minm, avg, maxm) = self.get_result_stats(res_type)
        self.update_colors_stats(res_values, res_unit, minm, avg, maxm)

    def update_colors_stats(self, res_values, res_unit, minm, avg, maxm):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.mesh_obj.ViewObject.setNodeColorByScalars(
                self.result_obj.NodeNumbers,
                res_values
            )
        self.set_result_stats(res_unit, minm, avg, maxm)
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
            self.mesh_obj.ViewObject.setNodeDisplacementByVectors(
                self.result_obj.NodeNumbers,
                self.result_obj.DisplacementVectors
            )
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
                error_message = (
                    'FEM: Graphical bending stress output '
                    'for beam or shell FEM Meshes not yet supported.\n'
                )
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
        # if the tasks panel is called from Command obj is not in edit mode
        # thus reset edit does not close the dialog, maybe don't call but set in edit instead
        FreeCADGui.Control.closeDialog()
        FreeCADGui.ActiveDocument.resetEdit()


# helper
def hide_femmeshes_postpiplines():
    # hide all visible FEM mesh objects and VTK FemPostPipeline objects
    for o in FreeCAD.ActiveDocument.Objects:
        if o.isDerivedFrom("Fem::FemMeshObject") or o.isDerivedFrom("Fem::FemPostPipeline"):
            o.ViewObject.hide()


def hide_parts_constraints():
    from FemGui import getActiveAnalysis
    fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
    hide_constraints = fem_prefs.GetBool("HideConstraint", False)
    if hide_constraints:
        for o in FreeCAD.ActiveDocument.Objects:
            if o.isDerivedFrom('Fem::FemAnalysis'):
                for acnstrmesh in getActiveAnalysis().Group:
                    if "Constraint" in acnstrmesh.TypeId:
                        acnstrmesh.ViewObject.Visibility = False
                break


def get_displacement_scale_factor(res_obj):
    node_items = res_obj.Mesh.FemMesh.Nodes.items()
    displacements = res_obj.DisplacementVectors
    x_max, y_max, z_max = map(max, zip(*displacements))
    positions = []  # list of node vectors
    for k, v in node_items:
        positions.append(v)
    p_x_max, p_y_max, p_z_max = map(max, zip(*positions))
    p_x_min, p_y_min, p_z_min = map(min, zip(*positions))
    x_span = abs(p_x_max - p_x_min)
    y_span = abs(p_y_max - p_y_min)
    z_span = abs(p_z_max - p_z_min)
    span = max(x_span, y_span, z_span)
    max_disp = max(x_max, y_max, z_max)
    # FIXME - add max_allowed_disp to Preferences
    max_allowed_disp = 0.01 * span
    scale = max_allowed_disp / max_disp
    return scale

# ***************************************************************************
# *   Copyright (c) 2015 Qingfeng Xia <qingfeng.xia()eng.ox.ac.uk>          *
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

__title__ = "FreeCAD result mechanical task panel"
__author__ = "Qingfeng Xia, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package view_result_mechanical
#  \ingroup FEM
#  \brief task panel for mechanical ResultObjectPython

try:
    import matplotlib
    matplotlib.use("Qt5Agg")
except Exception:
    print("Failed to set matplotlib backend to Qt5Agg")

import matplotlib.pyplot as plt
import numpy as np

from PySide import QtCore
from PySide import QtGui
from PySide.QtCore import Qt
from PySide.QtGui import QApplication

import FreeCAD
import FreeCADGui

import femresult.resulttools as resulttools

translate = FreeCAD.Qt.translate


class _TaskPanel:
    """
    The task panel for the post-processing
    """

    def __init__(self, obj):
        self.result_obj = obj
        self.mesh_obj = self.result_obj.Mesh
        # task panel should be started by use of setEdit of view provider
        # in view provider checks: Mesh, active analysis and
        # if Mesh and result are in active analysis
        # activate the result mesh object
        self.mesh_obj.ViewObject.show()

        ui_path = FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/"
        self.result_widget = FreeCADGui.PySideUic.loadUi(ui_path + "ResultShow.ui")
        self.info_widget = FreeCADGui.PySideUic.loadUi(ui_path + "ResultHints.ui")
        self.form = [self.result_widget, self.info_widget]

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
            self.result_widget.rb_none, QtCore.SIGNAL("toggled(bool)"),
            self.none_selected
        )
        QtCore.QObject.connect(
            self.result_widget.rb_abs_displacement,
            QtCore.SIGNAL("toggled(bool)"),
            self.abs_displacement_selected
        )
        QtCore.QObject.connect(
            self.result_widget.rb_x_displacement,
            QtCore.SIGNAL("toggled(bool)"),
            self.x_displacement_selected
        )
        QtCore.QObject.connect(
            self.result_widget.rb_y_displacement,
            QtCore.SIGNAL("toggled(bool)"),
            self.y_displacement_selected
        )
        QtCore.QObject.connect(
            self.result_widget.rb_z_displacement,
            QtCore.SIGNAL("toggled(bool)"),
            self.z_displacement_selected
        )
        QtCore.QObject.connect(
            self.result_widget.rb_temperature,
            QtCore.SIGNAL("toggled(bool)"),
            self.temperature_selected
        )
        QtCore.QObject.connect(
            self.result_widget.rb_vm_stress,
            QtCore.SIGNAL("toggled(bool)"),
            self.vm_stress_selected
        )
        QtCore.QObject.connect(
            self.result_widget.rb_maxprin,
            QtCore.SIGNAL("toggled(bool)"),
            self.max_prin_selected
        )
        QtCore.QObject.connect(
            self.result_widget.rb_minprin,
            QtCore.SIGNAL("toggled(bool)"),
            self.min_prin_selected
        )
        QtCore.QObject.connect(
            self.result_widget.rb_max_shear_stress,
            QtCore.SIGNAL("toggled(bool)"),
            self.max_shear_selected
        )
        QtCore.QObject.connect(
            self.result_widget.rb_massflowrate,
            QtCore.SIGNAL("toggled(bool)"),
            self.massflowrate_selected
        )
        QtCore.QObject.connect(
            self.result_widget.rb_networkpressure,
            QtCore.SIGNAL("toggled(bool)"),
            self.networkpressure_selected
        )
        QtCore.QObject.connect(
            self.result_widget.rb_peeq,
            QtCore.SIGNAL("toggled(bool)"),
            self.peeq_selected
        )

        # stats
        self.result_widget.show_histogram.clicked.connect(
            self.show_histogram_clicked
        )

        # displacement
        QtCore.QObject.connect(
            self.result_widget.cb_show_displacement,
            QtCore.SIGNAL("clicked(bool)"),
            self.show_displacement
        )
        QtCore.QObject.connect(
            self.result_widget.hsb_displacement_factor,
            QtCore.SIGNAL("valueChanged(int)"),
            self.hsb_disp_factor_changed
        )

        self.result_widget.sb_displacement_factor.valueChanged.connect(
            self.sb_disp_factor_changed
        )
        self.result_widget.sb_displacement_factor_max.valueChanged.connect(
            self.sb_disp_factor_max_changed
        )

        # user defined equation
        self.result_widget.user_def_eq.textChanged.connect(
            self.user_defined_text
        )
        QtCore.QObject.connect(
            self.result_widget.calculate,
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
            self.result_widget.sb_displacement_factor_max.setValue(10. * scale_factor)
            self.result_widget.sb_displacement_factor.setValue(scale_factor)

    def restore_result_dialog(self):
        try:
            rt = FreeCAD.FEM_dialog["results_type"]
            if rt == "None":
                self.result_widget.rb_none.setChecked(True)
                self.none_selected(True)
            elif rt == "Uabs":
                self.result_widget.rb_abs_displacement.setChecked(True)
                self.abs_displacement_selected(True)
            elif rt == "U1":
                self.result_widget.rb_x_displacement.setChecked(True)
                self.x_displacement_selected(True)
            elif rt == "U2":
                self.result_widget.rb_y_displacement.setChecked(True)
                self.y_displacement_selected(True)
            elif rt == "U3":
                self.result_widget.rb_z_displacement.setChecked(True)
                self.z_displacement_selected(True)
            elif rt == "Temp":
                self.result_widget.rb_temperature.setChecked(True)
                self.temperature_selected(True)
            elif rt == "Sabs":
                self.result_widget.rb_vm_stress.setChecked(True)
                self.vm_stress_selected(True)
            elif rt == "MaxPrin":
                self.result_widget.rb_maxprin.setChecked(True)
                self.max_prin_selected(True)
            elif rt == "MinPrin":
                self.result_widget.rb_minprin.setChecked(True)
                self.min_prin_selected(True)
            elif rt == "MaxShear":
                self.result_widget.rb_max_shear_stress.setChecked(True)
                self.max_shear_selected(True)
            elif rt == "MFlow":
                self.result_widget.rb_massflowrate.setChecked(True)
                self.massflowrate_selected(True)
            elif rt == "NPress":
                self.result_widget.rb_networkpressure.setChecked(True)
                self.networkpressure_selected(True)
            elif rt == "Peeq":
                self.result_widget.rb_peeq.setChecked(True)
                self.peeq_selected(True)

            sd = FreeCAD.FEM_dialog["show_disp"]
            self.result_widget.cb_show_displacement.setChecked(sd)
            self.show_displacement(sd)

            df = FreeCAD.FEM_dialog["disp_factor"]
            dfm = FreeCAD.FEM_dialog["disp_factor_max"]
            # self.result_widget.hsb_displacement_factor.setMaximum(dfm)
            # self.result_widget.hsb_displacement_factor.setValue(df)
            self.result_widget.sb_displacement_factor_max.setValue(dfm)
            self.result_widget.sb_displacement_factor.setValue(df)
        except Exception:
            self.restore_initial_result_dialog()

    def restore_initial_result_dialog(self):
        # initialize FreeCAD.FEM_dialog and set standard values
        # the FEM result mechanical task panel restore values
        # are saved in a dictionary which is an attribute of FreeCAD
        # the name is FEM_dialog
        # in python console after result task panel has been opened once
        # FreeCAD.FEM_dialog or FreeCAD.__dir__()
        # This is not smart at all IMHO (Bernd)
        # It was added with commit 3a7772d
        # https://github.com/FreeCAD/FreeCAD/commit/3a7772d
        FreeCAD.FEM_dialog = {
            "results_type": "None",
            "show_disp": False,
            "disp_factor": 0.,
            "disp_factor_max": 100.
        }
        self.result_widget.sb_displacement_factor_max.setValue(100.)    # init non standard values

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def get_result_stats(self, type_name):
        return resulttools.get_stats(self.result_obj, type_name)

    def none_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "None"
        self.set_result_stats("mm", 0.0, 0.0)
        self.reset_mesh_color()
        if len(plt.get_fignums()) > 0:
            plt.close()

    # if an analysis has different result types and one has
    # stress and the other not the restore result dialog
    # could trigger stress selected for a result object
    # which has not stress
    # see https://forum.freecad.org/viewtopic.php?f=18&t=39162
    # check if the results len is not 0 on any selected method

    def abs_displacement_selected(self, state):
        if len(self.result_obj.DisplacementLengths) > 0:
            self.result_selected(
                "Uabs",
                self.result_obj.DisplacementLengths,
                "mm",
                translate("FEM", "Displacement Magnitude")
            )
        else:
            self.result_widget.rb_none.setChecked(True)
            self.none_selected(True)

    def x_displacement_selected(self, state):
        if len(self.result_obj.DisplacementVectors) > 0:
            res_disp_u1 = self.get_scalar_disp_list(
                self.result_obj.DisplacementVectors, 0
            )
            self.result_selected(
                "U1",
                res_disp_u1,
                "mm",
                translate("FEM", "Displacement X")
            )
        else:
            self.result_widget.rb_none.setChecked(True)
            self.none_selected(True)

    def y_displacement_selected(self, state):
        if len(self.result_obj.DisplacementVectors) > 0:
            res_disp_u2 = self.get_scalar_disp_list(
                self.result_obj.DisplacementVectors, 1
            )
            self.result_selected(
                "U2",
                res_disp_u2,
                "mm",
                translate("FEM", "Displacement Y")
            )
        else:
            self.result_widget.rb_none.setChecked(True)
            self.none_selected(True)

    def z_displacement_selected(self, state):
        if len(self.result_obj.DisplacementVectors) > 0:
            res_disp_u3 = self.get_scalar_disp_list(
                self.result_obj.DisplacementVectors, 2
            )
            self.result_selected(
                "U3",
                res_disp_u3,
                "mm",
                translate("FEM", "Displacement Z")
            )
        else:
            self.result_widget.rb_none.setChecked(True)
            self.none_selected(True)

    def vm_stress_selected(self, state):
        if len(self.result_obj.vonMises) > 0:
            self.result_selected(
                "Sabs",
                self.result_obj.vonMises,
                "MPa",
                translate("FEM", "von Mises Stress")
            )
        else:
            self.result_widget.rb_none.setChecked(True)
            self.none_selected(True)

    def max_shear_selected(self, state):
        if len(self.result_obj.MaxShear) > 0:
            self.result_selected(
                "MaxShear",
                self.result_obj.MaxShear,
                "MPa",
                translate("FEM", "Max Shear Stress")
            )
        else:
            self.result_widget.rb_none.setChecked(True)
            self.none_selected(True)

    def max_prin_selected(self, state):
        if len(self.result_obj.PrincipalMax) > 0:
            self.result_selected(
                "MaxPrin",
                self.result_obj.PrincipalMax,
                "MPa",
                translate("FEM", "Max Principal Stress")
            )
        else:
            self.result_widget.rb_none.setChecked(True)
            self.none_selected(True)

    def temperature_selected(self, state):
        if len(self.result_obj.Temperature) > 0:
            self.result_selected(
                "Temp",
                self.result_obj.Temperature,
                "K",
                translate("FEM", "Temperature")
            )
        else:
            self.result_widget.rb_none.setChecked(True)
            self.none_selected(True)

    def massflowrate_selected(self, state):
        if len(self.result_obj.MassFlowRate) > 0:
            self.result_selected(
                "MFlow",
                self.result_obj.MassFlowRate,
                "kg/s",
                translate("FEM", "Mass Flow Rate")
            )
        else:
            self.result_widget.rb_none.setChecked(True)
            self.none_selected(True)

    def networkpressure_selected(self, state):
        if len(self.result_obj.NetworkPressure) > 0:
            self.result_selected(
                "NPress",
                self.result_obj.NetworkPressure,
                "MPa",
                translate("FEM", "Network Pressure")
            )
        else:
            self.result_widget.rb_none.setChecked(True)
            self.none_selected(True)

    def min_prin_selected(self, state):
        if len(self.result_obj.PrincipalMin) > 0:
            self.result_selected(
                "MinPrin",
                self.result_obj.PrincipalMin,
                "MPa",
                translate("FEM", "Min Principal Stress")
            )
        else:
            self.result_widget.rb_none.setChecked(True)
            self.none_selected(True)

    def peeq_selected(self, state):
        if len(self.result_obj.Peeq) > 0:
            self.result_selected(
                "Peeq",
                self.result_obj.Peeq,
                "",
                translate("FEM", "Equivalent Plastic Strain")
            )
        else:
            self.result_widget.rb_none.setChecked(True)
            self.none_selected(True)

    def show_histogram_clicked(self):
        if len(plt.get_fignums()) > 0:
            plt.show()
        else:
            # if the plot was closed and subsequently the Histogram button was pressed again
            # we have valid settings, but must restore the dialog to refill the plot content
            # see https://github.com/FreeCAD/FreeCAD/issues/6975
            if FreeCAD.FEM_dialog["results_type"] != "None":
                self.restore_result_dialog()
            if len(plt.get_fignums()) > 0:
                plt.show()
            else:
                QtGui.QMessageBox.information(
                    None,
                    self.result_obj.Label + " - " + translate("FEM", "Information"),
                    translate("FEM", "No histogram available.\nPlease select a result type first.")
                )

    def user_defined_text(self, equation):
        FreeCAD.FEM_dialog["results_type"] = "user"
        self.result_widget.user_def_eq.toPlainText()

    def calculate(self):

        # Convert existing result values to numpy array
        # scalars
        P1 = np.array(self.result_obj.PrincipalMax)
        P2 = np.array(self.result_obj.PrincipalMed)
        P3 = np.array(self.result_obj.PrincipalMin)
        MS = np.array(self.result_obj.MaxShear)
        vM = np.array(self.result_obj.vonMises)
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
        s1x, s1y, s1z = np.array([]), np.array([]), np.array([])
        s2x, s2y, s2z = np.array([]), np.array([]), np.array([])
        s3x, s3y, s3z = np.array([]), np.array([]), np.array([])
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
            ps3vector = np.array(self.result_obj.PS3Vector)
            s3x = np.array(ps3vector[:, 0])
            s3y = np.array(ps3vector[:, 1])
            s3z = np.array(ps3vector[:, 2])

        FreeCAD.FEM_dialog["results_type"] = "None"
        self.update()
        self.restore_result_dialog()
        userdefined_eq = self.result_widget.user_def_eq.toPlainText()  # Get equation to be used

        # https://forum.freecad.org/viewtopic.php?f=18&t=42425&start=10#p368774 ff
        # https://github.com/FreeCAD/FreeCAD/pull/3020
        from ply import lex
        from ply import yacc
        import femtools.tokrules as tokrules
        identifiers = [
            "x", "y", "z", "T", "vM", "Peeq", "P1", "P2", "P3",
            "sxx", "syy", "szz", "sxy", "sxz", "syz",
            "exx", "eyy", "ezz", "exy", "exz", "eyz",
            "MS", "MF", "NP", "rx", "ry", "rz", "mc",
            "s1x", "s1y", "s1z", "s2x", "s2y", "s2z", "s3x", "s3y", "s3z"
        ]
        tokrules.names = {}
        for i in identifiers:
            tokrules.names[i] = locals()[i]

        lexer = lex.lex(module=tokrules)
        yacc.parse(input="UserDefinedFormula={0}".format(userdefined_eq), lexer=lexer)
        UserDefinedFormula = tokrules.names["UserDefinedFormula"].tolist()
        tokrules.names = {}
        # UserDefinedFormula = eval(userdefined_eq).tolist()

        if UserDefinedFormula:
            self.result_obj.UserDefined = UserDefinedFormula
            minm = min(UserDefinedFormula)
            maxm = max(UserDefinedFormula)
            self.update_colors_stats(UserDefinedFormula, "", minm, maxm)

        # finally we must recompute the result_obj
        self.result_obj.Document.recompute()

    def get_scalar_disp_list(self, vector_list, axis):
        # list is needed, as zib-object is not subscriptable in py3
        d = list(zip(*self.result_obj.DisplacementVectors))
        scalar_list = list(d[axis])
        return scalar_list

    def result_selected(self, res_type, res_values, res_unit, res_title):
        FreeCAD.FEM_dialog["results_type"] = res_type
        (minm, maxm) = self.get_result_stats(res_type)
        self.update_colors_stats(res_values, res_unit, minm, maxm)

        if len(plt.get_fignums()) > 0:
            plt.close()
        plt.ioff()  # disable interactive mode so we have full control when plot is shown
        plt.figure(res_title)
        plt.hist(res_values, bins=50, alpha=0.5, facecolor="blue")
        plt.xlabel(res_unit)
        plt.title(translate("FEM", "Histogram of {}").format(res_title))
        plt.ylabel(translate("FEM", "Nodes"))
        plt.grid(True)
        fig_manager = plt.get_current_fig_manager()
        # Lines below tells Qt that plot widget/dialog should be kept on top of its parent,
        # its parent being defined as FC main window
        # "Tool" type also is non modal, and dialog doesn't add a tab in the OS task bar
        # See Qt::WindowFlags for more details
        fig_manager.window.setParent(FreeCADGui.getMainWindow())
        fig_manager.window.setWindowFlag(QtCore.Qt.Tool)

    def update_colors_stats(self, res_values, res_unit, minm, maxm):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if self.suitable_results:
            self.mesh_obj.ViewObject.setNodeColorByScalars(
                self.result_obj.NodeNumbers,
                res_values
            )
        self.set_result_stats(res_unit, minm, maxm)
        QtGui.QApplication.restoreOverrideCursor()

    def set_result_stats(self, unit, minm, maxm):
        self.result_widget.le_min.setProperty("unit", unit)
        self.result_widget.le_min.setProperty("rawText", "{:.6} {}".format(minm, unit))
        self.result_widget.le_max.setProperty("unit", unit)
        self.result_widget.le_max.setProperty("rawText", "{:.6} {}".format(maxm, unit))

    def update_displacement(self, factor=None):
        if factor is None:
            if FreeCAD.FEM_dialog["show_disp"]:
                factor = self.result_widget.sb_displacement_factor.value()
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
        self.result_widget.sb_displacement_factor.setValue(
            value / 100. * self.result_widget.sb_displacement_factor_max.value()
        )
        self.update_displacement()

    def sb_disp_factor_max_changed(self, value):
        FreeCAD.FEM_dialog["disp_factor_max"] = value
        if value < self.result_widget.sb_displacement_factor.value():
            self.result_widget.sb_displacement_factor.setValue(value)
        if value == 0.:
            self.result_widget.hsb_displacement_factor.setValue(0)
        else:
            self.result_widget.hsb_displacement_factor.setValue(
                round(self.result_widget.sb_displacement_factor.value() / value * 100.)
            )

    def sb_disp_factor_changed(self, value):
        FreeCAD.FEM_dialog["disp_factor"] = value
        if value > self.result_widget.sb_displacement_factor_max.value():
            self.result_widget.sb_displacement_factor.setValue(
                self.result_widget.sb_displacement_factor_max.value()
            )
        if self.result_widget.sb_displacement_factor_max.value() == 0.:
            self.result_widget.hsb_displacement_factor.setValue(0.)
        else:
            self.result_widget.hsb_displacement_factor.setValue(
                round(value / self.result_widget.sb_displacement_factor_max.value() * 100.)
            )

    def disable_empty_result_buttons(self):
        """ disable radio buttons if result does not exists in result object"""
        """assignments
        DisplacementLengths --> rb_abs_displacement
        DisplacementVectors --> rb_x_displacement, rb_y_displacement, rb_z_displacement
        Temperature         --> rb_temperature
        vonMises            --> rb_vm_stress
        PrincipalMax        --> rb_maxprin
        PrincipalMin        --> rb_minprin
        MaxShear            --> rb_max_shear_stress
        MassFlowRate        --> rb_massflowrate
        NetworkPressure     --> rb_networkpressure
        Peeq                --> rb_peeq"""
        if len(self.result_obj.DisplacementLengths) == 0:
            self.result_widget.rb_abs_displacement.setEnabled(0)
        if len(self.result_obj.DisplacementVectors) == 0:
            self.result_widget.rb_x_displacement.setEnabled(0)
            self.result_widget.rb_y_displacement.setEnabled(0)
            self.result_widget.rb_z_displacement.setEnabled(0)
        if len(self.result_obj.Temperature) == 0:
            self.result_widget.rb_temperature.setEnabled(0)
        if len(self.result_obj.vonMises) == 0:
            self.result_widget.rb_vm_stress.setEnabled(0)
        if len(self.result_obj.PrincipalMax) == 0:
            self.result_widget.rb_maxprin.setEnabled(0)
        if len(self.result_obj.PrincipalMin) == 0:
            self.result_widget.rb_minprin.setEnabled(0)
        if len(self.result_obj.MaxShear) == 0:
            self.result_widget.rb_max_shear_stress.setEnabled(0)
        if len(self.result_obj.MassFlowRate) == 0:
            self.result_widget.rb_massflowrate.setEnabled(0)
        if len(self.result_obj.NetworkPressure) == 0:
            self.result_widget.rb_networkpressure.setEnabled(0)
        if len(self.result_obj.Peeq) == 0:
            self.result_widget.rb_peeq.setEnabled(0)

    def update(self):
        self.reset_result_mesh()
        self.suitable_results = False
        self.disable_empty_result_buttons()
        if self.mesh_obj.FemMesh.NodeCount == 0:
            the_error_messagetext = (
                "FEM: there are no nodes in result mesh, "
                "there will be nothing to show."
            )
            error_message = (
                translate("FEM", the_error_messagetext) + "\n"
            )
            FreeCAD.Console.PrintError(error_message)
            QtGui.QMessageBox.critical(
                None,
                translate("FEM", "Empty result mesh"),
                error_message
            )
        elif (self.mesh_obj.FemMesh.NodeCount == len(self.result_obj.NodeNumbers)):
            self.suitable_results = True
            hide_parts_constraints()
        else:
            if not self.mesh_obj.FemMesh.VolumeCount:
                the_error_messagetext = (
                    "FEM: Graphical bending stress output "
                    "for beam or shell FEM Meshes not yet supported."
                )
                error_message = (
                    translate("FEM", the_error_messagetext) + "\n"
                )
                FreeCAD.Console.PrintError(error_message)
                QtGui.QMessageBox.critical(
                    None,
                    translate("FEM", "No result object"),
                    error_message
                )
            else:
                the_error_messagetext = (
                    "FEM: Result node numbers are "
                    "not equal to FEM Mesh NodeCount."
                )
                error_message = translate("FEM", the_error_messagetext) + "\n"
                FreeCAD.Console.PrintError(error_message)
                QtGui.QMessageBox.critical(
                    None,
                    translate("FEM", "No result object"),
                    error_message
                )

    def reset_mesh_color(self):
        self.mesh_obj.ViewObject.NodeColor = {}
        self.mesh_obj.ViewObject.ElementColor = {}
        self.mesh_obj.ViewObject.resetNodeColor()

    def reset_result_mesh(self):
        self.mesh_obj.ViewObject.resetNodeDisplacement()
        self.reset_mesh_color()

    def reject(self):
        self.reset_result_mesh()
        plt.close()
        # if the tasks panel is called from Command obj is not in edit mode
        # thus reset edit does not close the dialog, maybe don't call but set in edit instead
        FreeCADGui.Control.closeDialog()
        FreeCADGui.ActiveDocument.resetEdit()


# helper
def hide_parts_constraints():
    from FemGui import getActiveAnalysis
    fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
    hide_constraints = fem_prefs.GetBool("HideConstraint", False)
    if hide_constraints:
        for o in FreeCAD.ActiveDocument.Objects:
            if o.isDerivedFrom("Fem::FemAnalysis"):
                for acnstrmesh in getActiveAnalysis().Group:
                    if "Constraint" in acnstrmesh.TypeId:
                        acnstrmesh.ViewObject.Visibility = False
                break


def get_displacement_scale_factor(res_obj):
    node_items = res_obj.Mesh.FemMesh.Nodes.items()
    displacements = res_obj.DisplacementVectors
    # use standard scale if there are no displacements in result object
    if len(displacements) == 0:
        return 1
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
    if max_disp == 0.0:
        return 0.0  # avoid float division by zero
    # FIXME - add max_allowed_disp to Preferences
    max_allowed_disp = 0.01 * span
    scale = max_allowed_disp / max_disp
    return scale

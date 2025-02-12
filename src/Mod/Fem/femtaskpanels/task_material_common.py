# ***************************************************************************
# *   Copyright (c) 2013 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

__title__ = "FreeCAD FEM material task panel for the document object"
__author__ = "Juergen Riegel, Bernd Hahnebach, Qingfeng Xia"
__url__ = "https://www.freecad.org"

## @package task_material_common
#  \ingroup FEM
#  \brief FreeCAD FEM _ViewProviderFemMaterial
#  \brief task panel for common material object

from PySide import QtCore

import FreeCAD
import FreeCADGui
from FreeCAD import Units
import Materials
import MatGui

from femguiutils import selection_widgets
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):
    """
    The editmode TaskPanel for FemMaterial objects
    """

    def __init__(self, obj):
        super().__init__(obj)

        # FreeCAD material dictionary of current material
        self.material = self.obj.Material
        self.uuid = self.obj.UUID
        self.material_manager = Materials.MaterialManager()

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/Material.ui"
        )
        self.material_tree = MatGui.MaterialTreeWidget(self.parameterWidget.wgt_material_tree)
        self.material_tree.expanded = False
        self.material_tree.IncludeEmptyFolders = False
        self.material_tree.IncludeEmptyLibraries = False

        QtCore.QObject.connect(
            self.parameterWidget.chbu_allow_edit,
            QtCore.SIGNAL("clicked()"),
            self.toggleInputFieldsReadOnly,
        )
        # basic properties must be provided
        QtCore.QObject.connect(
            self.parameterWidget.qsb_density,
            QtCore.SIGNAL("editingFinished()"),
            self.density_changed,
        )
        # mechanical properties
        QtCore.QObject.connect(
            self.parameterWidget.qsb_young_modulus,
            QtCore.SIGNAL("editingFinished()"),
            self.ym_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_poisson_ratio,
            QtCore.SIGNAL("editingFinished()"),
            self.pr_changed,
        )
        # thermal properties
        QtCore.QObject.connect(
            self.parameterWidget.qsb_thermal_conductivity,
            QtCore.SIGNAL("editingFinished()"),
            self.tc_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_expansion_coefficient,
            QtCore.SIGNAL("editingFinished()"),
            self.tec_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_expansion_reference_temperature,
            QtCore.SIGNAL("editingFinished()"),
            self.tec_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_specific_heat,
            QtCore.SIGNAL("editingFinished()"),
            self.sh_changed,
        )
        # fluidic properties, only volumetric thermal expansion coeff makes sense
        QtCore.QObject.connect(
            self.parameterWidget.qsb_kinematic_viscosity,
            QtCore.SIGNAL("editingFinished()"),
            self.kinematic_viscosity_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.wgt_material_tree,
            QtCore.SIGNAL("onMaterial(QString)"),
            self.set_from_editor,
        )

        # init all parameter input files with read only
        self.parameterWidget.chbu_allow_edit.setCheckState(QtCore.Qt.CheckState.Unchecked)
        self.toggleInputFieldsReadOnly()

        # hide some groupBox according to material category
        if self.obj.Category == "Fluid":
            self.filter_models(self.obj.Category)
            self.parameterWidget.groupBox_mechanical.setVisible(0)
        else:
            self.parameterWidget.groupBox_fluidic.setVisible(0)

        # geometry selection widget
        self.selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Solid", "Face", "Edge"], False, True
        )  # start with Solid in list!

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

        # check references, has to be after initialisation of selectionWidget
        self.material_tree.UUID = self.uuid
        self.set_mat_params_in_input_fields(self.material)

        self.selectionWidget.has_equal_references_shape_types()

    # leave task panel ***************************************************************************
    def accept(self):
        if self.selectionWidget.has_equal_references_shape_types():
            self.obj.Material = self.material
            self.obj.UUID = self.uuid
            self.obj.References = self.selectionWidget.references

        self.selectionWidget.finish_selection()
        return super().accept()

    def reject(self):
        self.selectionWidget.finish_selection()
        return super().reject()

    # choose material ****************************************************************************
    def filter_models(self, category):
        material_filter = Materials.MaterialFilter()
        models = set(Materials.ModelManager().Models.keys())
        uuids = Materials.UUIDs()
        if category == "Fluid":
            material_filter.RequiredModels = [uuids.Fluid]
            self.material_tree.setFilter(material_filter)

    def toggleInputFieldsReadOnly(self):
        if self.parameterWidget.chbu_allow_edit.isChecked():
            self.parameterWidget.qsb_density.setDisabled(False)
            self.parameterWidget.qsb_young_modulus.setDisabled(False)
            self.parameterWidget.qsb_poisson_ratio.setDisabled(False)
            self.parameterWidget.qsb_thermal_conductivity.setDisabled(False)
            self.parameterWidget.qsb_expansion_coefficient.setDisabled(False)
            self.parameterWidget.qsb_expansion_reference_temperature.setDisabled(False)
            self.parameterWidget.qsb_specific_heat.setDisabled(False)
            self.parameterWidget.qsb_kinematic_viscosity.setDisabled(False)
            self.parameterWidget.wgt_material_tree.setEnabled(False)
            self.uuid = ""
            self.mat_from_input_fields()
        else:
            self.parameterWidget.qsb_density.setDisabled(True)
            self.parameterWidget.qsb_young_modulus.setDisabled(True)
            self.parameterWidget.qsb_poisson_ratio.setDisabled(True)
            self.parameterWidget.qsb_thermal_conductivity.setDisabled(True)
            self.parameterWidget.qsb_expansion_coefficient.setDisabled(True)
            self.parameterWidget.qsb_expansion_reference_temperature.setDisabled(True)
            self.parameterWidget.qsb_specific_heat.setDisabled(True)
            self.parameterWidget.qsb_kinematic_viscosity.setDisabled(True)
            self.parameterWidget.wgt_material_tree.setEnabled(True)
            self.set_from_editor(self.material_tree.UUID)

    # material parameter input fields ************************************************************
    # mechanical input fields
    def ym_changed(self):
        if self.parameterWidget.chbu_allow_edit.isChecked():
            self.material["YoungsModulus"] = self.parameterWidget.qsb_young_modulus.property(
                "value"
            ).UserString

    def density_changed(self):
        if self.parameterWidget.chbu_allow_edit.isChecked():
            self.material["Density"] = self.parameterWidget.qsb_density.property("value").UserString

    def pr_changed(self):
        if self.parameterWidget.chbu_allow_edit.isChecked():
            self.material["PoissonRatio"] = str(
                self.parameterWidget.qsb_poisson_ratio.property("value").Value
            )

    # thermal input fields
    def tc_changed(self):
        if self.parameterWidget.chbu_allow_edit.isChecked():
            self.material["ThermalConductivity"] = (
                self.parameterWidget.qsb_thermal_conductivity.property("value").UserString
            )

    def tec_changed(self):
        if self.parameterWidget.chbu_allow_edit.isChecked():
            self.material["ThermalExpansionCoefficient"] = (
                self.parameterWidget.qsb_expansion_coefficient.property("value").UserString
            )

    def tec_changed(self):
        if self.parameterWidget.chbu_allow_edit.isChecked():
            self.material["ThermalExpansionReferenceTemperature"] = (
                self.parameterWidget.qsb_expansion_reference_temperature.property(
                    "value"
                ).UserString
            )

    def sh_changed(self):
        if self.parameterWidget.chbu_allow_edit.isChecked():
            self.material["SpecificHeat"] = self.parameterWidget.qsb_specific_heat.property(
                "value"
            ).UserString

    # fluidic input fields
    def kinematic_viscosity_changed(self):
        if self.parameterWidget.chbu_allow_edit.isChecked():
            self.material["KinematicViscosity"] = (
                self.parameterWidget.qsb_kinematic_viscosity.property("value").UserString
            )

    def set_mat_params_in_input_fields(self, matmap):
        if "YoungsModulus" in matmap:
            self.parameterWidget.qsb_young_modulus.setProperty(
                "value", Units.Quantity(matmap["YoungsModulus"])
            )
        else:
            self.parameterWidget.qsb_young_modulus.setProperty("rawValue", 0.0)

        if "PoissonRatio" in matmap:
            self.parameterWidget.qsb_poisson_ratio.setProperty(
                "value", Units.Quantity(matmap["PoissonRatio"])
            )
        else:
            self.parameterWidget.qsb_poisson_ratio.setProperty("rawValue", 0.0)
        # Fluidic properties
        if "KinematicViscosity" in matmap:
            self.parameterWidget.qsb_kinematic_viscosity.setProperty(
                "value", Units.Quantity(matmap["KinematicViscosity"])
            )
        else:
            self.parameterWidget.qsb_kinematic_viscosity.setProperty("rawValue", 0.0)

        if "Density" in matmap:
            self.parameterWidget.qsb_density.setProperty("value", Units.Quantity(matmap["Density"]))
        else:
            self.parameterWidget.qsb_density.setProperty("rawValue", 0.0)
        # thermal properties
        if "ThermalConductivity" in matmap:
            self.parameterWidget.qsb_thermal_conductivity.setProperty(
                "value", Units.Quantity(matmap["ThermalConductivity"])
            )
        else:
            self.parameterWidget.qsb_thermal_conductivity.setProperty("rawValue", 0.0)

        if "ThermalExpansionCoefficient" in matmap:
            v = Units.Quantity(matmap["ThermalExpansionCoefficient"])
            v.Format = {"Precision": 3}
            self.parameterWidget.qsb_expansion_coefficient.setProperty("value", v)
        else:
            self.parameterWidget.qsb_expansion_coefficient.setProperty("rawValue", 0.0)
        if "ThermalExpansionReferenceTemperature" in matmap:
            self.parameterWidget.qsb_expansion_reference_temperature.setProperty(
                "value", Units.Quantity(matmap["ThermalExpansionReferenceTemperature"])
            )
        else:
            self.parameterWidget.qsb_expansion_reference_temperature.setProperty("rawValue", 0.0)
        if "SpecificHeat" in matmap:
            self.parameterWidget.qsb_specific_heat.setProperty(
                "value", Units.Quantity(matmap["SpecificHeat"])
            )
        else:
            self.parameterWidget.qsb_specific_heat.setProperty("rawValue", 0.0)

    def set_from_editor(self, value):
        if not value:
            return
        mat = self.material_manager.getMaterial(value)
        self.material = mat.Properties
        self.uuid = mat.UUID
        self.set_mat_params_in_input_fields(self.material)

    def mat_from_input_fields(self):
        d = {}
        d["Name"] = "Custom"
        p = self.parameterWidget
        d["Density"] = p.qsb_density.property("value").UserString
        d["ThermalConductivity"] = p.qsb_thermal_conductivity.property("value").UserString
        d["ThermalExpansionCoefficient"] = p.qsb_expansion_coefficient.property("value").UserString
        d["ThermalExpansionReferenceTemperature"] = p.qsb_expansion_reference_temperature.property(
            "value"
        ).UserString
        d["SpecificHeat"] = p.qsb_specific_heat.property("value").UserString
        if self.obj.Category == "Solid":
            d["YoungsModulus"] = p.qsb_young_modulus.property("value").UserString
            d["PoissonRatio"] = str(p.qsb_poisson_ratio.property("value").Value)
        elif self.obj.Category == "Fluid":
            d["KinematicViscosity"] = p.qsb_kinematic_viscosity.property("value").UserString
        self.material = d

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
from PySide import QtGui

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
            self.parameterWidget.input_fd_density,
            QtCore.SIGNAL("editingFinished()"),
            self.density_changed,
        )
        # mechanical properties
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_young_modulus,
            QtCore.SIGNAL("editingFinished()"),
            self.ym_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.spinBox_poisson_ratio,
            QtCore.SIGNAL("editingFinished()"),
            self.pr_changed,
        )
        # thermal properties
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_thermal_conductivity,
            QtCore.SIGNAL("editingFinished()"),
            self.tc_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_expansion_coefficient,
            QtCore.SIGNAL("editingFinished()"),
            self.tec_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_specific_heat,
            QtCore.SIGNAL("editingFinished()"),
            self.sh_changed,
        )
        # fluidic properties, only volumetric thermal expansion coeff makes sense
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_kinematic_viscosity,
            QtCore.SIGNAL("editingFinished()"),
            self.kinematic_viscosity_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_vol_expansion_coefficient,
            QtCore.SIGNAL("editingFinished()"),
            self.vtec_changed,
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
        # note: input_fd_vol_expansion_coefficient is currently not used
        #       it might be used in future for solids
        if self.obj.Category == "Fluid":
            self.filter_models(self.obj.Category)
            self.parameterWidget.groupBox_mechanical.setVisible(0)
            self.parameterWidget.label_vol_expansion_coefficient.setVisible(0)
            self.parameterWidget.input_fd_vol_expansion_coefficient.setVisible(0)
        else:
            self.parameterWidget.groupBox_fluidic.setVisible(0)
            self.parameterWidget.label_vol_expansion_coefficient.setVisible(0)
            self.parameterWidget.input_fd_vol_expansion_coefficient.setVisible(0)

        # geometry selection widget
        self.selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Solid", "Face", "Edge"], False, True
        )  # start with Solid in list!

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

        # check references, has to be after initialisation of selectionWidget
        self.material_tree.UUID = self.get_material_uuid(self.material)
        self.check_material_keys()
        self.set_mat_params_in_input_fields(self.material)

        self.selectionWidget.has_equal_references_shape_types()

    # leave task panel ***************************************************************************
    def accept(self):
        if self.selectionWidget.has_equal_references_shape_types():
            self.do_not_set_thermal_zeros()
            from materialtools.cardutils import check_mat_units as checkunits

            if checkunits(self.material) is True:
                self.obj.Material = self.material
                self.obj.UUID = self.uuid
                self.obj.References = self.selectionWidget.references
            else:
                error_message = (
                    "Due to some wrong material quantity units in the changed "
                    "material data, the task panel changes where not accepted.\n"
                )
                FreeCAD.Console.PrintError(error_message)
                QtGui.QMessageBox.critical(None, "Material data not changed", error_message)
        self.selectionWidget.finish_selection()
        return super().accept()

    def reject(self):
        self.selectionWidget.finish_selection()
        return super().reject()

    def do_not_set_thermal_zeros(self):
        """thermal material parameter are set to 0.0 if not available
        this leads to wrong material values and to not finding the card
        on reopen the task pane, thus do not write thermal parameter,
        if they are 0.0
        """
        if Units.Quantity(self.material["ThermalConductivity"]) == 0.0:
            self.material.pop("ThermalConductivity", None)
            FreeCAD.Console.PrintMessage(
                "Zero ThermalConductivity value. "
                "This parameter is not saved in the material data.\n"
            )
        if Units.Quantity(self.material["ThermalExpansionCoefficient"]) == 0.0:
            self.material.pop("ThermalExpansionCoefficient", None)
            FreeCAD.Console.PrintMessage(
                "Zero ThermalExpansionCoefficient value. "
                "This parameter is not saved in the material data.\n"
            )
        if Units.Quantity(self.material["SpecificHeat"]) == 0.0:
            self.material.pop("SpecificHeat", None)
            FreeCAD.Console.PrintMessage(
                "Zero SpecificHeat value. This parameter is not saved in the material data.\n"
            )

    def isfloat(self, num):
        try:
            float(num)
            return True
        except ValueError:
            return False

    # choose material ****************************************************************************
    def filter_models(self, category):
        material_filter = Materials.MaterialFilter()
        models = set(Materials.ModelManager().Models.keys())
        uuids = Materials.UUIDs()
        if category == "Fluid":
            material_filter.RequiredModels = [uuids.Fluid]
            self.material_tree.setFilter(material_filter)

    def get_material_uuid(self, material):
        if self.uuid:
            try:
                self.material_manager.getMaterial(self.uuid)
                return self.uuid
            except:
                return ""

        if not self.material:
            return ""

        for a_mat in self.material_manager.Materials:
            # check if every item of the current material fits to a known material card
            # if all items were found we know it is the right card
            # we can hereby not simply perform
            # set(self.materials[a_mat].items()) ^ set(material.items())
            # because entries are often identical, just appear in the set in a different order
            unmatched_item = False
            a_mat_prop = self.material_manager.getMaterial(a_mat).Properties.items()
            for item in material.items():
                if item not in a_mat_prop:
                    unmatched_item = True
                    # often the difference is just a decimal e.g. "120" to "120.0"
                    # therefore first check if the item name exists
                    for a_mat_item in a_mat_prop:
                        if item[0] == a_mat_item[0]:
                            # now check if we have a number value in a unit
                            if item[1].split() and not self.isfloat(item[1].split()[0]):
                                break
                            if item[1].split() and float(item[1].split()[0]) == float(
                                a_mat_item[1].split()[0]
                            ):
                                unmatched_item = False
                            elif not item[1].split():
                                # handle the case where item[1] is an empty string
                                if not self.isfloat(item[1]):
                                    break
                                if float(item[1]) == float(a_mat_item[1]):
                                    unmatched_item = False
                    break
            if not unmatched_item:
                return a_mat
        return ""

    def toggleInputFieldsReadOnly(self):
        if self.parameterWidget.chbu_allow_edit.isChecked():
            self.parameterWidget.input_fd_density.setReadOnly(False)
            self.parameterWidget.input_fd_young_modulus.setReadOnly(False)
            self.parameterWidget.spinBox_poisson_ratio.setReadOnly(False)
            self.parameterWidget.input_fd_thermal_conductivity.setReadOnly(False)
            self.parameterWidget.input_fd_expansion_coefficient.setReadOnly(False)
            self.parameterWidget.input_fd_specific_heat.setReadOnly(False)
            self.parameterWidget.input_fd_kinematic_viscosity.setReadOnly(False)
            self.parameterWidget.input_fd_vol_expansion_coefficient.setReadOnly(False)
        else:
            self.parameterWidget.input_fd_density.setReadOnly(True)
            self.parameterWidget.input_fd_young_modulus.setReadOnly(True)
            self.parameterWidget.spinBox_poisson_ratio.setReadOnly(True)
            self.parameterWidget.input_fd_thermal_conductivity.setReadOnly(True)
            self.parameterWidget.input_fd_expansion_coefficient.setReadOnly(True)
            self.parameterWidget.input_fd_specific_heat.setReadOnly(True)
            self.parameterWidget.input_fd_kinematic_viscosity.setReadOnly(True)
            self.parameterWidget.input_fd_vol_expansion_coefficient.setReadOnly(True)

    # material parameter input fields ************************************************************
    def check_material_keys(self):
        # FreeCAD units definition is at file end of src/Base/Unit.cpp
        if not self.material:
            self.material["Name"] = "NoName"
        if "Density" in self.material:
            if "Density" not in str(Units.Unit(self.material["Density"])):
                FreeCAD.Console.PrintMessage(
                    "Density in material data seems to have no unit "
                    "or a wrong unit (reset the value): {}\n".format(self.material["Name"])
                )
                self.material["Density"] = "0 kg/m^3"
        else:
            if self.material["Name"] != "NoName":
                FreeCAD.Console.PrintMessage(
                    "Density not found in {}\n".format(self.material["Name"])
                )
            self.material["Density"] = "0 kg/m^3"
        if self.obj.Category == "Solid":
            # mechanical properties
            if "YoungsModulus" in self.material:
                # unit type of YoungsModulus is Pressure
                if "Pressure" not in str(Units.Unit(self.material["YoungsModulus"])):
                    FreeCAD.Console.PrintMessage(
                        "YoungsModulus in material data seems to have no unit "
                        "or a wrong unit (reset the value): {}\n".format(self.material["Name"])
                    )
                    self.material["YoungsModulus"] = "0 MPa"
            else:
                if self.material["Name"] != "NoName":
                    FreeCAD.Console.PrintMessage(
                        "YoungsModulus not found in {}\n".format(self.material["Name"])
                    )
                self.material["YoungsModulus"] = "0 MPa"
            if "PoissonRatio" in self.material:
                # PoissonRatio does not have a unit, but it is checked if there is no value at all
                try:
                    # next line with float() is ok
                    # a quantity of a empty string returns very small value, would be wrong here
                    float(self.material["PoissonRatio"])
                except ValueError:
                    FreeCAD.Console.PrintMessage(
                        "PoissonRatio has wrong or no data (reset the value): {}\n".format(
                            self.material["PoissonRatio"]
                        )
                    )
                    self.material["PoissonRatio"] = "0"
            else:
                if self.material["Name"] != "NoName":
                    FreeCAD.Console.PrintMessage(
                        "PoissonRatio not found in {}\n".format(self.material["Name"])
                    )
                self.material["PoissonRatio"] = "0"
        if self.obj.Category == "Fluid":
            # Fluidic properties
            if "KinematicViscosity" in self.material:
                ki_vis = self.material["KinematicViscosity"]
                if "KinematicViscosity" not in str(Units.Unit(ki_vis)):
                    FreeCAD.Console.PrintMessage(
                        "KinematicViscosity in material data seems to have no unit "
                        "or a wrong unit (reset the value): {}\n".format(self.material["Name"])
                    )
                    self.material["KinematicViscosity"] = "0 m^2/s"
            else:
                if self.material["Name"] != "NoName":
                    FreeCAD.Console.PrintMessage(
                        "KinematicViscosity not found in {}\n".format(self.material["Name"])
                    )
                self.material["KinematicViscosity"] = "0 m^2/s"
            if "ThermalExpansionCoefficient" in self.material:
                vol_ther_ex_co = self.material["ThermalExpansionCoefficient"]
                if "ThermalExpansionCoefficient" not in str(Units.Unit(vol_ther_ex_co)):
                    FreeCAD.Console.PrintMessage(
                        "ThermalExpansionCoefficient in material data "
                        "seems to have no unit or a wrong unit (reset the value): {}\n".format(
                            self.material["Name"]
                        )
                    )
                    self.material["ThermalExpansionCoefficient"] = "0 1/K"
            else:
                if self.material["Name"] != "NoName":
                    FreeCAD.Console.PrintMessage(
                        "ThermalExpansionCoefficient not found in {}\n".format(
                            self.material["Name"]
                        )
                    )
                self.material["ThermalExpansionCoefficient"] = "0 1/K"
            if "VolumetricThermalExpansionCoefficient" in self.material:
                # unit type VolumetricThermalExpansionCoefficient is ThermalExpansionCoefficient
                vol_ther_ex_co = self.material["VolumetricThermalExpansionCoefficient"]
                if "ThermalExpansionCoefficient" not in str(Units.Unit(vol_ther_ex_co)):
                    FreeCAD.Console.PrintMessage(
                        "VolumetricThermalExpansionCoefficient in material data "
                        "seems to have no unit or a wrong unit (reset the value): {}\n".format(
                            self.material["Name"]
                        )
                    )
                    self.material["VolumetricThermalExpansionCoefficient"] = "0 1/K"
            else:
                # as fallback only add VolumetricThermalExpansionCoefficient if there is no
                # ThermalExpansionCoefficient
                if "ThermalExpansionCoefficient" not in self.material:
                    self.material["VolumetricThermalExpansionCoefficient"] = "0 1/K"
        # Thermal properties
        if "ThermalConductivity" in self.material:
            # TODO implement for all task panel values
            # https://forum.freecad.org/viewtopic.php?f=18&t=56912&p=516826#p516800
            try:
                Units.Quantity(self.material["ThermalConductivity"])
            except Exception:
                FreeCAD.Console.PrintError(
                    "Problem with the quantity for ThermalConductivity: '{}' Reset value.\n"
                    "May try the following in Python console:\n"
                    "from FreeCAD import Units\n"
                    "Units.Quantity('{}')\n".format(
                        self.material["ThermalConductivity"], self.material["ThermalConductivity"]
                    )
                )
                self.material["ThermalConductivity"] = "0 W/m/K"
            if "ThermalConductivity" not in str(Units.Unit(self.material["ThermalConductivity"])):
                FreeCAD.Console.PrintMessage(
                    "ThermalConductivity in material data seems to have no unit "
                    "or a wrong unit (reset the value): {}\n".format(self.material["Name"])
                )
                self.material["ThermalConductivity"] = "0 W/m/K"
        else:
            if self.material["Name"] != "NoName":
                FreeCAD.Console.PrintMessage(
                    "ThermalConductivity not found in {}\n".format(self.material["Name"])
                )
            self.material["ThermalConductivity"] = "0 W/m/K"
        if "ThermalExpansionCoefficient" in self.material:
            the_ex_co = self.material["ThermalExpansionCoefficient"]
            if "ThermalExpansionCoefficient" not in str(Units.Unit(the_ex_co)):
                FreeCAD.Console.PrintMessage(
                    "ThermalExpansionCoefficient in material data seems to have no unit "
                    "or a wrong unit (reset the value): {}\n".format(self.material["Name"])
                )
                self.material["ThermalExpansionCoefficient"] = "0 um/m/K"
        else:
            if self.material["Name"] != "NoName":
                FreeCAD.Console.PrintMessage(
                    "ThermalExpansionCoefficient not found in {}\n".format(self.material["Name"])
                )
            self.material["ThermalExpansionCoefficient"] = "0 um/m/K"
        if "SpecificHeat" in self.material:
            if "SpecificHeat" not in str(Units.Unit(self.material["SpecificHeat"])):
                FreeCAD.Console.PrintMessage(
                    "SpecificHeat in material data seems to have no unit "
                    "or a wrong unit (reset the value): {}\n".format(self.material["Name"])
                )
                self.material["SpecificHeat"] = "0 J/kg/K"
        else:
            if self.material["Name"] != "NoName":
                FreeCAD.Console.PrintMessage(
                    "SpecificHeat not found in {}\n".format(self.material["Name"])
                )
            self.material["SpecificHeat"] = "0 J/kg/K"
        FreeCAD.Console.PrintMessage("\n")

    def update_material_property(self, inputfield_text, matProperty, qUnit, variation=0.001):
        # print(inputfield_text)
        # this update property works for all Gui::InputField widgets
        if qUnit != "":
            value = Units.Quantity(inputfield_text).getValueAs(qUnit)
            old_value = Units.Quantity(self.material[matProperty]).getValueAs(qUnit)
        else:
            # for example PoissonRatio
            value = Units.Quantity(inputfield_text).Value
            old_value = Units.Quantity(self.material[matProperty]).Value
            # value = float(inputfield_text)  # this fails on locale with comma
            # https://forum.freecad.org/viewtopic.php?f=18&t=56912&p=523313#p523313

        if not (1 - variation < float(old_value) / value < 1 + variation):
            material = self.material
            if qUnit != "":
                material[matProperty] = str(value) + " " + qUnit
            else:
                material[matProperty] = str(value)
            self.material = material

    # mechanical input fields
    def ym_changed(self):
        # FreeCADs standard unit for stress is kPa for UnitsSchemeInternal, but MPa can be used
        self.update_material_property(
            self.parameterWidget.input_fd_young_modulus.text(),
            "YoungsModulus",
            "kPa",
        )

    def density_changed(self):
        print(
            "String read from density input field: {}".format(
                self.parameterWidget.input_fd_density.text()
            )
        )
        # FreeCADs standard unit for density is kg/mm^3 for UnitsSchemeInternal
        self.update_material_property(
            self.parameterWidget.input_fd_density.text(),
            "Density",
            "kg/m^3",
        )

    def pr_changed(self):
        value = self.parameterWidget.spinBox_poisson_ratio.value()
        if value:
            self.update_material_property(
                self.parameterWidget.spinBox_poisson_ratio.text(),
                "PoissonRatio",
                "",
            )
        elif value == 0:
            # PoissonRatio was set to 0.0 what is possible
            material = self.material
            material["PoissonRatio"] = str(value)
            self.material = material

    # thermal input fields
    def tc_changed(self):
        self.update_material_property(
            self.parameterWidget.input_fd_thermal_conductivity.text(),
            "ThermalConductivity",
            "W/m/K",
        )

    def tec_changed(self):
        self.update_material_property(
            self.parameterWidget.input_fd_expansion_coefficient.text(),
            "ThermalExpansionCoefficient",
            "um/m/K",
        )

    def sh_changed(self):
        self.update_material_property(
            self.parameterWidget.input_fd_specific_heat.text(),
            "SpecificHeat",
            "J/kg/K",
        )

    # fluidic input fields
    def vtec_changed(self):
        self.update_material_property(
            self.parameterWidget.input_fd_vol_expansion_coefficient.text(),
            "VolumetricThermalExpansionCoefficient",
            "m^3/m^3/K",
        )

    def kinematic_viscosity_changed(self):
        self.update_material_property(
            self.parameterWidget.input_fd_kinematic_viscosity.text(),
            "KinematicViscosity",
            "m^2/s",
        )

    def set_mat_params_in_input_fields(self, matmap):
        if "YoungsModulus" in matmap:
            ym_new_unit = "MPa"
            ym = Units.Quantity(matmap["YoungsModulus"])
            ym_with_new_unit = ym.getValueAs(ym_new_unit)
            q = Units.Quantity(f"{ym_with_new_unit} {ym_new_unit}")
            self.parameterWidget.input_fd_young_modulus.setText(q.UserString)
        if "PoissonRatio" in matmap:
            self.parameterWidget.spinBox_poisson_ratio.setValue(float(matmap["PoissonRatio"]))
        # Fluidic properties
        if "KinematicViscosity" in matmap:
            nu_new_unit = "m^2/s"
            nu = Units.Quantity(matmap["KinematicViscosity"])
            nu_with_new_unit = nu.getValueAs(nu_new_unit)
            q = Units.Quantity(f"{nu_with_new_unit} {nu_new_unit}")
            self.parameterWidget.input_fd_kinematic_viscosity.setText(q.UserString)
        # For isotropic materials and fluidic material
        # use the volumetric thermal expansion coefficient
        # is approximately three times the linear coefficient for solids
        if "VolumetricThermalExpansionCoefficient" in matmap:
            vtec_new_unit = "m^3/m^3/K"
            vtec = Units.Quantity(matmap["VolumetricThermalExpansionCoefficient"])
            vtec_with_new_unit = vtec.getValueAs(vtec_new_unit)
            q = Units.Quantity(f"{vtec_with_new_unit} {vtec_new_unit}")
            self.parameterWidget.input_fd_vol_expansion_coefficient.setText(q.UserString)
        if "Density" in matmap:
            density_new_unit = "kg/m^3"
            density = Units.Quantity(matmap["Density"])
            density_with_new_unit = density.getValueAs(density_new_unit)
            # self.parameterWidget.input_fd_density.setText(
            #     "{} {}".format(density_with_new_unit, density_new_unit)
            # )
            q = Units.Quantity(f"{density_with_new_unit} {density_new_unit}")
            self.parameterWidget.input_fd_density.setText(q.UserString)
        # thermal properties
        if "ThermalConductivity" in matmap:
            tc_new_unit = "W/m/K"
            tc = Units.Quantity(matmap["ThermalConductivity"])
            tc_with_new_unit = tc.getValueAs(tc_new_unit)
            q = Units.Quantity(f"{tc_with_new_unit} {tc_new_unit}")
            self.parameterWidget.input_fd_thermal_conductivity.setText(q.UserString)
        if "ThermalExpansionCoefficient" in matmap:  # linear, only for solid
            tec_new_unit = "um/m/K"
            tec = Units.Quantity(matmap["ThermalExpansionCoefficient"])
            tec_with_new_unit = tec.getValueAs(tec_new_unit)
            q = Units.Quantity(f"{tec_with_new_unit} {tec_new_unit}")
            self.parameterWidget.input_fd_expansion_coefficient.setText(q.UserString)
        if "SpecificHeat" in matmap:
            sh_new_unit = "J/kg/K"
            sh = Units.Quantity(matmap["SpecificHeat"])
            sh_with_new_unit = sh.getValueAs(sh_new_unit)
            q = Units.Quantity(f"{sh_with_new_unit} {sh_new_unit}")
            self.parameterWidget.input_fd_specific_heat.setText(q.UserString)

    def set_from_editor(self, value):
        mat = self.material_manager.getMaterial(value)
        self.material = mat.Properties
        self.uuid = mat.UUID
        self.check_material_keys()
        self.set_mat_params_in_input_fields(self.material)

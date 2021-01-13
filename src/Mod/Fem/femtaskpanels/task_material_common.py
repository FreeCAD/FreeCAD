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

__title__  = "FreeCAD FEM material task panel for the document object"
__author__ = "Juergen Riegel, Bernd Hahnebach, Qingfeng Xia"
__url__    = "https://www.freecadweb.org"

## @package task_material_common
#  \ingroup FEM
#  \brief FreeCAD FEM _ViewProviderFemMaterial
#  \brief task panel for common material object

import sys
from PySide import QtCore
from PySide import QtGui

import FreeCAD
import FreeCADGui
from FreeCAD import Units

from femguiutils import selection_widgets


if sys.version_info.major >= 3:
    unicode = str


class _TaskPanel:
    """
    The editmode TaskPanel for FemMaterial objects
    """

    def __init__(self, obj):

        FreeCAD.Console.PrintMessage("\n")  # empty line on start task panel
        self.obj = obj
        self.material = self.obj.Material  # FreeCAD material dictionary of current material
        self.card_path = ""
        self.materials = {}  # { card_path : FreeCAD material dict, ... }
        self.cards = {}  # { card_path : card_names, ... }
        self.icons = {}  # { card_path : icon_path, ... }
        # mat_card is the FCMat file
        # card_name is the file name of the mat_card
        # card_path is the whole file path of the mat_card
        # material_name is the value of the key name in FreeCAD material dictionary
        # they might not match because of special letters in the material_name
        # which are changed in the card_name to english standard characters
        self.has_transient_mat = False

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/Material.ui"
        )
        # globals
        QtCore.QObject.connect(
            self.parameterWidget.cb_materials,
            QtCore.SIGNAL("activated(int)"),
            self.choose_material
        )
        QtCore.QObject.connect(
            self.parameterWidget.chbu_allow_edit,
            QtCore.SIGNAL("clicked()"),
            self.toggleInputFieldsReadOnly
        )
        QtCore.QObject.connect(
            self.parameterWidget.pushButton_editMat,
            QtCore.SIGNAL("clicked()"),
            self.edit_material
        )
        # basic properties must be provided
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_density,
            QtCore.SIGNAL("editingFinished()"),
            self.density_changed
        )
        # mechanical properties
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_young_modulus,
            QtCore.SIGNAL("editingFinished()"),
            self.ym_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.spinBox_poisson_ratio,
            QtCore.SIGNAL("editingFinished()"),
            self.pr_changed
        )
        # thermal properties
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_thermal_conductivity,
            QtCore.SIGNAL("editingFinished()"),
            self.tc_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_expansion_coefficient,
            QtCore.SIGNAL("editingFinished()"),
            self.tec_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_specific_heat,
            QtCore.SIGNAL("editingFinished()"),
            self.sh_changed
        )
        # fluidic properties, only volumetric thermal expansion coeff makes sense
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_kinematic_viscosity,
            QtCore.SIGNAL("editingFinished()"),
            self.kinematic_viscosity_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.input_fd_vol_expansion_coefficient,
            QtCore.SIGNAL("editingFinished()"),
            self.vtec_changed
        )

        # init all parameter input files with read only
        self.parameterWidget.chbu_allow_edit.setCheckState(QtCore.Qt.CheckState.Unchecked)
        self.toggleInputFieldsReadOnly()

        # hide some groupBox according to material category
        self.parameterWidget.label_category.setText(self.obj.Category)
        if self.obj.Category == "Fluid":
            self.parameterWidget.groupBox_mechanical.setVisible(0)
            self.parameterWidget.label_expansion_coefficient.setVisible(0)
            self.parameterWidget.input_fd_expansion_coefficient.setVisible(0)
        else:
            self.parameterWidget.groupBox_fluidic.setVisible(0)
            self.parameterWidget.label_vol_expansion_coefficient.setVisible(0)
            self.parameterWidget.input_fd_vol_expansion_coefficient.setVisible(0)

        # get all available materials (fill self.materials, self.cards and self.icons)
        from materialtools.cardutils import import_materials as getmats
        # Note: import_materials(category="Solid", ...),
        #           category default to Solid, but must be given for FluidMaterial to be imported
        self.materials, self.cards, self.icons = getmats(self.obj.Category)
        # fill the material comboboxes with material cards
        self.add_cards_to_combo_box()

        # search for exact this mat_card in all known cards, choose the current material
        self.card_path = self.get_material_card(self.material)
        FreeCAD.Console.PrintLog("card_path: {}\n".format(self.card_path))
        if not self.card_path:
            # we have not found our material in self.materials dict :-(
            # we're going to add a user-defined temporary material: a document material
            FreeCAD.Console.PrintMessage(
                "Previously used material card can not be found in material directories. "
                "Add document material.\n"
            )
            self.card_path = "_document_material"
            self.materials[self.card_path] = self.material
            self.parameterWidget.cb_materials.addItem(
                QtGui.QIcon(":/icons/help-browser.svg"),
                self.card_path,
                self.card_path
            )
            index = self.parameterWidget.cb_materials.findData(self.card_path)
            # print(index)
            # fill input fields and set the current material in the cb widget
            self.choose_material(index)
        else:
            # we found our exact material in self.materials dict :-)
            FreeCAD.Console.PrintLog(
                "Previously used material card was found in material directories. "
                "We will use this material.\n"
            )
            index = self.parameterWidget.cb_materials.findData(self.card_path)
            # print(index)
            # fill input fields and set the current material in the cb widget
            self.choose_material(index)

        # geometry selection widget
        self.selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References,
            ["Solid", "Face", "Edge"],
            False
        )  # start with Solid in list!

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

        # check references, has to be after initialisation of selectionWidget
        self.selectionWidget.has_equal_references_shape_types()

    # leave task panel ***************************************************************************
    def accept(self):
        # print(self.material)
        if self.selectionWidget.has_equal_references_shape_types():
            self.do_not_set_thermal_zeros()
            from materialtools.cardutils import check_mat_units as checkunits
            if checkunits(self.material) is True:
                self.obj.Material = self.material
                self.obj.References = self.selectionWidget.references
            else:
                error_message = (
                    "Due to some wrong material quantity units in the changed "
                    "material data, the task panel changes where not accepted.\n"
                )
                FreeCAD.Console.PrintError(error_message)
                QtGui.QMessageBox.critical(None, "Material data not changed", error_message)
        self.recompute_and_set_back_all()
        return True

    def reject(self):
        self.recompute_and_set_back_all()
        return True

    def recompute_and_set_back_all(self):
        doc = FreeCADGui.getDocument(self.obj.Document)
        doc.Document.recompute()
        self.selectionWidget.setback_listobj_visibility()
        if self.selectionWidget.sel_server:
            FreeCADGui.Selection.removeObserver(self.selectionWidget.sel_server)
        doc.resetEdit()

    def do_not_set_thermal_zeros(self):
        """ thermal material parameter are set to 0.0 if not available
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
                "Zero SpecificHeat value. "
                "This parameter is not saved in the material data.\n"
            )

    # choose material ****************************************************************************
    def get_material_card(self, material):
        for a_mat in self.materials:
            unmatched_items = set(self.materials[a_mat].items()) ^ set(material.items())
            # print(a_mat + "  -->  unmatched_items = " + str(len(unmatched_items)))
            # if len(unmatched_items) < 4:
            #     print(unmatched_items)
            if len(unmatched_items) == 0:
                return a_mat
        return ""

    def choose_material(self, index):
        if index < 0:
            return
        self.card_path = self.parameterWidget.cb_materials.itemData(index)  # returns whole path
        FreeCAD.Console.PrintMessage(
            "choose_material in FEM material task panel:\n"
            "    {}\n".format(self.card_path)
        )
        self.material = self.materials[self.card_path]
        self.check_material_keys()
        self.set_mat_params_in_input_fields(self.material)
        self.parameterWidget.cb_materials.setCurrentIndex(index)  # set after input fields
        gen_mat_desc = ""
        gen_mat_name = ""
        if "Description" in self.material:
            gen_mat_desc = self.material["Description"]
        if "Name" in self.material:
            gen_mat_name = self.material["Name"]
        self.parameterWidget.l_mat_description.setText(gen_mat_desc)
        self.parameterWidget.l_mat_name.setText(gen_mat_name)
        # print("choose_material: done")

    def set_transient_material(self):
        self.card_path = "_transient_material"
        self.materials[self.card_path] = self.material  # = the current input fields data
        index = self.parameterWidget.cb_materials.findData(self.card_path)
        self.choose_material(index)

    def add_transient_material(self):
        self.has_transient_mat = True
        self.card_path = "_transient_material"
        self.parameterWidget.cb_materials.addItem(
            QtGui.QIcon(":/icons/help-browser.svg"),
            self.card_path,
            self.card_path
        )
        self.set_transient_material()

    # how to edit a material *********************************************************************
    def edit_material(self):
        # opens the material editor to choose a material or edit material params
        import MaterialEditor
        if self.card_path not in self.cards:
            FreeCAD.Console.PrintLog(
                "Card path not in cards, material dict will be used to open Material Editor.\n"
            )
            new_material_params = MaterialEditor.editMaterial(material=self.material)
        else:
            new_material_params = MaterialEditor.editMaterial(card_path=self.card_path)
        # material editor returns the mat_dict only, not a card_path
        # if the material editor was canceled a empty dict will be returned
        # do not change the self.material
        # check if dict is not empty (do not use "is True")
        if new_material_params:
            # check material quantity units
            from materialtools.cardutils import check_mat_units as checkunits
            if checkunits(new_material_params) is True:
                self.material = new_material_params
                self.card_path = self.get_material_card(self.material)
                # print("card_path: " + self.card_path)
                self.check_material_keys()
                self.set_mat_params_in_input_fields(self.material)
                if not self.card_path:
                    FreeCAD.Console.PrintMessage(
                        "Material card chosen by the material editor "
                        "was not found in material directories.\n"
                        "Either the card does not exist or some material "
                        "parameter where changed in material editor.\n"
                    )
                    if self.has_transient_mat is False:
                        self.add_transient_material()
                    else:
                        self.set_transient_material()
                else:
                    # we found our exact material in self.materials dict :-)
                    FreeCAD.Console.PrintLog(
                        "Material card chosen by the material editor "
                        "was found in material directories. "
                        "The found material card will be used.\n"
                    )
                    index = self.parameterWidget.cb_materials.findData(self.card_path)
                    # print(index)
                    # set the current material in the cb widget
                    self.choose_material(index)
            else:
                error_message = (
                    "Due to some wrong material quantity units in data passed "
                    "by the material editor, the material data was not changed.\n"
                )
                FreeCAD.Console.PrintError(error_message)
                QtGui.QMessageBox.critical(None, "Material data not changed", error_message)
        else:
            FreeCAD.Console.PrintLog(
                "No changes where made by the material editor.\n"
            )

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
            FreeCAD.Console.PrintMessage("For some reason all material data is empty!\n")
            self.material["Name"] = "Empty"
        if "Density" in self.material:
            if "Density" not in str(Units.Unit(self.material["Density"])):
                FreeCAD.Console.PrintMessage(
                    "Density in material data seems to have no unit "
                    "or a wrong unit (reset the value): {}\n"
                    .format(self.material["Name"])
                )
                self.material["Density"] = "0 kg/m^3"
        else:
            FreeCAD.Console.PrintMessage(
                "Density not found in material data of: {}\n"
                .format(self.material["Name"])
            )
            self.material["Density"] = "0 kg/m^3"
        if self.obj.Category == "Solid":
            # mechanical properties
            if "YoungsModulus" in self.material:
                # unit type of YoungsModulus is Pressure
                if "Pressure" not in str(Units.Unit(self.material["YoungsModulus"])):
                    FreeCAD.Console.PrintMessage(
                        "YoungsModulus in material data seems to have no unit "
                        "or a wrong unit (reset the value): {}\n"
                        .format(self.material["Name"])
                    )
                    self.material["YoungsModulus"] = "0 MPa"
            else:
                FreeCAD.Console.PrintMessage(
                    "YoungsModulus not found in material data of: {}\n"
                    .format(self.material["Name"])
                )
                self.material["YoungsModulus"] = "0 MPa"
            if "PoissonRatio" in self.material:
                # PoissonRatio does not have a unit, but it is checked it there is no value at all
                try:
                    float(self.material["PoissonRatio"])
                except ValueError:
                    FreeCAD.Console.PrintMessage(
                        "PoissonRatio has wrong or no data (reset the value): {}\n"
                        .format(self.material["PoissonRatio"])
                    )
                    self.material["PoissonRatio"] = "0"
            else:
                FreeCAD.Console.PrintMessage(
                    "PoissonRatio not found in material data of: {}\n"
                    .format(self.material["Name"])
                )
                self.material["PoissonRatio"] = "0"
        if self.obj.Category == "Fluid":
            # Fluidic properties
            if "KinematicViscosity" in self.material:
                ki_vis = self.material["KinematicViscosity"]
                if "KinematicViscosity" not in str(Units.Unit(ki_vis)):
                    FreeCAD.Console.PrintMessage(
                        "KinematicViscosity in material data seems to have no unit "
                        "or a wrong unit (reset the value): {}\n"
                        .format(self.material["Name"])
                    )
                    self.material["KinematicViscosity"] = "0 m^2/s"
            else:
                FreeCAD.Console.PrintMessage(
                    "KinematicViscosity not found in material data of: {}\n"
                    .format(self.material["Name"])
                )
                self.material["KinematicViscosity"] = "0 m^2/s"
            if "VolumetricThermalExpansionCoefficient" in self.material:
                # unit type VolumetricThermalExpansionCoefficient is ThermalExpansionCoefficient
                vol_ther_ex_co = self.material["VolumetricThermalExpansionCoefficient"]
                if "VolumetricThermalExpansionCoefficient" not in str(Units.Unit(vol_ther_ex_co)):
                    FreeCAD.Console.PrintMessage(
                        "VolumetricThermalExpansionCoefficient in material data "
                        "seems to have no unit or a wrong unit (reset the value): {}\n"
                        .format(self.material["Name"])
                    )
                    self.material["VolumetricThermalExpansionCoefficient"] = "0 m^3/m^3/K"
            else:
                FreeCAD.Console.PrintMessage(
                    "VolumetricThermalExpansionCoefficient not found in material data of: {}\n"
                    .format(self.material["Name"])
                )
                self.material["VolumetricThermalExpansionCoefficient"] = "0 m^3/m^3/K"
        # Thermal properties
        if "ThermalConductivity" in self.material:
            if "ThermalConductivity" not in str(Units.Unit(self.material["ThermalConductivity"])):
                FreeCAD.Console.PrintMessage(
                    "ThermalConductivity in material data seems to have no unit "
                    "or a wrong unit (reset the value): {}\n"
                    .format(self.material["Name"])
                )
                self.material["ThermalConductivity"] = "0 W/m/K"
        else:
            FreeCAD.Console.PrintMessage(
                "ThermalConductivity not found in material data of: {}\n"
                .format(self.material["Name"])
            )
            self.material["ThermalConductivity"] = "0 W/m/K"
        if "ThermalExpansionCoefficient" in self.material:
            the_ex_co = self.material["ThermalExpansionCoefficient"]
            if "ThermalExpansionCoefficient" not in str(Units.Unit(the_ex_co)):
                FreeCAD.Console.PrintMessage(
                    "ThermalExpansionCoefficient in material data seems to have no unit "
                    "or a wrong unit (reset the value): {}\n"
                    .format(self.material["Name"])
                )
                self.material["ThermalExpansionCoefficient"] = "0 um/m/K"
        else:
            FreeCAD.Console.PrintMessage(
                "ThermalExpansionCoefficient not found in material data of: {}\n"
                .format(self.material["Name"])
            )
            self.material["ThermalExpansionCoefficient"] = "0 um/m/K"
        if "SpecificHeat" in self.material:
            if "SpecificHeat" not in str(Units.Unit(self.material["SpecificHeat"])):
                FreeCAD.Console.PrintMessage(
                    "SpecificHeat in material data seems to have no unit "
                    "or a wrong unit (reset the value): {}\n"
                    .format(self.material["Name"])
                )
                self.material["SpecificHeat"] = "0 J/kg/K"
        else:
            FreeCAD.Console.PrintMessage(
                "SpecificHeat not found in material data of: {}\n"
                .format(self.material["Name"])
            )
            self.material["SpecificHeat"] = "0 J/kg/K"
        FreeCAD.Console.PrintMessage("\n")

    def update_material_property(self, input_field, matProperty, qUnit, variation=0.001):
        # this update property works for all Gui::InputField widgets
        value = Units.Quantity(input_field.text()).getValueAs(qUnit)
        old_value = Units.Quantity(self.material[matProperty]).getValueAs(qUnit)
        if value:
            if not (1 - variation < float(old_value) / value < 1 + variation):
                material = self.material
                # unicode() is an alias to str for py3
                material[matProperty] = unicode(value) + " " + qUnit
                self.material = material
                if self.has_transient_mat is False:
                    self.add_transient_material()
                else:
                    self.set_transient_material()
        else:
            pass  # some check or default value set can be done here

    # mechanical input fields
    def ym_changed(self):
        # FreeCADs standard unit for stress is kPa for UnitsSchemeInternal, but MPa can be used
        input_field = self.parameterWidget.input_fd_young_modulus
        variation = 0.001
        self.update_material_property(
            input_field,
            "YoungsModulus",
            "kPa",
            variation
        )

    def density_changed(self):
        # FreeCADs standard unit for density is kg/mm^3 for UnitsSchemeInternal
        input_field = self.parameterWidget.input_fd_density
        variation = 0.001
        self.update_material_property(
            input_field,
            "Density",
            "kg/m^3",
            variation
        )

    def pr_changed(self):
        value = self.parameterWidget.spinBox_poisson_ratio.value()
        input_field = self.parameterWidget.spinBox_poisson_ratio
        if value:
            variation = 0.001
            self.update_material_property(
                input_field,
                "PoissonRatio",
                "",
                variation
            )
        elif value == 0:
            # PoissonRatio was set to 0.0 what is possible
            material = self.material
            material["PoissonRatio"] = unicode(value)
            self.material = material
            if self.has_transient_mat is False:
                self.add_transient_material()
            else:
                self.set_transient_material()

    # thermal input fields
    def tc_changed(self):
        input_field = self.parameterWidget.input_fd_thermal_conductivity
        variation = 0.001
        self.update_material_property(
            input_field,
            "ThermalConductivity",
            "W/m/K",
            variation
        )

    def tec_changed(self):
        input_field = self.parameterWidget.input_fd_expansion_coefficient
        variation = 0.001
        self.update_material_property(
            input_field,
            "ThermalExpansionCoefficient",
            "um/m/K",
            variation
        )

    def sh_changed(self):
        input_field = self.parameterWidget.input_fd_specific_heat
        variation = 0.001
        self.update_material_property(
            input_field,
            "SpecificHeat",
            "J/kg/K",
            variation
        )

    # fluidic input fields
    def vtec_changed(self):
        input_field = self.parameterWidget.input_fd_vol_expansion_coefficient
        self.update_material_property(
            input_field,
            "VolumetricThermalExpansionCoefficient",
            "m^3/m^3/K"
        )

    def kinematic_viscosity_changed(self):
        input_field = self.parameterWidget.input_fd_kinematic_viscosity
        self.update_material_property(
            input_field,
            "KinematicViscosity",
            "m^2/s"
        )

    def set_mat_params_in_input_fields(self, matmap):
        if "YoungsModulus" in matmap:
            ym_new_unit = "MPa"
            ym = FreeCAD.Units.Quantity(matmap["YoungsModulus"])
            ym_with_new_unit = ym.getValueAs(ym_new_unit)
            q = FreeCAD.Units.Quantity("{} {}".format(ym_with_new_unit, ym_new_unit))
            self.parameterWidget.input_fd_young_modulus.setText(q.UserString)
        if "PoissonRatio" in matmap:
            self.parameterWidget.spinBox_poisson_ratio.setValue(float(matmap["PoissonRatio"]))
        # Fluidic properties
        if "KinematicViscosity" in matmap:
            nu_new_unit = "m^2/s"
            nu = FreeCAD.Units.Quantity(matmap["KinematicViscosity"])
            nu_with_new_unit = nu.getValueAs(nu_new_unit)
            q = FreeCAD.Units.Quantity("{} {}".format(nu_with_new_unit, nu_new_unit))
            self.parameterWidget.input_fd_kinematic_viscosity.setText(q.UserString)
        # For isotropic materials and fluidic material
        # use the volumetric thermal expansion coefficient
        # is approximately three times the linear coefficient for solids
        if "VolumetricThermalExpansionCoefficient" in matmap:
            vtec_new_unit = "m^3/m^3/K"
            vtec = FreeCAD.Units.Quantity(matmap["VolumetricThermalExpansionCoefficient"])
            vtec_with_new_unit = vtec.getValueAs(vtec_new_unit)
            q = FreeCAD.Units.Quantity("{} {}".format(vtec_with_new_unit, vtec_new_unit))
            self.parameterWidget.input_fd_vol_expansion_coefficient.setText(q.UserString)
        if "Density" in matmap:
            density_new_unit = "kg/m^3"
            density = FreeCAD.Units.Quantity(matmap["Density"])
            density_with_new_unit = density.getValueAs(density_new_unit)
            # self.parameterWidget.input_fd_density.setText(
            #     "{} {}".format(density_with_new_unit, density_new_unit)
            # )
            q = FreeCAD.Units.Quantity("{} {}".format(density_with_new_unit, density_new_unit))
            self.parameterWidget.input_fd_density.setText(q.UserString)
        # thermal properties
        if "ThermalConductivity" in matmap:
            tc_new_unit = "W/m/K"
            tc = FreeCAD.Units.Quantity(matmap["ThermalConductivity"])
            tc_with_new_unit = tc.getValueAs(tc_new_unit)
            q = FreeCAD.Units.Quantity("{} {}".format(tc_with_new_unit, tc_new_unit))
            self.parameterWidget.input_fd_thermal_conductivity.setText(q.UserString)
        if "ThermalExpansionCoefficient" in matmap:  # linear, only for solid
            tec_new_unit = "um/m/K"
            tec = FreeCAD.Units.Quantity(matmap["ThermalExpansionCoefficient"])
            tec_with_new_unit = tec.getValueAs(tec_new_unit)
            q = FreeCAD.Units.Quantity("{} {}".format(tec_with_new_unit, tec_new_unit))
            self.parameterWidget.input_fd_expansion_coefficient.setText(q.UserString)
        if "SpecificHeat" in matmap:
            sh_new_unit = "J/kg/K"
            sh = FreeCAD.Units.Quantity(matmap["SpecificHeat"])
            sh_with_new_unit = sh.getValueAs(sh_new_unit)
            q = FreeCAD.Units.Quantity("{} {}".format(sh_with_new_unit, sh_new_unit))
            self.parameterWidget.input_fd_specific_heat.setText(q.UserString)

    # fill the combo box with cards **************************************************************
    def add_cards_to_combo_box(self):
        # fill combobox, in combo box the card name is used not the material name
        self.parameterWidget.cb_materials.clear()

        mat_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Cards")
        sort_by_resources = mat_prefs.GetBool("SortByResources", False)

        card_name_list = []  # [ [card_name, card_path, icon_path], ... ]

        if sort_by_resources is True:
            for a_path in sorted(self.materials.keys()):
                card_name_list.append([self.cards[a_path], a_path, self.icons[a_path]])
        else:
            card_names_tmp = {}
            for path, name in self.cards.items():
                card_names_tmp[name] = path
            for a_name in sorted(card_names_tmp.keys()):
                a_path = card_names_tmp[a_name]
                card_name_list.append([a_name, a_path, self.icons[a_path]])

        for mat in card_name_list:
            self.parameterWidget.cb_materials.addItem(QtGui.QIcon(mat[2]), mat[0], mat[1])
            # the whole card path is added to the combo box to make it unique
            # see def choose_material:
            # for assignment of self.card_path the path form the parameterWidget ist used

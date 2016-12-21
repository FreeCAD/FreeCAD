# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *
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

__title__ = "_TaskPanelMechanicalMaterial"
__author__ = "Juergen Riegel, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package TaskPanelMechanicalMaterial
#  \ingroup FEM

import FreeCAD
import FreeCADGui
from PySide import QtGui
from PySide import QtCore
import Units


class _TaskPanelMechanicalMaterial:
    '''The editmode TaskPanel for MechanicalMaterial objects'''
    def __init__(self, obj):
        FreeCADGui.Selection.clearSelection()
        self.sel_server = None
        self.obj = obj
        self.selection_mode_solid = False
        self.selection_mode_std_print_message = "Select Faces and Edges by single click on them to add them to the list."
        self.selection_mode_solid_print_message = "Select Solids by single click on a Face or Edge which belongs to the Solid, to add the Solid to the list."
        self.material = self.obj.Material
        self.references = []
        if self.obj.References:
            self.tuplereferences = self.obj.References
            self.get_references()
        self.references_shape_type = None

        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/TaskPanelMechanicalMaterial.ui")
        QtCore.QObject.connect(self.form.pushButton_MatWeb, QtCore.SIGNAL("clicked()"), self.goMatWeb)
        QtCore.QObject.connect(self.form.cb_materials, QtCore.SIGNAL("activated(int)"), self.choose_material)
        QtCore.QObject.connect(self.form.input_fd_young_modulus, QtCore.SIGNAL("valueChanged(double)"), self.ym_changed)
        QtCore.QObject.connect(self.form.spinBox_poisson_ratio, QtCore.SIGNAL("valueChanged(double)"), self.pr_changed)
        QtCore.QObject.connect(self.form.input_fd_density, QtCore.SIGNAL("valueChanged(double)"), self.density_changed)
        QtCore.QObject.connect(self.form.pushButton_Reference, QtCore.SIGNAL("clicked()"), self.add_references)
        QtCore.QObject.connect(self.form.rb_standard, QtCore.SIGNAL("toggled(bool)"), self.choose_selection_mode_standard)
        QtCore.QObject.connect(self.form.rb_solid, QtCore.SIGNAL("toggled(bool)"), self.choose_selection_mode_solid)
        QtCore.QObject.connect(self.form.input_fd_thermal_conductivity, QtCore.SIGNAL("valueChanged(double)"), self.tc_changed)
        QtCore.QObject.connect(self.form.input_fd_expansion_coefficient, QtCore.SIGNAL("valueChanged(double)"), self.tec_changed)
        QtCore.QObject.connect(self.form.input_fd_specific_heat, QtCore.SIGNAL("valueChanged(double)"), self.sh_changed)

        self.form.list_References.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.form.list_References.connect(self.form.list_References, QtCore.SIGNAL("customContextMenuRequested(QPoint)"), self.references_list_right_clicked)

        self.import_materials()
        previous_mat_path = self.get_material_path(self.material)
        if not previous_mat_path:
            material_name = self.get_material_name(self.material)
            if material_name != 'None':
                FreeCAD.Console.PrintMessage("Previously used material cannot be found in material directories. Using transient material.\n")
                self.add_transient_material(self.material)
                index = self.form.cb_materials.findData(material_name)
            else:
                if not self.material:
                    index = self.form.cb_materials.findText(material_name)
                else:
                    FreeCAD.Console.PrintMessage("None material was previously used. Reload values.\n")
                    self.add_transient_material(self.material)
                    index = self.form.cb_materials.findData(material_name)
            self.choose_material(index)
        else:
            index = self.form.cb_materials.findData(previous_mat_path)
            self.choose_material(index)
        self.has_equal_references_shape_types()
        self.rebuild_list_References()

    def accept(self):
        # print(self.material)
        self.remove_active_sel_server()
        if self.has_equal_references_shape_types():
            self.obj.Material = self.material
            self.obj.References = self.references
            doc = FreeCADGui.getDocument(self.obj.Document)
            doc.resetEdit()
            doc.Document.recompute()

    def reject(self):
        self.remove_active_sel_server()
        doc = FreeCADGui.getDocument(self.obj.Document)
        doc.resetEdit()

    def remove_active_sel_server(self):
        if self.sel_server:
            FreeCADGui.Selection.removeObserver(self.sel_server)

    def choose_selection_mode_standard(self, state):
        self.selection_mode_solid = not state
        if self.sel_server and not self.selection_mode_solid:
            print(self.selection_mode_std_print_message)

    def choose_selection_mode_solid(self, state):
        self.selection_mode_solid = state
        if self.sel_server and self.selection_mode_solid:
            print(self.selection_mode_solid_print_message)

    def get_references(self):
        for ref in self.tuplereferences:
            for elem in ref[1]:
                self.references.append((ref[0], elem))

    def has_equal_references_shape_types(self):
        if not self.references:
            self.references_shape_type = None
        for ref in self.references:
            if ref[1].startswith('Solid'):
                r = ref[0].Shape.Solids[int(ref[1].lstrip('Solid')) - 1]  # Solid
            else:
                r = ref[0].Shape.getElement(ref[1])  # Face, Edge
            # print('  ReferenceShape : ', r.ShapeType, ', ', ref[0].Name, ', ', ref[0].Label, ' --> ', ref[1])
            if self.references_shape_type is None:
                self.references_shape_type = r.ShapeType
            if r.ShapeType != self.references_shape_type:
                FreeCAD.Console.PrintError('Different ShapeTypes in Reference List not allowed\n')
                return False
        return True

    def goMatWeb(self):
        import webbrowser
        webbrowser.open("http://matweb.com")

    def check_material_keys(self):
        if 'YoungsModulus' not in self.material:
            print('YoungsModulus not found in material data of: ' + self.material['Name'])
            self.material['YoungsModulus'] = '0 MPa'
        if 'Density' not in self.material:
            print('Density not found in material data of: ' + self.material['Name'])
            self.material['Density'] = '0 kg/m^3'
        if 'PoissonRatio' not in self.material:
            print('PoissonRatio not found in material data of: ' + self.material['Name'])
            self.material['PoissonRatio'] = '0'
        if 'ThermalConductivity' not in self.material:
            print('ThermalConductivity not found in material data of: ' + self.material['Name'])
            self.material['ThermalConductivity'] = '0 W/m/K'
        if 'ThermalExpansionCoefficient' not in self.material:
            print('ThermalExpansionCoefficient not found in material data of: ' + self.material['Name'])
            self.material['ThermalExpansionCoefficient'] = '0 um/m/K'
        if 'SpecificHeat' not in self.material:
            print('SpecificHeat not found in material data of: ' + self.material['Name'])
            self.material['SpecificHeat'] = '0 J/kg/K'

    def ym_changed(self, value):
        # FreeCADs standard unit for stress is kPa
        old_ym = Units.Quantity(self.material['YoungsModulus']).getValueAs("kPa")
        variation = 0.001
        if value:
            if not (1 - variation < float(old_ym) / value < 1 + variation):
                # YoungsModulus has changed
                material = self.material
                material['YoungsModulus'] = unicode(value) + " kPa"
                self.material = material

    def density_changed(self, value):
        # FreeCADs standard unit for density is kg/mm^3
        old_density = Units.Quantity(self.material['Density']).getValueAs("kg/m^3")
        variation = 0.001
        if value:
            if not (1 - variation < float(old_density) / value < 1 + variation):
                # density has changed
                material = self.material
                value_in_kg_per_m3 = value * 1e9
                material['Density'] = unicode(value_in_kg_per_m3) + " kg/m^3"  # SvdW:Keep density in SI units for easier readability
                self.material = material

    def pr_changed(self, value):
        old_pr = Units.Quantity(self.material['PoissonRatio'])
        variation = 0.001
        if value:
            if not (1 - variation < float(old_pr) / value < 1 + variation):
                # PoissonRatio has changed
                material = self.material
                material['PoissonRatio'] = unicode(value)
                self.material = material

    def tc_changed(self, value):
        old_tc = Units.Quantity(self.material['ThermalConductivity']).getValueAs("W/m/K")
        variation = 0.001
        if value:
            if not (1 - variation < float(old_tc) / value < 1 + variation):
                # ThermalConductivity has changed
                material = self.material
                value_in_W_per_mK = value * 1e-3  # To compensate for use of SI units
                material['ThermalConductivity'] = unicode(value_in_W_per_mK) + " W/m/K"
                self.material = material

    def tec_changed(self, value):
        old_tec = Units.Quantity(self.material['ThermalExpansionCoefficient']).getValueAs("um/m/K")
        variation = 0.001
        if value:
            if not (1 - variation < float(old_tec) / value < 1 + variation):
                # ThermalExpansionCoefficient has changed
                material = self.material
                value_in_um_per_mK = value * 1e6  # To compensate for use of SI units
                material['ThermalExpansionCoefficient'] = unicode(value_in_um_per_mK) + " um/m/K"
                self.material = material

    def sh_changed(self, value):
        old_sh = Units.Quantity(self.material['SpecificHeat']).getValueAs("J/kg/K")
        variation = 0.001
        if value:
            if not (1 - variation < float(old_sh) / value < 1 + variation):
                # SpecificHeat has changed
                material = self.material
                value_in_J_per_kgK = value * 1e-6  # To compensate for use of SI units
                material['SpecificHeat'] = unicode(value_in_J_per_kgK) + " J/kg/K"
                self.material = material

    def choose_material(self, index):
        if index < 0:
            return
        mat_file_path = self.form.cb_materials.itemData(index)
        self.material = self.materials[mat_file_path]
        self.form.cb_materials.setCurrentIndex(index)
        self.check_material_keys()
        self.set_mat_params_in_combo_box(self.material)
        gen_mat_desc = ""
        if 'Description' in self.material:
            gen_mat_desc = self.material['Description']
        self.form.l_mat_description.setText(gen_mat_desc)

    def get_material_name(self, material):
        if 'Name' in self.material:
            return self.material['Name']
        else:
            return 'None'

    def get_material_path(self, material):
        for a_mat in self.materials:
            unmatched_items = set(self.materials[a_mat].items()) ^ set(material.items())
            if len(unmatched_items) == 0:
                return a_mat
        return ""

    def set_mat_params_in_combo_box(self, matmap):
        if 'YoungsModulus' in matmap:
            ym_new_unit = "MPa"
            ym = FreeCAD.Units.Quantity(matmap['YoungsModulus'])
            ym_with_new_unit = ym.getValueAs(ym_new_unit)
            self.form.input_fd_young_modulus.setText("{} {}".format(ym_with_new_unit, ym_new_unit))
        if 'PoissonRatio' in matmap:
            self.form.spinBox_poisson_ratio.setValue(float(matmap['PoissonRatio']))
        if 'Density' in matmap:
            density_new_unit = "kg/m^3"
            density = FreeCAD.Units.Quantity(matmap['Density'])
            density_with_new_unit = density.getValueAs(density_new_unit)
            self.form.input_fd_density.setText("{} {}".format(density_with_new_unit, density_new_unit))
        if 'ThermalConductivity' in matmap:
            tc_new_unit = "W/m/K"
            tc = FreeCAD.Units.Quantity(matmap['ThermalConductivity'])
            tc_with_new_unit = tc.getValueAs(tc_new_unit)
            self.form.input_fd_thermal_conductivity.setText("{} {}".format(tc_with_new_unit, tc_new_unit))
        if 'ThermalExpansionCoefficient' in matmap:
            tec_new_unit = "um/m/K"
            tec = FreeCAD.Units.Quantity(matmap['ThermalExpansionCoefficient'])
            tec_with_new_unit = tec.getValueAs(tec_new_unit)
            self.form.input_fd_expansion_coefficient.setText("{} {}".format(tec_with_new_unit, tec_new_unit))
        if 'SpecificHeat' in matmap:
            sh_new_unit = "J/kg/K"
            sh = FreeCAD.Units.Quantity(matmap['SpecificHeat'])
            sh_with_new_unit = sh.getValueAs(sh_new_unit)
            self.form.input_fd_specific_heat.setText("{} {}".format(sh_with_new_unit, sh_new_unit))

    def add_transient_material(self, material):
        material_name = self.get_material_name(material)
        self.form.cb_materials.addItem(QtGui.QIcon(":/icons/help-browser.svg"), material_name, material_name)
        self.materials[material_name] = material

    def add_mat_dir(self, mat_dir, icon):
        import glob
        import os
        import Material
        mat_file_extension = ".FCMat"
        ext_len = len(mat_file_extension)
        dir_path_list = glob.glob(mat_dir + '/*' + mat_file_extension)
        self.pathList = self.pathList + dir_path_list
        material_name_list = []
        for a_path in dir_path_list:
            material_name = os.path.basename(a_path[:-ext_len])
            self.materials[a_path] = Material.importFCMat(a_path)
            material_name_list.append([material_name, a_path])
        material_name_list.sort()
        for mat in material_name_list:
            self.form.cb_materials.addItem(QtGui.QIcon(icon), mat[0], mat[1])

    def import_materials(self):
        self.materials = {}
        self.pathList = []
        self.form.cb_materials.clear()
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
        use_built_in_materials = self.fem_prefs.GetBool("UseBuiltInMaterials", True)
        if use_built_in_materials:
            system_mat_dir = FreeCAD.getResourceDir() + "/Mod/Material/StandardMaterial"
            self.add_mat_dir(system_mat_dir, ":/icons/freecad.svg")

        use_mat_from_config_dir = self.fem_prefs.GetBool("UseMaterialsFromConfigDir", True)
        if use_mat_from_config_dir:
            user_mat_dirname = FreeCAD.getUserAppDataDir() + "Materials"
            self.add_mat_dir(user_mat_dirname, ":/icons/preferences-general.svg")

        use_mat_from_custom_dir = self.fem_prefs.GetBool("UseMaterialsFromCustomDir", True)
        if use_mat_from_custom_dir:
            custom_mat_dir = self.fem_prefs.GetString("CustomMaterialsDir", "")
            self.add_mat_dir(custom_mat_dir, ":/icons/user.svg")

    def references_list_right_clicked(self, QPos):
        self.form.contextMenu = QtGui.QMenu()
        menu_item = self.form.contextMenu.addAction("Remove Reference")
        if not self.references:
            menu_item.setDisabled(True)
        self.form.connect(menu_item, QtCore.SIGNAL("triggered()"), self.remove_reference)
        parentPosition = self.form.list_References.mapToGlobal(QtCore.QPoint(0, 0))
        self.form.contextMenu.move(parentPosition + QPos)
        self.form.contextMenu.show()

    def remove_reference(self):
        if not self.references:
            return
        currentItemName = str(self.form.list_References.currentItem().text())
        for ref in self.references:
            if ref[1]:
                refname_to_compare_listentry = ref[0].Name + ':' + ref[1]
            else:
                refname_to_compare_listentry = ref[0].Name
            if refname_to_compare_listentry == currentItemName:
                self.references.remove(ref)
        self.rebuild_list_References()

    def add_references(self):
        '''Called if Button add_reference is triggered'''
        # in constraints EditTaskPanel the selection is active as soon as the taskpanel is open
        # here the addReference button EditTaskPanel has to be triggered to start selection mode
        FreeCADGui.Selection.clearSelection()
        # start SelectionObserver and parse the function to add the References to the widget
        if self.selection_mode_solid:  # print message on button click
            print_message = self.selection_mode_solid_print_message
        else:
            print_message = self.selection_mode_std_print_message
        import FemSelectionObserver
        self.sel_server = FemSelectionObserver.FemSelectionObserver(self.selectionParser, print_message)

    def selectionParser(self, selection):
        print('selection: ', selection[0].Shape.ShapeType, ' --> ', selection[0].Name, ' --> ', selection[1])
        if hasattr(selection[0], "Shape") and selection[1]:
            elt = selection[0].Shape.getElement(selection[1])
            if self.selection_mode_solid:
                # in solid selection mode use edges and faces for selection of a solid
                solid_to_add = None
                if elt.ShapeType == 'Edge':
                    found_edge = False
                    for i, s in enumerate(selection[0].Shape.Solids):
                        for e in s.Edges:
                            if elt.isSame(e):
                                if not found_edge:
                                    solid_to_add = str(i + 1)
                                else:
                                    FreeCAD.Console.PrintMessage('Edge belongs to more than one solid\n')
                                    solid_to_add = None
                                found_edge = True
                elif elt.ShapeType == 'Face':
                    found_face = False
                    for i, s in enumerate(selection[0].Shape.Solids):
                        for e in s.Faces:
                            if elt.isSame(e):
                                if not found_face:
                                    solid_to_add = str(i + 1)
                                else:
                                    FreeCAD.Console.PrintMessage('Face belongs to more than one solid\n')
                                    solid_to_add = None
                                found_edge = True
                if solid_to_add:
                    selection = (selection[0], 'Solid' + solid_to_add)
                    print('selection element changed to Solid: ', selection[0].Shape.ShapeType, '  ', selection[0].Name, '  ', selection[1])
                else:
                    return
            if selection not in self.references:
                self.references.append(selection)
                self.rebuild_list_References()
            else:
                FreeCAD.Console.PrintMessage(selection[0].Name + ' --> ' + selection[1] + ' is in reference list already!\n')

    def rebuild_list_References(self):
        self.form.list_References.clear()
        items = []
        for ref in self.references:
            if ref[1]:
                item_name = ref[0].Name + ':' + ref[1]
            else:
                item_name = ref[0].Name
            items.append(item_name)
        for listItemName in sorted(items):
            self.form.list_References.addItem(listItemName)

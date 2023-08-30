# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM material reinforced task panel for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package task_material_reinforced
#  \ingroup FEM
#  \brief task panel for reinforced material object

from PySide import QtCore
from PySide import QtGui

import FreeCAD
import FreeCADGui


unicode = str


class _TaskPanel:
    """
    The editmode TaskPanel for MaterialReinforced objects
    """

    unicode = str

    def __init__(self, obj):

        FreeCAD.Console.PrintMessage("\n")  # empty line on start task panel
        self.obj = obj

        # init matrix and reinforcement material
        self.material_m = self.obj.Material
        self.card_path_m = ""
        self.has_transient_mat_m = False
        self.material_r = self.obj.Reinforcement
        self.card_path_r = ""
        self.has_transient_mat_r = False
        # mat_card is the FCMat file
        # card_name is the file name of the mat_card
        # card_path is the whole file path of the mat_card
        # material_name is the value of the key name in FreeCAD material dictionary
        # they might not match because of special letters in the material_name which are
        # changed in the card_name to english standard characters

        # init for collecting all mat data and icons
        self.materials = {}  # { card_path : FreeCAD material dict }
        self.cards = {}  # { card_path : card_names, ... }
        self.icons = {}  # { card_path : icon_path }

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MaterialReinforcement.ui"
        )

        # globals
        QtCore.QObject.connect(
            self.parameterWidget.cb_materials_m,
            QtCore.SIGNAL("activated(int)"),
            self.choose_material_m
        )
        QtCore.QObject.connect(
            self.parameterWidget.pb_edit_m,
            QtCore.SIGNAL("clicked()"),
            self.edit_material_m
        )
        QtCore.QObject.connect(
            self.parameterWidget.cb_materials_r,
            QtCore.SIGNAL("activated(int)"),
            self.choose_material_r
        )
        QtCore.QObject.connect(
            self.parameterWidget.pb_edit_r,
            QtCore.SIGNAL("clicked()"),
            self.edit_material_r
        )

        # get all available materials (fill self.materials, self.cards and self.icons)
        from materialtools.cardutils import import_materials as getmats
        self.materials, self.cards, self.icons = getmats()
        # fill the material comboboxes with material cards
        self.add_cards_to_combo_boxes()

        # search for exact the mat_card_m and mat_card_r in all known cards
        # choose the current matrix material
        self.card_path_m = self.get_material_card(self.material_m)
        FreeCAD.Console.PrintLog("card_path: {}\n".format(self.card_path_m))
        if not self.card_path_m:
            # we have not found our material in self.materials dict :-(
            # we're going to add a user-defined temporary material: a document material
            FreeCAD.Console.PrintMessage(
                "Previously used material card cannot be found in material directories. "
                "Add document material.\n"
            )
            self.card_path_m = "_Document_Matrix_Material"
            self.materials[self.card_path_m] = self.material_m
            self.parameterWidget.cb_materials_m.addItem(
                QtGui.QIcon(":/icons/help-browser.svg"),
                self.card_path_m,
                self.card_path_m
            )
            index = self.parameterWidget.cb_materials_m.findData(self.card_path_m)
            # fill input fields and set the current material in the cb widget
            self.choose_material_m(index)
        else:
            # we found our exact material in self.materials dict :-)
            FreeCAD.Console.PrintLog(
                "Previously used material card was found in material directories. "
                "We will use this material.\n"
            )
            index = self.parameterWidget.cb_materials_m.findData(self.card_path_m)
            # set the current material in the cb widget
            self.choose_material_m(index)

        # choose the current reinforcement material
        self.card_path_r = self.get_material_card(self.material_r)
        FreeCAD.Console.PrintLog("card_path: {}\n".format(self.card_path_r))
        if not self.card_path_r:
            # we have not found our material in self.materials dict :-(
            # we're going to add a user-defined temporary material: a document material
            FreeCAD.Console.PrintMessage(
                "Previously used material card cannot be found in material directories. "
                "Add document material.\n"
            )
            self.card_path_r = "_Document_Reinforcement_Material"
            self.materials[self.card_path_r] = self.material_r
            self.parameterWidget.cb_materials_r.addItem(
                QtGui.QIcon(":/icons/help-browser.svg"),
                self.card_path_r,
                self.card_path_r
            )
            index = self.parameterWidget.cb_materials_r.findData(self.card_path_r)
            # set the current material in the cb widget
            self.choose_material_r(index)
        else:
            # we found our exact material in self.materials dict :-)
            FreeCAD.Console.PrintLog(
                "Previously used material card was found in material directories. "
                "We will use this material.\n"
            )
            index = self.parameterWidget.cb_materials_r.findData(self.card_path_r)
            # fill input fields and set the current material in the cb widget
            self.choose_material_r(index)

        # set up the form
        self.form = self.parameterWidget

    # leave task panel ***************************************************************************
    def accept(self):
        from materialtools.cardutils import check_mat_units as checkunits
        if checkunits(self.material_m) is True and checkunits(self.material_r) is True:
            self.obj.Material = self.material_m
            self.obj.Reinforcement = self.material_r
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
        guidoc = FreeCADGui.getDocument(self.obj.Document)
        guidoc.Document.recompute()
        guidoc.resetEdit()
        self.output_obj_mat_param()

    def output_obj_mat_param(self):
        self.print_mat_dict(self.obj.Material)
        self.print_mat_dict(self.obj.Reinforcement)
        print("\n")

    def print_mat_dict(self, mat_dict):
        if "Name" in mat_dict:
            print("Material: {}".format(mat_dict["Name"]))
        else:
            print("Matrix material: no Name")
        for key in mat_dict:
            print("    {}: {}".format(key, mat_dict[key]))

    # choose material card ***********************************************************************
    def get_material_card(self, material):
        for a_mat in self.materials:
            unmatched_items = set(self.materials[a_mat].items()) ^ set(material.items())
            # print(a_mat + "  -->  unmatched_items = " + str(len(unmatched_items)))
            if len(unmatched_items) < 4:
                FreeCAD.Console.PrintLog("{}\n".format(unmatched_items))
            if len(unmatched_items) == 0:
                return a_mat
        return ""

    def choose_material_m(self, index):
        if index < 0:
            return
        # get the whole card path
        self.card_path_m = self.parameterWidget.cb_materials_m.itemData(index)
        FreeCAD.Console.PrintMessage(
            "choose_material in FEM material task panel:\n"
            "    {}\n".format(self.card_path_m)
        )
        self.material_m = self.materials[self.card_path_m]
        self.parameterWidget.cb_materials_m.setCurrentIndex(index)
        gen_mat_desc = ""
        gen_mat_name = ""
        if "Description" in self.material_m:
            gen_mat_desc = self.material_m["Description"]
        if "Name" in self.material_m:
            gen_mat_name = self.material_m["Name"]
        self.parameterWidget.l_description_m.setText(gen_mat_desc)
        self.parameterWidget.l_name_m.setText(gen_mat_name)

    def choose_material_r(self, index):
        if index < 0:
            return
        # get the whole card path
        self.card_path_r = self.parameterWidget.cb_materials_r.itemData(index)
        FreeCAD.Console.PrintMessage(
            "choose_material in FEM material task panel:\n"
            "    {}\n".format(self.card_path_r)
        )
        self.material_r = self.materials[self.card_path_r]
        self.parameterWidget.cb_materials_r.setCurrentIndex(index)
        gen_mat_desc = ""
        gen_mat_name = ""
        if "Description" in self.material_r:
            gen_mat_desc = self.material_r["Description"]
        if "Name" in self.material_r:
            gen_mat_name = self.material_r["Name"]
        self.parameterWidget.l_description_r.setText(gen_mat_desc)
        self.parameterWidget.l_name_r.setText(gen_mat_name)

    # transient material is needed if the user changed mat parameter by the mat editor
    def set_transient_material_m(self):
        self.card_path_m = "_Transient_Matrix_Material"
        self.materials[self.card_path_m] = self.material_m  # = the current matrix mat dict
        index = self.parameterWidget.cb_materials_m.findData(self.card_path_m)
        self.choose_material_m(index)

    def add_transient_material_m(self):
        self.has_transient_mat_m = True
        self.card_path_m = "_Transient_Matrix_Material"
        self.parameterWidget.cb_materials_m.addItem(
            QtGui.QIcon(":/icons/help-browser.svg"),
            self.card_path_m,
            self.card_path_m
        )
        self.set_transient_material_m()

    def set_transient_material_r(self):
        self.card_path_r = "_Transient_Reinforcement_Material"
        self.materials[self.card_path_r] = self.material_r  # = the current reinforced mat dict
        index = self.parameterWidget.cb_materials_r.findData(self.card_path_r)
        self.choose_material_r(index)

    def add_transient_material_r(self):
        self.has_transient_mat_r = True
        self.card_path_r = "_Transient_Reinforcement_Material"
        self.parameterWidget.cb_materials_r.addItem(
            QtGui.QIcon(":/icons/help-browser.svg"),
            self.card_path_r,
            self.card_path_r
        )
        self.set_transient_material_r()

    # edit material parameter ********************************************************************
    # TODO, also all mat parameter checks should be moved to material editor
    # and mat parameter checks should be done on analysis precheck in according to the analysis
    # should be checked if all needed parameter are defined and have all right values and units
    def edit_material_m(self):
        # opens the material editor to choose a material or edit material params
        import MaterialEditor
        if self.card_path_m not in self.cards:
            FreeCAD.Console.PrintLog(
                "Card path not in cards, material dict will be used to open Material Editor.\n"
            )
            new_material_params = MaterialEditor.editMaterial(material=self.material_m)
        else:
            new_material_params = MaterialEditor.editMaterial(card_path=self.card_path_m)
        # material editor returns the mat_dict only, not a card_path
        # if the material editor was canceled a empty dict will be returned
        # do not change the self.material
        # check if dict is not empty (do not use "is True")
        if new_material_params:
            # check material quantity units
            from materialtools.cardutils import check_mat_units as checkunits
            if checkunits(new_material_params) is True:
                self.material_m = new_material_params
                self.card_path_m = self.get_material_card(self.material_m)
                FreeCAD.Console.PrintMessage("card_path: {}\n".format(self.card_path_m))
                if not self.card_path_m:
                    FreeCAD.Console.PrintMessage(
                        "Material card chosen by the material editor "
                        "was not found in material directories.\n"
                        "Either the card does not exist or some material "
                        "parameter where changed in material editor.\n"
                    )
                    if self.has_transient_mat_m is False:
                        self.add_transient_material_m()
                    else:
                        self.set_transient_material_m()
                else:
                    # we found our exact material in self.materials dict :-)
                    FreeCAD.Console.PrintLog(
                        "Material card chosen by the material editor "
                        "was found in material directories. "
                        "The found material card will be used.\n"
                    )
                    index = self.parameterWidget.cb_materials_m.findData(self.card_path_m)
                    # set the current material in the cb widget
                self.choose_material_m(index)
            else:
                error_message = (
                    "Due to some wrong material quantity units in data passed "
                    "by the material editor, the material data was not changed.\n"
                )
                FreeCAD.Console.PrintError(error_message)
                QtGui.QMessageBox.critical(None, "Material data not changed", error_message)
        else:
            FreeCAD.Console.PrintMessage("No changes where made by the material editor.\n")

    def edit_material_r(self):
        # opens the material editor to choose a material or edit material params
        import MaterialEditor
        if self.card_path_r not in self.cards:
            FreeCAD.Console.PrintLog(
                "Card path not in cards, material dict will be used to open Material Editor.\n"
            )
            new_material_params = MaterialEditor.editMaterial(material=self.material_r)
        else:
            new_material_params = MaterialEditor.editMaterial(card_path=self.card_path_r)
        # material editor returns the mat_dict only, not a card_path
        # if the material editor was canceled a empty dict will be returned
        # do not change the self.material
        # check if dict is not empty (do not use "is True")
        if new_material_params:
            # check material quantity units
            from materialtools.cardutils import check_mat_units as checkunits
            if checkunits(new_material_params) is True:
                self.material_r = new_material_params
                self.card_path_r = self.get_material_card(self.material_r)
                FreeCAD.Console.PrintMessage("card_path: {}\n".format(self.card_path_r))
                if not self.card_path_r:
                    FreeCAD.Console.PrintMessage(
                        "Material card chosen by the material editor "
                        "was not found in material directories.\n"
                        "Either the card does not exist or some material "
                        "parameter where changed in material editor.\n"
                    )
                    if self.has_transient_mat_r is False:
                        self.add_transient_material_r()
                    else:
                        self.set_transient_material_r()
                else:
                    # we found our exact material in self.materials dict :-)
                    FreeCAD.Console.PrintLog(
                        "Material card chosen by the material editor "
                        "was found in material directories. "
                        "The found material card will be used.\n"
                    )
                    index = self.parameterWidget.cb_materials_r.findData(self.card_path_r)
                    # set the current material in the cb widget
                self.choose_material_r(index)
            else:
                error_message = (
                    "Due to some wrong material quantity units in data passed "
                    "by the material editor, the material data was not changed.\n"
                )
                FreeCAD.Console.PrintError(error_message)
                QtGui.QMessageBox.critical(None, "Material data not changed", error_message)
        else:
            FreeCAD.Console.PrintMessage("No changes where made by the material editor.\n")

    # fill the combo box with cards **************************************************************
    def add_cards_to_combo_boxes(self):
        # fill comboboxes, in combo box the card name is used not the material name
        self.parameterWidget.cb_materials_m.clear()
        self.parameterWidget.cb_materials_r.clear()

        mat_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Cards")
        sort_by_resources = mat_prefs.GetBool("SortByResources", False)

        card_name_list = []  # [ [card_name, card_path, icon_path], ... ]

        if sort_by_resources is True:
            for a_path in sorted(self.materials):
                card_name_list.append([self.cards[a_path], a_path, self.icons[a_path]])
        else:
            card_names_tmp = {}
            for path, name in self.cards.items():
                card_names_tmp[name] = path
            for a_name in sorted(card_names_tmp):
                a_path = card_names_tmp[a_name]
                card_name_list.append([a_name, a_path, self.icons[a_path]])

        for mat in card_name_list:
            self.parameterWidget.cb_materials_m.addItem(QtGui.QIcon(mat[2]), mat[0], mat[1])
            self.parameterWidget.cb_materials_r.addItem(QtGui.QIcon(mat[2]), mat[0], mat[1])
            # the whole card path is added to the combo box to make it unique
            # see def choose_material:
            # for assignment of self.card_path the path form the parameterWidget is used

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

import FreeCAD
from FemCommands import FemCommands

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui
    from PySide import QtGui
    from PySide import QtCore


__title__ = "Machine-Distortion FemSetGeometryObject managment"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"


def makeMechanicalMaterial(name):
    '''makeMaterial(name): makes an Material
    name there fore is a material name or an file name for a FCMat file'''
    obj = FreeCAD.ActiveDocument.addObject("App::MaterialObjectPython", name)
    _MechanicalMaterial(obj)
    if FreeCAD.GuiUp:
        _ViewProviderMechanicalMaterial(obj.ViewObject)
    # FreeCAD.ActiveDocument.recompute()
    return obj


class _CommandMechanicalMaterial(FemCommands):
    "the Fem Material command definition"
    def __init__(self):
        super(_CommandMechanicalMaterial, self).__init__()
        self.resources = {'Pixmap': 'fem-material',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Material", "Mechanical material..."),
                          'Accel': "M, M",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Material", "Creates or edit the mechanical material definition.")}
        self.is_active = 'with_analysis'

    def Activated(self):
        femDoc = FemGui.getActiveAnalysis().Document
        if FreeCAD.ActiveDocument is not femDoc:
            FreeCADGui.setActiveDocument(femDoc)
        FreeCAD.ActiveDocument.openTransaction("Create MechanicalMaterial")
        FreeCADGui.addModule("MechanicalMaterial")
        FreeCADGui.doCommand("MechanicalMaterial.makeMechanicalMaterial('MechanicalMaterial')")
        FreeCADGui.doCommand("App.activeDocument()." + FemGui.getActiveAnalysis().Name + ".Member = App.activeDocument()." + FemGui.getActiveAnalysis().Name + ".Member + [App.ActiveDocument.ActiveObject]")
        FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")


class _MechanicalMaterial:
    "The Material object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyLinkSubList", "References", "Material", "List of material shapes")
        obj.Proxy = self
        self.Type = "MechanicalMaterial"

    def execute(self, obj):
        return


class _ViewProviderMechanicalMaterial:
    "A View Provider for the MechanicalMaterial object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-material.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode):
        taskd = _MechanicalMaterialTaskPanel(self.Object)
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
        return

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _MechanicalMaterialTaskPanel:
    '''The editmode TaskPanel for MechanicalMaterial objects'''
    def __init__(self, obj):
        FreeCADGui.Selection.clearSelection()
        self.sel_server = None
        self.obj = obj
        self.material = self.obj.Material
        self.references = self.obj.References

        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/MechanicalMaterial.ui")
        QtCore.QObject.connect(self.form.pushButton_MatWeb, QtCore.SIGNAL("clicked()"), self.goMatWeb)
        QtCore.QObject.connect(self.form.cb_materials, QtCore.SIGNAL("activated(int)"), self.choose_material)
        QtCore.QObject.connect(self.form.input_fd_young_modulus, QtCore.SIGNAL("valueChanged(double)"), self.ym_changed)
        QtCore.QObject.connect(self.form.spinBox_poisson_ratio, QtCore.SIGNAL("valueChanged(double)"), self.pr_changed)
        QtCore.QObject.connect(self.form.input_fd_density, QtCore.SIGNAL("valueChanged(double)"), self.density_changed)
        QtCore.QObject.connect(self.form.pushButton_Reference, QtCore.SIGNAL("clicked()"), self.add_references)
        self.form.list_References.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.form.list_References.connect(self.form.list_References, QtCore.SIGNAL("customContextMenuRequested(QPoint)"), self.references_list_right_clicked)

        self.import_materials()
        previous_mat_path = self.get_material_path(self.material)
        if not previous_mat_path:
            print("Previously used material cannot be found in material directories. Using transient material.")
            material_name = self.get_material_name(self.material)
            if material_name != 'None':
                self.add_transient_material(self.material)
                index = self.form.cb_materials.findData(material_name)
            else:
                index = self.form.cb_materials.findText(material_name)
            self.choose_material(index)
        else:
            index = self.form.cb_materials.findData(previous_mat_path)
            self.choose_material(index)
        self.rebuild_list_References()

    def accept(self):
        if self.sel_server:
            FreeCADGui.Selection.removeObserver(self.sel_server)
        self.obj.Material = self.material
        self.obj.References = self.references
        doc = FreeCADGui.getDocument(self.obj.Document)
        doc.resetEdit()
        doc.Document.recompute()

    def reject(self):
        if self.sel_server:
            FreeCADGui.Selection.removeObserver(self.sel_server)
        doc = FreeCADGui.getDocument(self.obj.Document)
        doc.resetEdit()

    def goMatWeb(self):
        import webbrowser
        webbrowser.open("http://matweb.com")

    def ym_changed(self, value):
        import Units
        old_ym = Units.Quantity(self.material['YoungsModulus'])
        if old_ym != value:
            material = self.material
            # FreeCAD uses kPa internall for Stress
            material['YoungsModulus'] = unicode(value) + " kPa"
            self.material = material

    def density_changed(self, value):
        import Units
        old_density = Units.Quantity(self.material['Density'])
        if old_density != value:
            material = self.material
            material['Density'] = unicode(value) + " kg/mm^3"
            self.material = material

    def pr_changed(self, value):
        import Units
        old_pr = Units.Quantity(self.material['PoissonRatio'])
        if old_pr != value:
            material = self.material
            material['PoissonRatio'] = unicode(value)
            self.material = material

    def choose_material(self, index):
        if index < 0:
            return
        mat_file_path = self.form.cb_materials.itemData(index)
        self.material = self.materials[mat_file_path]
        self.form.cb_materials.setCurrentIndex(index)
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
        self.fem_preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        use_built_in_materials = self.fem_preferences.GetBool("UseBuiltInMaterials", True)
        if use_built_in_materials:
            system_mat_dir = FreeCAD.getResourceDir() + "/Mod/Material/StandardMaterial"
            self.add_mat_dir(system_mat_dir, ":/icons/freecad.svg")

        use_mat_from_config_dir = self.fem_preferences.GetBool("UseMaterialsFromConfigDir", True)
        if use_mat_from_config_dir:
            user_mat_dirname = FreeCAD.getUserAppDataDir() + "Materials"
            self.add_mat_dir(user_mat_dirname, ":/icons/preferences-general.svg")

        use_mat_from_custom_dir = self.fem_preferences.GetBool("UseMaterialsFromCustomDir", True)
        if use_mat_from_custom_dir:
            custom_mat_dir = self.fem_preferences.GetString("CustomMaterialsDir", "")
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
            refname_to_compare_listentry = ref[0].Name + '-->' + ref[1]
            if refname_to_compare_listentry == currentItemName:
                self.references.remove(ref)
        self.rebuild_list_References()

    def add_references(self):
        '''Called if Button add_reference is triggered'''
        # in constraints EditTaskPanel the selection is active as soon as the taskpanel is open
        # here the addReference button EditTaskPanel has to be triggered to start selection mode
        FreeCADGui.Selection.clearSelection()
        # start SelectionObserver and parse the function to add the References to the widget
        self.sel_server = ReferenceShapeSelectionObserver(self.selectionParser)

    def selectionParser(self, selection):
        print 'selection: ', selection[0].Shape.ShapeType, '  ', selection[0].Name, '  ', selection[1]
        if hasattr(selection[0], "Shape"):
            elt = selection[0].Shape.getElement(selection[1])
            if elt.ShapeType == 'Edge' or elt.ShapeType == 'Face':
                if selection not in self.references:
                    self.references.append(selection)
                    self.rebuild_list_References()
                else:
                    print selection[0].Name, '-->', selection[1], ' is already in reference list!'
            else:
                print 'Select Edge or Face!'
        else:
            print 'Selection has no shape!'

    def rebuild_list_References(self):
        self.form.list_References.clear()
        items = []
        for i in self.references:
            item_name = i[0].Name + '-->' + i[1]
            items.append(item_name)
        for listItemName in sorted(items):
            listItem = QtGui.QListWidgetItem(listItemName, self.form.list_References)  # listItem =   is needed


class ReferenceShapeSelectionObserver:
    '''ReferenceShapeSelectionObserver
       started on click  button addReference'''
    def __init__(self, parseSelectionFunction):
        self.parseSelectionFunction = parseSelectionFunction
        FreeCADGui.Selection.addObserver(self)
        FreeCAD.Console.PrintMessage("Select Edges or Faces!\n")

    def addSelection(self, docName, objName, sub, pos):
        selected_object = FreeCAD.getDocument(docName).getObject(objName)  # get the obj objName
        self.added_obj = (selected_object, sub)
        if sub:         # on doubleClick the solid is selected and sub will be empty
            self.parseSelectionFunction(self.added_obj)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_MechanicalMaterial', _CommandMechanicalMaterial())

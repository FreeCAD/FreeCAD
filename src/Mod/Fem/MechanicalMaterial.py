#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import FreeCAD

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
    _ViewProviderMechanicalMaterial(obj.ViewObject)
    # FreeCAD.ActiveDocument.recompute()
    return obj


class _CommandMechanicalMaterial:
    "the Fem Material command definition"
    def GetResources(self):
        return {'Pixmap': 'Fem_Material',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Material", "Mechanical material..."),
                'Accel': "M, M",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Material", "Creates or edit the mechanical material definition.")}

    def Activated(self):
        MatObj = None
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("App::MaterialObject"):
                    MatObj = i

        if (not MatObj):
            FreeCAD.ActiveDocument.openTransaction("Create Material")
            FreeCADGui.addModule("MechanicalMaterial")
            FreeCADGui.doCommand("MechanicalMaterial.makeMechanicalMaterial('MechanicalMaterial')")
            FreeCADGui.doCommand("App.activeDocument()." + FemGui.getActiveAnalysis().Name + ".Member = App.activeDocument()." + FemGui.getActiveAnalysis().Name + ".Member + [App.ActiveDocument.ActiveObject]")
            FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name,0)")
            # FreeCADGui.doCommand("Fem.makeMaterial()")
        else:
            FreeCADGui.doCommand("Gui.activeDocument().setEdit('" + MatObj.Name + "',0)")

    def IsActive(self):
        if FemGui.getActiveAnalysis():
            return True
        else:
            return False


class _MechanicalMaterial:
    "The Material object"
    def __init__(self, obj):
        self.Type = "MechanicaltMaterial"
        obj.Proxy = self
        # obj.Material = StartMat

    def execute(self, obj):
        return


class _ViewProviderMechanicalMaterial:
    "A View Provider for the MechanicalMaterial object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/Fem_Material.svg"

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
        self.obj = obj

        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/MechanicalMaterial.ui")

        QtCore.QObject.connect(self.form.pushButton_MatWeb, QtCore.SIGNAL("clicked()"), self.goMatWeb)
        QtCore.QObject.connect(self.form.cb_materials, QtCore.SIGNAL("activated(int)"), self.choose_material)
        QtCore.QObject.connect(self.form.input_fd_young_modulus, QtCore.SIGNAL("valueChanged(double)"), self.ym_changed)
        QtCore.QObject.connect(self.form.spinBox_poisson_ratio, QtCore.SIGNAL("valueChanged(double)"), self.pr_changed)
        self.previous_material = self.obj.Material
        self.import_materials()
        previous_mat_path = self.get_material_path(self.previous_material)
        if not previous_mat_path:
            print "Previously used material cannot be found in material directories. Using transient material."
            material_name = self.get_material_name(self.previous_material)
            if material_name != 'None':
                self.add_transient_material(self.previous_material)
                index = self.form.cb_materials.findData(material_name)
            else:
                index = self.form.cb_materials.findText(material_name)
            self.choose_material(index)
        else:
            index = self.form.cb_materials.findData(previous_mat_path)
            self.choose_material(index)

    def accept(self):
        FreeCADGui.ActiveDocument.resetEdit()

    def reject(self):
        self.obj.Material = self.previous_material
        print "Reverting to material:"
        self.print_mat_data(self.previous_material)
        FreeCADGui.ActiveDocument.resetEdit()

# Function not yet used
#    def saveMat(self):
#        self.transferTo()
#        filename = QtGui.QFileDialog.getSaveFileName(None, 'Save Material file file',self.params.GetString("MaterialDir",'/'),'FreeCAD material file (*.FCMat)')
#        if(filename):
#            import Material
#            Material.exportFCMat(filename,self.obj.Material)

    def goMatWeb(self):
        import webbrowser
        webbrowser.open("http://matweb.com")

    def ym_changed(self, value):
        import Units
        old_ym = Units.Quantity(self.obj.Material['YoungsModulus'])
        if old_ym != value:
            material = self.obj.Material
            material['YoungsModulus'] = unicode(value)
            self.obj.Material = material

    def pr_changed(self, value):
        import Units
        old_pr = Units.Quantity(self.obj.Material['PoissonRatio'])
        if old_pr != value:
            material = self.obj.Material
            material['PoissonRatio'] = unicode(value)
            self.obj.Material = material

    def choose_material(self, index):
        if index < 0:
            return
        mat_file_path = self.form.cb_materials.itemData(index)
        self.obj.Material = self.materials[mat_file_path]
        self.form.cb_materials.setCurrentIndex(index)
        self.set_mat_params_in_combo_box(self.obj.Material)
        gen_mat_desc = ""
        if 'General_description' in self.obj.Material:
            gen_mat_desc = self.obj.Material['General_description']
        self.form.l_mat_description.setText(gen_mat_desc)
        self.print_mat_data(self.obj.Material)

    def get_material_name(self, material):
        if 'General_name' in self.previous_material:
            return self.previous_material['General_name']
        else:
            return 'None'

    def get_material_path(self, material):
        for a_mat in self.materials:
            unmatched_items = set(self.materials[a_mat].items()) ^ set(material.items())
            if len(unmatched_items) == 0:
                return a_mat
        return ""

    def print_mat_data(self, matmap):
        print 'Material data:'
        print ' Name = {}'.format(self.get_material_name(matmap))
        if 'YoungsModulus' in matmap:
            print ' YM = ', matmap['YoungsModulus']
        if 'PoissonRatio' in matmap:
            print ' PR = ', matmap['PoissonRatio']

    def set_mat_params_in_combo_box(self, matmap):
        if 'YoungsModulus' in matmap:
            self.form.input_fd_young_modulus.setText(matmap['YoungsModulus'])
        if 'PoissonRatio' in matmap:
            self.form.spinBox_poisson_ratio.setValue(float(matmap['PoissonRatio']))

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
            custom_mat_dir = self.fem_preferences.GetString("CustomMaterialsDir","")
            self.add_mat_dir(custom_mat_dir, ":/icons/user.svg")


FreeCADGui.addCommand('Fem_MechanicalMaterial', _CommandMechanicalMaterial())

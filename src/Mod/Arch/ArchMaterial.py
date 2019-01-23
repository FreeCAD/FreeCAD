#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - Yorik van Havre <yorik@uncreated.net>            *
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
    import FreeCADGui, Arch_rc, os
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

__title__ = "Arch Material Management"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

## @package ArchMaterial
#  \ingroup ARCH
#  \brief The Material object and tools
#
#  This module provides tools to add materials to
#  Arch objects

def makeMaterial(name="Material"):

    '''makeMaterial(name): makes an Material object'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("App::MaterialObjectPython",name)
    obj.Label = name
    _ArchMaterial(obj)
    if FreeCAD.GuiUp:
        _ViewProviderArchMaterial(obj.ViewObject)
    getMaterialContainer().addObject(obj)
    return obj


def getMaterialContainer():

    '''getMaterialContainer(): returns a group object to put materials in'''
    for obj in FreeCAD.ActiveDocument.Objects:
        if obj.Name == "MaterialContainer":
            return obj
    obj = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroupPython","MaterialContainer")
    obj.Label = "Materials"
    _ArchMaterialContainer(obj)
    if FreeCAD.GuiUp:
        _ViewProviderArchMaterialContainer(obj.ViewObject)
    return obj


def makeMultiMaterial(name="MultiMaterial"):

    '''makeMultiMaterial(name): makes an Material object'''
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython",name)
    obj.Label = name
    _ArchMultiMaterial(obj)
    if FreeCAD.GuiUp:
        _ViewProviderArchMultiMaterial(obj.ViewObject)
    getMaterialContainer().addObject(obj)
    return obj


def getDocumentMaterials():

    '''getDocumentMaterials(): returns all the arch materials of the document'''
    for obj in FreeCAD.ActiveDocument.Objects:
        if obj.Name == "MaterialContainer":
            mats = []
            for o in obj.Group:
                if o.isDerivedFrom("App::MaterialObjectPython"):
                    mats.append(o)
            return mats
    return []


class _CommandArchMaterial:


    "the Arch Material command definition"

    def GetResources(self):

        return {'Pixmap': 'Arch_Material_Group',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Material","Material"),
                'Accel': "M, T",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Material","Creates or edits the material definition of a selected object.")}

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create material"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.Control.closeDialog()
        FreeCADGui.doCommand("mat = Arch.makeMaterial()")
        for obj in sel:
            if hasattr(obj,"Material"):
                FreeCADGui.doCommand("FreeCAD.ActiveDocument.getObject(\""+obj.Name+"\").Material = mat")
        FreeCADGui.doCommandGui("mat.ViewObject.Document.setEdit(mat.ViewObject, 0)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    def IsActive(self):

        if FreeCAD.ActiveDocument:
            return True
        else:
            return False


class _CommandArchMultiMaterial:


    "the Arch MultiMaterial command definition"

    def GetResources(self):

        return {'Pixmap': 'Arch_Material_Multi',
                'MenuText': QT_TRANSLATE_NOOP("Arch_MultiMaterial","Multi-Material"),
                'Accel': "M, T",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_MultiMaterial","Creates or edits multi-materials")}

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create multi-material"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.Control.closeDialog()
        FreeCADGui.doCommand("mat = Arch.makeMultiMaterial()")
        for obj in sel:
            if hasattr(obj,"Material"):
                if not obj.isDerivedFrom("App::MaterialObject"):
                    FreeCADGui.doCommand("FreeCAD.ActiveDocument."+obj.Name+".Material = mat")
        FreeCADGui.doCommandGui("mat.ViewObject.Document.setEdit(mat.ViewObject, 0)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    def IsActive(self):

        if FreeCAD.ActiveDocument:
            return True
        else:
            return False


class _ArchMaterialContainer:


    "The Material Container"

    def __init__(self,obj):
        self.Type = "MaterialContainer"
        obj.Proxy = self

    def execute(self,obj):
        return

    def __getstate__(self):
        if hasattr(self,"Type"):
            return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state


class _ViewProviderArchMaterialContainer:


    "A View Provider for the Material Container"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/Arch_Material_Group.svg"

    def attach(self,vobj):
        self.Object = vobj.Object

    def setupContextMenu(self,vobj,menu):
        from PySide import QtCore,QtGui
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Arch_Material_Group.svg"),"Merge duplicates",menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),self.mergeByName)
        menu.addAction(action1)

    def mergeByName(self):
        if hasattr(self,"Object"):
            mats = [o for o in self.Object.Group if o.isDerivedFrom("App::MaterialObject")]
            todelete = []
            for mat in mats:
                if mat.Label[-1].isdigit() and mat.Label[-2].isdigit() and mat.Label[-3].isdigit():
                    orig = None
                    for om in mats:
                        if om.Label == mat.Label[:-3].strip():
                            orig = om
                            break
                    if orig:
                        for par in mat.InList:
                            for prop in par.PropertiesList:
                                if getattr(par,prop) == mat:
                                    FreeCAD.Console.PrintMessage("Changed property '"+prop+"' of object "+par.Label+" from "+mat.Label+" to "+orig.Label+"\n")
                                    setattr(par,prop,orig)
                        todelete.append(mat)
            for tod in todelete:
                if not tod.InList:
                    FreeCAD.Console.PrintMessage("Merging duplicate material "+tod.Label+"\n")
                    FreeCAD.ActiveDocument.removeObject(tod.Name)
                elif (len(tod.InList) == 1) and (tod.InList[0].isDerivedFrom("App::DocumentObjectGroup")):
                    FreeCAD.Console.PrintMessage("Merging duplicate material "+tod.Label+"\n")
                    FreeCAD.ActiveDocument.removeObject(tod.Name)
                else:
                    FreeCAD.Console.PrintMessage("Unable to delete material "+tod.Label+": InList not empty\n")

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class _ArchMaterial:


    "The Material object"

    def __init__(self,obj):
        self.Type = "Material"
        obj.Proxy = self
        obj.addProperty("App::PropertyString","Description","Arch",QT_TRANSLATE_NOOP("App::Property","A description for this material"))
        obj.addProperty("App::PropertyString","StandardCode","Arch",QT_TRANSLATE_NOOP("App::Property","A standard code (MasterFormat, OmniClass,...)"))
        obj.addProperty("App::PropertyString","ProductURL","Arch",QT_TRANSLATE_NOOP("App::Property","A URL where to find information about this material"))
        obj.addProperty("App::PropertyPercent","Transparency","Arch",QT_TRANSLATE_NOOP("App::Property","The transparency value of this material"))
        obj.addProperty("App::PropertyColor","Color","Arch",QT_TRANSLATE_NOOP("App::Property","The color of this material"))

    def onChanged(self,obj,prop):
        d = None
        if prop == "Material":
            if "DiffuseColor" in obj.Material:
                c = tuple([float(f) for f in obj.Material['DiffuseColor'].strip("()").split(",")])
                if hasattr(obj,"Color"):
                    if obj.Color != c:
                        obj.Color = c
            if "Transparency" in obj.Material:
                t = int(obj.Material['Transparency'])
                if hasattr(obj,"Transparency"):
                    if obj.Transparency != t:
                        obj.Transparency = t
            if "ProductURL" in obj.Material:
                if hasattr(obj,"ProductURL"):
                    if obj.ProductURL != obj.Material["ProductURL"]:
                        obj.ProductURL = obj.Material["ProductURL"]
            if "StandardCode" in obj.Material:
                if hasattr(obj,"StandardCode"):
                    if obj.StandardCode != obj.Material["StandardCode"]:
                        obj.StandardCode = obj.Material["StandardCode"]
            if "Description" in obj.Material:
                if hasattr(obj,"Description"):
                    if obj.Description != obj.Material["Description"]:
                        obj.Description = obj.Material["Description"]
        elif prop == "Color":
            if hasattr(obj,"Color"):
                if obj.Material:
                    d = obj.Material
                    val = str(obj.Color[:3])
                    if "DiffuseColor" in d:
                        if d["DiffuseColor"] == val:
                            return
                    d["DiffuseColor"] = val
        elif prop == "Transparency":
            if hasattr(obj,"Transparency"):
                if obj.Material:
                    d = obj.Material
                    val = str(obj.Transparency)
                    if "Transparency" in d:
                        if d["Transparency"] == val:
                            return
                    d["Transparency"] = val
        elif prop == "ProductURL":
            if hasattr(obj,"ProductURL"):
                if obj.Material:
                    d = obj.Material
                    val = obj.ProductURL
                    if "ProductURL" in d:
                        if d["ProductURL"] == val:
                            return
                    obj.Material["ProductURL"] = val
        elif prop == "StandardCode":
            if hasattr(obj,"StandardCode"):
                if obj.Material:
                    d = obj.Material
                    val = obj.StandardCode
                    if "StandardCode" in d:
                        if d["StandardCode"] == val:
                            return
                    d["StandardCode"] = val
        elif prop == "Description":
            if hasattr(obj,"Description"):
                if obj.Material:
                    d = obj.Material
                    val = obj.Description
                    if "Description" in d:
                        if d["Description"] == val:
                            return
                    d["Description"] = val
        if d:
            obj.Material = d
            if FreeCAD.GuiUp:
                import FreeCADGui
                # not sure why this is needed, but it is...
                FreeCADGui.ActiveDocument.resetEdit()

    def execute(self,obj):
        if obj.Material:
            if FreeCAD.GuiUp:
                if "DiffuseColor" in obj.Material:
                    c = tuple([float(f) for f in obj.Material['DiffuseColor'].strip("()").split(",")])
                    for p in obj.InList:
                        if hasattr(p,"Material"):
                            if p.Material.Name == obj.Name:
                                p.ViewObject.ShapeColor = c
        return

    def __getstate__(self):
        if hasattr(self,"Type"):
            return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state


class _ViewProviderArchMaterial:

    "A View Provider for the Material object"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/Arch_Material.svg"

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        if prop == "Material":
            if "Father" in vobj.Object.Material:
                for o in FreeCAD.ActiveDocument.Objects:
                    if o.isDerivedFrom("App::MaterialObject"):
                        if o.Label == vobj.Object.Material["Father"]:
                            o.touch()

    def setEdit(self,vobj,mode):
        self.taskd = _ArchMaterialTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(self.taskd)
        self.taskd.form.FieldName.setFocus()
        self.taskd.form.FieldName.selectAll()
        return True

    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        if hasattr(self,"taskd"):
            del self.taskd
        return

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def claimChildren(self):
        ch = []
        if hasattr(self,"Object"):
            for o in self.Object.Document.Objects:
                if o.isDerivedFrom("App::MaterialObject"):
                    if o.Material:
                        if "Father" in o.Material:
                            if o.Material["Father"] == self.Object.Label:
                                ch.append(o)
        return ch


class _ArchMaterialTaskPanel:

    '''The editmode TaskPanel for Arch Material objects'''

    def __init__(self,obj=None):
        self.cards = None
        self.existingmaterials = []
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/ArchMaterial.ui")
        self.color = QtGui.QColor(128,128,128)
        colorPix = QtGui.QPixmap(16,16)
        colorPix.fill(self.color)
        self.form.ButtonColor.setIcon(QtGui.QIcon(colorPix))
        self.form.ButtonUrl.setIcon(QtGui.QIcon(":/icons/internet-web-browser.svg"))
        QtCore.QObject.connect(self.form.comboBox_MaterialsInDir, QtCore.SIGNAL("currentIndexChanged(QString)"), self.chooseMat)
        QtCore.QObject.connect(self.form.comboBox_FromExisting, QtCore.SIGNAL("currentIndexChanged(int)"), self.fromExisting)
        QtCore.QObject.connect(self.form.comboFather, QtCore.SIGNAL("currentIndexChanged(QString)"), self.setFather)
        QtCore.QObject.connect(self.form.comboFather, QtCore.SIGNAL("currentTextChanged(QString)"), self.setFather)
        QtCore.QObject.connect(self.form.ButtonColor,QtCore.SIGNAL("pressed()"),self.getColor)
        QtCore.QObject.connect(self.form.ButtonUrl,QtCore.SIGNAL("pressed()"),self.openUrl)
        QtCore.QObject.connect(self.form.ButtonEditor,QtCore.SIGNAL("pressed()"),self.openEditor)
        QtCore.QObject.connect(self.form.ButtonCode,QtCore.SIGNAL("pressed()"),self.getCode)
        self.fillMaterialCombo()
        self.fillExistingCombo()
        try:
            import BimClassification
        except:
            self.form.ButtonCode.hide()
        else:
            import os
            self.form.ButtonCode.setIcon(QtGui.QIcon(os.path.join(os.path.dirname(BimClassification.__file__),"icons","BIM_Classification.svg")))
        if self.obj:
            if hasattr(self.obj,"Material"):
                self.material = self.obj.Material
        self.setFields()

    def setFields(self):
        "sets the task box contents from self.material"
        if 'Name' in self.material:
            self.form.FieldName.setText(self.material['Name'])
        elif self.obj:
            self.form.FieldName.setText(self.obj.Label)
        if 'Description' in self.material:
            self.form.FieldDescription.setText(self.material['Description'])
        col = None
        if 'DiffuseColor' in self.material:
            col = self.material["DiffuseColor"]
        elif 'ViewColor' in self.material:
            col = self.material["ViewColor"]
        elif 'Color' in self.material:
            col = self.material["Color"]
        if col:
            if "(" in col:
                c = tuple([float(f) for f in col.strip("()").split(",")])
                self.color = QtGui.QColor()
                self.color.setRgbF(c[0],c[1],c[2])
                colorPix = QtGui.QPixmap(16,16)
                colorPix.fill(self.color)
                self.form.ButtonColor.setIcon(QtGui.QIcon(colorPix))
        if 'StandardCode' in self.material:
            self.form.FieldCode.setText(self.material['StandardCode'])
        if 'ProductURL' in self.material:
            self.form.FieldUrl.setText(self.material['ProductURL'])
        if 'Transparency' in self.material:
            self.form.SpinBox_Transparency.setValue(int(self.material["Transparency"]))
        if "Father" in self.material:
            father = self.material["Father"]
        else:
            father = None
        found = False
        self.form.comboFather.addItem("None")
        for o in FreeCAD.ActiveDocument.Objects:
            if o.isDerivedFrom("App::MaterialObject"):
                if o != self.obj:
                    self.form.comboFather.addItem(o.Label)
                    if o.Label == father:
                        self.form.comboFather.setCurrentIndex(self.form.comboFather.count()-1)
                        found = True
        if father and not found:
            self.form.comboFather.addItem(father)
            self.form.comboFather.setCurrentIndex(self.form.comboFather.count()-1)
            

    def getFields(self):
        "sets self.material from the contents of the task box"
        self.material['Name'] = self.form.FieldName.text()
        self.material['Description'] = self.form.FieldDescription.text()
        self.material['DiffuseColor'] = str(self.color.getRgbF()[:3])
        self.material['ViewColor'] = self.material['DiffuseColor']
        self.material['Color'] = self.material['DiffuseColor']
        self.material['StandardCode'] = self.form.FieldCode.text()
        self.material['ProductURL'] = self.form.FieldUrl.text()
        self.material['Transparency'] = str(self.form.SpinBox_Transparency.value())

    def accept(self):
        self.getFields()
        if self.obj:
            if hasattr(self.obj,"Material"):
                self.obj.Material = self.material
                self.obj.Label = self.material['Name']
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def chooseMat(self, card):
        "sets self.material from a card"
        if card in self.cards:
            import importFCMat
            self.material = importFCMat.read(self.cards[card])
            self.setFields()

    def fromExisting(self,index):
        "sets the contents from an existing material"
        if index > 0:
            if index <= len(self.existingmaterials):
                m = self.existingmaterials[index-1]
                if m.Material:
                    self.material = m.Material
                    self.setFields()

    def setFather(self,text):
        "sets the father"
        if text:
            if text != "None":
                self.material["Father"] = text

    def getColor(self):
        "opens a color picker dialog"
        self.color = QtGui.QColorDialog.getColor()
        colorPix = QtGui.QPixmap(16,16)
        colorPix.fill(self.color)
        self.form.ButtonColor.setIcon(QtGui.QIcon(colorPix))

    def fillMaterialCombo(self):
        "fills the combo with the existing FCMat cards"
        # look for cards in both resources dir and a Materials sub-folder in the user folder.
        # User cards with same name will override system cards
        paths = [FreeCAD.getResourceDir() + os.sep + "Mod" + os.sep + "Material" + os.sep + "StandardMaterial"]
        ap = FreeCAD.ConfigGet("UserAppData") + os.sep + "Materials"
        if os.path.exists(ap):
            paths.append(ap)
        self.cards = {}
        for p in paths:
            for f in os.listdir(p):
                b,e = os.path.splitext(f)
                if e.upper() == ".FCMAT":
                    self.cards[b] = p + os.sep + f
        if self.cards:
            for k in sorted(self.cards.keys()):
                self.form.comboBox_MaterialsInDir.addItem(k)

    def fillExistingCombo(self):
        "fills the existing materials combo"
        self.existingmaterials = []
        for obj in FreeCAD.ActiveDocument.Objects:
            if obj.isDerivedFrom("App::MaterialObject"):
                if obj != self.obj:
                    self.existingmaterials.append(obj)
        for m in self.existingmaterials:
            self.form.comboBox_FromExisting.addItem(m.Label)


    def openEditor(self):
        "opens the full material editor from the material module"
        self.getFields()
        if self.material:
            import MaterialEditor
            self.material = MaterialEditor.editMaterial(self.material)
            self.setFields()

    def openUrl(self):
        self.getFields()
        if self.material:
            if 'ProductURL' in self.material:
                QtGui.QDesktopServices.openUrl(self.material['ProductURL'])

    def getCode(self):
        FreeCADGui.Selection.addSelection(self.obj)
        FreeCADGui.runCommand("BIM_Classification")


class _ArchMultiMaterial:

    "The MultiMaterial object"

    def __init__(self,obj):
        self.Type = "MultiMaterial"
        obj.Proxy = self
        obj.addProperty("App::PropertyString","Description","Arch",QT_TRANSLATE_NOOP("App::Property","A description for this material"))
        obj.addProperty("App::PropertyStringList","Names","Arch",QT_TRANSLATE_NOOP("App::Property","The list of layer names"))
        obj.addProperty("App::PropertyLinkList","Materials","Arch",QT_TRANSLATE_NOOP("App::Property","The list of layer materials"))
        obj.addProperty("App::PropertyFloatList","Thicknesses","Arch",QT_TRANSLATE_NOOP("App::Property","The list of layer thicknesses"))

    def __getstate__(self):
        if hasattr(self,"Type"):
            return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state

class _ViewProviderArchMultiMaterial:

    "A View Provider for the MultiMaterial object"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/Arch_Material_Multi.svg"

    def setEdit(self,vobj,mode=0):
        taskd = _ArchMultiMaterialTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode=0):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        return True

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def doubleClicked(self,vobj):
        self.setEdit(vobj)
        
    def isShow(self):
        return True

if FreeCAD.GuiUp:

    class MultiMaterialDelegate(QtGui.QStyledItemDelegate):

        def __init__(self, parent=None, *args):
            self.mats = []
            for obj in FreeCAD.ActiveDocument.Objects:
                if obj.isDerivedFrom("App::MaterialObject"):
                    self.mats.append(obj)
            QtGui.QStyledItemDelegate.__init__(self, parent, *args)
    
        def createEditor(self,parent,option,index):
            if index.column() == 0:
                editor = QtGui.QComboBox(parent)
                editor.setEditable(True)
            elif index.column() == 1:
                editor = QtGui.QComboBox(parent)
            elif index.column() == 2:
                ui = FreeCADGui.UiLoader()
                editor = ui.createWidget("Gui::InputField")
                editor.setSizePolicy(QtGui.QSizePolicy.Preferred,QtGui.QSizePolicy.Minimum)
                editor.setParent(parent)
            else:
                editor = QtGui.QLineEdit(parent)
            return editor
    
        def setEditorData(self, editor, index):
            if index.column() == 0:
                import ArchWindow
                editor.addItems([index.data()]+ArchWindow.WindowPartTypes)
            elif index.column() == 1:
                idx = -1
                for i,m in enumerate(self.mats):
                    editor.addItem(m.Label)
                    if m.Label == index.data():
                        idx = i
                editor.setCurrentIndex(idx)
            else:
                QtGui.QStyledItemDelegate.setEditorData(self, editor, index)
    
        def setModelData(self, editor, model, index):
            if index.column() == 0:
                if editor.currentIndex() == -1:
                    model.setData(index, "")
                else:
                    model.setData(index, editor.currentText())
            elif index.column() == 1:
                if editor.currentIndex() == -1:
                    model.setData(index, "")
                else:
                    model.setData(index, self.mats[editor.currentIndex()].Label)
            else:
                QtGui.QStyledItemDelegate.setModelData(self, editor, model, index)


class _ArchMultiMaterialTaskPanel:

    '''The editmode TaskPanel for MultiMaterial objects'''

    def __init__(self,obj=None):
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/ArchMultiMaterial.ui")
        self.model = QtGui.QStandardItemModel()
        self.model.setHorizontalHeaderLabels([translate("Arch","Name"),translate("Arch","Material"),translate("Arch","Thickness")])
        self.form.tree.setModel(self.model)
        self.form.tree.setUniformRowHeights(True)
        self.form.tree.setItemDelegate(MultiMaterialDelegate())
        QtCore.QObject.connect(self.form.chooseCombo, QtCore.SIGNAL("currentIndexChanged(int)"), self.fromExisting)
        QtCore.QObject.connect(self.form.addButton,QtCore.SIGNAL("pressed()"),self.addLayer)
        QtCore.QObject.connect(self.form.upButton,QtCore.SIGNAL("pressed()"),self.upLayer)
        QtCore.QObject.connect(self.form.downButton,QtCore.SIGNAL("pressed()"),self.downLayer)
        QtCore.QObject.connect(self.form.delButton,QtCore.SIGNAL("pressed()"),self.delLayer)
        self.fillExistingCombo()
        self.fillData()
        
    def fillData(self,obj=None):
        if not obj:
            obj = self.obj
        if obj:
            self.model.clear()
            self.model.setHorizontalHeaderLabels([translate("Arch","Name"),translate("Arch","Material"),translate("Arch","Thickness")])
            for i in range(len(obj.Names)):
                item1 = QtGui.QStandardItem(obj.Names[i])
                item2 = QtGui.QStandardItem(obj.Materials[i].Label)
                item3 = QtGui.QStandardItem(FreeCAD.Units.Quantity(obj.Thicknesses[i],FreeCAD.Units.Length).getUserPreferred()[0])
                self.model.appendRow([item1,item2,item3])
            self.form.nameField.setText(obj.Label)

    def fillExistingCombo(self):
        "fills the existing multimaterials combo"
        import Draft
        self.existingmaterials = []
        for obj in FreeCAD.ActiveDocument.Objects:
            if Draft.getType(obj) == "MultiMaterial":
                if obj != self.obj:
                    self.existingmaterials.append(obj)
        for m in self.existingmaterials:
            self.form.chooseCombo.addItem(m.Label)
            
    def fromExisting(self,index):
        "sets the contents from an existing material"
        if index > 0:
            if index <= len(self.existingmaterials):
                m = self.existingmaterials[index-1]
                if m:
                    self.fillData(m)
            
    def addLayer(self):
        item1 = QtGui.QStandardItem(translate("Arch","New layer"))
        item2 = QtGui.QStandardItem()
        item3 = QtGui.QStandardItem()
        self.model.appendRow([item1,item2,item3])
        
    def delLayer(self):
        sel = self.form.tree.selectedIndexes()
        if sel:
            row = sel[0].row()
            if row >= 0:
                self.model.takeRow(row)
        
    def moveLayer(self,mvt=0):
        sel = self.form.tree.selectedIndexes()
        if sel and mvt:
            row = sel[0].row()
            if row >= 0:
                if row+mvt >= 0:
                    data = self.model.takeRow(row)
                    self.model.insertRow(row+mvt,data)
                    ind = self.model.index(row+mvt,0)
                    self.form.tree.setCurrentIndex(ind)

    def upLayer(self):
        self.moveLayer(mvt=-1)
        
    def downLayer(self):
        self.moveLayer(mvt=1)
        
    def accept(self):
        if self.obj:
            mats = []
            for m in FreeCAD.ActiveDocument.Objects:
                if m.isDerivedFrom("App::MaterialObject"):
                    mats.append(m)
            names = []
            materials = []
            thicknesses = []
            for row in range(self.model.rowCount()):
                name = self.model.item(row,0).text()
                mat = None
                ml = self.model.item(row,1).text()
                for m in mats:
                    if m.Label == ml:
                        mat = m
                d = self.model.item(row,2).text()
                try:
                    d = float(d)
                except:
                    thick = FreeCAD.Units.Quantity(d).Value
                else:
                    thick = FreeCAD.Units.Quantity(d,FreeCAD.Units.Length).Value
                if round(thick,32) == 0:
                    thick = 0.0
                if name and mat:
                    names.append(name)
                    materials.append(mat)
                    thicknesses.append(thick)
            self.obj.Names = names
            self.obj.Materials = materials
            self.obj.Thicknesses = thicknesses
            if self.form.nameField.text():
                self.obj.Label = self.form.nameField.text()
        FreeCAD.ActiveDocument.recompute()
        return True


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Material',_CommandArchMaterial())
    FreeCADGui.addCommand('Arch_MultiMaterial',_CommandArchMultiMaterial())

    class _ArchMaterialToolsCommand:

        def GetCommands(self):
            return tuple(['Arch_Material','Arch_MultiMaterial'])
        def GetResources(self):
            return { 'MenuText': QT_TRANSLATE_NOOP("Arch_MaterialTools",'Material tools'),
                     'ToolTip': QT_TRANSLATE_NOOP("Arch_MaterialTools",'Material tools')
                   }
        def IsActive(self):
            return not FreeCAD.ActiveDocument is None

    FreeCADGui.addCommand('Arch_MaterialTools', _ArchMaterialToolsCommand())

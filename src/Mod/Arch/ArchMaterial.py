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
    _ViewProviderArchMaterialContainer(obj.ViewObject)
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
                'MenuText': QT_TRANSLATE_NOOP("Arch_Material","Set material..."),
                'Accel': "M, T",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Material","Creates or edits the material definition of a selected object.")}

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create material"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.Control.closeDialog()
        FreeCADGui.doCommand("mat = Arch.makeMaterial()")
        for obj in sel:
            if hasattr(obj,"BaseMaterial"):
                FreeCADGui.doCommand("FreeCAD.ActiveDocument."+obj.Name+".BaseMaterial = mat")
        FreeCADGui.doCommandGui("mat.ViewObject.startEditing()")
        FreeCAD.ActiveDocument.commitTransaction()

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


class _ViewProviderArchMaterialContainer:
    "A View Provider for the Material Container"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/Arch_Material_Group.svg"


class _ArchMaterial:
    "The Material object"
    def __init__(self,obj):
        self.Type = "Material"
        obj.Proxy = self
        obj.addProperty("App::PropertyString","Description","Arch",QT_TRANSLATE_NOOP("App::Property","A description for this material"))
        obj.addProperty("App::PropertyString","StandardCode","Arch",QT_TRANSLATE_NOOP("App::Property","A standard code (MasterFormat, OmniClass,...)"))
        obj.addProperty("App::PropertyString","ProductURL","Arch",QT_TRANSLATE_NOOP("App::Property","An URL where to find information about this material"))
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
            if FreeCADGui:
                # not sure why this is needed, but it is...
                FreeCADGui.ActiveDocument.resetEdit()
        
    def execute(self,obj):
        if obj.Material: 
            if FreeCAD.GuiUp:
                if "DiffuseColor" in obj.Material:
                    c = tuple([float(f) for f in obj.Material['DiffuseColor'].strip("()").split(",")])
                    for p in obj.InList:
                        if hasattr(p,"BaseMaterial"):
                            if p.BaseMaterial.Name == obj.Name:
                                p.ViewObject.ShapeColor = c
        return


class _ViewProviderArchMaterial:
    "A View Provider for the Material object"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/Arch_Material.svg"

    def attach(self, vobj):
        return

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self,vobj,mode):
        taskd = _ArchMaterialTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class _ArchMaterialTaskPanel:
    '''The editmode TaskPanel for MechanicalMaterial objects'''
    def __init__(self,obj=None):
        self.cards = None
        self.existingmaterials = []
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/ArchMaterial.ui")
        self.color = QtGui.QColor(128,128,128)
        colorPix = QtGui.QPixmap(16,16)
        colorPix.fill(self.color)
        self.form.ButtonColor.setIcon(QtGui.QIcon(colorPix))
        QtCore.QObject.connect(self.form.comboBox_MaterialsInDir, QtCore.SIGNAL("currentIndexChanged(QString)"), self.chooseMat)
        QtCore.QObject.connect(self.form.comboBox_FromExisting, QtCore.SIGNAL("currentIndexChanged(int)"), self.fromExisting)
        QtCore.QObject.connect(self.form.ButtonColor,QtCore.SIGNAL("pressed()"),self.getColor)
        QtCore.QObject.connect(self.form.ButtonUrl,QtCore.SIGNAL("pressed()"),self.openUrl)
        QtCore.QObject.connect(self.form.ButtonEditor,QtCore.SIGNAL("pressed()"),self.openEditor)
        QtCore.QObject.connect(self.form.ButtonCode,QtCore.SIGNAL("pressed()"),self.getCode)
        self.fillMaterialCombo()
        self.fillExistingCombo()
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
        baseurl = "http://bsdd.buildingsmart.org/#concept/browse"
        QtGui.QDesktopServices.openUrl(baseurl)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Material',_CommandArchMaterial())

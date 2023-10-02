#***************************************************************************
#*   Copyright (c) 2014 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

import pathlib
import FreeCAD, Part
from PySide import QtCore
from fcgear import involute
from fcgear import fcgear

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui

__title__="PartDesign InvoluteGearObject management"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecad.org"


def makeInvoluteGear(name):
    '''makeInvoluteGear(name): makes an InvoluteGear'''
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",name)
    _InvoluteGear(obj)
    if FreeCAD.GuiUp:
        _ViewProviderInvoluteGear(obj.ViewObject)
    #FreeCAD.ActiveDocument.recompute()
    if FreeCAD.GuiUp:
        body=FreeCADGui.ActiveDocument.ActiveView.getActiveObject("pdbody")
        part=FreeCADGui.ActiveDocument.ActiveView.getActiveObject("part")
        if body:
            body.Group=body.Group+[obj]
        elif part:
            part.Group=part.Group+[obj]
    return obj


class _CommandInvoluteGear:
    "GUI command to create an InvoluteGear"
    def GetResources(self):
        return {'Pixmap'  : 'PartDesign_InternalExternalGear',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PartDesign_InvoluteGear","Involute gear..."),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PartDesign_InvoluteGear","Creates or edit the involute gear definition.")}

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create involute gear")
        FreeCADGui.addModule("InvoluteGearFeature")
        FreeCADGui.doCommand("InvoluteGearFeature.makeInvoluteGear('InvoluteGear')")
        FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name,0)")

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False


class _InvoluteGear:
    "The InvoluteGear object"
    def __init__(self,obj):
        self.Type = "InvoluteGear"
        self._ensure_properties(obj, is_restore=False)
        obj.Proxy = self

    def onDocumentRestored(self, obj):
        """hook used to migrate older versions of this object"""
        self._ensure_properties(obj, is_restore=True)

    def _ensure_properties(self, obj, is_restore):
        def ensure_property(type_, name, doc, default):
            if not hasattr(obj, name):
                obj.addProperty(type_, name, "Gear", doc)
                if callable(default):
                    setattr(obj, name, default())
                else:
                    setattr(obj, name, default)

        # for details about the property's docstring translation,
        # see https://tracker.freecad.org/view.php?id=2524
        ensure_property("App::PropertyInteger", "NumberOfTeeth",
            doc=QtCore.QT_TRANSLATE_NOOP("App::Property", "Number of gear teeth"),
            default=26)
        ensure_property("App::PropertyLength", "Modules",
            doc=QtCore.QT_TRANSLATE_NOOP("App::Property", "Modules of the gear"),
            default="2.5 mm")
        ensure_property("App::PropertyAngle", "PressureAngle",
            doc=QtCore.QT_TRANSLATE_NOOP("App::Property", "Pressure angle of gear teeth"),
            default="20 deg")
        ensure_property("App::PropertyBool", "HighPrecision",
            doc=QtCore.QT_TRANSLATE_NOOP("App::Property",
                "True=2 curves with each 3 control points False=1 curve with 4 control points."),
            default=True)
        ensure_property("App::PropertyBool", "ExternalGear",
            doc=QtCore.QT_TRANSLATE_NOOP("App::Property", "True=external Gear False=internal Gear"),
            default=True)
        ensure_property("App::PropertyFloat", "AddendumCoefficient",
            doc=QtCore.QT_TRANSLATE_NOOP("App::Property",
                "The height of the tooth from the pitch circle up to its tip, normalized by the module."),
            default=lambda: 1.0 if obj.ExternalGear else 0.6)
        ensure_property("App::PropertyFloat","DedendumCoefficient",
            doc=QtCore.QT_TRANSLATE_NOOP("App::Property",
                "The height of the tooth from the pitch circle down to its root, normalized by the module."),
            default=1.25)
        ensure_property("App::PropertyFloat","RootFilletCoefficient",
            doc=QtCore.QT_TRANSLATE_NOOP("App::Property",
                "The radius of the fillet at the root of the tooth, normalized by the module."),
            default=lambda: 0.375 if is_restore else 0.38)
        ensure_property("App::PropertyFloat","ProfileShiftCoefficient",
            doc=QtCore.QT_TRANSLATE_NOOP("App::Property",
                "The distance by which the reference profile is shifted outwards, normalized by the module."),
            default=0.0)

    def execute(self,obj):
        w = fcgear.FCWireBuilder()
        generator_func = involute.CreateExternalGear if obj.ExternalGear else involute.CreateInternalGear
        generator_func(w, obj.Modules.Value, obj.NumberOfTeeth, obj.PressureAngle.Value,
            split=obj.HighPrecision, addCoeff=obj.AddendumCoefficient, dedCoeff=obj.DedendumCoefficient,
            filletCoeff=obj.RootFilletCoefficient, shiftCoeff=obj.ProfileShiftCoefficient)
        gearw = Part.Wire([o.toShape() for o in w.wire])
        obj.Shape = gearw
        obj.positionBySupport()
        return


class _ViewProviderInvoluteGear:
    "A View Provider for the InvoluteGear object"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/PartDesign_InternalExternalGear.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def setEdit(self,vobj,mode):
        taskd = _InvoluteGearTaskPanel(self.Object,mode)
        taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return

    def dumps(self):
        return None

    def loads(self,state):
        return None


class _InvoluteGearTaskPanel:
    '''The editmode TaskPanel for InvoluteGear objects'''
    def __init__(self,obj,mode):
        self.obj = obj

        self.form=FreeCADGui.PySideUic.loadUi(str(pathlib.Path(__file__).with_suffix(".ui")))
        self.form.setWindowIcon(QtGui.QIcon(":/icons/PartDesign_InternalExternalGear.svg"))
        self.assignToolTipsFromPropertyDocs()

        def assignValue(property_name, fitView=False):
            """Returns a function that takes a single value and assigns it to the given property"""
            def assigner(value):
                setattr(self.obj, property_name, value)
                self.obj.Proxy.execute(self.obj)
                if fitView:
                    FreeCAD.Gui.SendMsgToActiveView("ViewFit")
            return assigner

        def assignIndexAsBool(property_name):
            """Variant of assignValue that transforms the index of a Yes/No Combobox to a bool."""
            assigner = assignValue(property_name)
            def transformingAssigner(value):
                assigner(True if value == 0 else False)
            return transformingAssigner

        self.form.Quantity_Modules.valueChanged.connect(assignValue("Modules", fitView=True))
        self.form.Quantity_PressureAngle.valueChanged.connect(assignValue("PressureAngle"))
        self.form.spinBox_NumberOfTeeth.valueChanged.connect(assignValue("NumberOfTeeth", fitView=True))
        self.form.comboBox_HighPrecision.currentIndexChanged.connect(assignIndexAsBool("HighPrecision"))
        self.form.comboBox_ExternalGear.currentIndexChanged.connect(assignIndexAsBool("ExternalGear"))
        self.form.doubleSpinBox_Addendum.valueChanged.connect(assignValue("AddendumCoefficient"))
        self.form.doubleSpinBox_Dedendum.valueChanged.connect(assignValue("DedendumCoefficient"))
        self.form.doubleSpinBox_RootFillet.valueChanged.connect(assignValue("RootFilletCoefficient"))
        self.form.doubleSpinBox_ProfileShift.valueChanged.connect(assignValue("ProfileShiftCoefficient"))

        self.update()

        if mode == 0: # fresh created
            self.obj.Proxy.execute(self.obj)  # calculate once
            FreeCAD.Gui.SendMsgToActiveView("ViewFit")

    def assignToolTipsFromPropertyDocs(self):
        def assign(property_name, *widgets):
            doc = self.obj.getDocumentationOfProperty(property_name)
            translated_doc = QtGui.QApplication.translate("App::Property", doc)
            for w in widgets:
                w.setToolTip(translated_doc)

        # we assign the tool tip to both, the label and the input field, for user convenience
        assign("Modules", self.form.Quantity_Modules, self.form.label_Modules)
        assign("PressureAngle", self.form.Quantity_PressureAngle, self.form.label_PressureAngle)
        assign("NumberOfTeeth", self.form.spinBox_NumberOfTeeth, self.form.label_NumberOfTeeth)
        assign("HighPrecision", self.form.comboBox_HighPrecision, self.form.label_HighPrecision)
        assign("ExternalGear", self.form.comboBox_ExternalGear, self.form.label_ExternalGear)
        assign("AddendumCoefficient", self.form.doubleSpinBox_Addendum, self.form.label_Addendum)
        assign("DedendumCoefficient", self.form.doubleSpinBox_Dedendum, self.form.label_Dedendum)
        assign("RootFilletCoefficient", self.form.doubleSpinBox_RootFillet, self.form.label_RootFillet)
        assign("ProfileShiftCoefficient", self.form.doubleSpinBox_ProfileShift, self.form.label_ProfileShift)

    def changeEvent(self, event):
        if event == QtCore.QEvent.LanguageChange:
            self.assignToolTipsFromPropertyDocs()

    def transferTo(self):
        "Transfer from the dialog to the object"
        self.obj.NumberOfTeeth  = self.form.spinBox_NumberOfTeeth.value()
        self.obj.Modules        = self.form.Quantity_Modules.text()
        self.obj.PressureAngle  = self.form.Quantity_PressureAngle.text()
        self.obj.HighPrecision = True if self.form.comboBox_HighPrecision.currentIndex() == 0 else False
        self.obj.ExternalGear = True if self.form.comboBox_ExternalGear.currentIndex() == 0 else False
        self.obj.AddendumCoefficient = self.form.doubleSpinBox_Addendum.value()
        self.obj.DedendumCoefficient = self.form.doubleSpinBox_Dedendum.value()
        self.obj.RootFilletCoefficient = self.form.doubleSpinBox_RootFillet.value()
        self.obj.ProfileShiftCoefficient = self.form.doubleSpinBox_ProfileShift.value()

    def transferFrom(self):
        "Transfer from the object to the dialog"
        self.form.spinBox_NumberOfTeeth.setValue(self.obj.NumberOfTeeth)
        self.form.Quantity_Modules.setText(self.obj.Modules.UserString)
        self.form.Quantity_PressureAngle.setText(self.obj.PressureAngle.UserString)
        self.form.comboBox_HighPrecision.setCurrentIndex(0 if self.obj.HighPrecision else 1)
        self.form.comboBox_ExternalGear.setCurrentIndex(0 if self.obj.ExternalGear else 1)
        self.form.doubleSpinBox_Addendum.setValue(self.obj.AddendumCoefficient)
        self.form.doubleSpinBox_Dedendum.setValue(self.obj.DedendumCoefficient)
        self.form.doubleSpinBox_RootFillet.setValue(self.obj.RootFilletCoefficient)
        self.form.doubleSpinBox_ProfileShift.setValue(self.obj.ProfileShiftCoefficient)

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok) | int(QtGui.QDialogButtonBox.Cancel)| int(QtGui.QDialogButtonBox.Apply)

    def clicked(self,button):
        if button == QtGui.QDialogButtonBox.Apply:
            self.transferTo()
            self.obj.Proxy.execute(self.obj)

    def update(self):
        'fills the widgets'
        self.transferFrom()

    def accept(self):
        self.transferTo()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()

    def reject(self):
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.abortTransaction()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('PartDesign_InvoluteGear',_CommandInvoluteGear())

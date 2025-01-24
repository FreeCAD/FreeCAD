#***************************************************************************
#*   Copyright (c) 2020 Adam Spontarelli <adam@vector-space.org>           *
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

import FreeCAD, Part
from fcsprocket import fcsprocket
from fcsprocket import sprocket

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from FreeCADGui import PySideUic as uic

__title__="PartDesign SprocketObject management"
__author__ = "Adam Spontarelli"
__url__ = "https://www.freecad.org"


def makeSprocket(name):
    """
    makeSprocket(name): makes a Sprocket
    """
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",name)
    Sprocket(obj)
    if FreeCAD.GuiUp:
        ViewProviderSprocket(obj.ViewObject)
    #FreeCAD.ActiveDocument.recompute()
    if FreeCAD.GuiUp:
        body=FreeCADGui.ActiveDocument.ActiveView.getActiveObject("pdbody")
        part=FreeCADGui.ActiveDocument.ActiveView.getActiveObject("part")
        if body:
            body.Group=body.Group+[obj]
        elif part:
            part.Group=part.Group+[obj]
    return obj

class CommandSprocket:

    """
    the Fem Sprocket command definition
    """

    def GetResources(self):
        return {'Pixmap'  : 'PartDesign_Sprocket',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PartDesign_Sprocket","Sprocket..."),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PartDesign_Sprocket","Creates or edit the sprocket definition.")}

    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction("Create Sprocket")
        FreeCADGui.addModule("SprocketFeature")
        FreeCADGui.doCommand("SprocketFeature.makeSprocket('Sprocket')")
        FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name,0)")

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False


class Sprocket:
    """
    The Sprocket object
    """

    """
        ANSI B29.1-2011 standard roller chain sizes in USCS units (inches)
        {size: [Pitch, Roller Diameter]}
        """
    SprocketReferenceRollerTable = {
        0: [0.250, 0.130, 0.110, "ANSI 25"],
        1: [0.375, 0.200, 0.168, "ANSI 35"],
        2: [0.500, 0.306, 0.227, "ANSI 41"],
        3: [0.500, 0.312, 0.284, "ANSI 40"],
        4: [0.625, 0.400, 0.343, "ANSI 50"],
        5: [0.750, 0.469, 0.459, "ANSI 60"],
        6: [1.000, 0.625, 0.575, "ANSI 80"],
        7: [1.250, 0.750, 0.692, "ANSI 100"],
        8: [1.500, 0.875, 0.924, "ANSI 120"],
        9: [1.750, 1.000, 0.924, "ANSI 140"],
        10: [2.000, 1.125, 1.156, "ANSI 160"],
        11: [2.250, 1.460, 1.301, "ANSI 180"],
        12: [2.500, 1.562, 1.389, "ANSI 200"],
        13: [3.000, 1.875, 1.738, "ANSI 240"],
        14: [0.500, 0.3125, 0.11, "Bicycle with Derailleur"],
        15: [0.500, 0.3125, 0.084, "Bicycle without Derailleur"],
        16: [0.375, 5.72 / 25.4, 5.2 / 25.4, "ISO 606 06B"],
        17: [0.500, 7.75 / 25.4, 7.0 / 25.4, "ISO 606 08B"],
        18: [0.625, 9.65 / 25.4, 9.1 / 25.4, "ISO 606 10B"],
        19: [0.750, 11.68 / 25.4, 11.1 / 25.4, "ISO 606 12B"],
        20: [1.000, 17.02 / 25.4, 16.2 / 25.4, "ISO 606 16B"],
        21: [1.250, 19.56 / 25.4, 18.5 / 25.4, "ISO 606 20B"],
        22: [1.500, 25.4 / 25.4, 24.1 / 25.4, "ISO 606 24B"],
        23: [0.500, 0.3125, 0.227, "Motorcycle 420"],
        24: [0.500, 0.3125, 0.284, "Motorcycle 425"],
        25: [0.500, 0.335, 0.284, "Motorcycle 428"],
        26: [0.625, 0.400, 0.227, "Motorcycle 520"],
        27: [0.625, 0.400, 0.284, "Motorcycle 525"],
        28: [0.625, 0.400, 0.343, "Motorcycle 530"],
        29: [0.750, 0.400, 0.343, "Motorcycle 630"],
    }

    def __init__(self, obj):
        self.Type = "Sprocket"
        self.indx = 0
        self.sprockRef = []
        # As the UI file combobox has been translated the reference field needs populating here
        tempForm = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/PartDesign/SprocketFeature.ui"
        )
        for sKey in self.SprocketReferenceRollerTable:
            tempForm.comboBox_SprocketReference.setCurrentIndex(self.indx)
            self.sprockRef.append(tempForm.comboBox_SprocketReference.currentText())
            self.indx += 1
        self._ensure_properties(obj, is_restore=False)
        obj.addProperty(
            "App::PropertyEnumeration",
            "SprocketReference",
            "Sprocket",
            "Sprocket Reference",
        )
        obj.SprocketReference = list(self.sprockRef)
        obj.Proxy = self

    def onDocumentRestored(self, obj):
        """hook used to migrate older versions of this object"""
        self._ensure_properties(obj, is_restore=True)

    def _ensure_properties(self, obj, is_restore):
        def ensure_property(type_, name, doc, default):
            if not hasattr(obj, name):
                obj.addProperty(type_, name, "Sprocket")
                if callable(default):
                    setattr(obj, name, default())
                else:
                    setattr(obj, name, default)

        ensure_property(
            "App::PropertyInteger", "NumberOfTeeth", "Number of gear teeth", 50
        )
        ensure_property("App::PropertyLength", "Pitch", "Chain Pitch", "0.375 in")
        ensure_property(
            "App::PropertyLength", "RollerDiameter", "Roller Diameter", "0.20 in"
        )
        ensure_property(
            "App::PropertyLength",
            "Thickness",
            "Thickness as stated in the reference specification",
            "0.11 in",
        )

    def execute(self, obj):
        w = fcsprocket.FCWireBuilder()
        sprocket.CreateSprocket(
            w, obj.Pitch.Value, obj.NumberOfTeeth, obj.RollerDiameter.Value
        )

        sprocketw = Part.Wire([o.toShape() for o in w.wire])
        obj.Shape = sprocketw
        obj.positionBySupport()
        return


class ViewProviderSprocket:
    """
    A View Provider for the Sprocket object
    """

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/PartDesign_Sprocket.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def setEdit(self, vobj, mode):
        taskd = SprocketTaskPanel(self.Object, mode)
        taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
        return

    def dumps(self):
        return None

    def loads(self, state):
        return None


class SprocketTaskPanel:
    """
    The editmode TaskPanel for Sprocket objects
    """

    def __init__(self, obj, mode):
        self.obj = obj

        self.form = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/PartDesign/SprocketFeature.ui"
        )
        self.form.setWindowIcon(QtGui.QIcon(":/icons/PartDesign_Sprocket.svg"))

        QtCore.QObject.connect(
            self.form.Quantity_Pitch,
            QtCore.SIGNAL("valueChanged(double)"),
            self.pitchChanged,
        )
        QtCore.QObject.connect(
            self.form.Quantity_RollerDiameter,
            QtCore.SIGNAL("valueChanged(double)"),
            self.rollerDiameterChanged,
        )
        QtCore.QObject.connect(
            self.form.spinBox_NumberOfTeeth,
            QtCore.SIGNAL("valueChanged(int)"),
            self.numTeethChanged,
        )
        QtCore.QObject.connect(
            self.form.comboBox_SprocketReference,
            QtCore.SIGNAL("currentIndexChanged(int)"),
            self.sprocketReferenceChanged,
        )
        QtCore.QObject.connect(
            self.form.Quantity_Thickness,
            QtCore.SIGNAL("valueChanged(double)"),
            self.thicknessChanged,
        )

        self.update()

        if mode == 0:  # fresh created
            self.obj.Proxy.execute(self.obj)  # calculate once
            FreeCAD.Gui.SendMsgToActiveView("ViewFit")

    def transferTo(self):
        """
        Transfer from the dialog to the object
        """
        self.obj.NumberOfTeeth = self.form.spinBox_NumberOfTeeth.value()
        self.obj.Pitch = self.form.Quantity_Pitch.text()
        self.obj.RollerDiameter = self.form.Quantity_RollerDiameter.text()
        self.obj.SprocketReference = self.form.comboBox_SprocketReference.currentIndex()
        self.obj.Thickness = self.form.Quantity_Thickness.text()

    def transferFrom(self):
        """
        Transfer from the object to the dialog
        """
        self.form.spinBox_NumberOfTeeth.setValue(self.obj.NumberOfTeeth)
        self.form.Quantity_Pitch.setText(self.obj.Pitch.UserString)
        self.form.Quantity_RollerDiameter.setText(self.obj.RollerDiameter.UserString)
        self.form.comboBox_SprocketReference.setCurrentText(self.obj.SprocketReference)
        self.form.Quantity_Thickness.setText(self.obj.Thickness.UserString)

    def pitchChanged(self, value):
        self.obj.Pitch = value
        self.obj.Proxy.execute(self.obj)
        FreeCAD.Gui.SendMsgToActiveView("ViewFit")

    def sprocketReferenceChanged(self, size):
        self.obj.Pitch = str(Sprocket.SprocketReferenceRollerTable[size][0]) + " in"
        self.obj.RollerDiameter = (
            str(Sprocket.SprocketReferenceRollerTable[size][1]) + " in"
        )
        self.obj.Thickness = str(Sprocket.SprocketReferenceRollerTable[size][2]) + " in"
        self.obj.SprocketReference = self.obj.getEnumerationsOfProperty(
            "SprocketReference"
        )[size]
        self.form.Quantity_Pitch.setText(self.obj.Pitch.UserString)
        self.form.Quantity_RollerDiameter.setText(self.obj.RollerDiameter.UserString)
        self.form.Quantity_Thickness.setText(self.obj.Thickness.UserString)
        self.obj.Proxy.execute(self.obj)
        FreeCAD.Gui.SendMsgToActiveView("ViewFit")

    def rollerDiameterChanged(self, value):
        self.obj.RollerDiameter = value
        self.obj.Proxy.execute(self.obj)

    def numTeethChanged(self, value):
        self.obj.NumberOfTeeth = value
        self.obj.Proxy.execute(self.obj)
        FreeCAD.Gui.SendMsgToActiveView("ViewFit")

    def thicknessChanged(self, value):
        self.obj.Thickness = str(value)
        self.obj.Proxy.execute(self.obj)

    def getStandardButtons(self):
        return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel | QtGui.QDialogButtonBox.Apply

    def clicked(self, button):
        if button == QtGui.QDialogButtonBox.Apply:
            self.transferTo()
            self.obj.Proxy.execute(self.obj)

    def update(self):
        self.transferFrom()

    def accept(self):
        self.transferTo()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()

    def reject(self):
        FreeCAD.ActiveDocument.removeObject(self.obj.Name)
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.abortTransaction()

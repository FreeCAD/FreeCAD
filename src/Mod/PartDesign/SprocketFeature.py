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
__url__ = "http://www.freecad.org"


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
    SprocketReferenceRollerTable = {"ANSI 25": [0.250, 0.130, 0.110],
                           "ANSI 35": [0.375, 0.200, 0.168],
                           "ANSI 41": [0.500, 0.306, 0.227],
                           "ANSI 40": [0.500, 0.312, 0.284],
                           "ANSI 50": [0.625, 0.400, 0.343],
                           "ANSI 60": [0.750, 0.469, 0.459],
                           "ANSI 80": [1.000, 0.625, 0.575],
                           "ANSI 100":[1.250, 0.750, 0.692],
                           "ANSI 120":[1.500, 0.875, 0.924],
                           "ANSI 140":[1.750, 1.000, 0.924],
                           "ANSI 160":[2.000, 1.125, 1.156],
                           "ANSI 180":[2.250, 1.460, 1.301],
                           "ANSI 200":[2.500, 1.562, 1.389],
                           "ANSI 240":[3.000, 1.875, 1.738],
                           "Bicycle with Derailleur":[0.500, 0.3125, 0.11],
                           "Bicycle without Derailleur":[0.500, 0.3125, 0.084],
                           "ISO 606 06B":[0.375, 5.72/25.4, 5.2/25.4],
                           "ISO 606 08B":[0.500, 7.75/25.4, 7.0/25.4],
                           "ISO 606 10B":[0.625, 9.65/25.4, 9.1/25.4],
                           "ISO 606 12B":[0.750, 11.68/25.4, 11.1/25.4],
                           "ISO 606 16B":[1.000, 17.02/25.4, 16.2/25.4],
                           "ISO 606 20B":[1.250, 19.56/25.4, 18.5/25.4],
                           "ISO 606 24B":[1.500, 25.4/25.4, 24.1/25.4],
                           "Motorcycle 420":[0.500, 0.3125, 0.227],
                           "Motorcycle 425":[0.500, 0.3125, 0.284],
                           "Motorcycle 428":[0.500, 0.335, 0.284],
                           "Motorcycle 520":[0.625, 0.400, 0.227],
                           "Motorcycle 525":[0.625, 0.400, 0.284],
                           "Motorcycle 530":[0.625, 0.400, 0.343],
                           "Motorcycle 630":[0.750, 0.400, 0.343]}

    def __init__(self,obj):
        self.Type = "Sprocket"
        obj.addProperty("App::PropertyInteger","NumberOfTeeth","Sprocket","Number of gear teeth")
        obj.addProperty("App::PropertyLength","Pitch","Sprocket","Chain Pitch")
        obj.addProperty("App::PropertyLength","RollerDiameter","Sprocket","Roller Diameter")
        obj.addProperty("App::PropertyEnumeration","SprocketReference","Sprocket","Sprocket Reference")
        obj.addProperty("App::PropertyLength","Thickness","Sprocket","Thickness as stated in the reference specification")

        obj.SprocketReference = list(self.SprocketReferenceRollerTable)

        obj.NumberOfTeeth = 50
        obj.Pitch = "0.375 in"
        obj.RollerDiameter = "0.20 in"
        obj.SprocketReference = "ANSI 35"
        obj.Thickness = "0.11 in"

        obj.Proxy = self


    def execute(self,obj):
        w = fcsprocket.FCWireBuilder()
        sprocket.CreateSprocket(w, obj.Pitch.Value, obj.NumberOfTeeth, obj.RollerDiameter.Value)

        sprocketw = Part.Wire([o.toShape() for o in w.wire])
        obj.Shape = sprocketw
        obj.positionBySupport();
        return


class ViewProviderSprocket:
    """
    A View Provider for the Sprocket object
    """

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/PartDesign_Sprocket.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def setEdit(self,vobj,mode):
        taskd = SprocketTaskPanel(self.Object,mode)
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


class SprocketTaskPanel:
    """
    The editmode TaskPanel for Sprocket objects
    """

    def __init__(self,obj,mode):
        self.obj = obj

        self.form=FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/PartDesign/SprocketFeature.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/PartDesign_Sprocket.svg"))

        QtCore.QObject.connect(self.form.Quantity_Pitch, QtCore.SIGNAL("valueChanged(double)"), self.pitchChanged)
        QtCore.QObject.connect(self.form.Quantity_RollerDiameter, QtCore.SIGNAL("valueChanged(double)"), self.rollerDiameterChanged)
        QtCore.QObject.connect(self.form.spinBox_NumberOfTeeth, QtCore.SIGNAL("valueChanged(int)"), self.numTeethChanged)
        QtCore.QObject.connect(self.form.comboBox_SprocketReference, QtCore.SIGNAL("currentTextChanged(const QString)"), self.sprocketReferenceChanged)
        QtCore.QObject.connect(self.form.Quantity_Thickness, QtCore.SIGNAL("valueChanged(double)"), self.thicknessChanged)

        self.update()

        if mode == 0: # fresh created
            self.obj.Proxy.execute(self.obj)  # calculate once
            FreeCAD.Gui.SendMsgToActiveView("ViewFit")

    def transferTo(self):
        """
        Transfer from the dialog to the object
        """
        self.obj.NumberOfTeeth = self.form.spinBox_NumberOfTeeth.value()
        self.obj.Pitch = self.form.Quantity_Pitch.text()
        self.obj.RollerDiameter = self.form.Quantity_RollerDiameter.text()
        self.obj.SprocketReference = self.form.comboBox_SprocketReference.currentText()
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
        self.obj.Pitch          = str(Sprocket.SprocketReferenceRollerTable[size][0]) + " in"
        self.obj.RollerDiameter = str(Sprocket.SprocketReferenceRollerTable[size][1]) + " in"
        self.obj.Thickness      = str(Sprocket.SprocketReferenceRollerTable[size][2]) + " in"
        self.obj.SprocketReference = str(size)
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
        return int(QtGui.QDialogButtonBox.Ok) | int(QtGui.QDialogButtonBox.Cancel)| int(QtGui.QDialogButtonBox.Apply)

    def clicked(self,button):
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
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.abortTransaction()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('PartDesign_Sprocket', CommandSprocket())

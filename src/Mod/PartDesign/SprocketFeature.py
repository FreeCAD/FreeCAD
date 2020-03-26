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

import FreeCAD, Part
from fcsprocket import fcsprocket
from fcsprocket import sprocket

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from FreeCADGui import PySideUic as uic

__title__="PartDesign SprocketObject management"
__author__ = "Adam Spontarelli"
__url__ = "http://www.freecadweb.org"
          

def makeSprocket(name):
    '''makeSprocket(name): makes a Sprocket'''
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",name)
    _Sprocket(obj)
    if FreeCAD.GuiUp:
        _ViewProviderSprocket(obj.ViewObject)
    #FreeCAD.ActiveDocument.recompute()
    if FreeCAD.GuiUp:
        body=FreeCADGui.ActiveDocument.ActiveView.getActiveObject("pdbody")
        part=FreeCADGui.ActiveDocument.ActiveView.getActiveObject("part")
        if body:
            body.Group=body.Group+[obj]
        elif part:
            part.Group=part.Group+[obj]
    return obj

class _CommandSprocket:
    "the Fem Sprocket command definition"
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

       
class _Sprocket:
    "The Sprocket object"
    def __init__(self,obj):
        self.Type = "Sprocket"
        obj.addProperty("App::PropertyInteger","NumberOfTeeth","Gear","Number of gear teeth")
        obj.addProperty("App::PropertyLength","Pitch","Gear","Chain Pitch")
        obj.addProperty("App::PropertyLength","RollerDiameter","Gear","Roller Diameter")

        obj.NumberOfTeeth = 50
        obj.Pitch = "0.375 in" 
        obj.RollerDiameter = ".2 in"

        obj.Proxy = self
        
        
    def execute(self,obj):
        #print "_Sprocket.execute()"
        w = fcsprocket.FCWireBuilder()
        sprocket.CreateSprocket(w, obj.Pitch.Value, obj.NumberOfTeeth, obj.RollerDiameter.Value)

        gearw = Part.Wire([o.toShape() for o in w.wire])
        obj.Shape = gearw
        obj.positionBySupport();
        return
        
        
class _ViewProviderSprocket:
    "A View Provider for the Sprocket object"

    def __init__(self,vobj):
        vobj.Proxy = self
       
    def getIcon(self):
        return ":/icons/PartDesign_InternalExternalGear.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

  
    def setEdit(self,vobj,mode):
        taskd = _SprocketTaskPanel(self.Object,mode)
        taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True
    
    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class _SprocketTaskPanel:
    '''The editmode TaskPanel for Sprocket objects'''
    def __init__(self,obj,mode):
        self.obj = obj
        
        self.form=FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/PartDesign/SprocketFeature.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/PartDesign_InternalExternalGear.svg"))
        
        QtCore.QObject.connect(self.form.Quantity_Pitch, QtCore.SIGNAL("valueChanged(double)"), self.pitchChanged)
        QtCore.QObject.connect(self.form.Quantity_RollerDiameter, QtCore.SIGNAL("valueChanged(double)"), self.rollerDiameterChanged)
        QtCore.QObject.connect(self.form.spinBox_NumberOfTeeth, QtCore.SIGNAL("valueChanged(int)"), self.numTeethChanged)
        
        self.update()
        
        if mode == 0: # fresh created
            self.obj.Proxy.execute(self.obj)  # calculate once 
            FreeCAD.Gui.SendMsgToActiveView("ViewFit")
        
    def transferTo(self):
        "Transfer from the dialog to the object" 
        self.obj.NumberOfTeeth  = self.form.spinBox_NumberOfTeeth.value()
        self.obj.Pitch        = self.form.Quantity_Pitch.text()
        self.obj.RollerDiameter  = self.form.Quantity_RollerDiameter.text()
        
    
    def transferFrom(self):
        "Transfer from the object to the dialog"
        self.form.spinBox_NumberOfTeeth.setValue(self.obj.NumberOfTeeth)
        self.form.Quantity_Pitch.setText(self.obj.Pitch.UserString)
        self.form.Quantity_RollerDiameter.setText(self.obj.RollerDiameter.UserString)
        
    def pitchChanged(self, value):
        #print value
        self.obj.Pitch = value
        self.obj.Proxy.execute(self.obj)
        FreeCAD.Gui.SendMsgToActiveView("ViewFit")
        
    def rollerDiameterChanged(self, value):
        #print value
        self.obj.RollerDiameter = value
        self.obj.Proxy.execute(self.obj)

    def numTeethChanged(self, value):
        #print value
        self.obj.NumberOfTeeth = value
        self.obj.Proxy.execute(self.obj)
        FreeCAD.Gui.SendMsgToActiveView("ViewFit")
        
        
    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok) | int(QtGui.QDialogButtonBox.Cancel)| int(QtGui.QDialogButtonBox.Apply)
    
    def clicked(self,button):
        if button == QtGui.QDialogButtonBox.Apply:
            #print "Apply"
            self.transferTo()
            self.obj.Proxy.execute(self.obj) 
        
    def update(self):
        'fills the widgets'
        self.transferFrom()
                
    def accept(self):
        #print 'accept(self)'
        self.transferTo()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        
                    
    def reject(self):
        #print 'reject(self)'
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.abortTransaction()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('PartDesign_Sprocket',_CommandSprocket())

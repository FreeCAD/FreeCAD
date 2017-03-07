#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *  
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
from fcgear import involute
from fcgear import fcgear

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from FreeCADGui import PySideUic as uic

__title__="PartDesign InvoluteGearObject management"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

          

def makeInvoluteGear(name):
    '''makeInvoluteGear(name): makes an InvoluteGear'''
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",name)
    _InvoluteGear(obj)
    if FreeCAD.GuiUp:
        _ViewProviderInvoluteGear(obj.ViewObject)
    #FreeCAD.ActiveDocument.recompute()
    return obj

class _CommandInvoluteGear:
    "the Fem InvoluteGear command definition"
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
        obj.addProperty("App::PropertyInteger","NumberOfTeeth","Gear","Number of gear teeth")
        obj.addProperty("App::PropertyLength","Modules","Gear","Modules of the gear")
        obj.addProperty("App::PropertyAngle","PressureAngle","Gear","Pressure angle of gear teeth")
        obj.addProperty("App::PropertyBool","HighPrecision","Gear","True=2 curves with each 3 control points False=1 curve with 4 control points ")
        obj.addProperty("App::PropertyBool","ExternalGear","Gear","True=external Gear False=internal Gear ")
        
        obj.NumberOfTeeth = 26
        obj.Modules = "2.5 mm" 
        obj.PressureAngle = "20 deg" 
        obj.HighPrecision = True 
        obj.ExternalGear = True 
        
        obj.Proxy = self
        
        
    def execute(self,obj):
        #print "_InvoluteGear.execute()"
        w = fcgear.FCWireBuilder()
        if obj.ExternalGear:
            involute.CreateExternalGear(w, obj.Modules.Value,obj.NumberOfTeeth, obj.PressureAngle.Value, obj.HighPrecision)
        else:
            involute.CreateInternalGear(w, obj.Modules.Value,obj.NumberOfTeeth, obj.PressureAngle.Value, obj.HighPrecision)
        gearw = Part.Wire([o.toShape() for o in w.wire])
        obj.Shape = gearw
        obj.positionBySupport();
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

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class _InvoluteGearTaskPanel:
    '''The editmode TaskPanel for InvoluteGear objects'''
    def __init__(self,obj,mode):
        self.obj = obj
        
        self.form=FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/PartDesign/InvoluteGearFeature.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/PartDesign_InternalExternalGear.svg"))
        
        QtCore.QObject.connect(self.form.Quantity_Modules, QtCore.SIGNAL("valueChanged(double)"), self.modulesChanged)
        QtCore.QObject.connect(self.form.Quantity_PressureAngle, QtCore.SIGNAL("valueChanged(double)"), self.angleChanged)
        QtCore.QObject.connect(self.form.spinBox_NumberOfTeeth, QtCore.SIGNAL("valueChanged(int)"), self.numTeethChanged)
        QtCore.QObject.connect(self.form.comboBox_HighPrecision, QtCore.SIGNAL("currentIndexChanged(int)"), self.numCurvesChanged)
        #QtCore.QObject.connect(self.form.comboBox_ExternalGear, QtCore.SIGNAL("activated(QString)"), self.externalGearChanged)
        #QtCore.QObject.connect(self.form.comboBox_ExternalGear, QtCore.SIGNAL("currentIndexChanged(int)"), self.externalGearChanged)
        QtCore.QObject.connect(self.form.comboBox_ExternalGear, QtCore.SIGNAL("currentIndexChanged(int)"), self.externalGearChanged)
        
        self.update()
        
        if mode == 0: # fresh created
            self.obj.Proxy.execute(self.obj)  # calculate once 
            FreeCAD.Gui.SendMsgToActiveView("ViewFit")
        
    def transferTo(self):
        "Transfer from the dialog to the object" 
        self.obj.NumberOfTeeth  = self.form.spinBox_NumberOfTeeth.value()
        self.obj.Modules        = self.form.Quantity_Modules.text()
        self.obj.PressureAngle  = self.form.Quantity_PressureAngle.text()
        if self.form.comboBox_HighPrecision.currentIndex() == 0:
            self.obj.HighPrecision = True
        else:
            self.obj.HighPrecision = False
        #self.obj.HighPrecision = self.form.comboBox_HighPrecision.currentIndex()
        if self.form.comboBox_ExternalGear.currentIndex() == 0:
            self.obj.ExternalGear = True
        else:
            self.obj.ExternalGear = False
        #self.obj.ExternalGear       = self.form.comboBox_ExternalGear.currentIndex()
        
    
    def transferFrom(self):
        "Transfer from the object to the dialog"
        self.form.spinBox_NumberOfTeeth.setValue(self.obj.NumberOfTeeth)
        self.form.Quantity_Modules.setText(self.obj.Modules.UserString)
        self.form.Quantity_PressureAngle.setText(self.obj.PressureAngle.UserString)
        if self.obj.HighPrecision:
            self.form.comboBox_HighPrecision.setCurrentIndex(0)
        else:
            self.form.comboBox_HighPrecision.setCurrentIndex(1)
        #self.form.comboBox_HighPrecision.setCurrentIndex(self.obj.HighPrecision)
        if self.obj.ExternalGear:
            self.form.comboBox_ExternalGear.setCurrentIndex(0)
        else:
            self.form.comboBox_ExternalGear.setCurrentIndex(1)
        #self.form.comboBox_ExternalGear.setCurrentIndex(self.obj.ExternalGear)
        
    def modulesChanged(self, value):
        #print value
        self.obj.Modules = value
        self.obj.Proxy.execute(self.obj)
        FreeCAD.Gui.SendMsgToActiveView("ViewFit")
        
    def angleChanged(self, value):
        #print value
        self.obj.PressureAngle = value
        self.obj.Proxy.execute(self.obj)

    def numTeethChanged(self, value):
        #print value
        self.obj.NumberOfTeeth = value
        self.obj.Proxy.execute(self.obj)
        FreeCAD.Gui.SendMsgToActiveView("ViewFit")
        
    def numCurvesChanged(self, value):
        #print value
        if value == 0:
            v=True
        else:
            v=False
        self.obj.HighPrecision = v
        self.obj.Proxy.execute(self.obj)
        
    def externalGearChanged(self, value):
        #print value
        if value == 0:
           v=True
        else:
           v=False
        self.obj.ExternalGear = v
        self.obj.Proxy.execute(self.obj)
        
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


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('PartDesign_InvoluteGear',_CommandInvoluteGear())

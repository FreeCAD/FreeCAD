# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD,Path
from PySide import QtCore,QtGui
from PathScripts import PathUtils,PathSelection,PathProject

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Drilling object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectDrilling:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSubList","Base","Path",translate("Parent Object(s)","The base geometry of this toolpath"))
        obj.addProperty("App::PropertyVectorList","locations","Path","The drilling locations")

        obj.addProperty("App::PropertyLength", "PeckDepth", "Depth", translate("PathProject","Incremental Drill depth before retracting to clear chips"))
        obj.addProperty("App::PropertyLength", "StartDepth", "Depth", translate("PathProject","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", translate("PathProject","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", translate("PathProject","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", translate("PathProject","Height to clear top of materil"))
        obj.addProperty("App::PropertyDistance", "RetractHeight", "Depth", translate("Retract Height","The height where feed starts and height during retract tool when path is finished"))
        #obj.addProperty("App::PropertyLength", "VertFeed", "Feed",translate("Vert Feed","Feed rate for vertical moves in Z"))
        obj.addProperty("App::PropertyString","Comment","Path",translate("PathProject","An optional comment for this profile"))
        obj.addProperty("App::PropertyBool","Active","Path",translate("Active","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyIntegerConstraint","ToolNumber","Tool",translate("PathProfile","The tool number in use"))
        obj.ToolNumber = (0,0,1000,1) 
        obj.setEditorMode('ToolNumber',1) #make this read only
        
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None
        
    def execute(self,obj):
        output = ""
        toolLoad = PathUtils.getLastToolLoad(obj)
        if toolLoad == None:
            self.vertFeed = 100
            self.horizFeed = 100
            radius = 0.25
            obj.ToolNumber= 0   
        else:
            self.vertFeed = toolLoad.VertFeed.Value
            self.horizFeed = toolLoad.HorizFeed.Value
            tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
            radius = tool.Diameter/2
            obj.ToolNumber= toolLoad.ToolNumber   
        if obj.Base:
            locations = []
            for loc in obj.Base:

                if "Face" in loc[1] or "Edge" in loc[1]:    
                    s = getattr(loc[0].Shape,loc[1])
                else:
                    s = loc[0].Shape

                if s.ShapeType in ['Face', 'Wire', 'Edge' ]:
                        X = s.Edges[0].Curve.Center.x
                        Y = s.Edges[0].Curve.Center.y
                        Z = s.Edges[0].Curve.Center.z
                elif s.ShapeType == 'Vertex':
                        X = s.Point.x
                        Y = s.Point.y
                        Z = s.Point.z

                locations.append(FreeCAD.Vector(X,Y,Z))

                output = "G90 G98\n"
                # rapid to clearance height
                output += "G0 Z" + str(obj.ClearanceHeight.Value)
                # rapid to first hole location, with spindle still retracted:
                p0 = locations[0]
                output += "G0 X"+str(p0.x) + " Y" + str(p0.y)+ "\n"
                # move tool to clearance plane
                output += "G0 Z" + str(obj.ClearanceHeight.Value) + "\n"
                if obj.PeckDepth.Value > 0:
                    cmd = "G83"
                    qword = " Q"+ str(obj.PeckDepth.Value)
                else:
                    cmd = "G81"
                    qword = ""
                for p in locations:

                    output += cmd + " X" + str(p.x) + " Y" + str(p.y) + " Z" + str(obj.FinalDepth.Value) + qword + " R" + str(obj.RetractHeight.Value) + " F" + str(self.vertFeed) + "\n"

                output += "G80\n"

        path = Path.Path(output)
        obj.Path = path


    def addDrillableLocation(self, obj, ss, sub=""):
        baselist = obj.Base
        item = (ss, sub)
        if item in baselist:
            FreeCAD.Console.PrintWarning("Drillable location already in the list"+ "\n")
        else:
            baselist.append (item)
        obj.Base = baselist
        self.execute(obj)

class _ViewProviderDrill:
    def __init__(self,obj): #mandatory
        obj.Proxy = self

    def __getstate__(self): #mandatory
        return None

    def __setstate__(self,state): #mandatory
        return None

    def getIcon(self): #optional
        return ":/icons/Path-Drilling.svg"

#    def attach(self): #optional
#        # this is executed on object creation and object load from file
#        pass

    def onChanged(self,obj,prop): #optional
        # this is executed when a property of the VIEW PROVIDER changes
        pass

    def updateData(self,obj,prop): #optional
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self,vobj,mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel()
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        return True


    def unsetEdit(self,vobj,mode): #optional
        # this is executed when the user cancels or terminates edit mode
        pass

class CommandPathDrilling:

    def GetResources(self):
        return {'Pixmap'  : 'Path-Drilling',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Drilling","Drilling"),
                'Accel': "P, D",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Drilling","Creates a Path Drilling object")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        import Path, Part


        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Drilling","Create Drilling"))
        FreeCADGui.addModule("PathScripts.PathDrilling")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Drilling")')
        FreeCADGui.doCommand('PathScripts.PathDrilling.ObjectDrilling(obj)')
        FreeCADGui.doCommand('obj.Active = True')
        FreeCADGui.doCommand('PathScripts.PathDrilling._ViewProviderDrill(obj.ViewObject)')

        ztop = 10.0
        zbottom = 0.0
        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(ztop))
        FreeCADGui.doCommand('obj.RetractHeight= ' + str(ztop))
        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')

class TaskPanel:
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/DrillingEdit.ui")
        #self.form = FreeCADGui.PySideUic.loadUi(":/DrillingEdit.ui")

    def accept(self):
        self.getFields()

        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s) 

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s) 

    def getFields(self):    
        if self.obj:
            if hasattr(self.obj,"StartDepth"):
                self.obj.StartDepth = self.form.startDepth.text()
            if hasattr(self.obj,"FinalDepth"):
                self.obj.FinalDepth = self.form.finalDepth.text()
            if hasattr(self.obj,"PeckDepth"):
                self.obj.PeckDepth = self.form.peckDepth.text()
            if hasattr(self.obj,"SafeHeight"):
                self.obj.SafeHeight = self.form.safeHeight.text()
            if hasattr(self.obj,"ClearanceHeight"):
                self.obj.ClearanceHeight = self.form.clearanceHeight.text()
            if hasattr(self.obj,"RetractHeight"):
                self.obj.RetractHeight = self.form.retractHeight.text()

        self.obj.Proxy.execute(self.obj)

    def open(self):
        self.s =SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)   

    def addBase(self):
         # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()

        if not len(selection) >= 1:
            FreeCAD.Console.PrintError(translate("PathProject","Please select at least one Drillable Location\n"))
            return
        for s in selection:
            if s.HasSubObjects:
                for i in s.SubElementNames:
                    self.obj.Proxy.addDrillableLocation(self.obj, s.Object, i)
            else:      
                self.obj.Proxy.addDrillableLocation(self.obj, s.Object)

        self.form.baseList.clear()
        for i in self.obj.Base:         
            self.form.baseList.addItem(i[0].Name + "." + i[1])

         
    def deleteBase(self):
        dlist = self.form.baseList.selectedItems()
        for d in dlist:
            newlist = []
            for i in self.obj.Base:
                if not i[0].Name == d.text().partition(".")[0]:
                    newlist.append (i)
            self.obj.Base = newlist
        self.form.baseList.takeItem(self.form.baseList.row(d))
        # self.obj.Proxy.execute(self.obj)
        # FreeCAD.ActiveDocument.recompute()

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        slist = self.form.baseList.selectedItems()
        for i in slist:
            objstring = i.text().partition(".")
            obj = FreeCAD.ActiveDocument.getObject(objstring[0])
          #  sub = o.Shape.getElement(objstring[2])
            if objstring[2] != "": 
                FreeCADGui.Selection.addSelection(obj,objstring[2])
            else:
                FreeCADGui.Selection.addSelection(obj)

        FreeCADGui.updateGui()

    def reorderBase(self):
        newlist = []
        for i in range(self.form.baseList.count()):
            s = self.form.baseList.item(i).text()
            objstring = s.partition(".")

            obj = FreeCAD.ActiveDocument.getObject(objstring[0])
            item = (obj, str(objstring[2]))
            newlist.append(item)
        self.obj.Base=newlist

        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def setupUi(self):
        self.form.startDepth.setText(str(self.obj.StartDepth.Value))
        self.form.finalDepth.setText(str(self.obj.FinalDepth.Value))
        self.form.peckDepth.setText(str(self.obj.PeckDepth.Value))
        self.form.safeHeight.setText(str(self.obj.SafeHeight.Value))
        self.form.clearanceHeight.setText(str(self.obj.ClearanceHeight.Value))
        self.form.retractHeight.setText(str(self.obj.RetractHeight.Value))


        for i in self.obj.Base:         
            self.form.baseList.addItem(i[0].Name + "." + i[1])

        #Connect Signals and Slots
        self.form.startDepth.editingFinished.connect(self.getFields) #This is newer syntax
        #QtCore.QObject.connect(self.form.startDepth, QtCore.SIGNAL("editingFinished()"), self.getFields)
        QtCore.QObject.connect(self.form.finalDepth, QtCore.SIGNAL("editingFinished()"), self.getFields)
        QtCore.QObject.connect(self.form.safeHeight, QtCore.SIGNAL("editingFinished()"), self.getFields)
        QtCore.QObject.connect(self.form.clearanceHeight, QtCore.SIGNAL("editingFinished()"), self.getFields)

        QtCore.QObject.connect(self.form.addBase, QtCore.SIGNAL("clicked()"), self.addBase)
        QtCore.QObject.connect(self.form.deleteBase, QtCore.SIGNAL("clicked()"), self.deleteBase)
        QtCore.QObject.connect(self.form.reorderBase, QtCore.SIGNAL("clicked()"), self.reorderBase)

        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)

class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST 
        PST.drillselect()

    def __del__(self):
        import PathScripts.PathSelection as PST 
        PST.clear()

    def addSelection(self,doc,obj,sub,pnt):               # Selection object
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj +')')
        FreeCADGui.updateGui()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Drilling',CommandPathDrilling())

FreeCAD.Console.PrintLog("Loading PathDrilling... done\n")

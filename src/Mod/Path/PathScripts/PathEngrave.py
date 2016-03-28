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

import FreeCAD,FreeCADGui,Path,PathGui,Draft

from PySide import QtCore,QtGui

"""Path Engrave object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectPathEngrave:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSubList","Base","Path","The base geometry of this object")

        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm",translate("Path", "The library or Algorithm used to generate the path"))
        obj.Algorithm = ['OCC Native']

        #Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", translate("Path","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", translate("Path","Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", translate("Path","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", translate("Path","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyInteger","StartVertex","Path","The vertex index to start the path from")
        #Feed Properties
        obj.addProperty("App::PropertySpeed", "VertFeed", "Feed",translate("Path","Feed rate for vertical moves in Z"))
        obj.addProperty("App::PropertySpeed", "HorizFeed", "Feed",translate("Path","Feed rate for horizontal moves"))

        if FreeCAD.GuiUp:
            _ViewProviderEngrave(obj.ViewObject)

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None
        
    def execute(self,obj):
        output = ""
        if obj.Base:
            for o in obj.Base:
                output += "G0 " + str(obj.ClearanceHeight.Value)+"\n"
                # we only consider the outer wire if this is a Face
                wires = o[0].Shape.Wires

                if obj.Algorithm == "OCC Native":
                    output += self.buildpathocc(obj, wires)

        #print output
        if output == "":
            #output += "G0 Z" + str(obj.ClearanceHeight.Value)
            output +="G0"
        path = Path.Path(output)
        obj.Path = path

    def buildpathocc(self, obj, wires):
        import Part,DraftGeomUtils
        output = "G90\nG21\nG40\n"
        output += "G0 Z" + str(obj.ClearanceHeight.Value)

        # absolute coords, millimeters, cancel offsets

        for wire in wires:
            offset = wire
        
            # reorder the wire
            offset = DraftGeomUtils.rebaseWire(offset,obj.StartVertex)
            
            # we create the path from the offset shape
            last = None
            for edge in offset.Edges:
                if not last:
                    # we set the first move to our first point
                    last = edge.Vertexes[0].Point
                    output += "G0" + " X" + str("%f" % last.x) + " Y" + str("%f" % last.y) #Rapid sto starting position
                    output += "G1" + " Z" + str("%f" % last.z) +"F " + str(obj.VertFeed.Value)+ "\n" #Vertical feed to depth
                if isinstance(edge.Curve,Part.Circle):
                    point = edge.Vertexes[-1].Point
                    if point == last: # edges can come flipped
                        point = edge.Vertexes[0].Point
                    center = edge.Curve.Center
                    relcenter = center.sub(last)
                    v1 = last.sub(center)
                    v2 = point.sub(center)
                    if v1.cross(v2).z < 0:
                        output += "G2"
                    else:
                        output += "G3"
                    output += " X" + str("%f" % point.x) + " Y" + str("%f" % point.y) + " Z" + str("%f" % point.z)
                    output += " I" + str("%f" % relcenter.x) + " J" + str("%f" % relcenter.y) + " K" + str("%f" % relcenter.z)
                    output +=  " F " + str(obj.HorizFeed.Value)
                    output += "\n"
                    last = point
                else:
                    point = edge.Vertexes[-1].Point
                    if point == last: # edges can come flipped
                        point = edge.Vertexes[0].Point
                    output += "G1 X" + str("%f" % point.x) + " Y" + str("%f" % point.y) + " Z" + str("%f" % point.z)
                    output +=  " F " + str(obj.HorizFeed.Value)
                    output +=  "\n"
                    last = point
            output += "G0 Z " + str(obj.SafeHeight.Value)
        return output

    def addShapeString(self, obj, ss):
        baselist = obj.Base
        item = (ss, "")
        if item in baselist:
            FreeCAD.Console.PrintWarning("ShapeString already in the Engraving list"+ "\n")

        else:
            baselist.append (item)
        obj.Base = baselist
        self.execute(obj)


class _ViewProviderEngrave:

    def __init__(self,vobj):
        vobj.Proxy = self

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def setEdit(self,vobj,mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel()
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        return True

    def getIcon(self):
        return ":/icons/Path-Profile.svg"

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

class CommandPathEngrave:

    def GetResources(self):
        return {'Pixmap'  : 'Path-Engrave',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Engrave","ShapeString Engrave"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Engrave","Creates an Engraving Path around a Draft ShapeString")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        
        # if everything is ok, execute and register the transaction in the undo/redo stack

        FreeCAD.ActiveDocument.openTransaction("Create Engrave Path")
        FreeCADGui.addModule("PathScripts.PathFaceProfile")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","PathEngrave")')
        FreeCADGui.doCommand('PathScripts.PathEngrave.ObjectPathEngrave(obj)')

        FreeCADGui.doCommand('obj.ClearanceHeight = 10')
        FreeCADGui.doCommand('obj.StartDepth= 0')
        FreeCADGui.doCommand('obj.FinalDepth= -0.1' )
        FreeCADGui.doCommand('obj.SafeHeight= 5.0' )

        #FreeCADGui.doCommand('obj.ViewObject.Proxy = 0')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')

class TaskPanel:
    def __init__(self):
        #self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/EngraveEdit.ui")
        self.form = FreeCADGui.PySideUic.loadUi(":/EngraveEdit.ui")

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
            if hasattr(self.obj,"SafeHeight"):
                self.obj.SafeHeight = self.form.safeHeight.text()
            if hasattr(self.obj,"ClearanceHeight"):
                self.obj.ClearanceHeight = self.form.clearanceHeight.text()
            if hasattr(self.obj,"VertFeed"):
                self.obj.VertFeed= self.form.vertFeed.text()
            if hasattr(self.obj,"HorizFeed"):
                self.obj.HorizFeed = self.form.horizFeed.text()

        self.obj.Proxy.execute(self.obj)

    def open(self):
        self.s =SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)   

    def addBase(self):
         # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()

        if not len(selection) >= 1:
            FreeCAD.Console.PrintError(translate("Path_Engrave","Please select at least one ShapeString\n"))
            return
        for s in selection:
            if not Draft.getType(s.Object) == "ShapeString":
                FreeCAD.Console.PrintError(translate("Path_Engrave","Please select at least one ShapeString\n"))
                return
            self.obj.Proxy.addShapeString(self.obj, s.Object)

        self.form.baseList.clear()
        for i in self.obj.Base:         
            self.form.baseList.addItem(i[0].Name)

    def deleteBase(self):
        dlist = self.form.baseList.selectedItems()
        for d in dlist:
            newlist = []
            for i in self.obj.Base:
                if not i[0].Name == d.text():
                    newlist.append (i)
            self.obj.Base = newlist
        self.form.baseList.takeItem(self.form.baseList.row(d))
        # self.obj.Proxy.execute(self.obj)
        # FreeCAD.ActiveDocument.recompute()

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        slist = self.form.baseList.selectedItems()
        for i in slist:
            o = FreeCAD.ActiveDocument.getObject(i.text())
            FreeCADGui.Selection.addSelection(o)
        FreeCADGui.updateGui()

    def reorderBase(self):
        newlist = []
        for i in range(self.form.baseList.count()):
	    s = self.form.baseList.item(i).text()
            obj = FreeCAD.ActiveDocument.getObject(s)
            newlist.append(obj)
        self.obj.Base=newlist
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def setupUi(self):
        self.form.startDepth.setText(str(self.obj.StartDepth.Value))
        self.form.finalDepth.setText(str(self.obj.FinalDepth.Value))
        self.form.safeHeight.setText(str(self.obj.SafeHeight.Value))
        self.form.clearanceHeight.setText(str(self.obj.ClearanceHeight.Value))
        self.form.vertFeed.setText(str(self.obj.VertFeed.Value))
        self.form.horizFeed.setText(str(self.obj.HorizFeed.Value))

        for i in self.obj.Base:         
            self.form.baseList.addItem(i[0].Name)

        #Connect Signals and Slots
        self.form.startDepth.editingFinished.connect(self.getFields) #This is newer syntax
        #QtCore.QObject.connect(self.form.startDepth, QtCore.SIGNAL("editingFinished()"), self.getFields)
        QtCore.QObject.connect(self.form.finalDepth, QtCore.SIGNAL("editingFinished()"), self.getFields)
        QtCore.QObject.connect(self.form.horizFeed, QtCore.SIGNAL("editingFinished()"), self.getFields)
        QtCore.QObject.connect(self.form.vertFeed, QtCore.SIGNAL("editingFinished()"), self.getFields)
        QtCore.QObject.connect(self.form.safeHeight, QtCore.SIGNAL("editingFinished()"), self.getFields)
        QtCore.QObject.connect(self.form.clearanceHeight, QtCore.SIGNAL("editingFinished()"), self.getFields)

        QtCore.QObject.connect(self.form.addBase, QtCore.SIGNAL("clicked()"), self.addBase)
        QtCore.QObject.connect(self.form.deleteBase, QtCore.SIGNAL("clicked()"), self.deleteBase)
        QtCore.QObject.connect(self.form.reorderBase, QtCore.SIGNAL("clicked()"), self.reorderBase)

        QtCore.QObject.connect(self.form.baseList, QtCore.SIGNAL("itemSelectionChanged()"), self.itemActivated)



class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST 
        PST.engraveselect()

    def __del__(self):
        import PathScripts.PathSelection as PST 
        PST.clear()

    def addSelection(self,doc,obj,sub,pnt):               # Selection object
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj +')')
        FreeCADGui.updateGui()

if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Engrave',CommandPathEngrave())

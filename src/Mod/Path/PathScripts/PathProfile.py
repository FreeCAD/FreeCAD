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
from FreeCAD import Vector
from PathScripts import PathUtils #,PathSelection,PathProject
from PathScripts.PathUtils import depth_params

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    #from pivy import coin
else:
    def translate(ctxt,txt):
        return txt

__title__="Path Profile Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path Profile object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

class ObjectProfile:

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSubList","Base","Path",translate("Parent Object","The base geometry of this toolpath"))
        obj.addProperty("App::PropertyBool","Active","Path",translate("Path","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString","Comment","Path",translate("Path","An optional comment for this profile"))

        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm",translate("Path", "The library or algorithm used to generate the path"))
        obj.Algorithm = ['OCC Native','libareal']

        obj.addProperty("App::PropertyIntegerConstraint","ToolNumber","Tool",translate("Path","The tool number in use"))
        obj.ToolNumber = (0,0,1000,1) 
        obj.setEditorMode('ToolNumber',1) #make this read only

        #Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", translate("Path","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", translate("Path","Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyFloatConstraint", "StepDown", "Depth", translate("Path","Incremental Step Down of Tool"))
        obj.StepDown = (1,0.01,1000,0.5)
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", translate("Path","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", translate("Path","Final Depth of Tool- lowest value in Z"))
      
        #Start Point Properties
        obj.addProperty("App::PropertyVector","StartPoint","Start Point",translate("Path_Profile","The start point of this path"))
        obj.addProperty("App::PropertyBool","UseStartPoint","Start Point",translate("Path_Profile","make True, if specifying a Start Point"))
        obj.addProperty("App::PropertyLength", "ExtendAtStart", "Start Point", translate("Path_Profile", "extra length of tool path before start of part edge"))
        obj.addProperty("App::PropertyLength", "LeadInLineLen", "Start Point", translate("Path_Profile","length of straight segment of toolpath that comes in at angle to first part edge"))

        #End Point Properties
        obj.addProperty("App::PropertyBool","UseEndPoint","End Point",translate("Path_Profile","make True, if specifying an End Point"))
        obj.addProperty("App::PropertyLength", "ExtendAtEnd", "End Point", translate("Path_Profile","extra length of tool path after end of part edge"))
        obj.addProperty("App::PropertyLength", "LeadOutLineLen", "End Point", translate("Path_Profile","length of straight segment of toolpath that comes in at angle to last part edge"))
        obj.addProperty("App::PropertyVector","EndPoint","End Point",translate("Path_Profile","The end point of this path"))

        #Profile Properties
        obj.addProperty("App::PropertyEnumeration", "Side", "Profile", translate("Path_Profile","Side of edge that tool should cut"))
        obj.Side = ['Left','Right','On'] #side of profile that cutter is on in relation to direction of profile
        obj.addProperty("App::PropertyEnumeration", "Direction", "Profile",translate("Path_Profile", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.Direction = ['CW','CCW'] #this is the direction that the profile runs
        obj.addProperty("App::PropertyBool","UseComp","Profile",translate("Path_Profile","make True, if using Cutter Radius Compensation"))

        obj.addProperty("App::PropertyDistance", "RollRadius", "Profile", translate("Path_Profile","Radius at start and end"))
        obj.addProperty("App::PropertyDistance", "OffsetExtra", "Profile",translate("Path_Profile","Extra value to stay away from final profile- good for roughing toolpath"))
        obj.addProperty("App::PropertyLength", "SegLen", "Profile",translate("Path_Profile","Tesselation  value for tool paths made from beziers, bsplines, and ellipses"))

        obj.addProperty("App::PropertyVectorList", "locs", "Tags", translate("Path_Profile", "List of holding tag locations"))

        obj.addProperty("App::PropertyFloatList","angles","Tags", translate("Path_Profile", "List of angles for the holding tags"))
        obj.addProperty("App::PropertyFloatList","heights","Tags", translate("Path_Profile", "List of angles for the holding tags"))
        obj.addProperty("App::PropertyFloatList","lengths","Tags", translate("Path_Profile", "List of angles for the holding tags"))
        locations = []
        angles = []
        lengths = []
        heights = []

        obj.locs = locations
        obj.angles = angles
        obj.lengths = lengths
        obj.heights = heights

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def addprofilebase(self, obj, ss, sub=""):
        baselist = obj.Base
        item = (ss, sub)
        if item in baselist:
            FreeCAD.Console.PrintWarning("this object already in the list"+ "\n")
        else:
            baselist.append (item)
        obj.Base = baselist
        self.execute(obj)

    def _buildPathOCC(self,obj,wire):
        import DraftGeomUtils
        output = ""
        output += '('+ str(obj.Comment)+')\n'

        if obj.Direction == 'CCW':
            clockwise=False
        else:
            clockwise=True

        FirstEdge= None
        PathClosed = DraftGeomUtils.isReallyClosed(wire)

        output += PathUtils.MakePath(wire, \
                obj.Side, \
                self.radius, \
                clockwise, \
                obj.ClearanceHeight.Value, \
                obj.StepDown, \
                obj.StartDepth.Value, \
                obj.FinalDepth.Value, \
                FirstEdge, \
                PathClosed, \
                obj.SegLen.Value, \
                self.vertFeed, \
                self.horizFeed)

        return output

    def _buildPathLibarea(self,obj, edgelist):
        import PathScripts.PathKurveUtils as PathKurveUtils
        import math, area
        output = ""

        if obj.StartPoint and obj.UseStartPoint:
            startpoint = obj.StartPoint                
        else:
            startpoint = None

        if obj.EndPoint and obj.UseEndPoint:
            endpoint = obj.EndPoint
        else:
            endpoint = None


        PathKurveUtils.output('mem')
        PathKurveUtils.feedrate_hv(self.horizFeed, self.vertFeed)

        output = ""
        output += "G0 Z" + str(obj.ClearanceHeight.Value)
        curve = PathKurveUtils.makeAreaCurve(edgelist,obj.Direction,startpoint, endpoint)

        '''The following line uses a profile function written for use with FreeCAD.  It's clean but incomplete.  It doesn't handle
print "x = " + str(point.x)
print "y - " + str(point.y)
            holding tags
            start location
            CRC
            or probably other features in heekscnc'''
        #output += PathKurveUtils.profile(curve, side, radius, vf, hf, offset_extra, rapid_safety_space, clearance, start_depth, step_down, final_depth, use_CRC)

        '''The following calls the original procedure from heekscnc profile function.  This, in turn, calls many other procedures to modify the profile.
            This procedure is hacked together from heekscnc and has not been thoroughly reviewed or understood for FreeCAD.  It can probably be 
            thoroughly optimized and improved but it'll take a smarter mind than mine to do it.  -sliptonic Feb16'''
        roll_radius = 2.0
        extend_at_start = 0.0
        extend_at_end = 0.0
        lead_in_line_len=0.0
        lead_out_line_len= 0.0
        
        '''

        Right here, I need to know the Holding Tags group from the tree that refers to this profile operation and build up the tags for PathKurve Utils.
        I need to access the location vector, length, angle in radians and height.

        '''
        PathKurveUtils.clear_tags()
        for i in range(len(obj.locs)):
            tag = obj.locs[i]
            h = obj.heights[i]
            l = obj.lengths[i]
            a = math.radians(obj.angles[i])
            PathKurveUtils.add_tag(area.Point(tag.x,tag.y), l, a, h)
            
        depthparams = depth_params (obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown, 0.0, obj.FinalDepth.Value, None)

        PathKurveUtils.profile2(curve, \
                obj.Side, \
                self.radius, \
                self.vertFeed, \
                self.horizFeed, \
                obj.OffsetExtra.Value,\
                roll_radius, \
                None,\
                None, \
                depthparams, \
                extend_at_start, \
                extend_at_end, \
                lead_in_line_len,\
                lead_out_line_len)

        output += PathKurveUtils.retrieve_gcode()
        return output

    def execute(self,obj):
        import Part #math #DraftGeomUtils
        output = ""
        toolLoad = PathUtils.getLastToolLoad(obj)
        if toolLoad == None:
            self.vertFeed = 100
            self.horizFeed = 100
            self.radius = 0.25
            obj.ToolNumber= 0   
        else:
            self.vertFeed = toolLoad.VertFeed.Value
            self.horizFeed = toolLoad.HorizFeed.Value
            tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
            self.radius = tool.Diameter/2
            obj.ToolNumber= toolLoad.ToolNumber   

        if obj.Base:
            for b in obj.Base:
                
            # we only consider the outer wire if this is a Face
            #shape = getattr(obj.Base[0][0].Shape,obj.Base[0][1])
                shape = getattr(b[0].Shape,b[1])

                if shape.ShapeType in ["Edge"]:
                    edges = [getattr(obj.Base[0].Shape,sub) for sub in obj.Base[1]]
                    wire = Part.Wire(edges)
                
                    if not wire.Edges[0].isSame(shape):
                        wire.Edges.reverse()

                else:
                    wire = shape.OuterWire

                edgelist = wire.Edges
                edgelist = Part.__sortEdges__(edgelist)
            
                if obj.Algorithm == "OCC Native":
                    output += self._buildPathOCC(obj, wire)
               
                else:
                    try:
                        import area
                    except:
                        FreeCAD.Console.PrintError(translate("Path","libarea needs to be installed for this command to work.\n"))
                        return
                    output += self._buildPathLibarea(obj,edgelist)

        if obj.Active:
            path = Path.Path(output)
            obj.Path = path
            obj.ViewObject.Visibility = True

        else:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False

class _ViewProviderProfile:

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


class _CommandAddTag:
    def GetResources(self):
        return {'Pixmap'  : 'Path-Holding',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Profile","Add Holding Tag"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Profile","Add Holding Tag")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
    
    def setpoint(self,point,o):
        obj=FreeCADGui.Selection.getSelection()[0]
        obj.StartPoint.x = point.x
        obj.StartPoint.y = point.y
        loc = obj.locs
        h = obj.heights
        l = obj.lengths
        a = obj.angles
        
        x =  point.x
        y =  point.y
        z =  float(0.0)
        loc.append(Vector(x,y,z))
        h.append(4.0)
        l.append(5.0)
        a.append(45.0)

        obj.locs = loc
        obj.heights = h
        obj.lengths = l
        obj.angles = a

    def Activated(self):

        FreeCADGui.Snapper.getPoint(callback=self.setpoint)

class _CommandSetStartPoint:
    def GetResources(self):
        return {'Pixmap'  : 'Path-Holding',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Profile","Pick Start Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Profile","Pick Start Point")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
    
    def setpoint(self,point,o):
        obj=FreeCADGui.Selection.getSelection()[0]
        obj.StartPoint.x = point.x
        obj.StartPoint.y = point.y
    def Activated(self):

        FreeCADGui.Snapper.getPoint(callback=self.setpoint)

class _CommandSetEndPoint:
    def GetResources(self):
        return {'Pixmap'  : 'Path-Holding',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Profile","Pick End Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Profile","Pick End Point")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
    
    def setpoint(self,point,o):
        obj=FreeCADGui.Selection.getSelection()[0]
        obj.EndPoint.x = point.x
        obj.EndPoint.y = point.y
    def Activated(self):

        FreeCADGui.Snapper.getPoint(callback=self.setpoint)


class CommandPathProfile:
    def GetResources(self):
        return {'Pixmap'  : 'Path-Profile',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathProfile","Profile"),
                'Accel': "P, P",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathProfile","Creates a Path Profile object from selected faces")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        #import Path
        #from PathScripts import PathProject, PathUtils, PathKurveUtils

        ztop = 10.0
        zbottom = 0.0

        FreeCAD.ActiveDocument.openTransaction(translate("Path","Create a Profile"))
        FreeCADGui.addModule("PathScripts.PathProfile")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Profile")')
        FreeCADGui.doCommand('PathScripts.PathProfile.ObjectProfile(obj)')
        FreeCADGui.doCommand('PathScripts.PathProfile._ViewProviderProfile(obj.ViewObject)')

        FreeCADGui.doCommand('obj.Active = True')

        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(ztop + 10.0))
        FreeCADGui.doCommand('obj.StepDown = 1.0')
        FreeCADGui.doCommand('obj.StartDepth= ' + str(ztop))
        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))
        
        FreeCADGui.doCommand('obj.SafeHeight = '+ str(ztop + 2.0))
        FreeCADGui.doCommand('obj.Side = "Left"')
        FreeCADGui.doCommand('obj.OffsetExtra = 0.0')
        FreeCADGui.doCommand('obj.Direction = "CW"')
        FreeCADGui.doCommand('obj.UseComp = False')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')

class TaskPanel:
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/ProfileEdit.ui")
        #self.form = FreeCADGui.PySideUic.loadUi(":/ProfileEdit.ui")
        self.updating = False


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
            if hasattr(self.obj,"StepDown"):
                self.obj.StepDown = self.form.stepDown.value()
            if hasattr(self.obj,"OffsetExtra"):
                self.obj.OffsetExtra = self.form.extraOffset.value()
            if hasattr(self.obj,"SegLen"):
                self.obj.SegLen = self.form.segLen.value()
            if hasattr(self.obj,"RollRadius"):
                self.obj.RollRadius = self.form.rollRadius.value()
            if hasattr(self.obj,"UseComp"):
                self.obj.UseComp = self.form.useCompensation.isChecked()
            if hasattr(self.obj,"UseStartPoint"):
                self.obj.UseStartPoint = self.form.useStartPoint.isChecked()
            if hasattr(self.obj,"UseEndPoint"):
                self.obj.UseEndPoint = self.form.useEndPoint.isChecked()
            if hasattr(self.obj,"Algorithm"):
                self.obj.Algorithm = str(self.form.algorithmSelect.currentText())
            if hasattr(self.obj,"Side"):
                self.obj.Side = str(self.form.cutSide.currentText())
            if hasattr(self.obj,"Direction"):
                self.obj.Direction = str(self.form.direction.currentText())
        self.obj.Proxy.execute(self.obj)

    def open(self):
        self.s =SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)   

    def addBase(self):
         # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()

        if not len(selection) >= 1:
            FreeCAD.Console.PrintError(translate("PathProject","Please select at least one profileable object\n"))
            return
        for s in selection:
            if s.HasSubObjects:
                for i in s.SubElementNames:
                    self.obj.Proxy.addprofilebase(self.obj, s.Object, i)
            else:      
                self.obj.Proxy.addprofilebase(self.obj, s.Object)

        self.form.baseList.clear()
        for i in self.obj.Base:         
            self.form.baseList.addItem(i[0].Name + "." + i[1])
         
    def deleteBase(self):
        dlist = self.form.baseList.selectedItems()
        newlist = []
        for d in dlist:
            for i in self.obj.Base:
                if i[0].Name != d.text().partition(".")[0] or i[1] != d.text().partition(".")[2] :
                    newlist.append (i)
            self.form.baseList.takeItem(self.form.baseList.row(d))
        self.obj.Base = newlist
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

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


    def edit(self,item,column):
        if not self.updating:
            self.resetObject()

    def resetObject(self,remove=None):
        "transfers the values from the widget to the object"
        loc = []
        h = []
        l = []
        a = []
        
        for i in range(self.form.tagTree.topLevelItemCount()):
            it = self.form.tagTree.findItems(str(i+1),QtCore.Qt.MatchExactly,0)[0]
            if (remove == None) or (remove != i):
                if it.text(1):
                    x =  float(it.text(1).split()[0].rstrip(","))
                    y =  float(it.text(1).split()[1].rstrip(","))
                    z =  float(it.text(1).split()[2].rstrip(","))
                    loc.append(Vector(x,y,z))

                else:
                    loc.append(0.0)
                if it.text(2):
                    h.append(float(it.text(2)))
                else:
                    h.append(4.0)
                if it.text(3):
                    l.append(float(it.text(3)))
                else:
                    l.append(5.0)
                if it.text(4):
                    a.append(float(it.text(4)))
                else:
                    a.append(45.0)

        self.obj.locs = loc
        self.obj.heights = h
        self.obj.lengths = l
        self.obj.angles = a

        self.obj.touch()
        FreeCAD.ActiveDocument.recompute()

    def addElement(self):
        self.updating = True

        item = QtGui.QTreeWidgetItem(self.form.tagTree)
        item.setText(0,str(self.form.tagTree.topLevelItemCount()))
        item.setText(1,"0.0, 0.0, 0.0")
        item.setText(2, str(float(4.0)))
        item.setText(3, str(float(10.0)))
        item.setText(4, str(float(45.0)))
        item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
        self.updating = False

        self.resetObject()

    def removeElement(self):
        it = self.form.tagTree.currentItem()
        if it:
            nr = int(it.text(0))-1
            self.resetObject(remove=nr)
            self.update()

    def update(self):
        'fills the treewidget'
        self.updating = True
        self.form.tagTree.clear()
        if self.obj:
            for i in range(len(self.obj.locs)):
                item = QtGui.QTreeWidgetItem(self.form.tagTree)
                item.setText(0,str(i+1))
                l = self.obj.locs[i]
                item.setText(1,str(l.x)+", " + str(l.y) +", " + str(l.z))
                item.setText(2,str(self.obj.heights[i]))
                item.setText(3,str(self.obj.lengths[i]))
                item.setText(4,str(self.obj.angles[i]))
                item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
                item.setTextAlignment(0,QtCore.Qt.AlignLeft)
        self.updating = False
        return

    def setupUi(self):
        self.form.startDepth.setText(str(self.obj.StartDepth.Value))
        self.form.finalDepth.setText(str(self.obj.FinalDepth.Value))
        self.form.safeHeight.setText(str(self.obj.SafeHeight.Value))
        self.form.clearanceHeight.setText(str(self.obj.ClearanceHeight.Value))
        self.form.stepDown.setValue(self.obj.StepDown)
        self.form.extraOffset.setValue(self.obj.OffsetExtra.Value)
        self.form.segLen.setValue(self.obj.SegLen.Value)
        self.form.rollRadius.setValue(self.obj.RollRadius.Value)
        self.form.useCompensation.setChecked(self.obj.UseComp)
        self.form.useStartPoint.setChecked(self.obj.UseStartPoint)
        self.form.useEndPoint.setChecked(self.obj.UseEndPoint)

        index = self.form.algorithmSelect.findText(self.obj.Algorithm, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.algorithmSelect.setCurrentIndex(index)

        index = self.form.cutSide.findText(self.obj.Side, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.cutSide.setCurrentIndex(index)

        index = self.form.direction.findText(self.obj.Direction, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.direction.setCurrentIndex(index)


        for i in self.obj.Base:         
            self.form.baseList.addItem(i[0].Name + "." + i[1])

        for i in range(len(self.obj.locs)):
            item = QtGui.QTreeWidgetItem(self.form.tagTree)
            item.setText(0,str(i+1))
            l = self.obj.locs[i]
            item.setText(1,str(l.x)+", " + str(l.y) +", " + str(l.z))
            item.setText(2,str(self.obj.heights[i]))
            item.setText(3,str(self.obj.lengths[i]))
            item.setText(4,str(self.obj.angles[i]))
            item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
            item.setTextAlignment(0,QtCore.Qt.AlignLeft)

        #Connect Signals and Slots
        #Base Controls
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.reorderBase.clicked.connect(self.reorderBase)

        #Depths
        self.form.startDepth.editingFinished.connect(self.getFields) 
        self.form.finalDepth.editingFinished.connect(self.getFields)
        self.form.stepDown.editingFinished.connect(self.getFields)
        
        #Heights
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)
        
        #operation
        self.form.algorithmSelect.currentIndexChanged.connect(self.getFields)
        self.form.cutSide.currentIndexChanged.connect(self.getFields)
        self.form.direction.currentIndexChanged.connect(self.getFields)
        self.form.useCompensation.clicked.connect(self.getFields)
        self.form.useStartPoint.clicked.connect(self.getFields)
        self.form.useEndPoint.clicked.connect(self.getFields)
        self.form.extraOffset.editingFinished.connect(self.getFields)
        self.form.segLen.editingFinished.connect(self.getFields)
        self.form.rollRadius.editingFinished.connect(self.getFields)
        
        #Tag Form
        QtCore.QObject.connect(self.form.tagTree, QtCore.SIGNAL("itemChanged(QTreeWidgetItem *, int)"), self.edit)
        self.form.addTag.clicked.connect(self.addElement)
        self.form.deleteTag.clicked.connect(self.removeElement)


class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST 
        PST.profileselect()

    def __del__(self):
        import PathScripts.PathSelection as PST 
        PST.clear()

    def addSelection(self,doc,obj,sub,pnt):               # Selection object
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj +')')
        FreeCADGui.updateGui()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Profile',CommandPathProfile())
    FreeCADGui.addCommand('Add_Tag',_CommandAddTag())
    FreeCADGui.addCommand('Set_StartPoint',_CommandSetStartPoint())
    FreeCADGui.addCommand('Set_EndPoint',_CommandSetEndPoint())

FreeCAD.Console.PrintLog("Loading PathProfile... done\n")

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
from PathScripts import PathUtils,PathSelection,PathProject

if FreeCAD.GuiUp:
    import FreeCADGui, PathGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from pivy import coin
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

def makeProfile(self, name="Profile"):
    '''creates a Profile operation'''

    #obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython",name)
    #obj.Label = translate("Path",name)
    obj=self
    ObjectProfile(obj)
    if FreeCAD.GuiUp:
        _ViewProviderProfile(obj.ViewObject)

    locations = []
    angles = []
    lengths = []
    heights = []

    obj.locs = locations
    obj.angles = angles
    obj.lengths = lengths
    obj.heights = heights
    FreeCAD.ActiveDocument.recompute()
    return obj


class ObjectProfile:

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSub","Base","Path",translate("Parent Object","The base geometry of this toolpath"))
        obj.addProperty("App::PropertyBool","Active","Path",translate("Path","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString","Comment","Path",translate("Path","An optional comment for this profile"))

        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm",translate("Path", "The library or algorithm used to generate the path"))
        obj.Algorithm = ['OCC Native','libarea']

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
        #obj.addProperty("App::PropertyDistance", "RetractHeight", "Depth", translate("Retract Height","The height desired to retract tool when path is finished"))

        #Feed Properties
        obj.addProperty("App::PropertySpeed", "VertFeed", "Feed",translate("Path","Feed rate for vertical moves in Z"))
        obj.addProperty("App::PropertySpeed", "HorizFeed", "Feed",translate("Path","Feed rate for horizontal moves"))
       
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

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def execute(self,obj):
        import Part, DraftGeomUtils, math
        import PathScripts.PathKurveUtils as PathKurveUtils
        from PathScripts.PathUtils import depth_params

        if obj.Base:
            # tie the toolnumber to the PathLoadTool object ToolNumber
            if len(obj.InList)>0: #check to see if obj is in the Project group yet
                project = obj.InList[0]
                tl = int(PathUtils.changeTool(obj,project))
                obj.ToolNumber= tl   
            
            tool = PathUtils.getTool(obj,obj.ToolNumber)
            if tool:
                radius = tool.Diameter/2
            else:
                # temporary value,in case we don't have any tools defined already
                radius = 0.25
            
            depthparams = depth_params (obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown, 0.0, obj.FinalDepth.Value, None)
            
            clearance = obj.ClearanceHeight.Value
            step_down=obj.StepDown
            start_depth=obj.StartDepth.Value
            final_depth=obj.FinalDepth.Value
            rapid_safety_space=obj.SafeHeight.Value

            side=obj.Side
            offset_extra=obj.OffsetExtra.Value
            use_CRC=obj.UseComp
            vf=obj.VertFeed.Value
            hf=obj.HorizFeed.Value
            seglen=obj.SegLen.Value
            direction = obj.Direction


            # we only consider the outer wire if this is a Face
            shape = getattr(obj.Base[0].Shape,obj.Base[1][0])
            if shape.ShapeType in ["Edge"]:
                edges = [getattr(obj.Base[0].Shape,sub) for sub in obj.Base[1]]
                wire = Part.Wire(edges)
            
                if not wire.Edges[0].isSame(shape):
                    wire.Edges.reverse()

            else:
                wire = shape.OuterWire

            edgelist = wire.Edges
            #edgelist = Part.__sortEdges__(wire.Edges)

            if obj.StartPoint and obj.UseStartPoint:
                startpoint = obj.StartPoint                
            else:
                startpoint = None

            if obj.EndPoint and obj.UseEndPoint:
                endpoint = obj.EndPoint
            else:
                endpoint = None
                
            edgelist = Part.__sortEdges__(edgelist)
        
            if obj.Algorithm == "OCC Native":
                output = ""
                output += '('+ str(obj.Comment)+')\n'

                if obj.Direction == 'CCW':
                    clockwise=False
                else:
                    clockwise=True

                FirstEdge= None
                PathClosed = DraftGeomUtils.isReallyClosed(wire)

                output += PathUtils.MakePath(wire, side, radius, clockwise, clearance, step_down, start_depth, final_depth, FirstEdge, PathClosed, seglen, vf, hf)
           
            else:
                try:
                    import area
                except:
                    FreeCAD.Console.PrintError(translate("Path","libarea needs to be installed for this command to work.\n"))
                    return
                
                PathKurveUtils.output('mem')
                PathKurveUtils.feedrate_hv(obj.HorizFeed.Value, obj.VertFeed.Value)

                output = ""
                curve = PathKurveUtils.makeAreaCurve(edgelist,direction,startpoint, endpoint)

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

                PathKurveUtils.profile2(curve, side, radius, vf, hf, offset_extra, roll_radius, None,None, depthparams, extend_at_start, extend_at_end, lead_in_line_len,lead_out_line_len) 
                output += PathKurveUtils.retrieve_gcode()

            if obj.Active:
                path = Path.Path(output)
                obj.Path = path
                obj.ViewObject.Visibility = True

            else:
                path = Path.Path("(inactive operation)")
                obj.Path = path
                obj.ViewObject.Visibility = False


# class ViewProviderProfile:

#     def __init__(self,vobj):
#         vobj.Proxy = self

#     def attach(self,vobj):
#         self.Object = vobj.Object
#         return

#     def getIcon(self):
#         return ":/icons/Path-Profile.svg"

#     def __getstate__(self):
#         return None

#     def __setstate__(self,state):
#         return None


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
        import Path
        from PathScripts import PathProject, PathUtils, PathKurve, PathKurveUtils

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("Path","Select one or more edges or a face from one Document object.\n"))
            return
        if len(selection[0].SubObjects) == 0:
            FreeCAD.Console.PrintError(translate("Path","Select one or more edges or a face from one Document object.\n"))
            return
        for s in selection[0].SubObjects:
            if s.ShapeType != "Edge":
                if (s.ShapeType != "Face") or (len(selection[0].SubObjects) != 1):
                    FreeCAD.Console.PrintError(translate("Path","Please select only edges or a single face\n"))
                    return
        if selection[0].SubObjects[0].ShapeType == "Edge":
            try:
                import Part
                w = Part.Wire(selection[0].SubObjects)
            except:
                FreeCAD.Console.PrintError(translate("Path","The selected edges don't form a loop\n"))
                return

        # if everything is ok, execute and register the transaction in the undo/redo stack

        # Take a guess at some reasonable values for Finish depth.
        bb = selection[0].Object.Shape.BoundBox  #parent boundbox
        fbb = selection[0].SubObjects[0].BoundBox #feature boundbox
        if fbb.ZMax < bb.ZMax:
            zbottom = fbb.ZMax
        else:
            zbottom = bb.ZMin

        FreeCAD.ActiveDocument.openTransaction(translate("Path","Create a Profile operation using libarea"))
        FreeCADGui.addModule("PathScripts.PathProfile")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Profile")')
        FreeCADGui.doCommand('PathScripts.PathProfile.makeProfile(obj)')
        #FreeCADGui.doCommand('PathScripts.PathProfile.ObjectProfile(obj)')

        FreeCADGui.doCommand('obj.Active = True')
        subs ="["
        for s in selection[0].SubElementNames:
            subs += '"' + s + '",'
        subs += "]"
        FreeCADGui.doCommand('obj.Base = (FreeCAD.ActiveDocument.' + selection[0].ObjectName + ',' + subs + ')')

        #FreeCADGui.doCommand('obj.ViewObject.Proxy = 0')
        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(bb.ZMax + 10.0))
        FreeCADGui.doCommand('obj.StepDown = 1.0')
        FreeCADGui.doCommand('obj.StartDepth= ' + str(bb.ZMax))
        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))
        
        FreeCADGui.doCommand('obj.SafeHeight = '+ str(bb.ZMax + 2.0))
        FreeCADGui.doCommand('obj.Side = "Left"')
        FreeCADGui.doCommand('obj.OffsetExtra = 0.0')
        FreeCADGui.doCommand('obj.Direction = "CW"')
        FreeCADGui.doCommand('obj.UseComp = False')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

class _ViewProviderProfile:
    "A View Provider for the Holding object"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/Path-Profile.svg"
    def claimChildren(self):
        return []

    def attach(self, vobj):
        return

    def getDisplayModes(self,vobj):
        return ["Default"]

    def getDefaultDisplayMode(self):
        return "Default"

    def setDisplayMode(self,mode):
        return mode

    def updateData(self,obj,prop):
        return
    def onChanged(self, vobj, prop):
        return
 
    def setEdit(self,vobj,mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = _EditPanel()
        taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return

    def doubleClicked(self,vobj):
        self.setEdit(vobj)

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

class _EditPanel:
    '''The editmode TaskPanel for profile tags'''
    def __init__(self):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.

        self.updating = False

        self.obj = None
        self.form = QtGui.QWidget()
        self.form.setObjectName("TaskPanel")
        self.grid = QtGui.QGridLayout(self.form)
        self.grid.setObjectName("grid")
        self.title = QtGui.QLabel(self.form)
        self.grid.addWidget(self.title, 0, 0, 1, 2)

        # tree
        self.tree = QtGui.QTreeWidget(self.form)
        self.grid.addWidget(self.tree, 1, 0, 1, 2)
        self.tree.setColumnCount(4)
        self.tree.header().resizeSection(0,50)
        self.tree.header().resizeSection(1,80)
        self.tree.header().resizeSection(2,60)
        self.tree.header().resizeSection(3,60)

        # buttons
        self.addButton = QtGui.QPushButton(self.form)
        self.addButton.setObjectName("addButton")
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addButton, 3, 0, 1, 1)
        self.addButton.setEnabled(True)

        self.delButton = QtGui.QPushButton(self.form)
        self.delButton.setObjectName("delButton")
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delButton, 3, 1, 1, 1)
        self.delButton.setEnabled(True)

        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        QtCore.QObject.connect(self.tree, QtCore.SIGNAL("itemChanged(QTreeWidgetItem *, int)"), self.edit)
        self.update()
        self.retranslateUi(self.form)

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def update(self):
        'fills the treewidget'
        self.updating = True
        self.tree.clear()
        if self.obj:
            for i in range(len(self.obj.locs)):
                item = QtGui.QTreeWidgetItem(self.tree)
                item.setText(0,str(i+1))
                l = self.obj.locs[i]
                item.setText(1,str(l.x)+", " + str(l.y) +", " + str(l.z))
                item.setText(2,str(self.obj.heights[i]))
                item.setText(3,str(self.obj.lengths[i]))
                item.setText(4,str(self.obj.angles[i]))
                item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
                item.setTextAlignment(0,QtCore.Qt.AlignLeft)
        self.retranslateUi(self.form)
        self.updating = False
        return

    def addElement(self):
        self.updating = True

        item = QtGui.QTreeWidgetItem(self.tree)
        item.setText(0,str(self.tree.topLevelItemCount()))
        item.setText(1,"0.0, 0.0, 0.0")
        item.setText(2, str(float(4.0)))
        item.setText(3, str(float(10.0)))
        item.setText(4, str(float(45.0)))
        item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
        self.updating = False

        self.resetObject()

    def removeElement(self):
        it = self.tree.currentItem()
        if it:
            nr = int(it.text(0))-1
            self.resetObject(remove=nr)
            self.update()

    def edit(self,item,column):
        if not self.updating:
            self.resetObject()

    def resetObject(self,remove=None):
        "transfers the values from the widget to the object"
        loc = []
        h = []
        l = []
        a = []
        
        for i in range(self.tree.topLevelItemCount()):
            it = self.tree.findItems(str(i+1),QtCore.Qt.MatchExactly,0)[0]
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

    def reject(self):
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel=None):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Path", "Holding Tags", None, QtGui.QApplication.UnicodeUTF8))
        self.delButton.setText(QtGui.QApplication.translate("Path", "Remove", None, QtGui.QApplication.UnicodeUTF8))
        self.addButton.setText(QtGui.QApplication.translate("Path", "Add", None, QtGui.QApplication.UnicodeUTF8))
        self.title.setText(QtGui.QApplication.translate("Path", "Tag Locations and Properties", None, QtGui.QApplication.UnicodeUTF8))
        self.tree.setHeaderLabels([QtGui.QApplication.translate("Path", "", None, QtGui.QApplication.UnicodeUTF8),
                                   QtGui.QApplication.translate("Path", "Location", None, QtGui.QApplication.UnicodeUTF8),
                                   QtGui.QApplication.translate("Path", "Height", None, QtGui.QApplication.UnicodeUTF8),
                                   QtGui.QApplication.translate("Path", "Length", None, QtGui.QApplication.UnicodeUTF8),
                                   QtGui.QApplication.translate("Path", "Angle", None, QtGui.QApplication.UnicodeUTF8)])


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Profile',CommandPathProfile())
    FreeCADGui.addCommand('Add_Tag',_CommandAddTag())
    FreeCADGui.addCommand('Set_StartPoint',_CommandSetStartPoint())
    FreeCADGui.addCommand('Set_EndPoint',_CommandSetEndPoint())

FreeCAD.Console.PrintLog("Loading PathProfile... done\n")

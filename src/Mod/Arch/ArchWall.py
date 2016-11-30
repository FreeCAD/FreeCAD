#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *
#*   Yorik van Havre <yorik@uncreated.net>                                 *
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

import FreeCAD,Draft,ArchComponent,DraftVecUtils,ArchCommands,math
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
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
    
## @package ArchWall
#  \ingroup ARCH
#  \brief The Wall object and tools
#
#  This module provides tools to build Wall objects.
#  Walls are simple objects, usually vertical, obtained
#  by giving a thickness to a base line, then extruding it
#  vertically.

__title__="FreeCAD Wall"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

# Possible roles for walls
Roles = ['Wall','Wall Layer','Beam','Column','Curtain Wall']

def makeWall(baseobj=None,length=None,width=None,height=None,align="Center",face=None,name="Wall"):
    '''makeWall([obj],[length],[width],[height],[align],[face],[name]): creates a wall based on the
    given object, which can be a sketch, a draft object, a face or a solid, or no object at
    all, then you must provide length, width and height. Align can be "Center","Left" or "Right",
    face can be an index number of a face in the base object to base the wall on.'''
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    obj.Label = translate("Arch",name)
    _Wall(obj)
    if FreeCAD.GuiUp:
        _ViewProviderWall(obj.ViewObject)
    if baseobj:
        if baseobj.isDerivedFrom("Part::Feature") or baseobj.isDerivedFrom("Mesh::Feature"):
            obj.Base = baseobj
        else:
            FreeCAD.Console.PrintWarning(str(translate("Arch","Walls can only be based on Part or Mesh objects")))
    if face:
        obj.Face = face
    if length:
        obj.Length = length
    if width:
        obj.Width = width
    else:
        obj.Width = p.GetFloat("WallWidth",200)
    if height:
        obj.Height = height
    else:
        obj.Height = p.GetFloat("WallHeight",3000)
    obj.Align = align
    if obj.Base and FreeCAD.GuiUp:
        if Draft.getType(obj.Base) != "Space":
            obj.Base.ViewObject.hide()
    return obj

def joinWalls(walls,delete=False):
    """joins the given list of walls into one sketch-based wall. If delete
    is True, merged wall objects are deleted"""
    if not walls:
        return None
    if not isinstance(walls,list):
        walls = [walls]
    if not areSameWallTypes(walls):
        return None
    deleteList = []
    base = walls.pop()
    if base.Base:
        if base.Base.Shape.Faces:
            return None
        if Draft.getType(base.Base) == "Sketch":
            sk = base.Base
        else:
            sk = Draft.makeSketch(base.Base,autoconstraints=True)
            if sk:
                base.Base = sk
    for w in walls:
        if w.Base:
            if not w.Base.Shape.Faces:
                for e in w.Base.Shape.Edges:
                    sk.addGeometry(e.Curve)
                    deleteList.append(w.Name)
    if delete:
        for n in deleteList:
            FreeCAD.ActiveDocument.removeObject(n)
    FreeCAD.ActiveDocument.recompute()
    base.ViewObject.show()
    return base

def mergeShapes(w1,w2):
    "returns a Shape built on two walls that share same properties and have a coincident endpoint"
    if not areSameWallTypes([w1,w2]):
        return None
    if (not hasattr(w1.Base,"Shape")) or (not hasattr(w2.Base,"Shape")):
        return None
    if w1.Base.Shape.Faces or w2.Base.Shape.Faces:
        return None

    # TODO fix this
    return None

    eds = w1.Base.Shape.Edges + w2.Base.Shape.Edges
    import DraftGeomUtils
    w = DraftGeomUtils.findWires(eds)
    if len(w) == 1:
        #print "found common wire"
        normal,length,width,height = w1.Proxy.getDefaultValues(w1)
        print w[0].Edges
        sh = w1.Proxy.getBase(w1,w[0],normal,width,height)
        print sh
        return sh
    return None

def areSameWallTypes(walls):
    "returns True is all the walls in the given list have same height, width, and alignment"
    for att in ["Width","Height","Align"]:
        value = None
        for w in walls:
            if not hasattr(w,att):
                return False
            if not value:
                value = getattr(w,att)
            else:
                if type(value) == float:
                    if round(value,Draft.precision()) != round(getattr(w,att),Draft.precision()):
                        return False
                else:
                    if value != getattr(w,att):
                        return False
    return True

class _CommandWall:
    "the Arch Wall command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Wall',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Wall","Wall"),
                'Accel': "W, A",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Wall","Creates a wall object from scratch or from a selected object (wire, face or solid)")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        self.Align = "Center"
        self.Length = None
        self.lengthValue = 0
        self.continueCmd = False
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        self.Width = p.GetFloat("WallWidth",200)
        self.Height = p.GetFloat("WallHeight",3000)
        self.JOIN_WALLS_SKETCHES = p.GetBool("joinWallSketches",False)
        self.AUTOJOIN = p.GetBool("autoJoinWalls",True)
        sel = FreeCADGui.Selection.getSelectionEx()
        done = False
        self.existing = []

        if sel:
            # automatic mode
            import Draft
            if Draft.getType(sel[0].Object) != "Wall":
                FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Wall"))
                FreeCADGui.addModule("Arch")
                for selobj in sel:
                    if Draft.getType(selobj.Object) == "Space":
                        spacedone = False
                        if selobj.HasSubObjects:
                            if "Face" in selobj.SubElementNames[0]:
                                idx = int(selobj.SubElementNames[0][4:])
                                FreeCADGui.doCommand("Arch.makeWall(FreeCAD.ActiveDocument."+selobj.Object.Name+",face="+str(idx)+")")
                                spacedone = True
                        if not spacedone:
                            FreeCADGui.doCommand('Arch.makeWall(FreeCAD.ActiveDocument.'+selobj.Object.Name+')')
                    else:
                        FreeCADGui.doCommand('Arch.makeWall(FreeCAD.ActiveDocument.'+selobj.Object.Name+')')
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
                done = True

        if not done:
            # interactive mode
            import DraftTrackers
            self.points = []
            self.tracker = DraftTrackers.boxTracker()
            if hasattr(FreeCAD,"DraftWorkingPlane"):
                FreeCAD.DraftWorkingPlane.setup()
            FreeCADGui.Snapper.getPoint(callback=self.getPoint,extradlg=self.taskbox())

    def getPoint(self,point=None,obj=None):
        "this function is called by the snapper when it has a 3D point"
        if obj:
            if Draft.getType(obj) == "Wall":
                if not obj in self.existing:
                    self.existing.append(obj)
        if point == None:
            self.tracker.finalize()
            return
        self.points.append(point)
        if len(self.points) == 1:
            self.tracker.width(self.Width)
            self.tracker.height(self.Height)
            self.tracker.on()
            FreeCADGui.Snapper.getPoint(last=self.points[0],callback=self.getPoint,movecallback=self.update,extradlg=self.taskbox())
        elif len(self.points) == 2:
            import Part
            l = Part.Line(FreeCAD.DraftWorkingPlane.getLocalCoords(self.points[0]),FreeCAD.DraftWorkingPlane.getLocalCoords(self.points[1]))
            self.tracker.finalize()
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Wall"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand('import Part')
            FreeCADGui.doCommand('trace=Part.Line(FreeCAD.'+str(l.StartPoint)+',FreeCAD.'+str(l.EndPoint)+')')
            if not self.existing:
                # no existing wall snapped, just add a default wall
                self.addDefault(l)
            else:
                if self.JOIN_WALLS_SKETCHES:
                    # join existing subwalls first if possible, then add the new one
                    w = joinWalls(self.existing)
                    if w:
                        if areSameWallTypes([w,self]):
                            FreeCADGui.doCommand('FreeCAD.ActiveDocument.'+w.Name+'.Base.addGeometry(trace)')
                        else:
                            # if not possible, add new wall as addition to the existing one
                            self.addDefault(l)
                            if self.AUTOJOIN:
                                FreeCADGui.doCommand('Arch.addComponents(FreeCAD.ActiveDocument.'+FreeCAD.ActiveDocument.Objects[-1].Name+',FreeCAD.ActiveDocument.'+w.Name+')')
                    else:
                        self.addDefault(l)
                else:
                    # add new wall as addition to the first existing one
                    self.addDefault(l)
                    if self.AUTOJOIN:
                        FreeCADGui.doCommand('Arch.addComponents(FreeCAD.ActiveDocument.'+FreeCAD.ActiveDocument.Objects[-1].Name+',FreeCAD.ActiveDocument.'+self.existing[0].Name+')')
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            if self.continueCmd:
                self.Activated()

    def addDefault(self,l):
        FreeCADGui.doCommand('base=FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject","'+translate('Arch','WallTrace')+'")')
        FreeCADGui.doCommand('base.Placement = FreeCAD.DraftWorkingPlane.getPlacement()')
        FreeCADGui.doCommand('base.addGeometry(trace)')
        FreeCADGui.doCommand('wall = Arch.makeWall(base,width='+str(self.Width)+',height='+str(self.Height)+',align="'+str(self.Align)+'")')
        FreeCADGui.doCommand('wall.Normal = FreeCAD.DraftWorkingPlane.axis')

    def update(self,point,info):
        "this function is called by the Snapper when the mouse is moved"
        if FreeCADGui.Control.activeDialog():
            b = self.points[0]
            n = FreeCAD.DraftWorkingPlane.axis
            bv = point.sub(b)
            dv = bv.cross(n)
            dv = DraftVecUtils.scaleTo(dv,self.Width/2)
            if self.Align == "Center":
                self.tracker.update([b,point])
            elif self.Align == "Left":
                self.tracker.update([b.add(dv),point.add(dv)])
            else:
                dv = dv.negative()
                self.tracker.update([b.add(dv),point.add(dv)])
            if self.Length:
                self.Length.setText(FreeCAD.Units.Quantity(bv.Length,FreeCAD.Units.Length).UserString)

    def taskbox(self):
        "sets up a taskbox widget"
        w = QtGui.QWidget()
        ui = FreeCADGui.UiLoader()
        w.setWindowTitle(translate("Arch","Wall options").decode("utf8"))
        grid = QtGui.QGridLayout(w)

        label5 = QtGui.QLabel(translate("Arch","Length").decode("utf8"))
        self.Length = ui.createWidget("Gui::InputField")
        self.Length.setText("0.00 mm")
        grid.addWidget(label5,0,0,1,1)
        grid.addWidget(self.Length,0,1,1,1)

        label1 = QtGui.QLabel(translate("Arch","Width").decode("utf8"))
        value1 = ui.createWidget("Gui::InputField")
        value1.setText(FreeCAD.Units.Quantity(self.Width,FreeCAD.Units.Length).UserString)
        grid.addWidget(label1,1,0,1,1)
        grid.addWidget(value1,1,1,1,1)

        label2 = QtGui.QLabel(translate("Arch","Height").decode("utf8"))
        value2 = ui.createWidget("Gui::InputField")
        value2.setText(FreeCAD.Units.Quantity(self.Height,FreeCAD.Units.Length).UserString)
        grid.addWidget(label2,2,0,1,1)
        grid.addWidget(value2,2,1,1,1)

        label3 = QtGui.QLabel(translate("Arch","Alignment").decode("utf8"))
        value3 = QtGui.QComboBox()
        items = ["Center","Left","Right"]
        value3.addItems(items)
        value3.setCurrentIndex(items.index(self.Align))
        grid.addWidget(label3,3,0,1,1)
        grid.addWidget(value3,3,1,1,1)

        label4 = QtGui.QLabel(translate("Arch","Con&tinue").decode("utf8"))
        value4 = QtGui.QCheckBox()
        value4.setObjectName("ContinueCmd")
        value4.setLayoutDirection(QtCore.Qt.RightToLeft)
        label4.setBuddy(value4)
        if hasattr(FreeCADGui,"draftToolBar"):
            value4.setChecked(FreeCADGui.draftToolBar.continueMode)
            self.continueCmd = FreeCADGui.draftToolBar.continueMode
        grid.addWidget(label4,4,0,1,1)
        grid.addWidget(value4,4,1,1,1)

        QtCore.QObject.connect(self.Length,QtCore.SIGNAL("valueChanged(double)"),self.setLength)
        QtCore.QObject.connect(value1,QtCore.SIGNAL("valueChanged(double)"),self.setWidth)
        QtCore.QObject.connect(value2,QtCore.SIGNAL("valueChanged(double)"),self.setHeight)
        QtCore.QObject.connect(value3,QtCore.SIGNAL("currentIndexChanged(int)"),self.setAlign)
        QtCore.QObject.connect(value4,QtCore.SIGNAL("stateChanged(int)"),self.setContinue)
        QtCore.QObject.connect(self.Length,QtCore.SIGNAL("returnPressed()"),value1.setFocus)
        QtCore.QObject.connect(self.Length,QtCore.SIGNAL("returnPressed()"),value1.selectAll)
        QtCore.QObject.connect(value1,QtCore.SIGNAL("returnPressed()"),value2.setFocus)
        QtCore.QObject.connect(value1,QtCore.SIGNAL("returnPressed()"),value2.selectAll)
        QtCore.QObject.connect(value2,QtCore.SIGNAL("returnPressed()"),self.createFromGUI)
        return w
        
    def setLength(self,d):
        self.lengthValue = d

    def setWidth(self,d):
        self.Width = d
        self.tracker.width(d)

    def setHeight(self,d):
        self.Height = d
        self.tracker.height(d)

    def setAlign(self,i):
        self.Align = ["Center","Left","Right"][i]

    def setContinue(self,i):
        self.continueCmd = bool(i)
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.continueMode = bool(i)
            
    def createFromGUI(self):
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Wall"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand('Arch.makeWall(length='+str(self.lengthValue)+',width='+str(self.Width)+',height='+str(self.Height)+',align="'+str(self.Align)+'")')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.escape()

class _CommandMergeWalls:
    "the Arch Merge Walls command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_MergeWalls',
                'MenuText': QT_TRANSLATE_NOOP("Arch_MergeWalls","Merge Walls"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_MergeWalls","Merges the selected walls, if possible")}

    def IsActive(self):
        return bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        walls = FreeCADGui.Selection.getSelection()
        if len(walls) == 1:
            if Draft.getType(walls[0]) == "Wall":
                ostr = "FreeCAD.ActiveDocument."+ walls[0].Name
                ok = False
                for o in walls[0].Additions:
                    if Draft.getType(o) == "Wall":
                        ostr += ",FreeCAD.ActiveDocument." + o.Name
                        ok = True
                if ok:
                    FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Merge Wall")))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("Arch.joinWalls(["+ostr+"],delete=True)")
                    FreeCAD.ActiveDocument.commitTransaction()
                    return
                else:
                    FreeCAD.Console.PrintWarning(str(translate("Arch","The selected wall contain no subwall to merge")))
                    return
            else:
                FreeCAD.Console.PrintWarning(str(translate("Arch","Please select only wall objects")))
                return
        for w in walls:
            if Draft.getType(w) != "Wall":
                FreeCAD.Console.PrintMessage(str(translate("Arch","Please select only wall objects")))
                return
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Merge Walls")))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("Arch.joinWalls(FreeCADGui.Selection.getSelection(),delete=True)")
        FreeCAD.ActiveDocument.commitTransaction()


class _Wall(ArchComponent.Component):
    "The Wall object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLength","Length","Arch",QT_TRANSLATE_NOOP("App::Property","The length of this wall. Not used if this wall is based on an underlying object"))
        obj.addProperty("App::PropertyLength","Width","Arch",QT_TRANSLATE_NOOP("App::Property","The width of this wall. Not used if this wall is based on a face"))
        obj.addProperty("App::PropertyLength","Height","Arch",QT_TRANSLATE_NOOP("App::Property","The height of this wall. Keep 0 for automatic. Not used if this wall is based on a solid"))
        obj.addProperty("App::PropertyEnumeration","Align","Arch",QT_TRANSLATE_NOOP("App::Property","The alignment of this wall on its base object, if applicable"))
        obj.addProperty("App::PropertyVector","Normal","Arch",QT_TRANSLATE_NOOP("App::Property","The normal extrusion direction of this object (keep (0,0,0) for automatic normal)"))
        obj.addProperty("App::PropertyInteger","Face","Arch",QT_TRANSLATE_NOOP("App::Property","The face number of the base object used to build this wall"))
        obj.addProperty("App::PropertyDistance","Offset","Arch",QT_TRANSLATE_NOOP("App::Property","The offset between this wall and its baseline (only for left and right alignments)"))
        obj.Align = ['Left','Right','Center']
        obj.Role = Roles
        self.Type = "Wall"

    def execute(self,obj):
        "builds the wall shape"
        
        if self.clone(obj):
            return

        import Part, DraftGeomUtils
        base = None
        pl = obj.Placement
        extdata = self.getExtrusionData(obj)
        if extdata:
            base = extdata[0]
            base.Placement = extdata[2].multiply(base.Placement)
            extv = extdata[2].Rotation.multVec(extdata[1])
            base = base.extrude(extv)
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.isNull():
                    return
                if not obj.Base.Shape.isValid():
                    if not obj.Base.Shape.Solids:
                        # let pass invalid objects if they have solids...
                        return
                elif obj.Base.Shape.Solids:
                    base = obj.Base.Shape.copy()
            elif obj.Base.isDerivedFrom("Mesh::Feature"):
                if obj.Base.Mesh.isSolid():
                    if obj.Base.Mesh.countComponents() == 1:
                        sh = ArchCommands.getShapeFromMesh(obj.Base.Mesh)
                        if sh.isClosed() and sh.isValid() and sh.Solids and (not sh.isNull()):
                            base = sh
                        else:
                            FreeCAD.Console.PrintWarning(translate("Arch","This mesh is an invalid solid")+"\n")
                            obj.Base.ViewObject.show()
        if not base:
            FreeCAD.Console.PrintError(translate("Arch","Error: Invalid base object")+"\n")
            return

        base = self.processSubShapes(obj,base,pl)
        self.applyShape(obj,base,pl)

        # set the length property
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.Edges:
                    if not obj.Base.Shape.Faces:
                        if hasattr(obj.Base.Shape,"Length"):
                            l = obj.Base.Shape.Length
                            if obj.Length.Value != l:
                                obj.Length = l

    def onChanged(self,obj,prop):
        self.hideSubobjects(obj,prop)
        ArchComponent.Component.onChanged(self,obj,prop)
        
    def getFootprint(self,obj):
        faces = []
        if obj.Shape:
            for f in obj.Shape.Faces:
                if f.normalAt(0,0).getAngle(FreeCAD.Vector(0,0,-1)) < 0.01:
                    if abs(abs(f.CenterOfMass.z) - abs(obj.Shape.BoundBox.ZMin)) < 0.001:
                        faces.append(f)
        return faces
        
    def getExtrusionData(self,obj):
        """returns (shape,extrusion vector,placement) or None"""
        import Part,DraftGeomUtils
        data = ArchComponent.Component.getExtrusionData(self,obj)
        if data:
            return data
        length  = obj.Length.Value
        width = obj.Width.Value
        height = obj.Height.Value
        if not height:
            for p in obj.InList:
                if Draft.getType(p) == "Floor":
                    if p.Height.Value:
                        height = p.Height.Value
        if obj.Normal == Vector(0,0,0):
            normal = Vector(0,0,1)
        else:
            normal = Vector(obj.Normal)
        base = None
        placement = None
        basewires = None
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape:
                    if obj.Base.Shape.Solids:
                        return None
                    elif obj.Face > 0:
                        if len(obj.Base.Shape.Faces) >= obj.Face:
                            face = obj.Base.Shape.Faces[obj.Face-1]
                            # this wall is based on a specific face of its base object
                            normal = face.normalAt(0,0)
                            if normal.getAngle(Vector(0,0,1)) > math.pi/4:
                                normal.multiply(width)
                                base = face.extrude(normal)
                                if obj.Align == "Center":
                                    base.translate(normal.negative().multiply(0.5))
                                elif obj.Align == "Right":
                                    base.translate(normal.negative())
                            else:
                                normal.multiply(height)
                                base = face.extrude(normal)
                            base,placement = self.rebase(base)
                            return (base,normal,placement)
                    elif obj.Base.Shape.Faces:
                        if not DraftGeomUtils.isCoplanar(obj.Base.Shape.Faces):
                            return None
                        else:
                            base,placement = self.rebase(obj.Base.Shape)
                    elif obj.Base.Shape.Wires:
                        basewires = obj.Base.Shape.Wires
                    elif len(obj.Base.Shape.Edges) == 1:
                        basewires = [Part.Wire(obj.Base.Shape.Edges)]
                    if basewires and width:
                        baseface = None
                        for wire in basewires:
                            e = wire.Edges[0]
                            if isinstance(e.Curve,Part.Circle):
                                dvec = e.Vertexes[0].Point.sub(e.Curve.Center)
                            else:
                                dvec = DraftGeomUtils.vec(wire.Edges[0]).cross(normal)
                            if not DraftVecUtils.isNull(dvec):
                                dvec.normalize()
                            sh = None
                            if obj.Align == "Left":
                                dvec.multiply(width)
                                if obj.Offset.Value:
                                    dvec2 = DraftVecUtils.scaleTo(dvec,obj.Offset.Value)
                                    wire = DraftGeomUtils.offsetWire(wire,dvec2)
                                w2 = DraftGeomUtils.offsetWire(wire,dvec)
                                w1 = Part.Wire(Part.__sortEdges__(wire.Edges))
                                sh = DraftGeomUtils.bind(w1,w2)
                            elif obj.Align == "Right":
                                dvec.multiply(width)
                                dvec = dvec.negative()
                                if obj.Offset.Value:
                                    dvec2 = DraftVecUtils.scaleTo(dvec,obj.Offset.Value)
                                    wire = DraftGeomUtils.offsetWire(wire,dvec2)
                                w2 = DraftGeomUtils.offsetWire(wire,dvec)
                                w1 = Part.Wire(Part.__sortEdges__(wire.Edges))
                                sh = DraftGeomUtils.bind(w1,w2)
                            elif obj.Align == "Center":
                                dvec.multiply(width/2)
                                w1 = DraftGeomUtils.offsetWire(wire,dvec)
                                dvec = dvec.negative()
                                w2 = DraftGeomUtils.offsetWire(wire,dvec)
                                sh = DraftGeomUtils.bind(w1,w2)
                            if sh:
                                sh.fix(0.1,0,1) # fixes self-intersecting wires
                                f = Part.Face(sh)
                                if baseface:
                                    baseface = baseface.fuse(f)
                                else:
                                    baseface = f
                        if baseface:
                            base,placement = self.rebase(baseface)
        else:
            l2 = length/2 or 0.5
            w2 = width/2 or 0.5
            v1 = Vector(-l2,-w2,0)
            v2 = Vector(l2,-w2,0)
            v3 = Vector(l2,w2,0)
            v4 = Vector(-l2,w2,0)
            base = Part.Face(Part.makePolygon([v1,v2,v3,v4,v1]))
            placement = FreeCAD.Placement()
        if base and placement:
            extrusion = normal.multiply(height)
            return (base,extrusion,placement)
        return None

class _ViewProviderWall(ArchComponent.ViewProviderComponent):
    "A View Provider for the Wall object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        vobj.ShapeColor = ArchCommands.getDefaultColor("Wall")

    def getIcon(self):
        import Arch_rc
        if hasattr(self,"Object"):
            for o in self.Object.OutList:
                if Draft.getType(o) == "Wall":
                    return ":/icons/Arch_Wall_Tree_Assembly.svg"
        return ":/icons/Arch_Wall_Tree.svg"

    def attach(self,vobj):
        self.Object = vobj.Object
        from pivy import coin
        tex = coin.SoTexture2()
        tex.image = Draft.loadTexture(Draft.svgpatterns()['simple'][1], 128)
        texcoords = coin.SoTextureCoordinatePlane()
        s = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetFloat("patternScale",0.01)
        texcoords.directionS.setValue(s,0,0)
        texcoords.directionT.setValue(0,s,0)
        self.fcoords = coin.SoCoordinate3()
        self.fset = coin.SoIndexedFaceSet()
        sep = coin.SoSeparator()
        sep.addChild(tex)
        sep.addChild(texcoords)
        sep.addChild(self.fcoords)
        sep.addChild(self.fset)
        vobj.RootNode.addChild(sep)
        return

    def updateData(self,obj,prop):
        if prop in ["Placement","Shape"]:
            if obj.ViewObject.DisplayMode == "Footprint":
                obj.ViewObject.Proxy.setDisplayMode("Footprint")

    def getDisplayModes(self,vobj):
        modes=["Footprint"]
        return modes

    def setDisplayMode(self,mode):
        if mode == "Footprint":
            if hasattr(self,"Object"):
                faces = self.Object.Proxy.getFootprint(self.Object)
                if faces:
                    verts = []
                    fdata = []
                    idx = 0
                    for face in faces:
                        tri = face.tessellate(1)
                        for v in tri[0]:
                            verts.append([v.x,v.y,v.z])
                        for f in tri[1]:
                            fdata.extend([f[0]+idx,f[1]+idx,f[2]+idx,-1])
                        idx += len(tri[0])
                    self.fcoords.point.setValues(verts)
                    self.fset.coordIndex.setValues(0,len(fdata),fdata)
            return "Wireframe"
        else:
            self.fset.coordIndex.deleteValues(0)
            self.fcoords.point.deleteValues(0)
            return mode


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Wall',_CommandWall())
    FreeCADGui.addCommand('Arch_MergeWalls',_CommandMergeWalls())

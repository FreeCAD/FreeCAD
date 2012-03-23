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

import FreeCAD,FreeCADGui,Draft,ArchComponent
from draftlibs import fcvec
from FreeCAD import Vector
from PyQt4 import QtCore

__title__="FreeCAD Wall"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeWall(baseobj=None,width=None,height=None,align="Center",name="Wall"):
    '''makeWall(obj,[width],[height],[align],[name]): creates a wall based on the
    given object, which can be a sketch, a draft object, a face or a solid. align
    can be "Center","Left" or "Right"'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Wall(obj)
    _ViewProviderWall(obj.ViewObject)
    if baseobj: obj.Base = baseobj
    if width: obj.Width = width
    if height: obj.Height = height
    obj.Align = align
    if obj.Base: obj.Base.ViewObject.hide()
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    c = p.GetUnsigned("WallColor")
    r = float((c>>24)&0xFF)/255.0
    g = float((c>>16)&0xFF)/255.0
    b = float((c>>8)&0xFF)/255.0
    obj.ViewObject.ShapeColor = (r,g,b,1.0)
    return obj

def joinWalls(walls):
    "joins the given list of walls into one sketch-based wall"
    if not walls:
        return None
    if not isinstance(walls,list):
        walls = [walls]
    if not areSameWallTypes(walls):
        return None
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
            if not base.Base.Shape.Faces:
                for e in base.Base.Shape.Edges:
                    sk.addGeometry(e)
    FreeCAD.ActiveDocument.recompute()
    return base

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
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Wall","Wall"),
                'Accel': "W, A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Wall","Creates a wall object from scratch or from a selected object (wire, face or solid)")}
        
    def Activated(self):

        global QtGui, QtCore
        from PyQt4 import QtGui, QtCore

        self.Width = 0.1
        self.Height = 1
        self.Align = "Center"
        
        sel = FreeCADGui.Selection.getSelection()
        done = False
        self.existing = []
        if sel:
            import Draft
            if Draft.getType(sel[0]) != "Wall":
                FreeCAD.ActiveDocument.openTransaction("Wall")
                for obj in sel:
                    makeWall(obj)
                FreeCAD.ActiveDocument.commitTransaction()
                done = True
        if not done:
            import DraftTrackers
            self.points = []
            self.tracker = DraftTrackers.boxTracker()
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
            self.tracker.on()
            FreeCADGui.Snapper.getPoint(last=self.points[0],callback=self.getPoint,movecallback=self.update,extradlg=self.taskbox())
        elif len(self.points) == 2:
            import Part
            add = False
            l = Part.Line(self.points[0],self.points[1])
            self.tracker.finalize()
            FreeCAD.ActiveDocument.openTransaction("Wall")
            if not self.existing:
                self.addDefault(l)
            else:
                w = joinWalls(self.existing)
                if w:
                    if areSameWallTypes([w,self]):
                        w.Base.addGeometry(l)
                    else:
                        nw = self.addDefault(l)
                        add = True
                else:
                    self.addDefault(l)
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            if add:
                import ArchCommands
                ArchCommands.addComponents(nw,w)

    def addDefault(self,l):
        s = FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject","WallTrace")
        s.addGeometry(l)
        w = makeWall(s,width=self.Width,height=self.Height,align=self.Align)
        return w

    def update(self,point):
        "this function is called by the Snapper when the mouse is moved"
        b = self.points[0]
        n = FreeCAD.DraftWorkingPlane.axis
        bv = point.sub(b)
        dv = bv.cross(n)
        dv = fcvec.scaleTo(dv,self.Width/2)
        if self.Align == "Center":
            self.tracker.update([b,point])
        elif self.Align == "Left":
            self.tracker.update([b.add(dv),point.add(dv)])
        else:
            dv = fcvec.neg(dv)
            self.tracker.update([b.add(dv),point.add(dv)])

    def taskbox(self):
        "sets up a taskbox widget"
        w = QtGui.QWidget()
        w.setWindowTitle("Wall options")
        lay0 = QtGui.QVBoxLayout(w)
        lay1 = QtGui.QHBoxLayout()
        lay0.addLayout(lay1)
        label1 = QtGui.QLabel("Width")
        lay1.addWidget(label1)
        value1 = QtGui.QDoubleSpinBox()
        value1.setDecimals(2)
        value1.setValue(self.Width)
        lay1.addWidget(value1)
        lay2 = QtGui.QHBoxLayout()
        lay0.addLayout(lay2)
        label2 = QtGui.QLabel("Height")
        lay2.addWidget(label2)
        value2 = QtGui.QDoubleSpinBox()
        value2.setDecimals(2)
        value2.setValue(self.Height)
        lay2.addWidget(value2)
        lay3 = QtGui.QHBoxLayout()
        lay0.addLayout(lay3)
        label3 = QtGui.QLabel("Alignment")
        lay3.addWidget(label3)
        value3 = QtGui.QComboBox()
        items = ["Center","Left","Right"]
        value3.addItems(items)
        value3.setCurrentIndex(items.index(self.Align))
        lay3.addWidget(value3)
        QtCore.QObject.connect(value1,QtCore.SIGNAL("valueChanged(double)"),self.setWidth)
        QtCore.QObject.connect(value2,QtCore.SIGNAL("valueChanged(double)"),self.setHeight)
        QtCore.QObject.connect(value3,QtCore.SIGNAL("currentIndexChanged(int)"),self.setAlign)
        return w
        
    def setWidth(self,d):
        self.Width = d
        self.tracker.width(d)

    def setHeight(self,d):
        self.Height = d
        self.tracker.height(d)

    def setAlign(self,i):
        self.Align = ["Center","Left","Right"][i]
        
class _Wall(ArchComponent.Component):
    "The Wall object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLength","Width","Base",
                        "The width of this wall. Not used if this wall is based on a face")
        obj.addProperty("App::PropertyLength","Height","Base",
                        "The height of this wall. Keep 0 for automatic. Not used if this wall is based on a solid")
        obj.addProperty("App::PropertyLength","Length","Base",
                        "The length of this wall. Not used if this wall is based on a shape")
        obj.addProperty("App::PropertyEnumeration","Align","Base",
                        "The alignment of this wall on its base object, if applicable")
        obj.Align = ['Left','Right','Center']
        self.Type = "Wall"
        obj.Width = 0.1
        obj.Length = 1
        obj.Height = 0
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        if prop in ["Base","Height","Width","Align","Additions","Subtractions"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):

        import Part
        from draftlibs import fcgeo

        flat = False
        if hasattr(obj.ViewObject,"DisplayMode"):
            flat = (obj.ViewObject.DisplayMode == "Flat 2D")
        
        def getbase(wire):
            "returns a full shape from a base wire"
            dvec = fcgeo.vec(wire.Edges[0]).cross(normal)
            dvec.normalize()
            if obj.Align == "Left":
                dvec = dvec.multiply(obj.Width)
                w2 = fcgeo.offsetWire(wire,dvec)
                sh = fcgeo.bind(wire,w2)
            elif obj.Align == "Right":
                dvec = dvec.multiply(obj.Width)
                dvec = fcvec.neg(dvec)
                w2 = fcgeo.offsetWire(wire,dvec)
                sh = fcgeo.bind(wire,w2)
            elif obj.Align == "Center":
                dvec = dvec.multiply(obj.Width/2)
                w1 = fcgeo.offsetWire(wire,dvec)
                dvec = fcvec.neg(dvec)
                w2 = fcgeo.offsetWire(wire,dvec)
                sh = fcgeo.bind(w1,w2)
            # fixing self-intersections
            sh.fix(0.1,0,1)
            if height and (not flat):
                norm = Vector(normal).multiply(height)
                sh = sh.extrude(norm)
            return sh
        
        pl = obj.Placement

        # getting default values
        height = normal = None
        if obj.Height:
            height = obj.Height
        else:
            for p in obj.InList:
                if Draft.getType(p) == "Floor":
                    height = p.Height
        if not height: height = 1
        if obj.Normal == Vector(0,0,0):
            normal = Vector(0,0,1)
        else:
            normal = Vector(obj.Normal)

        # computing shape
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                base = obj.Base.Shape.copy()
                if base.Solids:
                    pass
                elif base.Faces:
                    if height:
                        norm = normal.multiply(height)
                        base = base.extrude(norm)
                elif base.Wires:
                    temp = None
                    for wire in obj.Base.Shape.Wires:
                        sh = getbase(wire)
                        if temp:
                            temp = temp.oldFuse(sh)
                        else:
                            temp = sh
                    base = temp
        else:
            if obj.Length == 0:
                return
            v1 = Vector(0,0,0)
            v2 = Vector(obj.Length,0,0)
            w = Part.Wire(Part.Line(v1,v2).toShape())
            base = getbase(w)
            
        for app in obj.Additions:
            base = base.oldFuse(app.Shape)
            app.ViewObject.hide() #to be removed
        for hole in obj.Subtractions:
            cut = False
            if hasattr(hole,"Proxy"):
                if hasattr(hole.Proxy,"Subvolume"):
                    if hole.Proxy.Subvolume:
                        print "cutting subvolume",hole.Proxy.Subvolume
                        base = base.cut(hole.Proxy.Subvolume)
                        cut = True
            if not cut:
                if hasattr(obj,"Shape"):
                    base = base.cut(hole.Shape)
                    hole.ViewObject.hide() # to be removed
        obj.Shape = base
        if not fcgeo.isNull(pl):
            obj.Placement = pl

class _ViewProviderWall(ArchComponent.ViewProviderComponent):
    "A View Provider for the Wall object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):          
        return ":/icons/Arch_Wall_Tree.svg"

    def getDisplayModes(self,vobj):
        return ["Flat 2D"]

    def setDisplayMode(self,mode):
        self.Object.Proxy.createGeometry(self.Object)
        if mode == "Flat 2D":
            return "Flat Lines"
        else:
            return mode

    def attach(self,vobj):
        self.Object = vobj.Object
        return

FreeCADGui.addCommand('Arch_Wall',_CommandWall())

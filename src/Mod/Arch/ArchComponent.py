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

__title__="FreeCAD Arch Component"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

# Possible roles for IFC objects
Roles = ['Undefined','Beam','Beam Standard Case','Chimney','Column','Column Standard Case','Covering','Curtain Wall',
         'Door','Door Standard Case','Foundation','Furniture','Hydro Equipment','Electric Equipment', 
         'Member','Plate','Railing','Ramp','Ramp Flight','Rebar','Pile','Roof','Shading Device','Slab','Space'
         'Stair','Stair Flight','Tendon','Wall','Wall Standard Case','Wall Layer','Window','Window Standard Case']

import FreeCAD,Draft
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui,QtCore
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

def addToComponent(compobject,addobject,mod=None):
    '''addToComponent(compobject,addobject,mod): adds addobject
    to the given component. Default is in "Additions", "Objects" or
    "Components", the first one that exists in the component. Mod
    can be set to one of those attributes ("Objects", Base", etc...)
    to override the default.'''
    import Draft
    if compobject == addobject: return
    # first check zis already there
    found = False
    attribs = ["Additions","Objects","Components","Subtractions","Base"]
    for a in attribs:
        if hasattr(compobject,a):
            if a == "Base":
                if addobject == getattr(compobject,a):
                    found = True
            else:
                if addobject in getattr(compobject,a):
                    found = True
    if not found:
        if mod:
            if hasattr(compobject,mod):
                if mod == "Base":
                    setattr(compobject,mod,addobject)
                    addobject.ViewObject.hide()
                elif mod == "Axes":
                    if Draft.getType(addobject) == "Axis":
                        l = getattr(compobject,mod)
                        l.append(addobject)
                        setattr(compobject,mod,l)
                else:
                    l = getattr(compobject,mod)
                    l.append(addobject)
                    setattr(compobject,mod,l)
                    if mod != "Objects":
                        addobject.ViewObject.hide()
        else:
            for a in attribs[:3]:
                if hasattr(compobject,a):
                    l = getattr(compobject,a)
                    l.append(addobject)
                    setattr(compobject,a,l)
                    addobject.ViewObject.hide()
                    break


def removeFromComponent(compobject,subobject):
    '''removeFromComponent(compobject,subobject): subtracts subobject
    from the given component. If the subobject is already part of the
    component (as addition, subtraction, etc... it is removed. Otherwise,
    it is added as a subtraction.'''
    if compobject == subobject: return
    found = False
    attribs = ["Additions","Subtractions","Objects","Components","Base","Axes","Fixtures"]
    for a in attribs:
        if hasattr(compobject,a):
            if a == "Base":
                if subobject == getattr(compobject,a):
                    setattr(compobject,a,None)
                    subobject.ViewObject.show()
                    found = True
            else:
                if subobject in getattr(compobject,a):
                    l = getattr(compobject,a)
                    l.remove(subobject)
                    setattr(compobject,a,l)
                    subobject.ViewObject.show()
                    found = True
    if not found:
        if hasattr(compobject,"Subtractions"):
            l = compobject.Subtractions
            l.append(subobject)
            compobject.Subtractions = l
            if (Draft.getType(subobject) != "Window") and (not Draft.isClone(subobject,"Window",True)):
                subobject.ViewObject.hide()
                
                
class SelectionTaskPanel:
    """A temp taks panel to wait for a selection"""
    def __init__(self):
        self.form = QtGui.QLabel()
        self.form.setText(QtGui.QApplication.translate("Arch", "Please select a base object", None, QtGui.QApplication.UnicodeUTF8))
        
    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Cancel)
        
    def reject(self):
        if hasattr(FreeCAD,"ArchObserver"):
            FreeCADGui.Selection.removeObserver(FreeCAD.ArchObserver)
            del FreeCAD.ArchObserver
        return True

            
class ComponentTaskPanel:
    '''The default TaskPanel for all Arch components'''
    def __init__(self):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.
        
        self.obj = None
        self.attribs = ["Base","Additions","Subtractions","Objects","Components","Axes","Fixtures","Armatures"]
        self.form = QtGui.QWidget()
        self.form.setObjectName("TaskPanel")
        self.grid = QtGui.QGridLayout(self.form)
        self.grid.setObjectName("grid")
        self.title = QtGui.QLabel(self.form)
        self.grid.addWidget(self.title, 0, 0, 1, 2)

        # tree
        self.tree = QtGui.QTreeWidget(self.form)
        self.grid.addWidget(self.tree, 1, 0, 1, 2)
        self.tree.setColumnCount(1)
        self.tree.header().hide()
        
        # buttons       
        self.addButton = QtGui.QPushButton(self.form)
        self.addButton.setObjectName("addButton")
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addButton, 3, 0, 1, 1)
        self.addButton.setEnabled(False)

        self.delButton = QtGui.QPushButton(self.form)
        self.delButton.setObjectName("delButton")
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delButton, 3, 1, 1, 1)
        self.delButton.setEnabled(False)

        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        QtCore.QObject.connect(self.tree, QtCore.SIGNAL("itemClicked(QTreeWidgetItem*,int)"), self.check)
        QtCore.QObject.connect(self.tree, QtCore.SIGNAL("itemDoubleClicked(QTreeWidgetItem *,int)"), self.editObject)
        self.update()

    def isAllowedAlterSelection(self):
        return True

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)
    
    def check(self,wid,col):
        if not wid.parent():
            self.delButton.setEnabled(False)
            if self.obj:
                sel = FreeCADGui.Selection.getSelection()
                if sel:
                    if not(self.obj in sel):
                        self.addButton.setEnabled(True)
        else:
            self.delButton.setEnabled(True)
            self.addButton.setEnabled(False)

    def getIcon(self,obj):
        if hasattr(obj.ViewObject,"Proxy"):
            return QtGui.QIcon(obj.ViewObject.Proxy.getIcon())
        elif obj.isDerivedFrom("Sketcher::SketchObject"):
            return QtGui.QIcon(":/icons/Sketcher_Sketch.svg")
        else:
            return QtGui.QIcon(":/icons/Tree_Part.svg")

    def update(self):
        'fills the treewidget'
        self.tree.clear()
        dirIcon = QtGui.QApplication.style().standardIcon(QtGui.QStyle.SP_DirIcon)
        for a in self.attribs:
            setattr(self,"tree"+a,QtGui.QTreeWidgetItem(self.tree))
            c = getattr(self,"tree"+a)
            c.setIcon(0,dirIcon)
            c.ChildIndicatorPolicy = 2
            if self.obj:
                if not hasattr(self.obj,a):
                           c.setHidden(True)
            else:
                c.setHidden(True)
        if self.obj:
            for attrib in self.attribs:
                if hasattr(self.obj,attrib):
                    Oattrib = getattr(self.obj,attrib)
                    Tattrib = getattr(self,"tree"+attrib)
                    if Oattrib:
                        if attrib == "Base":
                            Oattrib = [Oattrib]
                        for o in Oattrib:
                            item = QtGui.QTreeWidgetItem()
                            item.setText(0,o.Name)
                            item.setIcon(0,self.getIcon(o))
                            Tattrib.addChild(item)
                        self.tree.expandItem(Tattrib)
        self.retranslateUi(self.form)

    def addElement(self):
        it = self.tree.currentItem()
        if it:
            mod = None
            for a in self.attribs:
                if it == getattr(self,"tree"+a):
                    mod = a
            for o in FreeCADGui.Selection.getSelection():
                addToComponent(self.obj,o,mod)
        self.update()

    def removeElement(self):
        it = self.tree.currentItem()
        if it:
            comp = FreeCAD.ActiveDocument.getObject(str(it.text(0)))
            removeFromComponent(self.obj,comp)
        self.update()

    def accept(self):
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def editObject(self,wid,col):
        if wid.parent():
            obj = FreeCAD.ActiveDocument.getObject(str(wid.text(0)))
            if obj:
                self.obj.ViewObject.Transparency = 80
                self.obj.ViewObject.Selectable = False
                obj.ViewObject.show()
                self.accept()
                if obj.isDerivedFrom("Sketcher::SketchObject"):
                    FreeCADGui.activateWorkbench("SketcherWorkbench")
                FreeCAD.ArchObserver = ArchSelectionObserver(self.obj,obj)
                FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)
                FreeCADGui.ActiveDocument.setEdit(obj.Name,0)

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Components", None, QtGui.QApplication.UnicodeUTF8))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None, QtGui.QApplication.UnicodeUTF8))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None, QtGui.QApplication.UnicodeUTF8))
        self.title.setText(QtGui.QApplication.translate("Arch", "Components of this object", None, QtGui.QApplication.UnicodeUTF8))
        self.treeBase.setText(0,QtGui.QApplication.translate("Arch", "Base component", None, QtGui.QApplication.UnicodeUTF8))
        self.treeAdditions.setText(0,QtGui.QApplication.translate("Arch", "Additions", None, QtGui.QApplication.UnicodeUTF8))
        self.treeSubtractions.setText(0,QtGui.QApplication.translate("Arch", "Subtractions", None, QtGui.QApplication.UnicodeUTF8))
        self.treeObjects.setText(0,QtGui.QApplication.translate("Arch", "Objects", None, QtGui.QApplication.UnicodeUTF8))
        self.treeAxes.setText(0,QtGui.QApplication.translate("Arch", "Axes", None, QtGui.QApplication.UnicodeUTF8))
        self.treeComponents.setText(0,QtGui.QApplication.translate("Arch", "Components", None, QtGui.QApplication.UnicodeUTF8))        
        self.treeFixtures.setText(0,QtGui.QApplication.translate("Arch", "Fixtures", None, QtGui.QApplication.UnicodeUTF8))
        self.treeArmatures.setText(0,QtGui.QApplication.translate("Arch", "Armatures", None, QtGui.QApplication.UnicodeUTF8))
        
class Component:
    "The default Arch Component object"
    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Arch",translate("Arch","The base object this component is built upon"))
        obj.addProperty("App::PropertyLinkList","Additions","Arch",translate("Arch","Other shapes that are appended to this object"))
        obj.addProperty("App::PropertyLinkList","Subtractions","Arch",translate("Arch","Other shapes that are subtracted from this object"))
        obj.addProperty("App::PropertyString","Description","Arch",translate("Arch","An optional description for this component"))
        obj.addProperty("App::PropertyString","Tag","Arch",translate("Arch","An optional tag for this component"))
        obj.addProperty("App::PropertyMap","IfcAttributes","Arch",translate("Arch","Custom IFC properties and attributes"))
        obj.addProperty("App::PropertyMap","Material","Arch",translate("Arch","A material for this object"))
        obj.addProperty("App::PropertyEnumeration","Role","Arch",translate("Arch","The role of this object"))
        obj.addProperty("App::PropertyBool","MoveWithHost","Arch",translate("Arch","Specifies if this object must move together when its host is moved"))
        obj.Proxy = self
        self.Type = "Component"
        self.Subvolume = None
        self.MoveWithHost = False

    def execute(self,obj):
        return

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state
            
    def onChanged(self,obj,prop):
        pass
        
    def getSiblings(self,obj):
        "returns a list of objects with the same type and same base as this object"
        if not hasattr(obj,"Base"):
            return []
        if not obj.Base:
            return []
        siblings = []
        for o in obj.Base.InList:
            if hasattr(o,"Base"):
                if o.Base:
                    if o.Base.Name == obj.Base.Name:
                        if o.Name != obj.Name:
                            if Draft.getType(o) == Draft.getType(obj):
                                siblings.append(o)
        return siblings

    def getAxis(self,obj):
        "Returns an open wire which is the axis of this component, if applicable"
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape:
                    if (len(obj.Base.Shape.Wires) == 1) and not(obj.Base.Shape.Faces):
                        if not obj.Base.Shape.Wires[0].isClosed():
                            return obj.Base.Shape.copy()
                    elif not(obj.Base.Shape.Solids):
                        if hasattr(obj.Base.Shape,"CenterOfMass"):
                            p1 = obj.Base.Shape.CenterOfMass
                            v = self.getExtrusionVector(obj)
                            if v:
                                p2 = p1.add(v)
                                import Part
                                return Part.Line(p1,p2).toShape()
        else:
            p1 = FreeCAD.Vector()
            v = self.getExtrusionVector(obj)
            if v:
                p2 = p1.add(v)
                import Part
                return Part.Line(p1,p2).toShape()
        return None

    def getProfiles(self,obj,noplacement=False):
        "Returns the base profile(s) of this component, if applicable"
        wires = []
        n,l,w,h = self.getDefaultValues(obj)
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape:
                    base = obj.Base.Shape.copy()
                    if noplacement:
                        base.Placement = FreeCAD.Placement()
                    if not base.Solids:
                        if base.Faces:
                            import DraftGeomUtils
                            if not DraftGeomUtils.isCoplanar(base.Faces):
                                return []
                            return [base]
                                
                        basewires = []
                        if not base.Wires:
                            if len(base.Edges) == 1:
                                import Part
                                basewires = [Part.Wire(base.Edges)]
                        else:
                            basewires = base.Wires
                        if basewires:
                            import DraftGeomUtils,DraftVecUtils,Part
                            for wire in basewires:
                                e = wire.Edges[0]
                                if isinstance(e.Curve,Part.Circle):
                                    dvec = e.Vertexes[0].Point.sub(e.Curve.Center)
                                else:
                                    dvec = DraftGeomUtils.vec(wire.Edges[0]).cross(n)
                                if not DraftVecUtils.isNull(dvec):
                                    dvec.normalize()
                                sh = None
                                if hasattr(obj,"Align"):
                                    if obj.Align == "Left":
                                        dvec.multiply(w)
                                        if hasattr(obj,"Offset"):
                                            if obj.Offset.Value:
                                                dvec2 = DraftVecUtils.scaleTo(dvec,obj.Offset.Value)
                                                wire = DraftGeomUtils.offsetWire(wire,dvec2)
                                        w2 = DraftGeomUtils.offsetWire(wire,dvec)
                                        w1 = Part.Wire(DraftGeomUtils.sortEdges(wire.Edges))
                                        sh = DraftGeomUtils.bind(w1,w2)
                                    elif obj.Align == "Right":
                                        dvec.multiply(w)
                                        dvec = dvec.negative()
                                        if hasattr(obj,"Offset"):
                                            if obj.Offset.Value:
                                                dvec2 = DraftVecUtils.scaleTo(dvec,obj.Offset.Value)
                                                wire = DraftGeomUtils.offsetWire(wire,dvec2)
                                        w2 = DraftGeomUtils.offsetWire(wire,dvec)
                                        w1 = Part.Wire(DraftGeomUtils.sortEdges(wire.Edges))
                                        sh = DraftGeomUtils.bind(w1,w2)
                                    elif obj.Align == "Center":
                                        dvec.multiply(w/2)
                                        w1 = DraftGeomUtils.offsetWire(wire,dvec)
                                        dvec = dvec.negative()
                                        w2 = DraftGeomUtils.offsetWire(wire,dvec)
                                        sh = DraftGeomUtils.bind(w1,w2)
                                    if sh:
                                        wires.append(sh)
                                else:
                                    wires.append(wire)
        elif Draft.getType(obj) in ["Wall","Structure"]:
            if (Draft.getType(obj) == "Structure") and (l > h):
                if noplacement:
                    h2 = h/2 or 0.5
                    w2 = w/2 or 0.5
                    v1 = Vector(-h2,-w2,0)
                    v2 = Vector(h2,-w2,0)
                    v3 = Vector(h2,w2,0)
                    v4 = Vector(-h2,w2,0)
                else:
                    h2 = h/2 or 0.5
                    w2 = w/2 or 0.5
                    v1 = Vector(0,-w2,-h2)
                    v2 = Vector(0,-w2,h2)
                    v3 = Vector(0,w2,h2)
                    v4 = Vector(0,w2,-h2)
            else:
                l2 = l/2 or 0.5
                w2 = w/2 or 0.5
                v1 = Vector(-l2,-w2,0)
                v2 = Vector(l2,-w2,0)
                v3 = Vector(l2,w2,0)
                v4 = Vector(-l2,w2,0)
            import Part
            base = Part.makePolygon([v1,v2,v3,v4,v1])
            return [base]
        return wires

    def getExtrusionVector(self,obj,noplacement=False):
        "Returns an extrusion vector of this component, if applicable"
        n,l,w,h = self.getDefaultValues(obj)
        if Draft.getType(obj) == "Structure":
            if l > h:
                v = n.multiply(l)
                if noplacement:
                    import DraftVecUtils
                    v = DraftVecUtils.rounded(FreeCAD.Rotation(FreeCAD.Vector(0,1,0),-90).multVec(v))
                return v
        return n.multiply(h)

    def getDefaultValues(self,obj):
        "returns normal,length,width,height values from this component"
        length = 0
        if hasattr(obj,"Length"):
            if obj.Length.Value:
                length = obj.Length.Value
        width = 0
        if hasattr(obj,"Width"):
            if obj.Width.Value:
                width = obj.Width.Value
        height = 0
        if hasattr(obj,"Height"):
            if obj.Height.Value:
                height = obj.Height.Value
            else:
                for p in obj.InList:
                    if Draft.getType(p) == "Floor":
                        if p.Height.Value:
                            height = p.Height.Value
        default = Vector(0,0,1)
        if Draft.getType(obj) == "Structure":
            if length > height:
                default = Vector(1,0,0)
        if hasattr(obj,"Normal"):
            if obj.Normal == Vector(0,0,0):
                normal = default
            else:
                normal = Vector(obj.Normal)
        else:
            normal = default
        return normal,length,width,height
        
    def getPlacement(self,obj):
        "returns a total placement for the profile of this component"
        p = FreeCAD.Placement()
        if obj.Base:
            p = obj.Base.Placement.multiply(p)
        else:
            if Draft.getType(obj) == "Structure":
                n,l,w,h = self.getDefaultValues(obj)
                if l > h:
                    p.Rotation = FreeCAD.Rotation(FreeCAD.Vector(0,1,0),90)
        p = obj.Placement.multiply(p)
        return p

    def hideSubobjects(self,obj,prop):
        "Hides subobjects when a subobject lists change"
        if prop in ["Additions","Subtractions"]:
            if hasattr(obj,prop):
                for o in getattr(obj,prop):
                    if (Draft.getType(o) != "Window") and (not Draft.isClone(o,"Window",True)):
                        if (Draft.getType(obj) == "Wall"):
                            if (Draft.getType(o) == "Roof"):
                                continue
                        o.ViewObject.hide()

    def processSubShapes(self,obj,base,placement=None):
        "Adds additions and subtractions to a base shape"
        import Draft,Part
        #print "Processing subshapes of ",obj.Label, " : ",obj.Additions
        
        if placement:
            if placement.isNull():
                placement = None
            else:
                placement = FreeCAD.Placement(placement)
                placement = placement.inverse()

        # treat additions
        for o in obj.Additions:
            
            if not base:
                if o.isDerivedFrom("Part::Feature"):
                    base = o.Shape
            else:             
                if base.isNull():
                    if o.isDerivedFrom("Part::Feature"):
                        base = o.Shape
                else:  
                    # special case, both walls with coinciding endpoints
                    import ArchWall
                    js = ArchWall.mergeShapes(o,obj)
                    if js:
                        add = js.cut(base)
                        if placement:
                            add.Placement = add.Placement.multiply(placement)
                        base = base.fuse(add)

                    elif (Draft.getType(o) == "Window") or (Draft.isClone(o,"Window",True)):
                        f = o.Proxy.getSubVolume(o)
                        if f:
                            if base.Solids and f.Solids:
                                if placement:
                                    f.Placement = f.Placement.multiply(placement)
                                base = base.cut(f)
                                
                    elif o.isDerivedFrom("Part::Feature"):
                        if o.Shape:
                            if not o.Shape.isNull():
                                if o.Shape.Solids:
                                    s = o.Shape.copy()
                                    if placement:
                                        s.Placement = s.Placement.multiply(placement)
                                    if base:
                                        if base.Solids:
                                            try:
                                                base = base.fuse(s)
                                            except Part.OCCError:
                                                print "Arch: unable to fuse object ",obj.Name, " with ", o.Name
                                    else:
                                        base = s
        
        # treat subtractions
        for o in obj.Subtractions:
            
            if base:
                if base.isNull():
                    base = None
            
            if base:
                if (Draft.getType(o) == "Window") or (Draft.isClone(o,"Window",True)):
                        # windows can be additions or subtractions, treated the same way
                        f = o.Proxy.getSubVolume(o)
                        if f:
                            if base.Solids and f.Solids:
                                if placement:
                                    f.Placement = f.Placement.multiply(placement)
                                base = base.cut(f)

                elif (Draft.getType(o) == "Roof") or (Draft.isClone(o,"Roof")):
                    # roofs define their own special subtraction volume
                    f = o.Proxy.getSubVolume(o)
                    if f:
                        if base.Solids and f.Solids:
                            base = base.cut(f)
                            
                elif o.isDerivedFrom("Part::Feature"):
                    if o.Shape:
                        if not o.Shape.isNull():
                            if o.Shape.Solids and base.Solids:
                                    s = o.Shape.copy()
                                    if placement:
                                        s.Placement = s.Placement.multiply(placement)
                                    try:
                                        base = base.cut(s)
                                    except Part.OCCError:
                                        print "Arch: unable to cut object ",o.Name, " from ", obj.Name
        return base
        
    def applyShape(self,obj,shape,placement):
        "checks and cleans the given shape, and apply it to the object"
        if shape:
            if not shape.isNull():
                if shape.isValid():
                    if shape.Solids:
                        if shape.Volume < 0:
                            shape.reverse()
                        if shape.Volume < 0:
                            FreeCAD.Console.PrintError(translate("Arch","Error computing the shape of this object")+"\n")
                            return
                        shape = shape.removeSplitter()
                        obj.Shape = shape
                        if not placement.isNull():
                            obj.Placement = placement
                    else:
                        FreeCAD.Console.PrintWarning(obj.Label + " " + translate("Arch","has no solid")+"\n")
                else:
                    FreeCAD.Console.PrintWarning(obj.Label + " " + translate("Arch","has an invalid shape")+"\n")
            else:
                FreeCAD.Console.PrintWarning(obj.Label + " " + translate("Arch","has a null shape")+"\n")


class ViewProviderComponent:
    "A default View Provider for Component objects"
    def __init__(self,vobj):
        vobj.Proxy = self
        self.Object = vobj.Object
        
    def updateData(self,obj,prop):
        return

    def onChanged(self,vobj,prop):
        if prop == "Visibility":
            for obj in vobj.Object.Additions+vobj.Object.Subtractions:
                if (Draft.getType(obj) == "Window") or (Draft.isClone(obj,"Window",True)):
                    obj.ViewObject.Visibility = vobj.Visibility
        return

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def getDisplayModes(self,vobj):
        return []

    def setDisplayMode(self,mode):
        return mode

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def claimChildren(self):
        if hasattr(self,"Object"):
            c = []
            if hasattr(self.Object,"Base"):
                if Draft.getType(self.Object) != "Wall":
                    c = [self.Object.Base]
                elif Draft.getType(self.Object.Base) == "Space":
                    c = []
                else:
                    c = [self.Object.Base]
            if hasattr(self.Object,"Additions"):
                c.extend(self.Object.Additions)
            if hasattr(self.Object,"Subtractions"):
                for s in self.Object.Subtractions:
                    if Draft.getType(self.Object) == "Wall":
                        if Draft.getType(s) == "Roof":
                            continue
                    c.append(s)
            if hasattr(self.Object,"Armatures"):
                c.extend(self.Object.Armatures)
            if hasattr(self.Object,"Group"):
                c.extend(self.Object.Group)
            if hasattr(self.Object,"Tool"):
                if self.Object.Tool:
                    c.append(self.Object.Tool)
            return c
        return []

    def setEdit(self,vobj,mode):
        taskd = ComponentTaskPanel()
        taskd.obj = self.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True
    
    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return False
        
        
class ArchSelectionObserver:
    """ArchSelectionObserver([origin,watched,hide,nextCommand]): The ArchSelectionObserver 
    object can be added as a selection observer to the FreeCAD Gui. If watched is given (a
    document object), the observer will be triggered only when that object is selected/unselected.
    If hide is True, the watched object will be hidden. If origin is given (a document
    object), that object will have its visibility/selectability restored. If nextCommand
    is given (a FreeCAD command), it will be executed on leave."""
    
    def __init__(self,origin=None,watched=None,hide=True,nextCommand=None):
        self.origin = origin
        self.watched = watched
        self.hide = hide
        self.nextCommand = nextCommand
        
    def addSelection(self,document, object, element, position):
        if not self.watched:
            FreeCADGui.Selection.removeObserver(FreeCAD.ArchObserver)
            if self.nextCommand:
                FreeCADGui.runCommand(self.nextCommand)
            del FreeCAD.ArchObserver
        elif object == self.watched.Name:
            if not element:
                FreeCAD.Console.PrintMessage(translate("Arch","closing Sketch edit"))
                if self.hide:
                    if self.origin:
                        self.origin.ViewObject.Transparency = 0
                        self.origin.ViewObject.Selectable = True
                    self.watched.ViewObject.hide()
                FreeCADGui.activateWorkbench("ArchWorkbench")
                if hasattr(FreeCAD,"ArchObserver"):
                    FreeCADGui.Selection.removeObserver(FreeCAD.ArchObserver)
                    del FreeCAD.ArchObserver
                if self.nextCommand:
                    FreeCADGui.Selection.clearSelection()
                    FreeCADGui.Selection.addSelection(self.watched)
                    FreeCADGui.runCommand(self.nextCommand)

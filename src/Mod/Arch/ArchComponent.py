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
Roles = ['Undefined','Beam','Chimney','Column','Covering','Curtain Wall',
         'Door','Foundation','Furniture','Hydro Equipment','Electric Equipment',
         'Member','Plate','Railing','Ramp','Ramp Flight','Rebar','Pile','Roof','Shading Device','Slab','Space',
         'Stair','Stair Flight','Tendon','Wall','Wall Layer','Window']

import FreeCAD,Draft,ArchCommands,math
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui,QtCore
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchComponent
#  \ingroup ARCH
#  \brief The base class of all Arch objects
#
#  This module provides the base Arch component class, that
#  is shared by all of the Arch BIM objects

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
    attribs = ["Additions","Objects","Components","Subtractions","Base","Group","Hosts"]
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
                        if Draft.getType(compobject) == "PanelSheet":
                            addobject.Placement.move(compobject.Placement.Base.negative())
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
    attribs = ["Additions","Subtractions","Objects","Components","Base","Axes","Fixtures","Group","Hosts"]
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
                    if Draft.getType(compobject) == "PanelSheet":
                        subobject.Placement.move(compobject.Placement.Base)
                    found = True
    if not found:
        if hasattr(compobject,"Subtractions"):
            l = compobject.Subtractions
            l.append(subobject)
            compobject.Subtractions = l
            if (Draft.getType(subobject) != "Window") and (not Draft.isClone(subobject,"Window",True)):
                ArchCommands.setAsSubcomponent(subobject)


class SelectionTaskPanel:
    """A temp taks panel to wait for a selection"""
    def __init__(self):
        self.baseform = QtGui.QLabel()
        self.baseform.setText(QtGui.QApplication.translate("Arch", "Please select a base object", None))

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
        self.attribs = ["Base","Additions","Subtractions","Objects","Components","Axes","Fixtures","Group","Hosts"]
        self.baseform = QtGui.QWidget()
        self.baseform.setObjectName("TaskPanel")
        self.grid = QtGui.QGridLayout(self.baseform)
        self.grid.setObjectName("grid")
        self.title = QtGui.QLabel(self.baseform)
        self.grid.addWidget(self.title, 0, 0, 1, 2)
        self.form = self.baseform

        # tree
        self.tree = QtGui.QTreeWidget(self.baseform)
        self.grid.addWidget(self.tree, 1, 0, 1, 2)
        self.tree.setColumnCount(1)
        self.tree.header().hide()

        # buttons
        self.addButton = QtGui.QPushButton(self.baseform)
        self.addButton.setObjectName("addButton")
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addButton, 3, 0, 1, 1)
        self.addButton.setEnabled(False)

        self.delButton = QtGui.QPushButton(self.baseform)
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
            if hasattr(obj.ViewObject.Proxy,"getIcon"):
                return QtGui.QIcon(obj.ViewObject.Proxy.getIcon())
        if obj.isDerivedFrom("Sketcher::SketchObject"):
            return QtGui.QIcon(":/icons/Sketcher_Sketch.svg")
        if obj.isDerivedFrom("App::DocumentObjectGroup"):
            return QtGui.QApplication.style().standardIcon(QtGui.QStyle.SP_DirIcon)
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
                            item.setText(0,o.Label)
                            item.setToolTip(0,o.Name)
                            item.setIcon(0,self.getIcon(o))
                            Tattrib.addChild(item)
                        self.tree.expandItem(Tattrib)
        self.retranslateUi(self.baseform)

    def addElement(self):
        it = self.tree.currentItem()
        if it:
            mod = None
            for a in self.attribs:
                if it.text(0) == getattr(self,"tree"+a).text(0):
                    mod = a
            for o in FreeCADGui.Selection.getSelection():
                addToComponent(self.obj,o,mod)
        self.update()

    def removeElement(self):
        it = self.tree.currentItem()
        if it:
            comp = FreeCAD.ActiveDocument.getObject(str(it.toolTip(0)))
            removeFromComponent(self.obj,comp)
        self.update()

    def accept(self):
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def editObject(self,wid,col):
        if wid.parent():
            obj = FreeCAD.ActiveDocument.getObject(str(wid.toolTip(0)))
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
        self.baseform.setWindowTitle(QtGui.QApplication.translate("Arch", "Components", None))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None))
        self.title.setText(QtGui.QApplication.translate("Arch", "Components of this object", None))
        self.treeBase.setText(0,QtGui.QApplication.translate("Arch", "Base component", None))
        self.treeAdditions.setText(0,QtGui.QApplication.translate("Arch", "Additions", None))
        self.treeSubtractions.setText(0,QtGui.QApplication.translate("Arch", "Subtractions", None))
        self.treeObjects.setText(0,QtGui.QApplication.translate("Arch", "Objects", None))
        self.treeAxes.setText(0,QtGui.QApplication.translate("Arch", "Axes", None))
        self.treeComponents.setText(0,QtGui.QApplication.translate("Arch", "Components", None))
        self.treeFixtures.setText(0,QtGui.QApplication.translate("Arch", "Fixtures", None))
        self.treeGroup.setText(0,QtGui.QApplication.translate("Arch", "Group", None))
        self.treeHosts.setText(0,QtGui.QApplication.translate("Arch", "Hosts", None))

class Component:
    "The default Arch Component object"
    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Component",QT_TRANSLATE_NOOP("App::Property","The base object this component is built upon"))
        obj.addProperty("App::PropertyLink","CloneOf","Component",QT_TRANSLATE_NOOP("App::Property","The object this component is cloning"))
        obj.addProperty("App::PropertyLinkList","Additions","Component",QT_TRANSLATE_NOOP("App::Property","Other shapes that are appended to this object"))
        obj.addProperty("App::PropertyLinkList","Subtractions","Component",QT_TRANSLATE_NOOP("App::Property","Other shapes that are subtracted from this object"))
        obj.addProperty("App::PropertyString","Description","Component",QT_TRANSLATE_NOOP("App::Property","An optional description for this component"))
        obj.addProperty("App::PropertyString","Tag","Component",QT_TRANSLATE_NOOP("App::Property","An optional tag for this component"))
        obj.addProperty("App::PropertyMap","IfcAttributes","Component",QT_TRANSLATE_NOOP("App::Property","Custom IFC properties and attributes"))
        obj.addProperty("App::PropertyLink","Material","Component",QT_TRANSLATE_NOOP("App::Property","A material for this object"))
        obj.addProperty("App::PropertyEnumeration","Role","Component",QT_TRANSLATE_NOOP("App::Property","The role of this object"))
        obj.addProperty("App::PropertyBool","MoveWithHost","Component",QT_TRANSLATE_NOOP("App::Property","Specifies if this object must move together when its host is moved"))
        obj.addProperty("App::PropertyLink","IfcProperties","Component",QT_TRANSLATE_NOOP("App::Property","Custom IFC properties and attributes"))
        obj.addProperty("App::PropertyArea","VerticalArea","Component",QT_TRANSLATE_NOOP("App::Property","The area of all vertical faces of this object"))
        obj.addProperty("App::PropertyArea","HorizontalArea","Component",QT_TRANSLATE_NOOP("App::Property","The area of the projection of this object onto the XY plane"))
        obj.addProperty("App::PropertyLength","PerimeterLength","Component",QT_TRANSLATE_NOOP("App::Property","The perimeter length of the horizontal area"))
        obj.addProperty("App::PropertyLink","HiRes","Component",QT_TRANSLATE_NOOP("App::Property","An optional higher-resolution mesh or shape for this object"))
        obj.addProperty("App::PropertyLink","Axis","Component",QT_TRANSLATE_NOOP("App::Property","An optional axis or axis system on which this object should be duplicated"))
        obj.Proxy = self
        self.Type = "Component"
        self.Subvolume = None
        self.MoveWithHost = False
        obj.Role = Roles
        obj.setEditorMode("VerticalArea",1)
        obj.setEditorMode("HorizontalArea",1)
        obj.setEditorMode("PerimeterLength",1)

    def execute(self,obj):
        if self.clone(obj):
            return
        if obj.Base:
            shape = self.spread(obj,obj.Base.Shape)
            if obj.Additions or obj.Subtractions:
                shape = self.processSubShapes(obj,shape)
            obj.Shape = shape

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state

    def onDocumentRestored(self,obj):
        if hasattr(obj,"BaseMaterial"):
            if not hasattr(obj,"Material"):
                obj.addProperty("App::PropertyLink","Material","Arch",QT_TRANSLATE_NOOP("App::Property","A material for this object"))
                obj.Material = obj.BaseMaterial
                obj.removeProperty("BaseMaterial")
                print("Migrated old BaseMaterial property -> Material in ",obj.Label)

    def onBeforeChange(self,obj,prop):
        if prop == "Placement":
            self.placementBefore = FreeCAD.Placement(obj.Placement)

    def onChanged(self,obj,prop):
        if prop == "Placement":
            if hasattr(self,"placementBefore"):
                delta = FreeCAD.Placement()
                delta.Base = obj.Placement.Base.sub(self.placementBefore.Base)
                delta.Rotation = obj.Placement.multiply(self.placementBefore.inverse()).Rotation
                for o in self.getIncluded(obj,movable=True):
                    o.Placement = o.Placement.multiply(delta)

    def getIncluded(self,obj,movable=False):
        ilist = []
        for o in obj.InList:
            if hasattr(o,"Hosts"):
                if obj in o.Hosts:
                    if movable:
                        if hasattr(o,"MoveWithHost"):
                            if o.MoveWithHost:
                                ilist.append(o) 
                    else:
                        ilist.append(o)
        return ilist

    def clone(self,obj):
        "if this object is a clone, sets the shape. Returns True if this is the case"
        if hasattr(obj,"CloneOf"):
            if obj.CloneOf:
                if (Draft.getType(obj.CloneOf) == Draft.getType(obj)) or (Draft.getType(obj) == "Component"):
                    pl = obj.Placement
                    obj.Shape = obj.CloneOf.Shape.copy()
                    obj.Placement = pl
                    for prop in ["Length","Width","Height","Thickness","Area","PerimeterLength","HorizontalArea","VerticalArea"]:
                        if hasattr(obj,prop) and hasattr(obj.CloneOf,prop):
                            setattr(obj,prop,getattr(obj.CloneOf,prop))
                    return True
        return False

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

    def getExtrusionData(self,obj):
        "returns (shape,extrusion vector,placement) or None"
        if hasattr(obj,"CloneOf"):
            if obj.CloneOf:
                if hasattr(obj.CloneOf,"Proxy"):
                    if hasattr(obj.CloneOf.Proxy,"getExtrusionData"):
                        data = obj.CloneOf.Proxy.getExtrusionData(obj.CloneOf)
                        if data:
                            return data 
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Extrusion"):
                if obj.Base.Base:
                    base,placement = self.rebase(obj.Base.Base.Shape)
                    extrusion = FreeCAD.Vector(obj.Base.Dir)
                    if extrusion.Length == 0:
                        extrusion = FreeCAD.Vector(0,0,1)
                    else:
                        extrusion = placement.inverse().Rotation.multVec(extrusion)
                    if hasattr(obj.Base,"LengthFwd"):
                        if obj.Base.LengthFwd.Value:
                            extrusion = extrusion.multiply(obj.Base.LengthFwd.Value)
                    return (base,extrusion,placement)
            elif obj.Base.isDerivedFrom("Part::MultiFuse"):
                rshapes = []
                revs = []
                rpls = []
                for sub in obj.Base.Shapes:
                    if sub.isDerivedFrom("Part::Extrusion"):
                        if sub.Base:
                            base,placement = self.rebase(sub.Base.Shape)
                            extrusion = FreeCAD.Vector(sub.Dir)
                            if extrusion.Length == 0:
                                extrusion = FreeCAD.Vector(0,0,1)
                            else:
                                extrusion = placement.inverse().Rotation.multVec(extrusion)
                            if hasattr(sub,"LengthFwd"):
                                if sub.LengthFwd.Value:
                                    extrusion = extrusion.multiply(sub.LengthFwd.Value)
                            placement = obj.Placement.multiply(placement)
                            rshapes.append(base)
                            revs.append(extrusion)
                            rpls.append(placement)
                    else:
                        exdata = ArchCommands.getExtrusionData(sub.Shape)
                        if exdata:
                            base,placement = self.rebase(exdata[0])
                            extrusion = placement.inverse().Rotation.multVec(exdata[1])
                            placement = obj.Placement.multiply(placement)
                            rshapes.append(base)
                            revs.append(extrusion)
                            rpls.append(placement)
                if rshapes and revs and rpls:
                    return (rshapes,revs,rpls)
        return None
        
    def rebase(self,shape):
        """returns a shape that is a copy of the original shape
        but centered on the (0,0) origin, and a placement that is needed to
        reposition that shape to its original location/orientation"""
        import DraftGeomUtils,math
        if not isinstance(shape,list):
            shape = [shape]
        if hasattr(shape[0],"CenterOfMass"):
            v = shape[0].CenterOfMass
        else:
            v = shape[0].BoundBox.Center
        n = DraftGeomUtils.getNormal(shape[0])
        r = FreeCAD.Rotation(FreeCAD.Vector(0,0,1),n)
        if round(abs(r.Angle),8) == round(math.pi,8):
            r = FreeCAD.Rotation()
        shapes = []
        for s in shape:
            s = s.copy()
            s.translate(v.negative())
            s.rotate(FreeCAD.Vector(0,0,0),r.Axis,math.degrees(-r.Angle))
            shapes.append(s)
        p = FreeCAD.Placement()
        p.Base = v
        p.Rotation = r
        if len(shapes) == 1:
            return (shapes[0],p)
        else:
            return(shapes,p)

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
        elif prop in ["Mesh"]:
            if hasattr(obj,prop):
                o = getattr(obj,prop)
                if o:
                    o.ViewObject.hide()            

    def processSubShapes(self,obj,base,placement=None):
        "Adds additions and subtractions to a base shape"
        import Draft,Part
        #print("Processing subshapes of ",obj.Label, " : ",obj.Additions)

        if placement:
            if placement.isIdentity():
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
                                                print("Arch: unable to fuse object ", obj.Name, " with ", o.Name)
                                    else:
                                        base = s

        # treat subtractions
        subs = obj.Subtractions
        for link in obj.InList:
            if hasattr(link,"Hosts"):
                for host in link.Hosts:
                    if host == obj:
                        subs.append(link)
        for o in subs:

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
                                if len(base.Solids) > 1:
                                    base = Part.makeCompound([sol.cut(f) for sol in base.Solids])
                                else:
                                    base = base.cut(f)

                elif (Draft.getType(o) == "Roof") or (Draft.isClone(o,"Roof")):
                    # roofs define their own special subtraction volume
                    f = o.Proxy.getSubVolume(o)
                    if f:
                        if base.Solids and f.Solids:
                            if len(base.Solids) > 1:
                                base = Part.makeCompound([sol.cut(f) for sol in base.Solids])
                            else:
                                base = base.cut(f)

                elif o.isDerivedFrom("Part::Feature"):
                    if o.Shape:
                        if not o.Shape.isNull():
                            if o.Shape.Solids and base.Solids:
                                    s = o.Shape.copy()
                                    if placement:
                                        s.Placement = s.Placement.multiply(placement)
                                    try:
                                        if len(base.Solids) > 1:
                                            base = Part.makeCompound([sol.cut(s) for sol in base.Solids])
                                        else:
                                            base = base.cut(s)
                                    except Part.OCCError:
                                        print("Arch: unable to cut object ",o.Name, " from ", obj.Name)
        return base

    def spread(self,obj,shape,placement=None):
        "spreads this shape along axis positions"
        points = None
        if hasattr(obj,"Axis"):
            if obj.Axis:
                if hasattr(obj.Axis,"Proxy"):
                    if hasattr(obj.Axis.Proxy,"getPoints"):
                        points = obj.Axis.Proxy.getPoints(obj.Axis)
                if not points:
                    if obj.Axis.isDerivedFrom("Part.Feature"):
                        points = [v.Point for v in obj.Axis.Shape.Vertexes]
        if points:
            shps = []
            for p in points:
                sh = shape.copy()
                sh.translate(p)
                shps.append(sh)
            import Part
            shape = Part.makeCompound(shps)
        return shape

    def applyShape(self,obj,shape,placement,allowinvalid=False,allownosolid=False):
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
                        import Part
                        try:
                            r = shape.removeSplitter()
                        except Part.OCCError:
                            pass
                        else:
                            shape = r
                        obj.Shape = self.spread(obj,shape,placement)
                        if not placement.isIdentity():
                            obj.Placement = placement
                    else:
                        if allownosolid:
                            obj.Shape = self.spread(obj,shape,placement)
                            if not placement.isIdentity():
                                obj.Placement = placement
                        else:
                            FreeCAD.Console.PrintWarning(obj.Label + " " + translate("Arch","has no solid")+"\n")
                else:
                    if allowinvalid:
                        obj.Shape = self.spread(obj,shape,placement)
                        if not placement.isIdentity():
                            obj.Placement = placement
                    else:
                        FreeCAD.Console.PrintWarning(obj.Label + " " + translate("Arch","has an invalid shape")+"\n")
            else:
                FreeCAD.Console.PrintWarning(obj.Label + " " + translate("Arch","has a null shape")+"\n")
        self.computeAreas(obj)

    def computeAreas(self,obj):
        "computes the area properties"
        if not obj.Shape:
            return
        if obj.Shape.isNull():
            return
        if not obj.Shape.isValid():
            return
        if not obj.Shape.Faces:
            return
        import Drawing,Part
        a = 0
        fset = []
        for i,f in enumerate(obj.Shape.Faces):
            try:
                ang = f.normalAt(0,0).getAngle(FreeCAD.Vector(0,0,1))
            except Part.OCCError:
                print("Debug: Error computing areas for ",obj.Label,": normalAt() Face ",i)
                return
            else:
                if (ang > 1.57) and (ang < 1.571):
                    a += f.Area
                if ang < 1.5707:
                    fset.append(f)
        if a and hasattr(obj,"VerticalArea"):
            if obj.VerticalArea.Value != a:
                obj.VerticalArea = a
        if fset and hasattr(obj,"HorizontalArea"):
            pset = []
            for f in fset:
                if f.normalAt(0,0).getAngle(FreeCAD.Vector(0,0,1)) < 0.00001:
                    # already horizontal
                    pset.append(f)
                else:
                    try:
                        pf = Part.Face(Part.Wire(Drawing.project(f,FreeCAD.Vector(0,0,1))[0].Edges))
                    except Part.OCCError:
                        # error in computing the areas. Better set them to zero than show a wrong value
                        if obj.HorizontalArea.Value != 0:
                            print("Debug: Error computing areas for ",obj.Label,": unable to project face: ",str([v.Point for v in f.Vertexes])," (face normal:",f.normalAt(0,0),")")
                            obj.HorizontalArea = 0
                        if hasattr(obj,"PerimeterLength"):
                            if obj.PerimeterLength.Value != 0:
                                obj.PerimeterLength = 0
                    else:
                        pset.append(pf)
            if pset:
                self.flatarea = pset.pop()
                for f in pset:
                    self.flatarea = self.flatarea.fuse(f)
                self.flatarea = self.flatarea.removeSplitter()
                if obj.HorizontalArea.Value != self.flatarea.Area:
                    obj.HorizontalArea = self.flatarea.Area
                if hasattr(obj,"PerimeterLength") and (len(self.flatarea.Faces) == 1):
                    if obj.PerimeterLength.Value != self.flatarea.Faces[0].OuterWire.Length:
                        obj.PerimeterLength = self.flatarea.Faces[0].OuterWire.Length


class ViewProviderComponent:
    "A default View Provider for Component objects"
    def __init__(self,vobj):
        vobj.Proxy = self
        self.Object = vobj.Object

    def updateData(self,obj,prop):
        #print(obj.Name," : updating ",prop)
        if prop == "Material":
            if obj.Material:
                if hasattr(obj.Material,"Material"):
                    if 'DiffuseColor' in obj.Material.Material:
                        if "(" in obj.Material.Material['DiffuseColor']:
                            c = tuple([float(f) for f in obj.Material.Material['DiffuseColor'].strip("()").split(",")])
                            if obj.ViewObject:
                                if obj.ViewObject.ShapeColor != c:
                                    obj.ViewObject.ShapeColor = c
        elif prop == "Shape":
            if obj.Base:
                if obj.Base.isDerivedFrom("Part::Compound"):
                    if obj.ViewObject.DiffuseColor != obj.Base.ViewObject.DiffuseColor:
                        if len(obj.Base.ViewObject.DiffuseColor) > 1:
                            obj.ViewObject.DiffuseColor = obj.Base.ViewObject.DiffuseColor
                            obj.ViewObject.update()
                        #self.onChanged(obj.ViewObject,"ShapeColor")
        elif prop == "CloneOf":
            if obj.CloneOf:
                mat = None
                if hasattr(obj,"Material"):
                    if obj.Material:
                        mat = obj.Material
                if not mat: 
                    if obj.ViewObject.DiffuseColor != obj.CloneOf.ViewObject.DiffuseColor:
                        if len(obj.CloneOf.ViewObject.DiffuseColor) > 1:
                            obj.ViewObject.DiffuseColor = obj.CloneOf.ViewObject.DiffuseColor
                            obj.ViewObject.update()
                            #self.onChanged(obj.ViewObject,"ShapeColor")
        return

    def getIcon(self):
        import Arch_rc
        if hasattr(self,"Object"):
            if hasattr(self.Object,"CloneOf"):
                if self.Object.CloneOf:
                    return ":/icons/Arch_Component_Clone.svg"
        return ":/icons/Arch_Component.svg"

    def onChanged(self,vobj,prop):
        #print(vobj.Object.Name, " : changing ",prop)
        if prop == "Visibility":
            #for obj in vobj.Object.Additions+vobj.Object.Subtractions:
            #    if (Draft.getType(obj) == "Window") or (Draft.isClone(obj,"Window",True)):
            #        obj.ViewObject.Visibility = vobj.Visibility
            # this would now hide all previous windows... Not the desired behaviour anymore.
            pass
        elif prop == "DiffuseColor":
            if hasattr(vobj.Object,"CloneOf"):
                if vobj.Object.CloneOf:
                    if len(vobj.Object.CloneOf.ViewObject.DiffuseColor) > 1:
                        if vobj.DiffuseColor != vobj.Object.CloneOf.ViewObject.DiffuseColor:
                            vobj.DiffuseColor = vobj.Object.CloneOf.ViewObject.DiffuseColor
                            vobj.update()
        elif prop == "ShapeColor":
            # restore DiffuseColor after overridden by ShapeColor
            if hasattr(vobj,"DiffuseColor"):
                if len(vobj.DiffuseColor) > 1:
                    d = vobj.DiffuseColor
                    vobj.DiffuseColor = d
        return

    def attach(self,vobj):
        from pivy import coin
        self.Object = vobj.Object
        self.hiresgroup = coin.SoSeparator()
        self.meshcolor = coin.SoBaseColor()
        self.hiresgroup.addChild(self.meshcolor)
        self.hiresgroup.setName("HiRes")
        vobj.addDisplayMode(self.hiresgroup,"HiRes");
        return

    def getDisplayModes(self,vobj):
        modes=["HiRes"]
        return modes

    def setDisplayMode(self,mode):
        if hasattr(self,"meshnode"):
            if self.meshnode:
                self.hiresgroup.removeChild(self.meshnode)
                del self.meshnode
        if mode == "HiRes":
            from pivy import coin
            m = None
            if hasattr(self,"Object"):
                if hasattr(self.Object,"HiRes"):
                    if self.Object.HiRes:
                        # if the file was recently loaded, the node is not present yet
                        self.Object.HiRes.ViewObject.show()
                        self.Object.HiRes.ViewObject.hide()
                        m = self.Object.HiRes.ViewObject.RootNode
                if not m:
                    if hasattr(self.Object,"CloneOf"):
                        if self.Object.CloneOf:
                            if hasattr(self.Object.CloneOf,"HiRes"):
                                if self.Object.CloneOf.HiRes:
                                    # if the file was recently loaded, the node is not present yet
                                    self.Object.CloneOf.HiRes.ViewObject.show()
                                    self.Object.CloneOf.HiRes.ViewObject.hide()
                                    m = self.Object.CloneOf.HiRes.ViewObject.RootNode
            if m:
                self.meshnode = m.copy()
                for c in self.meshnode.getChildren():
                    # switch the first found SoSwitch on
                    if isinstance(c,coin.SoSwitch):
                        num = 0
                        if c.getNumChildren() > 0:
                            if c.getChild(0).getName() == "HiRes":
                                num = 1
                        #print "getting node ",num," for ",self.Object.Label
                        c.whichChild = num
                        break
                self.hiresgroup.addChild(self.meshnode)
            else:
                return "Flat Lines"
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
            for link in ["Armatures","Group"]:
                if hasattr(self.Object,link):
                    objlink = getattr(self.Object,link)
                    c.extend(objlink)
            for link in ["Tool","Subvolume","Mesh","HiRes"]:
                if hasattr(self.Object,link):
                    objlink = getattr(self.Object,link)
                    if objlink:
                        c.append(objlink)
            for link in self.Object.InList:
                if hasattr(link,"Host"):
                    if link.Host:
                        if link.Host == self.Object:
                            c.append(link)
                elif hasattr(link,"Hosts"):
                    for host in link.Hosts:
                        if host == self.Object:
                            c.append(link)
            return c
        return []

    def setEdit(self,vobj,mode):
        if mode == 0:
            taskd = ComponentTaskPanel()
            taskd.obj = self.Object
            taskd.update()
            FreeCADGui.Control.showDialog(taskd)
            return True
        return False

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

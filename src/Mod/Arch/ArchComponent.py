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

import FreeCAD,FreeCADGui,Draft
from FreeCAD import Vector
from PyQt4 import QtGui,QtCore
from DraftTools import translate

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
            if Draft.getType(subobject) != "Window":
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
        obj.addProperty("App::PropertyLink","Base","Arch",
                        "The base object this component is built upon")
        obj.addProperty("App::PropertyLinkList","Additions","Arch",
                        "Other shapes that are appended to this object")
        obj.addProperty("App::PropertyLinkList","Subtractions","Arch",
                        "Other shapes that are subtracted from this object")
        obj.Proxy = self
        self.Type = "Component"
        self.Subvolume = None

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
        "returns a list of objects with the same base as this object"
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
                            siblings.append(o)
        return siblings

    def hideSubobjects(self,obj,prop):
        "Hides subobjects when a subobject lists change"
        if prop in ["Additions","Subtractions"]:
            if hasattr(obj,prop):
                for o in getattr(obj,prop):
                    if Draft.getType(o) != "Window":
                        o.ViewObject.hide()

    def processSubShapes(self,obj,base,pl=None):
        "Adds additions and subtractions to a base shape"
        import Draft
        
        if pl:
            if pl.isNull():
                pl = None
            else:
                pl = FreeCAD.Placement(pl)
                pl = pl.inverse()

        # treat additions
        for o in obj.Additions:
            
            if base:
                if base.isNull():
                    base = None
                    
            # special case, both walls with coinciding endpoints
            import ArchWall
            js = ArchWall.mergeShapes(o,obj)
            if js:
                add = js.cut(base)
                if pl:
                    add.Placement = add.Placement.multiply(pl)
                base = base.fuse(add)

            elif (Draft.getType(o) == "Window") or (Draft.isClone(o,"Window")):
                f = o.Proxy.getSubVolume(o)
                if f:
                    if base.Solids and f.Solids:
                        if pl:
                            f.Placement = f.Placement.multiply(pl)
                        base = base.cut(f)
                        
            elif o.isDerivedFrom("Part::Feature"):
                if o.Shape:
                    if not o.Shape.isNull():
                        if o.Shape.Solids:
                            s = o.Shape.copy()
                            if pl:
                                s.Placement = s.Placement.multiply(pl)
                            if base:
                                if base.Solids:
                                    base = base.fuse(s)
                            else:
                                base = s
        
        # treat subtractions
        for o in obj.Subtractions:
            
            if base:
                if base.isNull():
                    base = None
            
            if base:
                if (Draft.getType(o) == "Window") or (Draft.isClone(o,"Window")):
                        # windows can be additions or subtractions, treated the same way
                        f = o.Proxy.getSubVolume(o)
                        if f:
                            if base.Solids and f.Solids:
                                if pl:
                                    f.Placement = f.Placement.multiply(pl)
                                base = base.cut(f)
                            
                elif o.isDerivedFrom("Part::Feature"):
                    if o.Shape:
                        if not o.Shape.isNull():
                            if o.Shape.Solids and base.Solids:
                                    s = o.Shape.copy()
                                    if pl:
                                        s.Placement = s.Placement.multiply(pl)
                                    base = base.cut(s)
        return base

class ViewProviderComponent:
    "A default View Provider for Component objects"
    def __init__(self,vobj):
        vobj.Proxy = self
        self.Object = vobj.Object
        
    def updateData(self,obj,prop):
        return

    def onChanged(self,vobj,prop):
        return

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def getDisplayModes(self,vobj):
        modes=["Detailed"]
        return modes

    def setDisplayMode(self,mode):
        if mode == "Detailed":
            if hasattr(self,"Object"):
                if hasattr(self.Object,"Fixtures"):
                    for f in self.Object.Fixtures:
                        f.ViewObject.show()
            return "Flat Lines"
        else:
            if hasattr(self,"Object"):
                if hasattr(self.Object,"Fixtures"):
                    for f in self.Object.Fixtures:
                        f.ViewObject.hide()
            return mode

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def claimChildren(self):
        if hasattr(self,"Object"):
            if Draft.getType(self.Object) != "Wall":
                c = [self.Object.Base]
            elif Draft.getType(self.Object.Base) == "Space":
                c = []
            else:
                c = [self.Object.Base]
            c = c + self.Object.Additions + self.Object.Subtractions
            if hasattr(self.Object,"Fixtures"):
                c.extend(self.Object.Fixtures)
            if hasattr(self.Object,"Armatures"):
                c.extend(self.Object.Armatures)
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
                FreeCAD.Console.PrintMessage(str(translate("Arch","closing Sketch edit")))
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

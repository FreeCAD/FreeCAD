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

import FreeCAD,FreeCADGui,Draft,ArchComponent,DraftVecUtils,ArchCommands
from FreeCAD import Vector
from PyQt4 import QtCore,QtGui
from DraftTools import translate

__title__="FreeCAD Wall"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeWindow(baseobj=None,width=None,name=str(translate("Arch","Window"))):
    '''makeWindow(obj,[name]): creates a window based on the
    given object'''
    if baseobj:
        if Draft.getType(baseobj) == "Window":
            obj = Draft.clone(baseobj)
            return obj
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Window(obj)
    _ViewProviderWindow(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
        if width:
            obj.WindowParts = ["Default","Panel","Wire0",str(width),"0"]
        else:
            obj.WindowParts = makeDefaultWindowPart(baseobj)
    if obj.Base:
        obj.Base.ViewObject.DisplayMode = "Wireframe"
        obj.Base.ViewObject.hide()
    obj.ViewObject.ShapeColor = ArchCommands.getDefaultColor("Window")
    return obj

def makeDefaultWindowPart(obj):
    "returns a list of 5 strings defining a default window part from a 2D object"
    part = []
    if obj.isDerivedFrom("Part::Feature"):
        if obj.Shape.Wires:
            i = 0
            ws = ''
            for w in obj.Shape.Wires:
                if w.isClosed():
                    if ws: ws += ","
                    ws += "Wire" + str(i)
                    i += 1
            part = ["Default","Panel",ws,"1","0"]
    return part

class _CommandWindow:
    "the Arch Window command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Window',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Window","Window"),
                'Accel': "W, N",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Window","Creates a window object from a selected object (wire, rectangle or sketch)")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            if Draft.getType(sel[0]) == "Wall":
                FreeCADGui.activateWorkbench("SketcherWorkbench")
                FreeCADGui.runCommand("Sketcher_NewSketch")
                FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(sel[0],FreeCAD.ActiveDocument.Objects[-1],hide=False,nextCommand="Arch_Window")
                FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)
            else:
                FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Window")))
                FreeCADGui.doCommand("import Arch")
                for obj in sel:
                    FreeCADGui.doCommand("Arch.makeWindow(FreeCAD.ActiveDocument."+obj.Name+")")
                    if hasattr(obj,"Support"):
                        if obj.Support:
                            if isinstance(obj.Support,tuple):
                                s = obj.Support[0]
                            else:
                                s = obj.Support
                            w = FreeCAD.ActiveDocument.Objects[-1] # last created object
                            FreeCADGui.doCommand("Arch.removeComponents(FreeCAD.ActiveDocument."+w.Name+",host=FreeCAD.ActiveDocument."+s.Name+")")
                    elif Draft.isClone(obj,"Window"):
                        if obj.Objects[0].Inlist:
                            FreeCADGui.doCommand("Arch.removeComponents(FreeCAD.ActiveDocument."+obj.Name+",host=FreeCAD.ActiveDocument."+obj.Objects[0].Inlist[0].Name+")")
                FreeCAD.ActiveDocument.commitTransaction()
       
class _Window(ArchComponent.Component):
    "The Window object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyStringList","WindowParts","Base",
                        str(translate("Arch","the components of this window")))
        self.Type = "Window"
        obj.Proxy = self
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        if prop in ["Base","WindowParts"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):
        import Part, DraftGeomUtils
        pl = obj.Placement
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if hasattr(obj,"WindowParts"):
                    if obj.WindowParts and (len(obj.WindowParts)%5 == 0):
                        shapes = []
                        for i in range(len(obj.WindowParts)/5):
                            wires = []
                            wstr = obj.WindowParts[(i*5)+2].split(',')
                            for s in wstr:
                                j = int(s[4:])
                                if obj.Base.Shape.Wires:
                                    if len(obj.Base.Shape.Wires) >= j:
                                        wires.append(obj.Base.Shape.Wires[j])
                            if wires:
                                max_length = 0
                                for w in wires:
                                    if w.BoundBox.DiagonalLength > max_length:
                                        max_length = w.BoundBox.DiagonalLength
                                        ext = w
                                wires.remove(ext)
                                shape = Part.Face(ext)
                                norm = shape.normalAt(0,0)
                                thk = float(obj.WindowParts[(i*5)+3])
                                if thk:
                                    exv = DraftVecUtils.scaleTo(norm,thk)
                                    shape = shape.extrude(exv)
                                    for w in wires:
                                        f = Part.Face(w)
                                        f = f.extrude(exv)
                                        shape = shape.cut(f)
                                if obj.WindowParts[(i*5)+4]:
                                    zof = float(obj.WindowParts[(i*5)+4])
                                    if zof:
                                        zov = DraftVecUtils.scaleTo(norm,zof)
                                        shape.translate(zov)
                                shapes.append(shape)
                        if shapes:
                            obj.Shape = Part.makeCompound(shapes)
                            if not DraftGeomUtils.isNull(pl):
                                obj.Placement = pl

class _ViewProviderWindow(ArchComponent.ViewProviderComponent):
    "A View Provider for the Window object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Window_Tree.svg"

    def setEdit(self,vobj,mode):
        taskd = _ArchWindowTaskPanel()
        taskd.obj = self.Object
        self.sets = [vobj.DisplayMode,vobj.Transparency]
        vobj.DisplayMode = "Shaded"
        vobj.Transparency = 80
        if self.Object.Base:
            self.Object.Base.ViewObject.show()
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True
    
    def unsetEdit(self,vobj,mode):
        vobj.DisplayMode = self.sets[0]
        vobj.Transparency = self.sets[1]
        if self.Object.Base:
            self.Object.Base.ViewObject.hide()
        FreeCADGui.Control.closeDialog()
        return

class _ArchWindowTaskPanel:
    '''The TaskPanel for Arch Windows'''
    def __init__(self):

        self.obj = None
        self.form = QtGui.QWidget()
        self.form.setObjectName("TaskPanel")
        self.grid = QtGui.QGridLayout(self.form)
        self.grid.setObjectName("grid")
        self.title = QtGui.QLabel(self.form)
        self.grid.addWidget(self.title, 0, 0, 1, 7)

        # trees
        self.tree = QtGui.QTreeWidget(self.form)
        self.grid.addWidget(self.tree, 1, 0, 1, 7)
        self.tree.setColumnCount(1)
        self.tree.setMaximumSize(QtCore.QSize(500,24))
        self.tree.header().hide()

        self.wiretree = QtGui.QTreeWidget(self.form)
        self.grid.addWidget(self.wiretree, 2, 0, 1, 3)
        self.wiretree.setColumnCount(1)
        self.wiretree.setSelectionMode(QtGui.QAbstractItemView.MultiSelection) 

        self.comptree = QtGui.QTreeWidget(self.form)
        self.grid.addWidget(self.comptree, 2, 4, 1, 3)
        self.comptree.setColumnCount(1)        
        
        # buttons       
        self.addButton = QtGui.QPushButton(self.form)
        self.addButton.setObjectName("addButton")
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addButton, 3, 0, 1, 1)
        self.addButton.setMaximumSize(QtCore.QSize(70,40))

        self.editButton = QtGui.QPushButton(self.form)
        self.editButton.setObjectName("editButton")
        self.editButton.setIcon(QtGui.QIcon(":/icons/Draft_Edit.svg"))
        self.grid.addWidget(self.editButton, 3, 2, 1, 3)
        self.editButton.setMaximumSize(QtCore.QSize(60,40))
        self.editButton.setEnabled(False)
        
        self.delButton = QtGui.QPushButton(self.form)
        self.delButton.setObjectName("delButton")
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delButton, 3, 6, 1, 1)
        self.delButton.setMaximumSize(QtCore.QSize(70,40))
        self.delButton.setEnabled(False)

        # add new

        self.newtitle = QtGui.QLabel(self.form)
        self.new1 = QtGui.QLabel(self.form)
        self.new2 = QtGui.QLabel(self.form)
        self.new3 = QtGui.QLabel(self.form)
        self.new4 = QtGui.QLabel(self.form)
        self.new5 = QtGui.QLabel(self.form)
        self.field1 = QtGui.QLineEdit(self.form)
        self.field2 = QtGui.QLineEdit(self.form)
        self.field3 = QtGui.QLineEdit(self.form)
        self.field4 = QtGui.QLineEdit(self.form)
        self.field5 = QtGui.QLineEdit(self.form)
        self.createButton = QtGui.QPushButton(self.form)
        self.createButton.setObjectName("createButton")
        self.createButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.newtitle, 6, 0, 1, 7)
        self.grid.addWidget(self.new1, 7, 0, 1, 1)
        self.grid.addWidget(self.field1, 7, 2, 1, 5)
        self.grid.addWidget(self.new2, 8, 0, 1, 1)
        self.grid.addWidget(self.field2, 8, 2, 1, 5)
        self.grid.addWidget(self.new3, 9, 0, 1, 1)
        self.grid.addWidget(self.field3, 9, 2, 1, 5)
        self.grid.addWidget(self.new4, 10, 0, 1, 1)
        self.grid.addWidget(self.field4, 10, 2, 1, 5) 
        self.grid.addWidget(self.new5, 11, 0, 1, 1)
        self.grid.addWidget(self.field5, 11, 2, 1, 5)
        self.grid.addWidget(self.createButton, 12, 0, 1, 7)
        self.newtitle.setVisible(False)
        self.new1.setVisible(False)
        self.new2.setVisible(False)
        self.new3.setVisible(False)
        self.new4.setVisible(False)
        self.new5.setVisible(False)
        self.field1.setVisible(False)
        self.field2.setVisible(False)
        self.field3.setVisible(False)
        self.field3.setReadOnly(True)
        self.field4.setVisible(False)
        self.field5.setVisible(False)
        self.createButton.setVisible(False)
        
        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        QtCore.QObject.connect(self.editButton, QtCore.SIGNAL("clicked()"), self.editElement)
        QtCore.QObject.connect(self.createButton, QtCore.SIGNAL("clicked()"), self.create)
        QtCore.QObject.connect(self.comptree, QtCore.SIGNAL("itemClicked(QTreeWidgetItem*,int)"), self.check)
        QtCore.QObject.connect(self.wiretree, QtCore.SIGNAL("itemClicked(QTreeWidgetItem*,int)"), self.select)
        self.update()

        FreeCADGui.Selection.clearSelection()

    def isAllowedAlterSelection(self):
        return True

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def check(self,wid,col):
        self.editButton.setEnabled(True)
        self.delButton.setEnabled(True)
        
    def select(self,wid,col):
        FreeCADGui.Selection.clearSelection()
        ws = ''
        for it in self.wiretree.selectedItems():
            if ws: ws += ","
            ws += str(it.text(0))
            w = int(str(it.text(0)[4:]))
            if self.obj:
                if self.obj.Base:
                    edges = self.obj.Base.Shape.Wires[w].Edges
                    for e in edges:
                        for i in range(len(self.obj.Base.Shape.Edges)):
                            if e.hashCode() == self.obj.Base.Shape.Edges[i].hashCode():
                                FreeCADGui.Selection.addSelection(self.obj.Base,"Edge"+str(i+1))
        self.field3.setText(ws)
        
    def getIcon(self,obj):
        if hasattr(obj.ViewObject,"Proxy"):
            return QtGui.QIcon(obj.ViewObject.Proxy.getIcon())
        elif obj.isDerivedFrom("Sketcher::SketchObject"):
            return QtGui.QIcon(":/icons/Sketcher_Sketch.svg")
        else:
            return QtGui.QIcon(":/icons/Tree_Part.svg")

    def update(self):
        'fills the tree widgets'
        self.tree.clear()
        self.wiretree.clear()
        self.comptree.clear()
        if self.obj:
            if self.obj.Base:
                item = QtGui.QTreeWidgetItem(self.tree)
                item.setText(0,self.obj.Base.Name)
                item.setIcon(0,self.getIcon(self.obj.Base))
                if self.obj.Base.isDerivedFrom("Part::Feature"):
                    i = 0
                    for w in self.obj.Base.Shape.Wires:
                        if w.isClosed():
                            item = QtGui.QTreeWidgetItem(self.wiretree)
                            item.setText(0,"Wire" + str(i))
                            item.setIcon(0,QtGui.QIcon(":/icons/Draft_Draft.svg"))
                        i += 1
                if self.obj.WindowParts:
                    for p in range(0,len(self.obj.WindowParts),5):
                        item = QtGui.QTreeWidgetItem(self.comptree)
                        item.setText(0,self.obj.WindowParts[p])
                        item.setIcon(0,QtGui.QIcon(":/icons/Tree_Part.svg"))
                self.retranslateUi(self.form)

    def addElement(self):
        'opens the component creation dialog'
        self.field1.setText('')
        self.field2.setText('')
        self.field3.setText('')
        self.field4.setText('')
        self.field5.setText('')
        self.newtitle.setVisible(True)
        self.new1.setVisible(True)
        #self.new2.setVisible(True)
        self.new3.setVisible(True)
        self.new4.setVisible(True)
        self.new5.setVisible(True)
        self.field1.setVisible(True)
        #self.field2.setVisible(True)
        self.field3.setVisible(True)
        self.field4.setVisible(True)
        self.field5.setVisible(True)
        self.createButton.setVisible(True)
        self.addButton.setEnabled(False)
        self.editButton.setEnabled(False)
        self.delButton.setEnabled(False)

    def removeElement(self):
        for it in self.comptree.selectedItems():
            comp = str(it.text(0))
            if self.obj:
                p = self.obj.WindowParts
                if comp in self.obj.WindowParts:
                    ind = self.obj.WindowParts.index(comp)
                    for i in range(5):
                        p.pop(ind)
                    self.obj.WindowParts = p
                    self.update()
                    self.editButton.setEnabled(False)
                    self.delButton.setEnabled(False)

    def editElement(self):
        for it in self.comptree.selectedItems():
            self.addElement()
            comp = str(it.text(0))
            if self.obj:
                if comp in self.obj.WindowParts:
                    ind = self.obj.WindowParts.index(comp)
                    for i in range(5):
                        f = getattr(self,"field"+str(i+1))
                        t = self.obj.WindowParts[ind+i]
                        f.setText(t)

    def create(self):
        'adds a new component'
        # testing if fields are ok
        ok = True
        ar = []
        for i in range(5):
            t = str(getattr(self,"field"+str(i+1)).text())
            if t == "":
                if not(i in [1,5]):
                    ok = False
            else:
                if i > 2:
                    try:
                        n=float(t)
                    except:
                        ok = False
            ar.append(t)
            
        if ok:
            if self.obj:
                parts = self.obj.WindowParts
                if ar[0] in parts:
                    b = parts.index(ar[0])
                    for i in range(5):
                        parts[b+i] = ar[i]
                else:
                    parts.extend(ar)
                self.obj.WindowParts = parts
                self.update()
        else:
            FreeCAD.Console.PrintWarning(str(translate("Arch", "Unable to create component")))
        
        self.newtitle.setVisible(False)
        self.new1.setVisible(False)
        self.new2.setVisible(False)
        self.new3.setVisible(False)
        self.new4.setVisible(False)
        self.new5.setVisible(False)
        self.field1.setVisible(False)
        self.field2.setVisible(False)
        self.field3.setVisible(False)
        self.field4.setVisible(False)
        self.field5.setVisible(False)
        self.createButton.setVisible(False)
        self.addButton.setEnabled(True)
    
    def accept(self):
        FreeCAD.ActiveDocument.recompute()
        if self.obj:
            self.obj.ViewObject.finishEditing()
        return True
                    
    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Components", None, QtGui.QApplication.UnicodeUTF8))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None, QtGui.QApplication.UnicodeUTF8))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None, QtGui.QApplication.UnicodeUTF8))
        self.editButton.setText(QtGui.QApplication.translate("Arch", "Edit", None, QtGui.QApplication.UnicodeUTF8))
        self.createButton.setText(QtGui.QApplication.translate("Arch", "Create/update component", None, QtGui.QApplication.UnicodeUTF8))
        self.title.setText(QtGui.QApplication.translate("Arch", "Base 2D object", None, QtGui.QApplication.UnicodeUTF8))
        self.wiretree.setHeaderLabels([QtGui.QApplication.translate("Arch", "Wires", None, QtGui.QApplication.UnicodeUTF8)])
        self.comptree.setHeaderLabels([QtGui.QApplication.translate("Arch", "Components", None, QtGui.QApplication.UnicodeUTF8)])
        self.newtitle.setText(QtGui.QApplication.translate("Arch", "Create new component", None, QtGui.QApplication.UnicodeUTF8))
        self.new1.setText(QtGui.QApplication.translate("Arch", "Name", None, QtGui.QApplication.UnicodeUTF8))
        self.new2.setText(QtGui.QApplication.translate("Arch", "Type", None, QtGui.QApplication.UnicodeUTF8))
        self.new3.setText(QtGui.QApplication.translate("Arch", "Wires", None, QtGui.QApplication.UnicodeUTF8))
        self.new4.setText(QtGui.QApplication.translate("Arch", "Thickness", None, QtGui.QApplication.UnicodeUTF8))
        self.new5.setText(QtGui.QApplication.translate("Arch", "Z offset", None, QtGui.QApplication.UnicodeUTF8))
        
FreeCADGui.addCommand('Arch_Window',_CommandWindow())

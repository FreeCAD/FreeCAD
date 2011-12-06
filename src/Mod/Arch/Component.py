#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
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
__url__ = "http://free-cad.sourceforge.net"

import FreeCAD,FreeCADGui
from PyQt4 import QtGui,QtCore

def addToComponent(compobject,addobject):
    '''addToComponent(compobject,addobject): adds addobject
    to the given component'''
    if hasattr(compobject,"Additions"):
        if not addobject in compobject.Additions:
            l = compobject.Additions
            l.append(addobject)
            compobject.Additions = l
    elif hasattr(compobject,"Objects"):
        if not addobject in compobject.Objects:
            l = compobject.Objects
            l.append(addobject)
            compobject.Objects = l

def removeFromComponent(compobject,subobject):
    '''removeFromComponent(compobject,subobject): subtracts subobject
    from the given component'''
    if hasattr(compobject,"Subtractions") and hasattr(compobject,"Additions"):
        if subobject in compobject.Subtractions:
            l = compobject.Subtractions
            l.remove(subobject)
            compobject.Subtractions = l
            subobject.ViewObject.show()
        elif subobject in compobject.Additions:
            l = compobject.Additions
            l.remove(subobject)
            compobject.Additions = l
            subobject.ViewObject.show()
        else:
            l = compobject.Subtractions
            l.append(subobject)
            compobject.Subtractions = l
    elif hasattr(compobject,"Objects"):
        if subobject in compobject.Objects:
            l = compobject.Objects
            l.remove(subobject)
            compobject.Objects = l

class ComponentTaskPanel:
    '''The default TaskPanel for all Arch components'''
    def __init__(self):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.
        
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
        self.tree.setColumnCount(1)
        self.tree.header().hide()
        self.treeBase = QtGui.QTreeWidgetItem(self.tree)
        self.treeBase.setIcon(0,QtGui.QIcon(":/icons/Tree_Part.svg"))
        self.treeBase.ChildIndicatorPolicy = 2
        self.treeBase.setHidden(True)
        self.treeAdditions = QtGui.QTreeWidgetItem(self.tree)
        self.treeAdditions.setIcon(0,QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.treeAdditions.ChildIndicatorPolicy = 2
        self.treeAdditions.setHidden(True)
        self.treeSubtractions = QtGui.QTreeWidgetItem(self.tree)
        self.treeSubtractions.setIcon(0,QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.treeSubtractions.ChildIndicatorPolicy = 2
        self.treeSubtractions.setHidden(True)
        self.treeObjects = QtGui.QTreeWidgetItem(self.tree)
        self.treeObjects.setIcon(0,QtGui.QIcon(":/icons/Tree_Part.svg"))
        self.treeObjects.ChildIndicatorPolicy = 2
        self.treeObjects.setHidden(True)
        
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

        self.okButton = QtGui.QPushButton(self.form)
        self.okButton.setObjectName("okButton")
        self.okButton.setIcon(QtGui.QIcon(":/icons/edit_OK.svg"))
        self.grid.addWidget(self.okButton, 4, 0, 1, 2)

        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        QtCore.QObject.connect(self.okButton, QtCore.SIGNAL("clicked()"), self.finish)
        QtCore.QObject.connect(self.tree, QtCore.SIGNAL("itemClicked(QTreeWidgetItem*,int)"), self.check)
        self.retranslateUi(self.form)
        self.populate()

    def isAllowedAlterSelection(self):
        return True

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return 0

    def removeElement(self):
        return

    def check(self,wid,col):
        if not wid.parent():
            self.delButton.setEnabled(False)
        else:
            self.delButton.setEnabled(True)

    def getIcon(self,obj):
        if hasattr(obj.ViewObject,"Proxy"):
            return QtGui.QIcon(obj.ViewObject.Proxy.getIcon())
        elif obj.isDerivedFrom("Sketcher::SketchObject"):
            return QtGui.QIcon(":/icons/Sketcher_Sketch.svg")
        else:
            return QtGui.QIcon(":/icons/Tree_Part.svg")

    def populate(self):
        'fills the treewidget'
        if self.obj:
            for attrib in ["Base","Additions","Subtractions","Objects"]:
                if hasattr(self.obj,attrib):
                    Oattrib = getattr(self.obj,attrib)
                    Tattrib = getattr(self,"tree"+attrib)
                    if Oattrib:
                        if attrib == "Base":
                            Oattrib = [Oattrib]
                        for o in Oattrib:
                            item = QtGui.QTreeWidgetItem()
                            item.setText(0,o.Label)
                            item.setIcon(0,self.getIcon(o))
                            Tattrib.addChild(item)
                        self.tree.expandItem(Tattrib)
                        Tattrib.setHidden(False)
                    else:
                        Tattrib.setHidden(True)

    def addElement(self):
        FreeCAD.Console.PrintWarning("Not implemented yet!\n")

    def removeElement(self):
        it = self.tree.currentItem()
        if it:
            comp = FreeCAD.ActiveDocument.getObject(str(it.text()))
            removeFromComponent(self.obj,comp)
        self.populate()

    def finish(self):
        FreeCAD.ActiveDocument.recompute()
        if self.obj:
            self.obj.ViewObject.finishEditing()
                    
    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Components", None, QtGui.QApplication.UnicodeUTF8))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None, QtGui.QApplication.UnicodeUTF8))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None, QtGui.QApplication.UnicodeUTF8))
        self.okButton.setText(QtGui.QApplication.translate("Arch", "Done", None, QtGui.QApplication.UnicodeUTF8))
        self.title.setText(QtGui.QApplication.translate("Arch", "Components of this object", None, QtGui.QApplication.UnicodeUTF8))
        self.treeBase.setText(0,QtGui.QApplication.translate("Arch", "Base component", None, QtGui.QApplication.UnicodeUTF8))
        self.treeAdditions.setText(0,QtGui.QApplication.translate("Arch", "Additions", None, QtGui.QApplication.UnicodeUTF8))
        self.treeSubtractions.setText(0,QtGui.QApplication.translate("Arch", "Subtractions", None, QtGui.QApplication.UnicodeUTF8))
        self.treeObjects.setText(0,QtGui.QApplication.translate("Arch", "Objects", None, QtGui.QApplication.UnicodeUTF8))
        
        
class Component:
    "The default Arch Component object"
    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object this component is built upon")
        obj.addProperty("App::PropertyLinkList","Additions","Base",
                        "Other shapes that are appended to this wall")
        obj.addProperty("App::PropertyLinkList","Subtractions","Base",
                        "Other shapes that are subtracted from this wall")
        obj.addProperty("App::PropertyVector","Normal","Base",
                        "The normal extrusion direction of this wall (keep (0,0,0) for automatic normal)")
        obj.Proxy = self
        self.Type = "Component"
        self.Object = obj
        self.Subvolume = None
        
        
class ViewProviderComponent:
    "A default View Provider for Component objects"
    def __init__(self,vobj):
        vobj.Proxy = self
        self.Object = vobj.Object
        
    def updateData(self,vobj,prop):
        return

    def onChanged(self,vobj,prop):
        return

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def getDisplayModes(self,vobj):
        modes=[]
        return modes

    def setDisplayMode(self,mode):
        return mode

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def claimChildren(self):
        return [self.Object.Base]+self.Object.Additions+self.Object.Subtractions

    def setEdit(self,vobj,mode):
        taskd = ComponentTaskPanel()
        taskd.obj = self.Object
        taskd.populate()
        FreeCADGui.Control.showDialog(taskd)
        return True
    
    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return
    

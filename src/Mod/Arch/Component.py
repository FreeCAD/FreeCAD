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

import FreeCADGui
from PyQt4 import QtGui,QtCore

def addToComponent(compobject,addobject):
    '''addToComponent(compobject,addobject): adds addobject
    to the given component'''
    if "Additions" in compobject.PropertiesList:
        if not addobject in compobject.Additions:
            compobject.Additions.append(addobject)

def removeFromComponent(compobject,subobject):
    '''removeFromComponent(compobject,subobject): subtracts subobject
    from the given component'''
    if "Subtractions" in compobject.PropertiesList:
        if not subobject in compobject.Subtractions:
            compobject.Subtractions.append(subobject)

class ComponentTaskPanel:
    def __init__(self):
        self.obj = None
        self.form = QtGui.QWidget()
        self.form.setObjectName("TaskPanel")
        #self.form.resize(210, 260)
        self.gridLayout = QtGui.QGridLayout(self.form)
        self.gridLayout.setObjectName("gridLayout")
        self.title = QtGui.QLabel(self.form)
        self.gridLayout.addWidget(self.title, 0, 0, 1, 1)
        self.listWidget = QtGui.QListWidget(self.form)
        self.listWidget.setObjectName("listWidget")
        self.gridLayout.addWidget(self.listWidget, 1, 0, 1, 2)
        self.addButton = QtGui.QPushButton(self.form)
        self.addButton.setObjectName("addButton")
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.gridLayout.addWidget(self.addButton, 2, 0, 1, 1)
        self.delButton = QtGui.QPushButton(self.form)
        self.delButton.setObjectName("delButton")
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.gridLayout.addWidget(self.delButton, 3, 0, 1, 1)
        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        self.retranslateUi(self.form)

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)
        
    def addElement(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            for o in sel:
                addToComponent(self.obj,o)
                    
    def fillList(self):
        self.listWidget.clear()
        if self.obj.Base:
            i = QtGui.QListWidgetItem(self.listWidget)
            i.setText(self.obj.Base.Name)
            i.setIcon(QtGui.QIcon(":/icons/Tree_Part.svg"))
            self.listWidget.addItem(i)
        for o in self.obj.Additions:
            i = QtGui.QListWidgetItem(self.listWidget)
            i.setText(o.Name)
            i.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
            self.listWidget.addItem(i)
        for o in self.obj.Subtractions:
            i = QtGui.QListWidgetItem(self.listWidget)
            i.setText(o.Name)
            i.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
            self.listWidget.addItem(i)

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Components", None, QtGui.QApplication.UnicodeUTF8))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Append selected objects", None, QtGui.QApplication.UnicodeUTF8))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove child", None, QtGui.QApplication.UnicodeUTF8))
        self.title.setText(QtGui.QApplication.translate("Arch", "Components of this object", None, QtGui.QApplication.UnicodeUTF8))
        
class Component:
    "The Component object"
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
    "A View Provider for Component objects"
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
        taskd.fillList()
        FreeCADGui.Control.showDialog(taskd)
        return True
    
    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return True
    

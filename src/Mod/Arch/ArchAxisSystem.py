#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD
import DraftGeomUtils
if FreeCAD.GuiUp:
    import FreeCADGui
    import Draft
    from PySide import QtCore, QtGui
    from draftutils.translate import translate
    from pivy import coin
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

__title__  = "FreeCAD Axis System"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

## @package ArchAxisSystem
#  \ingroup ARCH
#  \brief Axis system for the Arch workbench
#
#  This module provides tools to build axis systems
#  An axis system is a collection of multiple axes


def makeAxisSystem(axes,name=None):

    '''makeAxisSystem(axes,[name]): makes a system from the given list of axes'''

    if not isinstance(axes,list):
        axes = [axes]
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","AxisSystem")
    obj.Label = name if name else translate("Arch","Axis System")
    _AxisSystem(obj)
    obj.Axes = axes
    if FreeCAD.GuiUp:
        _ViewProviderAxisSystem(obj.ViewObject)
    FreeCAD.ActiveDocument.recompute()
    return obj


class _CommandAxisSystem:

    "the Arch Axis System command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Axis_System',
                'MenuText': QT_TRANSLATE_NOOP("Arch_AxisSystem","Axis System"),
                'Accel': "X, S",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_AxisSystem","Creates an axis system from a set of axes")}

    def Activated(self):

        if FreeCADGui.Selection.getSelection():
            s = "["
            for o in FreeCADGui.Selection.getSelection():
                if Draft.getType(o) != "Axis":
                    FreeCAD.Console.PrintError(translate("Arch","Only axes must be selected")+"\n")
                    return
                s += "FreeCAD.ActiveDocument."+o.Name+","
            s += "]"
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Axis System"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("Arch.makeAxisSystem("+s+")")
            FreeCAD.ActiveDocument.commitTransaction()
        else:
            FreeCAD.Console.PrintError(translate("Arch","Please select at least one axis")+"\n")

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None


class _AxisSystem:

    "The Axis System object"

    def __init__(self,obj):

        obj.Proxy = self
        self.setProperties(obj)

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Axes" in pl:
            obj.addProperty("App::PropertyLinkList","Axes","AxisSystem", QT_TRANSLATE_NOOP("App::Property","The axes this system is made of"))
        if not "Placement" in pl:
            obj.addProperty("App::PropertyPlacement","Placement","AxisSystem",QT_TRANSLATE_NOOP("App::Property","The placement of this axis system"))
        self.Type = "AxisSystem"

    def onDocumentRestored(self,obj):

        self.setProperties(obj)

    def execute(self,obj):

        pass

    def onBeforeChange(self,obj,prop):

        if prop == "Placement":
            self.Placement = obj.Placement

    def onChanged(self,obj,prop):

        if prop == "Placement":
            if hasattr(self,"Placement"):
                delta = obj.Placement.multiply(self.Placement.inverse())
                for o in obj.Axes:
                    o.Placement = delta.multiply(o.Placement)

    def dumps(self):

        return None

    def loads(self,state):

        return None

    def getPoints(self,obj):

        "returns the gridpoints of linked axes"

        pts = []
        if len(obj.Axes) == 1:
            for e in obj.Axes[0].Shape.Edges:
                pts.append(e.Vertexes[0].Point)
        elif len(obj.Axes) == 2:
            set1 = obj.Axes[0].Shape.Edges # X
            set2 = obj.Axes[1].Shape.Edges # Y
            for e1 in set1:
                for e2 in set2:
                    pts.extend(DraftGeomUtils.findIntersection(e1,e2))
        elif len(obj.Axes) == 3:
            set1 = obj.Axes[0].Shape.Edges # X
            set2 = obj.Axes[1].Shape.Edges # Y
            set3 = obj.Axes[2].Shape.Edges # Z
            bset = []
            cv = None
            for e1 in set1:
                for e2 in set2:
                    bset.extend(DraftGeomUtils.findIntersection(e1,e2))
            for e3 in set3:
                if not cv:
                    cv = e3.Vertexes[0].Point
                    pts.extend(bset)
                else:
                    v = e3.Vertexes[0].Point.sub(cv)
                    pts.extend([p.add(v) for p in bset])
        return pts

    def getAxisData(self,obj):
        data = []
        for axis in obj.Axes:
            if hasattr(axis,"Proxy") and hasattr(axis.Proxy,"getAxisData"):
                data.append(axis.Proxy.getAxisData(axis))
        return data


class _ViewProviderAxisSystem:

    "A View Provider for the Axis object"

    def __init__(self,vobj):

        vobj.Proxy = self

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Axis_System_Tree.svg"

    def claimChildren(self):

        if hasattr(self,"axes"):
            return self.axes
        return []

    def attach(self, vobj):
        self.Object = vobj.Object
        self.axes = vobj.Object.Axes
        vobj.addDisplayMode(coin.SoSeparator(),"Default")

    def getDisplayModes(self,vobj):

        return ["Default"]

    def getDefaultDisplayMode(self):

        return "Default"

    def setDisplayMode(self,mode):

        return mode

    def updateData(self,obj,prop):

        self.axes = obj.Axes

    def onChanged(self, vobj, prop):

        if prop == "Visibility":
            for o in vobj.Object.Axes:
                o.ViewObject.Visibility = vobj.Visibility

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None

        taskd = AxisSystemTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None

        FreeCADGui.Control.closeDialog()
        return True

    def doubleClicked(self, vobj):
        self.edit()

    def setupContextMenu(self, vobj, menu):
        actionEdit = QtGui.QAction(translate("Arch", "Edit"),
                                   menu)
        QtCore.QObject.connect(actionEdit,
                               QtCore.SIGNAL("triggered()"),
                               self.edit)
        menu.addAction(actionEdit)

    def edit(self):
        FreeCADGui.ActiveDocument.setEdit(self.Object, 0)

    def dumps(self):

        return None

    def loads(self,state):

        return None


class AxisSystemTaskPanel:

    '''A TaskPanel for all the section plane object'''

    def __init__(self,obj):

        self.obj = obj
        self.form = QtGui.QWidget()
        self.form.setObjectName("Axis System")
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

        self.delButton = QtGui.QPushButton(self.form)
        self.delButton.setObjectName("delButton")
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delButton, 3, 1, 1, 1)

        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        self.update()

    def isAllowedAlterSelection(self):

        return True

    def isAllowedAlterView(self):

        return True

    def getStandardButtons(self):

        return int(QtGui.QDialogButtonBox.Ok)

    def getIcon(self,obj):

        if hasattr(obj.ViewObject,"Proxy"):
            return QtGui.QIcon(obj.ViewObject.Proxy.getIcon())
        elif obj.isDerivedFrom("Sketcher::SketchObject"):
            return QtGui.QIcon(":/icons/Sketcher_Sketch.svg")
        elif obj.isDerivedFrom("App::DocumentObjectGroup"):
            return QtGui.QApplication.style().standardIcon(QtGui.QStyle.SP_DirIcon)
        elif hasattr(obj.ViewObject, "Icon"):
            return QtGui.QIcon(obj.ViewObject.Icon)
        return QtGui.QIcon(":/icons/Part_3D_object.svg")

    def update(self):

        self.tree.clear()
        if self.obj:
            for o in self.obj.Axes:
                item = QtGui.QTreeWidgetItem(self.tree)
                item.setText(0,o.Label)
                item.setToolTip(0,o.Name)
                item.setIcon(0,self.getIcon(o))
        self.retranslateUi(self.form)

    def addElement(self):

        if self.obj:
            for o in FreeCADGui.Selection.getSelection():
                if not(o in self.obj.Axes) and (o != self.obj):
                    g = self.obj.Axes
                    g.append(o)
                    self.obj.Axes = g
            self.update()

    def removeElement(self):

        if self.obj:
            it = self.tree.currentItem()
            if it:
                o = FreeCAD.ActiveDocument.getObject(str(it.toolTip(0)))
                if o in self.obj.Axes:
                    g = self.obj.Axes
                    g.remove(o)
                    self.obj.Axes = g
            self.update()

    def accept(self):

        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel):

        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Axes", None))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None))
        self.title.setText(QtGui.QApplication.translate("Arch", "Axis system components", None))

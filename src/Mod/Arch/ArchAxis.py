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

import FreeCAD,FreeCADGui,Draft,math,DraftVecUtils,ArchCommands
from FreeCAD import Vector
from PyQt4 import QtCore, QtGui
from pivy import coin
from DraftTools import translate

__title__="FreeCAD Axis System"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeAxis(num=5,size=1,name=str(translate("Arch","Axes"))):
    '''makeAxis(num,size): makes an Axis System
    based on the given number of axes and interval distances'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Axis(obj)
    _ViewProviderAxis(obj.ViewObject)
    if num:
        dist = []
        angles = []
        for i in range(num):
            dist.append(float(size))
            angles.append(float(0))
        obj.Distances = dist
        obj.Angles = angles
    FreeCAD.ActiveDocument.recompute()
    return obj

class _CommandAxis:
    "the Arch Axis command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Axis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Axis","Axis"),
                'Accel': "A, X",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Axis","Creates an axis system.")}
        
    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Axis")))
        FreeCADGui.doCommand("import Arch")
        sel = FreeCADGui.Selection.getSelection()
        st = Draft.getObjectsOfType(sel,"Structure")
        if st:
            FreeCADGui.doCommand("axe = Arch.makeAxis()")
            FreeCADGui.doCommand("Arch.makeStructuralSystem(" + ArchCommands.getStringList(st) + ",[axe])")
        else:
            FreeCADGui.doCommand("Arch.makeAxis()")
        FreeCAD.ActiveDocument.commitTransaction()
       
class _Axis:
    "The Axis object"
    def __init__(self,obj):
        obj.addProperty("App::PropertyFloatList","Distances","Base", str(translate("Arch","The intervals between axes")))
        obj.addProperty("App::PropertyFloatList","Angles","Base", str(translate("Arch","The angles of each axis")))
        obj.addProperty("App::PropertyFloat","Length","Base", str(translate("Arch","The length of the axes")))
        self.Type = "Axis"
        obj.Length=1.0
        obj.Proxy = self
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        if not prop in ["Shape","Placement"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):
        import Part
        pl = obj.Placement
        geoms = []
        dist = 0
        if obj.Distances:
            if len(obj.Distances) == len(obj.Angles):
                for i in range(len(obj.Distances)):
                    dist += obj.Distances[i]
                    ang = math.radians(obj.Angles[i])
                    p1 = Vector(dist,0,0)
                    p2 = Vector(dist+(obj.Length/math.cos(ang))*math.sin(ang),obj.Length,0)
                    geoms.append(Part.Line(p1,p2).toShape())
        if geoms:
            obj.Shape = Part.Compound(geoms)
        obj.Placement = pl

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state
        
class _ViewProviderAxis:
    "A View Provider for the Axis object"

    def __init__(self,vobj):
        vobj.addProperty("App::PropertyLength","BubbleSize","Base", str(translate("Arch","The size of the axis bubbles")))
        vobj.addProperty("App::PropertyEnumeration","NumerationStyle","Base", str(translate("Arch","The numeration style")))
        vobj.NumerationStyle = ["1,2,3","01,02,03","001,002,003","A,B,C","a,b,c","I,II,III","L0,L1,L2"]
        vobj.Proxy = self
        vobj.BubbleSize = .1
        vobj.LineWidth = 1
        vobj.LineColor = (0.13,0.15,0.37)
        vobj.DrawStyle = "Dashdot"
    
    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Axis_Tree.svg"

    def claimChildren(self):
        return []

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.bubbles = None

    def getNumber(self,num):
        chars = "abcdefghijklmnopqrstuvwxyz"
        roman=(('M',1000),('CM',900),('D',500),('CD',400),
               ('C',100),('XC',90),('L',50),('XL',40),
               ('X',10),('IX',9),('V',5),('IV',4),('I',1))
        if self.ViewObject.NumerationStyle == "1,2,3":
            return str(num+1)
        elif self.ViewObject.NumerationStyle == "01,02,03":
            return str(num+1).zfill(2)
        elif self.ViewObject.NumerationStyle == "001,002,003":
            return str(num+1).zfill(3)
        elif self.ViewObject.NumerationStyle == "A,B,C":
            result = ""
            base = num/26
            if base:
                result += chars[base].upper()
            remainder = num % 26
            result += chars[remainder].upper()
            return result
        elif self.ViewObject.NumerationStyle == "a,b,c":
            result = ""
            base = num/26
            if base:
                result += chars[base]
            remainder = num % 26
            result += chars[remainder]
            return result
        elif self.ViewObject.NumerationStyle == "I,II,III":
            result = ""
            num += 1
            for numeral, integer in roman:
                while num >= integer:
                    result += numeral
                    num -= integer
            return result
        elif self.ViewObject.NumerationStyle == "L0,L1,L2":
            return "L"+str(num)
        return ""

    def makeBubbles(self):
        import Part

        def getNode():
            # make sure we already have the complete node built
            r = self.ViewObject.RootNode
            if r.getChildren().getLength() > 2:
                if r.getChild(2).getChildren().getLength() > 0:
                    if r.getChild(2).getChild(0).getChildren().getLength() > 0:
                        return self.ViewObject.RootNode.getChild(2).getChild(0).getChild(0)
            return None
        
        rn = getNode()
        if rn:
            if self.bubbles:
                rn.removeChild(self.bubbles)
                self.bubbles = None
            self.bubbles = coin.SoSeparator()
            isep = coin.SoSeparator()
            self.bubblestyle = coin.SoDrawStyle()
            self.bubblestyle.linePattern = 0xffff
            self.bubbles.addChild(self.bubblestyle)
            for i in range(len(self.ViewObject.Object.Distances)):
                invpl = self.ViewObject.Object.Placement.inverse()
                verts = self.ViewObject.Object.Shape.Edges[i].Vertexes
                p1 = invpl.multVec(verts[0].Point)
                p2 = invpl.multVec(verts[1].Point)
                dv = p2.sub(p1)
                dv.normalize()
                rad = self.ViewObject.BubbleSize
                center = p2.add(dv.scale(rad,rad,rad))
                ts = Part.makeCircle(rad,center).writeInventor()
                cin = coin.SoInput()
                cin.setBuffer(ts)
                cob = coin.SoDB.readAll(cin)
                co = cob.getChild(1).getChild(0).getChild(2)
                li = cob.getChild(1).getChild(0).getChild(3)
                self.bubbles.addChild(co)
                self.bubbles.addChild(li)
                st = coin.SoSeparator()
                tr = coin.SoTransform()
                tr.translation.setValue((center.x,center.y-rad/4,center.z))
                fo = coin.SoFont()
                fo.name = "Arial,Sans"
                fo.size = rad*100
                tx = coin.SoText2()
                tx.justification = coin.SoText2.CENTER
                tx.string = self.getNumber(i)
                st.addChild(tr)
                st.addChild(fo)
                st.addChild(tx)
                isep.addChild(st)
            self.bubbles.addChild(isep)
            rn.addChild(self.bubbles)

    def updateData(self, obj, prop):
        if prop == "Shape":
            self.makeBubbles()
        return

    def onChanged(self, vobj, prop):
        if prop in ["NumerationStyle","BubbleSize"]:
            self.makeBubbles()
        elif prop == "LineWidth":
            if self.bubbles:
                self.bubblestyle.lineWidth = vobj.LineWidth
        return
  
    def setEdit(self,vobj,mode):
        taskd = _AxisTaskPanel()
        taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True
    
    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class _AxisTaskPanel:
    '''The editmode TaskPanel for Axis objects'''
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
        self.tree.setColumnCount(3)
        self.tree.header().resizeSection(0,50)
        self.tree.header().resizeSection(1,80)
        self.tree.header().resizeSection(2,60)
        
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
        self.update()

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)
    
    def update(self):
        'fills the treewidget'
        self.tree.clear()
        if self.obj:
            for i in range(len(self.obj.Distances)):
                item = QtGui.QTreeWidgetItem(self.tree)
                item.setText(0,str(i+1))
                item.setText(1,str(self.obj.Distances[i]))
                item.setText(2,str(self.obj.Angles[i]))
                item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
                item.setTextAlignment(0,QtCore.Qt.AlignLeft)
        self.retranslateUi(self.form)
                
    def addElement(self):
        item = QtGui.QTreeWidgetItem(self.tree)
        item.setText(0,str(self.tree.topLevelItemCount()))
        item.setText(1,"1.0")
        item.setText(2,"0.0")
        item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
        self.resetObject()

    def removeElement(self):
        it = self.tree.currentItem()
        if it:
            nr = int(it.text(0))-1
            self.resetObject(remove=nr)
            self.update()

    def resetObject(self,remove=None):
        d = []
        a = []
        for i in range(self.tree.topLevelItemCount()):
            it = self.tree.findItems(str(i+1),QtCore.Qt.MatchExactly,0)[0]
            if (remove == None) or (remove != i):
                d.append(float(it.text(1)))
                a.append(float(it.text(2)))
        self.obj.Distances = d
        self.obj.Angles = a
        FreeCAD.ActiveDocument.recompute()
    
    def accept(self):
        self.resetObject()
        FreeCADGui.ActiveDocument.resetEdit()
                    
    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Axes", None, QtGui.QApplication.UnicodeUTF8))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None, QtGui.QApplication.UnicodeUTF8))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None, QtGui.QApplication.UnicodeUTF8))
        self.title.setText(QtGui.QApplication.translate("Arch", "Distances and angles between axes", None, QtGui.QApplication.UnicodeUTF8))
        self.tree.setHeaderLabels([QtGui.QApplication.translate("Arch", "Axis", None, QtGui.QApplication.UnicodeUTF8),
                                   QtGui.QApplication.translate("Arch", "Distance", None, QtGui.QApplication.UnicodeUTF8),
                                   QtGui.QApplication.translate("Arch", "Angle", None, QtGui.QApplication.UnicodeUTF8)])
          
FreeCADGui.addCommand('Arch_Axis',_CommandAxis())

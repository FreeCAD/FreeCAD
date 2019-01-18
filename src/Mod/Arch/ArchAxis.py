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

import FreeCAD,Draft,math,DraftVecUtils,ArchCommands,sys
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from pivy import coin
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

__title__="FreeCAD Axis System"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

## @package ArchAxis
#  \ingroup ARCH
#  \brief Axis system for the Arch workbench
#
#  This module provides tools to build axis systems
#  An axis system is a collection of planar axes with a number/tag


def makeAxis(num=5,size=1000,name="Axes"):

    '''makeAxis(num,size): makes an Axis set
    based on the given number of axes and interval distances'''

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Axis")
    obj.Label = translate("Arch",name)
    _Axis(obj)
    if FreeCAD.GuiUp:
        _ViewProviderAxis(obj.ViewObject)
    if num:
        dist = []
        angles = []
        for i in range(num):
            if i == 0:
                dist.append(0)
            else:
                dist.append(float(size))
            angles.append(float(0))
        obj.Distances = dist
        obj.Angles = angles
    FreeCAD.ActiveDocument.recompute()
    return obj


def makeAxisSystem(axes,name="Axis System"):

    '''makeAxisSystem(axes): makes a system from the given list of axes'''

    if not isinstance(axes,list):
        axes = [axes]
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","AxisSystem")
    obj.Label = translate("Arch",name)
    _AxisSystem(obj)
    obj.Axes = axes
    if FreeCAD.GuiUp:
        _ViewProviderAxisSystem(obj.ViewObject)
    FreeCAD.ActiveDocument.recompute()
    return obj


def makeGrid(name="Grid"):

    '''makeGrid(): makes a grid object'''

    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Grid")
    obj.Label = translate("Arch",name)
    ArchGrid(obj)
    if FreeCAD.GuiUp:
        ViewProviderArchGrid(obj.ViewObject)
        obj.ViewObject.Transparency = 85
    FreeCAD.ActiveDocument.recompute()
    return obj


class _CommandAxis:

    "the Arch Axis command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Axis',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Axis","Axis"),
                'Accel': "A, X",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Axis","Creates a set of axes")}

    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Axis"))
        FreeCADGui.addModule("Arch")

        FreeCADGui.doCommand("Arch.makeAxis()")
        FreeCAD.ActiveDocument.commitTransaction()

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None


class _CommandAxisSystem:

    "the Arch Axis System command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Axis_System',
                'MenuText': QT_TRANSLATE_NOOP("Arch_AxisSystem","Axis System"),
                'Accel': "X, S",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_AxisSystem","Creates an axis system from a set of axes")}

    def Activated(self):

        if FreeCADGui.Selection.getSelection():
            import Draft
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


class CommandArchGrid:

    "the Arch Grid command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Grid',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Axis","Grid"),
                'Accel': "A, X",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Axis","Creates a customizable grid object")}

    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Grid"))
        FreeCADGui.addModule("Arch")

        FreeCADGui.doCommand("Arch.makeGrid()")
        FreeCAD.ActiveDocument.commitTransaction()

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None



class _Axis:

    "The Axis object"

    def __init__(self,obj):

        obj.Proxy = self
        self.setProperties(obj)

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Distances" in pl:
            obj.addProperty("App::PropertyFloatList","Distances","Axis", QT_TRANSLATE_NOOP("App::Property","The intervals between axes"))
        if not "Angles" in pl:
            obj.addProperty("App::PropertyFloatList","Angles","Axis", QT_TRANSLATE_NOOP("App::Property","The angles of each axis"))
        if not "Labels" in pl:
            obj.addProperty("App::PropertyStringList","Labels","Axis", QT_TRANSLATE_NOOP("App::Property","The label of each axis"))
        if not "CustomNumber" in pl:
            obj.addProperty("App::PropertyString","CustomNumber","Axis", QT_TRANSLATE_NOOP("App::Property","An optional custom bubble number"))
        if not "Length" in pl:
            obj.addProperty("App::PropertyLength","Length","Axis", QT_TRANSLATE_NOOP("App::Property","The length of the axes"))
            obj.Length=3000
        if not "Placement" in pl:
            obj.addProperty("App::PropertyPlacement","Placement","Base","")
        if not "Shape" in pl:
            obj.addProperty("Part::PropertyPartShape","Shape","Base","")
        self.Type = "Axis"

    def onDocumentRestored(self,obj):

        self.setProperties(obj)

    def execute(self,obj):

        import Part
        geoms = []
        dist = 0
        if obj.Distances:
            if len(obj.Distances) == len(obj.Angles):
                for i in range(len(obj.Distances)):
                    if hasattr(obj.Length,"Value"):
                        l = obj.Length.Value
                    else:
                        l = obj.Length
                    dist += obj.Distances[i]
                    ang = math.radians(obj.Angles[i])
                    p1 = Vector(dist,0,0)
                    p2 = Vector(dist+(l/math.cos(ang))*math.sin(ang),l,0)
                    geoms.append(Part.LineSegment(p1,p2).toShape())
        if geoms:
            sh = Part.Compound(geoms)
            sh.Placement = obj.Placement
            obj.Shape = sh

    def onChanged(self,obj,prop):

        if prop in ["Angles","Distances","Placement"]:
            self.execute(obj)

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def getPoints(self,obj):

        "returns the gridpoints of linked axes"

        pts = []
        for e in obj.Shape.Edges:
            pts.append(e.Vertexes[0].Point)
        return pts

    def getAxisData(self,obj):
        data = []
        num = 0
        for e in obj.Shape.Edges:
            axdata = []
            axdata.append(e.Vertexes[0].Point)
            axdata.append(e.Vertexes[-1].Point)
            if obj.ViewObject:
                axdata.append(obj.ViewObject.Proxy.getNumber(obj.ViewObject,num))
            else:
                axdata.append(str(num))
            data.append(axdata)
            num += 1
        return data


class _ViewProviderAxis:

    "A View Provider for the Axis object"

    def __init__(self,vobj):

        vobj.Proxy = self
        self.setProperties(vobj)

    def setProperties(self,vobj):

        ts = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetFloat("textheight",350)
        pl = vobj.PropertiesList
        if not "BubbleSize" in pl:
            vobj.addProperty("App::PropertyLength","BubbleSize","Axis", QT_TRANSLATE_NOOP("App::Property","The size of the axis bubbles"))
            vobj.BubbleSize = ts*1.42
        if not "NumberingStyle" in pl:
            vobj.addProperty("App::PropertyEnumeration","NumberingStyle","Axis", QT_TRANSLATE_NOOP("App::Property","The numbering style"))
            vobj.NumberingStyle = ["1,2,3","01,02,03","001,002,003","A,B,C","a,b,c","I,II,III","L0,L1,L2"]
            vobj.NumberingStyle = "1,2,3"
        if not "DrawStyle" in pl:
            vobj.addProperty("App::PropertyEnumeration","DrawStyle","Axis",QT_TRANSLATE_NOOP("App::Property","The type of line to draw this axis"))
            vobj.DrawStyle = ["Solid","Dashed","Dotted","Dashdot"]
            vobj.DrawStyle = "Dashdot"
        if not "BubblePosition" in pl:
            vobj.addProperty("App::PropertyEnumeration","BubblePosition","Axis",QT_TRANSLATE_NOOP("App::Property","Where to add bubbles to this axis: Start, end, both or none"))
            vobj.BubblePosition = ["Start","End","Both","None"]
        if not "LineWidth" in pl:
            vobj.addProperty("App::PropertyFloat","LineWidth","Axis",QT_TRANSLATE_NOOP("App::Property","The line width to draw this axis"))
            vobj.LineWidth = 1
        if not "LineColor" in pl:
            vobj.addProperty("App::PropertyColor","LineColor","Axis",QT_TRANSLATE_NOOP("App::Property","The color of this axis"))
            vobj.LineColor = ArchCommands.getDefaultColor("Helpers")
        if not "StartNumber" in pl:
            vobj.addProperty("App::PropertyInteger","StartNumber","Axis",QT_TRANSLATE_NOOP("App::Property","The number of the first axis"))
            vobj.StartNumber = 1
        if not "FontName" in pl:
            vobj.addProperty("App::PropertyFont","FontName","Axis",QT_TRANSLATE_NOOP("App::Property","The font to use for texts"))
            vobj.FontName = Draft.getParam("textfont","Arial,Sans")
        if not "FontSize" in pl:
            vobj.addProperty("App::PropertyLength","FontSize","Axis",QT_TRANSLATE_NOOP("App::Property","The font size"))
            vobj.FontSize = ts
        if not "ShowLabel" in pl:
            vobj.addProperty("App::PropertyBool","ShowLabel","Axis",QT_TRANSLATE_NOOP("App::Property","If true, show the labels"))
        if not "LabelOffset" in pl:
            vobj.addProperty("App::PropertyPlacement","LabelOffset","Axis",QT_TRANSLATE_NOOP("App::Property","A transformation to apply to each label"))

    def onDocumentRestored(self,vobj):

        self.setProperties(vobj)

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Axis_Tree.svg"

    def claimChildren(self):

        return []

    def attach(self, vobj):

        self.bubbles = None
        self.bubbletexts = []
        sep = coin.SoSeparator()
        self.mat = coin.SoMaterial()
        self.linestyle = coin.SoDrawStyle()
        self.linecoords = coin.SoCoordinate3()
        self.lineset = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.bubbleset = coin.SoSeparator()
        self.labelset = coin.SoSeparator()
        sep.addChild(self.mat)
        sep.addChild(self.linestyle)
        sep.addChild(self.linecoords)
        sep.addChild(self.lineset)
        sep.addChild(self.bubbleset)
        sep.addChild(self.labelset)
        vobj.addDisplayMode(sep,"Default")
        self.onChanged(vobj,"BubbleSize")
        self.onChanged(vobj,"ShowLabel")
        self.onChanged(vobj,"LineColor")
        self.onChanged(vobj,"LineWidth")
        self.onChanged(vobj,"DrawStyle")

    def getDisplayModes(self,vobj):

        return ["Default"]

    def getDefaultDisplayMode(self):

        return "Default"

    def setDisplayMode(self,mode):

        return mode

    def updateData(self,obj,prop):

        if prop == "Shape":
            if obj.Shape:
                if obj.Shape.Edges:
                    verts = []
                    vset = []
                    i = 0
                    for e in obj.Shape.Edges:
                        for v in e.Vertexes:
                            verts.append([v.X,v.Y,v.Z])
                            vset.append(i)
                            i += 1
                        vset.append(-1)
                    self.linecoords.point.setValues(verts)
                    self.lineset.coordIndex.setValues(0,len(vset),vset)
                    self.lineset.coordIndex.setNum(len(vset))
            self.onChanged(obj.ViewObject,"BubbleSize")
            self.onChanged(obj.ViewObject,"ShowLabel")

    def onChanged(self, vobj, prop):

        if prop == "LineColor":
            if hasattr(vobj,"LineColor"):
                l = vobj.LineColor
                self.mat.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "DrawStyle":
            if hasattr(vobj,"DrawStyle"):
                if vobj.DrawStyle == "Solid":
                    self.linestyle.linePattern = 0xffff
                elif vobj.DrawStyle == "Dashed":
                    self.linestyle.linePattern = 0xf00f
                elif vobj.DrawStyle == "Dotted":
                    self.linestyle.linePattern = 0x0f0f
                else:
                    self.linestyle.linePattern = 0xff88
        elif prop == "LineWidth":
            if hasattr(vobj,"LineWidth"):
                self.linestyle.lineWidth = vobj.LineWidth
        elif prop in ["BubbleSize","BubblePosition","FontName","FontSize"]:
            if hasattr(self,"bubbleset"):
                if self.bubbles:
                    self.bubbleset.removeChild(self.bubbles)
                    self.bubbles = None
                if vobj.Object.Shape:
                    if vobj.Object.Shape.Edges:
                        self.bubbles = coin.SoSeparator()
                        self.bubblestyle = coin.SoDrawStyle()
                        self.bubblestyle.linePattern = 0xffff
                        self.bubbles.addChild(self.bubblestyle)
                        import Part,Draft
                        self.bubbletexts = []
                        pos = ["Start"]
                        if hasattr(vobj,"BubblePosition"):
                            if vobj.BubblePosition == "Both":
                                pos = ["Start","End"]
                            elif vobj.BubblePosition == "None":
                                pos = []
                            else:
                                pos = [vobj.BubblePosition]
                        for i in range(len(vobj.Object.Shape.Edges)):
                            for p in pos:
                                verts = vobj.Object.Shape.Edges[i].Vertexes
                                if p == "Start":
                                    p1 = verts[0].Point
                                    p2 = verts[1].Point
                                else:
                                    p1 = verts[1].Point
                                    p2 = verts[0].Point
                                dv = p2.sub(p1)
                                dv.normalize()
                                if hasattr(vobj.BubbleSize,"Value"):
                                    rad = vobj.BubbleSize.Value/2
                                else:
                                    rad = vobj.BubbleSize/2
                                center = p2.add(dv.scale(rad,rad,rad))
                                buf = Part.makeCircle(rad,center).writeInventor()
                                try:
                                    cin = coin.SoInput()
                                    cin.setBuffer(buf)
                                    cob = coin.SoDB.readAll(cin)
                                except:
                                    import re
                                    # workaround for pivy SoInput.setBuffer() bug
                                    buf = buf.replace("\n","")
                                    pts = re.findall("point \[(.*?)\]",buf)[0]
                                    pts = pts.split(",")
                                    pc = []
                                    for p in pts:
                                        v = p.strip().split()
                                        pc.append([float(v[0]),float(v[1]),float(v[2])])
                                    coords = coin.SoCoordinate3()
                                    coords.point.setValues(0,len(pc),pc)
                                    line = coin.SoLineSet()
                                    line.numVertices.setValue(-1)
                                else:
                                    coords = cob.getChild(1).getChild(0).getChild(2)
                                    line = cob.getChild(1).getChild(0).getChild(3)
                                self.bubbles.addChild(coords)
                                self.bubbles.addChild(line)
                                st = coin.SoSeparator()
                                tr = coin.SoTransform()
                                fs = rad*1.5
                                if hasattr(vobj,"FontSize"):
                                    fs = vobj.FontSize.Value
                                tr.translation.setValue((center.x,center.y-fs/2.5,center.z))
                                fo = coin.SoFont()
                                fn = Draft.getParam("textfont","Arial,Sans")
                                if hasattr(vobj,"FontName"):
                                    if vobj.FontName:
                                        try:
                                            fn = str(vobj.FontName)
                                        except:
                                            pass
                                fo.name = fn
                                fo.size = fs
                                tx = coin.SoAsciiText()
                                tx.justification = coin.SoText2.CENTER
                                self.bubbletexts.append(tx)
                                st.addChild(tr)
                                st.addChild(fo)
                                st.addChild(tx)
                                self.bubbles.addChild(st)
                        self.bubbleset.addChild(self.bubbles)
                        self.onChanged(vobj,"NumberingStyle")
            if prop in ["FontName","FontSize"]:
                self.onChanged(vobj,"ShowLabel")
        elif prop in ["NumberingStyle","StartNumber"]:
            if hasattr(self,"bubbletexts"):
                num = 0
                if hasattr(vobj,"StartNumber"):
                    if vobj.StartNumber > 1:
                        num = vobj.StartNumber-1
                alt = False
                for t in self.bubbletexts:
                    t.string = self.getNumber(vobj,num)
                    num += 1
                    if hasattr(vobj,"BubblePosition"):
                        if vobj.BubblePosition == "Both":
                            if not alt:
                                num -= 1
                    alt = not alt
        elif prop in ["ShowLabel", "LabelOffset"]:
            if hasattr(self,"labels"):
                if self.labels:
                    self.labelset.removeChild(self.labels)
            self.labels = None
            if hasattr(vobj,"ShowLabel") and hasattr(vobj.Object,"Labels"):
                if vobj.ShowLabel:
                    self.labels = coin.SoSeparator()
                    for i in range(len(vobj.Object.Shape.Edges)):
                        if len(vobj.Object.Labels) > i:
                            if vobj.Object.Labels[i]:
                                import Draft
                                vert = vobj.Object.Shape.Edges[i].Vertexes[0].Point
                                if hasattr(vobj,"LabelOffset"):
                                    pl = FreeCAD.Placement(vobj.LabelOffset)
                                    pl.Base = vert.add(pl.Base)
                                st = coin.SoSeparator()
                                tr = coin.SoTransform()
                                fo = coin.SoFont()
                                tx = coin.SoAsciiText()
                                tx.justification = coin.SoText2.LEFT
                                t = vobj.Object.Labels[i]
                                if sys.version_info.major < 3 and isinstance(t,unicode):
                                    t = t.encode("utf8")
                                tx.string.setValue(t)
                                if hasattr(vobj,"FontSize"):
                                    fs = vobj.FontSize.Value
                                elif hasattr(vobj.BubbleSize,"Value"):
                                    fs = vobj.BubbleSize.Value*0.75
                                else:
                                    fs = vobj.BubbleSize*0.75
                                tr.translation.setValue(tuple(pl.Base))
                                tr.rotation.setValue(pl.Rotation.Q)
                                fn = Draft.getParam("textfont","Arial,Sans")
                                if hasattr(vobj,"FontName"):
                                    if vobj.FontName:
                                        try:
                                            fn = str(vobj.FontName)
                                        except:
                                            pass
                                fo.name = fn
                                fo.size = fs
                                st.addChild(tr)
                                st.addChild(fo)
                                st.addChild(tx)
                                self.labels.addChild(st)
                    self.labelset.addChild(self.labels)

    def getNumber(self,vobj,num):

        chars = "abcdefghijklmnopqrstuvwxyz"
        roman=(('M',1000),('CM',900),('D',500),('CD',400),
               ('C',100),('XC',90),('L',50),('XL',40),
               ('X',10),('IX',9),('V',5),('IV',4),('I',1))
        if hasattr(vobj.Object,"CustomNumber") and vobj.Object.CustomNumber:
            if sys.version_info.major < 3:
                return vobj.Object.CustomNumber.encode("utf8")
            else:
                return vobj.Object.CustomNumber
        elif hasattr(vobj,"NumberingStyle"):
            if vobj.NumberingStyle == "1,2,3":
                return str(num+1)
            elif vobj.NumberingStyle == "01,02,03":
                return str(num+1).zfill(2)
            elif vobj.NumberingStyle == "001,002,003":
                return str(num+1).zfill(3)
            elif vobj.NumberingStyle == "A,B,C":
                result = ""
                base = num//26
                if base:
                    result += chars[base].upper()
                remainder = num % 26
                result += chars[remainder].upper()
                return result
            elif vobj.NumberingStyle == "a,b,c":
                result = ""
                base = num//26
                if base:
                    result += chars[base]
                remainder = num % 26
                result += chars[remainder]
                return result
            elif vobj.NumberingStyle == "I,II,III":
                result = ""
                n = num
                n += 1
                for numeral, integer in roman:
                    while n >= integer:
                        result += numeral
                        n -= integer
                return result
            elif vobj.NumberingStyle == "L0,L1,L2":
                return "L"+str(num)
        else:
            return str(num+1)

    def setEdit(self,vobj,mode=0):

        taskd = _AxisTaskPanel()
        taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode):

        FreeCADGui.Control.closeDialog()
        return

    def doubleClicked(self,vobj):

        self.setEdit(vobj)

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

        self.updating = False

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
        self.tree.setColumnCount(4)
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
        QtCore.QObject.connect(self.tree, QtCore.SIGNAL("itemChanged(QTreeWidgetItem *, int)"), self.edit)
        self.update()

    def isAllowedAlterSelection(self):

        return False

    def isAllowedAlterView(self):

        return True

    def getStandardButtons(self):

        return int(QtGui.QDialogButtonBox.Close)

    def update(self):

        'fills the treewidget'
        self.updating = True
        self.tree.clear()
        if self.obj:
            for i in range(len(self.obj.Distances)):
                item = QtGui.QTreeWidgetItem(self.tree)
                item.setText(0,str(i+1))
                if len(self.obj.Distances) > i:
                    item.setText(1,str(self.obj.Distances[i]))
                if len(self.obj.Angles) > i:
                    item.setText(2,str(self.obj.Angles[i]))
                if hasattr(self.obj,"Labels"):
                    if len(self.obj.Labels) > i:
                        item.setText(3,str(self.obj.Labels[i]))
                item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
                item.setTextAlignment(0,QtCore.Qt.AlignLeft)
        self.retranslateUi(self.form)
        self.updating = False

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

    def edit(self,item,column):

        if not self.updating:
            self.resetObject()

    def resetObject(self,remove=None):

        "transfers the values from the widget to the object"

        d = []
        a = []
        l = []
        for i in range(self.tree.topLevelItemCount()):
            it = self.tree.findItems(str(i+1),QtCore.Qt.MatchExactly,0)[0]
            if (remove == None) or (remove != i):
                if it.text(1):
                    d.append(float(it.text(1)))
                else:
                    d.append(0.0)
                if it.text(2):
                    a.append(float(it.text(2)))
                else:
                    a.append(0.0)
                l.append(it.text(3))
        self.obj.Distances = d
        self.obj.Angles = a
        self.obj.Labels = l
        self.obj.touch()
        FreeCAD.ActiveDocument.recompute()

    def reject(self):

        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel):

        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Axes", None))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None))
        self.title.setText(QtGui.QApplication.translate("Arch", "Distances (mm) and angles (deg) between axes", None))
        self.tree.setHeaderLabels([QtGui.QApplication.translate("Arch", "Axis", None),
                                   QtGui.QApplication.translate("Arch", "Distance", None),
                                   QtGui.QApplication.translate("Arch", "Angle", None),
                                   QtGui.QApplication.translate("Arch", "Label", None)])


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

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def getPoints(self,obj):

        "returns the gridpoints of linked axes"

        import DraftGeomUtils
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

    def setEdit(self,vobj,mode=0):

        taskd = AxisSystemTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode):

        FreeCADGui.Control.closeDialog()
        return

    def doubleClicked(self,vobj):

        self.setEdit(vobj)

    def __getstate__(self):

        return None

    def __setstate__(self,state):

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
        else:
            return QtGui.QIcon(":/icons/Tree_Part.svg")

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
                if (not(o in self.obj.Axes)) and (o != self.obj):
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


class ArchGrid:

    "The Grid object"

    def __init__(self,obj):

        obj.Proxy = self
        self.setProperties(obj)

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Rows" in pl:
            obj.addProperty("App::PropertyInteger","Rows","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The number of rows'))
        if not "Columns" in pl:
            obj.addProperty("App::PropertyInteger","Columns","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The number of columns'))
        if not "RowSize" in pl:
            obj.addProperty("App::PropertyFloatList","RowSize","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The sizes for rows'))
        if not "ColumnSize" in pl:
            obj.addProperty("App::PropertyFloatList","ColumnSize","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The sizes of columns'))
        if not "Spans" in pl:
            obj.addProperty("App::PropertyStringList","Spans","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The span ranges of cells that are merged together'))
        if not "PointsOutput" in pl:
            obj.addProperty("App::PropertyEnumeration","PointsOutput","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The type of 3D points produced by this grid object'))
            obj.PointsOutput = ["Vertices","Edges","Vertical Edges","Horizontal Edges","Faces"]
        if not "Width" in pl:
            obj.addProperty("App::PropertyLength","Width","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The total width of this grid'))
        if not "Height" in pl:
            obj.addProperty("App::PropertyLength","Height","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The total height of this grid'))
        if not "AutoWidth" in pl:
            obj.addProperty("App::PropertyLength","AutoWidth","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'Creates automatic column divisions (set to 0 to disable)'))
        if not "AutoHeight" in pl:
            obj.addProperty("App::PropertyLength","AutoHeight","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'Creates automatic row divisions (set to 0 to disable)'))
        if not "Reorient" in pl:
            obj.addProperty("App::PropertyBool","Reorient","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'When in edge midpoint mode, if this grid must reorient its children along edge normals or not'))
        if not "HiddenFaces" in pl:
            obj.addProperty("App::PropertyIntegerList","HiddenFaces","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The indices of faces to hide'))
        self.Type = "Grid"

    def onDocumentRestored(self,obj):

        self.setProperties(obj)

    def getSizes(self,obj):

        "returns rowsizes,columnsizes,spangroups"

        if not obj.Height.Value:
            return [],[],[]
        if not obj.Width.Value:
            return [],[],[]
        if (not obj.Rows) and (not obj.AutoHeight.Value):
            return [],[],[]
        if (not obj.Columns) and (not obj.AutoWidth.Value):
            return [],[],[]
        # rescale rows
        rowsizes = []
        if obj.AutoHeight.Value:
            if obj.AutoHeight.Value > obj.Height.Value:
                FreeCAD.Console.PrintError(translate("Arch","Auto height is larger than height"))
                return [],[],[]
            rows = int(math.floor(obj.Height.Value/obj.AutoHeight.Value))
            for i in range(rows):
                rowsizes.append(obj.AutoHeight.Value)
            rowsizes.append(obj.Height.Value-rows*obj.AutoHeight.Value)
        else:
            reserved_rowsize = sum(v for v in obj.RowSize)
            if reserved_rowsize > obj.Height.Value:
                FreeCAD.Console.PrintError(translate("Arch","Total row size is larger than height"))
                return [],[],[]
            for i in range(obj.Rows):
                v = 0
                if i < len(obj.RowSize):
                    v = obj.RowSize[i]
                rowsizes.append(v)
            e = len([v for v in rowsizes if v == 0])
            default = obj.Height.Value - reserved_rowsize
            if e:
                default = default / e
            t = []
            for v in rowsizes:
                if v:
                    t.append(v)
                else:
                    t.append(default)
            rowsizes = t
        # rescale columns
        columnsizes = []
        if obj.AutoWidth.Value:
            if obj.AutoWidth.Value > obj.Width.Value:
                FreeCAD.Console.PrintError(translate("Arch","Auto width is larger than width"))
                return [],[],[]
            cols = int(math.floor(obj.Width.Value/obj.AutoWidth.Value))
            for i in range(cols):
                columnsizes.append(obj.AutoWidth.Value)
            columnsizes.append(obj.Width.Value-cols*obj.AutoWidth.Value)
        else:
            reserved_columnsize = sum(v for v in obj.ColumnSize)
            if reserved_columnsize > obj.Width.Value:
                FreeCAD.Console.PrintError(translate("Arch","Total column size is larger than width"))
                return [],[],[]
            for i in range(obj.Columns):
                v = 0
                if i < len(obj.ColumnSize):
                    v = obj.ColumnSize[i]
                columnsizes.append(v)
            e = len([v for v in columnsizes if v == 0])
            default = obj.Width.Value - reserved_columnsize
            if e:
                default = default / e
            t = []
            for v in columnsizes:
                if v:
                    t.append(v)
                else:
                    t.append(default)
            columnsizes = t
        # format span groups from [row,col,rowspan,colspan] to [faceindexes]
        spangroups = []
        for s in obj.Spans:
            nspan = []
            span = [int(i.strip()) for i in s.split(",")]
            for row in range(span[2]):
                for column in range(span[3]):
                    nspan.append((span[0]+row)*obj.Columns + (span[1]+column))
            spangroups.append(nspan)
        return rowsizes,columnsizes,spangroups

    def execute(self,obj):

        pl = obj.Placement
        import Part
        rowsizes,columnsizes,spangroups = self.getSizes(obj)
        #print rowsizes,columnsizes,spangroups
        # create one face for each cell
        faces = []
        facenumber = 0
        rowoffset = 0
        for row in rowsizes:
            columnoffset = 0
            for column in columnsizes:
                v1 = FreeCAD.Vector(columnoffset,rowoffset,0)
                v2 = v1.add(FreeCAD.Vector(column,0,0))
                v3 = v2.add(FreeCAD.Vector(0,-row,0))
                v4 = v3.add(FreeCAD.Vector(-column,0,0))
                f = Part.Face(Part.makePolygon([v1,v2,v3,v4,v1]))
                if not facenumber in obj.HiddenFaces:
                    spanning = False
                    for i in range(len(spangroups)):
                        if facenumber in spangroups[i]:
                            g = spangroups[i]
                            g[g.index(facenumber)] = f
                            spangroups[i] = g
                            spanning = True
                            break
                    if not spanning:
                        faces.append(f)
                facenumber += 1
                columnoffset += column
            rowoffset -= row
        # join spangroups
        for g in spangroups:
            s = Part.makeShell(g)
            s = s.removeSplitter()
            faces.extend(s.Faces)
        if faces:
            obj.Shape = Part.makeCompound(faces)
            obj.Placement = pl

    def getPoints(self,obj):

        "returns the gridpoints"

        def remdupes(pts):
            # eliminate possible duplicates
            ret = []
            for p in pts:
                if not p in ret:
                    ret.append(p)
            return ret
        if obj.PointsOutput == "Vertices":
            return [v.Point for v in obj.Shape.Vertexes]
        elif obj.PointsOutput == "Edges":
            return remdupes([e.CenterOfMass for e in obj.Shape.Edges])
        elif obj.PointsOutput == "Vertical Edges":
            rv = obj.Placement.Rotation.multVec(FreeCAD.Vector(0,1,0))
            edges = [e for e in obj.Shape.Edges if round(rv.getAngle(e.tangentAt(e.FirstParameter)),4) in [0,3.1416]]
            return remdupes([e.CenterOfMass for e in edges])
        elif obj.PointsOutput == "Horizontal Edges":
            rv = obj.Placement.Rotation.multVec(FreeCAD.Vector(1,0,0))
            edges = [e for e in obj.Shape.Edges if round(rv.getAngle(e.tangentAt(e.FirstParameter)),4) in [0,3.1416]]
            return remdupes([e.CenterOfMass for e in edges])
        else:
            return [f.CenterOfMass for f in obj.Shape.Faces]

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None


class ViewProviderArchGrid:

    "A View Provider for the Arch Grid"

    def __init__(self,vobj):

        vobj.Proxy = self

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Grid.svg"

    def setEdit(self,vobj,mode=0):

        taskd = ArchGridTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode):

        FreeCADGui.Control.closeDialog()
        return

    def doubleClicked(self,vobj):

        self.setEdit(vobj)

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None


class ArchGridTaskPanel:

    '''A TaskPanel for the Arch Grid'''

    def __init__(self,obj):

        # length, width, label
        self.width = 0
        self.height = 0
        self.spans = []
        self.obj = obj
        self.form = QtGui.QWidget()
        uil = FreeCADGui.UiLoader()
        layout = QtGui.QVBoxLayout(self.form)
        hbox3 = QtGui.QHBoxLayout()
        layout.addLayout(hbox3)
        self.wLabel = QtGui.QLabel(self.form)
        hbox3.addWidget(self.wLabel)
        self.widthUi = uil.createWidget("Gui::InputField")
        hbox3.addWidget(self.widthUi)
        hbox4 = QtGui.QHBoxLayout()
        layout.addLayout(hbox4)
        self.hLabel = QtGui.QLabel(self.form)
        hbox4.addWidget(self.hLabel)
        self.heightUi = uil.createWidget("Gui::InputField")
        hbox4.addWidget(self.heightUi)
        self.title = QtGui.QLabel(self.form)
        layout.addWidget(self.title)

        # grid
        self.table = QtGui.QTableWidget(self.form)
        layout.addWidget(self.table)
        style = "QTableWidget { background-color: #ffffff; gridline-color: #000000; }"
        self.table.setStyleSheet(style)
        self.table.horizontalHeader().setResizeMode(QtGui.QHeaderView.Stretch)

        # row/column buttons
        hbox1 = QtGui.QHBoxLayout()
        layout.addLayout(hbox1)
        self.addRowButton = QtGui.QPushButton(self.form)
        self.addRowButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        hbox1.addWidget(self.addRowButton)
        self.delRowButton = QtGui.QPushButton(self.form)
        self.delRowButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        hbox1.addWidget(self.delRowButton)
        self.addColumnButton = QtGui.QPushButton(self.form)
        self.addColumnButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        hbox1.addWidget(self.addColumnButton)
        self.delColumnButton = QtGui.QPushButton(self.form)
        self.delColumnButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        hbox1.addWidget(self.delColumnButton)

        # span buttons
        import SpreadsheetGui
        hbox2 = QtGui.QHBoxLayout()
        layout.addLayout(hbox2)
        self.spanButton = QtGui.QPushButton(self.form)
        self.spanButton.setIcon(QtGui.QIcon(":/icons/SpreadsheetMergeCells.svg"))
        hbox2.addWidget(self.spanButton)
        self.spanButton.setEnabled(False)
        self.delSpanButton = QtGui.QPushButton(self.form)
        self.delSpanButton.setIcon(QtGui.QIcon(":/icons/SpreadsheetSplitCell.svg"))
        hbox2.addWidget(self.delSpanButton)
        self.delSpanButton.setEnabled(False)

        #signals
        QtCore.QObject.connect(self.widthUi,QtCore.SIGNAL("valueChanged(double)"),self.setWidth)
        QtCore.QObject.connect(self.heightUi,QtCore.SIGNAL("valueChanged(double)"),self.setHeight)
        QtCore.QObject.connect(self.table, QtCore.SIGNAL("itemSelectionChanged()"), self.checkSpan)
        QtCore.QObject.connect(self.addRowButton, QtCore.SIGNAL("clicked()"), self.addRow)
        QtCore.QObject.connect(self.delRowButton, QtCore.SIGNAL("clicked()"), self.delRow)
        QtCore.QObject.connect(self.addColumnButton, QtCore.SIGNAL("clicked()"), self.addColumn)
        QtCore.QObject.connect(self.delColumnButton, QtCore.SIGNAL("clicked()"), self.delColumn)
        QtCore.QObject.connect(self.spanButton, QtCore.SIGNAL("clicked()"), self.addSpan)
        QtCore.QObject.connect(self.delSpanButton, QtCore.SIGNAL("clicked()"), self.removeSpan)
        QtCore.QObject.connect(self.table.horizontalHeader(),QtCore.SIGNAL("sectionDoubleClicked(int)"), self.editHorizontalHeader)
        QtCore.QObject.connect(self.table.verticalHeader(),QtCore.SIGNAL("sectionDoubleClicked(int)"), self.editVerticalHeader)
        self.update()
        self.retranslateUi()

    def retranslateUi(self,widget=None):

        self.form.setWindowTitle(QtGui.QApplication.translate("Arch", "Grid", None))
        self.wLabel.setText(QtGui.QApplication.translate("Arch", "Total width", None))
        self.hLabel.setText(QtGui.QApplication.translate("Arch", "Total height", None))
        self.addRowButton.setText(QtGui.QApplication.translate("Arch", "Add row", None))
        self.delRowButton.setText(QtGui.QApplication.translate("Arch", "Del row", None))
        self.addColumnButton.setText(QtGui.QApplication.translate("Arch", "Add col", None))
        self.delColumnButton.setText(QtGui.QApplication.translate("Arch", "Del col", None))
        self.spanButton.setText(QtGui.QApplication.translate("Arch", "Create span", None))
        self.delSpanButton.setText(QtGui.QApplication.translate("Arch", "Remove span", None))
        self.title.setText(QtGui.QApplication.translate("Arch", "Rows", None)+": "+str(self.table.rowCount())+" / "+QtGui.QApplication.translate("Arch", "Columns", None)+": "+str(self.table.columnCount()))

    def update(self):

        self.table.clear()
        if self.obj.Rows:
            self.table.setRowCount(self.obj.Rows)
            vlabels = ["0" for i in range(self.obj.Rows)]
            for i,v in enumerate(self.obj.RowSize):
                if i < len(vlabels):
                    vlabels[i] = FreeCAD.Units.Quantity(v,FreeCAD.Units.Length).getUserPreferred()[0]
            self.table.setVerticalHeaderLabels(vlabels)
        if self.obj.Columns:
            self.table.setColumnCount(self.obj.Columns)
            hlabels = ["0" for i in range(self.obj.Columns)]
            for i,v in enumerate(self.obj.ColumnSize):
                if i < len(hlabels):
                    hlabels[i] = FreeCAD.Units.Quantity(v,FreeCAD.Units.Length).getUserPreferred()[0]
            self.table.setHorizontalHeaderLabels(hlabels)
        self.widthUi.setText(self.obj.Width.getUserPreferred()[0])
        self.heightUi.setText(self.obj.Height.getUserPreferred()[0])
        self.spans = []
        for s in self.obj.Spans:
            span = [int(i.strip()) for i in s.split(",")]
            if len(span) == 4:
                self.table.setSpan(span[0],span[1],span[2],span[3])
                self.spans.append(span)

    def checkSpan(self):

        if self.table.selectedRanges():
            self.spanButton.setEnabled(False)
            self.delSpanButton.setEnabled(False)
            if len(self.table.selectedRanges()) > 1:
                self.spanButton.setEnabled(True)
            for r in self.table.selectedRanges():
                if (r.rowCount() * r.columnCount()) > 1:
                    self.spanButton.setEnabled(True)
                elif (r.rowCount() * r.columnCount()) == 1:
                    if self.table.rowSpan(r.topRow(),r.leftColumn()) > 1 \
                    or self.table.columnSpan(r.topRow(),r.leftColumn()) > 1:
                        self.delSpanButton.setEnabled(True)
        else:
            self.spanButton.setEnabled(False)
            self.delSpanButton.setEnabled(False)

    def addRow(self):

        c = self.table.currentRow()
        self.table.insertRow(c+1)
        self.table.setVerticalHeaderItem(c+1,QtGui.QTableWidgetItem("0"))
        self.retranslateUi()

    def delRow(self):

        if self.table.selectedRanges():
            self.table.removeRow(self.table.currentRow())
            self.retranslateUi()

    def addColumn(self):

        c = self.table.currentColumn()
        self.table.insertColumn(c+1)
        self.table.setHorizontalHeaderItem(c+1,QtGui.QTableWidgetItem("0"))
        self.retranslateUi()

    def delColumn(self):

        if self.table.selectedRanges():
            self.table.removeColumn(self.table.currentColumn())
            self.retranslateUi()

    def addSpan(self):

        for r in self.table.selectedRanges():
            if r.rowCount() * r.columnCount() > 1:
                self.table.setSpan(r.topRow(),r.leftColumn(),r.rowCount(),r.columnCount())
                self.spans.append([r.topRow(),r.leftColumn(),r.rowCount(),r.columnCount()])
                return
        if len(self.table.selectedRanges()) > 1:
            tr = 99999
            br = 0
            lc = 99999
            rc = 0
            for r in self.table.selectedRanges():
                if r.topRow() < tr:
                    tr = r.topRow()
                if r.bottomRow() > br:
                    br = r.bottomRow()
                if r.leftColumn() < lc:
                    lc = r.leftColumn()
                if r.rightColumn() > rc:
                    rc = r.rightColumn()
            if (rc >= lc) and (br >= tr):
                self.table.setSpan(tr,lc,(br-tr)+1,(rc-lc)+1)
                self.spans.append([tr,lc,(br-tr)+1,(rc-lc)+1])

    def removeSpan(self):

        for r in self.table.selectedRanges():
            if r.rowCount() * r.columnCount() == 1:
                if self.table.rowSpan(r.topRow(),r.leftColumn()) > 1 \
                or self.table.columnSpan(r.topRow(),r.leftColumn()) > 1:
                    self.table.setSpan(r.topRow(),r.leftColumn(),1,1)
                    f = None
                    for i,s in enumerate(self.spans):
                        if (s[0] == r.topRow()) and (s[1] == r.leftColumn()):
                            f = i
                            break
                    if f != None:
                        self.spans.pop(f)

    def editHorizontalHeader(self, index):

        val,ok = QtGui.QInputDialog.getText(None,'Edit size','New size')
        if ok:
            self.table.setHorizontalHeaderItem(index,QtGui.QTableWidgetItem(val))

    def editVerticalHeader(self, index):

        val,ok = QtGui.QInputDialog.getText(None,'Edit size','New size')
        if ok:
            self.table.setVerticalHeaderItem(index,QtGui.QTableWidgetItem(val))

    def setWidth(self,d):

        self.width = d

    def setHeight(self,d):

        self.height = d

    def accept(self):

        self.obj.Width = self.width
        self.obj.Height = self.height
        self.obj.Rows = self.table.rowCount()
        self.obj.Columns = self.table.columnCount()
        self.obj.RowSize = [FreeCAD.Units.Quantity(self.table.verticalHeaderItem(i).text()).Value for i in range(self.table.rowCount())]
        self.obj.ColumnSize = [FreeCAD.Units.Quantity(self.table.horizontalHeaderItem(i).text()).Value for i in range(self.table.columnCount())]
        self.obj.Spans = [str(s)[1:-1] for s in self.spans]
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def reject(self):

        FreeCADGui.ActiveDocument.resetEdit()
        return True





if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Axis',_CommandAxis())
    FreeCADGui.addCommand('Arch_AxisSystem',_CommandAxisSystem())
    FreeCADGui.addCommand('Arch_Grid',CommandArchGrid())

    class _ArchAxisGroupCommand:

        def GetCommands(self):
            return tuple(['Arch_Axis','Arch_AxisSystem','Arch_Grid'])
        def GetResources(self):
            return { 'MenuText': QT_TRANSLATE_NOOP("Arch_AxisTools",'Axis tools'),
                     'ToolTip': QT_TRANSLATE_NOOP("Arch_AxisTools",'Axis tools')
                   }
        def IsActive(self):
            return not FreeCAD.ActiveDocument is None

    FreeCADGui.addCommand('Arch_AxisTools', _ArchAxisGroupCommand())

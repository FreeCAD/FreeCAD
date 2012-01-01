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

import FreeCAD,FreeCADGui,Draft,ArchComponent,math
from draftlibs import fcvec
from FreeCAD import Vector
from PyQt4 import QtCore
from pivy import coin

__title__="FreeCAD Axis System"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeAxis(num=0,size=0,name="Axes"):
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
        FreeCAD.ActiveDocument.openTransaction("Axis")
        makeAxis(5,1)
        FreeCAD.ActiveDocument.commitTransaction()
       
class _Axis(ArchComponent.Component):
    "The Axis object"
    def __init__(self,obj):
        obj.addProperty("App::PropertyFloatList","Distances","Base", "The intervals between axes")
        obj.addProperty("App::PropertyFloatList","Angles","Base", "The angles of each axis")
        obj.addProperty("App::PropertyFloatList","Limits","Base", "The inferior and superior drawing limits")
        self.Type = "Axis"

        obj.Limits=[0.0,1.0]
        obj.Proxy = self
        self.Object = obj
        
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
                    p1 = Vector(dist,obj.Limits[0],0)
                    p2 = Vector(dist+(obj.Limits[1]/math.cos(ang))*math.sin(ang),obj.Limits[1],0)
                    geoms.append(Part.Line(p1,p2).toShape())
        if geoms:
            obj.Shape = Part.Compound(geoms)
        obj.Placement = pl
        
class _ViewProviderAxis(ArchComponent.ViewProviderComponent):
    "A View Provider for the Axis object"

    def __init__(self,vobj):
        vobj.addProperty("App::PropertyLength","BubbleSize","Base", "The size of the axis bubbles")
        vobj.addProperty("App::PropertyEnumeration","NumerationStyle","Base", "The numeration style")
        vobj.NumerationStyle = ["1,2,3","01,02,03","A,B,C","a,b,c","I,II,III"]
        vobj.Proxy = self
        self.Object = vobj.Object
        self.ViewObject = vobj
        vobj.BubbleSize = .1
        vobj.LineWidth = 1
    
    def getIcon(self):          
        return ":/icons/Arch_Axis_Tree.svg"

    def claimChildren(self):
        return []

    def attach(self, vobj):
        self.Object = vobj.Object
        self.ViewObject = vobj
        self.bubbles = None

    def makeBubbles(self):
        import Part
        rn = self.ViewObject.RootNode.getChild(2).getChild(0).getChild(0)
        if self.bubbles:
            rn.removeChild(self.bubbles)
            self.bubbles = None
        self.bubbles = coin.SoSeparator()
        for i in range(len(self.Object.Distances)):
            invpl = self.Object.Placement.inverse()
            verts = self.Object.Shape.Edges[i].Vertexes
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
            tr.translation.setValue((center.x,center.y,center.z))
            fo = coin.SoFont()
            fo.name = "Arial,Sans"
            fo.size = rad*100
            tx = coin.SoText2()
            tx.justification = coin.SoText2.CENTER
            tx.string = str(i)
            st.addChild(tr)
            st.addChild(fo)
            st.addChild(tx)
            self.bubbles.addChild(st)
            
        rn.addChild(self.bubbles)
            
    def updateData(self, obj, prop):
        if prop == "Shape":
            self.makeBubbles()
        return

    def onChanged(self, vobj, prop):
        if prop in ["NumerationStyle","BubbleSize"]:
            self.makeBubbles()
        return

    
FreeCADGui.addCommand('Arch_Axis',_CommandAxis())

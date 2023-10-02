#***************************************************************************
#*   Copyright (c) 2020 Yorik van Havre <yorik@uncreated.net>              *
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

from FreeCAD import Vector
from draftutils.translate import translate


WindowPresets =  ["Fixed", "Open 1-pane", "Open 2-pane", "Sash 2-pane",
                  "Sliding 2-pane", "Simple door", "Glass door", "Sliding 4-pane", "Awning"]

def makeWindowPreset(windowtype,width,height,h1,h2,h3,w1,w2,o1,o2,placement=None):

    """makeWindowPreset(windowtype,width,height,h1,h2,h3,w1,w2,o1,o2,[placement]): makes a
    window object based on the given data. windowtype must be one of the names
    defined in Arch.WindowPresets"""

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return

    def makeSketch(windowtype,width,height,h1,h2,h3,w1,w2,o1,o2):

        import Part
        import Sketcher
        width = float(width)
        height = float(height)
        h1 = float(h1)
        h2 = float(h2)
        h3 = float(h3)
        w1 = float(w1)
        w2 = float(w2)
        o1 = float(o1)
        o2 = float(o2)
        # h1, h2, w1, w2 cannot be null (for now)
        # TODO allow these to be null (don't create the component if so)
        if h1*h2*w1*w2 == 0:
            FreeCAD.Console.PrintError("H1, H2, W1 and W2 parameters cannot be zero. Aborting\n")
            return
        # small spacing to avoid wrong auto-wires in sketch
        tol = h1/10
        # glass size divider
        gla = 10
        s = FreeCAD.ActiveDocument.addObject('Sketcher::SketchObject','Sketch')

        def addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8):

            "adds two rectangles to the given sketch"

            idx = s.GeometryCount
            s.addGeometry(Part.LineSegment(p1,p2))
            s.addGeometry(Part.LineSegment(p2,p3))
            s.addGeometry(Part.LineSegment(p3,p4))
            s.addGeometry(Part.LineSegment(p4,p1))
            s.addConstraint(Sketcher.Constraint('Coincident',idx,2,idx+1,1))
            s.addConstraint(Sketcher.Constraint('Coincident',idx+1,2,idx+2,1))
            s.addConstraint(Sketcher.Constraint('Coincident',idx+2,2,idx+3,1))
            s.addConstraint(Sketcher.Constraint('Coincident',idx+3,2,idx,1))
            s.addConstraint(Sketcher.Constraint('Horizontal',idx))
            s.addConstraint(Sketcher.Constraint('Horizontal',idx+2))
            s.addConstraint(Sketcher.Constraint('Vertical',idx+1))
            s.addConstraint(Sketcher.Constraint('Vertical',idx+3))
            s.addGeometry(Part.LineSegment(p5,p6))
            s.addGeometry(Part.LineSegment(p6,p7))
            s.addGeometry(Part.LineSegment(p7,p8))
            s.addGeometry(Part.LineSegment(p8,p5))
            s.addConstraint(Sketcher.Constraint('Coincident',idx+4,2,idx+5,1))
            s.addConstraint(Sketcher.Constraint('Coincident',idx+5,2,idx+6,1))
            s.addConstraint(Sketcher.Constraint('Coincident',idx+6,2,idx+7,1))
            s.addConstraint(Sketcher.Constraint('Coincident',idx+7,2,idx+4,1))
            s.addConstraint(Sketcher.Constraint('Horizontal',idx+4))
            s.addConstraint(Sketcher.Constraint('Horizontal',idx+6))
            s.addConstraint(Sketcher.Constraint('Vertical',idx+5))
            s.addConstraint(Sketcher.Constraint('Vertical',idx+7))

        def simpleFrame(s,width,height,h1,h2,tol):

            "creates a simple frame with constraints"

            p1 = Vector(h1+tol,h1+tol,0)
            p2 = Vector(width-(h1+tol),h1+tol,0)
            p3 = Vector(width-(h1+tol),height-(h1+tol),0)
            p4 = Vector(h1+tol,height-(h1+tol),0)
            p5 = Vector(h1+h2,h1+h2,0)
            p6 = Vector(width-(h1+h2),h1+h2,0)
            p7 = Vector(width-(h1+h2),height-(h1+h2),0)
            p8 = Vector(h1+h2,height-(h1+h2),0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            s.addConstraint(Sketcher.Constraint('DistanceX',8,1,12,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',8,1,12,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',14,1,10,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',14,1,10,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',4,1,8,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',4,1,8,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceX',10,1,6,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',10,1,6,1,tol))
            if h2 == h1:
                s.renameConstraint(39,'Frame5')
                s.renameConstraint(40,'Frame6')
                s.renameConstraint(42,'Frame7')
                s.renameConstraint(41,'Frame8')

        def outerFrame(s,width,height,h1,w1,o1):

            p1 = Vector(0,0,0)
            p2 = Vector(width,0,0)
            p3 = Vector(width,height,0)
            p4 = Vector(0,height,0)
            p5 = Vector(h1,h1,0)
            p6 = Vector(width-h1,h1,0)
            p7 = Vector(width-h1,height-h1,0)
            p8 = Vector(h1,height-h1,0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            s.addConstraint(Sketcher.Constraint('DistanceY',1,height)) #16
            s.addConstraint(Sketcher.Constraint('DistanceX',0,width)) #17
            s.renameConstraint(16, 'Height')
            s.renameConstraint(17, 'Width')
            s.addConstraint(Sketcher.Constraint('DistanceY',6,2,2,2,h1))
            s.addConstraint(Sketcher.Constraint('DistanceX',2,2,6,2,h1))
            s.addConstraint(Sketcher.Constraint('DistanceX',4,2,0,2,h1))
            s.addConstraint(Sketcher.Constraint('DistanceY',0,2,4,2,h1))
            s.renameConstraint(18, 'Frame1')
            s.renameConstraint(19, 'Frame2')
            s.renameConstraint(20, 'Frame3')
            s.renameConstraint(21, 'Frame4')
            s.addConstraint(Sketcher.Constraint('Coincident',0,1,-1,1))
            return ["OuterFrame","Frame","Wire0,Wire1",str(w1-w2)+"+V","0.00+V"]

        def doorFrame(s,width,height,h1,w1,o1):

            p1 = Vector(0,0,0)
            p2 = Vector(width,0,0)
            p3 = Vector(width,height,0)
            p4 = Vector(0,height,0)
            p5 = Vector(h1,0,0)
            p6 = Vector(width-h1,0,0)
            p7 = Vector(width-h1,height-h1,0)
            p8 = Vector(h1,height-h1,0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            s.addConstraint(Sketcher.Constraint('DistanceY',1,height)) #16
            s.addConstraint(Sketcher.Constraint('DistanceX',0,width)) #17
            s.renameConstraint(16, 'Height')
            s.renameConstraint(17, 'Width')
            s.addConstraint(Sketcher.Constraint('DistanceY',6,2,2,2,h1))
            s.addConstraint(Sketcher.Constraint('DistanceX',2,2,6,2,h1))
            s.addConstraint(Sketcher.Constraint('DistanceX',4,2,0,2,h1))
            s.addConstraint(Sketcher.Constraint('DistanceY',0,2,4,2,0.0))
            s.addConstraint(Sketcher.Constraint('Coincident',0,1,-1,1))
            s.renameConstraint(18, 'Frame1')
            s.renameConstraint(19, 'Frame2')
            s.renameConstraint(20, 'Frame3')
            return ["OuterFrame","Frame","Wire0,Wire1",str(w1-w2)+"+V","0.00+V"]

        if windowtype == "Fixed":

            wp = outerFrame(s,width,height,h1,w1,o1)
            wp.extend(["Glass","Glass panel","Wire1",str(w1/gla),str(w1/2)+"+V"])

        elif windowtype == "Open 1-pane":

            wp = outerFrame(s,width,height,h1,w1,o1)
            simpleFrame(s,width,height,h1,h2,tol)
            fw = str(w2)
            if w2 == w1:
                fw = "0.00+V"
            wp.extend(["InnerFrame","Frame","Wire2,Wire3,Edge8,Mode1",fw,str(o2)+"+V"])
            wp.extend(["InnerGlass","Glass panel","Wire3",str(w2/gla),str(o2+w2/2)+"+V"])

        elif windowtype == "Open 2-pane":

            wp = outerFrame(s,width,height,h1,w1,o1)
            p1 = Vector(h1+tol,h1+tol,0)
            p2 = Vector((width/2)-tol,h1+tol,0)
            p3 = Vector((width/2)-tol,height-(h1+tol),0)
            p4 = Vector(h1+tol,height-(h1+tol),0)
            p5 = Vector(h1+h2,h1+h2,0)
            p6 = Vector((width/2)-h2,h1+h2,0)
            p7 = Vector((width/2)-h2,height-(h1+h2),0)
            p8 = Vector(h1+h2,height-(h1+h2),0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            p1 = Vector((width/2)+tol,h1+tol,0)
            p2 = Vector(width-(h1+tol),h1+tol,0)
            p3 = Vector(width-(h1+tol),height-(h1+tol),0)
            p4 = Vector((width/2)+tol,height-(h1+tol),0)
            p5 = Vector((width/2)+h2,h1+h2,0)
            p6 = Vector(width-(h1+h2),h1+h2,0)
            p7 = Vector(width-(h1+h2),height-(h1+h2),0)
            p8 = Vector((width/2)+h2,height-(h1+h2),0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            s.addConstraint(Sketcher.Constraint('DistanceY',8,1,12,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',8,1,12,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',21,2,17,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',21,2,17,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',16,1,20,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',14,1,10,1,h2))
            s.addConstraint(Sketcher.Constraint('Equal',22,14))
            s.addConstraint(Sketcher.Constraint('DistanceY',8,2,16,1,0.0))
            s.addConstraint(Sketcher.Constraint('DistanceY',10,1,18,2,0.0))
            s.addConstraint(Sketcher.Constraint('DistanceX',4,1,8,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',4,1,8,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceX',6,1,18,1,-tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',6,1,18,1,-tol))
            s.addConstraint(Sketcher.Constraint('DistanceX',9,1,19,2,tol))
            s.addConstraint(Sketcher.Constraint('PointOnObject',13,2,22))
            s.addConstraint(Sketcher.Constraint('PointOnObject',20,1,12))
            if h1 == h2:
                s.renameConstraint(55,'Frame5')
                s.renameConstraint(56,'Frame6')
                s.renameConstraint(57,'Frame7')
                s.renameConstraint(58,'Frame8')
                s.renameConstraint(59,'Frame9')
                s.renameConstraint(60,'Frame10')
            fw = str(w2)
            if w2 == w1:
                fw = "0.00+V"
            wp.extend(["LeftFrame","Frame","Wire2,Wire3,Edge8,Mode1",fw,str(o2)+"+V"])
            wp.extend(["LeftGlass","Glass panel","Wire3",str(w2/gla),str(o2+w2/2)+"+V"])
            wp.extend(["RightFrame","Frame","Wire4,Wire5,Edge6,Mode2",fw,str(o2)+"+V"])
            wp.extend(["RightGlass","Glass panel","Wire5",str(w2/gla),str(o2+w2/2)+"+V"])

        elif windowtype == "Sash 2-pane":

            wp = outerFrame(s,width,height,h1,w1,o1)
            p1 = Vector(h1+tol,h1+tol,0)
            p2 = Vector(width-(h1+tol),h1+tol,0)
            p3 = Vector(width-(h1+tol),(height/2)-tol,0)
            p4 = Vector(h1+tol,(height/2)-tol,0)
            p5 = Vector(h1+h2,h1+h2,0)
            p6 = Vector(width-(h1+h2),h1+h2,0)
            p7 = Vector(width-(h1+h2),(height/2)-h2,0)
            p8 = Vector(h1+h2,(height/2)-h2,0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            p1 = Vector(h1+tol,(height/2)+tol,0)
            p2 = Vector(width-(h1+tol),(height/2)+tol,0)
            p3 = Vector(width-(h1+tol),height-(h1+tol),0)
            p4 = Vector(h1+tol,height-(h1+tol),0)
            p5 = Vector(h1+h2,(height/2)+h2,0)
            p6 = Vector(width-(h1+h2),(height/2)+h2,0)
            p7 = Vector(width-(h1+h2),height-(h1+h2),0)
            p8 = Vector(h1+h2,height-(h1+h2),0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            s.addConstraint(Sketcher.Constraint('DistanceY',8,1,12,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',8,1,12,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',21,2,17,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',21,2,17,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',16,2,20,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',10,2,14,2,-h2))
            s.addConstraint(Sketcher.Constraint('Equal',23,15))
            s.addConstraint(Sketcher.Constraint('DistanceX',12,1,20,1,0.0))
            s.addConstraint(Sketcher.Constraint('DistanceX',13,2,20,2,0.0))
            s.addConstraint(Sketcher.Constraint('DistanceX',4,1,8,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',4,1,8,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceX',6,1,18,1,-tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',6,1,18,1,-tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',10,1,16,1,tol))
            s.addConstraint(Sketcher.Constraint('PointOnObject',9,2,17))
            s.addConstraint(Sketcher.Constraint('PointOnObject',16,1,11))
            if h1 == h2:
                s.renameConstraint(55,'Frame5')
                s.renameConstraint(56,'Frame6')
                s.renameConstraint(57,'Frame7')
                s.renameConstraint(58,'Frame8')
                s.renameConstraint(59,'Frame9')
                s.renameConstraint(60,'F10')
                s.setExpression('.Constraints.F10','-.Constraints.Frame5')
            fw = str(w2)
            if w2 == w1:
                fw = "0.00+V"
            wp.extend(["LowerFrame","Frame","Wire2,Wire3",fw,str(o2+w2)+"+V"])
            wp.extend(["LowerGlass","Glass panel","Wire3",str(w2/gla),str(o2+w2+w2/2)+"+V"])
            wp.extend(["UpperFrame","Frame","Wire4,Wire5",fw,str(o2)+"+V"])
            wp.extend(["UpperGlass","Glass panel","Wire5",str(w2/gla),str(o2+w2/2)+"+V"])

        elif windowtype == "Sliding 2-pane":

            wp = outerFrame(s,width,height,h1,w1,o1)
            p1 = Vector(h1+tol,h1+tol,0)
            p2 = Vector((width/2)-tol,h1+tol,0)
            p3 = Vector((width/2)-tol,height-(h1+tol),0)
            p4 = Vector(h1+tol,height-(h1+tol),0)
            p5 = Vector(h1+h2,h1+h2,0)
            p6 = Vector((width/2)-h2,h1+h2,0)
            p7 = Vector((width/2)-h2,height-(h1+h2),0)
            p8 = Vector(h1+h2,height-(h1+h2),0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            p1 = Vector((width/2)+tol,h1+tol,0)
            p2 = Vector(width-(h1+tol),h1+tol,0)
            p3 = Vector(width-(h1+tol),height-(h1+tol),0)
            p4 = Vector((width/2)+tol,height-(h1+tol),0)
            p5 = Vector((width/2)+h2,h1+h2,0)
            p6 = Vector(width-(h1+h2),h1+h2,0)
            p7 = Vector(width-(h1+h2),height-(h1+h2),0)
            p8 = Vector((width/2)+h2,height-(h1+h2),0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            s.addConstraint(Sketcher.Constraint('DistanceY',8,1,12,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',8,1,12,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',21,2,17,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',21,2,17,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',16,1,20,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',14,1,10,1,h2))
            s.addConstraint(Sketcher.Constraint('Equal',22,14))
            s.addConstraint(Sketcher.Constraint('DistanceY',8,2,16,1,0.0))
            s.addConstraint(Sketcher.Constraint('DistanceY',10,1,18,2,0.0))
            s.addConstraint(Sketcher.Constraint('DistanceX',4,1,8,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',4,1,8,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceX',6,1,18,1,-tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',6,1,18,1,-tol))
            s.addConstraint(Sketcher.Constraint('DistanceX',9,1,19,2,tol))
            s.addConstraint(Sketcher.Constraint('PointOnObject',13,2,22))
            s.addConstraint(Sketcher.Constraint('PointOnObject',12,2,20))
            if h1 == h2:
                s.renameConstraint(55,'Frame5')
                s.renameConstraint(56,'Frame6')
                s.renameConstraint(57,'Frame7')
                s.renameConstraint(58,'Frame8')
                s.renameConstraint(59,'Frame9')
                s.renameConstraint(60,'Frame10')
            fw = str(w2)
            if w2 == w1:
                fw = "0.00+V"
            wp.extend(["LeftFrame","Frame","Wire2,Wire3",fw,str(o2)+"+V"])
            wp.extend(["LeftGlass","Glass panel","Wire3",str(w2/gla),str(o2+w2/2)+"+V"])
            wp.extend(["RightFrame","Frame","Wire4,Wire5",fw,str(o2+w2)+"+V"])
            wp.extend(["RightGlass","Glass panel","Wire5",str(w2/gla),str(o2+w2+w2/2)+"+V"])

        elif windowtype == "Sliding 4-pane":

            wp = outerFrame(s,width,height,h1,w1,o1)
            p1 = Vector(h1+tol,h1+tol,0)
            p2 = Vector(width/4-tol,h1+tol,0)
            p3 = Vector(width/4-tol,height-(h1+tol),0)
            p4 = Vector(h1+tol,height-(h1+tol),0)
            p5 = Vector(h1+h2,h1+h2,0)
            p6 = Vector(width/4-h2,h1+h2,0)
            p7 = Vector(width/4-h2,height-(h1+h2),0)
            p8 = Vector(h1+h2,height-(h1+h2),0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            p1 = Vector(width/4+tol,h1+tol,0)
            p2 = Vector(width/2-tol,h1+tol,0)
            p3 = Vector(width/2-tol,height-(h1+tol),0)
            p4 = Vector(width/4+tol,height-(h1+tol),0)
            p5 = Vector(width/4+h2,h1+h2,0)
            p6 = Vector(width/2-h2,h1+h2,0)
            p7 = Vector(width/2-h2,height-(h1+h2),0)
            p8 = Vector(width/4+h2,height-(h1+h2),0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            p1 = Vector(width/2+tol,h1+tol,0)
            p2 = Vector(width*3/4-tol,h1+tol,0)
            p3 = Vector(width*3/4-tol,height-(h1+tol),0)
            p4 = Vector(width/2+tol,height-(h1+tol),0)
            p5 = Vector(width/2+h2,h1+h2,0)
            p6 = Vector(width*3/4-h2,h1+h2,0)
            p7 = Vector(width*3/4-h2,height-(h1+h2),0)
            p8 = Vector(width/2+h2,height-(h1+h2),0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            p1 = Vector(width*3/4+tol,h1+tol,0)
            p2 = Vector(width-(h1+tol),h1+tol,0)
            p3 = Vector(width-(h1+tol),height-(h1+tol),0)
            p4 = Vector(width*3/4+tol,height-(h1+tol),0)
            p5 = Vector(width*3/4+h2,h1+h2,0)
            p6 = Vector(width-(h1+h2),h1+h2,0)
            p7 = Vector(width-(h1+h2),height-(h1+h2),0)
            p8 = Vector(width*3/4+h2,height-(h1+h2),0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            s.addConstraint(Sketcher.Constraint('DistanceX',4,1,8,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceX',8,2,16,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceX',17,1,27,2,tol))
            s.addConstraint(Sketcher.Constraint('DistanceX',24,2,32,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceX',32,2,4,2,tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',4,1,8,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',10,2,6,2,tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',17,2,26,2,0.0))
            s.addConstraint(Sketcher.Constraint('DistanceY',25,2,34,2,0.0))
            s.addConstraint(Sketcher.Constraint('DistanceY',8,2,16,1,0.0))
            s.addConstraint(Sketcher.Constraint('DistanceY',9,2,18,2,0.0))
            s.addConstraint(Sketcher.Constraint('DistanceY',16,2,24,1,0.0))
            s.addConstraint(Sketcher.Constraint('DistanceY',24,2,32,1,0.0))
            s.addConstraint(Sketcher.Constraint('DistanceX',8,1,12,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',8,1,12,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',13,2,9,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',13,2,9,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',16,1,20,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',16,1,20,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',21,2,17,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',21,2,17,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',24,1,28,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',24,1,28,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',29,2,25,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',29,2,25,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',32,1,36,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',32,1,36,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',37,2,33,2,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',37,2,33,2,h2))
            s.addConstraint(Sketcher.Constraint('Equal',14,22))
            s.addConstraint(Sketcher.Constraint('Equal',22,30))
            s.addConstraint(Sketcher.Constraint('Equal',30,38))
            if h1 == h2:
                s.renameConstraint(100,'Frame5')
                s.renameConstraint(101,'Frame6')
                s.renameConstraint(102,'Frame7')
                s.renameConstraint(103,'Frame8')
                s.renameConstraint(104,'Frame9')
                s.renameConstraint(105,'Frame10')
                s.renameConstraint(106,'Frame11')
                s.renameConstraint(107,'Frame12')
                s.renameConstraint(108,'Frame13')
                s.renameConstraint(109,'Frame14')
                s.renameConstraint(110,'Frame15')
                s.renameConstraint(111,'Frame16')
                s.renameConstraint(112,'Frame17')
                s.renameConstraint(113,'Frame18')
                s.renameConstraint(114,'Frame19')
                s.renameConstraint(115,'Frame20')
            fw = str(w2)
            if w2 == w1:
                fw = "0.00+V"
            wp.extend(["LeftMostFrame","Frame","Wire2,Wire3",fw,str(o2)+"+V"])
            wp.extend(["LeftMostGlass","Glass panel","Wire3",str(w2/gla),str(o2+w2/2)+"+V"])
            wp.extend(["LeftFrame","Frame","Wire4,Wire5",fw,str(o2+w2)+"+V"])
            wp.extend(["LeftGlass","Glass panel","Wire5",str(w2/gla),str(o2+w2+w2/2)+"+V"])
            wp.extend(["RightFrame","Frame","Wire6,Wire7",fw,str(o2+w2)+"+V"])
            wp.extend(["RightGlass","Glass panel","Wire7",str(w2/gla),str(o2+w2+w2/2)+"+V"])
            wp.extend(["RightMostFrame","Frame","Wire8,Wire9",fw,str(o2)+"+V"])
            wp.extend(["RightMostGlass","Glass panel","Wire9",str(w2/gla),str(o2+w2/2)+"+V"])

        elif windowtype == "Awning":

            wp = outerFrame(s,width,height,h1,w1,o1)
            simpleFrame(s,width,height,h1,h2,tol)
            fw = str(w2)
            if w2 == w1:
                fw = "0.00+V"
            wp.extend(["InnerFrame","Frame","Wire2,Wire3,Edge7,Mode1",fw,str(o2)+"+V"])
            wp.extend(["InnerGlass","Glass panel","Wire3",str(w2/gla),str(o2+w2/2)+"+V"])


        elif windowtype == "Simple door":

            wp = doorFrame(s,width,height,h1,w1,o1)
            wp.extend(["Door","Solid panel","Wire1,Edge8,Mode1",str(w2),str(o2)+"+V"])

        elif windowtype == "Glass door":

            wp = doorFrame(s,width,height,h1,w1,o1)
            p1 = Vector(h1+tol,h1+tol,0)
            p2 = Vector(width-(h1+tol),h1+tol,0)
            p3 = Vector(width-(h1+tol),height-(h1+tol),0)
            p4 = Vector(h1+tol,height-(h1+tol),0)
            p5 = Vector(h1+h2,h1+h3,0)
            p6 = Vector(width-(h1+h2),h1+h3,0)
            p7 = Vector(width-(h1+h2),height-(h1+h2),0)
            p8 = Vector(h1+h2,height-(h1+h2),0)
            addFrame(s,p1,p2,p3,p4,p5,p6,p7,p8)
            s.addConstraint(Sketcher.Constraint('DistanceX',8,1,12,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',8,1,12,1,h3))
            s.addConstraint(Sketcher.Constraint('DistanceX',14,1,10,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceY',14,1,10,1,h2))
            s.addConstraint(Sketcher.Constraint('DistanceX',4,1,8,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',4,1,8,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceX',10,1,6,1,tol))
            s.addConstraint(Sketcher.Constraint('DistanceY',10,1,6,1,tol))
            if h2 == h1:
                s.renameConstraint(39,'Frame5')
                s.renameConstraint(40,'Frame6')
                s.renameConstraint(42,'Frame7')
                s.renameConstraint(41,'Frame8')
            fw = str(w2)
            if w2 == w1:
                fw = "0.00+V"
            wp.extend(["InnerFrame","Frame","Wire2,Wire3,Edge8,Mode1",fw,str(o2)+"+V"])
            wp.extend(["InnerGlass","Glass panel","Wire3",str(w2/gla),str(o2+w2/2)+"+V"])

        return (s,wp)

    if windowtype in WindowPresets:
        import ArchWindow
        default = makeSketch(windowtype,width,height,h1,h2,h3,w1,w2,o1,o2)
        FreeCAD.ActiveDocument.recompute()
        if default:
            if placement:
                default[0].Placement = placement
                FreeCAD.ActiveDocument.recompute()
            obj = ArchWindow.makeWindow(default[0],width,height,default[1])
            obj.Preset = WindowPresets.index(windowtype)+1
            obj.Frame = h1
            obj.Offset = o1
            obj.Placement = FreeCAD.Placement() # unable to find where this bug comes from...
            if "door" in windowtype:
                obj.IfcType = "Door"
                obj.Label = translate("Arch","Door")
            FreeCAD.ActiveDocument.recompute()
            return obj

    print("Arch: Unknown window type")

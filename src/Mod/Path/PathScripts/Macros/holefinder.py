#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014 Daniel Falck  <ddfalck@gmail.com>                  *  
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
from FreeCAD import Base
import Part
import Draft
from PySide import QtGui,QtCore
from math import pi
'''
This macro makes a list of holes for drilling  from a solid 
1. Select a solid object that has holes in it and run the macro
2. It only collects info on holes that are parallel to the Z axis- I don't have a 4 or 5 axis mill at the moment
3. It pulls the center of the hole bounding box and the XLength for it's diameter
4. It will place a list of the holes on the clipboard
5. Uncomment the line that starts with '#Draft.makeLine' and manipulate it, if you want to see lines down the center of each hole.
6. Manipulate the line that starts with  'holelist.append' to make the list fit your own needs- I've put the ZMax at the ZMax of the solid's bounding box
because I want to make sure that my drill tip doesn't collide with anything on the top of the part. YMMV.
'''
def findholes(obj):
    facelist = []
    holelist =[]
    vz = Base.Vector(0,0,1)
    for f in obj.Faces:
        if ( round(f.ParameterRange[0], 8)==0.0 ) and ( round(f.ParameterRange[1],8) == round(pi*2, 8)  ) : #eliminate flat faces
            facelist.append(f)

    for h in facelist:
        for w in h.Wires:
            for c in w.Edges:
                if ( isinstance(c.Curve,Part.Line)):
                    v0=Base.Vector(c.Vertexes[0].X, c.Vertexes[0].Y, c.Vertexes[0].Z); v1=Base.Vector(c.Vertexes[1].X,c.Vertexes[1].Y, c.Vertexes[1].Z)
                    if (v1.sub(v0).x == 0) and (v1.sub(v0).y == 0):
                        lsp = Base.Vector(h.BoundBox.Center.x,h.BoundBox.Center.y,h.BoundBox.ZMax)
                        lep = Base.Vector(h.BoundBox.Center.x,h.BoundBox.Center.y,h.BoundBox.ZMin)
                        if obj.isInside(lsp, 0,False) or obj.isInside(lep, 0,False):
                            pass
                        else:
                            Draft.makeLine((h.BoundBox.Center.x,h.BoundBox.Center.y,obj.BoundBox.ZMax ),(h.BoundBox.Center.x,h.BoundBox.Center.y,h.BoundBox.ZMin  ))
                            x =h.BoundBox.Center.x;y=h.BoundBox.Center.y;zmax=obj.BoundBox.ZMax;zmin=h.BoundBox.ZMin;diameter=h.BoundBox.XLength
                            holelist.append((diameter, x,y,zmax,zmin))
    clipboard = QtGui.QApplication.clipboard()
    clipboard.setText(str(holelist))

sel=Gui.Selection.getSelection()
obj = sel[0].Shape
findholes(obj)

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
'''
This macro finds the outside profile of a 3D shape. Right now it just creates wires that represent the shape,but it could be used for finding the outside profile of an object for a toolpath
'''
import FreeCADGui
import DraftGeomUtils
from FreeCAD import Vector
from PathScripts import find_outer_profile as fop

sel = FreeCADGui.Selection.getSelection()[0]
obj = sel
el = fop.edgelist(obj)
hl = fop.horizontal(el)
connected = DraftGeomUtils.findWires(hl)
goodwires = fop.openFilter(connected)

outerwires ,innerwires, same = fop.findOutsideWire(goodwires)
#get distance from outerwires Z to bottom of part
zdiff = obj.Shape.BoundBox.ZMin- outerwires.BoundBox.ZMax
outerwires.Placement.move(Vector(0,0,zdiff))
Part.show(outerwires)

zupperouter = outerwires
zupperouter.Placement.move(Vector(0,0,obj.Shape.BoundBox.ZMax))
Part.show(zupperouter)

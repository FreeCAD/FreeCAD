#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *  
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

import FreeCAD, Fem


__title__="Machine-Distortion Move tools"
__author__ = "Juergen Riegel"
__url__ = "http://free-cad.sourceforge.net"



def moveHome(obj):
    b = obj.FemMesh.BoundBox
    m = FreeCAD.Vector(-(b.XMin),-(b.YMin),-(b.ZMin))
    p = obj.Placement
    p2 = FreeCAD.Placement(p.Base + m,p.Rotation)
    obj.Placement = p2
    return

def getBoundBoxVolume(obj):
    b = obj.FemMesh.BoundBox
    return b.XLength * b.YLength * b.ZLength

def minimizeBoundVolume(obj):
    p = obj.Placement
    VolOld = getBoundBoxVolume(obj)
    OverallSteps = 0
    # rotate a fraction and test if it get better
    for a in (3.0,1.0,0.5,0.1,0.05,0.01,0.005,0.001):
        for v in ( (0.0, 0.0, 1.0),(0.0, 1.0, 0.0),(1.0, 0.0, 0.0) ):
            for dir in (-1.0,1.0):
                Better = True
                i = 0
                while(Better):
                    p.Rotation = p.Rotation.multiply(FreeCAD.Rotation(FreeCAD.Vector(v[0],v[1],v[2]),a*dir))
                    NewVol = getBoundBoxVolume(obj)
                    i = i+1
                    if(NewVol>VolOld):
                        Better = False
                        print "Axis: (",v[0],v[1],v[2],") Angle: ",a*dir," -> End with after ",i," Steps with V=",NewVol
                    VolOld = NewVol
                OverallSteps = OverallSteps + i
    print "OverallSteps: ",OverallSteps
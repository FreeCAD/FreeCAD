#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *  
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

# FreeCAD modules
import FreeCAD,FreeCADGui
from FreeCAD import Base
from FreeCAD import Part
# FreeCADShip modules
from surfUtils import Paths
from surfUtils import Geometry
from surfUtils import Math

class Preview(object):
    def __init__(self):
        """ Constructor.
        @param self Auto call object.
        @note Start it as unactive
        """
        self.objs = None
        self.reinit()

    def reinit(self):
        """ Reinitializate drawer.
        @param self Auto call object.
        """
        self.clean()
        self.objs = None

    def update(self, surf, direction, r):
        """ Update the 3D view printing curve.
        @param surf Surf where get the curve.
        @param direction Slice plane normal vector.
        @param r Absolute position at Slice plane normal direction.
        @return Curve from object (as Part::Feature).
        """
        # Errors
        if not surf:
            return None
        # Get curve
        curve = self.getSlice(surf, direction, r)
        # Draw at 3D view
        self.clean()
        self.objs = []
        for i in range(0,len(curve)):
            for j in range(0,len(curve[i])):
                Part.show(curve[i][j])
                objs = FreeCAD.ActiveDocument.Objects
                objs[len(objs)-1].Label = 'surfSliceCurve'
                self.objs.append(objs[len(objs)-1])
        return self.objs

    def getSlice(self, surf, direction, r):
        """ Get surface slice.
        @param surf Surf where get the curve.
        @param direction Slice plane normal vector.
        @param r Absolute position at Slice plane normal direction.
        @return Curve from object.
        """
        # Errors
        if not surf:
            return None
        # Get curve
        curve = []
        for i in range(0,len(surf)):
            curve.append(surf[i].slice(direction, r))
        return curve

    def clean(self,Destroy=True):
        """ Erase all sections from screen.
        @param self Auto call object.
        @param Destroy True if the object must be destroyed, False otherwise.
        """
        if not self.objs:
            return
        for i in range(0,len(self.objs)):
            FreeCADGui.ActiveDocument.hide(self.objs[i].Name)
            if Destroy:
                FreeCAD.ActiveDocument.removeObject(self.objs[i].Name)
        if Destroy:
            self.objs=None

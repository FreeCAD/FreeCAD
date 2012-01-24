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
from shipUtils import Paths, Translator

class Preview(object):
    def __init__(self):
        """ Constructor.
        """
        self.reinit()

    def reinit(self):
        """ Reinitializate drawer.
        """
        self.obj   = None
        self.clean()

    def update(self, draft, trim, ship):
        """ Update free surface 3D view
        @param traft Draft.
        @param trim Trim in degrees.
        """
        # Destroy old object if exist
        self.clean()
        # Set free surface bounds
        bbox = ship.Shape.BoundBox
        L = 1.5 * bbox.XLength
        B = 3.0 * bbox.YLength
        # Create plane
        x = - 0.5 * L
        y = - 0.5 * B
        point = Base.Vector(x,y,0.0)
        plane = Part.makePlane(L,B, point, Base.Vector(0,0,1))
        # Set position
        plane.rotate(Base.Vector(0,0,0), Base.Vector(0,1,0), trim)
        plane.translate(Base.Vector(0,0,draft))
        # Create the FreeCAD object
        Part.show(plane)
        objs = FreeCAD.ActiveDocument.Objects
        self.obj = objs[len(objs)-1]
        self.obj.Label = 'FreeSurface'
        # Set properties of object
        guiObj = FreeCADGui.ActiveDocument.getObject(self.obj.Name)
        guiObj.ShapeColor = (0.4,0.8,0.85)
        guiObj.Transparency = 50

    def clean(self):
        """ Erase all annotations from screen.
        """
        if not self.obj:
            return
        FreeCAD.ActiveDocument.removeObject(self.obj.Name)
        self.obj=None

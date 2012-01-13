#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
#*   Jose Luis Cercós Pita <jlcercos@gmail.com>                            *  
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
    def __init__(self,U,V):
        """ Constructor.
        @param U Part::topoShape object that contains the U direction edge.
        @param V Part::topoShape object that contains the V direction edge.
        """
        self.objU = None
        self.objV = None
        self.U = U
        self.V = V
        self.buildObjs()
        self.setProperties()

    def buildObjs(self):
        """ Builds objects to show.
        """
        Part.show(self.U)
        objs = FreeCAD.ActiveDocument.Objects
        self.objU = objs[len(objs)-1]
        Part.show(self.V)
        objs = FreeCAD.ActiveDocument.Objects
        self.objV = objs[len(objs)-1]

    def setProperties(self):
        """ Set colour of lines and width.
        """
        self.objU.Label = 'U direction'
        self.objV.Label = 'V direction'
        # Get GUI objects instance
        objU = FreeCADGui.ActiveDocument.getObject(self.objU.Name)
        objV = FreeCADGui.ActiveDocument.getObject(self.objV.Name)
        objU.LineColor = (0.0,0.0,1.0)
        objU.LineWidth = 5.00
        objV.LineColor = (1.0,0.0,0.0)
        objV.LineWidth = 5.00
        
    def clean(self,Destroy=True):
        """ Erase all sections from screen.
        @param self Auto call object.
        @param Destroy True if the object must be destroyed, False otherwise.
        """
        if (not self.objU) or (not self.objV):
            return
        FreeCADGui.ActiveDocument.hide(self.objU.Name)
        FreeCADGui.ActiveDocument.hide(self.objV.Name)
        if Destroy:
            FreeCAD.ActiveDocument.removeObject(self.objU.Name)
            FreeCAD.ActiveDocument.removeObject(self.objV.Name)
            self.objU=None
            self.objV=None

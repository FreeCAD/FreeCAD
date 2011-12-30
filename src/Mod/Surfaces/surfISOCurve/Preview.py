# ##### BEGIN GPL LICENSE BLOCK #####
#
#  Author: Jose Luis Cercos Pita <jlcercos@gmail.com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

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
        self.obj = None
        self.reinit()

    def reinit(self):
        """ Reinitializate drawer.
        @param self Auto call object.
        """
        self.clean()
        self.obj = None

    def update(self, surf, direction, uv):
        """ Update the 3D view printing curve.
        @param self Auto call object.
        @param surf Surf where get the curve.
        @param direction 0 if u direction, 1 if v.
        @param uv Curve uv index, between 0 and 1.
        @return Curve from object (as Part::Feature).
        """
        # Errors
        if not surf:
            return None
        # Get curve
        if direction == 0:
            curve = self.getU(surf, uv)
        elif direction == 1:
            curve = self.getV(surf, uv)
        else:
            return None
        # Draw at 3D view
        self.clean()
        Part.show(curve.toShape())
        objs = FreeCAD.ActiveDocument.Objects
        self.obj = objs[len(objs)-1]
        self.obj.Label = 'surfISOCurve'
        return self.obj

    def getU(self, surf, uv):
        """ Get U curve from object.
        @param self Auto call object.
        @param surf Surf where get the curve.
        @param uv Curve uv index, between 0 and 1.
        @return Curve from object.
        """
        # Errors
        if not surf:
            return None
        if (uv < 0.0) or (uv > 1.0):
            return None
        # Get UV data
        knots = surf.UKnotSequence
        id0 = knots[0]
        id1 = knots[len(knots)-1]
        # Get curve
        curve = surf.uIso(id0 + uv*(id1-id0))
        return curve

    def getV(self, surf, uv):
        """ Get U curve from object.
        @param self Auto call object.
        @param surf Surf where get the curve.
        @param uv Curve uv index, between 0 and 1.
        @return Curve from object.
        """
        # Errors
        if not surf:
            return None
        if (uv < 0.0) or (uv > 1.0):
            return None
        # Get UV data
        knots = surf.VKnotSequence
        id0 = knots[0]
        id1 = knots[len(knots)-1]
        # Get curve
        curve = surf.vIso(id0 + uv*(id1-id0))
        return curve
        
    def clean(self,Destroy=True):
        """ Erase all sections from screen.
        @param self Auto call object.
        @param Destroy True if the object must be destroyed, False otherwise.
        """
        if not self.obj:
            return
        FreeCADGui.ActiveDocument.hide(self.obj.Name)
        if Destroy:
            FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            self.obj=None

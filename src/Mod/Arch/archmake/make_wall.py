#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
#*   Copyright (c) 2020 Carlo Pavan                                        *
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
"""Provide the object code for Arch Wall object."""
## @package wall
# \ingroup ARCH
# \brief Provide the object code for Arch Wall.

import FreeCAD as App
import DraftVecUtils
from archobjects.wall import Wall 
from archobjects.wallsegment import WallSegment

if App.GuiUp:
    import FreeCADGui as Gui
    from archviewproviders.view_wall import ViewProviderWall
    from archviewproviders.view_wallsegment import ViewProviderWallSegment


def make_wall_from_base(baseobj):
    """
    NOT IMPLEMENTED YET
    """
    pass
    '''if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    p = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    obj = App.ActiveDocument.addObject("Part::FeaturePython","Wall")
    obj.Label = translate("Arch",name)

    Wall(obj)
    if App.GuiUp:
        ViewProviderWall(obj.ViewObject)

    if baseobj:
        if hasattr(baseobj,'Shape') or baseobj.isDerivedFrom("Mesh::Feature"):
            obj.Base = baseobj
        else:
            App.Console.PrintWarning(str(translate("Arch","Walls can only be based on Part or Mesh objects")))
    '''


def make_wall_from_points(p1, p2, join_first=None, join_last=None,
                          width=None, height=None, align="Center",
                          name="Wall"):

    '''makeWall([obj],[length],[width],[height],[align],[face],[name]): creates a wall based on the
    given object, which can be a sketch, a draft object, a face or a solid, or no object at
    all, then you must provide length, width and height. Align can be "Center","Left" or "Right",
    face can be an index number of a face in the base object to base the wall on.'''

    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    # Add a Wall object to the document
    obj = App.ActiveDocument.addObject('Part::FeaturePython', 'Wall', Wall(), ViewProviderWall(), True)
    
    # Add a WallShape object to the document
    shp = App.ActiveDocument.addObject('Part::FeaturePython', obj.Name + 'Segment')
    WallSegment(shp)
    ViewProviderWallSegment(shp.ViewObject)
    
    # Add the WallShape as the Wall BaseGeometry
    obj.addObject(shp)
    obj.BaseGeometry = shp
    
    # Align the wall to the given point
    obj.Placement.Base = p1
    length = p1.distanceToPoint(p2)
    angle = DraftVecUtils.angle(p2-p1,App.Vector(1,0,0))
    obj.Placement.Rotation.Angle = -angle
    obj.BaseGeometry.LastPoint = App.Vector(length,0,0)
    
    # Apply end joining if present
    if join_first != join_last:
        if join_first:
            obj.JoinFirstEndTo = join_first
        if join_last:
            obj.JoinLastEndTo = join_last            

    App.ActiveDocument.recompute()

    return obj



# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides the object code for the WorkingPlaneProxy object."""
## @package wpproxy
# \ingroup draftobjects
# \brief Provides the object code for the WorkingPlaneProxy object.

## \addtogroup draftobjects
# @{
import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP


class WorkingPlaneProxy:
    """The Draft working plane proxy object"""

    def __init__(self,obj):
        obj.Proxy = self

        _tip = QT_TRANSLATE_NOOP("App::Property", "The placement of this object")
        obj.addProperty("App::PropertyPlacement", "Placement", "Base", _tip)

        obj.addProperty("Part::PropertyPartShape","Shape","Base","")

        obj.addExtension("Part::AttachExtensionPython")
        obj.changeAttacherType("Attacher::AttachEnginePlane")

        self.Type = "WorkingPlaneProxy"

    def execute(self,obj):
        import Part
        l = 1
        if obj.ViewObject:
            if hasattr(obj.ViewObject,"DisplaySize"):
                l = obj.ViewObject.DisplaySize.Value
        p = Part.makePlane(l,
                           l,
                           App.Vector(l/2, -l/2, 0),
                           App.Vector(0, 0, -1))
        # make sure the normal direction is pointing outwards, you never know what OCC will decide...
        if p.normalAt(0,0).getAngle(obj.Placement.Rotation.multVec(App.Vector(0,0,1))) > 1:
            p.reverse()
        p.Placement = obj.Placement
        obj.Shape = p

    def onChanged(self,obj,prop):
        pass

    def getNormal(self,obj):
        return obj.Shape.Faces[0].normalAt(0,0)

    def dumps(self):
        return self.Type

    def loads(self,state):
        if state:
            self.Type = state

## @}

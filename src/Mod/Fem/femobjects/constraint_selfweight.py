# ***************************************************************************
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM constraint self weight document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package constraint_selfweight
#  \ingroup FEM
#  \brief constraint self weight object

import FreeCAD
from . import base_fempythonobject
from femtools import constants


class ConstraintSelfWeight(base_fempythonobject.BaseFemPythonObject):
    """
    The ConstraintSelfWeight object"
    """

    Type = "Fem::ConstraintSelfWeight"

    def __init__(self, obj):
        super().__init__(obj)

        self.addProperty(obj)

        # https://wiki.freecad.org/Scripted_objects#Property_Type
        # https://forum.freecad.org/viewtopic.php?f=18&t=13460&start=20#p109709
        # https://forum.freecad.org/viewtopic.php?t=25524
        # obj.setEditorMode("References", 1)  # read only in PropertyEditor, but writeable by Python
        obj.setEditorMode("References", 2)  # do not show in Editor

    def addProperty(self, obj):
        obj.addProperty(
            "App::PropertyAcceleration", "GravityAcceleration", "Gravity", "Gravity acceleration"
        )
        obj.setPropertyStatus("GravityAcceleration", "LockDynamic")
        obj.GravityAcceleration = constants.gravity()

        obj.addProperty(
            "App::PropertyVector", "GravityDirection", "Gravity", "Normalized gravity direction"
        )
        obj.setPropertyStatus("GravityDirection", "LockDynamic")
        obj.GravityDirection = FreeCAD.Vector(0, 0, -1)

        obj.setPropertyStatus("NormalDirection", "Hidden")

    def onDocumentRestored(self, obj):
        # migrate old App::PropertyFloat "Gravity_*" if exists
        try:
            grav_x = obj.getPropertyByName("Gravity_x")
            grav_y = obj.getPropertyByName("Gravity_y")
            grav_z = obj.getPropertyByName("Gravity_z")
            grav = FreeCAD.Vector(grav_x, grav_y, grav_z)

            self.addProperty(obj)
            obj.GravityAcceleration = constants.gravity()
            obj.GravityAcceleration *= grav.Length
            obj.GravityDirection = grav.normalize()

            obj.removeProperty("Gravity_x")
            obj.removeProperty("Gravity_y")
            obj.removeProperty("Gravity_z")

            return

        except:
            return

    def execute(self, obj):
        obj.GravityDirection.normalize()

        return False

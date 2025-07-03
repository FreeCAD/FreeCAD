# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM base python object"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package base_fempythonobject
#  \ingroup FEM
#  \brief base object for FEM Python Features


class BaseFemPythonObject:

    BaseType = "Fem::BaseFemPythonObject"

    def __init__(self, obj):
        # self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to this object

    # they are needed, see:
    # https://forum.freecad.org/viewtopic.php?f=18&t=44021
    # https://forum.freecad.org/viewtopic.php?f=18&t=44009
    def dumps(self):
        return None

    def loads(self, state):
        return None


class _PropHelper:
    """
    Helper class to manage property data inside proxy objects.
    Initialization keywords are the same used with PropertyContainer
    to add dynamics properties plus "value" for the initial value.
    Note: Is used as base for a GUI version, be aware when refactoring
    """

    def __init__(self, **kwds):
        self.value = kwds.pop("value")
        self.info = kwds
        self.name = kwds["name"]

    def add_to_object(self, obj):
        obj.addProperty(**self.info)
        obj.setPropertyStatus(self.name, "LockDynamic")
        setattr(obj, self.name, self.value)

    def handle_change_type(self, obj, old_type, convert_old_value=lambda x: x):
        if obj.getTypeIdOfProperty(self.name) == old_type:
            new_value = convert_old_value(obj.getPropertyByName(self.name))
            obj.setPropertyStatus(self.name, "-LockDynamic")
            obj.removeProperty(self.name)
            self.add_to_object(obj)
            setattr(obj, self.name, new_value)

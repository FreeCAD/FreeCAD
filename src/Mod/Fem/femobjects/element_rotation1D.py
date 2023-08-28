# ***************************************************************************
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM element rotation 1D document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package element_rotation1D
#  \ingroup FEM
#  \brief element rotation 1D object

from . import base_fempythonobject


class ElementRotation1D(base_fempythonobject.BaseFemPythonObject):
    """
    The ElementRotation1D object
    """

    Type = "Fem::ElementRotation1D"

    def __init__(self, obj):
        super(ElementRotation1D, self).__init__(obj)

        obj.addProperty(
            "App::PropertyAngle",
            "Rotation",
            "BeamRotation",
            "Set the rotation of beam elements"
        )

        obj.addProperty(
            "App::PropertyLinkSubList",
            "References",
            "BeamRotation",
            "List of beam rotation shapes"
        )

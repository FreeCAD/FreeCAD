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

__title__ = "FreeCAD FEM element geometry 1D document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package element_geometry1D
#  \ingroup FEM
#  \brief element geometry 1D object

from . import base_fempythonobject


class ElementGeometry1D(base_fempythonobject.BaseFemPythonObject):
    """
    The ElementGeometry1D object
    """

    Type = "Fem::ElementGeometry1D"
    known_beam_types = ["Rectangular", "Circular", "Pipe"]

    def __init__(self, obj):
        super(ElementGeometry1D, self).__init__(obj)

        obj.addProperty(
            "App::PropertyLength",
            "RectWidth",
            "RectBeamSection",
            "set width of the rectangular beam elements"
        )

        obj.addProperty(
            "App::PropertyLength",
            "RectHeight",
            "RectBeamSection",
            "set height of therectangular beam elements"
        )

        obj.addProperty(
            "App::PropertyLength",
            "CircDiameter",
            "CircBeamSection",
            "set diameter of the circular beam elements"
        )

        obj.addProperty(
            "App::PropertyLength",
            "PipeDiameter",
            "PipeBeamSection",
            "set outer diameter of the pipe beam elements"
        )

        obj.addProperty(
            "App::PropertyLength",
            "PipeThickness",
            "PipeBeamSection",
            "set thickness of the pipe beam elements"
        )

        obj.addProperty(
            "App::PropertyEnumeration",
            "SectionType",
            "BeamSection",
            "select beam section type"
        )

        obj.addProperty(
            "App::PropertyLinkSubList",
            "References",
            "BeamSection",
            "List of beam section shapes"
        )

        obj.SectionType = ElementGeometry1D.known_beam_types
        obj.SectionType = "Rectangular"

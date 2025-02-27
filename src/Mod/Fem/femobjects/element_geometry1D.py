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

from FreeCAD import Base
from . import base_femelement
from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class ElementGeometry1D(base_femelement.BaseFemElement):
    """
    The ElementGeometry1D object
    """

    Type = "Fem::ElementGeometry1D"

    def __init__(self, obj):
        super().__init__(obj)

    def _get_properties(self):
        prop = super()._get_properties()

        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="RectWidth",
                group="RectBeamSection",
                doc="Set width of the rectangular beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="RectHeight",
                group="RectBeamSection",
                doc="Set height of there ctangular beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="CircDiameter",
                group="CircBeamSection",
                doc="Set diameter of the circular beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="PipeDiameter",
                group="PipeBeamSection",
                doc="Set outer diameter of the pipe beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="PipeThickness",
                group="PipeBeamSection",
                doc="Set thickness of the pipe beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="Axis1Length",
                group="EllipticalBeamSection",
                doc="Set first principal axis length of the elliptical beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="Axis2Length",
                group="EllipticalBeamSection",
                doc="Set second principal axis length of the elliptical beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="BoxWidth",
                group="BoxBeamSection",
                doc="Set width of the box beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="BoxHeight",
                group="BoxBeamSection",
                doc="Set height of the box beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="BoxT1",
                group="BoxBeamSection",
                doc="Set thickness parameter t1 of the box beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="BoxT2",
                group="BoxBeamSection",
                doc="Set thickness parameter t2 of the box beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="BoxT3",
                group="BoxBeamSection",
                doc="Set thickness parameter t3 of the box beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="BoxT4",
                group="BoxBeamSection",
                doc="Set thickness parameter t4 of the box beam elements",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="SectionType",
                group="BeamSection",
                doc="Select beam section type",
                value=["Rectangular", "Circular", "Pipe", "Elliptical", "Box"],
            )
        )

        return prop

    def onDocumentRestored(self, obj):
        # update old project with new properties
        for prop in self._get_properties():
            try:
                obj.getPropertyByName(prop.name)
            except Base.PropertyError:
                prop.add_to_object(obj)

            if prop.name == "SectionType":
                # refresh the list of known section types for old projects
                obj.SectionType = prop.value

# ***************************************************************************
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2026 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "FreeCAD FEM material mechanical nonlinear document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package material_mechanicalnonlinear
#  \ingroup FEM
#  \brief nonlinear mechanical material object

from FreeCAD import Base

from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class MaterialMechanicalNonlinear(base_fempythonobject.BaseFemPythonObject):
    """
    The MaterialMechanicalNonlinear object
    """

    Type = "Fem::MaterialMechanicalNonlinear"

    def __init__(self, obj):
        super().__init__(obj)

        for prop in self._get_properties():
            prop.add_to_object(obj)

        obj.addExtension("App::SuppressibleExtensionPython")

    def _get_properties(self):
        prop = []

        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="MaterialModelNonlinearity",
                group="Material",
                doc="Set the type on nonlinear material model",
                value=["isotropic hardening", "kinematic hardening"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyStringList",
                name="YieldPoints",
                group="Material",
                doc="Set stress and strain for yield points as a list of strings,\n"
                + "each point 'stress, plastic strain'",
                value=[],
            )
        )

        return prop

    def onDocumentRestored(self, obj):
        # update old project with new properties
        for prop in self._get_properties():
            try:
                value = obj.getPropertyByName(prop.name)
            except Base.PropertyError:
                prop.add_to_object(obj)

            # update material model enum
            if prop.name == "MaterialModelNonlinearity" and value == "simple hardening":
                obj.MaterialModelNonlinearity = prop.value

        # YieldPoints was (until 0.19) stored as 3 separate variables.
        try:
            yp1 = obj.getPropertyByName("YieldPoint1")
            yp2 = obj.getPropertyByName("YieldPoint2")
            yp3 = obj.getPropertyByName("YieldPoint3")
            obj.setPropertyStatus("YieldPoint1", "-LockDynamic")
            obj.setPropertyStatus("YieldPoint2", "-LockDynamic")
            obj.setPropertyStatus("YieldPoint3", "-LockDynamic")
            obj.removeProperty("YieldPoint1")
            obj.removeProperty("YieldPoint2")
            obj.removeProperty("YieldPoint3")

            obj.YieldPoints = [yp1, yp2, yp3]
        except Base.PropertyError:
            # do nothing
            pass

        # set reference on base linear material
        try:
            # invert dependency with base material
            base = obj.getPropertyByName("LinearBaseMaterial")
            obj.LinearBaseMaterial = None
            obj.setPropertyStatus("LinearBaseMaterial", "-LockDynamic")
            obj.removeProperty("LinearBaseMaterial")
            # remove from analysis group
            analysis = obj.getParent()
            if analysis is not None:
                analysis.removeObject(obj)
            if base is not None:
                base.Nonlinear = obj
                base.purgeTouched()
        except Base.PropertyError:
            # do nothing
            pass

        if not obj.hasExtension("App::SuppressibleExtensionPython"):
            obj.addExtension("App::SuppressibleExtensionPython")


/***************************************************************************
 *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "FemShapeExtension.h"

using namespace Fem;
EXTENSION_PROPERTY_SOURCE(Fem::BoxExtension, App::DocumentObjectExtension)
EXTENSION_PROPERTY_SOURCE(Fem::CylinderExtension, App::DocumentObjectExtension)
EXTENSION_PROPERTY_SOURCE(Fem::SphereExtension, App::DocumentObjectExtension)
EXTENSION_PROPERTY_SOURCE(Fem::PlaneExtension, App::DocumentObjectExtension)

namespace App
{
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Fem::BoxExtensionPython, Fem::BoxExtension)
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Fem::CylinderExtensionPython, Fem::CylinderExtension)
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Fem::SphereExtensionPython, Fem::SphereExtension)
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Fem::PlaneExtensionPython, Fem::PlaneExtension)

// explicit template instantiation
template class FemExport App::ExtensionPythonT<BoxExtension>;
template class FemExport App::ExtensionPythonT<CylinderExtension>;
template class FemExport App::ExtensionPythonT<SphereExtension>;
template class FemExport App::ExtensionPythonT<PlaneExtension>;

}  // namespace App

BoxExtension::BoxExtension()
{
    initExtensionType(BoxExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY_TYPE(Center,
                                (Base::Vector3d(0.0, 0.0, 0.0)),
                                "Box",
                                (App::PropertyType)(App::Prop_None),
                                "the center point of the box");

    EXTENSION_ADD_PROPERTY_TYPE(Length,
                                (10),
                                "Box",
                                (App::PropertyType)(App::Prop_None),
                                "The length of the box (along X axis)");

    EXTENSION_ADD_PROPERTY_TYPE(Width,
                                (10),
                                "Box",
                                (App::PropertyType)(App::Prop_None),
                                "The width of the box (along Y axis)");

    EXTENSION_ADD_PROPERTY_TYPE(Height,
                                (10),
                                "Box",
                                (App::PropertyType)(App::Prop_None),
                                "The height of the box (along Z axis)");
}

BoxExtension::~BoxExtension() = default;


CylinderExtension::CylinderExtension()
{
    initExtensionType(CylinderExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY_TYPE(Center,
                                (Base::Vector3d(0.0, 0.0, 0.0)),
                                "Cylinder",
                                (App::PropertyType)(App::Prop_None),
                                "The center point of the cylinder");

    EXTENSION_ADD_PROPERTY_TYPE(Axis,
                                (Base::Vector3d(0.0, 0.0, 1.0)),
                                "Cylinder",
                                (App::PropertyType)(App::Prop_None),
                                "The axis along wich the cylinder is defined");

    EXTENSION_ADD_PROPERTY_TYPE(Radius,
                                (5),
                                "Cylinder",
                                (App::PropertyType)(App::Prop_None),
                                "The cylinders radius");
}

CylinderExtension::~CylinderExtension() = default;


SphereExtension::SphereExtension()
{
    initExtensionType(SphereExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY_TYPE(Center,
                                (Base::Vector3d(0.0, 0.0, 0.0)),
                                "Sphere",
                                (App::PropertyType)(App::Prop_None),
                                "The center point of the sphere");

    EXTENSION_ADD_PROPERTY_TYPE(Radius,
                                (5),
                                "Sphere",
                                (App::PropertyType)(App::Prop_None),
                                "The sphere radius");
}

SphereExtension::~SphereExtension() = default;


PlaneExtension::PlaneExtension()
{
    initExtensionType(PlaneExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY_TYPE(Origin,
                                (Base::Vector3d(0.0, 0.0, 0.0)),
                                "Plane",
                                (App::PropertyType)(App::Prop_None),
                                "The origin of the plane");

    EXTENSION_ADD_PROPERTY_TYPE(Normal,
                                (Base::Vector3d(0.0, 0.0, 1.0)),
                                "Plane",
                                (App::PropertyType)(App::Prop_None),
                                "The normal direction of the plane");
}

PlaneExtension::~PlaneExtension() = default;

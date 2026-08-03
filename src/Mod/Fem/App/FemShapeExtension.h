// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Stefan Tröger <stefantroeger@gmx.net>
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#pragma once

#include <App/DocumentObjectExtension.h>
#include <App/ExtensionPython.h>
#include <App/PropertyUnits.h>
#include <App/PropertyGeo.h>

#include <Mod/Fem/FemGlobal.h>


namespace Fem
{

class BoxExtensionPy;

// Axis aligned box
class FemExport BoxExtension: public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Fem::BoxExtension);
    using inherited = App::DocumentObjectExtension;

public:
    /// Constructor
    BoxExtension();
    ~BoxExtension() override;

    /// Properties
    App::PropertyVectorDistance BoxCenter;
    App::PropertyDistance BoxLength;
    App::PropertyDistance BoxWidth;
    App::PropertyDistance BoxHeight;
};

using BoxExtensionPython = App::ExtensionPythonT<BoxExtension>;


// Cylinder
class FemExport CylinderExtension: public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Fem::CylinderExtension);
    using inherited = App::DocumentObjectExtension;

public:
    /// Constructor
    CylinderExtension();
    ~CylinderExtension() override;

    /// Properties
    App::PropertyVector CylinderAxis;
    App::PropertyVectorDistance CylinderCenter;
    App::PropertyDistance CylinderRadius;
};

using CylinderExtensionPython = App::ExtensionPythonT<CylinderExtension>;


// Sphere
class FemExport SphereExtension: public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Fem::SphereExtension);
    using inherited = App::DocumentObjectExtension;

public:
    /// Constructor
    SphereExtension();
    ~SphereExtension() override;

    /// Properties
    App::PropertyDistance SphereRadius;
    App::PropertyVectorDistance SphereCenter;
};

using SphereExtensionPython = App::ExtensionPythonT<SphereExtension>;


// Plane
class FemExport PlaneExtension: public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Fem::PlaneExtension);
    using inherited = App::DocumentObjectExtension;

public:
    /// Constructor
    PlaneExtension();
    ~PlaneExtension() override;

    /// Properties
    App::PropertyVector PlaneNormal;
    App::PropertyVectorDistance PlaneOrigin;
};


using PlaneExtensionPython = App::ExtensionPythonT<PlaneExtension>;


}  // namespace Fem

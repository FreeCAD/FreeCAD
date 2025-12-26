/***************************************************************************
 *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net               *
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


#ifndef FEM_FEMSHAPEMANIPULATOREXTENSION_H
#define FEM_FEMSHAPEMANIPULATOREXTENSION_H

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


#endif  // FEM_FEMSHAPEMANIPULATOREXTENSION_H

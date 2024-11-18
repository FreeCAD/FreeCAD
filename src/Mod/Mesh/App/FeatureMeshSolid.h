/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef FEATURE_MESH_SOLID_H
#define FEATURE_MESH_SOLID_H

#include "MeshFeature.h"

#include <App/PropertyUnits.h>


namespace Mesh
{

/**
 * @author Werner Mayer
 */
class Sphere: public Mesh::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Mesh::Sphere);

public:
    Sphere();

    App::PropertyLength Radius;
    App::PropertyIntegerConstraint Sampling;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    void handleChangedPropertyType(Base::XMLReader& reader,
                                   const char* TypeName,
                                   App::Property* prop) override;
    //@}
};

// -------------------------------------------------------------

class Ellipsoid: public Mesh::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Mesh::Ellipsoid);

public:
    Ellipsoid();

    App::PropertyLength Radius1;
    App::PropertyLength Radius2;
    App::PropertyIntegerConstraint Sampling;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    void handleChangedPropertyType(Base::XMLReader& reader,
                                   const char* TypeName,
                                   App::Property* prop) override;
    //@}
};

// -------------------------------------------------------------

class Cylinder: public Mesh::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Mesh::Cylinder);

public:
    Cylinder();

    App::PropertyLength Radius;
    App::PropertyLength Length;
    App::PropertyLength EdgeLength;
    App::PropertyBool Closed;
    App::PropertyIntegerConstraint Sampling;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    void handleChangedPropertyType(Base::XMLReader& reader,
                                   const char* TypeName,
                                   App::Property* prop) override;
    //@}
};

// -------------------------------------------------------------

class Cone: public Mesh::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Mesh::Cone);

public:
    Cone();

    App::PropertyLength Radius1;
    App::PropertyLength Radius2;
    App::PropertyLength Length;
    App::PropertyLength EdgeLength;
    App::PropertyBool Closed;
    App::PropertyIntegerConstraint Sampling;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    void handleChangedPropertyType(Base::XMLReader& reader,
                                   const char* TypeName,
                                   App::Property* prop) override;
    //@}
};

// -------------------------------------------------------------

class Torus: public Mesh::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Mesh::Torus);

public:
    Torus();

    App::PropertyLength Radius1;
    App::PropertyLength Radius2;
    App::PropertyIntegerConstraint Sampling;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    void handleChangedPropertyType(Base::XMLReader& reader,
                                   const char* TypeName,
                                   App::Property* prop) override;
    //@}
};

// -------------------------------------------------------------

class Cube: public Mesh::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Mesh::Cube);

public:
    Cube();

    App::PropertyLength Length;
    App::PropertyLength Width;
    App::PropertyLength Height;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    void handleChangedPropertyType(Base::XMLReader& reader,
                                   const char* TypeName,
                                   App::Property* prop) override;
    //@}
};

}  // namespace Mesh

#endif  // FEATURE_MESH_SOLID_H

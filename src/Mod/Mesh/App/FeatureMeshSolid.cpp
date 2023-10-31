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

#include "PreCompiled.h"

#include <App/Document.h>

#include "FeatureMeshSolid.h"


namespace Mesh
{
const App::PropertyIntegerConstraint::Constraints intSampling = {0, INT_MAX, 1};
const App::PropertyLength::Constraints floatRange = {0.0, FLT_MAX, 1.0};
}  // namespace Mesh

using namespace Mesh;
using namespace MeshCore;

PROPERTY_SOURCE(Mesh::Sphere, Mesh::Feature)

Sphere::Sphere()
{
    ADD_PROPERTY(Radius, (5.0));
    ADD_PROPERTY(Sampling, (50));
    Radius.setConstraints(&floatRange);
    Sampling.setConstraints(&intSampling);
}

short Sphere::mustExecute() const
{
    if (Radius.isTouched() || Sampling.isTouched()) {
        return 1;
    }
    return Feature::mustExecute();
}

App::DocumentObjectExecReturn* Sphere::execute()
{
    std::unique_ptr<MeshObject> mesh(
        MeshObject::createSphere((float)Radius.getValue(), Sampling.getValue()));
    if (mesh.get()) {
        mesh->setPlacement(this->Placement.getValue());
        Mesh.setValue(mesh->getKernel());
        return App::DocumentObject::StdReturn;
    }
    else {
        return new App::DocumentObjectExecReturn("Cannot create sphere", this);
    }
}

void Sphere::handleChangedPropertyType(Base::XMLReader& reader,
                                       const char* TypeName,
                                       App::Property* prop)
{
    if (prop == &Radius && strcmp(TypeName, "App::PropertyFloatConstraint") == 0) {
        App::PropertyFloatConstraint r;
        r.Restore(reader);
        Radius.setValue(r.getValue());
    }
    else {
        Mesh::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

// -------------------------------------------------------------

PROPERTY_SOURCE(Mesh::Ellipsoid, Mesh::Feature)

Ellipsoid::Ellipsoid()
{
    ADD_PROPERTY(Radius1, (2.0));
    ADD_PROPERTY(Radius2, (4.0));
    ADD_PROPERTY(Sampling, (50));
    Radius1.setConstraints(&floatRange);
    Radius2.setConstraints(&floatRange);
    Sampling.setConstraints(&intSampling);
}

short Ellipsoid::mustExecute() const
{
    if (Radius1.isTouched() || Radius2.isTouched() || Sampling.isTouched()) {
        return 1;
    }
    return Feature::mustExecute();
}

App::DocumentObjectExecReturn* Ellipsoid::execute()
{
    std::unique_ptr<MeshObject> mesh(MeshObject::createEllipsoid((float)Radius1.getValue(),
                                                                 (float)Radius2.getValue(),
                                                                 Sampling.getValue()));
    if (mesh.get()) {
        mesh->setPlacement(this->Placement.getValue());
        Mesh.setValue(mesh->getKernel());
        return App::DocumentObject::StdReturn;
    }
    else {
        return new App::DocumentObjectExecReturn("Cannot create ellipsoid", this);
    }
}

void Ellipsoid::handleChangedPropertyType(Base::XMLReader& reader,
                                          const char* TypeName,
                                          App::Property* prop)
{
    if ((prop == &Radius1 || prop == &Radius2)
        && strcmp(TypeName, "App::PropertyFloatConstraint") == 0) {
        App::PropertyFloatConstraint r;
        r.Restore(reader);
        static_cast<App::PropertyLength*>(prop)->setValue(r.getValue());
    }
    else {
        Mesh::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

// -------------------------------------------------------------

PROPERTY_SOURCE(Mesh::Cylinder, Mesh::Feature)

Cylinder::Cylinder()
{
    ADD_PROPERTY(Radius, (2.0));
    ADD_PROPERTY(Length, (10.0));
    ADD_PROPERTY(EdgeLength, (1.0));
    ADD_PROPERTY(Closed, (true));
    ADD_PROPERTY(Sampling, (50));
    Radius.setConstraints(&floatRange);
    Length.setConstraints(&floatRange);
    EdgeLength.setConstraints(&floatRange);
    Sampling.setConstraints(&intSampling);
}

short Cylinder::mustExecute() const
{
    if (Radius.isTouched() || Length.isTouched() || EdgeLength.isTouched() || Closed.isTouched()
        || Sampling.isTouched()) {
        return 1;
    }
    return Feature::mustExecute();
}

App::DocumentObjectExecReturn* Cylinder::execute()
{
    std::unique_ptr<MeshObject> mesh(MeshObject::createCylinder((float)Radius.getValue(),
                                                                (float)Length.getValue(),
                                                                Closed.getValue(),
                                                                (float)EdgeLength.getValue(),
                                                                Sampling.getValue()));
    if (mesh.get()) {
        mesh->setPlacement(this->Placement.getValue());
        Mesh.setValue(mesh->getKernel());
        return App::DocumentObject::StdReturn;
    }
    else {
        return new App::DocumentObjectExecReturn("Cannot create cylinder", this);
    }
}

void Cylinder::handleChangedPropertyType(Base::XMLReader& reader,
                                         const char* TypeName,
                                         App::Property* prop)
{
    if ((prop == &Radius || prop == &Length || prop == &EdgeLength)
        && strcmp(TypeName, "App::PropertyFloatConstraint") == 0) {
        App::PropertyFloatConstraint r;
        r.Restore(reader);
        static_cast<App::PropertyLength*>(prop)->setValue(r.getValue());
    }
    else {
        Mesh::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

// -------------------------------------------------------------

PROPERTY_SOURCE(Mesh::Cone, Mesh::Feature)

Cone::Cone()
{
    ADD_PROPERTY(Radius1, (2.0));
    ADD_PROPERTY(Radius2, (4.0));
    ADD_PROPERTY(Length, (10.0));
    ADD_PROPERTY(EdgeLength, (1.0));
    ADD_PROPERTY(Closed, (true));
    ADD_PROPERTY(Sampling, (50));
    Radius1.setConstraints(&floatRange);
    Radius2.setConstraints(&floatRange);
    Length.setConstraints(&floatRange);
    EdgeLength.setConstraints(&floatRange);
    Sampling.setConstraints(&intSampling);
}

short Cone::mustExecute() const
{
    if (Radius1.isTouched() || Radius2.isTouched() || Length.isTouched() || EdgeLength.isTouched()
        || Closed.isTouched() || Sampling.isTouched()) {
        return 1;
    }
    return Feature::mustExecute();
}

App::DocumentObjectExecReturn* Cone::execute()
{
    std::unique_ptr<MeshObject> mesh(MeshObject::createCone((float)Radius1.getValue(),
                                                            (float)Radius2.getValue(),
                                                            (float)Length.getValue(),
                                                            Closed.getValue(),
                                                            (float)EdgeLength.getValue(),
                                                            Sampling.getValue()));
    if (mesh.get()) {
        mesh->setPlacement(this->Placement.getValue());
        Mesh.setValue(mesh->getKernel());
        return App::DocumentObject::StdReturn;
    }
    else {
        return new App::DocumentObjectExecReturn("Cannot create cone", this);
    }
}

void Cone::handleChangedPropertyType(Base::XMLReader& reader,
                                     const char* TypeName,
                                     App::Property* prop)
{
    if ((prop == &Radius1 || prop == &Radius2 || prop == &Length || prop == &EdgeLength)
        && strcmp(TypeName, "App::PropertyFloatConstraint") == 0) {
        App::PropertyFloatConstraint r;
        r.Restore(reader);
        static_cast<App::PropertyLength*>(prop)->setValue(r.getValue());
    }
    else {
        Mesh::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

// -------------------------------------------------------------

PROPERTY_SOURCE(Mesh::Torus, Mesh::Feature)

Torus::Torus()
{
    ADD_PROPERTY(Radius1, (10.0));
    ADD_PROPERTY(Radius2, (2.0));
    ADD_PROPERTY(Sampling, (50));
    Radius1.setConstraints(&floatRange);
    Radius2.setConstraints(&floatRange);
    Sampling.setConstraints(&intSampling);
}

short Torus::mustExecute() const
{
    if (Radius1.isTouched() || Radius2.isTouched() || Sampling.isTouched()) {
        return 1;
    }
    return Feature::mustExecute();
}

App::DocumentObjectExecReturn* Torus::execute()
{
    std::unique_ptr<MeshObject> mesh(MeshObject::createTorus((float)Radius1.getValue(),
                                                             (float)Radius2.getValue(),
                                                             Sampling.getValue()));
    if (mesh.get()) {
        mesh->setPlacement(this->Placement.getValue());
        Mesh.setValue(mesh->getKernel());
        return App::DocumentObject::StdReturn;
    }
    else {
        return new App::DocumentObjectExecReturn("Cannot create torus", this);
    }
}

void Torus::handleChangedPropertyType(Base::XMLReader& reader,
                                      const char* TypeName,
                                      App::Property* prop)
{
    if ((prop == &Radius1 || prop == &Radius2)
        && strcmp(TypeName, "App::PropertyFloatConstraint") == 0) {
        App::PropertyFloatConstraint r;
        r.Restore(reader);
        static_cast<App::PropertyLength*>(prop)->setValue(r.getValue());
    }
    else {
        Mesh::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

// -------------------------------------------------------------

PROPERTY_SOURCE(Mesh::Cube, Mesh::Feature)

Cube::Cube()
{
    ADD_PROPERTY_TYPE(Length, (10.0f), "Cube", App::Prop_None, "The length of the cube");
    ADD_PROPERTY_TYPE(Width, (10.0f), "Cube", App::Prop_None, "The width of the cube");
    ADD_PROPERTY_TYPE(Height, (10.0f), "Cube", App::Prop_None, "The height of the cube");
    Length.setConstraints(&floatRange);
    Width.setConstraints(&floatRange);
    Height.setConstraints(&floatRange);
}

short Cube::mustExecute() const
{
    if (Length.isTouched() || Width.isTouched() || Height.isTouched()) {
        return 1;
    }
    return Feature::mustExecute();
}

App::DocumentObjectExecReturn* Cube::execute()
{
    std::unique_ptr<MeshObject> mesh(MeshObject::createCube((float)Length.getValue(),
                                                            (float)Width.getValue(),
                                                            (float)Height.getValue()));
    if (mesh.get()) {
        mesh->setPlacement(this->Placement.getValue());
        Mesh.setValue(mesh->getKernel());
        return App::DocumentObject::StdReturn;
    }
    else {
        return new App::DocumentObjectExecReturn("Cannot create cube", this);
    }
}

void Cube::handleChangedPropertyType(Base::XMLReader& reader,
                                     const char* TypeName,
                                     App::Property* prop)
{
    if ((prop == &Length || prop == &Width || prop == &Height)
        && strcmp(TypeName, "App::PropertyFloatConstraint") == 0) {
        App::PropertyFloatConstraint r;
        r.Restore(reader);
        static_cast<App::PropertyLength*>(prop)->setValue(r.getValue());
    }
    else {
        Mesh::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

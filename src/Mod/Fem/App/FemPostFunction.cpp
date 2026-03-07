/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#include "FemPostFunction.h"
#include <App/Document.h>


using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostFunctionProvider, App::DocumentObject)

FemPostFunctionProvider::FemPostFunctionProvider()
    : DocumentObjectGroup()
{}

FemPostFunctionProvider::~FemPostFunctionProvider() = default;

bool FemPostFunctionProvider::allowObject(App::DocumentObject* obj)
{
    return obj->isDerivedFrom<FemPostFunction>();
}

void FemPostFunctionProvider::unsetupObject()
{
    // remove all children!
    auto document = getExtendedObject()->getDocument();
    for (const auto& obj : Group.getValues()) {
        document->removeObject(obj->getNameInDocument());
    }
}

void FemPostFunctionProvider::handleChangedPropertyName(
    Base::XMLReader& reader,
    const char* typeName,
    const char* propName
)
{
    if (strcmp(propName, "Functions") == 0
        && Base::Type::fromName(typeName) == App::PropertyLinkList::getClassTypeId()) {

        // restore the property into Group, instead of the old Functions property
        Group.Restore(reader);
    }
    else {
        App::DocumentObject::handleChangedPropertyName(reader, typeName, propName);
    }
}

PROPERTY_SOURCE(Fem::FemPostFunction, App::DocumentObject)

FemPostFunction::FemPostFunction() = default;

FemPostFunction::~FemPostFunction() = default;

DocumentObjectExecReturn* FemPostFunction::execute()
{

    return DocumentObject::StdReturn;
}

// ***************************************************************************
// box function
PROPERTY_SOURCE_WITH_EXTENSIONS(Fem::FemPostBoxFunction, Fem::FemPostFunction)

FemPostBoxFunction::FemPostBoxFunction()
    : FemPostFunction()
    , BoxExtension()
{
    BoxExtension::initExtension(this);

    m_box = vtkSmartPointer<vtkBox>::New();
    m_implicit = m_box;

    m_box->SetBounds(-5.0, 5.0, -5.0, 5.0, -5.0, 5.0);
}

FemPostBoxFunction::~FemPostBoxFunction() = default;

void FemPostBoxFunction::onChanged(const Property* prop)
{
    if (prop == &BoxCenter || prop == &BoxLength || prop == &BoxWidth || prop == &BoxHeight) {
        const Base::Vector3d& vec = BoxCenter.getValue();
        float l = BoxLength.getValue();
        float w = BoxWidth.getValue();
        float h = BoxHeight.getValue();
        m_box->SetBounds(
            vec[0] - l / 2,
            vec[0] + l / 2,
            vec[1] - w / 2,
            vec[1] + w / 2,
            vec[2] - h / 2,
            vec[2] + h / 2
        );
    }

    Fem::FemPostFunction::onChanged(prop);
}

void FemPostBoxFunction::onDocumentRestored()
{
    // This is to notify the view provider that the document has been fully restored
    BoxCenter.touch();
}

void FemPostBoxFunction::handleChangedPropertyName(
    Base::XMLReader& reader,
    const char* typeName,
    const char* propName
)
{

    App::PropertyVectorDistance BoxCenter;
    App::PropertyDistance BoxLength;
    App::PropertyDistance BoxWidth;
    App::PropertyDistance BoxHeight;

    if (strcmp(propName, "Center") == 0
        && Base::Type::fromName(typeName) == App::PropertyVectorDistance::getClassTypeId()) {

        BoxCenter.Restore(reader);
    }

    if (strcmp(propName, "Length") == 0
        && Base::Type::fromName(typeName) == App::PropertyDistance::getClassTypeId()) {

        BoxLength.Restore(reader);
    }

    if (strcmp(propName, "Width") == 0
        && Base::Type::fromName(typeName) == App::PropertyDistance::getClassTypeId()) {

        BoxWidth.Restore(reader);
    }

    if (strcmp(propName, "Height") == 0
        && Base::Type::fromName(typeName) == App::PropertyDistance::getClassTypeId()) {

        BoxHeight.Restore(reader);
    }
}


// ***************************************************************************
// cylinder function
PROPERTY_SOURCE_WITH_EXTENSIONS(Fem::FemPostCylinderFunction, Fem::FemPostFunction)

FemPostCylinderFunction::FemPostCylinderFunction()
    : FemPostFunction()
    , CylinderExtension()
{
    CylinderExtension::initExtension(this);

    m_cylinder = vtkSmartPointer<vtkCylinder>::New();
    m_implicit = m_cylinder;

    m_cylinder->SetAxis(0.0, 0.0, 1.0);
    m_cylinder->SetCenter(0.0, 0.0, 0.0);
    m_cylinder->SetRadius(5.0);
}

FemPostCylinderFunction::~FemPostCylinderFunction() = default;

void FemPostCylinderFunction::onChanged(const Property* prop)
{
    if (prop == &CylinderAxis) {
        const Base::Vector3d& vec = CylinderAxis.getValue();
        m_cylinder->SetAxis(vec[0], vec[1], vec[2]);
    }
    else if (prop == &CylinderCenter) {
        const Base::Vector3d& vec = CylinderCenter.getValue();
        m_cylinder->SetCenter(vec[0], vec[1], vec[2]);
    }
    else if (prop == &CylinderRadius) {
        m_cylinder->SetRadius(CylinderRadius.getValue());
    }

    Fem::FemPostFunction::onChanged(prop);
}

void FemPostCylinderFunction::onDocumentRestored()
{
    // This is to notify the view provider that the document has been fully restored
    CylinderAxis.touch();
}

void FemPostCylinderFunction::handleChangedPropertyName(
    Base::XMLReader& reader,
    const char* typeName,
    const char* propName
)
{
    if (strcmp(propName, "Radius") == 0
        && Base::Type::fromName(typeName) == App::PropertyDistance::getClassTypeId()) {

        CylinderRadius.Restore(reader);
    }

    if (strcmp(propName, "Center") == 0
        && Base::Type::fromName(typeName) == App::PropertyVectorDistance::getClassTypeId()) {

        CylinderCenter.Restore(reader);
    }

    if (strcmp(propName, "Axis") == 0
        && Base::Type::fromName(typeName) == App::PropertyVector::getClassTypeId()) {

        CylinderAxis.Restore(reader);
    }
}


// ***************************************************************************
// plane function
PROPERTY_SOURCE_WITH_EXTENSIONS(Fem::FemPostPlaneFunction, Fem::FemPostFunction)

FemPostPlaneFunction::FemPostPlaneFunction()
    : FemPostFunction()
    , PlaneExtension()
{

    PlaneExtension::initExtension(this);

    m_plane = vtkSmartPointer<vtkPlane>::New();
    m_implicit = m_plane;

    m_plane->SetOrigin(0.0, 0.0, 0.0);
    m_plane->SetNormal(0.0, 0.0, 1.0);
}

FemPostPlaneFunction::~FemPostPlaneFunction() = default;

void FemPostPlaneFunction::onChanged(const Property* prop)
{

    if (prop == &PlaneOrigin) {
        const Base::Vector3d& vec = PlaneOrigin.getValue();
        m_plane->SetOrigin(vec[0], vec[1], vec[2]);
    }
    else if (prop == &PlaneNormal) {
        const Base::Vector3d& vec = PlaneNormal.getValue();
        m_plane->SetNormal(vec[0], vec[1], vec[2]);
    }

    Fem::FemPostFunction::onChanged(prop);
}

void FemPostPlaneFunction::onDocumentRestored()
{
    // This is to notify the view provider that the document has been fully restored
    PlaneNormal.touch();
}

void FemPostPlaneFunction::handleChangedPropertyName(
    Base::XMLReader& reader,
    const char* typeName,
    const char* propName
)
{
    if (strcmp(propName, "Normal") == 0
        && Base::Type::fromName(typeName) == App::PropertyVector::getClassTypeId()) {

        PlaneNormal.Restore(reader);
    }

    if (strcmp(propName, "Origin") == 0
        && Base::Type::fromName(typeName) == App::PropertyVectorDistance::getClassTypeId()) {

        PlaneOrigin.Restore(reader);
    }
}


// ***************************************************************************
// sphere function
PROPERTY_SOURCE_WITH_EXTENSIONS(Fem::FemPostSphereFunction, Fem::FemPostFunction)

FemPostSphereFunction::FemPostSphereFunction()
    : FemPostFunction()
    , SphereExtension()
{
    SphereExtension::initExtension(this);

    m_sphere = vtkSmartPointer<vtkSphere>::New();
    m_implicit = m_sphere;

    m_sphere->SetCenter(0.0, 0.0, 0.0);
    m_sphere->SetRadius(5.0);
}

FemPostSphereFunction::~FemPostSphereFunction() = default;

void FemPostSphereFunction::onChanged(const Property* prop)
{

    if (prop == &SphereCenter) {
        const Base::Vector3d& vec = SphereCenter.getValue();
        m_sphere->SetCenter(vec[0], vec[1], vec[2]);
    }
    else if (prop == &SphereRadius) {
        m_sphere->SetRadius(SphereRadius.getValue());
    }

    Fem::FemPostFunction::onChanged(prop);
}

void FemPostSphereFunction::handleChangedPropertyName(
    Base::XMLReader& reader,
    const char* typeName,
    const char* propName
)
{
    if (strcmp(propName, "Radius") == 0
        && Base::Type::fromName(typeName) == App::PropertyDistance::getClassTypeId()) {

        SphereRadius.Restore(reader);
    }

    if (strcmp(propName, "Center") == 0
        && Base::Type::fromName(typeName) == App::PropertyVectorDistance::getClassTypeId()) {

        SphereCenter.Restore(reader);
    }
}

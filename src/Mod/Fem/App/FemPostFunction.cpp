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


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "FemPostFunction.h"
#include <Base/Console.h>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostFunctionProvider, App::DocumentObject)

FemPostFunctionProvider::FemPostFunctionProvider(void) : DocumentObject() {

    ADD_PROPERTY(Functions, (nullptr));
}

FemPostFunctionProvider::~FemPostFunctionProvider() {

}

void FemPostFunctionProvider::onChanged(const Property* prop) {
    App::DocumentObject::onChanged(prop);
}


PROPERTY_SOURCE(Fem::FemPostFunction, App::DocumentObject)

FemPostFunction::FemPostFunction()
{
}

FemPostFunction::~FemPostFunction()
{
}

DocumentObjectExecReturn* FemPostFunction::execute(void) {

    return DocumentObject::StdReturn;
}


// ***************************************************************************
// plane function
PROPERTY_SOURCE(Fem::FemPostPlaneFunction, Fem::FemPostFunction)

FemPostPlaneFunction::FemPostPlaneFunction(void) : FemPostFunction() {

    ADD_PROPERTY(Origin, (Base::Vector3d(0.0, 0.0, 0.0)));
    ADD_PROPERTY(Normal, (Base::Vector3d(0.0, 0.0, 1.0)));

    m_plane = vtkSmartPointer<vtkPlane>::New();
    m_implicit = m_plane;

    m_plane->SetOrigin(0., 0., 0.);
    m_plane->SetNormal(0., 0., 1.);
}

FemPostPlaneFunction::~FemPostPlaneFunction() {

}

void FemPostPlaneFunction::onChanged(const Property* prop) {

    if (prop == &Origin) {
        const Base::Vector3d& vec = Origin.getValue();
        m_plane->SetOrigin(vec[0], vec[1], vec[2]);
    }
    else if (prop == &Normal) {
        const Base::Vector3d& vec = Normal.getValue();
        m_plane->SetNormal(vec[0], vec[1], vec[2]);
    }

    Fem::FemPostFunction::onChanged(prop);
}

void FemPostPlaneFunction::onDocumentRestored() {
    // This is to notify the view provider that the document has been fully restored
    Normal.touch();
}


// ***************************************************************************
// sphere function
PROPERTY_SOURCE(Fem::FemPostSphereFunction, Fem::FemPostFunction)

FemPostSphereFunction::FemPostSphereFunction(void) : FemPostFunction() {

    ADD_PROPERTY(Radius, (5));
    ADD_PROPERTY(Center, (Base::Vector3d(1.0, 0.0, 0.0)));

    m_sphere = vtkSmartPointer<vtkSphere>::New();
    m_implicit = m_sphere;

    m_sphere->SetCenter(0., 0., 0.);
    m_sphere->SetRadius(5);
}

FemPostSphereFunction::~FemPostSphereFunction() {

}

void FemPostSphereFunction::onChanged(const Property* prop) {

    if (prop == &Center) {
        const Base::Vector3d& vec = Center.getValue();
        m_sphere->SetCenter(vec[0], vec[1], vec[2]);
    }
    else if (prop == &Radius) {
        m_sphere->SetRadius(Radius.getValue());
    }

    Fem::FemPostFunction::onChanged(prop);
}

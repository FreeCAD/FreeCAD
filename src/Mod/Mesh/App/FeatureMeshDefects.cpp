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

#ifndef _PreComp_
#endif

#include "FeatureMeshDefects.h"
#include "Core/Degeneration.h"
#include "Core/TopoAlgorithm.h"
#include "Core/Triangulation.h"
#include <Base/Tools.h>

using namespace Mesh;


//===========================================================================
// Defects Feature
//===========================================================================

PROPERTY_SOURCE(Mesh::FixDefects, Mesh::Feature)

FixDefects::FixDefects()
{
  ADD_PROPERTY(Source  ,(0));
  ADD_PROPERTY(Epsilon  ,(0));
}

FixDefects::~FixDefects()
{
}

short FixDefects::mustExecute() const
{
    if (Source.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *FixDefects::execute(void)
{
  return App::DocumentObject::StdReturn;
}

// ----------------------------------------------------------------------

PROPERTY_SOURCE(Mesh::HarmonizeNormals, Mesh::FixDefects)

HarmonizeNormals::HarmonizeNormals()
{
}

HarmonizeNormals::~HarmonizeNormals()
{
}

App::DocumentObjectExecReturn *HarmonizeNormals::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link) return new App::DocumentObjectExecReturn("No mesh linked");
    App::Property* prop = link->getPropertyByName("Mesh");
    if (prop && prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        Mesh::PropertyMeshKernel* kernel = static_cast<Mesh::PropertyMeshKernel*>(prop);
        std::unique_ptr<MeshObject> mesh(new MeshObject);
        *mesh = kernel->getValue();
        mesh->harmonizeNormals();
        this->Mesh.setValuePtr(mesh.release());
    }

    return App::DocumentObject::StdReturn;
}

// ----------------------------------------------------------------------

PROPERTY_SOURCE(Mesh::FlipNormals, Mesh::FixDefects)

FlipNormals::FlipNormals()
{
}

FlipNormals::~FlipNormals()
{
}

App::DocumentObjectExecReturn *FlipNormals::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link) return new App::DocumentObjectExecReturn("No mesh linked");
    App::Property* prop = link->getPropertyByName("Mesh");
    if (prop && prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        Mesh::PropertyMeshKernel* kernel = static_cast<Mesh::PropertyMeshKernel*>(prop);
        std::unique_ptr<MeshObject> mesh(new MeshObject);
        *mesh = kernel->getValue();
        mesh->flipNormals();
        this->Mesh.setValuePtr(mesh.release());
    }

    return App::DocumentObject::StdReturn;
}

// ----------------------------------------------------------------------

PROPERTY_SOURCE(Mesh::FixNonManifolds, Mesh::FixDefects)

FixNonManifolds::FixNonManifolds()
{
}

FixNonManifolds::~FixNonManifolds()
{
}

App::DocumentObjectExecReturn *FixNonManifolds::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link) return new App::DocumentObjectExecReturn("No mesh linked");
    App::Property* prop = link->getPropertyByName("Mesh");
    if (prop && prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        Mesh::PropertyMeshKernel* kernel = static_cast<Mesh::PropertyMeshKernel*>(prop);
        std::unique_ptr<MeshObject> mesh(new MeshObject);
        *mesh = kernel->getValue();
        mesh->removeNonManifolds();
        this->Mesh.setValuePtr(mesh.release());
    }

    return App::DocumentObject::StdReturn;
}

// ----------------------------------------------------------------------

PROPERTY_SOURCE(Mesh::FixDuplicatedFaces, Mesh::FixDefects)

FixDuplicatedFaces::FixDuplicatedFaces()
{
}

FixDuplicatedFaces::~FixDuplicatedFaces()
{
}

App::DocumentObjectExecReturn *FixDuplicatedFaces::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link) return new App::DocumentObjectExecReturn("No mesh linked");
    App::Property* prop = link->getPropertyByName("Mesh");
    if (prop && prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        Mesh::PropertyMeshKernel* kernel = static_cast<Mesh::PropertyMeshKernel*>(prop);
        std::unique_ptr<MeshObject> mesh(new MeshObject);
        *mesh = kernel->getValue();
        mesh->removeDuplicatedFacets();
        this->Mesh.setValuePtr(mesh.release());
    }

    return App::DocumentObject::StdReturn;
}

// ----------------------------------------------------------------------

PROPERTY_SOURCE(Mesh::FixDuplicatedPoints, Mesh::FixDefects)

FixDuplicatedPoints::FixDuplicatedPoints()
{
}

FixDuplicatedPoints::~FixDuplicatedPoints()
{
}

App::DocumentObjectExecReturn *FixDuplicatedPoints::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link) return new App::DocumentObjectExecReturn("No mesh linked");
    App::Property* prop = link->getPropertyByName("Mesh");
    if (prop && prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        Mesh::PropertyMeshKernel* kernel = static_cast<Mesh::PropertyMeshKernel*>(prop);
        std::unique_ptr<MeshObject> mesh(new MeshObject);
        *mesh = kernel->getValue();
        mesh->removeDuplicatedPoints();
        this->Mesh.setValuePtr(mesh.release());
    }

    return App::DocumentObject::StdReturn;
}

// ----------------------------------------------------------------------

PROPERTY_SOURCE(Mesh::FixDegenerations, Mesh::FixDefects)

FixDegenerations::FixDegenerations()
{
}

FixDegenerations::~FixDegenerations()
{
}

App::DocumentObjectExecReturn *FixDegenerations::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link) return new App::DocumentObjectExecReturn("No mesh linked");
    App::Property* prop = link->getPropertyByName("Mesh");
    if (prop && prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        Mesh::PropertyMeshKernel* kernel = static_cast<Mesh::PropertyMeshKernel*>(prop);
        std::unique_ptr<MeshObject> mesh(new MeshObject);
        *mesh = kernel->getValue();
        mesh->validateDegenerations(static_cast<float>(Epsilon.getValue()));
        this->Mesh.setValuePtr(mesh.release());
    }

    return App::DocumentObject::StdReturn;
}

// ----------------------------------------------------------------------

PROPERTY_SOURCE(Mesh::FixDeformations, Mesh::FixDefects)

FixDeformations::FixDeformations()
{
  ADD_PROPERTY(MaxAngle  ,(5.0f));
}

FixDeformations::~FixDeformations()
{
}

App::DocumentObjectExecReturn *FixDeformations::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link) return new App::DocumentObjectExecReturn("No mesh linked");
    App::Property* prop = link->getPropertyByName("Mesh");
    if (prop && prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        Mesh::PropertyMeshKernel* kernel = static_cast<Mesh::PropertyMeshKernel*>(prop);
        std::unique_ptr<MeshObject> mesh(new MeshObject);
        *mesh = kernel->getValue();
        float maxAngle = Base::toRadians(MaxAngle.getValue());
        mesh->validateDeformations(maxAngle,
                                   static_cast<float>(Epsilon.getValue()));
        this->Mesh.setValuePtr(mesh.release());
    }

    return App::DocumentObject::StdReturn;
}

// ----------------------------------------------------------------------

PROPERTY_SOURCE(Mesh::FixIndices, Mesh::FixDefects)

FixIndices::FixIndices()
{
}

FixIndices::~FixIndices()
{
}

App::DocumentObjectExecReturn *FixIndices::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link) return new App::DocumentObjectExecReturn("No mesh linked");
    App::Property* prop = link->getPropertyByName("Mesh");
    if (prop && prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        Mesh::PropertyMeshKernel* kernel = static_cast<Mesh::PropertyMeshKernel*>(prop);
        std::unique_ptr<MeshObject> mesh(new MeshObject);
        *mesh = kernel->getValue();
        mesh->validateIndices();
        this->Mesh.setValuePtr(mesh.release());
    }

    return App::DocumentObject::StdReturn;
}

// ----------------------------------------------------------------------

PROPERTY_SOURCE(Mesh::FillHoles, Mesh::FixDefects)

FillHoles::FillHoles()
{
    ADD_PROPERTY(FillupHolesOfLength,(0));
    ADD_PROPERTY(MaxArea,(0.1f));
}

FillHoles::~FillHoles()
{
}

App::DocumentObjectExecReturn *FillHoles::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link) return new App::DocumentObjectExecReturn("No mesh linked");
    App::Property* prop = link->getPropertyByName("Mesh");
    if (prop && prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        Mesh::PropertyMeshKernel* kernel = static_cast<Mesh::PropertyMeshKernel*>(prop);
        std::unique_ptr<MeshObject> mesh(new MeshObject);
        *mesh = kernel->getValue();
        MeshCore::ConstraintDelaunayTriangulator cTria((float)MaxArea.getValue());
        //MeshCore::Triangulator cTria(mesh->getKernel());
        mesh->fillupHoles(FillupHolesOfLength.getValue(), 1, cTria);
        this->Mesh.setValuePtr(mesh.release());
    }

    return App::DocumentObject::StdReturn;
}

// ----------------------------------------------------------------------

PROPERTY_SOURCE(Mesh::RemoveComponents, Mesh::FixDefects)

RemoveComponents::RemoveComponents()
{
    ADD_PROPERTY(RemoveCompOfSize,(0));
}

RemoveComponents::~RemoveComponents()
{
}

App::DocumentObjectExecReturn *RemoveComponents::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link) return new App::DocumentObjectExecReturn("No mesh linked");
    App::Property* prop = link->getPropertyByName("Mesh");
    if (prop && prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        Mesh::PropertyMeshKernel* kernel = static_cast<Mesh::PropertyMeshKernel*>(prop);
        std::unique_ptr<MeshObject> mesh(new MeshObject);
        *mesh = kernel->getValue();
        mesh->removeComponents(RemoveCompOfSize.getValue());
        this->Mesh.setValuePtr(mesh.release());
    }

    return App::DocumentObject::StdReturn;
}

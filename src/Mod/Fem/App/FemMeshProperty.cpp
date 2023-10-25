/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <sstream>
#endif

#include <Base/PlacementPy.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "FemMeshProperty.h"
#include "FemMeshPy.h"


using namespace Fem;

TYPESYSTEM_SOURCE(Fem::PropertyFemMesh, App::PropertyComplexGeoData)

PropertyFemMesh::PropertyFemMesh()
    : _FemMesh(new FemMesh)
{}

PropertyFemMesh::~PropertyFemMesh() = default;

void PropertyFemMesh::setValuePtr(FemMesh* mesh)
{
    // use the tmp. object to guarantee that the referenced mesh is not destroyed
    // before calling hasSetValue()
    Base::Reference<FemMesh> tmp(_FemMesh);
    aboutToSetValue();
    _FemMesh = mesh;
    hasSetValue();
}

void PropertyFemMesh::setValue(const FemMesh& sh)
{
    aboutToSetValue();
    *_FemMesh = sh;
    hasSetValue();
}

const FemMesh& PropertyFemMesh::getValue() const
{
    return *_FemMesh;
}

const Data::ComplexGeoData* PropertyFemMesh::getComplexData() const
{
    return static_cast<FemMesh*>(_FemMesh);
}

Base::BoundBox3d PropertyFemMesh::getBoundingBox() const
{
    return _FemMesh->getBoundBox();
}

void PropertyFemMesh::setTransform(const Base::Matrix4D& rclTrf)
{
    _FemMesh->setTransform(rclTrf);
}

Base::Matrix4D PropertyFemMesh::getTransform() const
{
    return _FemMesh->getTransform();
}

void PropertyFemMesh::transformGeometry(const Base::Matrix4D& rclMat)
{
    aboutToSetValue();
    _FemMesh->transformGeometry(rclMat);
    hasSetValue();
}

PyObject* PropertyFemMesh::getPyObject()
{
    FemMeshPy* mesh = new FemMeshPy(&*_FemMesh);
    mesh->setConst();
    return mesh;
}

void PropertyFemMesh::setPyObject(PyObject* value)
{
    if (PyObject_TypeCheck(value, &(FemMeshPy::Type))) {
        FemMeshPy* pcObject = static_cast<FemMeshPy*>(value);
        setValue(*pcObject->getFemMeshPtr());
    }
    else if (PyObject_TypeCheck(value, &(Base::PlacementPy::Type))) {
        Base::PlacementPy* pcObject = static_cast<Base::PlacementPy*>(value);
        transformGeometry(pcObject->getPlacementPtr()->toMatrix());
    }
    else {
        std::string error = std::string("type must be 'FemMesh', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

App::Property* PropertyFemMesh::Copy() const
{
    PropertyFemMesh* prop = new PropertyFemMesh();
    prop->_FemMesh = this->_FemMesh;
    return prop;
}

void PropertyFemMesh::Paste(const App::Property& from)
{
    aboutToSetValue();
    _FemMesh = dynamic_cast<const PropertyFemMesh&>(from)._FemMesh;
    hasSetValue();
}

unsigned int PropertyFemMesh::getMemSize() const
{
    return _FemMesh->getMemSize();
}

void PropertyFemMesh::Save(Base::Writer& writer) const
{
    _FemMesh->Save(writer);
}

void PropertyFemMesh::Restore(Base::XMLReader& reader)
{
    _FemMesh->Restore(reader);
}

void PropertyFemMesh::SaveDocFile(Base::Writer& writer) const
{
    _FemMesh->SaveDocFile(writer);
}

void PropertyFemMesh::RestoreDocFile(Base::Reader& reader)
{
    aboutToSetValue();
    _FemMesh->RestoreDocFile(reader);
    hasSetValue();
}

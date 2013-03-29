/***************************************************************************
 *   Copyright (c) J�rgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <sstream>
#endif


#include <strstream>
#include <Base/Console.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>

#include "FemMeshProperty.h"
#include "FemMeshPy.h"

using namespace Fem;

TYPESYSTEM_SOURCE(Fem::PropertyFemMesh , App::PropertyComplexGeoData);

PropertyFemMesh::PropertyFemMesh() : _FemMesh(new FemMesh)
{
}

PropertyFemMesh::~PropertyFemMesh()
{
}

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

const FemMesh &PropertyFemMesh::getValue(void)const 
{
    return *_FemMesh;
}

const Data::ComplexGeoData* PropertyFemMesh::getComplexData() const
{
    return (FemMesh*)_FemMesh;
}

Base::BoundBox3d PropertyFemMesh::getBoundingBox() const
{
    return _FemMesh->getBoundBox();
}

void PropertyFemMesh::transformGeometry(const Base::Matrix4D &rclMat)
{
    aboutToSetValue();
    _FemMesh->transformGeometry(rclMat);
    hasSetValue();
}

void PropertyFemMesh::getFaces(std::vector<Base::Vector3d> &aPoints,
                               std::vector<Data::ComplexGeoData::Facet> &aTopo,
                               float accuracy, uint16_t flags) const
{
    _FemMesh->getFaces(aPoints, aTopo, accuracy, flags);
}

PyObject *PropertyFemMesh::getPyObject(void)
{
    FemMeshPy* mesh = new FemMeshPy(&*_FemMesh);
    mesh->setConst();
    return mesh;
}

void PropertyFemMesh::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(FemMeshPy::Type))) {
        FemMeshPy *pcObject = static_cast<FemMeshPy*>(value);
        setValue(*pcObject->getFemMeshPtr());
    }
    else {
        std::string error = std::string("type must be 'FemMesh', not ");
        error += value->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

App::Property *PropertyFemMesh::Copy(void) const
{
    PropertyFemMesh *prop = new PropertyFemMesh();
    prop->_FemMesh = this->_FemMesh;
    return prop;
}

void PropertyFemMesh::Paste(const App::Property &from)
{
    aboutToSetValue();
    _FemMesh = dynamic_cast<const PropertyFemMesh&>(from)._FemMesh;
    hasSetValue();
}

unsigned int PropertyFemMesh::getMemSize (void) const
{
    return _FemMesh->getMemSize();
}

void PropertyFemMesh::Save (Base::Writer &writer) const
{
    if (!writer.isForceXML()) {
        //See SaveDocFile(), RestoreDocFile()
        writer.Stream() << writer.ind() << "<FemMesh file=\"" 
                        << writer.addFile("FemMesh.unv", this)
                        << "\"/>" << std::endl;
    }
}

void PropertyFemMesh::Restore(Base::XMLReader &reader)
{
    reader.readElement("FemMesh");
    std::string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // initate a file read
        reader.addFile(file.c_str(),this);
    }
}

void PropertyFemMesh::SaveDocFile (Base::Writer &writer) const
{
    _FemMesh->SaveDocFile(writer);
}

void PropertyFemMesh::RestoreDocFile(Base::Reader &reader)
{
    aboutToSetValue();
    _FemMesh->RestoreDocFile(reader);
    hasSetValue();
}

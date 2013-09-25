/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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

#include <CXX/Objects.hxx>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/VectorPy.h>

#include "Core/MeshKernel.h"
#include "Core/MeshIO.h"
#include "Core/Iterator.h"

#include "MeshProperties.h"
#include "Mesh.h"
#include "MeshPy.h"

using namespace Mesh;

TYPESYSTEM_SOURCE(Mesh::PropertyNormalList, App::PropertyLists);
TYPESYSTEM_SOURCE(Mesh::PropertyCurvatureList , App::PropertyLists);
TYPESYSTEM_SOURCE(Mesh::PropertyMeshKernel , App::PropertyComplexGeoData);

PropertyNormalList::PropertyNormalList()
{

}

PropertyNormalList::~PropertyNormalList()
{

}

void PropertyNormalList::setSize(int newSize)
{
    _lValueList.resize(newSize);
}

int PropertyNormalList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyNormalList::setValue(const Base::Vector3f& lValue)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0]=lValue;
    hasSetValue();
}

void PropertyNormalList::setValue(float x, float y, float z)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0].Set(x,y,z);
    hasSetValue();
}

void PropertyNormalList::setValues(const std::vector<Base::Vector3f>& values)
{
    aboutToSetValue();
    _lValueList = values;
    hasSetValue();
}

PyObject *PropertyNormalList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());

    for (int i = 0;i<getSize(); i++)
        PyList_SetItem(list, i, new Base::VectorPy(_lValueList[i]));

    return list;
}

void PropertyNormalList::setPyObject(PyObject *value)
{
    if (PyList_Check(value)) {
        Py_ssize_t nSize = PyList_Size(value);
        std::vector<Base::Vector3f> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i<nSize;++i) {
            PyObject* item = PyList_GetItem(value, i);
            App::PropertyVector val;
            val.setPyObject( item );
            values[i] = Base::convertTo<Base::Vector3f>(val.getValue());
        }

        setValues(values);
    }
    else if (PyObject_TypeCheck(value, &(Base::VectorPy::Type))) {
        Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(value);
        Base::Vector3d* val = pcObject->getVectorPtr();
        setValue(Base::convertTo<Base::Vector3f>(*val));
    }
    else if (PyTuple_Check(value) && PyTuple_Size(value) == 3) {
        App::PropertyVector val;
        val.setPyObject( value );
        setValue(Base::convertTo<Base::Vector3f>(val.getValue()));
    }
    else {
        std::string error = std::string("type must be 'Vector' or list of 'Vector', not ");
        error += value->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

void PropertyNormalList::Save (Base::Writer &writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<VectorList file=\"" << writer.addFile(getName(), this) << "\"/>" << std::endl;
    }
}

void PropertyNormalList::Restore(Base::XMLReader &reader)
{
    reader.readElement("VectorList");
    std::string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // initate a file read
        reader.addFile(file.c_str(),this);
    }
}

void PropertyNormalList::SaveDocFile (Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (std::vector<Base::Vector3f>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
        str << it->x << it->y << it->z;
    }
}

void PropertyNormalList::RestoreDocFile(Base::Reader &reader)
{
    Base::InputStream str(reader);
    uint32_t uCt=0;
    str >> uCt;
    std::vector<Base::Vector3f> values(uCt);
    for (std::vector<Base::Vector3f>::iterator it = values.begin(); it != values.end(); ++it) {
        str >> it->x >> it->y >> it->z;
    }
    setValues(values);
}

App::Property *PropertyNormalList::Copy(void) const
{
    PropertyNormalList *p= new PropertyNormalList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyNormalList::Paste(const App::Property &from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyNormalList&>(from)._lValueList;
    hasSetValue();
}

unsigned int PropertyNormalList::getMemSize (void) const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Base::Vector3f));
}

void PropertyNormalList::transform(const Base::Matrix4D &mat)
{
    // A normal vector is only a direction with unit length, so we only need to rotate it
    // (no translations or scaling)

    // Extract scale factors (assumes an orthogonal rotation matrix)
    // Use the fact that the length of the row vectors of R are all equal to 1
    // And that scaling is applied after rotating
    double s[3];
    s[0] = sqrt(mat[0][0] * mat[0][0] + mat[0][1] * mat[0][1] + mat[0][2] * mat[0][2]);
    s[1] = sqrt(mat[1][0] * mat[1][0] + mat[1][1] * mat[1][1] + mat[1][2] * mat[1][2]);
    s[2] = sqrt(mat[2][0] * mat[2][0] + mat[2][1] * mat[2][1] + mat[2][2] * mat[2][2]);

    // Set up the rotation matrix: zero the translations and make the scale factors = 1
    Base::Matrix4D rot;
    rot.setToUnity();
    for (unsigned short i = 0; i < 3; i++) {
        for (unsigned short j = 0; j < 3; j++) {
            rot[i][j] = mat[i][j] / s[i];
        }
    }

    aboutToSetValue();

    // Rotate the normal vectors
    for (int ii=0; ii<getSize(); ii++) {
        set1Value(ii, rot * operator[](ii));
    }

    hasSetValue();

}

// ----------------------------------------------------------------------------

PropertyCurvatureList::PropertyCurvatureList()
{

}

PropertyCurvatureList::~PropertyCurvatureList()
{

}

void PropertyCurvatureList::setValue(const CurvatureInfo& lValue)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0]=lValue;
    hasSetValue();
}

void PropertyCurvatureList::setValues(const std::vector<CurvatureInfo>& lValues)
{
    aboutToSetValue();
    _lValueList=lValues;
    hasSetValue();
}

std::vector<float> PropertyCurvatureList::getCurvature( int mode ) const
{
    const std::vector<Mesh::CurvatureInfo>& fCurvInfo = getValues();
    std::vector<float> fValues;
    fValues.reserve(fCurvInfo.size());

    // Mean curvature
    if (mode == MeanCurvature) {
        for ( std::vector<Mesh::CurvatureInfo>::const_iterator it=fCurvInfo.begin();it!=fCurvInfo.end(); ++it )
        {
            fValues.push_back( 0.5f*(it->fMaxCurvature+it->fMinCurvature) );
        }
    }
    // Gaussian curvature
    else if (mode == GaussCurvature) {
        for ( std::vector<Mesh::CurvatureInfo>::const_iterator it=fCurvInfo.begin();it!=fCurvInfo.end(); ++it )
        {
            fValues.push_back( it->fMaxCurvature * it->fMinCurvature );
        }
    }
    // Maximum curvature
    else if (mode == MaxCurvature) {
        for ( std::vector<Mesh::CurvatureInfo>::const_iterator it=fCurvInfo.begin();it!=fCurvInfo.end(); ++it )
        {
          fValues.push_back( it->fMaxCurvature );
        }
    }
    // Minimum curvature
    else if (mode == MinCurvature) {
        for ( std::vector<Mesh::CurvatureInfo>::const_iterator it=fCurvInfo.begin();it!=fCurvInfo.end(); ++it )
        {
          fValues.push_back( it->fMinCurvature );
        }
    }
    // Absolute curvature
    else if (mode == AbsCurvature) {
        for ( std::vector<Mesh::CurvatureInfo>::const_iterator it=fCurvInfo.begin();it!=fCurvInfo.end(); ++it )
        {
            if ( fabs(it->fMaxCurvature) > fabs(it->fMinCurvature) )
                fValues.push_back( it->fMaxCurvature );
            else
                fValues.push_back( it->fMinCurvature );
        }
    }

    return fValues;
}

void PropertyCurvatureList::transform(const Base::Matrix4D &mat)
{
    // The principal direction is only a vector with unit length, so we only need to rotate it
    // (no translations or scaling)
    
    // Extract scale factors (assumes an orthogonal rotation matrix)
    // Use the fact that the length of the row vectors of R are all equal to 1
    // And that scaling is applied after rotating
    double s[3];
    s[0] = sqrt(mat[0][0] * mat[0][0] + mat[0][1] * mat[0][1] + mat[0][2] * mat[0][2]);
    s[1] = sqrt(mat[1][0] * mat[1][0] + mat[1][1] * mat[1][1] + mat[1][2] * mat[1][2]);
    s[2] = sqrt(mat[2][0] * mat[2][0] + mat[2][1] * mat[2][1] + mat[2][2] * mat[2][2]);
    
    // Set up the rotation matrix: zero the translations and make the scale factors = 1
    Base::Matrix4D rot;
    rot.setToUnity();
    for (unsigned short i = 0; i < 3; i++) {
        for (unsigned short j = 0; j < 3; j++) {
            rot[i][j] = mat[i][j] / s[i];
        }
    }

    aboutToSetValue();

    // Rotate the principal directions
    for (int ii=0; ii<getSize(); ii++)
    {
        CurvatureInfo ci = operator[](ii);
        ci.cMaxCurvDir = rot * ci.cMaxCurvDir;
        ci.cMinCurvDir = rot * ci.cMinCurvDir;
        _lValueList[ii] = ci;
    }

    hasSetValue();
}

void PropertyCurvatureList::Save (Base::Writer &writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<CurvatureList file=\"" << 
        writer.addFile(getName(), this) << "\"/>" << std::endl;
    }
}

void PropertyCurvatureList::Restore(Base::XMLReader &reader)
{
    reader.readElement("CurvatureList");
    std::string file (reader.getAttribute("file") );
    
    if (!file.empty()) {
        // initate a file read
        reader.addFile(file.c_str(),this);
    }
}

void PropertyCurvatureList::SaveDocFile (Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (std::vector<CurvatureInfo>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
        str << it->fMaxCurvature << it->fMinCurvature;
        str << it->cMaxCurvDir.x << it->cMaxCurvDir.y << it->cMaxCurvDir.z;
        str << it->cMinCurvDir.x << it->cMinCurvDir.y << it->cMinCurvDir.z;
    }
}

void PropertyCurvatureList::RestoreDocFile(Base::Reader &reader)
{
    Base::InputStream str(reader);
    uint32_t uCt=0;
    str >> uCt;
    std::vector<CurvatureInfo> values(uCt);
    for (std::vector<CurvatureInfo>::iterator it = values.begin(); it != values.end(); ++it) {
        str >> it->fMaxCurvature >> it->fMinCurvature;
        str >> it->cMaxCurvDir.x >> it->cMaxCurvDir.y >> it->cMaxCurvDir.z;
        str >> it->cMinCurvDir.x >> it->cMinCurvDir.y >> it->cMinCurvDir.z;
    }

    setValues(values);
}

PyObject* PropertyCurvatureList::getPyObject(void)
{
    Py::List list;
    for (std::vector<CurvatureInfo>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
        Py::Tuple tuple(4);
        tuple.setItem(0, Py::Float(it->fMaxCurvature));
        tuple.setItem(1, Py::Float(it->fMinCurvature));
        Py::Tuple maxDir(3);
        maxDir.setItem(0, Py::Float(it->cMaxCurvDir.x));
        maxDir.setItem(1, Py::Float(it->cMaxCurvDir.y));
        maxDir.setItem(2, Py::Float(it->cMaxCurvDir.z));
        tuple.setItem(2, maxDir);
        Py::Tuple minDir(3);
        minDir.setItem(0, Py::Float(it->cMinCurvDir.x));
        minDir.setItem(1, Py::Float(it->cMinCurvDir.y));
        minDir.setItem(2, Py::Float(it->cMinCurvDir.z));
        tuple.setItem(3, minDir);
        list.append(tuple);
    }

    return Py::new_reference_to(list);
}

void PropertyCurvatureList::setPyObject(PyObject *value)
{
    throw Base::AttributeError(std::string("This attribute is read-only"));
}

App::Property *PropertyCurvatureList::Copy(void) const
{
    PropertyCurvatureList *p= new PropertyCurvatureList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyCurvatureList::Paste(const App::Property &from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyCurvatureList&>(from)._lValueList;
    hasSetValue();
}

// ----------------------------------------------------------------------------

PropertyMeshKernel::PropertyMeshKernel()
  : _meshObject(new MeshObject()), meshPyObject(0)
{
    // Note: Normally this property is a member of a document object, i.e. the setValue()
    // method gets called in the constructor of a sublcass of DocumentObject, e.g. Mesh::Feature.
    // This means that the created MeshObject here will be replaced and deleted immediately. 
    // However, we anyway create this object in case we use this class in another context.
}

PropertyMeshKernel::~PropertyMeshKernel()
{
    if (meshPyObject) {
        // Note: Do not call setInvalid() of the Python binding 
        // because the mesh should still be accessible afterwards.
        meshPyObject->parentProperty = 0;
        Py_DECREF(meshPyObject);
    }
}

void PropertyMeshKernel::setValuePtr(MeshObject* mesh)
{
    // use the tmp. object to guarantee that the referenced mesh is not destroyed
    // before calling hasSetValue()
    Base::Reference<MeshObject> tmp(_meshObject);
    aboutToSetValue();
    _meshObject = mesh;
    hasSetValue();
}

void PropertyMeshKernel::setValue(const MeshObject& mesh)
{
    aboutToSetValue();
    *_meshObject = mesh;
    hasSetValue();
}

void PropertyMeshKernel::setValue(const MeshCore::MeshKernel& mesh)
{
    aboutToSetValue();
    _meshObject->setKernel(mesh);
    hasSetValue();
}

void PropertyMeshKernel::swapMesh(MeshObject& mesh)
{
    aboutToSetValue();
    _meshObject->swap(mesh);
    hasSetValue();
}

void PropertyMeshKernel::swapMesh(MeshCore::MeshKernel& mesh)
{
    aboutToSetValue();
    _meshObject->swap(mesh);
    hasSetValue();
}

const MeshObject& PropertyMeshKernel::getValue(void)const 
{
    return *_meshObject;
}

const MeshObject* PropertyMeshKernel::getValuePtr(void)const 
{
    return (MeshObject*)_meshObject;
}

const Data::ComplexGeoData* PropertyMeshKernel::getComplexData() const
{
    return (MeshObject*)_meshObject;
}

Base::BoundBox3d PropertyMeshKernel::getBoundingBox() const
{
    return _meshObject->getBoundBox();
}

void PropertyMeshKernel::getFaces(std::vector<Base::Vector3d> &aPoints,
                                  std::vector<Data::ComplexGeoData::Facet> &aTopo,
                                  float accuracy, uint16_t flags) const
{
    _meshObject->getFaces(aPoints, aTopo, accuracy, flags);
}

unsigned int PropertyMeshKernel::getMemSize (void) const
{
    unsigned int size = 0;
    size += _meshObject->getMemSize();
    
    return size;
}

MeshObject* PropertyMeshKernel::startEditing()
{
    aboutToSetValue();
    return (MeshObject*)_meshObject;
}

void PropertyMeshKernel::finishEditing()
{
    hasSetValue();
}

void PropertyMeshKernel::transformGeometry(const Base::Matrix4D &rclMat)
{
    aboutToSetValue();
    _meshObject->transformGeometry(rclMat);
    hasSetValue();
}

void PropertyMeshKernel::setPointIndices(const std::vector<std::pair<unsigned long, Base::Vector3f> >& inds)
{
    aboutToSetValue();
    MeshCore::MeshKernel& kernel = _meshObject->getKernel();
    for (std::vector<std::pair<unsigned long, Base::Vector3f> >::const_iterator it = inds.begin(); it != inds.end(); ++it)
        kernel.SetPoint(it->first, it->second);
    hasSetValue();
}

PyObject *PropertyMeshKernel::getPyObject(void)
{
    if (!meshPyObject) {
        meshPyObject = new MeshPy(&*_meshObject);
        meshPyObject->setConst(); // set immutable
        meshPyObject->parentProperty = this;
    }

    Py_INCREF(meshPyObject);
    return meshPyObject;
}

void PropertyMeshKernel::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(MeshPy::Type))) {
        MeshPy* mesh = static_cast<MeshPy*>(value);
        // Do not allow to reassign the same instance
        if (&(*this->_meshObject) != mesh->getMeshObjectPtr()) {
            // Note: Copy the content, do NOT reference the same mesh object
            setValue(*(mesh->getMeshObjectPtr()));
        }
    }
    else if (PyList_Check(value)) {
        // new instance of MeshObject
        Py::List triangles(value);
        MeshObject* mesh = MeshObject::createMeshFromList(triangles);
        setValuePtr(mesh);
    }
    else {
        std::string error = std::string("type must be 'Mesh', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyMeshKernel::Save (Base::Writer &writer) const
{
    if (writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<Mesh>" << std::endl;
        MeshCore::MeshOutput saver(_meshObject->getKernel());
        saver.SaveXML(writer);
    }
    else {
        writer.Stream() << writer.ind() << "<Mesh file=\"" << 
        writer.addFile("MeshKernel.bms", this) << "\"/>" << std::endl;
    }
}

void PropertyMeshKernel::Restore(Base::XMLReader &reader)
{
    reader.readElement("Mesh");
    std::string file (reader.getAttribute("file") );
    
    if (file.empty()) {
        // read XML
        MeshCore::MeshKernel kernel;
        MeshCore::MeshInput restorer(kernel);
        restorer.LoadXML(reader);

        // avoid to duplicate the mesh in memory
        MeshCore::MeshPointArray points;
        MeshCore::MeshFacetArray facets;
        kernel.Adopt(points, facets);

        aboutToSetValue();
        _meshObject->getKernel().Adopt(points, facets);
        hasSetValue();
    } 
    else {
        // initate a file read
        reader.addFile(file.c_str(),this);
    }
}

void PropertyMeshKernel::SaveDocFile (Base::Writer &writer) const
{
    _meshObject->save(writer.Stream());
}

void PropertyMeshKernel::RestoreDocFile(Base::Reader &reader)
{
    aboutToSetValue();
    _meshObject->load(reader);
    hasSetValue();
}

App::Property *PropertyMeshKernel::Copy(void) const
{
    // Note: Copy the content, do NOT reference the same mesh object
    PropertyMeshKernel *prop = new PropertyMeshKernel();
    *(prop->_meshObject) = *(this->_meshObject);
    return prop;
}

void PropertyMeshKernel::Paste(const App::Property &from)
{
    // Note: Copy the content, do NOT reference the same mesh object
    aboutToSetValue();
    const PropertyMeshKernel& prop = dynamic_cast<const PropertyMeshKernel&>(from);
    *(this->_meshObject) = *(prop._meshObject);
    hasSetValue();
}

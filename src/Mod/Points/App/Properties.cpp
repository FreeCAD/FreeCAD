/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <QtConcurrentMap>
#include <algorithm>
#include <cmath>
#include <iostream>
#endif

#include <Base/Converter.h>
#include <Base/Matrix.h>
#include <Base/Persistence.h>
#include <Base/Stream.h>
#include <Base/VectorPy.h>
#include <Base/Writer.h>

#include "Points.h"
#include "Properties.h"

#ifdef _MSC_VER
#include <ppl.h>
#endif


using namespace Points;
using namespace std;

TYPESYSTEM_SOURCE(Points::PropertyGreyValue, App::PropertyFloat)
TYPESYSTEM_SOURCE(Points::PropertyGreyValueList, App::PropertyLists)
TYPESYSTEM_SOURCE(Points::PropertyNormalList, App::PropertyLists)
TYPESYSTEM_SOURCE(Points::PropertyCurvatureList, App::PropertyLists)

PropertyGreyValueList::PropertyGreyValueList() = default;

void PropertyGreyValueList::setSize(int newSize)
{
    _lValueList.resize(newSize);
}

int PropertyGreyValueList::getSize() const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyGreyValueList::setValue(float lValue)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0] = lValue;
    hasSetValue();
}

void PropertyGreyValueList::setValues(const std::vector<float>& values)
{
    aboutToSetValue();
    _lValueList = values;
    hasSetValue();
}

PyObject* PropertyGreyValueList::getPyObject()
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++) {
        PyList_SetItem(list, i, PyFloat_FromDouble(_lValueList[i]));
    }
    return list;
}

void PropertyGreyValueList::setPyObject(PyObject* value)
{
    if (PyList_Check(value)) {
        Py_ssize_t nSize = PyList_Size(value);
        std::vector<float> values;
        values.resize(nSize);

        for (Py_ssize_t i = 0; i < nSize; ++i) {
            PyObject* item = PyList_GetItem(value, i);
            if (!PyFloat_Check(item)) {
                std::string error = std::string("type in list must be float, not ");
                error += item->ob_type->tp_name;
                throw Py::TypeError(error);
            }

            values[i] = (float)PyFloat_AsDouble(item);
        }

        setValues(values);
    }
    else if (PyFloat_Check(value)) {
        setValue((float)PyFloat_AsDouble(value));
    }
    else {
        std::string error = std::string("type must be float or list of float, not ");
        error += value->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

void PropertyGreyValueList::Save(Base::Writer& writer) const
{
    if (writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<FloatList count=\"" << getSize() << "\">" << endl;
        writer.incInd();
        for (int i = 0; i < getSize(); i++) {
            writer.Stream() << writer.ind() << "<F v=\"" << _lValueList[i] << "\"/>" << endl;
        };
        writer.decInd();
        writer.Stream() << writer.ind() << "</FloatList>" << endl;
    }
    else {
        writer.Stream() << writer.ind() << "<FloatList file=\"" << writer.addFile(getName(), this)
                        << "\"/>" << std::endl;
    }
}

void PropertyGreyValueList::Restore(Base::XMLReader& reader)
{
    reader.readElement("FloatList");
    string file(reader.getAttribute("file"));

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), this);
    }
}

void PropertyGreyValueList::SaveDocFile(Base::Writer& writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (float it : _lValueList) {
        str << it;
    }
}

void PropertyGreyValueList::RestoreDocFile(Base::Reader& reader)
{
    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<float> values(uCt);
    for (float& value : values) {
        str >> value;
    }
    setValues(values);
}

App::Property* PropertyGreyValueList::Copy() const
{
    PropertyGreyValueList* p = new PropertyGreyValueList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyGreyValueList::Paste(const App::Property& from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyGreyValueList&>(from)._lValueList;
    hasSetValue();
}

unsigned int PropertyGreyValueList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(float));
}

void PropertyGreyValueList::removeIndices(const std::vector<unsigned long>& uIndices)
{
    // We need a sorted array
    std::vector<unsigned long> uSortedInds = uIndices;
    std::sort(uSortedInds.begin(), uSortedInds.end());

    const std::vector<float>& rValueList = getValues();

    assert(uSortedInds.size() <= rValueList.size());
    if (uSortedInds.size() > rValueList.size()) {
        return;
    }

    std::vector<float> remainValue;
    remainValue.reserve(rValueList.size() - uSortedInds.size());

    std::vector<unsigned long>::iterator pos = uSortedInds.begin();
    for (std::vector<float>::const_iterator it = rValueList.begin(); it != rValueList.end(); ++it) {
        unsigned long index = it - rValueList.begin();
        if (pos == uSortedInds.end()) {
            remainValue.push_back(*it);
        }
        else if (index != *pos) {
            remainValue.push_back(*it);
        }
        else {
            ++pos;
        }
    }

    setValues(remainValue);
}

PropertyNormalList::PropertyNormalList() = default;

void PropertyNormalList::setSize(int newSize)
{
    _lValueList.resize(newSize);
}

int PropertyNormalList::getSize() const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyNormalList::setValue(const Base::Vector3f& lValue)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0] = lValue;
    hasSetValue();
}

void PropertyNormalList::setValue(float x, float y, float z)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0].Set(x, y, z);
    hasSetValue();
}

void PropertyNormalList::setValues(const std::vector<Base::Vector3f>& values)
{
    aboutToSetValue();
    _lValueList = values;
    hasSetValue();
}

PyObject* PropertyNormalList::getPyObject()
{
    PyObject* list = PyList_New(getSize());

    for (int i = 0; i < getSize(); i++) {
        PyList_SetItem(list, i, new Base::VectorPy(_lValueList[i]));
    }

    return list;
}

void PropertyNormalList::setPyObject(PyObject* value)
{
    if (PyList_Check(value)) {
        Py_ssize_t nSize = PyList_Size(value);
        std::vector<Base::Vector3f> values;
        values.resize(nSize);

        for (Py_ssize_t i = 0; i < nSize; ++i) {
            PyObject* item = PyList_GetItem(value, i);
            App::PropertyVector val;
            val.setPyObject(item);
            values[i] = Base::convertTo<Base::Vector3f>(val.getValue());
        }

        setValues(values);
    }
    else if (PyObject_TypeCheck(value, &(Base::VectorPy::Type))) {
        Base::VectorPy* pcObject = static_cast<Base::VectorPy*>(value);
        Base::Vector3d* val = pcObject->getVectorPtr();
        setValue(Base::convertTo<Base::Vector3f>(*val));
    }
    else if (PyTuple_Check(value) && PyTuple_Size(value) == 3) {
        App::PropertyVector val;
        val.setPyObject(value);
        setValue(Base::convertTo<Base::Vector3f>(val.getValue()));
    }
    else {
        std::string error = std::string("type must be 'Vector' or list of 'Vector', not ");
        error += value->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

void PropertyNormalList::Save(Base::Writer& writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<VectorList file=\"" << writer.addFile(getName(), this)
                        << "\"/>" << std::endl;
    }
}

void PropertyNormalList::Restore(Base::XMLReader& reader)
{
    reader.readElement("VectorList");
    std::string file(reader.getAttribute("file"));

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), this);
    }
}

void PropertyNormalList::SaveDocFile(Base::Writer& writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (const auto& it : _lValueList) {
        str << it.x << it.y << it.z;
    }
}

void PropertyNormalList::RestoreDocFile(Base::Reader& reader)
{
    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<Base::Vector3f> values(uCt);
    for (auto& value : values) {
        str >> value.x >> value.y >> value.z;
    }
    setValues(values);
}

App::Property* PropertyNormalList::Copy() const
{
    PropertyNormalList* p = new PropertyNormalList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyNormalList::Paste(const App::Property& from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyNormalList&>(from)._lValueList;
    hasSetValue();
}

unsigned int PropertyNormalList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Base::Vector3f));
}

void PropertyNormalList::transformGeometry(const Base::Matrix4D& mat)
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
#ifdef _MSC_VER
    Concurrency::parallel_for_each(_lValueList.begin(),
                                   _lValueList.end(),
                                   [rot](Base::Vector3f& value) {
                                       value = rot * value;
                                   });
#else
    QtConcurrent::blockingMap(_lValueList, [rot](Base::Vector3f& value) {
        rot.multVec(value, value);
    });
#endif

    hasSetValue();
}

void PropertyNormalList::removeIndices(const std::vector<unsigned long>& uIndices)
{
    // We need a sorted array
    std::vector<unsigned long> uSortedInds = uIndices;
    std::sort(uSortedInds.begin(), uSortedInds.end());

    const std::vector<Base::Vector3f>& rValueList = getValues();

    assert(uSortedInds.size() <= rValueList.size());
    if (uSortedInds.size() > rValueList.size()) {
        return;
    }

    std::vector<Base::Vector3f> remainValue;
    remainValue.reserve(rValueList.size() - uSortedInds.size());

    std::vector<unsigned long>::iterator pos = uSortedInds.begin();
    for (std::vector<Base::Vector3f>::const_iterator it = rValueList.begin();
         it != rValueList.end();
         ++it) {
        unsigned long index = it - rValueList.begin();
        if (pos == uSortedInds.end()) {
            remainValue.push_back(*it);
        }
        else if (index != *pos) {
            remainValue.push_back(*it);
        }
        else {
            ++pos;
        }
    }

    setValues(remainValue);
}

PropertyCurvatureList::PropertyCurvatureList() = default;

void PropertyCurvatureList::setValue(const CurvatureInfo& lValue)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0] = lValue;
    hasSetValue();
}

void PropertyCurvatureList::setValues(const std::vector<CurvatureInfo>& lValues)
{
    aboutToSetValue();
    _lValueList = lValues;
    hasSetValue();
}

std::vector<float> PropertyCurvatureList::getCurvature(int mode) const
{
    const std::vector<Points::CurvatureInfo>& fCurvInfo = getValues();
    std::vector<float> fValues;
    fValues.reserve(fCurvInfo.size());

    // Mean curvature
    if (mode == MeanCurvature) {
        for (const auto& it : fCurvInfo) {
            fValues.push_back(0.5f * (it.fMaxCurvature + it.fMinCurvature));
        }
    }
    // Gaussian curvature
    else if (mode == GaussCurvature) {
        for (const auto& it : fCurvInfo) {
            fValues.push_back(it.fMaxCurvature * it.fMinCurvature);
        }
    }
    // Maximum curvature
    else if (mode == MaxCurvature) {
        for (const auto& it : fCurvInfo) {
            fValues.push_back(it.fMaxCurvature);
        }
    }
    // Minimum curvature
    else if (mode == MinCurvature) {
        for (const auto& it : fCurvInfo) {
            fValues.push_back(it.fMinCurvature);
        }
    }
    // Absolute curvature
    else if (mode == AbsCurvature) {
        for (const auto& it : fCurvInfo) {
            if (fabs(it.fMaxCurvature) > fabs(it.fMinCurvature)) {
                fValues.push_back(it.fMaxCurvature);
            }
            else {
                fValues.push_back(it.fMinCurvature);
            }
        }
    }

    return fValues;
}

void PropertyCurvatureList::transformGeometry(const Base::Matrix4D& mat)
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
    for (int ii = 0; ii < getSize(); ii++) {
        CurvatureInfo ci = operator[](ii);
        ci.cMaxCurvDir = rot * ci.cMaxCurvDir;
        ci.cMinCurvDir = rot * ci.cMinCurvDir;
        set1Value(ii, ci);
    }

    hasSetValue();
}

void PropertyCurvatureList::removeIndices(const std::vector<unsigned long>& uIndices)
{
    // We need a sorted array
    std::vector<unsigned long> uSortedInds = uIndices;
    std::sort(uSortedInds.begin(), uSortedInds.end());

    assert(uSortedInds.size() <= _lValueList.size());
    if (uSortedInds.size() > _lValueList.size()) {
        return;
    }

    std::vector<CurvatureInfo> remainValue;
    remainValue.reserve(_lValueList.size() - uSortedInds.size());

    std::vector<unsigned long>::iterator pos = uSortedInds.begin();
    for (std::vector<CurvatureInfo>::const_iterator it = _lValueList.begin();
         it != _lValueList.end();
         ++it) {
        unsigned long index = it - _lValueList.begin();
        if (pos == uSortedInds.end()) {
            remainValue.push_back(*it);
        }
        else if (index != *pos) {
            remainValue.push_back(*it);
        }
        else {
            ++pos;
        }
    }

    setValues(remainValue);
}

PyObject* PropertyCurvatureList::getPyObject()
{
    throw Py::NotImplementedError("Not yet implemented");
}

void PropertyCurvatureList::setPyObject(PyObject*)
{
    throw Py::NotImplementedError("Not yet implemented");
}

void PropertyCurvatureList::Save(Base::Writer& writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<CurvatureList file=\""
                        << writer.addFile(getName(), this) << "\"/>" << std::endl;
    }
}

void PropertyCurvatureList::Restore(Base::XMLReader& reader)
{
    reader.readElement("CurvatureList");
    std::string file(reader.getAttribute("file"));

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), this);
    }
}

void PropertyCurvatureList::SaveDocFile(Base::Writer& writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    if (uCt > 0) {
        for (const auto& it : _lValueList) {
            str << it.fMaxCurvature << it.fMinCurvature;
            str << it.cMaxCurvDir.x << it.cMaxCurvDir.y << it.cMaxCurvDir.z;
            str << it.cMinCurvDir.x << it.cMinCurvDir.y << it.cMinCurvDir.z;
        }
    }
}

void PropertyCurvatureList::RestoreDocFile(Base::Reader& reader)
{
    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<CurvatureInfo> values(uCt);
    for (auto& value : values) {
        str >> value.fMaxCurvature >> value.fMinCurvature;
        str >> value.cMaxCurvDir.x >> value.cMaxCurvDir.y >> value.cMaxCurvDir.z;
        str >> value.cMinCurvDir.x >> value.cMinCurvDir.y >> value.cMinCurvDir.z;
    }

    setValues(values);
}

App::Property* PropertyCurvatureList::Copy() const
{
    PropertyCurvatureList* prop = new PropertyCurvatureList();
    prop->_lValueList = this->_lValueList;
    return prop;
}

void PropertyCurvatureList::Paste(const App::Property& from)
{
    aboutToSetValue();
    const PropertyCurvatureList& prop = dynamic_cast<const PropertyCurvatureList&>(from);
    this->_lValueList = prop._lValueList;
    hasSetValue();
}

unsigned int PropertyCurvatureList::getMemSize() const
{
    return sizeof(CurvatureInfo) * this->_lValueList.size();
}

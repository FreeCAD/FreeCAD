/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <memory>
#endif

#include "ComplexGeoData.h"
#include "StringHasher.h"

// inclusion of the generated files (generated out of ComplexGeoDataPy.xml)
#include <App/ComplexGeoDataPy.h>
#include <App/ComplexGeoDataPy.cpp>
#include <App/StringHasherPy.h>
#include <App/StringIDPy.h>
#include <Base/BoundBoxPy.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include "Base/PyWrapParseTupleAndKeywords.h"
#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

using namespace Data;
using namespace Base;

// returns a string which represent the object e.g. when printed in python
std::string ComplexGeoDataPy::representation() const
{
    return {"<ComplexGeoData object>"};
}

PyObject* ComplexGeoDataPy::getElementTypes(PyObject* args) const
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    std::vector<const char*> types = getComplexGeoDataPtr()->getElementTypes();
    Py::List list;
    for (auto it : types) {
        list.append(Py::String(it));
    }
    return Py::new_reference_to(list);
}

PyObject* ComplexGeoDataPy::countSubElements(PyObject* args) const
{
    char* type;
    if (!PyArg_ParseTuple(args, "s", &type)) {
        return nullptr;
    }

    try {
        unsigned long count = getComplexGeoDataPtr()->countSubElements(type);
        return Py::new_reference_to(Py::Long(count));
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "failed to count sub-elements from object");
        return nullptr;
    }
}

PyObject* ComplexGeoDataPy::getFacesFromSubElement(PyObject* args) const
{
    char* type;
    unsigned long index;
    if (!PyArg_ParseTuple(args, "sk", &type, &index)) {
        return nullptr;
    }

    std::vector<Base::Vector3d> points;
    std::vector<Base::Vector3d> normals;
    std::vector<Data::ComplexGeoData::Facet> facets;
    try {
        std::unique_ptr<Data::Segment> segm(getComplexGeoDataPtr()->getSubElement(type, index));
        getComplexGeoDataPtr()->getFacesFromSubElement(segm.get(), points, normals, facets);
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "failed to get sub-element from object");
        return nullptr;
    }

    Py::Tuple tuple(2);
    Py::List vertex;
    for (const auto& it : points) {
        vertex.append(Py::asObject(new Base::VectorPy(it)));
    }
    tuple.setItem(0, vertex);
    Py::List facet;
    for (const auto& it : facets) {
        Py::Tuple f(3);
        f.setItem(0, Py::Long(int(it.I1)));
        f.setItem(1, Py::Long(int(it.I2)));
        f.setItem(2, Py::Long(int(it.I3)));
        facet.append(f);
    }
    tuple.setItem(1, facet);
    return Py::new_reference_to(tuple);
}

PyObject* ComplexGeoDataPy::getLinesFromSubElement(PyObject* args) const
{
    char* type;
    int index;
    if (!PyArg_ParseTuple(args, "si", &type, &index)) {
        return nullptr;
    }

    std::vector<Base::Vector3d> points;
    std::vector<Data::ComplexGeoData::Line> lines;
    try {
        std::unique_ptr<Data::Segment> segm(getComplexGeoDataPtr()->getSubElement(type, index));
        getComplexGeoDataPtr()->getLinesFromSubElement(segm.get(), points, lines);
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "failed to get sub-element from object");
        return nullptr;
    }

    Py::Tuple tuple(2);
    Py::List vertex;
    for (const auto& it : points) {
        vertex.append(Py::asObject(new Base::VectorPy(it)));
    }
    tuple.setItem(0, vertex);
    Py::List line;
    for (const auto& it : lines) {
        Py::Tuple l(2);
        l.setItem(0, Py::Long((int)it.I1));
        l.setItem(1, Py::Long((int)it.I2));
        line.append(l);
    }
    tuple.setItem(1, line);
    return Py::new_reference_to(tuple);
}

PyObject* ComplexGeoDataPy::getPoints(PyObject* args) const
{
    double accuracy = 0.05;
    if (!PyArg_ParseTuple(args, "d", &accuracy)) {
        return nullptr;
    }

    std::vector<Base::Vector3d> points;
    std::vector<Base::Vector3d> normals;
    try {
        getComplexGeoDataPtr()->getPoints(points, normals, accuracy);
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "failed to get sub-element from object");
        return nullptr;
    }

    Py::Tuple tuple(2);
    Py::List vertex;
    for (const auto& it : points) {
        vertex.append(Py::asObject(new Base::VectorPy(it)));
    }
    tuple.setItem(0, vertex);

    Py::List normal;
    for (const auto& it : normals) {
        normal.append(Py::asObject(new Base::VectorPy(it)));
    }
    tuple.setItem(1, normal);
    return Py::new_reference_to(tuple);
}

PyObject* ComplexGeoDataPy::getLines(PyObject* args) const
{
    double accuracy = 0.05;
    if (!PyArg_ParseTuple(args, "d", &accuracy)) {
        return nullptr;
    }

    std::vector<Base::Vector3d> points;
    std::vector<Data::ComplexGeoData::Line> lines;
    try {
        getComplexGeoDataPtr()->getLines(points, lines, accuracy);
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "failed to get sub-element from object");
        return nullptr;
    }

    Py::Tuple tuple(2);
    Py::List vertex;
    for (const auto& it : points) {
        vertex.append(Py::asObject(new Base::VectorPy(it)));
    }
    tuple.setItem(0, vertex);
    Py::List line;
    for (const auto& it : lines) {
        Py::Tuple l(2);
        l.setItem(0, Py::Long((int)it.I1));
        l.setItem(1, Py::Long((int)it.I2));
        line.append(l);
    }
    tuple.setItem(1, line);
    return Py::new_reference_to(tuple);
}

PyObject* ComplexGeoDataPy::getFaces(PyObject* args) const
{
    double accuracy = 0.05;
    if (!PyArg_ParseTuple(args, "d", &accuracy)) {
        return nullptr;
    }

    std::vector<Base::Vector3d> points;
    std::vector<Data::ComplexGeoData::Facet> facets;
    try {
        getComplexGeoDataPtr()->getFaces(points, facets, accuracy);
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "failed to get sub-element from object");
        return nullptr;
    }

    Py::Tuple tuple(2);
    Py::List vertex;
    for (const auto& it : points) {
        vertex.append(Py::asObject(new Base::VectorPy(it)));
    }
    tuple.setItem(0, vertex);
    Py::List facet;
    for (const auto& it : facets) {
        Py::Tuple f(3);
        f.setItem(0, Py::Long((int)it.I1));
        f.setItem(1, Py::Long((int)it.I2));
        f.setItem(2, Py::Long((int)it.I3));
        facet.append(f);
    }
    tuple.setItem(1, facet);
    return Py::new_reference_to(tuple);
}

PyObject* ComplexGeoDataPy::applyTranslation(PyObject* args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &obj)) {
        return nullptr;
    }

    try {
        Base::Vector3d move = static_cast<Base::VectorPy*>(obj)->value();
        getComplexGeoDataPtr()->applyTranslation(move);
        Py_Return;
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "failed to apply rotation");
        return nullptr;
    }
}

PyObject* ComplexGeoDataPy::applyRotation(PyObject* args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O!", &(Base::RotationPy::Type), &obj)) {
        return nullptr;
    }

    try {
        Base::Rotation rot = static_cast<Base::RotationPy*>(obj)->value();
        getComplexGeoDataPtr()->applyRotation(rot);
        Py_Return;
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "failed to apply rotation");
        return nullptr;
    }
}

PyObject* ComplexGeoDataPy::transformGeometry(PyObject* args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type), &obj)) {
        return nullptr;
    }

    try {
        Base::Matrix4D mat = static_cast<Base::MatrixPy*>(obj)->value();
        getComplexGeoDataPtr()->transformGeometry(mat);
        Py_Return;
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "failed to transform geometry");
        return nullptr;
    }
}

PyObject* ComplexGeoDataPy::getElementName(PyObject* args) const
{
    char* input;
    int direction = 0;
    if (!PyArg_ParseTuple(args, "s|i", &input, &direction)) {
        return NULL;
    }

    Data::MappedElement res = getComplexGeoDataPtr()->getElementName(input);
    std::string s;
    if (direction == 1) {
        return Py::new_reference_to(Py::String(res.name.appendToBuffer(s)));
    }
    else if (direction == 0) {
        return Py::new_reference_to(Py::String(res.index.appendToStringBuffer(s)));
    }
    else if (Data::IndexedName(input)) {
        return Py::new_reference_to(Py::String(res.name.appendToBuffer(s)));
    }
    else {
        return Py::new_reference_to(Py::String(res.index.appendToStringBuffer(s)));
    }
}

PyObject* ComplexGeoDataPy::getElementIndexedName(PyObject* args) const
{
    char* input;
    PyObject* returnID = Py_False;
    if (!PyArg_ParseTuple(args, "s|O", &input, &returnID)) {
        return NULL;
    }

    ElementIDRefs ids;
    Data::MappedElement res =
        getComplexGeoDataPtr()->getElementName(input, PyObject_IsTrue(returnID) ? &ids : nullptr);
    std::string s;
    Py::String name(res.index.appendToStringBuffer(s));
    if (!PyObject_IsTrue(returnID)) {
        return Py::new_reference_to(name);
    }

    Py::List list;
    for (auto& id : ids) {
        list.append(Py::Long(id.value()));
    }
    return Py::new_reference_to(Py::TupleN(name, list));
}

PyObject* ComplexGeoDataPy::getElementMappedName(PyObject* args) const
{
    char* input;
    PyObject* returnID = Py_False;
    if (!PyArg_ParseTuple(args, "s|O", &input, &returnID)) {
        return NULL;
    }

    ElementIDRefs ids;
    Data::MappedElement res =
        getComplexGeoDataPtr()->getElementName(input, PyObject_IsTrue(returnID) ? &ids : nullptr);
    std::string s;
    Py::String name(res.name.appendToBuffer(s));
    if (!PyObject_IsTrue(returnID)) {
        return Py::new_reference_to(name);
    }

    Py::List list;
    for (auto& id : ids) {
        list.append(Py::Long(id.value()));
    }
    return Py::new_reference_to(Py::TupleN(name, list));
}

PyObject* ComplexGeoDataPy::setElementName(PyObject* args, PyObject* kwds)
{
    const char* element;
    const char* name = 0;
    const char* postfix = 0;
    int tag = 0;
    PyObject* pySid = Py_None;
    PyObject* overwrite = Py_False;

    const std::array<const char*, 7> kwlist =
        {"element", "name", "postfix", "overwrite", "sid", "tag", nullptr};
    if (!Wrapped_ParseTupleAndKeywords(args,
                                       kwds,
                                       "s|sssOOi",
                                       kwlist,
                                       &element,
                                       &name,
                                       &postfix,
                                       &overwrite,
                                       &pySid,
                                       &tag)) {
        return NULL;
    }
    ElementIDRefs sids;
    if (pySid != Py_None) {
        if (PyObject_TypeCheck(pySid, &App::StringIDPy::Type)) {
            sids.push_back(static_cast<App::StringIDPy*>(pySid)->getStringIDPtr());
        }
        else if (PySequence_Check(pySid)) {
            Py::Sequence seq(pySid);
            for (auto it = seq.begin(); it != seq.end(); ++it) {
                auto ptr = (*it).ptr();
                if (PyObject_TypeCheck(ptr, &App::StringIDPy::Type)) {
                    sids.push_back(static_cast<App::StringIDPy*>(ptr)->getStringIDPtr());
                }
                else {
                    throw Py::TypeError("expect StringID in sid sequence");
                }
            }
        }
        else {
            throw Py::TypeError("expect sid to contain either StringID or sequence of StringID");
        }
    }
    PY_TRY
    {
        Data::IndexedName index(element, getComplexGeoDataPtr()->getElementTypes());
        Data::MappedName mapped = Data::MappedName::fromRawData(name);
        std::ostringstream ss;
        ElementMapPtr map = getComplexGeoDataPtr()->resetElementMap();
        map->encodeElementName(getComplexGeoDataPtr()->elementType(index),
                               mapped,
                               ss,
                               &sids,
                               tag,
                               postfix,
                               tag);
        Data::MappedName res =
            map->setElementName(index, mapped, tag, &sids, PyObject_IsTrue(overwrite));
        return Py::new_reference_to(Py::String(res.toString(0)));
    }
    PY_CATCH
}

Py::Object ComplexGeoDataPy::getHasher() const
{
    auto self = getComplexGeoDataPtr();
    if (!self->Hasher) {
        return Py::None();
    }
    return Py::Object(self->Hasher->getPyObject(), true);
}

Py::Dict ComplexGeoDataPy::getElementMap() const
{
    Py::Dict ret;
    std::string s;
    for (auto& v : getComplexGeoDataPtr()->getElementMap()) {
        s.clear();
        ret.setItem(v.name.toString(0), Py::String(v.index.appendToStringBuffer(s)));
    }
    return ret;
}

void ComplexGeoDataPy::setElementMap(Py::Dict dict)
{
    std::vector<Data::MappedElement> map;
    const auto& types = getComplexGeoDataPtr()->getElementTypes();
    for (auto it = dict.begin(); it != dict.end(); ++it) {
        const auto& value = *it;
        if (!value.first.isString() || !value.second.isString()) {
            throw Py::TypeError("expect only strings in the dict");
        }
        map.emplace_back(Data::MappedName(value.first.as_string().c_str()),
                         Data::IndexedName(Py::Object(value.second).as_string().c_str(), types));
    }
    getComplexGeoDataPtr()->setElementMap(map);
}

Py::Dict ComplexGeoDataPy::getElementReverseMap() const
{
    Py::Dict ret;
    std::string s;
    for (auto& v : getComplexGeoDataPtr()->getElementMap()) {
        s.clear();
        auto value = ret[Py::String(v.index.appendToStringBuffer(s))];
        Py::Object item(value);
        if (item.isNone()) {
            s.clear();
            value = Py::String(v.name.appendToBuffer(s));
        }
        else if (item.isList()) {
            Py::List list(item);
            s.clear();
            list.append(Py::String(v.name.appendToBuffer(s)));
        }
        else {
            Py::List list;
            list.append(item);
            s.clear();
            list.append(Py::String(v.name.appendToBuffer(s)));
            value = list;
        }
    }
    return ret;
}

Py::Long ComplexGeoDataPy::getElementMapSize() const
{
    return Py::Long((long)getComplexGeoDataPtr()->getElementMapSize());
}

void ComplexGeoDataPy::setHasher(Py::Object obj)
{
    auto self = getComplexGeoDataPtr();
    if (obj.isNone()) {
        if (self->Hasher) {
            self->Hasher = App::StringHasherRef();
            self->resetElementMap();
        }
    }
    else if (PyObject_TypeCheck(obj.ptr(), &App::StringHasherPy::Type)) {
        App::StringHasherRef ref(
            static_cast<App::StringHasherPy*>(obj.ptr())->getStringHasherPtr());
        if (self->Hasher != ref) {
            self->Hasher = ref;
            self->resetElementMap();
        }
    }
    else {
        throw Py::TypeError("invalid type");
    }
}

Py::Object ComplexGeoDataPy::getBoundBox() const
{
    return Py::BoundingBox(getComplexGeoDataPtr()->getBoundBox());
}

Py::Object ComplexGeoDataPy::getCenterOfGravity() const
{
    Base::Vector3d center;
    if (getComplexGeoDataPtr()->getCenterOfGravity(center)) {
        return Py::Vector(center);
    }
    throw Py::RuntimeError("Cannot get center of gravity");
}

Py::Object ComplexGeoDataPy::getPlacement() const
{
    return Py::Placement(getComplexGeoDataPtr()->getPlacement());
}

void ComplexGeoDataPy::setPlacement(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::PlacementPy::Type))) {
        Base::Placement* trf = static_cast<Base::PlacementPy*>(p)->getPlacementPtr();
        getComplexGeoDataPtr()->setPlacement(*trf);
    }
    else {
        std::string error = std::string("type must be 'Placement', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::String ComplexGeoDataPy::getElementMapVersion() const
{
    return Py::String(getComplexGeoDataPtr()->getElementMapVersion());
}


Py::Long ComplexGeoDataPy::getTag() const
{
    return Py::Long(getComplexGeoDataPtr()->Tag);
}

void ComplexGeoDataPy::setTag(Py::Long tag)
{
    getComplexGeoDataPtr()->Tag = tag;
}

PyObject* ComplexGeoDataPy::getCustomAttributes(const char* attr) const
{
    // Support for backward compatibility
    if (strcmp(attr, "Matrix") == 0) {
        Py::Matrix mat(getComplexGeoDataPtr()->getTransform());
        return Py::new_reference_to(mat);
    }
    return nullptr;
}

int ComplexGeoDataPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    // Support for backward compatibility
    if (strcmp(attr, "Matrix") == 0) {
        if (PyObject_TypeCheck(obj, &(Base::MatrixPy::Type))) {
            Base::Matrix4D mat = static_cast<Base::MatrixPy*>(obj)->value();
            getComplexGeoDataPtr()->setTransform(mat);
            return 1;
        }
        else {
            std::string error = std::string("type must be 'Matrix', not ");
            error += obj->ob_type->tp_name;
            throw Py::TypeError(error);
        }
    }
    return 0;
}

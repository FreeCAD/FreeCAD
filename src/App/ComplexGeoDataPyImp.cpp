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
# include <memory>
#endif

#include "ComplexGeoData.h"

// inclusion of the generated files (generated out of ComplexGeoDataPy.xml)
#include <App/ComplexGeoDataPy.h>
#include <App/ComplexGeoDataPy.cpp>
#include <Base/BoundBoxPy.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

using namespace Data;
using namespace Base;

// returns a string which represent the object e.g. when printed in python
std::string ComplexGeoDataPy::representation() const
{
    return {"<ComplexGeoData object>"};
}

PyObject* ComplexGeoDataPy::getElementTypes(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    std::vector<const char*> types = getComplexGeoDataPtr()->getElementTypes();
    Py::List list;
    for (auto it : types) {
        list.append(Py::String(it));
    }
    return Py::new_reference_to(list);
}

PyObject* ComplexGeoDataPy::countSubElements(PyObject *args)
{
    char *type;
    if (!PyArg_ParseTuple(args, "s", &type))
        return nullptr;

    try {
        unsigned long count = getComplexGeoDataPtr()->countSubElements(type);
        return Py::new_reference_to(Py::Long(count));
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "failed to count sub-elements from object");
        return nullptr;
    }
}

PyObject* ComplexGeoDataPy::getFacesFromSubElement(PyObject *args)
{
    char *type;
    unsigned long index;
    if (!PyArg_ParseTuple(args, "sk", &type, &index))
        return nullptr;

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
    for (const auto & it : points)
        vertex.append(Py::asObject(new Base::VectorPy(it)));
    tuple.setItem(0, vertex);
    Py::List facet;
    for (const auto & it : facets) {
        Py::Tuple f(3);
        f.setItem(0,Py::Int(int(it.I1)));
        f.setItem(1,Py::Int(int(it.I2)));
        f.setItem(2,Py::Int(int(it.I3)));
        facet.append(f);
    }
    tuple.setItem(1, facet);
    return Py::new_reference_to(tuple);
}

PyObject* ComplexGeoDataPy::getLinesFromSubElement(PyObject *args)
{
    char *type;
    int index;
    if (!PyArg_ParseTuple(args, "si", &type, &index))
        return nullptr;

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
    for (const auto & it : points)
        vertex.append(Py::asObject(new Base::VectorPy(it)));
    tuple.setItem(0, vertex);
    Py::List line;
    for (const auto & it : lines) {
        Py::Tuple l(2);
        l.setItem(0,Py::Int((int)it.I1));
        l.setItem(1,Py::Int((int)it.I2));
        line.append(l);
    }
    tuple.setItem(1, line);
    return Py::new_reference_to(tuple);
}

PyObject* ComplexGeoDataPy::getPoints(PyObject *args)
{
    double accuracy = 0.05;
    if (!PyArg_ParseTuple(args, "d", &accuracy))
        return nullptr;

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
    for (const auto & it : points) {
        vertex.append(Py::asObject(new Base::VectorPy(it)));
    }
    tuple.setItem(0, vertex);

    Py::List normal;
    for (const auto & it : normals) {
        normal.append(Py::asObject(new Base::VectorPy(it)));
    }
    tuple.setItem(1, normal);
    return Py::new_reference_to(tuple);
}

PyObject* ComplexGeoDataPy::getLines(PyObject *args)
{
    double accuracy = 0.05;
    if (!PyArg_ParseTuple(args, "d", &accuracy))
        return nullptr;

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
    for (const auto & it : points)
        vertex.append(Py::asObject(new Base::VectorPy(it)));
    tuple.setItem(0, vertex);
    Py::List line;
    for (const auto & it : lines) {
        Py::Tuple l(2);
        l.setItem(0,Py::Int((int)it.I1));
        l.setItem(1,Py::Int((int)it.I2));
        line.append(l);
    }
    tuple.setItem(1, line);
    return Py::new_reference_to(tuple);
}

PyObject* ComplexGeoDataPy::getFaces(PyObject *args)
{
    double accuracy = 0.05;
    if (!PyArg_ParseTuple(args, "d", &accuracy))
        return nullptr;

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
    for (const auto & it : points)
        vertex.append(Py::asObject(new Base::VectorPy(it)));
    tuple.setItem(0, vertex);
    Py::List facet;
    for (const auto & it : facets) {
        Py::Tuple f(3);
        f.setItem(0,Py::Int((int)it.I1));
        f.setItem(1,Py::Int((int)it.I2));
        f.setItem(2,Py::Int((int)it.I3));
        facet.append(f);
    }
    tuple.setItem(1, facet);
    return Py::new_reference_to(tuple);
}

PyObject* ComplexGeoDataPy::applyTranslation(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type),&obj))
        return nullptr;

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

PyObject* ComplexGeoDataPy::applyRotation(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(Base::RotationPy::Type),&obj))
        return nullptr;

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

PyObject* ComplexGeoDataPy::transformGeometry(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type),&obj))
        return nullptr;

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

Py::Object ComplexGeoDataPy::getBoundBox() const
{
    return Py::BoundingBox(getComplexGeoDataPtr()->getBoundBox());
}

Py::Object ComplexGeoDataPy::getCenterOfGravity() const
{
    Base::Vector3d center;
    if (getComplexGeoDataPtr()->getCenterOfGravity(center))
        return Py::Vector(center);
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

Py::Int ComplexGeoDataPy::getTag() const
{
    return Py::Int(getComplexGeoDataPtr()->Tag);
}

void ComplexGeoDataPy::setTag(Py::Int tag)
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

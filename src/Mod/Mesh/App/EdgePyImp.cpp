/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <Base/Converter.h>
#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "Edge.h"
#include "EdgePy.h"
#include "EdgePy.cpp"


using namespace Mesh;

// returns a string which represent the object e.g. when printed in python
std::string EdgePy::representation() const
{
    // clang-format off
    EdgePy::PointerType ptr = getEdgePtr();
    std::stringstream str;
    str << "Edge (";
    str << "(" << ptr->_aclPoints[0].x << ", "
               << ptr->_aclPoints[0].y << ", "
               << ptr->_aclPoints[0].z << ", Idx="
               << ptr->PIndex[0] << "), ";
    str << "(" << ptr->_aclPoints[1].x << ", "
               << ptr->_aclPoints[1].y << ", "
               << ptr->_aclPoints[1].z << ", Idx="
               << ptr->PIndex[1] << "), ";
    str << "Idx=" << ptr->Index << ", ("
                  << ptr->NIndex[0] << ", "
                  << ptr->NIndex[1] << ")";
    str << ")";
    // clang-format on

    return str.str();
}

PyObject* EdgePy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of EdgePy and the Twin object
    return new EdgePy(new Edge);
}

// constructor method
int EdgePy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* pt1 = nullptr;
    PyObject* pt2 = nullptr;
    if (!PyArg_ParseTuple(args,
                          "|O!O!",
                          &Base::VectorPy::Type,
                          &pt1,
                          &Base::VectorPy::Type,
                          &pt2)) {
        return -1;
    }

    if (pt1) {
        getEdgePtr()->_aclPoints[0] =
            Base::convertTo<Base::Vector3f>(Py::Vector(pt1, false).toVector());
    }
    if (pt2) {
        getEdgePtr()->_aclPoints[1] =
            Base::convertTo<Base::Vector3f>(Py::Vector(pt2, false).toVector());
    }
    return 0;
}

Py::Long EdgePy::getIndex() const
{
    return Py::Long((long)getEdgePtr()->Index);
}

PyObject* EdgePy::intersectWithEdge(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &EdgePy::Type, &object)) {
        return nullptr;
    }
    EdgePy* edge = static_cast<EdgePy*>(object);
    EdgePy::PointerType edge_ptr = edge->getEdgePtr();
    EdgePy::PointerType this_ptr = this->getEdgePtr();
    Base::Vector3f p;
    bool ok = this_ptr->IntersectWithEdge(*edge_ptr, p);

    try {
        Py::List sct;
        if (ok) {
            Py::Tuple pt(3);
            pt.setItem(0, Py::Float(p.x));
            pt.setItem(1, Py::Float(p.y));
            pt.setItem(2, Py::Float(p.z));
            sct.append(pt);
        }
        return Py::new_reference_to(sct);
    }
    catch (const Py::Exception&) {
        return nullptr;
    }
}

PyObject* EdgePy::isParallel(PyObject* args)
{
    PyObject* object {};
    if (!PyArg_ParseTuple(args, "O!", &EdgePy::Type, &object)) {
        return nullptr;
    }
    EdgePy* edge = static_cast<EdgePy*>(object);
    EdgePy::PointerType edge_ptr = edge->getEdgePtr();
    EdgePy::PointerType this_ptr = this->getEdgePtr();
    bool ok = this_ptr->IsParallel(*edge_ptr);
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* EdgePy::isCollinear(PyObject* args)
{
    PyObject* object {};
    if (!PyArg_ParseTuple(args, "O!", &EdgePy::Type, &object)) {
        return nullptr;
    }
    EdgePy* edge = static_cast<EdgePy*>(object);
    EdgePy::PointerType edge_ptr = edge->getEdgePtr();
    EdgePy::PointerType this_ptr = this->getEdgePtr();
    bool ok = this_ptr->IsCollinear(*edge_ptr);
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* EdgePy::unbound(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    getEdgePtr()->unbound();
    Py_Return;
}

Py::Boolean EdgePy::getBound() const
{
    return {getEdgePtr()->isBound()};
}

Py::List EdgePy::getPoints() const
{
    EdgePy::PointerType edge = this->getEdgePtr();

    Py::List pts;
    for (const auto& pnt : edge->_aclPoints) {
        Py::Tuple pt(3);
        pt.setItem(0, Py::Float(pnt.x));
        pt.setItem(1, Py::Float(pnt.y));
        pt.setItem(2, Py::Float(pnt.z));
        pts.append(pt);
    }

    return pts;
}

Py::Tuple EdgePy::getPointIndices() const
{
    EdgePy::PointerType edge = this->getEdgePtr();

    Py::Tuple idxTuple(2);
    for (int i = 0; i < 2; i++) {
        idxTuple.setItem(i, Py::Long(edge->PIndex[i]));
    }
    return idxTuple;
}

Py::Tuple EdgePy::getNeighbourIndices() const
{
    EdgePy::PointerType edge = this->getEdgePtr();

    Py::Tuple idxTuple(2);
    for (int i = 0; i < 2; i++) {
        idxTuple.setItem(i, Py::Long(edge->NIndex[i]));
    }
    return idxTuple;
}

Py::Float EdgePy::getLength() const
{
    EdgePy::PointerType edge = this->getEdgePtr();
    return Py::Float(Base::Distance(edge->_aclPoints[0], edge->_aclPoints[1]));
}

PyObject* EdgePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int EdgePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

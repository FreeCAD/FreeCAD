/***************************************************************************
 *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
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

#include "Base/Vector3D.h"
#include "Base/VectorPy.h"

#include "VoronoiVertexPy.h"
#include "VoronoiVertexPy.cpp"
#include "VoronoiEdgePy.h"


using namespace Path;

// returns a string which represents the object e.g. when printed in python
std::string VoronoiVertexPy::representation() const
{
  std::stringstream ss;
  ss.precision(5);
  ss << "VoronoiVertex(";
  VoronoiVertex *v = getVoronoiVertexPtr();
  if (v->isBound()) {
    ss << "[" << (v->ptr->x() / v->dia->getScale()) << ", " << (v->ptr->y() / v->dia->getScale()) << "]";
  }
  ss << ")";
  return ss.str();
}

PyObject *VoronoiVertexPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
  // create a new instance of VoronoiVertexPy and the Twin object
  return new VoronoiVertexPy(new VoronoiVertex);
}

// constructor method
int VoronoiVertexPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
  if (!PyArg_ParseTuple(args, "")) {
    PyErr_SetString(PyExc_RuntimeError, "no arguments accepted");
    return -1;
  }
  return 0;
}


PyObject* VoronoiVertexPy::richCompare(PyObject *lhs, PyObject *rhs, int op) {
  PyObject *cmp = (op == Py_EQ) ? Py_False : Py_True;
  if (   PyObject_TypeCheck(lhs, &VoronoiVertexPy::Type)
      && PyObject_TypeCheck(rhs, &VoronoiVertexPy::Type)
      && (op == Py_EQ || op == Py_NE)) {
    const VoronoiVertex *vl = static_cast<VoronoiVertexPy*>(lhs)->getVoronoiVertexPtr();
    const VoronoiVertex *vr = static_cast<VoronoiVertexPy*>(rhs)->getVoronoiVertexPtr();
    if (vl->index == vr->index && vl->dia == vr->dia) {
      cmp = (op == Py_EQ) ? Py_True : Py_False;
    }
  }
  Py_INCREF(cmp);
  return cmp;
}

const Voronoi::voronoi_diagram_type::vertex_type* getVertexFromPy(VoronoiVertexPy *v, bool throwIfNotBound = true) {
  auto self = v->getVoronoiVertexPtr();
  if (self->isBound()) {
    return self->ptr;
  }
  if (throwIfNotBound) {
    throw Py::TypeError("Vertex not bound to voronoi diagram");
  }
  return nullptr;
}

VoronoiVertex* getVoronoiVertexFromPy(const VoronoiVertexPy *v, PyObject *args = nullptr) {
  VoronoiVertex *self = v->getVoronoiVertexPtr();
  if (!self->isBound()) {
    throw Py::TypeError("Vertex not bound to voronoi diagram");
  }
  if (args && !PyArg_ParseTuple(args, "")) {
    throw Py::RuntimeError("No arguments accepted");
  }
  return self;
}


Py::Long VoronoiVertexPy::getIndex() const {
  VoronoiVertex *v = getVoronoiVertexPtr();
  if (v->isBound()) {
    return Py::Long(v->dia->index(v->ptr));
  }
  return Py::Long(-1);
}

Py::Long VoronoiVertexPy::getColor() const {
  VoronoiVertex *v = getVoronoiVertexPtr();
  if (v->isBound()) {
    Voronoi::color_type color = v->ptr->color() & Voronoi::ColorMask;
    return Py::Long(PyLong_FromSize_t(color));
  }
  return Py::Long(0);
}

void VoronoiVertexPy::setColor(Py::Long color) {
  getVertexFromPy(this)->color(long(color) & Voronoi::ColorMask);
}

Py::Float VoronoiVertexPy::getX() const
{
  VoronoiVertex *v = getVoronoiVertexFromPy(this);
  return Py::Float(v->ptr->x() / v->dia->getScale());
}

Py::Float VoronoiVertexPy::getY() const
{
  VoronoiVertex *v = getVoronoiVertexFromPy(this);
  return Py::Float(v->ptr->y() / v->dia->getScale());
}

Py::Object VoronoiVertexPy::getIncidentEdge() const {
  VoronoiVertex *v = getVoronoiVertexFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(v->dia, v->ptr->incident_edge())));
}

PyObject* VoronoiVertexPy::toPoint(PyObject *args)
{
  double z = 0.0;
  if (!PyArg_ParseTuple(args, "|d", &z)) {
    throw Py::RuntimeError("single argument of type double accepted");
  }
  VoronoiVertex *v = getVoronoiVertexPtr();
  if (v->isBound()) {
    return new Base::VectorPy(new Base::Vector3d(v->ptr->x() / v->dia->getScale(), v->ptr->y() / v->dia->getScale(), z));
  }
  Py_INCREF(Py_None);
  return Py_None;
}

// custom attributes get/set

PyObject* VoronoiVertexPy::getCustomAttributes(const char* /*attr*/) const
{
  return nullptr;
}

int VoronoiVertexPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
  return 0;
}


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

#include "VoronoiCellPy.h"
#include "VoronoiCellPy.cpp"
#include "VoronoiEdgePy.h"


using namespace Path;

// returns a string which represents the object e.g. when printed in python
std::string VoronoiCellPy::representation() const
{
  std::stringstream ss;
  ss.precision(5);
  ss << "VoronoiCell(";
  VoronoiCell *c = getVoronoiCellPtr();
  if (c->isBound()) {
    ss << c->ptr->source_category() << ":" << c->ptr->source_index();
  }
  ss << ")";
  return ss.str();
}

PyObject *VoronoiCellPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
  // create a new instance of VoronoiCellPy and the Twin object
  return new VoronoiCellPy(new VoronoiCell);
}

// constructor method
int VoronoiCellPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
  if (!PyArg_ParseTuple(args, "")) {
    PyErr_SetString(PyExc_RuntimeError, "no arguments accepted");
    return -1;
  }
  return 0;
}


PyObject* VoronoiCellPy::richCompare(PyObject *lhs, PyObject *rhs, int op) {
  PyObject *cmp = (op == Py_EQ) ? Py_False : Py_True;
  if (   PyObject_TypeCheck(lhs, &VoronoiCellPy::Type)
      && PyObject_TypeCheck(rhs, &VoronoiCellPy::Type)
      && (op == Py_EQ || op == Py_NE)) {
    const VoronoiCell *vl = static_cast<VoronoiCellPy*>(lhs)->getVoronoiCellPtr();
    const VoronoiCell *vr = static_cast<VoronoiCellPy*>(rhs)->getVoronoiCellPtr();
    if (vl->index == vr->index && vl->dia == vr->dia) {
      cmp = (op == Py_EQ) ? Py_True : Py_False;
    }
  }
  Py_INCREF(cmp);
  return cmp;
}

const Voronoi::voronoi_diagram_type::cell_type* getCellFromPy(VoronoiCellPy *c, bool throwIfNotBound = true) {
  auto self = c->getVoronoiCellPtr();
  if (self->isBound()) {
    return self->ptr;
  }
  if (throwIfNotBound) {
    throw Py::TypeError("Cell not bound to voronoi diagram");
  }
  return nullptr;
}

VoronoiCell* getVoronoiCellFromPy(const VoronoiCellPy *c, PyObject *args = nullptr) {
  VoronoiCell *self = c->getVoronoiCellPtr();
  if (!self->isBound()) {
    throw Py::TypeError("Cell not bound to voronoi diagram");
  }
  if (args && !PyArg_ParseTuple(args, "")) {
    throw Py::RuntimeError("No arguments accepted");
  }
  return self;
}

Py::Long VoronoiCellPy::getIndex() const {
  VoronoiCell *c = getVoronoiCellPtr();
  if (c->isBound()) {
    return Py::Long(c->dia->index(c->ptr));
  }
  return Py::Long(-1);
}

Py::Long VoronoiCellPy::getColor() const {
  VoronoiCell *c = getVoronoiCellPtr();
  if (c->isBound()) {
    Voronoi::color_type color = c->ptr->color() & Voronoi::ColorMask;
    return Py::Long(PyLong_FromSize_t(color));
  }
  return Py::Long(0);
}

void VoronoiCellPy::setColor(Py::Long color) {
  getCellFromPy(this)->color(long(color) & Voronoi::ColorMask);
}

Py::Long VoronoiCellPy::getSourceIndex() const
{
  VoronoiCell *c = getVoronoiCellFromPy(this);
  long index = c->ptr->source_index();
  return Py::Long(index);
}

Py::Int VoronoiCellPy::getSourceCategory() const
{
  VoronoiCell *c = getVoronoiCellFromPy(this);
  return Py::Int(c->ptr->source_category());
}

Py::String VoronoiCellPy::getSourceCategoryName() const
{
  VoronoiCell *c = getVoronoiCellFromPy(this);
  switch (c->ptr->source_category()) {
    case boost::polygon::SOURCE_CATEGORY_SINGLE_POINT:        return Py::String("SINGLE_POINT");
    case boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT: return Py::String("SEGMENT_START_POINT");
    case boost::polygon::SOURCE_CATEGORY_SEGMENT_END_POINT:   return Py::String("SEGMENT_END_POINT");
    case boost::polygon::SOURCE_CATEGORY_INITIAL_SEGMENT:     return Py::String("INITIAL_SEGMENT");
    case boost::polygon::SOURCE_CATEGORY_REVERSE_SEGMENT:     return Py::String("REVERSE_SEGMENT");
    case boost::polygon::SOURCE_CATEGORY_GEOMETRY_SHIFT:      return Py::String("GEOMETRY_SHIFT");
    case boost::polygon::SOURCE_CATEGORY_BITMASK:             return Py::String("BITMASK");
  }
  return Py::String("");
}

Py::Object VoronoiCellPy::getIncidentEdge() const
{
  VoronoiCell *c = getVoronoiCellFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(c->dia, c->ptr->incident_edge())));
}

PyObject* VoronoiCellPy::containsPoint(PyObject *args)
{
  VoronoiCell *c = getVoronoiCellFromPy(this, args);
  PyObject *chk = c->ptr->contains_point() ? Py_True : Py_False;
  Py_INCREF(chk);
  return chk;
}

PyObject* VoronoiCellPy::containsSegment(PyObject *args)
{
  VoronoiCell *c = getVoronoiCellFromPy(this, args);
  PyObject *chk = c->ptr->contains_segment() ? Py_True : Py_False;
  Py_INCREF(chk);
  return chk;
}

PyObject* VoronoiCellPy::isDegenerate(PyObject *args)
{
  VoronoiCell *c = getVoronoiCellFromPy(this, args);
  PyObject *chk = c->ptr->is_degenerate() ? Py_True : Py_False;
  Py_INCREF(chk);
  return chk;
}

PyObject* VoronoiCellPy::getSource(PyObject *args)
{
  double z = 0;
  if (!PyArg_ParseTuple(args, "|d", &z)) {
    throw Py::TypeError("Optional z argument (double) accepted");
  }

  VoronoiCell *c = getVoronoiCellFromPy(this);
  if (c->ptr->contains_point()) {
    Base::Vector3d v = c->dia->scaledVector(c->dia->retrievePoint(c->ptr), z);
    return new Base::VectorPy(new Base::Vector3d(v));
  }
  Voronoi::segment_type s = c->dia->retrieveSegment(c->ptr);
  Base::Vector3d v0 = c->dia->scaledVector(low(s), z);
  Base::Vector3d v1 = c->dia->scaledVector(high(s), z);
  Py::List list;
  list.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(v0))));
  list.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(v1))));
  return Py::new_reference_to(list);
}


// custom attributes get/set

PyObject* VoronoiCellPy::getCustomAttributes(const char* /*attr*/) const
{
  return nullptr;
}

int VoronoiCellPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
  return 0;
}


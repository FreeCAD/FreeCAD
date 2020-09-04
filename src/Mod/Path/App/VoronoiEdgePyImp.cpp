/***************************************************************************
 *   Copyright (c) sliptonic (shopinthewoods@gmail.com) 2020               *
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
# include <boost/algorithm/string.hpp>
#endif

#include "Mod/Path/App/Voronoi.h"
#include "Mod/Path/App/Voronoi.h"
#include "Mod/Path/App/VoronoiCell.h"
#include "Mod/Path/App/VoronoiCellPy.h"
#include "Mod/Path/App/VoronoiEdge.h"
#include "Mod/Path/App/VoronoiEdgePy.h"
#include "Mod/Path/App/VoronoiVertex.h"
#include "Mod/Path/App/VoronoiVertexPy.h"
#include <Base/Exception.h>
#include <Base/GeometryPyCXX.h>
#include <Base/PlacementPy.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

// files generated out of VoronoiEdgePy.xml
#include "VoronoiEdgePy.cpp"

using namespace Path;

// returns a string which represents the object e.g. when printed in python
std::string VoronoiEdgePy::representation(void) const
{
  std::stringstream ss;
  ss.precision(5);
  ss << "VoronoiEdge(";
  VoronoiEdge *e = getVoronoiEdgePtr();
  if (e->isBound()) {
    const Voronoi::diagram_type::vertex_type *v0 = e->ptr->vertex0();
    const Voronoi::diagram_type::vertex_type *v1 = e->ptr->vertex1();
    if (v0) {
      ss << "[" << v0->x() << ", " << v0->y() << "]";
    } else {
      ss << "[~]";
    }
    ss << ", ";
    if (v1) {
      ss << "[" << v1->x() << ", " << v1->y() << "]";
    } else {
      ss << "[~]";
    }
  }
  ss << ")";
  return ss.str();
}

PyObject *VoronoiEdgePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
  // create a new instance of VoronoiEdgePy and the Twin object
  return new VoronoiEdgePy(new VoronoiEdge);
}

// constructor method
int VoronoiEdgePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
  if (!PyArg_ParseTuple(args, "")) {
    PyErr_SetString(PyExc_RuntimeError, "no arguments accepted");
    return -1;
  }
  return 0;
}


PyObject* VoronoiEdgePy::richCompare(PyObject *lhs, PyObject *rhs, int op) {
  PyObject *cmp = Py_False;
  if (   PyObject_TypeCheck(lhs, &VoronoiEdgePy::Type)
      && PyObject_TypeCheck(rhs, &VoronoiEdgePy::Type)
      && op == Py_EQ) {
    const VoronoiEdge *vl = static_cast<VoronoiEdgePy*>(lhs)->getVoronoiEdgePtr();
    const VoronoiEdge *vr = static_cast<VoronoiEdgePy*>(rhs)->getVoronoiEdgePtr();
    if (vl->index == vr->index && vl->dia == vr->dia) {
      cmp = Py_True;
    } else {
      std::cerr << "VoronoiEdge==(" << vl->index << " != " << vr->index << " || " << (vl->dia == vr->dia) << ")" << std::endl;
    }
  }
  Py_INCREF(cmp);
  return cmp;
}

const Voronoi::voronoi_diagram_type::edge_type* getEdgeFromPy(VoronoiEdgePy *e, bool throwIfNotBound = true) {
  auto self = e->getVoronoiEdgePtr();
  if (self->isBound()) {
    return self->ptr;
  }
  if (throwIfNotBound) {
    throw Py::TypeError("Edge not bound to voronoi diagram");
  }
  return 0;
}

VoronoiEdge* getVoronoiEdgeFromPy(const VoronoiEdgePy *e, PyObject *args = 0) {
  VoronoiEdge *self = e->getVoronoiEdgePtr();
  if (!self->isBound()) {
    throw Py::TypeError("Edge not bound to voronoi diagram");
  }
  if (args && !PyArg_ParseTuple(args, "")) {
    throw Py::RuntimeError("No arguments accepted");
  }
  return self;
}

Py::Int VoronoiEdgePy::getColor(void) const {
  VoronoiEdge *e = getVoronoiEdgePtr();
  if (e->isBound()) {
    return Py::Int(e->dia->edges()[e->index].color());
  }
  return Py::Int(0);
}

void VoronoiEdgePy::setColor(Py::Int color) {
  getEdgeFromPy(this)->color(int(color) & 0x0FFFFFFF);
}

Py::List VoronoiEdgePy::getVertices(void) const
{
  Py::List list;
  VoronoiEdge *e = getVoronoiEdgePtr();
  if (e->isBound()) {
    auto v0 = e->ptr->vertex0();
    auto v1 = e->ptr->vertex1();
    if (v0) {
      list.append(Py::asObject(new VoronoiVertexPy(new VoronoiVertex(e->dia, v0))));
    } else {
      Py_INCREF(Py_None);
      list.append(Py::asObject(Py_None));
    }
    if (v1) {
      list.append(Py::asObject(new VoronoiVertexPy(new VoronoiVertex(e->dia, v1))));
    } else {
      Py_INCREF(Py_None);
      list.append(Py::asObject(Py_None));
    }
  }
  return list;
}

Py::Object VoronoiEdgePy::getTwin(void) const
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(e->dia, e->ptr->twin())));
}

Py::Object VoronoiEdgePy::getNext(void) const
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(e->dia, e->ptr->next())));
}

Py::Object VoronoiEdgePy::getPrev(void) const
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(e->dia, e->ptr->prev())));
}

Py::Object VoronoiEdgePy::getRotatedNext(void) const
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(e->dia, e->ptr->rot_next())));
}

Py::Object VoronoiEdgePy::getRotatedPrev(void) const
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(e->dia, e->ptr->rot_prev())));
}

Py::Object VoronoiEdgePy::getCell(void) const
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this);
  return Py::asObject(new VoronoiCellPy(new VoronoiCell(e->dia, e->ptr->cell())));
}


PyObject* VoronoiEdgePy::isFinite(PyObject *args)
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this, args);
  PyObject *chk = e->ptr->is_finite() ? Py_True : Py_False;
  Py_INCREF(chk);
  return chk;
}

PyObject* VoronoiEdgePy::isInfinite(PyObject *args)
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this, args);
  PyObject *chk = e->ptr->is_infinite() ? Py_True : Py_False;
  Py_INCREF(chk);
  return chk;
}

PyObject* VoronoiEdgePy::isLinear(PyObject *args)
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this, args);
  PyObject *chk = e->ptr->is_linear() ? Py_True : Py_False;
  Py_INCREF(chk);
  return chk;
}

PyObject* VoronoiEdgePy::isCurved(PyObject *args)
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this, args);
  PyObject *chk = e->ptr->is_curved() ? Py_True : Py_False;
  Py_INCREF(chk);
  return chk;
}

PyObject* VoronoiEdgePy::isPrimary(PyObject *args)
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this, args);
  PyObject *chk = e->ptr->is_primary() ? Py_True : Py_False;
  Py_INCREF(chk);
  return chk;
}

PyObject* VoronoiEdgePy::isSecondary(PyObject *args)
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this, args);
  PyObject *chk = e->ptr->is_secondary() ? Py_True : Py_False;
  Py_INCREF(chk);
  return chk;
}


// custom attributes get/set

PyObject* VoronoiEdgePy::getCustomAttributes(const char* /*attr*/) const
{
  return 0;
}

int VoronoiEdgePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
  return 0;
}


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
#include "Mod/Path/App/VoronoiVertex.h"
#include <Base/Exception.h>
#include <Base/GeometryPyCXX.h>
#include <Base/PlacementPy.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

// files generated out of VoronoiVertexPy.xml
#include "VoronoiPy.h"
#include "VoronoiVertexPy.h"
#include "VoronoiVertexPy.cpp"

using namespace Path;

// returns a string which represents the object e.g. when printed in python
std::string VoronoiVertexPy::representation(void) const
{
  std::stringstream ss;
  ss.precision(5);
  ss << "VoronoiVertex(";
  VoronoiVertex *v = getVoronoiVertexPtr();
  if (v->isBound()) {
    Voronoi::diagram_type::vertex_type pt = v->dia->vertices()[v->index];
    ss << "[" << pt.x() << ", " << pt.y() << "]";
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
  PyObject *cmp = Py_False;
  if (   PyObject_TypeCheck(lhs, &VoronoiVertexPy::Type)
      && PyObject_TypeCheck(rhs, &VoronoiVertexPy::Type)
      && op == Py_EQ) {
    const VoronoiVertex *vl = static_cast<VoronoiVertexPy*>(lhs)->getVoronoiVertexPtr();
    const VoronoiVertex *vr = static_cast<VoronoiVertexPy*>(rhs)->getVoronoiVertexPtr();
    if (vl->index == vr->index && &(*vl->dia) == &(*vr->dia)) {
      cmp = Py_True;
    }
  }
  Py_INCREF(cmp);
  return cmp;
}

const Voronoi::voronoi_diagram_type::vertex_type* getVertexFromPy(VoronoiVertexPy *v, bool throwIfNotBound = true) {
  auto self = v->getVoronoiVertexPtr();
  if (self->isBound()) {
    return &self->dia->vertices()[self->index];
  }
  if (throwIfNotBound) {
    throw Py::TypeError("Vertex not bound to voronoi diagram");
  }
  return 0;
}

Py::Int VoronoiVertexPy::getColor(void) const {
  VoronoiVertex *v = getVoronoiVertexPtr();
  if (v->isBound()) {
    return Py::Int(v->dia->vertices()[v->index].color());
  }
  return Py::Int(0);
}

void VoronoiVertexPy::setColor(Py::Int color) {
  getVertexFromPy(this)->color(int(color) & 0x0FFFFFFF);
}

Py::Float VoronoiVertexPy::getX(void) const
{
  VoronoiVertex *v = getVoronoiVertexPtr();
  if (!v->isBound()) {
    throw Py::FloatingPointError("Cannot get coordinates of unbound voronoi vertex");
  }
  return Py::Float(v->dia->vertices()[v->index].x());
}

Py::Float VoronoiVertexPy::getY(void) const
{
  VoronoiVertex *v = getVoronoiVertexPtr();
  if (!v->isBound()) {
    throw Py::FloatingPointError("Cannot get coordinates of unbound voronoi vertex");
  }
  return Py::Float(v->dia->vertices()[v->index].y());
}

#if 0
Py::Object VoronoiVertexPy::getIncidentEdge() const {
  Py_INCREF(Py_None);
  return Py_None;
}
#endif

// custom attributes get/set

PyObject* VoronoiVertexPy::getCustomAttributes(const char* /*attr*/) const
{
  return 0;
}

int VoronoiVertexPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
  return 0;
}


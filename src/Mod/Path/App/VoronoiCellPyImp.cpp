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
#include <Base/Exception.h>
#include <Base/GeometryPyCXX.h>
#include <Base/PlacementPy.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

// files generated out of VoronoiCellPy.xml
#include "VoronoiCellPy.cpp"

using namespace Path;

// returns a string which represents the object e.g. when printed in python
std::string VoronoiCellPy::representation(void) const
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
  PyObject *cmp = Py_False;
  if (   PyObject_TypeCheck(lhs, &VoronoiCellPy::Type)
      && PyObject_TypeCheck(rhs, &VoronoiCellPy::Type)
      && op == Py_EQ) {
    const VoronoiCell *vl = static_cast<VoronoiCellPy*>(lhs)->getVoronoiCellPtr();
    const VoronoiCell *vr = static_cast<VoronoiCellPy*>(rhs)->getVoronoiCellPtr();
    if (vl->index == vr->index && vl->dia == vr->dia) {
      cmp = Py_True;
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
  return 0;
}

VoronoiCell* getVoronoiCellFromPy(const VoronoiCellPy *c, PyObject *args = 0) {
  VoronoiCell *self = c->getVoronoiCellPtr();
  if (!self->isBound()) {
    throw Py::TypeError("Cell not bound to voronoi diagram");
  }
  if (args && !PyArg_ParseTuple(args, "")) {
    throw Py::RuntimeError("No arguments accepted");
  }
  return self;
}

Py::Int VoronoiCellPy::getColor(void) const {
  VoronoiCell *c = getVoronoiCellPtr();
  if (c->isBound()) {
    return Py::Int(c->dia->cells()[c->index].color());
  }
  return Py::Int(0);
}

void VoronoiCellPy::setColor(Py::Int color) {
  getCellFromPy(this)->color(int(color) & 0x0FFFFFFF);
}

Py::Int VoronoiCellPy::getSourceIndex(void) const
{
  VoronoiCell *c = getVoronoiCellFromPy(this);
  return Py::Int(c->ptr->source_index());
}

Py::Int VoronoiCellPy::getSourceCategory(void) const
{
  VoronoiCell *c = getVoronoiCellFromPy(this);
  return Py::Int(c->ptr->source_category());
}

Py::Object VoronoiCellPy::getIncidentEdge(void) const
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


// custom attributes get/set

PyObject* VoronoiCellPy::getCustomAttributes(const char* /*attr*/) const
{
  return 0;
}

int VoronoiCellPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
  return 0;
}


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
#include "Mod/Path/App/VoronoiCell.h"
#include "Mod/Path/App/VoronoiEdge.h"
#include "Mod/Path/App/VoronoiVertex.h"
#include <Base/Exception.h>
#include <Base/GeometryPyCXX.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

// files generated out of VoronoiPy.xml
#include "VoronoiPy.h"
#include "VoronoiPy.cpp"
#include "VoronoiCellPy.h"
#include "VoronoiEdgePy.h"
#include "VoronoiVertexPy.h"

using namespace Path;

// returns a string which represents the object e.g. when printed in python
std::string VoronoiPy::representation(void) const
{
  std::stringstream ss;
  ss.precision(5);
  ss << "Voronoi("
    << "{" << getVoronoiPtr()->vd->segments.size() << ", " << getVoronoiPtr()->vd->points.size() << "}"
    << " -> "
    << "{" << getVoronoiPtr()->numCells() << ", " << getVoronoiPtr()->numEdges() << ", " << getVoronoiPtr()->numVertices() << "}"
    << ")";
  return ss.str();
}


PyObject *VoronoiPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
  // create a new instance of VoronoiPy and its twin object
  return new VoronoiPy(new Voronoi);
}

// constructor
int VoronoiPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
  if (!PyArg_ParseTuple(args, "")) {
    PyErr_SetString(PyExc_RuntimeError, "no arguments accepted");
    return -1;
  }
  return 0;
}

Voronoi::point_type getPointFromPy(PyObject *obj) {
  if (obj) {
    if (PyObject_TypeCheck(obj, &Base::VectorPy::Type)) {
      Base::Vector3d *vect = (static_cast<Base::VectorPy*>(obj))->getVectorPtr();
      return Voronoi::point_type(vect->x, vect->y);
    } else if (PyObject_TypeCheck(obj, Base::Vector2dPy::type_object())) {
      Base::Vector2d vect = Py::toVector2d(obj);
      return Voronoi::point_type(vect.x, vect.y);
    }
  }
  throw Py::TypeError("Points must be Base::Vector or Base::Vector2d");
  return Voronoi::point_type();
}

PyObject* VoronoiPy::addPoint(PyObject *args) {
  PyObject *obj = 0;
  if (PyArg_ParseTuple(args, "O", &obj)) {
    getVoronoiPtr()->vd->points.push_back(getPointFromPy(obj));
  }
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* VoronoiPy::addSegment(PyObject *args) {
  PyObject *objBegin = 0;
  PyObject *objEnd   = 0;

  if (PyArg_ParseTuple(args, "OO", &objBegin, &objEnd)) {
    auto p0 = getPointFromPy(objBegin);
    auto p1 = getPointFromPy(objEnd);
    getVoronoiPtr()->vd->segments.push_back(Voronoi::segment_type(p0, p1));
  }
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* VoronoiPy::construct(PyObject *args) {
  if (!PyArg_ParseTuple(args, "")) {
    throw  Py::RuntimeError("no arguments accepted");
  }
  getVoronoiPtr()->construct();

  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* VoronoiPy::numCells(PyObject *args)
{
  if (!PyArg_ParseTuple(args, "")) {
    throw  Py::RuntimeError("no arguments accepted");
  }
  return PyLong_FromLong(getVoronoiPtr()->numCells());
}

PyObject* VoronoiPy::numEdges(PyObject *args)
{
  if (!PyArg_ParseTuple(args, "")) {
    throw  Py::RuntimeError("no arguments accepted");
  }
  return PyLong_FromLong(getVoronoiPtr()->numEdges());
}

PyObject* VoronoiPy::numVertices(PyObject *args)
{
  if (!PyArg_ParseTuple(args, "")) {
    throw  Py::RuntimeError("no arguments accepted");
  }
  return PyLong_FromLong(getVoronoiPtr()->numVertices());
}

Py::List VoronoiPy::getVertices(void) const {
  Py::List list;
  for (int i=0; i<getVoronoiPtr()->numVertices(); ++i) {
    list.append(Py::asObject(new VoronoiVertexPy(new VoronoiVertex(getVoronoiPtr()->vd, i))));
  }
  return list;
}

Py::List VoronoiPy::getEdges(void) const {
  Py::List list;
  for (int i=0; i<getVoronoiPtr()->numEdges(); ++i) {
    list.append(Py::asObject(new VoronoiEdgePy(new VoronoiEdge(getVoronoiPtr()->vd, i))));
  }
  return list;
}

Py::List VoronoiPy::getCells(void) const {
  Py::List list;
  for (int i=0; i<getVoronoiPtr()->numCells(); ++i) {
    list.append(Py::asObject(new VoronoiCellPy(new VoronoiCell(getVoronoiPtr()->vd, i))));
  }
  return list;
}

// custom attributes get/set

PyObject *VoronoiPy::getCustomAttributes(const char* /*attr*/) const
{
  return 0;
}

int VoronoiPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
  return 0;
}



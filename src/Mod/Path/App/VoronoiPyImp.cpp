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

#include "Base/GeometryPyCXX.h"
#include "Base/Vector3D.h"
#include "Base/VectorPy.h"

#include "VoronoiPy.h"
#include "VoronoiPy.cpp"
#include "VoronoiCellPy.h"
#include "VoronoiEdgePy.h"
#include "VoronoiVertexPy.h"


using namespace Path;

// returns a string which represents the object e.g. when printed in python
std::string VoronoiPy::representation() const
{
  std::stringstream ss;
  ss.precision(5);
  ss << "VoronoiDiagram("
    << "{" << getVoronoiPtr()->numSegments() << ", " << getVoronoiPtr()->numPoints() << "}"
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
  Voronoi *vo = getVoronoiPtr();
  double scale = vo->getScale();
  if (!PyArg_ParseTuple(args, "|d", &scale)) {
    PyErr_SetString(PyExc_RuntimeError, "scale argument (double) accepted, default = 1000");
    return -1;
  }
  vo->setScale(scale);
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
  PyObject *obj = nullptr;
  if (PyArg_ParseTuple(args, "O", &obj)) {
    getVoronoiPtr()->addPoint(getPointFromPy(obj));
  }
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* VoronoiPy::addSegment(PyObject *args) {
  PyObject *objBegin = nullptr;
  PyObject *objEnd   = nullptr;

  if (PyArg_ParseTuple(args, "OO", &objBegin, &objEnd)) {
    auto p0 = getPointFromPy(objBegin);
    auto p1 = getPointFromPy(objEnd);
    getVoronoiPtr()->addSegment(Voronoi::segment_type(p0, p1));
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

Py::List VoronoiPy::getVertices() const {
  Py::List list;
  for (int i=0; i<getVoronoiPtr()->numVertices(); ++i) {
    list.append(Py::asObject(new VoronoiVertexPy(getVoronoiPtr()->create<VoronoiVertex>(i))));
  }
  return list;
}

Py::List VoronoiPy::getEdges() const {
  Py::List list;
  for (int i=0; i<getVoronoiPtr()->numEdges(); ++i) {
    list.append(Py::asObject(new VoronoiEdgePy(getVoronoiPtr()->create<VoronoiEdge>(i))));
  }
  return list;
}

Py::List VoronoiPy::getCells() const {
  Py::List list;
  for (int i=0; i<getVoronoiPtr()->numCells(); ++i) {
    list.append(Py::asObject(new VoronoiCellPy(getVoronoiPtr()->create<VoronoiCell>(i))));
  }
  return list;
}

using exterior_map_t = std::map<uintptr_t,bool>;
using coordinate_map_t = std::map<int32_t, std::set<int32_t> >;

#define VORONOI_USE_EXTERIOR_CACHE 1

static bool callbackWithVertex(Voronoi::diagram_type *dia, PyObject *callback, const Voronoi::diagram_type::vertex_type *v, bool &bail, exterior_map_t &cache) {
  bool rc = false;
  if (!bail && v->color() == 0) {
#if VORONOI_USE_EXTERIOR_CACHE
    auto it = cache.find(uintptr_t(v));
    if (it == cache.end()) {
#endif
      PyObject *vx = new VoronoiVertexPy(new VoronoiVertex(dia, v));
      PyObject *arglist = Py_BuildValue("(O)", vx);
#if PY_VERSION_HEX < 0x03090000
      PyObject *result = PyEval_CallObject(callback, arglist);
#else
      PyObject *result = PyObject_CallObject(callback, arglist);
#endif
      Py_DECREF(arglist);
      Py_DECREF(vx);
      if (!result) {
        bail = true;
      } else {
        rc = result == Py_True;
        Py_DECREF(result);
        cache.insert(exterior_map_t::value_type(uintptr_t(v), rc));
      }
#if VORONOI_USE_EXTERIOR_CACHE
    } else {
      rc = it->second;
    }
#else
    (void)cache;
#endif
  }
  return rc;
}

PyObject* VoronoiPy::colorExterior(PyObject *args) {
  Voronoi::color_type color = 0;
  PyObject *callback = nullptr;
  if (!PyArg_ParseTuple(args, "k|O", &color, &callback)) {
    throw  Py::RuntimeError("colorExterior requires an integer (color) argument");
  }
  Voronoi *vo = getVoronoiPtr();
  vo->colorExterior(color);
  if (callback) {
    exterior_map_t   cache;
    coordinate_map_t pts;
    for (auto e = vo->vd->edges().begin(); e != vo->vd->edges().end(); ++e) {
      if (e->is_finite() && e->color() == 0) {
        const Voronoi::diagram_type::vertex_type *v0 = e->vertex0();
        const Voronoi::diagram_type::vertex_type *v1 = e->vertex1();
        bool bail = false;
        if (callbackWithVertex(vo->vd, callback, v0, bail, cache) && callbackWithVertex(vo->vd, callback, v1, bail, cache)) {
          vo->colorExterior(&(*e), color);
        } else if (!bail && callbackWithVertex(vo->vd, callback, v1, bail, cache)) {
          if (pts.empty()) {
            for (auto s = vo->vd->segments.begin(); s != vo->vd->segments.end(); ++s) {
              pts[low(*s).x()].insert(low(*s).y());
              pts[high(*s).x()].insert(high(*s).y());
            }
          }
          auto ys = pts.find(int32_t(v0->x()));
          if (ys != pts.end() && ys->second.find(v0->y()) != ys->second.end()) {
            vo->colorExterior(&(*e), color);
          }
        }
        if (bail) {
          return nullptr;
        }
      }
    }
  }

  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* VoronoiPy::colorTwins(PyObject *args) {
  Voronoi::color_type color = 0;
  if (!PyArg_ParseTuple(args, "k", &color)) {
    throw  Py::RuntimeError("colorTwins requires an integer (color) argument");
  }
  getVoronoiPtr()->colorTwins(color);

  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* VoronoiPy::colorColinear(PyObject *args) {
  Voronoi::color_type color = 0;
  double degree = 10.;
  if (!PyArg_ParseTuple(args, "k|d", &color, &degree)) {
    throw  Py::RuntimeError("colorColinear requires an integer (color) and optionally a derivation in degrees argument (default 10)");
  }
  getVoronoiPtr()->colorColinear(color, degree);

  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* VoronoiPy::resetColor(PyObject *args) {
  Voronoi::color_type color = 0;
  if (!PyArg_ParseTuple(args, "k", &color)) {
    throw  Py::RuntimeError("clearColor requires an integer (color) argument");
  }

  getVoronoiPtr()->resetColor(color);

  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* VoronoiPy::getPoints(PyObject *args) {
  double z = 0;
  if (!PyArg_ParseTuple(args, "|d", &z)) {
    throw Py::RuntimeError("Optional z argument (double) accepted");
  }
  Voronoi *vo = getVoronoiPtr();
  Py::List list;
  for (auto it = vo->vd->points.begin(); it != vo->vd->points.end(); ++it) {
    list.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(vo->vd->scaledVector(*it, z)))));
  }
  return Py::new_reference_to(list);
}

PyObject* VoronoiPy::getSegments(PyObject *args) {
  double z = 0;
  if (!PyArg_ParseTuple(args, "|d", &z)) {
    throw Py::RuntimeError("Optional z argument (double) accepted");
  }
  Voronoi *vo = getVoronoiPtr();
  Py::List list;
  for (auto it = vo->vd->segments.begin(); it != vo->vd->segments.end(); ++it) {
    PyObject *p0 = new Base::VectorPy(new Base::Vector3d(vo->vd->scaledVector(low(*it), z)));
    PyObject *p1 = new Base::VectorPy(new Base::Vector3d(vo->vd->scaledVector(high(*it), z)));
    PyObject *tp = PyTuple_New(2);
    PyTuple_SetItem(tp, 0, p0);
    PyTuple_SetItem(tp, 1, p1);
    list.append(Py::asObject(tp));
  }
  return Py::new_reference_to(list);
}

PyObject* VoronoiPy::numPoints(PyObject *args)
{
  if (!PyArg_ParseTuple(args, "")) {
    throw  Py::RuntimeError("no arguments accepted");
  }
  return PyLong_FromLong(getVoronoiPtr()->vd->points.size());
}

PyObject* VoronoiPy::numSegments(PyObject *args)
{
  if (!PyArg_ParseTuple(args, "")) {
    throw  Py::RuntimeError("no arguments accepted");
  }
  return PyLong_FromLong(getVoronoiPtr()->vd->segments.size());
}


// custom attributes get/set

PyObject *VoronoiPy::getCustomAttributes(const char* /*attr*/) const
{
  return nullptr;
}

int VoronoiPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
  return 0;
}



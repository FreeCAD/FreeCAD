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
#include <Mod/Part/App/LineSegmentPy.h>
#include <Mod/Part/App/ArcOfParabolaPy.h>

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
    return Py::Int(e->ptr->color() & Voronoi::ColorMask);
  }
  return Py::Int(0);
}

void VoronoiEdgePy::setColor(Py::Int color) {
  getEdgeFromPy(this)->color(int(color) & Voronoi::ColorMask);
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

namespace {
  Voronoi::point_type retrievePoint(Voronoi::diagram_type *dia, const Voronoi::diagram_type::cell_type *cell) {
    Voronoi::diagram_type::cell_type::source_index_type index = cell->source_index();
    Voronoi::diagram_type::cell_type::source_category_type category = cell->source_category();
    if (category == boost::polygon::SOURCE_CATEGORY_SINGLE_POINT) {
      return dia->points[index];
    }
    index -= dia->points.size();
    if (category == boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT) {
      return low(dia->segments[index]);
    } else {
      return high(dia->segments[index]);
    }
  }

  Voronoi::segment_type retrieveSegment(Voronoi::diagram_type *dia, const Voronoi::diagram_type::cell_type *cell) {
    Voronoi::diagram_type::cell_type::source_index_type index = cell->source_index() - dia->points.size();
    return dia->segments[index];
  }

  Voronoi::point_type orthognalProjection(const Voronoi::point_type &point, const Voronoi::segment_type &segment) {
    // move segment so it goes through the origin (s)
    Voronoi::point_type offset;
    {
      offset.x(low(segment).x());
      offset.y(low(segment).y());
    }
    Voronoi::point_type s;
    {
      s.x(high(segment).x() - offset.x());
      s.y(high(segment).y() - offset.y());
    }
    // move point accordingly so it maintains it's relation to s (p)
    Voronoi::point_type p;
    {
      p.x(point.x() - offset.x());
      p.y(point.y() - offset.y());
    }
    // calculate the orthogonal projection of p onto s
    // ((p dot s) / (s dot s)) * s (https://en.wikibooks.org/wiki/Linear_Algebra/Orthogonal_Projection_Onto_a_Line)
    // and it back by original offset to get the projected point
    double proj = (p.x() * s.x() + p.y() * s.y()) / (s.x() * s.x() + s.y() * s.y());
    Voronoi::point_type pt;
    {
      pt.x(offset.x() + proj * s.x());
      pt.y(offset.y() + proj * s.y());
    }
    return pt;
  }
}

PyObject* VoronoiEdgePy::toGeom(PyObject *args)
{
  double z = 0.0;
  if (!PyArg_ParseTuple(args, "|d", &z)) {
    throw Py::RuntimeError("single argument of type double accepted");
  }
  VoronoiEdge *e = getVoronoiEdgePtr();
  if (e->isBound()) {
    if (e->ptr->is_linear()) {
      if (e->ptr->is_finite()) {
        auto v0 = e->ptr->vertex0();
        auto v1 = e->ptr->vertex1();
        if (v0 && v1) {
          auto p = new Part::GeomLineSegment;
          p->setPoints(Base::Vector3d(v0->x(), v0->y(), z), Base::Vector3d(v1->x(), v1->y(), z));
          return new Part::LineSegmentPy(p);
        }
      } else {
        // infinite linear, need to clip somehow
        const Voronoi::diagram_type::cell_type *c0 = e->ptr->cell();
        const Voronoi::diagram_type::cell_type *c1 = e->ptr->twin()->cell();
        Voronoi::point_type origin;
        Voronoi::point_type direction;
        if (c0->contains_point() && c1->contains_point()) {
          Voronoi::point_type p0 = retrievePoint(e->dia, c0);
          Voronoi::point_type p1 = retrievePoint(e->dia, c1);
          origin.x((p0.x() + p1.x()) / 2.);
          origin.y((p0.y() + p1.y()) / 2.);
          direction.x(p0.y() - p1.y());
          direction.y(p1.x() - p0.x());
        } else {
          origin = c0->contains_segment() ? retrievePoint(e->dia, c1) : retrievePoint(e->dia, c0);
          Voronoi::segment_type segment = c0->contains_segment() ? retrieveSegment(e->dia, c0) : retrieveSegment(e->dia, c1);
          Voronoi::coordinate_type dx = high(segment).x() - low(segment).x();
          Voronoi::coordinate_type dy = high(segment).y() - low(segment).y();
          if ((low(segment) == origin) ^ c0->contains_point()) {
            direction.x(dy);
            direction.y(-dx);
          } else {
            direction.x(-dy);
            direction.y(dx);
          }
        }
        double k = 10.0; // <-- need something smarter here
        Voronoi::point_type begin;
        Voronoi::point_type end;
        if (e->ptr->vertex0()) {
          begin.x(e->ptr->vertex0()->x());
          begin.y(e->ptr->vertex0()->y());
        } else {
          begin.x(origin.x() - direction.x() * k);
          begin.y(origin.y() - direction.y() * k);
        }
        if (e->ptr->vertex1()) {
          end.x(e->ptr->vertex1()->x());
          end.y(e->ptr->vertex1()->y());
        } else {
          end.x(origin.x() + direction.x() * k);
          end.y(origin.y() + direction.y() * k);
        }
        auto p = new Part::GeomLineSegment;
        p->setPoints(Base::Vector3d(begin.x(), begin.y(), z), Base::Vector3d(end.x(), end.y()));
        return new Part::LineSegmentPy(p);
      }
    } else {
      // parabolic curve, which is always formed by a point and an edge
      Voronoi::point_type   point   = e->ptr->cell()->contains_point() ? retrievePoint(e->dia, e->ptr->cell())  : retrievePoint(e->dia, e->ptr->twin()->cell());
      Voronoi::segment_type segment = e->ptr->cell()->contains_point() ? retrieveSegment(e->dia, e->ptr->twin()->cell()) : retrieveSegment(e->dia, e->ptr->cell());
      // the location is the mid point betwenn the normal on the segment through point
      // this is only the mid point of the segment if the parabola is symmetric
      Voronoi::point_type loc;
      {
        Voronoi::point_type proj = orthognalProjection(point, segment);
        // the location is the mid point between the projection on the segment and the point
        loc.x((proj.x() + point.x()) / 2);
        loc.y((proj.y() + point.y()) / 2);
      }
      Voronoi::point_type axis;
      {
        axis.x(point.x() - loc.x());
        axis.y(point.y() - loc.y());
      }
      auto p = new Part::GeomParabola;
      {
        p->setCenter(Base::Vector3d(point.x(), point.y(), z));
        p->setLocation(Base::Vector3d(loc.x(), loc.y(), z));
        p->setAngleXU(atan2(axis.y(), axis.x()));
        p->setFocal(sqrt(axis.x() * axis.x() + axis.y() * axis.y()));
      }
      auto a = new Part::GeomArcOfParabola;
      {
        a->setHandle(Handle(Geom_Parabola)::DownCast(p->handle()));

        // figure out the arc parameters
        auto v0 = e->ptr->vertex0();
        auto v1 = e->ptr->vertex1();
        double param0 = 0;
        double param1 = 0;
        if (!p->closestParameter(Base::Vector3d(v0->x(), v0->y(), z), param0)) {
          std::cerr << "closestParameter(v0) failed" << std::endl;
        }
        if (!p->closestParameter(Base::Vector3d(v1->x(), v1->y(), z), param1)) {
          std::cerr << "closestParameter(v0) failed" << std::endl;
        }
        a->setRange(param0, param1, false);
      }
      return new Part::ArcOfParabolaPy(a);
    }
  }
  Py_INCREF(Py_None);
  return Py_None;
}


namespace {

  double distanceBetween(const Voronoi::diagram_type::vertex_type &v0, const Voronoi::point_type &p1) {
    double x = v0.x() - p1.x();
    double y = v0.y() - p1.y();
    return sqrt(x * x + y * y);
  }

  void addDistanceBetween(const Voronoi::diagram_type::vertex_type *v0, const Voronoi::point_type &p1, Py::List *list) {
    if (v0) {
      list->append(Py::Float(distanceBetween(*v0, p1)));
    } else {
      Py_INCREF(Py_None);
      list->append(Py::asObject(Py_None));
    }
  }

  void addProjectedDistanceBetween(const Voronoi::diagram_type::vertex_type *v0, const Voronoi::segment_type &segment, Py::List *list) {
    if (v0) {
      Voronoi::point_type p0;
      {
        p0.x(v0->x());
        p0.y(v0->y());
      }
      Voronoi::point_type p1 = orthognalProjection(p0, segment);
      list->append(Py::Float(distanceBetween(*v0, p1)));
    } else {
      Py_INCREF(Py_None);
      list->append(Py::asObject(Py_None));
    }
  }

  bool addDistancesToPoint(const VoronoiEdge *edge, Voronoi::point_type p, Py::List *list) {
    addDistanceBetween(edge->ptr->vertex0(), p, list);
    addDistanceBetween(edge->ptr->vertex1(), p, list);
    return true;
  }

  bool retrieveDistances(const VoronoiEdge *edge, Py::List *list) {
    const Voronoi::diagram_type::cell_type *c0 = edge->ptr->cell();
    if (c0->contains_point()) {
      return addDistancesToPoint(edge, retrievePoint(edge->dia, c0), list);
    }
    const Voronoi::diagram_type::cell_type *c1 = edge->ptr->twin()->cell();
    if (c1->contains_point()) {
      return addDistancesToPoint(edge, retrievePoint(edge->dia, c1), list);
    }
    // at this point both cells are sourced from segments and it does not matter which one we use
    Voronoi::segment_type segment = retrieveSegment(edge->dia, c0);
    addProjectedDistanceBetween(edge->ptr->vertex0(), segment, list);
    addProjectedDistanceBetween(edge->ptr->vertex1(), segment, list);
    return false;
  }
}

PyObject* VoronoiEdgePy::getDistances(PyObject *args)
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this, args);
  Py::List list;
  retrieveDistances(e, &list);
  return Py::new_reference_to(list);
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


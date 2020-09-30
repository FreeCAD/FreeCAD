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
      ss << "[" << (v0->x() / e->dia->getScale()) << ", " << (v0->y() / e->dia->getScale()) << "]";
    } else {
      ss << "[~]";
    }
    ss << ", ";
    if (v1) {
      ss << "[" << (v1->x() / e->dia->getScale()) << ", " << (v1->y() / e->dia->getScale()) << "]";
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

Py::Long VoronoiEdgePy::getIndex(void) const {
  VoronoiEdge *e = getVoronoiEdgePtr();
  if (e->isBound()) {
    return Py::Long(e->dia->index(e->ptr));
  }
  return Py::Long(-1);
}

Py::Long VoronoiEdgePy::getColor(void) const {
  VoronoiEdge *e = getVoronoiEdgePtr();
  if (e->isBound()) {
    return Py::Long(e->ptr->color() & Voronoi::ColorMask);
  }
  return Py::Long(0);
}

void VoronoiEdgePy::setColor(Py::Long color) {
  getEdgeFromPy(this)->color(long(color) & Voronoi::ColorMask);
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

Py::Object VoronoiEdgePy::getRotNext(void) const
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(e->dia, e->ptr->rot_next())));
}

Py::Object VoronoiEdgePy::getRotPrev(void) const
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
          p->setPoints(e->dia->scaledVector(*v0, z), e->dia->scaledVector(*v1, z));
          return new Part::LineSegmentPy(p);
        }
      } else {
        // infinite linear, need to clip somehow
        const Voronoi::diagram_type::cell_type *c0 = e->ptr->cell();
        const Voronoi::diagram_type::cell_type *c1 = e->ptr->twin()->cell();
        Voronoi::point_type origin;
        Voronoi::point_type direction;
        if (c0->contains_point() && c1->contains_point()) {
          Voronoi::point_type p0 = e->dia->retrievePoint(c0);
          Voronoi::point_type p1 = e->dia->retrievePoint(c1);
          origin.x((p0.x() + p1.x()) / 2.);
          origin.y((p0.y() + p1.y()) / 2.);
          direction.x(p0.y() - p1.y());
          direction.y(p1.x() - p0.x());
        } else {
          origin = c0->contains_segment() ? e->dia->retrievePoint(c1) : e->dia->retrievePoint(c0);
          Voronoi::segment_type segment = c0->contains_segment() ? e->dia->retrieveSegment(c0) : e->dia->retrieveSegment(c1);
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
        p->setPoints(e->dia->scaledVector(begin, z), e->dia->scaledVector(end, z));
        return new Part::LineSegmentPy(p);
      }
    } else {
      // parabolic curve, which is always formed by a point and an edge
      Voronoi::point_type   point   = e->ptr->cell()->contains_point() ? e->dia->retrievePoint(e->ptr->cell())  : e->dia->retrievePoint(e->ptr->twin()->cell());
      Voronoi::segment_type segment = e->ptr->cell()->contains_point() ? e->dia->retrieveSegment(e->ptr->twin()->cell()) : e->dia->retrieveSegment(e->ptr->cell());
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
        p->setCenter(e->dia->scaledVector(point, z));
        p->setLocation(e->dia->scaledVector(loc, z));
        p->setAngleXU(atan2(axis.y(), axis.x()));
        p->setFocal(sqrt(axis.x() * axis.x() + axis.y() * axis.y()) / e->dia->getScale());
      }
      auto a = new Part::GeomArcOfParabola;
      {
        a->setHandle(Handle(Geom_Parabola)::DownCast(p->handle()));

        // figure out the arc parameters
        auto v0 = e->ptr->vertex0();
        auto v1 = e->ptr->vertex1();
        double param0 = 0;
        double param1 = 0;
        if (!p->closestParameter(e->dia->scaledVector(*v0, z), param0)) {
          std::cerr << "closestParameter(v0) failed" << std::endl;
        }
        if (!p->closestParameter(e->dia->scaledVector(*v1, z), param1)) {
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

  double distanceBetween(const Voronoi::diagram_type::vertex_type &v0, const Voronoi::point_type &p1, double scale) {
    double x = v0.x() - p1.x();
    double y = v0.y() - p1.y();
    return sqrt(x * x + y * y) / scale;
  }

  void addDistanceBetween(const Voronoi::diagram_type::vertex_type *v0, const Voronoi::point_type &p1, Py::List *list, double scale) {
    if (v0) {
      list->append(Py::Float(distanceBetween(*v0, p1, scale)));
    } else {
      Py_INCREF(Py_None);
      list->append(Py::asObject(Py_None));
    }
  }

  void addProjectedDistanceBetween(const Voronoi::diagram_type::vertex_type *v0, const Voronoi::segment_type &segment, Py::List *list, double scale) {
    if (v0) {
      Voronoi::point_type p0;
      {
        p0.x(v0->x());
        p0.y(v0->y());
      }
      Voronoi::point_type p1 = orthognalProjection(p0, segment);
      list->append(Py::Float(distanceBetween(*v0, p1, scale)));
    } else {
      Py_INCREF(Py_None);
      list->append(Py::asObject(Py_None));
    }
  }

  bool addDistancesToPoint(const VoronoiEdge *edge, Voronoi::point_type p, Py::List *list, double scale) {
    addDistanceBetween(edge->ptr->vertex0(), p, list, scale);
    addDistanceBetween(edge->ptr->vertex1(), p, list, scale);
    return true;
  }

  bool retrieveDistances(const VoronoiEdge *edge, Py::List *list) {
    const Voronoi::diagram_type::cell_type *c0 = edge->ptr->cell();
    if (c0->contains_point()) {
      return addDistancesToPoint(edge, edge->dia->retrievePoint(c0), list, edge->dia->getScale());
    }
    const Voronoi::diagram_type::cell_type *c1 = edge->ptr->twin()->cell();
    if (c1->contains_point()) {
      return addDistancesToPoint(edge, edge->dia->retrievePoint(c1), list, edge->dia->getScale());
    }
    // at this point both cells are sourced from segments and it does not matter which one we use
    Voronoi::segment_type segment = edge->dia->retrieveSegment(c0);
    addProjectedDistanceBetween(edge->ptr->vertex0(), segment, list, edge->dia->getScale());
    addProjectedDistanceBetween(edge->ptr->vertex1(), segment, list, edge->dia->getScale());
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

std::ostream& operator<<(std::ostream &str, const Voronoi::point_type &p) {
  return str << "[" << int(p.x()) << ", " << int(p.y()) << "]";
}

std::ostream& operator<<(std::ostream &str, const Voronoi::segment_type &s) {
  return str << '<' << low(s) << '-' << high(s) << '>';
}

static bool pointsMatch(const Voronoi::point_type &p0, const Voronoi::point_type &p1) {
  return long(p0.x()) == long(p1.x()) && long(p0.y()) == long(p1.y());
}

static void printCompare(const char *label, const Voronoi::point_type &p0, const Voronoi::point_type &p1) {
  std::cerr << "         " << label <<": " << pointsMatch(p1, p0) << pointsMatch(p0, p1) << "    " << p0 << ' ' << p1 << std::endl;
}

PyObject* VoronoiEdgePy::getSegmentAngle(PyObject *args)
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this, args);

  if (e->ptr->cell()->contains_segment() && e->ptr->twin()->cell()->contains_segment()) {
    int i0 = e->ptr->cell()->source_index() - e->dia->points.size();
    int i1 = e->ptr->twin()->cell()->source_index() - e->dia->points.size();
    if (e->dia->segmentsAreConnected(i0, i1)) {
      double a0 = e->dia->angleOfSegment(i0);
      double a1 = e->dia->angleOfSegment(i1);
      double a = a0 - a1;
      if (a > M_PI_2) {
        a -= M_PI;
      } else if (a < -M_PI_2) {
        a += M_PI;
      }
      return Py::new_reference_to(Py::Float(a));
    } else {
      std::cerr << "indices: " << std::endl;
      std::cerr << "   " << e->dia->segments[i0] << std::endl;
      std::cerr << "   " << e->dia->segments[i1] << std::endl;
      std::cerr << "   connected: " << e->dia->segmentsAreConnected(i0, i1) << std::endl;
      printCompare("l/l", low(e->dia->segments[i0]),  low(e->dia->segments[i1]));
      printCompare("l/h", low(e->dia->segments[i0]),  high(e->dia->segments[i1]));
      printCompare("h/l", high(e->dia->segments[i0]), low(e->dia->segments[i1]));
      printCompare("h/h", high(e->dia->segments[i0]), high(e->dia->segments[i1]));
    }
  } else {
    std::cerr << "constains_segment(" << e->ptr->cell()->contains_segment() << ", " << e->ptr->twin()->cell()->contains_segment() << ")" << std::endl;
  }
  Py_INCREF(Py_None);
  return Py_None;
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


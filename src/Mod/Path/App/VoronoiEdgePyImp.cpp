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
#ifndef _PreComp_
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <Geom_Parabola.hxx>
#endif

#include "Mod/Part/App/Geometry.h"
#include "Mod/Part/App/TopoShapeEdgePy.h"

#include "VoronoiEdgePy.h"
#include "VoronoiEdgePy.cpp"
#include "VoronoiCellPy.h"
#include "VoronoiVertexPy.h"


using namespace Path;

namespace {

  Voronoi::point_type pointFromVertex(const Voronoi::vertex_type v) {
    Voronoi::point_type pt;
    pt.x(v.x());
    pt.y(v.y());
    return pt;
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
    const double proj = (p.x() * s.x() + p.y() * s.y())
        / (s.x() * s.x() + s.y() * s.y() + std::numeric_limits<double>::epsilon());
    Voronoi::point_type pt;
    {
      pt.x(offset.x() + proj * s.x());
      pt.y(offset.y() + proj * s.y());
    }
    return pt;
  }

  double length(const Voronoi::point_type &p) {
    return sqrt(p.x() * p.x() + p.y() * p.y());
  }

  int sideOf(const Voronoi::point_type &p, const Voronoi::segment_type &s) {
    Voronoi::coordinate_type dxp = p.x()       - low(s).x();
    Voronoi::coordinate_type dyp = p.y()       - low(s).y();
    Voronoi::coordinate_type dxs = high(s).x() - low(s).x();
    Voronoi::coordinate_type dys = high(s).y() - low(s).y();

    double d = -dxs * dyp + dys * dxp;
    if (d < 0) {
      return -1;
    }
    if (d > 0) {
      return +1;
    }
    return 0;
  }

  template<typename pt0_type, typename pt1_type>
  double distanceBetween(const pt0_type &p0, const pt1_type &p1, double scale) {
    Voronoi::point_type dist;
    dist.x(p0.x() - p1.x());
    dist.y(p0.y() - p1.y());
    return length(dist) / scale;
  }

  template<typename pt0_type, typename pt1_type>
  double signedDistanceBetween(const pt0_type &p0, const pt1_type &p1, double scale) {
    if (length(p0) > length(p1)) {
      return -distanceBetween(p0, p1, scale);
    }
    return distanceBetween(p0, p1, scale);
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

  bool pointsMatch(const Voronoi::point_type &p0, const Voronoi::point_type &p1, double scale) {
    return 1e-6 > distanceBetween(p0, p1, scale);
  }

  bool isPointOnSegment(const Voronoi::point_type &point, const Voronoi::segment_type &segment, double scale) {
    return pointsMatch(point, low(segment), scale) || pointsMatch(point, high(segment), scale);
  }

  template<typename T>
  PyObject* makeLineSegment(const VoronoiEdge *e, const T &p0, double z0, const T &p1, double z1) {
    Part::GeomLineSegment p;
    p.setPoints(e->dia->scaledVector(p0, z0), e->dia->scaledVector(p1, z1));
    Handle(Geom_Curve) h = Handle(Geom_Curve)::DownCast(p.handle());
    BRepBuilderAPI_MakeEdge mkBuilder(h, h->FirstParameter(), h->LastParameter());
    return new Part::TopoShapeEdgePy(new Part::TopoShape(mkBuilder.Shape()));
  }
}

std::ostream& operator<<(std::ostream& os, const Voronoi::vertex_type &v) {
  return os << '(' << v.x() << ", " << v.y() << ')';
}

std::ostream& operator<<(std::ostream& os, const Voronoi::point_type &p) {
  return os << '(' << p.x() << ", " << p.y() << ')';
}

std::ostream& operator<<(std::ostream& os, const Voronoi::segment_type &s) {
  return os << '<' << low(s) << ", " << high(s) << '>';
}


// returns a string which represents the object e.g. when printed in python
std::string VoronoiEdgePy::representation() const
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
  PyObject *cmp = (op == Py_EQ) ? Py_False : Py_True;
  if (   PyObject_TypeCheck(lhs, &VoronoiEdgePy::Type)
      && PyObject_TypeCheck(rhs, &VoronoiEdgePy::Type)
      && (op == Py_EQ || op == Py_NE)) {
    const VoronoiEdge *vl = static_cast<VoronoiEdgePy*>(lhs)->getVoronoiEdgePtr();
    const VoronoiEdge *vr = static_cast<VoronoiEdgePy*>(rhs)->getVoronoiEdgePtr();
    if (vl->dia == vr->dia && vl->index == vr->index) {
      cmp = (op == Py_EQ) ? Py_True : Py_False;
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
  return nullptr;
}

VoronoiEdge* getVoronoiEdgeFromPy(const VoronoiEdgePy *e, PyObject *args = nullptr) {
  VoronoiEdge *self = e->getVoronoiEdgePtr();
  if (!self->isBound()) {
    throw Py::TypeError("Edge not bound to voronoi diagram");
  }
  if (args && !PyArg_ParseTuple(args, "")) {
    throw Py::RuntimeError("No arguments accepted");
  }
  return self;
}

Py::Long VoronoiEdgePy::getIndex() const {
  VoronoiEdge *e = getVoronoiEdgePtr();
  if (e->isBound()) {
    return Py::Long(e->dia->index(e->ptr));
  }
  return Py::Long(-1);
}

Py::Long VoronoiEdgePy::getColor() const {
  VoronoiEdge *e = getVoronoiEdgePtr();
  if (e->isBound()) {
    Voronoi::color_type color = e->ptr->color() & Voronoi::ColorMask;
    return Py::Long(PyLong_FromSize_t(color));
  }
  return Py::Long(0);
}

void VoronoiEdgePy::setColor(Py::Long color) {
  getEdgeFromPy(this)->color(long(color) & Voronoi::ColorMask);
}

Py::List VoronoiEdgePy::getVertices() const
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

Py::Object VoronoiEdgePy::getTwin() const
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(e->dia, e->ptr->twin())));
}

Py::Object VoronoiEdgePy::getNext() const
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(e->dia, e->ptr->next())));
}

Py::Object VoronoiEdgePy::getPrev() const
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(e->dia, e->ptr->prev())));
}

Py::Object VoronoiEdgePy::getRotNext() const
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(e->dia, e->ptr->rot_next())));
}

Py::Object VoronoiEdgePy::getRotPrev() const
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this);
  return Py::asObject(new VoronoiEdgePy(new VoronoiEdge(e->dia, e->ptr->rot_prev())));
}

Py::Object VoronoiEdgePy::getCell() const
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

PyObject* VoronoiEdgePy::isBorderline(PyObject *args)
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this, args);
  PyObject *chk = Py_False;
  if (e->isBound() && !e->ptr->is_linear()) {
    Voronoi::point_type   point   = e->ptr->cell()->contains_point() ? e->dia->retrievePoint(e->ptr->cell())  : e->dia->retrievePoint(e->ptr->twin()->cell());
    Voronoi::segment_type segment = e->ptr->cell()->contains_point() ? e->dia->retrieveSegment(e->ptr->twin()->cell()) : e->dia->retrieveSegment(e->ptr->cell());
    if (isPointOnSegment(point, segment, e->dia->getScale())) {
      chk = Py_True;
    }
  }
  Py_INCREF(chk);
  return chk;
}

PyObject* VoronoiEdgePy::toShape(PyObject *args)
{
  double z0 = 0.0;
  double z1 = DBL_MAX;
  int dbg   = 0;
  if (!PyArg_ParseTuple(args, "|ddp", &z0, &z1, &dbg)) {
    throw Py::RuntimeError("no, one or two arguments of type double accepted");
  }
  if (z1 == DBL_MAX) {
    z1 = z0;
  }
  VoronoiEdge *e = getVoronoiEdgePtr();
  if (e->isBound()) {
    if (e->ptr->is_linear()) {
      if (e->ptr->is_finite()) {
        auto v0 = e->ptr->vertex0();
        auto v1 = e->ptr->vertex1();
        if (v0 && v1) {
          return makeLineSegment(e, *v0, z0, *v1, z1);
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
        double k = 2.5; // <-- need something smarter here
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
        return makeLineSegment(e, begin, z0, end, z1);
      }
    } else {
      // parabolic curve, which is always formed by a point and an edge
      Voronoi::point_type   point   = e->ptr->cell()->contains_point() ? e->dia->retrievePoint(e->ptr->cell())  : e->dia->retrievePoint(e->ptr->twin()->cell());
      Voronoi::segment_type segment = e->ptr->cell()->contains_point() ? e->dia->retrieveSegment(e->ptr->twin()->cell()) : e->dia->retrieveSegment(e->ptr->cell());
      // the location is the mid point between the normal on the segment through point
      // this is only the mid point of the segment if the parabola is symmetric

      if (isPointOnSegment(point, segment, e->dia->getScale())) {
        return makeLineSegment(e, low(segment), z0, high(segment), z1);
      }

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
      Voronoi::segment_type xaxis;
      {
        xaxis.low(point);
        xaxis.high(loc);
      }

      // determine distances of the end points from the x-axis, those are the parameters for
      // the arc of the parabola in the horizontal plane
      auto pt0 = pointFromVertex(*e->ptr->vertex0());
      auto pt1 = pointFromVertex(*e->ptr->vertex1());
      Voronoi::point_type pt0x = orthognalProjection(pt0, xaxis);
      Voronoi::point_type pt1x = orthognalProjection(pt1, xaxis);
      double dist0 = distanceBetween(pt0, pt0x, e->dia->getScale()) * sideOf(pt0, xaxis);
      double dist1 = distanceBetween(pt1, pt1x, e->dia->getScale()) * sideOf(pt1, xaxis);
      if (dist1 < dist0) {
        // if the parabola is traversed in the revere direction we need to use the points
        // on the other side of the parabola - 'beauty of symmetric geometries
        dist0 = -dist0;
        dist1 = -dist1;
      }

      // at this point we have the direction of the x-axis and the two end points p0 and p1
      // which means we know the plane of the parabola
      auto p0 = e->dia->scaledVector(pt0, z0);
      auto p1 = e->dia->scaledVector(pt1, z1);
      // we get a third point by moving p0 along the axis of the parabola
      auto p_ = p0 + e->dia->scaledVector(axis, 0);

      // normal of the plane defined by those 3 points
      auto norm = ((p_ - p0).Cross(p1 - p0)).Normalize();

      // the next thing to figure out is the z level of the x-axis,
      double zx = z0 - (dist0 / (dist0 - dist1)) * (z0 - z1);

      auto locn = e->dia->scaledVector(loc, zx);
      auto xdir = e->dia->scaledVector(axis, zx);

      double focal;
      if (z0 == z1) {
        // focal length if parabola in the xy-plane is simply half the distance between the
        // point and segment - aka the distance between point and location, aka the length of axis
        focal = length(axis) / e->dia->getScale();
        if (dbg) {
          std::cerr << "focal = " << length(axis) << "/" << e->dia->getScale() << "\n";
        }
      } else {
        // if the parabola is not in the xy-plane we need to find the
        // (x,y) coordinates of a point on the parabola in the parabola's
        // coordinate system.
        // see: http://amsi.org.au/ESA_Senior_Years/SeniorTopic2/2a/2a_2content_10.html
        // note that above website uses Y as the symmetry axis of the parabola whereas
        // OCC uses X as the symmetry axis. The math below is in the website's system.
        // We already know 2 points on the parabola (p0 and p1), we know their X values
        // (dist0 and dist1) if the parabola is in the xy-plane, and we know their orthogonal
        // projection onto the parabola's symmetry axis Y (pt0x and pt1x). The resulting Y
        // values are the distance between the parabola's location (loc) and the respective
        // orthogonal projection. Pythagoras gives us the X values, using the X from the
        // xy-plane and the difference in z.
        // Note that this calculation also gives correct results if the parabola is in
        // the xy-plane (z0 == z1), it's just that above calculation is so much simpler.
        double flenX0 = sqrt(dist0 * dist0 + (z0 - zx) * (z0 - zx));
        double flenX1 = sqrt(dist1 * dist1 + (zx - z1) * (zx - z1));
        double flenX;
        double flenY;
        // if one of the points is the location, we have to use the other to get sensible values
        if (fabs(dist0) > fabs(dist1)) {
          flenX = flenX0;
          flenY = distanceBetween(loc, pt0x, e->dia->getScale());
        } else {
          flenX = flenX1;
          flenY = distanceBetween(loc, pt1x, e->dia->getScale());
        }
        // parabola: (x - p)^2 = 4*focal*(y - q)   |  (p,q) ... location of parabola
        focal = (flenX * flenX) / (4 * fabs(flenY));
        if (dbg) {
          std::cerr << "segment" << segment << ", point" << point << std::endl;
          std::cerr << "  loc" << loc << ", axis" << axis << std::endl;
          std::cerr << "  dist0(" << dist0 << " : " << flenX0 << ", dist1(" << dist1 << " : " << flenX1 << ")" << std::endl;
          std::cerr << "  z(" << z0 << ", " << zx << ", " << z1 << ")" << std::endl;
          std::cerr << "  focal = (" << flenX << " * " << flenX << ") / (4 * fabs(" << flenY << "))\n";
        }
        // use new X values to set the parameters
        dist0 = dist0 >= 0 ? flenX0 : -flenX0;
        dist1 = dist1 >= 0 ? flenX1 : -flenX1;
      }

      gp_Pnt  pbLocn(locn.x, locn.y, locn.z);
      gp_Dir  pbNorm(norm.x, norm.y, norm.z);
      gp_Dir  pbXdir(xdir.x, xdir.y, 0);

      gp_Ax2  pb(pbLocn, pbNorm, pbXdir);
      Handle(Geom_Parabola) parabola = new Geom_Parabola(pb, focal);

      Part::GeomArcOfParabola arc;
      arc.setHandle(parabola);
      arc.setRange(dist0, dist1, false);

      // get a shape for the parabola arc
      Handle(Geom_Curve) h = Handle(Geom_Curve)::DownCast(arc.handle());
      BRepBuilderAPI_MakeEdge mkBuilder(h, h->FirstParameter(), h->LastParameter());
      return new Part::TopoShapeEdgePy(new Part::TopoShape(mkBuilder.Shape()));
    }
  }
  Py_INCREF(Py_None);
  return Py_None;
}


PyObject* VoronoiEdgePy::getDistances(PyObject *args)
{
  VoronoiEdge *e = getVoronoiEdgeFromPy(this, args);
  Py::List list;
  retrieveDistances(e, &list);
  return Py::new_reference_to(list);
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
    }
  }
  Py_INCREF(Py_None);
  return Py_None;
}

// custom attributes get/set

PyObject* VoronoiEdgePy::getCustomAttributes(const char* /*attr*/) const
{
  return nullptr;
}

int VoronoiEdgePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
  return 0;
}


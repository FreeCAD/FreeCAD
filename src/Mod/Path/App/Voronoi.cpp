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
# include <Standard_math.hxx>
#endif

#include <Base/Vector3D.h>

#include "Voronoi.h"


using namespace Base;
using namespace Path;

TYPESYSTEM_SOURCE(Path::Voronoi , Base::BaseClass)

// Helpers

// Voronoi::diagram_type

Voronoi::diagram_type::diagram_type()
  :scale(1000)
{
}

double Voronoi::diagram_type::getScale() const {
  return scale;
}

void Voronoi::diagram_type::setScale(double s) {
  scale = s;
}

Base::Vector3d Voronoi::diagram_type::scaledVector(double x, double y, double z) const {
  return Base::Vector3d(x / scale, y / scale, z);
}

Base::Vector3d Voronoi::diagram_type::scaledVector(const point_type &p, double z) const {
  return scaledVector(p.x(), p.y(), z);
}

Base::Vector3d Voronoi::diagram_type::scaledVector(const vertex_type &v, double z) const {
  return scaledVector(v.x(), v.y(), z);
}


int Voronoi::diagram_type::index(const Voronoi::diagram_type::cell_type   *cell)   const {
  auto it = cell_index.find(intptr_t(cell));
  if (it == cell_index.end()) {
    return Voronoi::InvalidIndex;
  }
  return it->second;
}
int Voronoi::diagram_type::index(const Voronoi::diagram_type::edge_type   *edge)   const {
  auto it = edge_index.find(intptr_t(edge));
  if (it == edge_index.end()) {
    return Voronoi::InvalidIndex;
  }
  return it->second;
}
int Voronoi::diagram_type::index(const Voronoi::diagram_type::vertex_type *vertex) const {
  auto it = vertex_index.find(intptr_t(vertex));
  if (it == vertex_index.end()) {
    return Voronoi::InvalidIndex;
  }
  return it->second;
}

void Voronoi::diagram_type::reIndex() {
  int idx = 0;
  cell_index.clear();
  edge_index.clear();
  vertex_index.clear();

  idx = 0;
  for (auto it = cells().begin(); it != cells().end(); ++it, ++idx) {
    cell_index[intptr_t(&(*it))] = idx;
  }
  idx = 0;
  for (auto it = edges().begin(); it != edges().end(); ++it, ++idx) {
    edge_index[intptr_t(&(*it))] = idx;
  }
  idx = 0;
  for (auto it = vertices().begin(); it != vertices().end(); ++it, ++idx) {
    vertex_index[intptr_t(&(*it))] = idx;
  }
}

Voronoi::point_type Voronoi::diagram_type::retrievePoint(const Voronoi::diagram_type::cell_type *cell) const {
  Voronoi::diagram_type::cell_type::source_index_type index = cell->source_index();
  Voronoi::diagram_type::cell_type::source_category_type category = cell->source_category();
  if (category == boost::polygon::SOURCE_CATEGORY_SINGLE_POINT) {
    return points[index];
  }
  index -= points.size();
  if (category == boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT) {
    return low(segments[index]);
  } else {
    return high(segments[index]);
  }
}

Voronoi::segment_type Voronoi::diagram_type::retrieveSegment(const Voronoi::diagram_type::cell_type *cell) const {
  Voronoi::diagram_type::cell_type::source_index_type index = cell->source_index() - points.size();
  return segments[index];
}


// Voronoi

Voronoi::Voronoi()
  :vd(new diagram_type)
{
}

Voronoi::~Voronoi()
{
}


void Voronoi::addPoint(const Voronoi::point_type &p) {
  Voronoi::point_type pi;
  pi.x(p.x() * vd->getScale());
  pi.y(p.y() * vd->getScale());
  vd->points.push_back(pi);
}

void Voronoi::addSegment(const Voronoi::segment_type &s) {
  Voronoi::point_type pil, pih;
  pil.x(low(s).x() * vd->getScale());
  pil.y(low(s).y() * vd->getScale());
  pih.x(high(s).x() * vd->getScale());
  pih.y(high(s).y() * vd->getScale());
  vd->segments.emplace_back(pil, pih);
}

long Voronoi::numPoints() const {
  return vd->points.size();
}

long Voronoi::numSegments() const {
  return vd->segments.size();
}


long Voronoi::numCells() const {
  return vd->num_cells();
}

long Voronoi::numEdges() const {
  return vd->num_edges();
}

long Voronoi::numVertices() const {
  return vd->num_vertices();
}

void Voronoi::construct()
{
  vd->clear();
  construct_voronoi(vd->points.begin(), vd->points.end(), vd->segments.begin(), vd->segments.end(), static_cast<voronoi_diagram_type*>(vd));
  vd->reIndex();
}

void Voronoi::colorExterior(const Voronoi::diagram_type::edge_type *edge, std::size_t colorValue) {
  if (edge->color()) {
    // end recursion
    return;
  }
  edge->color(colorValue);
  edge->twin()->color(colorValue);
  auto v = edge->vertex1();
  if (!v || !edge->is_primary()) {
    return;
  }
  v->color(colorValue);
  auto e = v->incident_edge();
  do {
    colorExterior(e, colorValue);
    e = e->rot_next();
  } while (e != v->incident_edge());
}

void Voronoi::colorExterior(Voronoi::color_type color) {
  for (diagram_type::const_edge_iterator it = vd->edges().begin(); it != vd->edges().end(); ++it) {
    if (it->is_infinite()) {
      colorExterior(&(*it), color);
    }
  }
}

void Voronoi::colorTwins(Voronoi::color_type color) {
  for (diagram_type::const_edge_iterator it = vd->edges().begin(); it != vd->edges().end(); ++it) {
    if (!it->color()) {
      auto twin = it->twin();
      if (!twin->color()) {
        twin->color(color);
      }
    }
  }
}

double Voronoi::diagram_type::angleOfSegment(int i, Voronoi::diagram_type::angle_map_t *angle) const {
  Voronoi::diagram_type::angle_map_t::const_iterator a = angle ? angle->find(i) : Voronoi::diagram_type::angle_map_t::const_iterator();
  if (!angle || a == angle->end()) {
    Voronoi::point_type p0 = low(segments[i]);
    Voronoi::point_type p1 = high(segments[i]);
    double ang = 0;
    if (p0.x() == p1.x()) {
      if (p0.y() < p1.y()) {
        ang = M_PI_2;
      } else {
        ang = -M_PI_2;
      }
    } else {
      ang = atan((p0.y() - p1.y()) / (p0.x() - p1.x()));
    }
    if (angle) {
      angle->insert(angle_map_t::value_type(i, ang));
    }
    return ang;
  }
  return a->second;
}

static bool pointsMatch(const Voronoi::point_type &p0, const Voronoi::point_type &p1) {
  return long(p0.x()) == long(p1.x()) && long(p0.y()) == long(p1.y());
}

bool Voronoi::diagram_type::segmentsAreConnected(int i, int j) const {
  return
       pointsMatch(low(segments[i]), low(segments[j]))
    || pointsMatch(low(segments[i]), high(segments[j]))
    || pointsMatch(high(segments[i]), low(segments[j]))
    || pointsMatch(high(segments[i]), high(segments[j]));
}

void Voronoi::colorColinear(Voronoi::color_type color, double degree) {
  double rad = degree * M_PI / 180;

  Voronoi::diagram_type::angle_map_t angle;
  int psize = vd->points.size();

  for (diagram_type::const_edge_iterator it = vd->edges().begin(); it != vd->edges().end(); ++it) {
    int i0 = it->cell()->source_index() - psize;
    int i1 = it->twin()->cell()->source_index() - psize;
    if (it->color() == 0
        && it->cell()->contains_segment()
        && it->twin()->cell()->contains_segment()
        && vd->segmentsAreConnected(i0, i1)) {
      double a0 = vd->angleOfSegment(i0, &angle);
      double a1 = vd->angleOfSegment(i1, &angle);
      double a = a0 - a1;
      if (a > M_PI_2) {
        a -= M_PI;
      } else if (a < -M_PI_2) {
        a += M_PI;
      }
      if (fabs(a) < rad) {
        it->color(color);
        it->twin()->color(color);
      }
    }
  }
}

void Voronoi::resetColor(Voronoi::color_type color) {
  for (auto it = vd->cells().begin(); it != vd->cells().end(); ++it) {
    if (color == 0 || it->color() == color) {
      it->color(0);
    }
  }
  for (auto it = vd->edges().begin(); it != vd->edges().end(); ++it) {
    if (it->color() == color) {
      it->color(0);
    }
  }
  for (auto it = vd->vertices().begin(); it != vd->vertices().end(); ++it) {
    if (it->color() == color) {
      it->color(0);
    }
  }
}

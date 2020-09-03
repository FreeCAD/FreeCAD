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
# include <cinttypes>
# include <iomanip>
# include <boost/algorithm/string.hpp>
# include <boost/lexical_cast.hpp>
#endif

#include <Base/Vector3D.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include "Voronoi.h"

using namespace Base;
using namespace Path;

TYPESYSTEM_SOURCE(Path::Voronoi , Base::BaseClass);

// Helpers

#if 0
static const std::size_t EXTERNAL_COLOR = 1;

static void color_exterior(const Voronoi::diagram_type::edge_type *edge) {
  if (edge->color() == EXTERNAL_COLOR) {
    // end recursion
    return;
  }
  edge->color(EXTERNAL_COLOR);
  edge->twin()->color(EXTERNAL_COLOR);
  auto v = edge->vertex1();
  if (v == NULL || !edge->is_primary()) {
    return;
  }
  v->color(EXTERNAL_COLOR);
  auto e = v->incident_edge();
  do {
    color_exterior(e);
    e = e->rot_next();
  } while (e != v->incident_edge());
}
#endif

// Constructors & destructors

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

Voronoi::Voronoi()
  :vd(new diagram_type)
{
}

Voronoi::~Voronoi()
{
}


void Voronoi::addPoint(const Voronoi::point_type &p) {
  points.push_back(p);
}

void Voronoi::addSegment(const Voronoi::segment_type &s) {
  segments.push_back(s);
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
  construct_voronoi(points.begin(), points.end(), segments.begin(), segments.end(), (voronoi_diagram_type*)vd);
  vd->reIndex();
}

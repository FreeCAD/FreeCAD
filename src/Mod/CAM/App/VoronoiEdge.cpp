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

#include "VoronoiEdge.h"


using namespace Base;
using namespace Path;

TYPESYSTEM_SOURCE(Path::VoronoiEdge , Base::Persistence)

VoronoiEdge::VoronoiEdge(Voronoi::diagram_type *d, long index)
  : dia(d)
  , index(index)
  , ptr(nullptr)
{
  if (dia && long(dia->num_edges()) > index) {
    ptr = &(dia->edges()[index]);
  }
}

VoronoiEdge::VoronoiEdge(Voronoi::diagram_type *d, const Voronoi::diagram_type::edge_type *e)
  : dia(d)
  , index(Voronoi::InvalidIndex)
  , ptr(e)
{
  if (d && e) {
    index = dia->index(e);
  }
}

VoronoiEdge::~VoronoiEdge() {
}

bool VoronoiEdge::isBound() const {
  if (ptr && dia.isValid() && index != Voronoi::InvalidIndex) {
    if (&(dia->edges()[index]) == ptr) {
      return true;
    }
  }
  ptr = nullptr;
  return false;
}

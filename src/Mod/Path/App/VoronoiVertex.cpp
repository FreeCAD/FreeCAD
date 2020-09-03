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
#include "VoronoiVertex.h"

using namespace Base;
using namespace Path;

TYPESYSTEM_SOURCE(Path::VoronoiVertex , Base::Persistence)

VoronoiVertex::VoronoiVertex(Voronoi::diagram_type *d, long index)
  : dia(d)
  , index(index)
  , ptr(0)
{
  if (dia && long(dia->num_vertices()) > index) {
    ptr = &(dia->vertices()[index]);
  }
}

VoronoiVertex::VoronoiVertex(Voronoi::diagram_type *d, const Voronoi::diagram_type::vertex_type *v)
  : dia(d)
  , index(Voronoi::InvalidIndex)
  , ptr(v)
{
  if (dia && v) {
    index = dia->index(v);
  }
}

VoronoiVertex::~VoronoiVertex() {
}

bool VoronoiVertex::isBound(void) const {
  if (ptr != 0 && dia.isValid() && index != Voronoi::InvalidIndex) {
    if (&(dia->vertices()[index]) == ptr) {
      return true;
    }
  }
  ptr = 0;
  return false;
}

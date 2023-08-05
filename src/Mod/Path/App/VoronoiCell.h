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
#ifndef PATH_VORONOICELL_H
#define PATH_VORONOICELL_H

#include <Base/BaseClass.h>
#include "Voronoi.h"

namespace Path
{

class Voronoi;

class PathExport VoronoiCell
  : public Base::BaseClass
{
  TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:

  VoronoiCell(Voronoi::diagram_type *dia = nullptr, long index = Voronoi::InvalidIndex);
  VoronoiCell(Voronoi::diagram_type *dia, const Voronoi::diagram_type::cell_type *cell);
  ~VoronoiCell() override;

  bool isBound() const;

  Voronoi::point_type   sourcePoint()  const;
  Voronoi::segment_type sourceSegment() const;

  Base::Reference<Voronoi::diagram_type> dia;
  long index;
  mutable const Voronoi::diagram_type::cell_type *ptr;
};

}
#endif

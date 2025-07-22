/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef MESH_PROJECTION_H
#define MESH_PROJECTION_H

#include <Base/BoundBox.h>
#include <vector>

#include "Elements.h"


using Base::Vector3f;

#ifdef FC_USE_OCC
class TopoDS_Edge;
class TopoDS_Shape;
#endif

namespace MeshCore
{

class MeshFacetGrid;
class MeshKernel;
class MeshGeomFacet;

class MeshExport MeshProjection
{
public:
    explicit MeshProjection(const MeshKernel&);

    bool projectLineOnMesh(const MeshFacetGrid& grid,
                           const Base::Vector3f& p1,
                           FacetIndex f1,
                           const Base::Vector3f& p2,
                           FacetIndex f2,
                           const Base::Vector3f& view,
                           std::vector<Base::Vector3f>& polyline);

protected:
    bool bboxInsideRectangle(const Base::BoundBox3f& bbox,
                             const Base::Vector3f& p1,
                             const Base::Vector3f& p2,
                             const Base::Vector3f& view) const;
    bool isPointInsideDistance(const Base::Vector3f& p1,
                               const Base::Vector3f& p2,
                               const Base::Vector3f& pt) const;
    bool connectLines(std::list<std::pair<Base::Vector3f, Base::Vector3f>>& cutLines,
                      const Base::Vector3f& startPoint,
                      const Base::Vector3f& endPoint,
                      std::vector<Base::Vector3f>& polyline) const;

private:
    const MeshKernel& kernel;
};

}  // namespace MeshCore

#endif  // MESH_PROJECTION_H

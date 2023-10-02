/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESH_KDTREE_H
#define MESH_KDTREE_H

#include "Elements.h"

namespace MeshCore
{

class MeshExport MeshKDTree
{
public:
    MeshKDTree();
    explicit MeshKDTree(const std::vector<Base::Vector3f>& points);
    explicit MeshKDTree(const MeshPointArray& points);
    ~MeshKDTree();

    void AddPoint(const Base::Vector3f& point);
    void AddPoints(const std::vector<Base::Vector3f>& points);
    void AddPoints(const MeshPointArray& points);

    bool IsEmpty() const;
    void Clear();
    void Optimize();

    PointIndex FindNearest(const Base::Vector3f& p, Base::Vector3f& n, float&) const;
    PointIndex
    FindNearest(const Base::Vector3f& p, float max_dist, Base::Vector3f& n, float&) const;
    PointIndex FindExact(const Base::Vector3f& p) const;
    void FindInRange(const Base::Vector3f&, float, std::vector<PointIndex>&) const;

    MeshKDTree(const MeshKDTree&) = delete;
    MeshKDTree(MeshKDTree&&) = delete;
    void operator=(const MeshKDTree&) = delete;
    void operator=(MeshKDTree&&) = delete;

private:
    class Private;
    Private* d;
};

}  // namespace MeshCore


#endif  // MESH_KDTREE_H

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

#include "PreCompiled.h"
#ifdef _MSC_VER
#pragma warning(disable : 4396)
#endif

#include "KDTree.h"
#include <kdtree++/kdtree.hpp>


using namespace MeshCore;

struct Point3d
{
    using value_type = float;

    Point3d(const Base::Vector3f& f, PointIndex i)
        : p(f)
        , i(i)
    {}

    Point3d(const Point3d& pnt) = default;

    Point3d(Point3d&& pnt)
        : p(pnt.p)
        , i(pnt.i)
    {}

    ~Point3d() = default;

    inline value_type operator[](const int N) const
    {
        return p[N];
    }

    inline bool operator==(const Point3d& other) const
    {
        return (this->p) == (other.p);
    }

    inline bool operator!=(const Point3d& other) const
    {
        return (this->p) != (other.p);
    }

    inline Point3d& operator=(const Point3d& other) = default;

    inline Point3d& operator=(Point3d&& other)
    {
        this->p = other.p;
        this->i = other.i;
        return *this;
    }

    Base::Vector3f p;
    PointIndex i;
};

using MyKDTree = KDTree::KDTree<3, Point3d>;

class MeshKDTree::Private
{
public:
    MyKDTree kd_tree;
};

MeshKDTree::MeshKDTree()
    : d(new Private)
{}

MeshKDTree::MeshKDTree(const std::vector<Base::Vector3f>& points)
    : d(new Private)
{
    PointIndex index = 0;
    for (auto it : points) {
        d->kd_tree.insert(Point3d(it, index++));
    }
}

MeshKDTree::MeshKDTree(const MeshPointArray& points)
    : d(new Private)
{
    PointIndex index = 0;
    for (const auto& it : points) {
        d->kd_tree.insert(Point3d(it, index++));
    }
}

MeshKDTree::~MeshKDTree()
{
    delete d;
}

void MeshKDTree::AddPoint(const Base::Vector3f& point)
{
    PointIndex index = d->kd_tree.size();
    d->kd_tree.insert(Point3d(point, index));
}

void MeshKDTree::AddPoints(const std::vector<Base::Vector3f>& points)
{
    PointIndex index = d->kd_tree.size();
    for (auto it : points) {
        d->kd_tree.insert(Point3d(it, index++));
    }
}

void MeshKDTree::AddPoints(const MeshPointArray& points)
{
    PointIndex index = d->kd_tree.size();
    for (const auto& it : points) {
        d->kd_tree.insert(Point3d(it, index++));
    }
}

bool MeshKDTree::IsEmpty() const
{
    return d->kd_tree.empty();
}

void MeshKDTree::Clear()
{
    d->kd_tree.clear();
}

void MeshKDTree::Optimize()
{
    d->kd_tree.optimize();
}

PointIndex MeshKDTree::FindNearest(const Base::Vector3f& p, Base::Vector3f& n, float& dist) const
{
    std::pair<MyKDTree::const_iterator, MyKDTree::distance_type> it =
        d->kd_tree.find_nearest(Point3d(p, 0));
    if (it.first == d->kd_tree.end()) {
        return POINT_INDEX_MAX;
    }
    PointIndex index = it.first->i;
    n = it.first->p;
    dist = it.second;
    return index;
}

PointIndex MeshKDTree::FindNearest(const Base::Vector3f& p,
                                   float max_dist,
                                   Base::Vector3f& n,
                                   float& dist) const
{
    std::pair<MyKDTree::const_iterator, MyKDTree::distance_type> it =
        d->kd_tree.find_nearest(Point3d(p, 0), max_dist);
    if (it.first == d->kd_tree.end()) {
        return POINT_INDEX_MAX;
    }
    PointIndex index = it.first->i;
    n = it.first->p;
    dist = it.second;
    return index;
}

PointIndex MeshKDTree::FindExact(const Base::Vector3f& p) const
{
    MyKDTree::const_iterator it = d->kd_tree.find_exact(Point3d(p, 0));
    if (it == d->kd_tree.end()) {
        return POINT_INDEX_MAX;
    }
    PointIndex index = it->i;
    return index;
}

void MeshKDTree::FindInRange(const Base::Vector3f& p,
                             float range,
                             std::vector<PointIndex>& indices) const
{
    std::vector<Point3d> v;
    d->kd_tree.find_within_range(Point3d(p, 0), range, std::back_inserter(v));
    indices.reserve(v.size());
    for (const auto& it : v) {
        indices.push_back(it.i);
    }
}

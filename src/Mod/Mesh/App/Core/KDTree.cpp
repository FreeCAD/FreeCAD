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
# pragma warning(disable : 4396)
#endif
#ifndef _PreComp_
#endif

#include "KDTree.h"
#include <kdtree++/kdtree.hpp>

using namespace MeshCore;

struct Point3d 
{
   typedef float value_type;

   Point3d(const Base::Vector3f& f, PointIndex i) : p(f), i(i)
   {
   }

   Point3d(const Point3d& pnt) : p(pnt.p), i(pnt.i)
   {
   }

   Point3d(Point3d&& pnt) : p(pnt.p), i(pnt.i)
   {
   }

   ~Point3d()
   {
   }

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

   inline void operator=(const Point3d& other)
   {
       this->p = other.p;
       this->i = other.i;
   }

   inline void operator=(Point3d&& other)
   {
       this->p = other.p;
       this->i = other.i;
   }

   Base::Vector3f p;
   PointIndex i;
};

typedef KDTree::KDTree<3, Point3d> MyKDTree;

class MeshKDTree::Private
{
public:
    MyKDTree kd_tree;
};

MeshKDTree::MeshKDTree() : d(new Private)
{
}

MeshKDTree::MeshKDTree(const std::vector<Base::Vector3f>& points) : d(new Private)
{
    PointIndex index=0;
    for (std::vector<Base::Vector3f>::const_iterator it = points.begin(); it != points.end(); ++it) {
        d->kd_tree.insert(Point3d(*it, index++));
    }
}

MeshKDTree::MeshKDTree(const MeshPointArray& points) : d(new Private)
{
    PointIndex index=0;
    for (MeshPointArray::_TConstIterator it = points.begin(); it != points.end(); ++it) {
        d->kd_tree.insert(Point3d(*it, index++));
    }
}

MeshKDTree::~MeshKDTree()
{
    delete d;
}

void MeshKDTree::AddPoint(Base::Vector3f& point)
{
    PointIndex index=d->kd_tree.size();
    d->kd_tree.insert(Point3d(point, index));
}

void MeshKDTree::AddPoints(const std::vector<Base::Vector3f>& points)
{
    PointIndex index=d->kd_tree.size();
    for (std::vector<Base::Vector3f>::const_iterator it = points.begin(); it != points.end(); ++it) {
        d->kd_tree.insert(Point3d(*it, index++));
    }
}

void MeshKDTree::AddPoints(const MeshPointArray& points)
{
    PointIndex index=d->kd_tree.size();
    for (MeshPointArray::_TConstIterator it = points.begin(); it != points.end(); ++it) {
        d->kd_tree.insert(Point3d(*it, index++));
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
        d->kd_tree.find_nearest(Point3d(p,0));
    if (it.first == d->kd_tree.end())
        return POINT_INDEX_MAX;
    PointIndex index = it.first->i;
    n = it.first->p;
    dist = it.second;
    return index;
}

PointIndex MeshKDTree::FindNearest(const Base::Vector3f& p, float max_dist,
                                   Base::Vector3f& n, float& dist) const
{
    std::pair<MyKDTree::const_iterator, MyKDTree::distance_type> it =
        d->kd_tree.find_nearest(Point3d(p,0), max_dist);
    if (it.first == d->kd_tree.end())
        return POINT_INDEX_MAX;
    PointIndex index = it.first->i;
    n = it.first->p;
    dist = it.second;
    return index;
}

PointIndex MeshKDTree::FindExact(const Base::Vector3f& p) const
{
    MyKDTree::const_iterator it = 
        d->kd_tree.find_exact(Point3d(p,0));
    if (it == d->kd_tree.end())
        return POINT_INDEX_MAX;
    PointIndex index = it->i;
    return index;
}

void MeshKDTree::FindInRange(const Base::Vector3f& p, float range, std::vector<PointIndex>& indices) const
{
    std::vector<Point3d> v;
    d->kd_tree.find_within_range(Point3d(p,0), range, std::back_inserter(v));
    indices.reserve(v.size());
    for (std::vector<Point3d>::iterator it = v.begin(); it != v.end(); ++it)
        indices.push_back(it->i);
}

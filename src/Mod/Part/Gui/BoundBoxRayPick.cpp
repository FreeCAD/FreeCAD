/****************************************************************************
 *   Copyright (c) 2021 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <boost/algorithm/string/predicate.hpp>

struct Ray;
class MySbBox3f;

namespace boost { namespace geometry {

// forward declaration before including boost geometry headers, so that this
// template overloads can be found first.

template <class Geometry>
inline bool intersects(Geometry const& box, Ray const &ray);

template <>
inline bool intersects<MySbBox3f>(MySbBox3f const& box, Ray const &ray);

}}

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/box.hpp>

#include "BoundBoxRayPick.h"

using namespace PartGui;

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

class MySbVec3f : public SbVec3f {
public:
    float x() const {
        return (*this)[0];
    }
    float y() const {
        return (*this)[1];
    }
    float z() const {
        return (*this)[2];
    }
    void setX(float x) {
        (*this)[0] = x;
    }
    void setY(float x) {
        (*this)[1] = x;
    }
    void setZ(float x) {
        (*this)[2] = x;
    }
};

class MySbBox3f: public SbBox3f {
public:
    MySbVec3f &minCorner() {
        return static_cast<MySbVec3f&>(getMin());
    }
    const MySbVec3f &minCorner() const{
        return static_cast<const MySbVec3f&>(getMin());
    }
    MySbVec3f &maxCorner() {
        return static_cast<MySbVec3f&>(getMax());
    }
    const MySbVec3f &maxCorner() const{
        return static_cast<const MySbVec3f&>(getMax());
    }
};

struct Ray {
    SoRayPickAction *action;
    Ray(SoRayPickAction *a) :action(a) {}
};

BOOST_GEOMETRY_REGISTER_POINT_3D_GET_SET(
        MySbVec3f,float,bg::cs::cartesian,x,y,z,setX,setY,setZ)

BOOST_GEOMETRY_REGISTER_BOX(MySbBox3f,MySbVec3f,minCorner(),maxCorner())

namespace boost { namespace geometry {

template <class Geometry>
inline bool intersects(Geometry const& box, Ray const &ray) {
    SbBox3f bbox(bg::get<bg::min_corner,0>(box),
                 bg::get<bg::min_corner,1>(box),
                 bg::get<bg::min_corner,2>(box),
                 bg::get<bg::max_corner,0>(box),
                 bg::get<bg::max_corner,1>(box),
                 bg::get<bg::max_corner,2>(box));
    return !bbox.isEmpty() && ray.action->intersect(bbox);
}

template <>
inline bool intersects<MySbBox3f>(MySbBox3f const& box, Ray const &ray) {
    return !box.isEmpty() && ray.action->intersect(box);
}

}}

////////////////////////////////////////////////////////////////////////////

class BoundBoxRayPick::Private {
public:
    typedef bgi::linear<16> Parameters;
    
    struct BoxGetter {
        typedef const MySbBox3f& result_type;
        
        const std::vector<SbBox3f> &boxes;
        
        explicit BoxGetter(const std::vector<SbBox3f> &b)
            :boxes(b)
        {}

        result_type operator()(int idx) const {
            return static_cast<result_type>(boxes[idx]);
        }
    };
    
    std::vector<SbBox3f> boxes;
    bgi::rtree<int,Parameters,BoxGetter> rtree;

    Private()
        :rtree(Parameters(),BoxGetter(this->boxes))
    {
    }

    void init();
};


BoundBoxRayPick::BoundBoxRayPick()
    :pimpl(new Private)
{
}

BoundBoxRayPick::BoundBoxRayPick(BoundBoxRayPick &&other)
    :pimpl(std::move(other.pimpl))
{
}

BoundBoxRayPick::~BoundBoxRayPick()
{
}

void BoundBoxRayPick::init(std::vector<SbBox3f> &&boxes, bool delay)
{
    if (!pimpl) return;
    clear();
    pimpl->boxes = std::move(boxes);
    if (!delay)
        pimpl->init();
}

void BoundBoxRayPick::init(const std::vector<SbBox3f> &boxes, bool delay)
{
    if (!pimpl) return;
    clear();
    pimpl->boxes = boxes;
    if (!delay)
        pimpl->init();
}

void BoundBoxRayPick::clear()
{
    if (pimpl)
        pimpl->rtree.clear();
}

bool BoundBoxRayPick::empty() const
{
    return !pimpl || pimpl->boxes.empty();
}

void BoundBoxRayPick::Private::init()
{
    if (!rtree.empty())
        return;
    for(int i=0,count=(int)boxes.size();i<count;++i) {
        if(!boxes[i].isEmpty())
            rtree.insert(i);
    }
}

const std::vector<SbBox3f> &BoundBoxRayPick::getBoundBoxes() const
{
    if (pimpl)
        return pimpl->boxes;
    static std::vector<SbBox3f> dummy;
    return dummy;
}

std::size_t BoundBoxRayPick::size() const
{
    return pimpl ? pimpl->boxes.size() : 0;
}

void BoundBoxRayPick::rayPick(SoRayPickAction *action, std::vector<int> &results) {
    if (pimpl) {
        pimpl->init();
        pimpl->rtree.query(bgi::intersects(Ray(action)),std::back_inserter(results));
    }
}

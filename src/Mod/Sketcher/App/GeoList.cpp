/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#endif  // #ifndef _PreComp_

#include <assert.h>

#include <Base/Vector3D.h>

#include <Mod/Sketcher/App/GeometryFacade.h>

#include <Mod/Sketcher/App/Constraint.h>

#include "GeoList.h"

using namespace Sketcher;

// Vector is moved
template <typename T>
GeoListModel<T>::GeoListModel(  std::vector<T> && geometrylist,
                                int intgeocount,
                                bool ownerT): geomlist(std::move(geometrylist)),
                                              intGeoCount(intgeocount),
                                              OwnerT(ownerT)
{

}

// Vector is shallow copied (copy constructed)
template <typename T>
GeoListModel<T>::GeoListModel(  const std::vector<T> & geometrylist,
                                int intgeocount,
                                bool ownerT): geomlist(geometrylist), // copy constructed here
                                              intGeoCount(intgeocount),
                                              OwnerT(ownerT)
{

}

template <typename T>
GeoListModel<T>::~GeoListModel()
{
    if(OwnerT) {
        for(auto & g : geomlist)
            delete g;
    }
}

template <typename T>
GeoListModel<T> GeoListModel<T>::getGeoListModel(std::vector<T> && geometrylist, int intgeocount, bool ownerT)
{
    return GeoListModel(std::move(geometrylist), intgeocount, ownerT);
}

template <typename T>
const GeoListModel<T> GeoListModel<T>::getGeoListModel(const std::vector<T> &geometrylist, int intgeocount, bool ownerT)
{
    return GeoListModel(geometrylist, intgeocount, ownerT);
}


template <typename T>
int GeoListModel<T>::getGeoIdFromGeomListIndex(int index) const
{
    assert(index < int(geomlist.size()));

    if(index < intGeoCount)
        return index;
    else
        return -( index - intGeoCount);
}

template <typename T>
const T GeoListModel<T>::getGeometryFromGeoId(const std::vector<T> & geometrylist, int geoId)
{
    if (geoId >= 0)
        return geometrylist[geoId];
    else
        return geometrylist[geometrylist.size()+geoId];
}

// this function is used to simulate cyclic periodic negative geometry indices (for external geometry)
template <typename T>
const T GeoListModel<T>::getGeometryFromGeoId(int geoId) const
{
    return GeoListModel<T>::getGeometryFromGeoId(geomlist, geoId);
}

template <typename T>
Base::Vector3d GeoListModel<T>::getPoint(int geoId, Sketcher::PointPos pos) const
{
    Part::Geometry * geo = getGeometryFromGeoId(geoId);

    return getPoint(geo, pos);
}

template <typename T>
Base::Vector3d GeoListModel<T>::getPoint(const Part::Geometry * geo, Sketcher::PointPos pos) const
{
    using namespace Sketcher;

    if (geo->getTypeId() == Part::GeomPoint::getClassTypeId()) {
        const Part::GeomPoint *p = static_cast<const Part::GeomPoint*>(geo);
        if (pos == start || pos == mid || pos == end)
            return p->getPoint();
    } else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment*>(geo);
        if (pos == start)
            return lineSeg->getStartPoint();
        else if (pos == end)
            return lineSeg->getEndPoint();
    } else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
        const Part::GeomCircle *circle = static_cast<const Part::GeomCircle*>(geo);
        if (pos == mid)
            return circle->getCenter();
    } else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
        const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse*>(geo);
        if (pos == mid)
            return ellipse->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
        const Part::GeomArcOfCircle *aoc = static_cast<const Part::GeomArcOfCircle*>(geo);
        if (pos == start)
            return aoc->getStartPoint(/*emulateCCW=*/true);
        else if (pos == end)
            return aoc->getEndPoint(/*emulateCCW=*/true);
        else if (pos == mid)
            return aoc->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
        const Part::GeomArcOfEllipse *aoc = static_cast<const Part::GeomArcOfEllipse*>(geo);
        if (pos == start)
            return aoc->getStartPoint(/*emulateCCW=*/true);
        else if (pos == end)
            return aoc->getEndPoint(/*emulateCCW=*/true);
        else if (pos == mid)
            return aoc->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
        const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola*>(geo);
        if (pos == start)
            return aoh->getStartPoint();
        else if (pos == end)
            return aoh->getEndPoint();
        else if (pos == mid)
            return aoh->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
        const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola*>(geo);
        if (pos == start)
            return aop->getStartPoint();
        else if (pos == end)
            return aop->getEndPoint();
        else if (pos == mid)
            return aop->getCenter();
    } else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
        const Part::GeomBSplineCurve *bsp = static_cast<const Part::GeomBSplineCurve*>(geo);
        if (pos == start)
            return bsp->getStartPoint();
        else if (pos == end)
            return bsp->getEndPoint();
    }

    return Base::Vector3d();
}

namespace Sketcher {

// Template specialisations
template <>
GeoListModel<std::unique_ptr< const Sketcher::GeometryFacade>>::GeoListModel(
                                std::vector<std::unique_ptr<const Sketcher::GeometryFacade>> && geometrylist,
                                int intgeocount,
                                bool ownerT) :  geomlist(std::move(geometrylist)),
                                                intGeoCount(intgeocount),
                                                OwnerT(false)
{
    // GeometryFacades hold the responsibility for releasing the resources.
    //
    // This means that each GeometryFacade that is passed to the GeoListModel,
    // must set the ownership (GF->setOwner(true)), if it should be the owner.
    // Otherwise, it follows the default behaviour that some other class, which
    // created the pointer, is responsible for freeing it.
    //
    // Under the Single Responsibility Principle GeoListModel cannot be made
    // responsible for releasing those pointers.
    assert(ownerT == false);
}

template <>
GeoListModel<std::unique_ptr< const Sketcher::GeometryFacade>>::GeoListModel(
                                const std::vector<std::unique_ptr< const Sketcher::GeometryFacade>> & geometrylist,
                                int intgeocount,
                                bool ownerT):   intGeoCount(intgeocount),
                                                OwnerT(false)
{
    // GeometryFacades are movable, but not copiable, so they need to be reconstructed (shallow copy of vector)
    // Under the Single Responsibility Principle, these will not take over a responsibility that shall be enforced
    // on the original GeometryFacade. Use the move version of getGeoListModel if moving the responsibility is intended.
    assert(ownerT == false);

    geomlist.reserve(geometrylist.size());

    for(auto & v : geometrylist) {
        geomlist.push_back(GeometryFacade::getFacade(v->getGeometry()));
    }
}

template <>
GeoListModel<std::unique_ptr<const Sketcher::GeometryFacade>>::~GeoListModel()
{
    // GeometryFacade is responsible for taken ownership of its pointers and deleting them.

}

template < >
const std::unique_ptr<const Sketcher::GeometryFacade>
GeoListModel<std::unique_ptr<const Sketcher::GeometryFacade>>::getGeometryFromGeoId
    (const std::vector<std::unique_ptr<const Sketcher::GeometryFacade>> & geometrylist, int geoId)
{
    if (geoId >= 0)
        return Sketcher::GeometryFacade::getFacade(geometrylist[geoId]->getGeometry());
    else
        return Sketcher::GeometryFacade::getFacade(geometrylist[geometrylist.size()+geoId]->getGeometry());
}

template < >
Base::Vector3d GeoListModel<std::unique_ptr<const Sketcher::GeometryFacade>>::getPoint(int geoId, Sketcher::PointPos pos) const
{
    const Part::Geometry * geo = getGeometryFromGeoId(geoId)->getGeometry();

    return getPoint(geo, pos);
}

// instantiate the types so that other translation units can access template constructors
template class GeoListModel<Part::Geometry *>;
template class GeoListModel<std::unique_ptr<const Sketcher::GeometryFacade>>;


} // namespace Sketcher

GeoListFacade Sketcher::getGeoListFacade(const GeoList & geolist)
{
    std::vector<std::unique_ptr<const GeometryFacade>> facade;
    facade.reserve( geolist.geomlist.size());

    for(auto geo : geolist.geomlist)
        facade.push_back(GeometryFacade::getFacade(geo));

    auto geolistfacade = GeoListFacade::getGeoListModel(std::move(facade), geolist.getInternalCount());

    return geolistfacade;
}

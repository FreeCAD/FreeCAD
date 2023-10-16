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
#include <cassert>
#endif  // #ifndef _PreComp_

#include <boost/core/ignore_unused.hpp>

#include <Base/Exception.h>
#include <Base/Vector3D.h>

#include "GeoList.h"
#include "GeometryFacade.h"


using namespace Sketcher;

// Vector is moved
template<typename T>
GeoListModel<T>::GeoListModel(std::vector<T>&& geometrylist, int intgeocount, bool ownerT)
    : geomlist(std::move(geometrylist))
    , intGeoCount(intgeocount)
    , OwnerT(ownerT)
    , indexInit(false)
{}

// Vector is shallow copied (copy constructed)
template<typename T>
GeoListModel<T>::GeoListModel(const std::vector<T>& geometrylist,
                              int intgeocount)
    : geomlist(geometrylist)
    ,  // copy constructed here
    intGeoCount(intgeocount)
    , OwnerT(false)
    , indexInit(false)
{}

template<typename T>
GeoListModel<T>::~GeoListModel()
{
    if (OwnerT) {
        for (auto& g : geomlist) {
            delete g;
        }
    }
}

template<typename T>
GeoListModel<T>
GeoListModel<T>::getGeoListModel(std::vector<T>&& geometrylist, int intgeocount, bool ownerT)
{
    return GeoListModel(std::move(geometrylist), intgeocount, ownerT);
}

template<typename T>
const GeoListModel<T> GeoListModel<T>::getGeoListModel(const std::vector<T>& geometrylist,
                                                       int intgeocount)
{
    return GeoListModel(geometrylist, intgeocount);
}


template<typename T>
int GeoListModel<T>::getGeoIdFromGeomListIndex(int index) const
{
    assert(index < int(geomlist.size()));

    if (index < intGeoCount) {
        return index;
    }
    else {
        return (index - geomlist.size());
    }
}

template<typename T>
const Part::Geometry* GeoListModel<T>::getGeometryFromGeoId(const std::vector<T>& geometrylist,
                                                            int geoId)
{
    if constexpr (std::is_same<T, GeometryPtr>()) {
        if (geoId >= 0) {
            return geometrylist[geoId];
        }
        else {
            return geometrylist[geometrylist.size() + geoId];
        }
    }
    else if constexpr (std::is_same<T, GeometryFacadeUniquePtr>()) {
        if (geoId >= 0) {
            return geometrylist[geoId]->getGeometry();
        }
        else {
            return geometrylist[geometrylist.size() + geoId]->getGeometry();
        }
    }
}

template<typename T>
const Sketcher::GeometryFacade*
GeoListModel<T>::getGeometryFacadeFromGeoId(const std::vector<T>& geometrylist, int geoId)
{
    if constexpr (std::is_same<T, GeometryPtr>()) {
        if (geoId >= 0) {
            return GeometryFacade::getFacade(geometrylist[geoId]).release();
        }
        else {
            return GeometryFacade::getFacade(geometrylist[geometrylist.size() + geoId]).release();
        }
    }
    else if constexpr (std::is_same<T, GeometryFacadeUniquePtr>()) {
        if (geoId >= 0) {
            return geometrylist[geoId].get();
        }
        else {
            return geometrylist[geometrylist.size() + geoId].get();
        }
    }
}

// this function is used to simulate cyclic periodic negative geometry indices (for external
// geometry)
template<typename T>
const Part::Geometry* GeoListModel<T>::getGeometryFromGeoId(int geoId) const
{
    return GeoListModel<T>::getGeometryFromGeoId(geomlist, geoId);
}

template<typename T>
const Sketcher::GeometryFacade* GeoListModel<T>::getGeometryFacadeFromGeoId(int geoId) const
{
    return GeoListModel<T>::getGeometryFacadeFromGeoId(geomlist, geoId);
}

template<typename T>
Base::Vector3d GeoListModel<T>::getPoint(int geoId, Sketcher::PointPos pos) const
{
    const Part::Geometry* geo = getGeometryFromGeoId(geoId);

    return getPoint(geo, pos);
}

template<typename T>
Base::Vector3d GeoListModel<T>::getPoint(const GeoElementId& geid) const
{
    return getPoint(geid.GeoId, geid.Pos);
}

template<typename T>
Base::Vector3d GeoListModel<T>::getPoint(const Part::Geometry* geo, Sketcher::PointPos pos) const
{
    using namespace Sketcher;

    if (geo->getTypeId() == Part::GeomPoint::getClassTypeId()) {
        const Part::GeomPoint* p = static_cast<const Part::GeomPoint*>(geo);
        if (pos == PointPos::start || pos == PointPos::mid || pos == PointPos::end) {
            return p->getPoint();
        }
    }
    else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        const Part::GeomLineSegment* lineSeg = static_cast<const Part::GeomLineSegment*>(geo);
        if (pos == PointPos::start) {
            return lineSeg->getStartPoint();
        }
        else if (pos == PointPos::end) {
            return lineSeg->getEndPoint();
        }
    }
    else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
        const Part::GeomCircle* circle = static_cast<const Part::GeomCircle*>(geo);
        if (pos == PointPos::mid) {
            return circle->getCenter();
        }
    }
    else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
        const Part::GeomEllipse* ellipse = static_cast<const Part::GeomEllipse*>(geo);
        if (pos == PointPos::mid) {
            return ellipse->getCenter();
        }
    }
    else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
        const Part::GeomArcOfCircle* aoc = static_cast<const Part::GeomArcOfCircle*>(geo);
        if (pos == PointPos::start) {
            return aoc->getStartPoint(/*emulateCCW=*/true);
        }
        else if (pos == PointPos::end) {
            return aoc->getEndPoint(/*emulateCCW=*/true);
        }
        else if (pos == PointPos::mid) {
            return aoc->getCenter();
        }
    }
    else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
        const Part::GeomArcOfEllipse* aoc = static_cast<const Part::GeomArcOfEllipse*>(geo);
        if (pos == PointPos::start) {
            return aoc->getStartPoint(/*emulateCCW=*/true);
        }
        else if (pos == PointPos::end) {
            return aoc->getEndPoint(/*emulateCCW=*/true);
        }
        else if (pos == PointPos::mid) {
            return aoc->getCenter();
        }
    }
    else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
        const Part::GeomArcOfHyperbola* aoh = static_cast<const Part::GeomArcOfHyperbola*>(geo);
        if (pos == PointPos::start) {
            return aoh->getStartPoint();
        }
        else if (pos == PointPos::end) {
            return aoh->getEndPoint();
        }
        else if (pos == PointPos::mid) {
            return aoh->getCenter();
        }
    }
    else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
        const Part::GeomArcOfParabola* aop = static_cast<const Part::GeomArcOfParabola*>(geo);
        if (pos == PointPos::start) {
            return aop->getStartPoint();
        }
        else if (pos == PointPos::end) {
            return aop->getEndPoint();
        }
        else if (pos == PointPos::mid) {
            return aop->getCenter();
        }
    }
    else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
        const Part::GeomBSplineCurve* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);
        if (pos == PointPos::start) {
            return bsp->getStartPoint();
        }
        else if (pos == PointPos::end) {
            return bsp->getEndPoint();
        }
    }

    return Base::Vector3d();
}

template<typename T>
void GeoListModel<T>::rebuildVertexIndex() const
{
    VertexId2GeoElementId.clear();
    GeoElementId2VertexId.clear();

    int geoId = 0;
    int pointId = 0;

    auto addGeoElement = [this, &pointId](int geoId, PointPos pos) {
        VertexId2GeoElementId.emplace_back(geoId, pos);
        GeoElementId2VertexId.emplace(std::piecewise_construct,
                                      std::forward_as_tuple(geoId, pos),
                                      std::forward_as_tuple(pointId++));
    };

    if (geomlist.size() <= 2) {
        return;
    }
    for (auto it = geomlist.begin(); it != geomlist.end(); ++it, geoId++) {

        Base::Type type;

        if constexpr (std::is_same<T, Part::Geometry*>::value) {
            type = (*it)->getTypeId();
        }
        else if constexpr (std::is_same<T,
                                        std::unique_ptr<const Sketcher::GeometryFacade>>::value) {
            type = (*it)->getGeometry()->getTypeId();
        }

        if (geoId > getInternalCount()) {
            geoId = -getExternalCount();
        }

        if (type == Part::GeomPoint::getClassTypeId()) {
            addGeoElement(geoId, PointPos::start);
        }
        else if (type == Part::GeomLineSegment::getClassTypeId()
                 || type == Part::GeomBSplineCurve::getClassTypeId()) {
            addGeoElement(geoId, PointPos::start);
            addGeoElement(geoId, PointPos::end);
        }
        else if (type == Part::GeomCircle::getClassTypeId()
                 || type == Part::GeomEllipse::getClassTypeId()) {
            addGeoElement(geoId, PointPos::mid);
        }
        else if (type == Part::GeomArcOfCircle::getClassTypeId()
                 || type == Part::GeomArcOfEllipse::getClassTypeId()
                 || type == Part::GeomArcOfHyperbola::getClassTypeId()
                 || type == Part::GeomArcOfParabola::getClassTypeId()) {
            addGeoElement(geoId, PointPos::start);
            addGeoElement(geoId, PointPos::end);
            addGeoElement(geoId, PointPos::mid);
        }
    }

    indexInit = true;
}

template<typename T>
Sketcher::GeoElementId GeoListModel<T>::getGeoElementIdFromVertexId(int vertexId)
{
    if (!indexInit) {  // lazy initialised
        rebuildVertexIndex();
    }

    return VertexId2GeoElementId[vertexId];
}

template<typename T>
int GeoListModel<T>::getVertexIdFromGeoElementId(const Sketcher::GeoElementId& geoelementId) const
{
    if (!indexInit) {  // lazy initialised
        rebuildVertexIndex();
    }

    auto found =
        std::find(VertexId2GeoElementId.begin(), VertexId2GeoElementId.end(), geoelementId);

    if (found != VertexId2GeoElementId.end()) {
        return std::distance(found, VertexId2GeoElementId.begin());
    }

    THROWM(Base::IndexError, "GeoElementId not indexed");
}


namespace Sketcher
{

// Template specialisations
template<>
GeoListModel<GeometryFacadeUniquePtr>::GeoListModel(
    std::vector<GeometryFacadeUniquePtr>&& geometrylist,
    int intgeocount,
    bool ownerT)
    : geomlist(std::move(geometrylist))
    , intGeoCount(intgeocount)
    , OwnerT(false)
    , indexInit(false)
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
    boost::ignore_unused(ownerT);
}

template<>
GeoListModel<GeometryFacadeUniquePtr>::GeoListModel(
    const std::vector<GeometryFacadeUniquePtr>& geometrylist,
    int intgeocount)
    : intGeoCount(intgeocount)
    , OwnerT(false)
    , indexInit(false)
{
    // GeometryFacades are movable, but not copiable, so they need to be reconstructed (shallow copy
    // of vector) Under the Single Responsibility Principle, these will not take over a
    // responsibility that shall be enforced on the original GeometryFacade. Use the move version of
    // getGeoListModel if moving the responsibility is intended.

    geomlist.reserve(geometrylist.size());

    for (auto& v : geometrylist) {
        geomlist.push_back(GeometryFacade::getFacade(v->getGeometry()));
    }
}

template<>
SketcherExport GeoListModel<std::unique_ptr<const Sketcher::GeometryFacade>>::~GeoListModel()
{
    // GeometryFacade is responsible for taken ownership of its pointers and deleting them.
}


// instantiate the types so that other translation units can access template constructors
template class SketcherExport GeoListModel<Part::Geometry*>;
#if !defined(__MINGW32__)
template class SketcherExport GeoListModel<std::unique_ptr<const Sketcher::GeometryFacade>>;
#else
// Remark: It looks like when implementing a method of GeoListModel for GeometryFacadeUniquePtr then
// under MinGW the explicit template instantiation doesn't do anything. As workaround all other
// methods must be declared separately
template SketcherExport const Part::Geometry*
GeoListModel<GeometryFacadeUniquePtr>::getGeometryFromGeoId(int geoId) const;
template SketcherExport const Sketcher::GeometryFacade*
GeoListModel<GeometryFacadeUniquePtr>::getGeometryFacadeFromGeoId(int geoId) const;
template SketcherExport int
GeoListModel<GeometryFacadeUniquePtr>::getGeoIdFromGeomListIndex(int index) const;
template SketcherExport int GeoListModel<GeometryFacadeUniquePtr>::getVertexIdFromGeoElementId(
    const Sketcher::GeoElementId&) const;
template SketcherExport GeoElementId
GeoListModel<GeometryFacadeUniquePtr>::getGeoElementIdFromVertexId(int);
template SketcherExport Base::Vector3d
GeoListModel<GeometryFacadeUniquePtr>::getPoint(int geoId, Sketcher::PointPos pos) const;
template SketcherExport Base::Vector3d
GeoListModel<GeometryFacadeUniquePtr>::getPoint(const GeoElementId&) const;
template SketcherExport GeoListModel<GeometryFacadeUniquePtr>
GeoListModel<GeometryFacadeUniquePtr>::getGeoListModel(
    std::vector<GeometryFacadeUniquePtr>&& geometrylist,
    int intgeocount,
    bool ownerT);
#endif


}  // namespace Sketcher

GeoListFacade Sketcher::getGeoListFacade(const GeoList& geolist)
{
    std::vector<std::unique_ptr<const GeometryFacade>> facade;
    facade.reserve(geolist.geomlist.size());

    for (auto geo : geolist.geomlist) {
        facade.push_back(GeometryFacade::getFacade(geo));
    }

    auto geolistfacade =
        GeoListFacade::getGeoListModel(std::move(facade), geolist.getInternalCount());

    return geolistfacade;
}

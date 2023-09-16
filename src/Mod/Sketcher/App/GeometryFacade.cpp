/***************************************************************************
 *   Copyright (c) 2020 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
#include <boost/uuid/uuid_io.hpp>
#endif

#include "GeometryFacade.h"
#include "GeometryFacadePy.h"


using namespace Sketcher;

TYPESYSTEM_SOURCE(Sketcher::GeometryFacade, Base::BaseClass)

GeometryFacade::GeometryFacade()
    : Geo(nullptr)
    , OwnerGeo(false)
    , SketchGeoExtension(nullptr)
{}

GeometryFacade::GeometryFacade(const Part::Geometry* geometry, bool owner)
    : Geo(geometry)
    , OwnerGeo(owner)
{
    assert(geometry);  // This should never be nullptr, as this constructor is protected

    initExtension();
}

GeometryFacade::~GeometryFacade()
{
    if (OwnerGeo && Geo) {
        delete Geo;
    }
}

std::unique_ptr<GeometryFacade> GeometryFacade::getFacade(Part::Geometry* geometry, bool owner)
{
    if (geometry) {
        return std::unique_ptr<GeometryFacade>(new GeometryFacade(geometry, owner));
    }
    else {
        return std::unique_ptr<GeometryFacade>(nullptr);
    }
    // make_unique has no access to private constructor
    // return std::make_unique<GeometryFacade>(geometry);
}

std::unique_ptr<const GeometryFacade> GeometryFacade::getFacade(const Part::Geometry* geometry)
{
    if (geometry) {
        return std::unique_ptr<const GeometryFacade>(new GeometryFacade(geometry));
    }
    else {
        return std::unique_ptr<const GeometryFacade>(nullptr);
    }
    // make_unique has no access to private constructor
    // return std::make_unique<const GeometryFacade>(geometry);
}

void GeometryFacade::setGeometry(Part::Geometry* geometry)
{
    Geo = geometry;

    if (geometry) {
        initExtension();
    }
    else {
        THROWM(Base::ValueError, "GeometryFacade initialized with Geometry null pointer");
    }
}

void GeometryFacade::initExtension()
{
    if (!Geo->hasExtension(SketchGeometryExtension::getClassTypeId())) {

        getGeo()->setExtension(std::make_unique<SketchGeometryExtension>());  // Create getExtension

        // Base::Console().Warning("%s\nSketcher Geometry without Extension: %s \n",
        // boost::uuids::to_string(Geo->getTag()).c_str());
    }

    SketchGeoExtension = std::static_pointer_cast<const SketchGeometryExtension>(
        (Geo->getExtension(SketchGeometryExtension::getClassTypeId())).lock());
}

void GeometryFacade::initExtension() const
{
    // const Geometry without SketchGeometryExtension cannot initialise a GeometryFacade
    if (!Geo->hasExtension(SketchGeometryExtension::getClassTypeId())) {
        THROWM(Base::ValueError,
               "Cannot create a GeometryFacade out of a const Geometry pointer not having a "
               "SketchGeometryExtension!");
    }

    auto ext = std::static_pointer_cast<const SketchGeometryExtension>(
        Geo->getExtension(SketchGeometryExtension::getClassTypeId()).lock());

    const_cast<GeometryFacade*>(this)->SketchGeoExtension = ext;
}

void GeometryFacade::throwOnNullPtr(const Part::Geometry* geo)
{
    if (!geo) {
        THROWM(Base::ValueError, "Geometry is nullptr!");
    }
}

void GeometryFacade::ensureSketchGeometryExtension(Part::Geometry* geometry)
{
    throwOnNullPtr(geometry);

    if (!geometry->hasExtension(SketchGeometryExtension::getClassTypeId())) {
        geometry->setExtension(std::make_unique<SketchGeometryExtension>());  // Create getExtension
    }
}

void GeometryFacade::copyId(const Part::Geometry* src, Part::Geometry* dst)
{
    throwOnNullPtr(src);
    throwOnNullPtr(dst);

    auto gfsrc = GeometryFacade::getFacade(src);
    auto gfdst = GeometryFacade::getFacade(dst);
    gfdst->setId(gfsrc->getId());
}

bool GeometryFacade::getConstruction(const Part::Geometry* geometry)
{
    throwOnNullPtr(geometry);

    auto gf = GeometryFacade::getFacade(geometry);
    return gf->getConstruction();
}

void GeometryFacade::setConstruction(Part::Geometry* geometry, bool construction)
{
    throwOnNullPtr(geometry);

    auto gf = GeometryFacade::getFacade(geometry);
    return gf->setConstruction(construction);
}

bool GeometryFacade::isInternalType(const Part::Geometry* geometry, InternalType::InternalType type)
{
    throwOnNullPtr(geometry);

    auto gf = GeometryFacade::getFacade(geometry);
    return gf->getInternalType() == type;
}

bool GeometryFacade::isInternalAligned(const Part::Geometry* geometry)
{
    throwOnNullPtr(geometry);

    auto gf = GeometryFacade::getFacade(geometry);
    return gf->isInternalAligned();
}

bool GeometryFacade::getBlocked(const Part::Geometry* geometry)
{
    throwOnNullPtr(geometry);

    auto gf = GeometryFacade::getFacade(geometry);
    return gf->getBlocked();
}

PyObject* GeometryFacade::getPyObject()
{
    return new GeometryFacadePy(new GeometryFacade(this->Geo));
}

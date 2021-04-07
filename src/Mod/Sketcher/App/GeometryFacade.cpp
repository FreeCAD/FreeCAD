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

#endif

#include "GeometryFacade.h"

//#include <Base/Console.h> // Only for Debug, when/if necessary
#include <Base/Exception.h>
#include <boost/uuid/uuid_io.hpp>

#include "GeometryFacadePy.h"

using namespace Sketcher;

TYPESYSTEM_SOURCE(Sketcher::GeometryFacade,Base::BaseClass)

GeometryFacade::GeometryFacade(): Geo(nullptr), SketchGeoExtension(nullptr)
{

}

GeometryFacade::GeometryFacade(const Part::Geometry * geometry)
: Geo(geometry)
{
    if(geometry != nullptr)
        initExtension();
    else
        THROWM(Base::ValueError, "GeometryFacade initialized with Geometry null pointer");

}

std::unique_ptr<GeometryFacade> GeometryFacade::getFacade(Part::Geometry * geometry)
{
    if(geometry != nullptr)
        return std::unique_ptr<GeometryFacade>(new GeometryFacade(geometry));
    else
        return std::unique_ptr<GeometryFacade>(nullptr);
    //return std::make_unique<GeometryFacade>(geometry); // make_unique has no access to private constructor
}

std::unique_ptr<const GeometryFacade> GeometryFacade::getFacade(const Part::Geometry * geometry)
{
    if(geometry != nullptr)
        return std::unique_ptr<const GeometryFacade>(new GeometryFacade(geometry));
     else
        return std::unique_ptr<const GeometryFacade>(nullptr);
    //return std::make_unique<const GeometryFacade>(geometry); // make_unique has no access to private constructor
}

void GeometryFacade::setGeometry(Part::Geometry *geometry)
{
    Geo = geometry;

    if(geometry != nullptr)
        initExtension();
    else
        THROWM(Base::ValueError, "GeometryFacade initialized with Geometry null pointer");
}

void GeometryFacade::initExtension()
{
    if(!Geo->hasExtension(SketchGeometryExtension::getClassTypeId())) {

        getGeo()->setExtension(std::make_unique<SketchGeometryExtension>()); // Create getExtension

        //Base::Console().Warning("%s\nSketcher Geometry without Extension: %s \n", boost::uuids::to_string(Geo->getTag()).c_str());
    }

    SketchGeoExtension =
        std::static_pointer_cast<const SketchGeometryExtension>(
            (Geo->getExtension(SketchGeometryExtension::getClassTypeId())).lock()
        );
}

void GeometryFacade::initExtension() const
{
    if(!Geo->hasExtension(SketchGeometryExtension::getClassTypeId()))
           THROWM(Base::ValueError, "GeometryConstFacade for const::Geometry without SketchGeometryExtension");

    auto ext = std::static_pointer_cast<const SketchGeometryExtension>(Geo->getExtension(SketchGeometryExtension::getClassTypeId()).lock());

    const_cast<GeometryFacade *>(this)->SketchGeoExtension = ext;
}

void GeometryFacade::ensureSketchGeometryExtension(Part::Geometry * geometry)
{
    if(!geometry->hasExtension(SketchGeometryExtension::getClassTypeId())) {
        geometry->setExtension(std::make_unique<SketchGeometryExtension>()); // Create getExtension
    }
}

void GeometryFacade::copyId(const Part::Geometry * src, Part::Geometry * dst)
{
    auto gfsrc = GeometryFacade::getFacade(src);
    auto gfdst = GeometryFacade::getFacade(dst);
    gfdst->setId(gfsrc->getId());
}

bool GeometryFacade::getConstruction(const Part::Geometry * geometry)
{
    auto gf = GeometryFacade::getFacade(geometry);
    return gf->getConstruction();
}

void GeometryFacade::setConstruction(Part::Geometry * geometry, bool construction)
{
    auto gf = GeometryFacade::getFacade(geometry);
    return gf->setConstruction(construction);
}

bool GeometryFacade::isInternalType(const Part::Geometry * geometry, InternalType::InternalType type)
{
    auto gf = GeometryFacade::getFacade(geometry);
    return gf->getInternalType() == type;
}

bool GeometryFacade::getBlocked(const Part::Geometry * geometry)
{
    auto gf = GeometryFacade::getFacade(geometry);
    return gf->getBlocked();
}

PyObject * GeometryFacade::getPyObject(void)
{
    return new GeometryFacadePy(new GeometryFacade(this->Geo));
}

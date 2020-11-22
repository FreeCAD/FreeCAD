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

#include "ExternalGeometryFacade.h"

#include <Base/Console.h> // Only for Debug - To be removed
#include <Base/Exception.h>
#include <boost/uuid/uuid_io.hpp>

#include "ExternalGeometryFacadePy.h"

using namespace Sketcher;

TYPESYSTEM_SOURCE(Sketcher::ExternalGeometryFacade,Part::GeometryExtension)

ExternalGeometryFacade::ExternalGeometryFacade(): Geo(nullptr), SketchGeoExtension(nullptr), ExternalGeoExtension(nullptr)
{

}

ExternalGeometryFacade::ExternalGeometryFacade(const Part::Geometry * geometry)
: Geo(geometry)
{
    if(geometry != nullptr)
        initExtensions();
    else
        THROWM(Base::ValueError, "ExternalGeometryFacade initialized with Geometry null pointer");
}

std::unique_ptr<ExternalGeometryFacade> ExternalGeometryFacade::getFacade(Part::Geometry * geometry)
{
    return std::unique_ptr<ExternalGeometryFacade>(new ExternalGeometryFacade(geometry));
    //return std::make_unique<ExternalGeometryFacade>(geometry); // make_unique has no access to private constructor
}

std::unique_ptr<const ExternalGeometryFacade> ExternalGeometryFacade::getFacade(const Part::Geometry * geometry)
{
    return std::unique_ptr<const ExternalGeometryFacade>(new ExternalGeometryFacade(geometry));
    //return std::make_unique<const ExternalGeometryFacade>(geometry); // make_unique has no access to private constructor
}

void ExternalGeometryFacade::setGeometry(Part::Geometry *geometry)
{
    Geo = geometry;

    if(geometry != nullptr)
        initExtensions();
    else
        THROWM(Base::ValueError, "ExternalGeometryFacade initialized with Geometry null pointer");
}

void ExternalGeometryFacade::initExtensions()
{
    if(!Geo->hasExtension(SketchGeometryExtension::getClassTypeId())) {

        getGeo()->setExtension(std::make_unique<SketchGeometryExtension>()); // Create getExtension

        Base::Console().Warning("%s\nSketcher External Geometry without Geometry Extension: %s \n", boost::uuids::to_string(Geo->getTag()).c_str());
    }

    if(!Geo->hasExtension(ExternalGeometryExtension::getClassTypeId())) {

        getGeo()->setExtension(std::make_unique<ExternalGeometryExtension>()); // Create getExtension

        Base::Console().Warning("%s\nSketcher External Geometry without ExternalGeometryExtension: %s \n", boost::uuids::to_string(Geo->getTag()).c_str());
    }

    SketchGeoExtension =
        std::static_pointer_cast<const SketchGeometryExtension>(
            (Geo->getExtension(SketchGeometryExtension::getClassTypeId())).lock()
        );

    ExternalGeoExtension =
        std::static_pointer_cast<const ExternalGeometryExtension>(
            (Geo->getExtension(ExternalGeometryExtension::getClassTypeId())).lock()
        );
}

void ExternalGeometryFacade::initExtensions() const
{
    if(!Geo->hasExtension(SketchGeometryExtension::getClassTypeId()))
           THROWM(Base::ValueError, "ExternalGeometryFacade for const::Geometry without SketchGeometryExtension");

    if(!Geo->hasExtension(ExternalGeometryExtension::getClassTypeId()))
           THROWM(Base::ValueError, "ExternalGeometryFacade for const::Geometry without ExternalGeometryExtension");

    auto ext = std::static_pointer_cast<const SketchGeometryExtension>(Geo->getExtension(SketchGeometryExtension::getClassTypeId()).lock());

    const_cast<ExternalGeometryFacade *>(this)->SketchGeoExtension = ext;

    auto extext = std::static_pointer_cast<const ExternalGeometryExtension>(Geo->getExtension(ExternalGeometryExtension::getClassTypeId()).lock());

    const_cast<ExternalGeometryFacade *>(this)->ExternalGeoExtension = extext;
}

void ExternalGeometryFacade::ensureSketchGeometryExtensions(Part::Geometry * geometry)
{
    if(!geometry->hasExtension(SketchGeometryExtension::getClassTypeId())) {
        geometry->setExtension(std::make_unique<SketchGeometryExtension>()); // Create geoExtension
    }

    if(!geometry->hasExtension(ExternalGeometryExtension::getClassTypeId())) {
        geometry->setExtension(std::make_unique<ExternalGeometryExtension>()); // Create external geoExtension
    }
}

void ExternalGeometryFacade::copyId(const Part::Geometry * src, Part::Geometry * dst)
{
    auto gfsrc = ExternalGeometryFacade::getFacade(src);
    auto gfdst = ExternalGeometryFacade::getFacade(dst);
    gfdst->setId(gfsrc->getId());
}

PyObject * ExternalGeometryFacade::getPyObject(void)
{
    return new ExternalGeometryFacadePy(new ExternalGeometryFacade(this->Geo));
}

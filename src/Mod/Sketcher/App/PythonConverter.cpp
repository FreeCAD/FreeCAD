/***************************************************************************
 *   Copyright (c) 2022 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
# include <boost/format.hpp>
#endif  // #ifndef _PreComp_

#include <Base/Exception.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/GeometryFacade.h>

#include "PythonConverter.h"


using namespace Sketcher;

std::string PythonConverter::convert(const Part::Geometry * geo)
{
    // "addGeometry(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)),%s)"

    std::string command;
    auto sg = process(geo);

    command = boost::str(boost::format("addGeometry(%s,%s)\n") %
                    sg.creation % (sg.construction ? "True":"False"));

    return command;
}

std::string PythonConverter::convert(const std::string & doc, const std::vector<Part::Geometry *> & geos)
{
    std::string geolist = "geoList = []\n";
    std::string constrgeolist = "constrGeoList = []\n";

    int ngeo = 0, nconstr = 0;

    for(auto geo : geos) {
        auto sg = process(geo);

        if (sg.construction) {
            constrgeolist = boost::str(boost::format("%s\nconstrGeoList.append(%s)\n") %
            constrgeolist % sg.creation);
            nconstr++;
        }
        else {
            geolist = boost::str(boost::format("%s\ngeoList.append(%s)\n") %
            geolist % sg.creation);
            ngeo++;
        }
    }

    if(ngeo > 0) {
        geolist = boost::str(boost::format("%s\n%s.addGeometry(geoList,%s)\ndel geoList\n") %
            geolist % doc % "False");
    }

    if(nconstr > 0) {
        constrgeolist = boost::str(boost::format("%s\n%s.addGeometry(constrGeoList,%s)\ndel constrGeoList") %
            constrgeolist % doc % "True");
    }

    std::string command;

    if(ngeo > 0 && nconstr > 0)
        command = geolist + constrgeolist;
    else if (ngeo > 0)
        command = std::move(geolist);
    else if (nconstr > 0)
        command = std::move(constrgeolist);

    return command;
}

PythonConverter::SingleGeometry PythonConverter::process(const Part::Geometry * geo)
{
    static std::map<const Base::Type, std::function<SingleGeometry(const Part::Geometry * geo)>> converterMap = {
    { Part::GeomLineSegment::getClassTypeId(),
        [](const Part::Geometry * geo){
            auto sgeo = static_cast<const Part::GeomLineSegment *>(geo);
            SingleGeometry sg;
            sg.creation = boost::str(boost::format("Part.LineSegment(App.Vector(%f,%f,%f),App.Vector(%f,%f,%f))") %
                    sgeo->getStartPoint().x % sgeo->getStartPoint().y % sgeo->getStartPoint().z %
                    sgeo->getEndPoint().x % sgeo->getEndPoint().y % sgeo->getEndPoint().z);
            sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
            return sg;
        }},
    { Part::GeomArcOfCircle::getClassTypeId(),
        [](const Part::Geometry * geo){
            auto arc = static_cast<const Part::GeomArcOfCircle *>(geo);
            SingleGeometry sg;
            sg.creation = boost::str(boost::format("Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f, %f), App.Vector(%f, %f, %f), %f), %f, %f)") %
                    arc->getCenter().x % arc->getCenter().y % arc->getCenter().z %
                    arc->getAxisDirection().x % arc->getAxisDirection().y % arc->getAxisDirection().z %
                    arc->getRadius() % arc->getFirstParameter() % arc->getLastParameter());
            sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
            return sg;
        }},

    };

    auto result = converterMap.find(geo->getTypeId());

    if( result == converterMap.end())
        THROWM(Base::ValueError, "PythonConverter: Geometry Type not supported")

    auto creator = result->second;

    return creator(geo);
}

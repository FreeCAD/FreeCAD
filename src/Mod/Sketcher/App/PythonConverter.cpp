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
#include <boost/algorithm/string/regex.hpp>
#include <boost/format.hpp>
#endif  // #ifndef _PreComp_

#include <Base/Exception.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/GeometryFacade.h>

#include "PythonConverter.h"


using namespace Sketcher;

std::string PythonConverter::convert(const Part::Geometry* geo)
{
    // "addGeometry(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)),%s)"

    std::string command;
    auto sg = process(geo);

    command = boost::str(boost::format("addGeometry(%s,%s)\n") % sg.creation
                         % (sg.construction ? "True" : "False"));

    return command;
}

std::string PythonConverter::convert(const Sketcher::Constraint* constraint)
{
    // addConstraint(Sketcher.Constraint('Distance',%d,%f))
    std::string command;
    auto cg = process(constraint);

    command = boost::str(boost::format("addConstraint(%s)\n") % cg);

    return command;
}

std::string PythonConverter::convert(const std::string& doc,
                                     const std::vector<Part::Geometry*>& geos)
{
    if (geos.empty()) {
        return std::string();
    }

    // Generates a list for consecutive geometries of construction type, or of normal type
    auto printGeoList = [&doc](const std::string& geolist, int ngeos, bool construction) {
        std::string command;

        if (ngeos > 0) {
            if (construction) {
                command = boost::str(
                    boost::format("constrGeoList = []\n%s\n%s.addGeometry(constrGeoList,%s)\ndel "
                                  "constrGeoList")
                    % geolist % doc % "True");
            }
            else {
                command = boost::str(
                    boost::format("geoList = []\n%s\n%s.addGeometry(geoList,%s)\ndel geoList")
                    % geolist % doc % "False");
            }
        }

        return command;
    };

    std::string command;

    // Adds a list of consecutive geometries of a same construction type to the generating command
    auto addToCommands = [&command,
                          &printGeoList](const std::string& geolist, int ngeos, bool construction) {
        auto newcommand = printGeoList(geolist, ngeos, construction);

        if (command.empty()) {
            command = std::move(newcommand);
        }
        else {
            command += "\n";
            command += newcommand;
        }
    };

    std::string geolist;
    int ngeos = 0;
    bool currentconstruction = Sketcher::GeometryFacade::getConstruction(geos[0]);

    for (auto geo : geos) {
        auto sg = process(geo);

        if (sg.construction != currentconstruction) {
            // if it switches from construction to normal or vice versa, flush elements so far in
            // order to keep order of creation
            addToCommands(geolist, ngeos, currentconstruction);

            geolist.clear();
            ngeos = 0;
            currentconstruction = sg.construction;
        }

        if (sg.construction) {
            geolist =
                boost::str(boost::format("%s\nconstrGeoList.append(%s)\n") % geolist % sg.creation);
        }
        else {
            geolist = boost::str(boost::format("%s\ngeoList.append(%s)\n") % geolist % sg.creation);
        }

        ngeos++;
    }

    addToCommands(geolist, ngeos, currentconstruction);

    return command;
}

std::string PythonConverter::convert(const std::string& doc,
                                     const std::vector<Sketcher::Constraint*>& constraints)
{
    if (constraints.size() == 1) {
        auto cg = convert(constraints[0]);

        return boost::str(boost::format("%s.%s\n") % doc % cg);
    }

    std::string constraintlist = "constraintList = []";

    for (auto constraint : constraints) {
        auto cg = process(constraint);

        constraintlist =
            boost::str(boost::format("%s\nconstraintList.append(%s)") % constraintlist % cg);
    }

    if (!constraints.empty()) {
        constraintlist =
            boost::str(boost::format("%s\n%s.addConstraint(constraintList)\ndel constraintList\n")
                       % constraintlist % doc);
    }

    return constraintlist;
}

PythonConverter::SingleGeometry PythonConverter::process(const Part::Geometry* geo)
{
    static std::map<const Base::Type, std::function<SingleGeometry(const Part::Geometry* geo)>>
        converterMap = {
            {Part::GeomLineSegment::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto sgeo = static_cast<const Part::GeomLineSegment*>(geo);
                 SingleGeometry sg;
                 sg.creation = boost::str(
                     boost::format("Part.LineSegment(App.Vector(%f,%f,%f),App.Vector(%f,%f,%f))")
                     % sgeo->getStartPoint().x % sgeo->getStartPoint().y % sgeo->getStartPoint().z
                     % sgeo->getEndPoint().x % sgeo->getEndPoint().y % sgeo->getEndPoint().z);
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
            {Part::GeomArcOfCircle::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto arc = static_cast<const Part::GeomArcOfCircle*>(geo);
                 SingleGeometry sg;
                 sg.creation =
                     boost::str(boost::format("Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f, "
                                              "%f), App.Vector(%f, %f, %f), %f), %f, %f)")
                                % arc->getCenter().x % arc->getCenter().y % arc->getCenter().z
                                % arc->getAxisDirection().x % arc->getAxisDirection().y
                                % arc->getAxisDirection().z % arc->getRadius()
                                % arc->getFirstParameter() % arc->getLastParameter());
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
            {Part::GeomPoint::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto sgeo = static_cast<const Part::GeomPoint*>(geo);
                 SingleGeometry sg;
                 sg.creation =
                     boost::str(boost::format("Part.Point(App.Vector(%f,%f,%f))")
                                % sgeo->getPoint().x % sgeo->getPoint().y % sgeo->getPoint().z);
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
            {Part::GeomEllipse::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto ellipse = static_cast<const Part::GeomEllipse*>(geo);
                 SingleGeometry sg;
                 auto periapsis =
                     ellipse->getCenter() + ellipse->getMajorAxisDir() * ellipse->getMajorRadius();
                 auto positiveB =
                     ellipse->getCenter() + ellipse->getMinorAxisDir() * ellipse->getMinorRadius();
                 auto center = ellipse->getCenter();
                 sg.creation =
                     boost::str(boost::format("Part.Ellipse(App.Vector(%f, %f, %f), App.Vector(%f, "
                                              "%f, %f), App.Vector(%f, %f, %f))")
                                % periapsis.x % periapsis.y % periapsis.z % positiveB.x
                                % positiveB.y % positiveB.z % center.x % center.y % center.z);
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
            {Part::GeomCircle::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto circle = static_cast<const Part::GeomCircle*>(geo);
                 SingleGeometry sg;
                 sg.creation = boost::str(
                     boost::format(
                         "Part.Circle(App.Vector(%f, %f, %f), App.Vector(%f, %f, %f), %f)")
                     % circle->getCenter().x % circle->getCenter().y % circle->getCenter().z
                     % circle->getAxisDirection().x % circle->getAxisDirection().y
                     % circle->getAxisDirection().z % circle->getRadius());
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
        };

    auto result = converterMap.find(geo->getTypeId());

    if (result == converterMap.end()) {
        THROWM(Base::ValueError, "PythonConverter: Geometry Type not supported")
    }

    auto creator = result->second;

    return creator(geo);
}

std::string PythonConverter::process(const Sketcher::Constraint* constraint)
{
    static std::map<const Sketcher::ConstraintType,
                    std::function<std::string(const Sketcher::Constraint*)>>
        converterMap = {
            {Sketcher::Coincident,
             [](const Sketcher::Constraint* constr) {
                 return boost::str(
                     boost::format("Sketcher.Constraint('Coincident', %i, %i, %i, %i)")
                     % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                     % static_cast<int>(constr->SecondPos));
             }},
            {Sketcher::Horizontal,
             [](const Sketcher::Constraint* constr) {
                 if (constr->Second == GeoEnum::GeoUndef) {
                     return boost::str(boost::format("Sketcher.Constraint('Horizontal', %i)")
                                       % constr->First);
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Horizontal', %i, %i, %i, %i)")
                         % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                         % static_cast<int>(constr->SecondPos));
                 }
             }},
            {Sketcher::Vertical,
             [](const Sketcher::Constraint* constr) {
                 if (constr->Second == GeoEnum::GeoUndef) {
                     return boost::str(boost::format("Sketcher.Constraint('Vertical', %i)")
                                       % constr->First);
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Vertical', %i, %i, %i, %i)")
                         % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                         % static_cast<int>(constr->SecondPos));
                 }
             }},
            {Sketcher::Block,
             [](const Sketcher::Constraint* constr) {
                 return boost::str(boost::format("Sketcher.Constraint('Block', %i)")
                                   % constr->First);
             }},
            {Sketcher::Tangent,
             [](const Sketcher::Constraint* constr) {
                 if (constr->FirstPos == Sketcher::PointPos::none) {
                     return boost::str(boost::format("Sketcher.Constraint('Tangent', %i, %i)")
                                       % constr->First % constr->Second);
                 }
                 else if (constr->SecondPos == Sketcher::PointPos::none) {
                     return boost::str(boost::format("Sketcher.Constraint('Tangent', %i, %i, %i)")
                                       % constr->First % static_cast<int>(constr->FirstPos)
                                       % constr->Second);
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Tangent', %i, %i, %i, %i)")
                         % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                         % static_cast<int>(constr->SecondPos));
                 }
             }},
            {Sketcher::Parallel,
             [](const Sketcher::Constraint* constr) {
                 return boost::str(boost::format("Sketcher.Constraint('Parallel', %i, %i)")
                                   % constr->First % constr->Second);
             }},
            {Sketcher::Perpendicular,
             [](const Sketcher::Constraint* constr) {
                 if (constr->FirstPos == Sketcher::PointPos::none) {
                     return boost::str(boost::format("Sketcher.Constraint('Perpendicular', %i, %i)")
                                       % constr->First % constr->Second);
                 }
                 else if (constr->SecondPos == Sketcher::PointPos::none) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Perpendicular', %i, %i, %i)")
                         % constr->First % static_cast<int>(constr->FirstPos) % constr->Second);
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Perpendicular', %i, %i, %i, %i)")
                         % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                         % static_cast<int>(constr->SecondPos));
                 }
             }},
            {Sketcher::Equal,
             [](const Sketcher::Constraint* constr) {
                 return boost::str(boost::format("Sketcher.Constraint('Equal', %i, %i)")
                                   % constr->First % constr->Second);
             }},
            {Sketcher::InternalAlignment,
             [](const Sketcher::Constraint* constr) {
                 if (constr->InternalAlignmentIndex == EllipseMajorDiameter
                     || constr->InternalAlignmentIndex == EllipseMinorDiameter) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('InternalAlignment:%s', %i, %i)")
                         % constr->internalAlignmentTypeToString() % constr->First
                         % constr->Second);
                 }
                 else if (constr->InternalAlignmentIndex == EllipseFocus1
                          || constr->InternalAlignmentIndex == EllipseFocus2) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('InternalAlignment:%s', %i, %i, %i)")
                         % constr->internalAlignmentTypeToString() % constr->First
                         % static_cast<int>(constr->FirstPos) % constr->Second);
                 }
                 else if (constr->InternalAlignmentIndex == BSplineControlPoint) {
                     return boost::str(
                         boost::format(
                             "Sketcher.Constraint('InternalAlignment:%s', %i, %i, %i, %i)")
                         % constr->internalAlignmentTypeToString() % constr->First
                         % static_cast<int>(constr->FirstPos) % constr->Second
                         % constr->InternalAlignmentIndex);
                 }

                 THROWM(Base::ValueError,
                        "PythonConverter: Constraint Alignment Type not supported")
             }},
            {Sketcher::Distance,
             [](const Sketcher::Constraint* constr) {
                 if (constr->Second == GeoEnum::GeoUndef) {
                     return boost::str(boost::format("Sketcher.Constraint('Distance', %i, %f)")
                                       % constr->First % constr->getValue());
                 }
                 else if (constr->FirstPos == Sketcher::PointPos::none) {
                     return boost::str(boost::format("Sketcher.Constraint('Distance', %i, %i, %f)")
                                       % constr->First % constr->Second % constr->getValue());
                 }
                 else if (constr->SecondPos == Sketcher::PointPos::none) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Distance', %i, %i, %i, %f)")
                         % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                         % constr->getValue());
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Distance', %i, %i, %i, %i, %f)")
                         % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                         % static_cast<int>(constr->SecondPos) % constr->getValue());
                 }
             }},
            {Sketcher::Angle,
             [](const Sketcher::Constraint* constr) {
                 if (constr->Second == GeoEnum::GeoUndef) {
                     return boost::str(boost::format("Sketcher.Constraint('Angle', %i, %f)")
                                       % constr->First % constr->getValue());
                 }
                 else if (constr->SecondPos == Sketcher::PointPos::none) {
                     return boost::str(boost::format("Sketcher.Constraint('Angle', %i, %i, %f)")
                                       % constr->First % constr->Second % constr->getValue());
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Angle', %i, %i, %i, %i, %f)")
                         % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                         % static_cast<int>(constr->SecondPos) % constr->getValue());
                 }
             }},
            {Sketcher::DistanceX,
             [](const Sketcher::Constraint* constr) {
                 if (constr->Second == GeoEnum::GeoUndef) {
                     return boost::str(boost::format("Sketcher.Constraint('DistanceX', %i, %f)")
                                       % constr->First % constr->getValue());
                 }
                 else if (constr->SecondPos == Sketcher::PointPos::none) {
                     return boost::str(boost::format("Sketcher.Constraint('DistanceX', %i, %i, %f)")
                                       % constr->First % static_cast<int>(constr->FirstPos)
                                       % constr->getValue());
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('DistanceX', %i, %i, %i, %i, %f)")
                         % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                         % static_cast<int>(constr->SecondPos) % constr->getValue());
                 }
             }},
            {Sketcher::DistanceY,
             [](const Sketcher::Constraint* constr) {
                 if (constr->Second == GeoEnum::GeoUndef) {
                     return boost::str(boost::format("Sketcher.Constraint('DistanceY', %i, %f)")
                                       % constr->First % constr->getValue());
                 }
                 else if (constr->SecondPos == Sketcher::PointPos::none) {
                     return boost::str(boost::format("Sketcher.Constraint('DistanceY', %i, %i, %f)")
                                       % constr->First % static_cast<int>(constr->FirstPos)
                                       % constr->getValue());
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('DistanceY', %i, %i, %i, %i, %f)")
                         % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                         % static_cast<int>(constr->SecondPos) % constr->getValue());
                 }
             }},
            {Sketcher::Radius,
             [](const Sketcher::Constraint* constr) {
                 return boost::str(boost::format("Sketcher.Constraint('Radius', %i, %f)")
                                   % constr->First % constr->getValue());
             }},
            {Sketcher::Diameter,
             [](const Sketcher::Constraint* constr) {
                 return boost::str(boost::format("Sketcher.Constraint('Diameter', %i, %f)")
                                   % constr->First % constr->getValue());
             }},
            {Sketcher::Weight,
             [](const Sketcher::Constraint* constr) {
                 return boost::str(boost::format("Sketcher.Constraint('Weight', %i, %f)")
                                   % constr->First % constr->getValue());
             }},
            {Sketcher::PointOnObject,
             [](const Sketcher::Constraint* constr) {
                 return boost::str(boost::format("Sketcher.Constraint('PointOnObject', %i, %i, %i)")
                                   % constr->First % static_cast<int>(constr->FirstPos)
                                   % constr->Second);
             }},
            {Sketcher::Symmetric,
             [](const Sketcher::Constraint* constr) {
                 if (constr->ThirdPos == Sketcher::PointPos::none) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Symmetric', %i, %i, %i, %i, %i)")
                         % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                         % static_cast<int>(constr->SecondPos) % constr->Third);
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Symmetric', %i, %i, %i, %i, %i, %i)")
                         % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                         % static_cast<int>(constr->SecondPos) % constr->Third
                         % static_cast<int>(constr->ThirdPos));
                 }
             }},
            {Sketcher::SnellsLaw,
             [](const Sketcher::Constraint* constr) {
                 return boost::str(
                     boost::format("Sketcher.Constraint('SnellsLaw', %i, %i, %i, %i, %i, %f)")
                     % constr->First % static_cast<int>(constr->FirstPos) % constr->Second
                     % static_cast<int>(constr->SecondPos) % constr->Third % constr->getValue());
             }},
        };

    auto result = converterMap.find(constraint->Type);

    if (result == converterMap.end()) {
        THROWM(Base::ValueError, "PythonConverter: Constraint Type not supported")
    }

    auto creator = result->second;

    return creator(constraint);
}

std::vector<std::string> PythonConverter::multiLine(std::string&& singlestring)
{
    std::vector<std::string> tokens;
    split_regex(tokens, singlestring, boost::regex("(\n)+"));
    return tokens;
}

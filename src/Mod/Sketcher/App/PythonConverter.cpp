// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <boost/algorithm/string/regex.hpp>
#include <boost/format.hpp>

#include <Base/Exception.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "PythonConverter.h"


using namespace Sketcher;

std::string PythonConverter::convert(const Part::Geometry* geo, Mode mode)
{
    // "addGeometry(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)),%s)"

    std::string command;
    auto sg = process(geo);

    command = boost::str(
        boost::format("addGeometry(%s,%s)\n") % sg.creation % (sg.construction ? "True" : "False")
    );

    // clang-format off: keep line breaks for readability
    if ((!geo->is<Part::GeomEllipse>()
         || !geo->is<Part::GeomArcOfEllipse>()
         || !geo->is<Part::GeomArcOfHyperbola>()
         || !geo->is<Part::GeomArcOfParabola>()
         || !geo->is<Part::GeomBSplineCurve>()) && mode == Mode::CreateInternalGeometry) {
        command +=
            boost::str(boost::format("exposeInternalGeometry(len(ActiveSketch.Geometry))\n"));
    }
    // clang-format on

    return command;
}

std::string PythonConverter::convert(const Sketcher::Constraint* constraint, GeoIdMode geoIdMode)
{
    // addConstraint(Sketcher.Constraint('Distance',%d,%f))
    std::string command;
    auto cg = process(constraint, geoIdMode);

    command = boost::str(boost::format("addConstraint(%s)\n") % cg);

    return command;
}

std::string PythonConverter::convert(
    const std::string& doc,
    const std::vector<Part::Geometry*>& geos,
    Mode mode
)
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
                    boost::format(
                        "constrGeoList = []\n%s%s.addGeometry(constrGeoList,%s)\n"
                        "del constrGeoList\n"
                    )
                    % geolist % doc % "True"
                );
            }
            else {
                command = boost::str(
                    boost::format("geoList = []\n%s%s.addGeometry(geoList,%s)\ndel geoList\n")
                    % geolist % doc % "False"
                );
            }
        }

        return command;
    };

    std::string command = boost::str(boost::format("lastGeoId = len(ActiveSketch.Geometry)\n"));

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
            geolist = boost::str(boost::format("%sconstrGeoList.append(%s)\n") % geolist % sg.creation);
        }
        else {
            geolist = boost::str(boost::format("%sgeoList.append(%s)\n") % geolist % sg.creation);
        }

        ngeos++;
    }

    addToCommands(geolist, ngeos, currentconstruction);

    int index = 0;
    if (mode == Mode::CreateInternalGeometry) {
        for (auto geo : geos) {
            index++;
            // clang-format off: keep line breaks for readability
            if (!geo->is<Part::GeomEllipse>()
                || !geo->is<Part::GeomArcOfEllipse>()
                || !geo->is<Part::GeomArcOfHyperbola>()
                || !geo->is<Part::GeomArcOfParabola>()
                || !geo->is<Part::GeomBSplineCurve>()) {
                std::string newcommand =
                    boost::str(boost::format("exposeInternalGeometry(lastGeoId + %d)\n") % (index));
                command += newcommand;
            }
            // clang-format on
        }
    }

    return command;
}

std::string PythonConverter::convert(
    const std::string& doc,
    const std::vector<Sketcher::Constraint*>& constraints,
    GeoIdMode geoIdMode
)
{
    if (constraints.size() == 1) {
        auto cg = convert(constraints[0], geoIdMode);

        return boost::str(boost::format("%s.%s\n") % doc % cg);
    }

    std::string constraintlist = "constraintList = []";

    for (auto constraint : constraints) {
        auto cg = process(constraint, geoIdMode);

        constraintlist = boost::str(
            boost::format("%s\nconstraintList.append(%s)") % constraintlist % cg
        );
    }

    if (!constraints.empty()) {
        constraintlist = boost::str(
            boost::format("%s\n%s.addConstraint(constraintList)\ndel constraintList\n")
            % constraintlist % doc
        );
    }

    return constraintlist;
}

template<typename T>
std::string makeSplineInfoArrayString(const std::vector<T>& rInfoVec)
{
    std::stringstream stream;
    if constexpr (std::is_same_v<T, Base::Vector3d>) {
        for (const auto& rInfo : rInfoVec) {
            stream << "App.Vector(" << rInfo.x << ", " << rInfo.y << "), ";
        }
    }
    else {
        for (const auto& rInfo : rInfoVec) {
            stream << rInfo << ", ";
        }
    }

    std::string res = stream.str();
    // remove last comma and add brackets
    int index = res.rfind(',');
    res.resize(index);
    return fmt::format("[{}]", res);
    ;
}

PythonConverter::SingleGeometry PythonConverter::process(const Part::Geometry* geo)
{
    static std::map<const Base::Type, std::function<SingleGeometry(const Part::Geometry* geo)>> converterMap
        = {
            {Part::GeomLineSegment::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto sgeo = static_cast<const Part::GeomLineSegment*>(geo);
                 SingleGeometry sg;
                 sg.creation = boost::str(
                     boost::format("Part.LineSegment(App.Vector(%f, %f, %f),App.Vector(%f, %f, %f))")
                     % sgeo->getStartPoint().x % sgeo->getStartPoint().y % sgeo->getStartPoint().z
                     % sgeo->getEndPoint().x % sgeo->getEndPoint().y % sgeo->getEndPoint().z
                 );
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
            {Part::GeomArcOfCircle::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto arc = static_cast<const Part::GeomArcOfCircle*>(geo);
                 double startAngle, endAngle;
                 arc->getRange(startAngle, endAngle, /*emulateCCWXY=*/true);
                 SingleGeometry sg;
                 sg.creation = boost::str(
                     boost::format(
                         "Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f, "
                         "%f), App.Vector(%f, %f, %f), %f), %f, %f)"
                     )
                     % arc->getCenter().x % arc->getCenter().y % arc->getCenter().z
                     % arc->getAxisDirection().x % arc->getAxisDirection().y
                     % arc->getAxisDirection().z % arc->getRadius() % startAngle % endAngle
                 );
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
            {Part::GeomPoint::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto sgeo = static_cast<const Part::GeomPoint*>(geo);
                 SingleGeometry sg;
                 sg.creation = boost::str(
                     boost::format("Part.Point(App.Vector(%f, %f, %f))") % sgeo->getPoint().x
                     % sgeo->getPoint().y % sgeo->getPoint().z
                 );
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
            {Part::GeomEllipse::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto ellipse = static_cast<const Part::GeomEllipse*>(geo);
                 SingleGeometry sg;
                 auto center = ellipse->getCenter();
                 auto periapsis = center + ellipse->getMajorAxisDir() * ellipse->getMajorRadius();
                 auto positiveB = center + ellipse->getMinorAxisDir() * ellipse->getMinorRadius();
                 sg.creation = boost::str(
                     boost::format(
                         "Part.Ellipse(App.Vector(%f, %f, %f), App.Vector(%f, "
                         "%f, %f), App.Vector(%f, %f, %f))"
                     )
                     % periapsis.x % periapsis.y % periapsis.z % positiveB.x % positiveB.y
                     % positiveB.z % center.x % center.y % center.z
                 );
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
            {Part::GeomArcOfEllipse::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto aoe = static_cast<const Part::GeomArcOfEllipse*>(geo);
                 double startAngle, endAngle;
                 aoe->getRange(startAngle, endAngle, /*emulateCCWXY=*/true);
                 SingleGeometry sg;
                 auto center = aoe->getCenter();
                 auto periapsis = center + aoe->getMajorAxisDir() * aoe->getMajorRadius();
                 auto positiveB = center + aoe->getMinorAxisDir() * aoe->getMinorRadius();
                 sg.creation = boost::str(
                     boost::format(
                         "Part.ArcOfEllipse(Part.Ellipse(App.Vector(%f, %f, %f), App.Vector(%f, "
                         "%f, %f), App.Vector(%f, %f, %f)), %f, %f)"
                     )
                     % periapsis.x % periapsis.y % periapsis.z % positiveB.x % positiveB.y
                     % positiveB.z % center.x % center.y % center.z % startAngle % endAngle
                 );
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
            {Part::GeomArcOfHyperbola::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto aoh = static_cast<const Part::GeomArcOfHyperbola*>(geo);
                 double startAngle, endAngle;
                 aoh->getRange(startAngle, endAngle, /*emulateCCWXY=*/true);
                 SingleGeometry sg;
                 auto center = aoh->getCenter();
                 auto majAxisPoint = center + aoh->getMajorAxisDir() * aoh->getMajorRadius();
                 auto minAxisPoint = center + aoh->getMinorAxisDir() * aoh->getMinorRadius();
                 sg.creation = boost::str(
                     boost::format(
                         "Part.ArcOfHyperbola(Part.Hyperbola(App.Vector(%f, %f, %f), "
                         "App.Vector(%f, %f, %f), App.Vector(%f, %f, %f)), %f, %f)"
                     )
                     % majAxisPoint.x % majAxisPoint.y % majAxisPoint.z % minAxisPoint.x
                     % minAxisPoint.y % minAxisPoint.z % center.x % center.y % center.z % startAngle
                     % endAngle
                 );
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
            {Part::GeomArcOfParabola::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto aop = static_cast<const Part::GeomArcOfParabola*>(geo);
                 double startAngle, endAngle;
                 aop->getRange(startAngle, endAngle, /*emulateCCWXY=*/true);
                 SingleGeometry sg;
                 auto focus = aop->getFocus();
                 auto axisPoint = aop->getCenter();
                 sg.creation = boost::str(
                     boost::format(
                         "Part.ArcOfParabola(Part.Parabola(App.Vector(%f, %f, %f), "
                         "App.Vector(%f, %f, %f), App.Vector(0, 0, 1)), %f, %f)"
                     )
                     % focus.x % focus.y % focus.z % axisPoint.x % axisPoint.y % axisPoint.z
                     % startAngle % endAngle
                 );
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
            {Part::GeomBSplineCurve::getClassTypeId(),
             [](const Part::Geometry* geo) {
                 auto bSpline = static_cast<const Part::GeomBSplineCurve*>(geo);

                 std::string controlpoints = makeSplineInfoArrayString(bSpline->getPoles());
                 std::string mults = makeSplineInfoArrayString(bSpline->getMultiplicities());
                 std::string knots = makeSplineInfoArrayString(bSpline->getKnots());
                 std::string weights = makeSplineInfoArrayString(bSpline->getWeights());

                 SingleGeometry sg;
                 sg.creation = boost::str(
                     boost::format("Part.BSplineCurve(%s, %s, %s, %s, %d, %s, False)")
                     % controlpoints.c_str() % mults.c_str() % knots.c_str()
                     % (bSpline->isPeriodic() ? "True" : "False") % bSpline->getDegree()
                     % weights.c_str()
                 );
                 sg.construction = Sketcher::GeometryFacade::getConstruction(geo);
                 return sg;
             }},
            {Part::GeomCircle::getClassTypeId(), [](const Part::Geometry* geo) {
                 auto circle = static_cast<const Part::GeomCircle*>(geo);
                 SingleGeometry sg;
                 sg.creation = boost::str(
                     boost::format("Part.Circle(App.Vector(%f, %f, %f), App.Vector(%f, %f, %f), %f)")
                     % circle->getCenter().x % circle->getCenter().y % circle->getCenter().z
                     % circle->getAxisDirection().x % circle->getAxisDirection().y
                     % circle->getAxisDirection().z % circle->getRadius()
                 );
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

std::string PythonConverter::process(const Sketcher::Constraint* constraint, GeoIdMode geoIdMode)
{
    // Helper to format geometry IDs for Python (handles 'lastGeoId + X' logic)
    auto formatGeoId = [&](int geoId) {
        if (geoId >= 0 && geoIdMode == GeoIdMode::AddLastGeoIdToGeoIds) {
            return "lastGeoId + " + std::to_string(geoId);
        }
        return std::to_string(geoId);
    };

    // Pre-format elements and positions as strings
    std::string id1 = formatGeoId(constraint->First);
    std::string id2 = formatGeoId(constraint->Second);
    std::string id3 = formatGeoId(constraint->Third);

    std::string pos1 = std::to_string(static_cast<int>(constraint->FirstPos));
    std::string pos2 = std::to_string(static_cast<int>(constraint->SecondPos));
    std::string pos3 = std::to_string(static_cast<int>(constraint->ThirdPos));

    std::string id1pos1 = id1 + ", " + pos1;
    std::string id2pos2 = id2 + ", " + pos2;

    std::string val = std::to_string(constraint->getValue());

    // Booleans to simplify the logic branches
    bool secondUndef = (constraint->Second == GeoEnum::GeoUndef);
    bool thirdUndef = (constraint->Third == GeoEnum::GeoUndef);
    bool firstIsEdge = (constraint->FirstPos == Sketcher::PointPos::none);
    bool secondIsEdge = (constraint->SecondPos == Sketcher::PointPos::none);
    bool thirdIsEdge = (constraint->ThirdPos == Sketcher::PointPos::none);

    std::string res;

    switch (constraint->Type) {
        case Sketcher::Coincident:
            res = "Coincident', " + id1pos1 + ", " + id2pos2;
            break;
        case Sketcher::Horizontal:
            if (secondUndef) {
                res = "Horizontal', " + id1;
            }
            else {
                res = "Horizontal', " + id1pos1 + ", " + id2pos2;
            }
            break;
        case Sketcher::Vertical:
            if (secondUndef) {
                res = "Vertical', " + id1;
            }
            else {
                res = "Vertical', " + id1pos1 + ", " + id2pos2;
            }
            break;
        case Sketcher::Block:
            res = "Block', " + id1;
            break;
        case Sketcher::Tangent:
            if (firstIsEdge) {
                res = "Tangent', " + id1 + ", " + id2;
            }
            else if (secondIsEdge) {
                res = "Tangent', " + id1pos1 + ", " + id2;
            }
            else {
                res = "Tangent', " + id1pos1 + ", " + id2pos2;
            }
            break;
        case Sketcher::Parallel:
            res = "Parallel', " + id1 + ", " + id2;
            break;
        case Sketcher::Perpendicular:
            if (firstIsEdge) {
                res = "Perpendicular', " + id1 + ", " + id2;
            }
            else if (secondIsEdge) {
                res = "Perpendicular', " + id1pos1 + ", " + id2;
            }
            else {
                res = "Perpendicular', " + id1pos1 + ", " + id2pos2;
            }
            break;
        case Sketcher::Equal:
            res = "Equal', " + id1 + ", " + id2;
            break;
        case Sketcher::Distance:
            if (secondUndef) {
                res = "Distance', " + id1 + ", " + val;
            }
            else if (firstIsEdge) {
                res = "Distance', " + id1 + ", " + id2 + ", " + val;
            }
            else if (secondIsEdge) {
                res = "Distance', " + id1pos1 + ", " + id2 + ", " + val;
            }
            else {
                res = "Distance', " + id1pos1 + ", " + id2pos2 + ", " + val;
            }
            break;
        case Sketcher::Angle:
            if (secondUndef) {
                res = "Angle', " + id1 + ", " + val;
            }
            else if (thirdUndef) {
                if (secondIsEdge) {
                    res = "Angle', " + id1 + ", " + id2 + ", " + val;
                }
                else {
                    res = "Angle', " + id1pos1 + ", " + id2pos2 + ", " + val;
                }
            }
            else {
                res = "AngleViaPoint', " + id1 + ", " + id2 + ", " + id3 + ", " + pos3 + ", " + val;
            }
            break;
        case Sketcher::DistanceX:
            if (firstIsEdge && secondUndef) {
                res = "DistanceX', " + id1 + ", " + val;
            }
            else if (secondIsEdge) {
                res = "DistanceX', " + id1pos1 + ", " + val;
            }
            else {
                res = "DistanceX', " + id1pos1 + ", " + id2pos2 + ", " + val;
            }
            break;
        case Sketcher::DistanceY:
            if (firstIsEdge && secondUndef) {
                res = "DistanceY', " + id1 + ", " + val;
            }
            else if (secondIsEdge) {
                res = "DistanceY', " + id1pos1 + ", " + val;
            }
            else {
                res = "DistanceY', " + id1pos1 + ", " + id2pos2 + ", " + val;
            }
            break;
        case Sketcher::Radius:
            res = "Radius', " + id1 + ", " + val;
            break;
        case Sketcher::Diameter:
            res = "Diameter', " + id1 + ", " + val;
            break;
        case Sketcher::Weight:
            res = "Weight', " + id1 + ", " + val;
            break;
        case Sketcher::PointOnObject:
            res = "PointOnObject', " + id1pos1 + ", " + id2;
            break;
        case Sketcher::Symmetric:
            if (thirdIsEdge) {
                res = "Symmetric', " + id1pos1 + ", " + id2pos2 + ", " + id3;
            }
            else {
                res = "Symmetric', " + id1pos1 + ", " + id2pos2 + ", " + id3 + ", " + pos3;
            }
            break;
        case Sketcher::SnellsLaw:
            res = "SnellsLaw', " + id1pos1 + ", " + id2pos2 + ", " + id3 + ", " + val;
            break;
        case Sketcher::InternalAlignment: {
            std::string alignType = constraint->internalAlignmentTypeToString();
            if (constraint->AlignmentType == EllipseMajorDiameter
                || constraint->AlignmentType == EllipseMinorDiameter
                || constraint->AlignmentType == HyperbolaMajor
                || constraint->AlignmentType == HyperbolaMinor
                || constraint->AlignmentType == ParabolaFocalAxis) {
                res = "InternalAlignment:" + alignType + "', " + id1 + ", " + id2;
            }
            else if (constraint->AlignmentType == EllipseFocus1
                     || constraint->AlignmentType == EllipseFocus2
                     || constraint->AlignmentType == HyperbolaFocus
                     || constraint->AlignmentType == ParabolaFocus) {
                res = "InternalAlignment:" + alignType + "', " + id1pos1 + ", " + id2;
            }
            else if (constraint->AlignmentType == BSplineControlPoint) {
                res = "InternalAlignment:" + alignType + "', " + id1pos1 + ", " + id2 + ", "
                    + std::to_string(constraint->InternalAlignmentIndex);
            }
            else if (constraint->AlignmentType == BSplineKnotPoint) {
                res = "InternalAlignment:" + alignType + "', " + id1 + ", 1, " + id2 + ", "
                    + std::to_string(constraint->InternalAlignmentIndex);
            }
            else {
                THROWM(Base::ValueError, "PythonConverter: Constraint Alignment Type not supported")
            }
            break;
        }
        case Sketcher::Group:
        case Sketcher::Text: {
            std::string list = "[";
            for (int i = 0; constraint->hasElement(i); ++i) {
                if (i > 0) {
                    list += ", ";
                }
                list += formatGeoId(constraint->getGeoId(i)) + ", "
                    + std::to_string(constraint->getPosIdAsInt(i));
            }
            list += "]";

            if (constraint->Type == Sketcher::Group) {
                res = "Group', " + list;
            }
            else {
                auto escapeForPython = [](const std::string& input) {
                    std::string result;
                    result.reserve(input.length());
                    for (char c : input) {
                        if (c == '\\') {
                            result += "\\\\";
                        }
                        else if (c == '\'') {
                            result += "\\'";
                        }
                        else {
                            result += c;
                        }
                    }
                    return result;
                };

                res = "Text', " + list + ", '" + escapeForPython(constraint->getText()) + "', '"
                    + escapeForPython(constraint->getFont()) + "', "
                    + (constraint->getIsTextHeight() ? "True" : "False");
            }
            break;
        }
        default:
            THROWM(Base::ValueError, "PythonConverter: Constraint Type not supported")
    }

    // Append active/driving flags
    if (!constraint->isActive || !constraint->isDriving) {
        res += constraint->isActive ? ", True" : ", False";
        if (constraint->isDimensional()) {
            res += constraint->isDriving ? ", True" : ", False";
        }
    }

    // Encapsulate everything correctly
    return "Sketcher.Constraint('" + res + ")";
}

std::vector<std::string> PythonConverter::multiLine(std::string&& singlestring)
{
    std::vector<std::string> tokens;
    split_regex(tokens, singlestring, boost::regex("(\n)+"));
    return tokens;
}

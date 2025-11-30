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
    bool addLastIdVar = geoIdMode == GeoIdMode::AddLastGeoIdToGeoIds;
    bool addLastIdVar1 = constraint->First >= 0 && addLastIdVar;
    bool addLastIdVar2 = constraint->Second >= 0 && addLastIdVar;
    bool addLastIdVar3 = constraint->Third >= 0 && addLastIdVar;

    std::string geoId1 = (addLastIdVar1 ? "lastGeoId + " : "") + std::to_string(constraint->First);
    std::string geoId2 = (addLastIdVar2 ? "lastGeoId + " : "") + std::to_string(constraint->Second);
    std::string geoId3 = (addLastIdVar3 ? "lastGeoId + " : "") + std::to_string(constraint->Third);


    static std::map<
        const Sketcher::ConstraintType,
        std::function<std::string(const Sketcher::Constraint*, std::string&, std::string&, std::string&)>>
        converterMap = {
            {Sketcher::Coincident,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 return boost::str(
                     boost::format("Sketcher.Constraint('Coincident', %s, %i, %s, %i") % geoId1
                     % static_cast<int>(constr->FirstPos) % geoId2
                     % static_cast<int>(constr->SecondPos)
                 );
             }},
            {Sketcher::Horizontal,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 if (constr->Second == GeoEnum::GeoUndef) {
                     return boost::str(boost::format("Sketcher.Constraint('Horizontal', %s") % geoId1);
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Horizontal', %s, %i, %s, %i") % geoId1
                         % static_cast<int>(constr->FirstPos) % geoId2
                         % static_cast<int>(constr->SecondPos)
                     );
                 }
             }},
            {Sketcher::Vertical,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 if (constr->Second == GeoEnum::GeoUndef) {
                     return boost::str(boost::format("Sketcher.Constraint('Vertical', %s") % geoId1);
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Vertical', %s, %i, %s, %i") % geoId1
                         % static_cast<int>(constr->FirstPos) % geoId2
                         % static_cast<int>(constr->SecondPos)
                     );
                 }
             }},
            {Sketcher::Block,
             []([[maybe_unused]] const Sketcher::Constraint* constr,
                std::string& geoId1,
                [[maybe_unused]] std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 return boost::str(boost::format("Sketcher.Constraint('Block', %s") % geoId1);
             }},
            {Sketcher::Tangent,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 if (constr->FirstPos == Sketcher::PointPos::none) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Tangent', %s, %s") % geoId1 % geoId2
                     );
                 }
                 else if (constr->SecondPos == Sketcher::PointPos::none) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Tangent', %s, %i, %s") % geoId1
                         % static_cast<int>(constr->FirstPos) % geoId2
                     );
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Tangent', %s, %i, %s, %i") % geoId1
                         % static_cast<int>(constr->FirstPos) % geoId2
                         % static_cast<int>(constr->SecondPos)
                     );
                 }
             }},
            {Sketcher::Parallel,
             []([[maybe_unused]] const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 return boost::str(
                     boost::format("Sketcher.Constraint('Parallel', %s, %s") % geoId1 % geoId2
                 );
             }},
            {Sketcher::Perpendicular,
             []([[maybe_unused]] const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 if (constr->FirstPos == Sketcher::PointPos::none) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Perpendicular', %s, %s") % geoId1 % geoId2
                     );
                 }
                 else if (constr->SecondPos == Sketcher::PointPos::none) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Perpendicular', %s, %i, %s") % geoId1
                         % static_cast<int>(constr->FirstPos) % geoId2
                     );
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Perpendicular', %s, %i, %s, %i")
                         % geoId1 % static_cast<int>(constr->FirstPos) % geoId2
                         % static_cast<int>(constr->SecondPos)
                     );
                 }
             }},
            {Sketcher::Equal,
             []([[maybe_unused]] const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 return boost::str(
                     boost::format("Sketcher.Constraint('Equal', %s, %s") % geoId1 % geoId2
                 );
             }},
            {Sketcher::InternalAlignment,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 if (constr->AlignmentType == EllipseMajorDiameter
                     || constr->AlignmentType == EllipseMinorDiameter
                     || constr->AlignmentType == HyperbolaMajor
                     || constr->AlignmentType == HyperbolaMinor
                     || constr->AlignmentType == ParabolaFocalAxis) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('InternalAlignment:%s', %s, %s")
                         % constr->internalAlignmentTypeToString() % geoId1 % geoId2
                     );
                 }
                 else if (constr->AlignmentType == EllipseFocus1
                          || constr->AlignmentType == EllipseFocus2
                          || constr->AlignmentType == HyperbolaFocus
                          || constr->AlignmentType == ParabolaFocus) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('InternalAlignment:%s', %s, %i, %s")
                         % constr->internalAlignmentTypeToString() % geoId1
                         % static_cast<int>(constr->FirstPos) % geoId2
                     );
                 }
                 else if (constr->AlignmentType == BSplineControlPoint) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('InternalAlignment:%s', %s, %i, %s, %i")
                         % constr->internalAlignmentTypeToString() % geoId1
                         % static_cast<int>(constr->FirstPos) % geoId2 % constr->InternalAlignmentIndex
                     );
                 }
                 else if (constr->AlignmentType == BSplineKnotPoint) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('InternalAlignment:%s', %s, 1, %s, %i")
                         % constr->internalAlignmentTypeToString() % geoId1 % geoId2
                         % constr->InternalAlignmentIndex
                     );
                 }

                 THROWM(Base::ValueError, "PythonConverter: Constraint Alignment Type not supported")
             }},
            {Sketcher::Distance,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 if (constr->Second == GeoEnum::GeoUndef) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Distance', %s, %f") % geoId1
                         % constr->getValue()
                     );
                 }
                 else if (constr->FirstPos == Sketcher::PointPos::none) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Distance', %s, %s, %f") % geoId1
                         % geoId2 % constr->getValue()
                     );
                 }
                 else if (constr->SecondPos == Sketcher::PointPos::none) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Distance', %s, %i, %s, %f") % geoId1
                         % static_cast<int>(constr->FirstPos) % geoId2 % constr->getValue()
                     );
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Distance', %s, %i, %s, %i, %f")
                         % geoId1 % static_cast<int>(constr->FirstPos) % geoId2
                         % static_cast<int>(constr->SecondPos) % constr->getValue()
                     );
                 }
             }},
            {Sketcher::Angle,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                std::string& geoId3) {
                 if (constr->Second == GeoEnum::GeoUndef) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Angle', %s, %f") % geoId1
                         % constr->getValue()
                     );
                 }
                 else if (constr->Third == GeoEnum::GeoUndef) {
                     if (constr->SecondPos == Sketcher::PointPos::none) {
                         return boost::str(
                             boost::format("Sketcher.Constraint('Angle', %s, %s, %f") % geoId1
                             % geoId2 % constr->getValue()
                         );
                     }
                     else {
                         return boost::str(
                             boost::format("Sketcher.Constraint('Angle', %s, %i, %s, %i, %f")
                             % geoId1 % static_cast<int>(constr->FirstPos) % geoId2
                             % static_cast<int>(constr->SecondPos) % constr->getValue()
                         );
                     }
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('AngleViaPoint', %s, %s, %s, %i, %f")
                         % geoId1 % geoId2 % geoId3 % static_cast<int>(constr->ThirdPos)
                         % constr->getValue()
                     );
                 }
             }},
            {Sketcher::DistanceX,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 if (constr->FirstPos == Sketcher::PointPos::none
                     && constr->Second == GeoEnum::GeoUndef) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('DistanceX', %s, %f") % geoId1
                         % constr->getValue()
                     );
                 }
                 else if (constr->SecondPos == Sketcher::PointPos::none) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('DistanceX', %s, %i, %f") % geoId1
                         % static_cast<int>(constr->FirstPos) % constr->getValue()
                     );
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('DistanceX', %s, %i, %s, %i, %f")
                         % geoId1 % static_cast<int>(constr->FirstPos) % geoId2
                         % static_cast<int>(constr->SecondPos) % constr->getValue()
                     );
                 }
             }},
            {Sketcher::DistanceY,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 if (constr->FirstPos == Sketcher::PointPos::none
                     && constr->Second == GeoEnum::GeoUndef) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('DistanceY', %s, %f") % geoId1
                         % constr->getValue()
                     );
                 }
                 else if (constr->SecondPos == Sketcher::PointPos::none) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('DistanceY', %s, %i, %f") % geoId1
                         % static_cast<int>(constr->FirstPos) % constr->getValue()
                     );
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('DistanceY', %s, %i, %s, %i, %f")
                         % geoId1 % static_cast<int>(constr->FirstPos) % geoId2
                         % static_cast<int>(constr->SecondPos) % constr->getValue()
                     );
                 }
             }},
            {Sketcher::Radius,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                [[maybe_unused]] std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 return boost::str(
                     boost::format("Sketcher.Constraint('Radius', %s, %f") % geoId1
                     % constr->getValue()
                 );
             }},
            {Sketcher::Diameter,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                [[maybe_unused]] std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 return boost::str(
                     boost::format("Sketcher.Constraint('Diameter', %s, %f") % geoId1
                     % constr->getValue()
                 );
             }},
            {Sketcher::Weight,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                [[maybe_unused]] std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 return boost::str(
                     boost::format("Sketcher.Constraint('Weight', %s, %f") % geoId1
                     % constr->getValue()
                 );
             }},
            {Sketcher::PointOnObject,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                [[maybe_unused]] std::string& geoId3) {
                 return boost::str(
                     boost::format("Sketcher.Constraint('PointOnObject', %s, %i, %s") % geoId1
                     % static_cast<int>(constr->FirstPos) % geoId2
                 );
             }},
            {Sketcher::Symmetric,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                std::string& geoId3) {
                 if (constr->ThirdPos == Sketcher::PointPos::none) {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Symmetric', %s, %i, %s, %i, %s")
                         % geoId1 % static_cast<int>(constr->FirstPos) % geoId2
                         % static_cast<int>(constr->SecondPos) % geoId3
                     );
                 }
                 else {
                     return boost::str(
                         boost::format("Sketcher.Constraint('Symmetric', %s, %i, %s, %i, %s, %i")
                         % geoId1 % static_cast<int>(constr->FirstPos) % geoId2
                         % static_cast<int>(constr->SecondPos) % geoId3
                         % static_cast<int>(constr->ThirdPos)
                     );
                 }
             }},
            {Sketcher::SnellsLaw,
             [](const Sketcher::Constraint* constr,
                std::string& geoId1,
                std::string& geoId2,
                std::string& geoId3) {
                 return boost::str(
                     boost::format("Sketcher.Constraint('SnellsLaw', %s, %i, %s, %i, %s, %f")
                     % geoId1 % static_cast<int>(constr->FirstPos) % geoId2
                     % static_cast<int>(constr->SecondPos) % geoId3 % constr->getValue()
                 );
             }},
        };

    auto result = converterMap.find(constraint->Type);

    if (result == converterMap.end()) {
        THROWM(Base::ValueError, "PythonConverter: Constraint Type not supported")
    }

    auto creator = result->second;
    std::string resultStr = creator(constraint, geoId1, geoId2, geoId3);

    if (!constraint->isActive || !constraint->isDriving) {
        std::string active = constraint->isActive ? "True" : "False";
        resultStr += ", " + active;
        if (constraint->isDimensional()) {
            std::string driving = constraint->isDriving ? "True" : "False";
            resultStr += ", " + driving;
        }
    }
    resultStr += ")";

    return resultStr;
}

std::vector<std::string> PythonConverter::multiLine(std::string&& singlestring)
{
    std::vector<std::string> tokens;
    split_regex(tokens, singlestring, boost::regex("(\n)+"));
    return tokens;
}

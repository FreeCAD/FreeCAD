// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>   *
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


#ifndef SKETCHERGUI_DrawSketchHandlerOffset_H
#define SKETCHERGUI_DrawSketchHandlerOffset_H

#include <FCConfig.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include <QApplication>

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepOffsetAPI_MakeOffset.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp.hxx>
#include <gp_Pln.hxx>

#include <Base/Exception.h>

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include <Mod/Sketcher/App/GeometryFacade.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"


using namespace Sketcher;

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerOffset;

namespace ConstructionMethods
{

enum class OffsetConstructionMethod
{
    Arc,
    // Tangent,
    Intersection,
    End  // Must be the last one
};

/* OCC offer various modes as follows, but we use only Arc and Intersection as the rest are buggy.
enum class JoinMode {
    Arc,
    Tangent,
    Intersection
};
//We use Pipe by default only. Skin is buggy.
enum class ModeEnums {
    Skin,
    Pipe,
    RectoVerso
};*/
}  // namespace ConstructionMethods

using DSHOffsetController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerOffset,
    StateMachines::OneSeekEnd,
    /*PAutoConstraintSize =*/0,
    /*OnViewParametersT =*/OnViewParameters<1, 1>,
    /*WidgetParametersT =*/WidgetParameters<0, 0>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<2, 2>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::OffsetConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHOffsetControllerBase = DSHOffsetController::ControllerBase;

using DrawSketchHandlerOffsetBase = DrawSketchControllableHandler<DSHOffsetController>;

class DrawSketchHandlerOffset: public DrawSketchHandlerOffsetBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerOffset)

    friend DSHOffsetController;
    friend DSHOffsetControllerBase;

public:
    DrawSketchHandlerOffset(
        std::vector<int> listOfGeoIds,
        ConstructionMethod constrMethod = ConstructionMethod::Arc
    )
        : DrawSketchHandlerOffsetBase(constrMethod)
        , listOfGeoIds(listOfGeoIds)
        , deleteOriginal(false)
        , offsetLengthSet(false)
        , offsetConstraint(false)
        , onlySingleLines(true)
        , offsetLength(1.)
    {}

    ~DrawSketchHandlerOffset() override = default;


private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        if (state() == SelectMode::SeekFirst) {
            endpoint = onSketchPos;

            if (!offsetLengthSet) {
                findOffsetLength();
                toolWidgetManager.drawDoubleAtCursor(onSketchPos, offsetLength);
            }

            if (fabs(offsetLength) > Precision::Confusion()) {
                drawOffsetPreview();
            }
        }
    }

    void executeCommands() override
    {
        if (fabs(offsetLength) > Precision::Confusion()) {
            createOffset();
        }
    }

    std::string getToolName() const override
    {
        return "DSH_Offset";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Create_Offset");
    }

    std::unique_ptr<QWidget> createWidget() const override
    {
        return std::make_unique<SketcherToolDefaultWidget>();
    }

    bool isWidgetVisible() const override
    {
        return true;
    };

    QPixmap getToolIcon() const override
    {
        return Gui::BitmapFactory().pixmap("Sketcher_Offset");
    }

    QString getToolWidgetText() const override
    {
        return QString(tr("Offset Parameters"));
    }

    void activated() override
    {
        DrawSketchDefaultHandler::activated();
        continuousMode = false;
        firstCurveCreated = getHighestCurveIndex() + 1;

        collectBranchPoints();
        generateSourceWires();
    }

public:
    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;

        return {
            {tr("%1 set offset direction and distance", "Sketcher Offset: hint"), {MouseLeft}},
        };
    }

private:
    class CoincidencePointPos
    {
    public:
        PointPos firstPos1;
        PointPos secondPos1;
        PointPos firstPos2;
        PointPos secondPos2;
    };

    std::vector<int> listOfGeoIds;
    std::vector<std::vector<int>> vCC;
    std::vector<std::vector<int>> vCCO;
    Base::Vector2d endpoint, pointOnSourceWire;
    std::vector<TopoDS_Wire> sourceWires;
    std::vector<Base::Vector3d> branchPoints;  // Points where 3+ edges meet

    bool deleteOriginal, offsetLengthSet, offsetConstraint, onlySingleLines;
    double offsetLength;
    int firstCurveCreated;

    TopoDS_Shape makeOffsetShape(bool allowOpenResult = false)
    {
        // in OCC the JointTypes are : Arc(0), Tangent(1), Intersection(2)
        short joinType = constructionMethod() == DrawSketchHandlerOffset::ConstructionMethod::Arc
            ? 0
            : 2;

        // Offset will fail for single lines if we don't set a plane in ctor.
        // But if we set a plane, then the direction of offset is forced...
        // so we set a plane if and only if there are not a single sourceWires with more than single
        // line.

        // For branching wires, compute offset geometry manually.
        // Each edge is offset once (to exterior), then wedge connections are computed.
        if (onlySingleLines && sourceWires.size() > 1 && !branchPoints.empty()) {
            TopoDS_Compound compound;
            BRep_Builder builder;
            builder.MakeCompound(compound);

            double tolerance = Precision::Confusion() * 10;
            double absOffset = fabs(offsetLength);

            // Track unique geometry by endpoints
            std::vector<std::tuple<Base::Vector3d, Base::Vector3d>> addedLines;
            std::vector<std::tuple<Base::Vector3d, double, double, double>> addedArcs;

            auto lineExists = [&](const Base::Vector3d& p1, const Base::Vector3d& p2) {
                for (const auto& [a, b] : addedLines) {
                    if (((p1 - a).Length() < tolerance && (p2 - b).Length() < tolerance)
                        || ((p1 - b).Length() < tolerance && (p2 - a).Length() < tolerance)) {
                        return true;
                    }
                }
                return false;
            };

            auto addLineEdge = [&](const Base::Vector3d& p1, const Base::Vector3d& p2) {
                if ((p1 - p2).Length() < tolerance) {
                    return;
                }
                if (lineExists(p1, p2)) {
                    return;
                }

                gp_Pnt gp1(p1.x, p1.y, p1.z);
                gp_Pnt gp2(p2.x, p2.y, p2.z);
                TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(gp1, gp2).Edge();
                builder.Add(compound, edge);
                addedLines.push_back({p1, p2});
            };

            auto arcExists =
                [&](const Base::Vector3d& center, double radius, double startAngle, double endAngle) {
                    for (const auto& [c, r, s, e] : addedArcs) {
                        if ((center - c).Length() < tolerance && fabs(radius - r) < tolerance) {
                            auto norm = [](double a) {
                                while (a < 0) {
                                    a += 2 * M_PI;
                                }
                                while (a >= 2 * M_PI) {
                                    a -= 2 * M_PI;
                                }
                                return a;
                            };
                            double ns = norm(startAngle), ne = norm(endAngle);
                            double os = norm(s), oe = norm(e);
                            if ((fabs(ns - os) < 0.1 && fabs(ne - oe) < 0.1)
                                || (fabs(ns - oe) < 0.1 && fabs(ne - os) < 0.1)) {
                                return true;
                            }
                        }
                    }
                    return false;
                };

            auto addArcEdge =
                [&](const Base::Vector3d& center, double radius, double startAngle, double endAngle) {
                    if (fabs(endAngle - startAngle) < 0.01) {
                        return;
                    }
                    if (arcExists(center, radius, startAngle, endAngle)) {
                        return;
                    }

                    gp_Pnt c(center.x, center.y, center.z);
                    gp_Circ circ(gp_Ax2(c, gp::DZ()), radius);
                    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(circ, startAngle, endAngle).Edge();
                    builder.Add(compound, edge);
                    addedArcs.push_back({center, radius, startAngle, endAngle});
                };

            auto lineIntersection = [](const Base::Vector3d& p1,
                                       const Base::Vector3d& d1,
                                       const Base::Vector3d& p2,
                                       const Base::Vector3d& d2,
                                       Base::Vector3d& intersection) -> bool {
                double denom = d1.x * d2.y - d1.y * d2.x;
                if (fabs(denom) < 1e-10) {
                    return false;
                }
                double t = ((p2.x - p1.x) * d2.y - (p2.y - p1.y) * d2.x) / denom;
                intersection = p1 + d1 * t;
                return true;
            };

            // Track which outer endpoints need semicircles vs partial arcs
            // A geoId can have MULTIPLE arc ranges (from different wedges)
            std::map<int, std::vector<std::pair<double, double>>> outerArcRanges;  // geoId -> list
                                                                                   // of (startAngle,
                                                                                   // endAngle) arcs
                                                                                   // already drawn

            for (const auto& branchPt : branchPoints) {
                std::vector<int> sortedEdges = getEdgesSortedByAngle(branchPt);
                size_t numEdges = sortedEdges.size();
                if (numEdges < 2) {
                    continue;
                }

                for (size_t i = 0; i < numEdges; i++) {
                    int geoId1 = sortedEdges[i];
                    int geoId2 = sortedEdges[(i + 1) % numEdges];

                    Base::Vector3d e1p1, e1p2, e2p1, e2p2;
                    getFirstSecondPoints(geoId1, e1p1, e1p2);
                    getFirstSecondPoints(geoId2, e2p1, e2p2);

                    Base::Vector3d e1Outer = ((e1p1 - branchPt).Length() < tolerance) ? e1p2 : e1p1;
                    Base::Vector3d e2Outer = ((e2p1 - branchPt).Length() < tolerance) ? e2p2 : e2p1;

                    Base::Vector3d dir1 = (e1Outer - branchPt).Normalize();
                    Base::Vector3d dir2 = (e2Outer - branchPt).Normalize();

                    Base::Vector3d perp1(-dir1.y, dir1.x, 0);
                    Base::Vector3d perp2(dir2.y, -dir2.x, 0);

                    Base::Vector3d off1Start = branchPt + perp1 * absOffset;
                    Base::Vector3d off1End = e1Outer + perp1 * absOffset;
                    Base::Vector3d off2Start = branchPt + perp2 * absOffset;
                    Base::Vector3d off2End = e2Outer + perp2 * absOffset;

                    Base::Vector3d intersection;
                    bool intFound = lineIntersection(off1Start, dir1, off2Start, dir2, intersection);
                    if (intFound) {
                        double len1 = (e1Outer - branchPt).Length();
                        double len2 = (e2Outer - branchPt).Length();
                        double t1 = (intersection - off1Start).Dot(dir1) / len1;
                        double t2 = (intersection - off2Start).Dot(dir2) / len2;

                        if (t1 > 0 && t1 < 1 && t2 > 0 && t2 < 1) {
                            // Concave: both lines trimmed to intersection
                            addLineEdge(off1End, intersection);
                            addLineEdge(intersection, off2End);
                        }
                        else if (t1 >= 1 && t2 >= 1) {
                            // Large offset case: offset lines don't intersect within the wedge.
                            // TODO: Proper handling of large offsets at branch points requires
                            // connecting via circle-circle intersections. For now, we skip the
                            // wedge connection and let each semicircle be drawn independently.
                            // This results in disconnected geometry at large offsets but avoids
                            // creating interior-crossing lines.
                        }
                        else if (t1 <= 0 && t2 <= 0) {
                            // Both convex: full lines + arc at branch
                            addLineEdge(off1End, off1Start);
                            addLineEdge(off2Start, off2End);
                            if (joinType == 0) {
                                double angle1 = atan2(perp1.y, perp1.x);
                                double angle2 = atan2(perp2.y, perp2.x);
                                while (angle2 < angle1) {
                                    angle2 += 2 * M_PI;
                                }
                                if (angle2 - angle1 > M_PI) {
                                    double temp = angle1;
                                    angle1 = angle2;
                                    angle2 = temp + 2 * M_PI;
                                }
                                addArcEdge(branchPt, absOffset, angle1, angle2);
                            }
                        }
                        else if (t1 > 0 && t1 < 1 && t2 >= 1) {
                            // Mixed: t1 valid (concave), t2 beyond
                            // Draw trimmed line from off1End to intersection
                            // The semicircle at e2Outer will provide the connection on that side
                            addLineEdge(off1End, intersection);
                        }
                        else if (t2 > 0 && t2 < 1 && t1 >= 1) {
                            // Mixed: t2 valid (concave), t1 beyond
                            // Draw trimmed line from intersection to off2End
                            // The semicircle at e1Outer will provide the connection on that side
                            addLineEdge(intersection, off2End);
                        }
                        else if (t1 <= 0 && t2 >= 1) {
                            // Edge 1 convex (intersection behind branch), edge 2 beyond
                            // Draw full line for edge 1, let semicircles handle the rest
                            addLineEdge(off1End, off1Start);
                        }
                        else if (t2 <= 0 && t1 >= 1) {
                            // Edge 2 convex, edge 1 beyond
                            // Draw full line for edge 2, let semicircles handle the rest
                            addLineEdge(off2Start, off2End);
                        }
                        else if (t1 <= 0 && t2 > 0 && t2 < 1) {
                            // Edge 1 convex, edge 2 valid (concave)
                            addLineEdge(off1End, off1Start);
                            addLineEdge(intersection, off2End);
                            if (joinType == 0) {
                                double angle1 = atan2(perp1.y, perp1.x);
                                double angle2
                                    = atan2((intersection - branchPt).y, (intersection - branchPt).x);
                                while (angle2 < angle1) {
                                    angle2 += 2 * M_PI;
                                }
                                if (angle2 - angle1 > M_PI) {
                                    std::swap(angle1, angle2);
                                    while (angle2 < angle1) {
                                        angle2 += 2 * M_PI;
                                    }
                                }
                                addArcEdge(branchPt, absOffset, angle1, angle2);
                            }
                        }
                        else if (t2 <= 0 && t1 > 0 && t1 < 1) {
                            // Edge 2 convex, edge 1 valid (concave)
                            addLineEdge(off1End, intersection);
                            addLineEdge(off2Start, off2End);
                            if (joinType == 0) {
                                double angle1
                                    = atan2((intersection - branchPt).y, (intersection - branchPt).x);
                                double angle2 = atan2(perp2.y, perp2.x);
                                while (angle2 < angle1) {
                                    angle2 += 2 * M_PI;
                                }
                                if (angle2 - angle1 > M_PI) {
                                    std::swap(angle1, angle2);
                                    while (angle2 < angle1) {
                                        angle2 += 2 * M_PI;
                                    }
                                }
                                addArcEdge(branchPt, absOffset, angle1, angle2);
                            }
                        }
                    }
                }

                // Add semicircular arcs at outer endpoints
                for (int geoId : sortedEdges) {
                    Base::Vector3d p1, p2;
                    getFirstSecondPoints(geoId, p1, p2);
                    Base::Vector3d outer = ((p1 - branchPt).Length() < tolerance) ? p2 : p1;

                    bool isShared = false;
                    for (int otherGeoId : listOfGeoIds) {
                        if (otherGeoId == geoId) {
                            continue;
                        }
                        Base::Vector3d op1, op2;
                        if (getFirstSecondPoints(otherGeoId, op1, op2)) {
                            if ((outer - op1).Length() < tolerance
                                || (outer - op2).Length() < tolerance) {
                                isShared = true;
                                break;
                            }
                        }
                    }

                    if (!isShared) {
                        Base::Vector3d dir = (outer - branchPt).Normalize();
                        double centerAngle = atan2(dir.y, dir.x);
                        double startAngle = centerAngle - M_PI / 2;
                        double endAngle = centerAngle + M_PI / 2;

                        // If this outer has partial arc coverage, compute complement
                        if (outerArcRanges.count(geoId) && !outerArcRanges[geoId].empty()) {
                            const auto& drawnArcs = outerArcRanges[geoId];

                            // Normalize angles relative to the semicircle range
                            auto normalizeToRange = [](double angle, double refAngle) {
                                while (angle < refAngle - M_PI) {
                                    angle += 2 * M_PI;
                                }
                                while (angle > refAngle + M_PI) {
                                    angle -= 2 * M_PI;
                                }
                                return angle;
                            };

                            // Collect all drawn arc boundaries, normalized
                            std::vector<std::pair<double, double>> normalizedArcs;
                            for (const auto& [ds, de] : drawnArcs) {
                                double normStart = normalizeToRange(ds, centerAngle);
                                double normEnd = normalizeToRange(de, centerAngle);
                                // Ensure start < end
                                if (normStart > normEnd) {
                                    std::swap(normStart, normEnd);
                                }
                                normalizedArcs.push_back({normStart, normEnd});
                            }

                            // Sort by start angle
                            std::sort(normalizedArcs.begin(), normalizedArcs.end());

                            // Find gaps in coverage and draw complement arcs
                            double currentPos = startAngle;
                            double angleTol = 0.05;

                            for (const auto& [arcStart, arcEnd] : normalizedArcs) {
                                // Gap before this arc?
                                if (arcStart > currentPos + angleTol) {
                                    addArcEdge(outer, absOffset, currentPos, arcStart);
                                }
                                // Move past this arc
                                if (arcEnd > currentPos) {
                                    currentPos = arcEnd;
                                }
                            }

                            // Gap after all arcs to end of semicircle?
                            if (currentPos < endAngle - angleTol) {
                                addArcEdge(outer, absOffset, currentPos, endAngle);
                            }
                        }
                        else {
                            addArcEdge(outer, absOffset, startAngle, endAngle);
                        }
                    }
                }
            }

            if (compound.NbChildren() == 0) {
                return TopoDS_Shape();
            }

            return BRepBuilderAPI_Copy(compound).Shape();
        }

        // Original behavior for non-branching cases
        BRepOffsetAPI_MakeOffset mkOffset;

        if (onlySingleLines) {
            TopoDS_Face workingPlane = BRepBuilderAPI_MakeFace(gp_Pln(gp::Origin(), gp::DZ()));
            mkOffset = BRepOffsetAPI_MakeOffset(workingPlane);
        }
        mkOffset.Init(GeomAbs_JoinType(joinType), allowOpenResult);

        for (TopoDS_Wire& wire : sourceWires) {
            mkOffset.AddWire(wire);
        }
        try {
#if defined(__GNUC__) && defined(FC_OS_LINUX)
            Base::SignalException se;
#endif
            mkOffset.Perform(offsetLength);
        }
        catch (Standard_Failure&) {
            throw;
        }
        catch (...) {
            throw Base::CADKernelError(
                "BRepOffsetAPI_MakeOffset has crashed! (Unknown exception caught)"
            );
        }

        TopoDS_Shape offsetShape = mkOffset.Shape();

        if (offsetShape.IsNull()) {
            return offsetShape;
        }

        // Copying shape to fix strange orientation behavior, OCC7.0.0. See bug #2699
        //  http://www.freecad.org/tracker/view.php?id=2699
        offsetShape = BRepBuilderAPI_Copy(offsetShape).Shape();
        return offsetShape;
    }

    Part::Geometry* curveToLine(BRepAdaptor_Curve curve)
    {
        double first = curve.FirstParameter();
        if (fabs(first) > 1E99) {
            first = -10000;
        }

        double last = curve.LastParameter();
        if (fabs(last) > 1E99) {
            last = +10000;
        }

        gp_Pnt P1 = curve.Value(first);
        gp_Pnt P2 = curve.Value(last);

        Base::Vector3d p1(P1.X(), P1.Y(), P1.Z());
        Base::Vector3d p2(P2.X(), P2.Y(), P2.Z());
        auto* line = new Part::GeomLineSegment();
        line->setPoints(p1, p2);
        GeometryFacade::setConstruction(line, false);
        return line;
    }

    Part::Geometry* curveToCircleOrArc(BRepAdaptor_Curve curve, const TopoDS_Edge& /*edge*/)
    {
        gp_Circ circle = curve.Circle();
        gp_Pnt cnt = circle.Location();
        gp_Pnt beg = curve.Value(curve.FirstParameter());
        gp_Pnt end = curve.Value(curve.LastParameter());

        if (beg.SquareDistance(end) < Precision::Confusion()) {
            auto* gCircle = new Part::GeomCircle();
            gCircle->setRadius(circle.Radius());
            gCircle->setCenter(Base::Vector3d(cnt.X(), cnt.Y(), cnt.Z()));

            GeometryFacade::setConstruction(gCircle, false);
            return gCircle;
        }
        else {
            Handle(Geom_Circle) hCircle = new Geom_Circle(circle);

            double u1 = curve.FirstParameter();
            double u2 = curve.LastParameter();

            auto* gArc = new Part::GeomArcOfCircle();
            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(hCircle, u1, u2);
            gArc->setHandle(tCurve);

            gArc->reverseIfReversed();

            GeometryFacade::setConstruction(gArc, false);
            return gArc;
        }
    }

    Part::Geometry* curveToEllipseOrArc(BRepAdaptor_Curve curve, const TopoDS_Edge& /*edge*/)
    {
        gp_Elips ellipse = curve.Ellipse();
        gp_Pnt beg = curve.Value(curve.FirstParameter());
        gp_Pnt end = curve.Value(curve.LastParameter());

        if (beg.SquareDistance(end) < Precision::Confusion()) {
            auto* gEllipse = new Part::GeomEllipse();
            Handle(Geom_Ellipse) hEllipse = new Geom_Ellipse(ellipse);

            gEllipse->setHandle(hEllipse);

            gEllipse->reverseIfReversed();

            GeometryFacade::setConstruction(gEllipse, false);
            return gEllipse;
        }
        else {
            Handle(Geom_Ellipse) hEllipse = new Geom_Ellipse(ellipse);

            double u1 = curve.FirstParameter();
            double u2 = curve.LastParameter();

            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(hEllipse, u1, u2);
            auto* gArc = new Part::GeomArcOfEllipse();
            gArc->setHandle(tCurve);

            gArc->reverseIfReversed();

            GeometryFacade::setConstruction(gArc, false);
            return gArc;
        }
    }

    void getOffsetGeos(std::vector<Part::Geometry*>& geometriesToAdd, std::vector<int>& listOfOffsetGeoIds)
    {
        TopoDS_Shape offsetShape = makeOffsetShape();
        if (offsetShape.IsNull()) {
            return;
        }

        TopExp_Explorer expl(offsetShape, TopAbs_EDGE);
        int geoIdToAdd = firstCurveCreated;
        for (; expl.More(); expl.Next(), geoIdToAdd++) {
            const TopoDS_Edge& edge = TopoDS::Edge(expl.Current());
            BRepAdaptor_Curve curve(edge);
            if (curve.GetType() == GeomAbs_Line) {
                geometriesToAdd.push_back(curveToLine(curve));
                listOfOffsetGeoIds.push_back(geoIdToAdd);
            }
            else if (curve.GetType() == GeomAbs_Circle) {
                geometriesToAdd.push_back(curveToCircleOrArc(curve, edge));
                listOfOffsetGeoIds.push_back(geoIdToAdd);
            }
            else if (curve.GetType() == GeomAbs_Ellipse) {
                geometriesToAdd.push_back(curveToEllipseOrArc(curve, edge));
                listOfOffsetGeoIds.push_back(geoIdToAdd);
            }
            // TODO Bspline support
        }
    }

    void drawOffsetPreview()
    {
        std::vector<Part::Geometry*> geometriesToAdd;
        std::vector<int> listOfOffsetGeoIds;
        getOffsetGeos(geometriesToAdd, listOfOffsetGeoIds);

        drawEdit(geometriesToAdd);
    }

    void createOffset()
    {
        std::vector<Part::Geometry*> geometriesToAdd;
        std::vector<int> listOfOffsetGeoIds;
        getOffsetGeos(geometriesToAdd, listOfOffsetGeoIds);

        SketchObject* Obj = sketchgui->getSketchObject();

        if (listOfOffsetGeoIds.empty()) {
            Gui::NotifyUserError(
                Obj,
                QT_TRANSLATE_NOOP("Notifications", "Offset Error"),
                QT_TRANSLATE_NOOP("Notifications", "Offset could not be created.")
            );
            return;
        }

        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Offset"));

        // Create geos
        Obj->addGeometry(std::move(geometriesToAdd));

        // Create coincident (& tangent) constraints
        jointOffsetCurves(listOfOffsetGeoIds);

        if (deleteOriginal) {
            deleteOriginalGeometries();
        }
        else if (offsetConstraint) {
            makeOffsetConstraint(listOfOffsetGeoIds);
        }

        Gui::Command::commitCommand();
    }

    void jointOffsetCurves(std::vector<int>& listOfOffsetGeoIds)
    {
        if (listOfOffsetGeoIds.empty()) {
            return;
        }
        std::stringstream stream;
        stream << "conList = []\n";
        for (size_t i = 0; i < listOfOffsetGeoIds.size() - 1; i++) {
            for (size_t j = i + 1; j < listOfOffsetGeoIds.size(); j++) {
                // There's a bug with created arcs. They end points seems to swap at some point...
                // Here we make coincidences based on distance. So they must change after.
                Base::Vector3d firstStartPoint, firstEndPoint, secondStartPoint, secondEndPoint;
                if (!getFirstSecondPoints(listOfOffsetGeoIds[i], firstStartPoint, firstEndPoint)
                    || !getFirstSecondPoints(listOfOffsetGeoIds[j], secondStartPoint, secondEndPoint)) {
                    continue;
                }

                bool create = false;
                PointPos posi, posj;

                if ((firstStartPoint - secondStartPoint).Length() < Precision::Confusion()) {
                    create = true;
                    posi = PointPos::start;
                    posj = PointPos::start;
                }
                else if ((firstStartPoint - secondEndPoint).Length() < Precision::Confusion()) {
                    create = true;
                    posi = PointPos::start;
                    posj = PointPos::end;
                }
                else if ((firstEndPoint - secondStartPoint).Length() < Precision::Confusion()) {
                    create = true;
                    posi = PointPos::end;
                    posj = PointPos::start;
                }
                else if ((firstEndPoint - secondEndPoint).Length() < Precision::Confusion()) {
                    create = true;
                    posi = PointPos::end;
                    posj = PointPos::end;
                }

                if (create) {
                    bool tangent
                        = needTangent(listOfOffsetGeoIds[i], listOfOffsetGeoIds[j], posi, posj);
                    stream << "conList.append(Sketcher.Constraint('"
                           << (tangent ? "Tangent" : "Coincident");
                    stream << "'," << listOfOffsetGeoIds[i] << "," << static_cast<int>(posi) << ", "
                           << listOfOffsetGeoIds[j] << "," << static_cast<int>(posj) << "))\n";
                }
            }
        }

        stream << Gui::Command::getObjectCmd(sketchgui->getObject()) << ".addConstraint(conList)\n";
        stream << "del conList\n";
        Gui::Command::doCommand(Gui::Command::Doc, stream.str().c_str());
    }

    bool needTangent(int geoId1, int geoId2, PointPos pos1, PointPos pos2)
    {
        // Todo: add cases for arcOfellipse parabolas hyperbolas bspline

        SketchObject* Obj = sketchgui->getSketchObject();
        const Part::Geometry* geo1 = Obj->getGeometry(geoId1);
        const Part::Geometry* geo2 = Obj->getGeometry(geoId2);

        if (!isArcOfCircle(*geo1) && !isArcOfCircle(*geo2)) {
            return false;
        }

        Base::Vector3d perpendicular1, perpendicular2, p1, p2;
        if (isArcOfCircle(*geo1)) {
            auto* arcOfCircle = static_cast<const Part::GeomArcOfCircle*>(geo1);
            p1 = pos1 == PointPos::start ? arcOfCircle->getStartPoint(true)
                                         : arcOfCircle->getEndPoint(true);

            perpendicular1.x = -(arcOfCircle->getCenter() - p1).y;
            perpendicular1.y = (arcOfCircle->getCenter() - p1).x;
        }
        else if (isLineSegment(*geo1)) {
            auto* line = static_cast<const Part::GeomLineSegment*>(geo1);
            perpendicular1 = line->getStartPoint() - line->getEndPoint();
        }
        else {
            return false;
        }

        if (isArcOfCircle(*geo2)) {
            auto* arcOfCircle = static_cast<const Part::GeomArcOfCircle*>(geo2);
            p2 = pos2 == PointPos::start ? arcOfCircle->getStartPoint(true)
                                         : arcOfCircle->getEndPoint(true);

            perpendicular2.x = -(arcOfCircle->getCenter() - p2).y;
            perpendicular2.y = (arcOfCircle->getCenter() - p2).x;
        }
        else if (isLineSegment(*geo2)) {
            auto* line = static_cast<const Part::GeomLineSegment*>(geo2);
            perpendicular2 = line->getStartPoint() - line->getEndPoint();
        }
        else {
            return false;
        }

        // if lines are parallel
        if ((perpendicular1 % perpendicular2).Length() < Precision::Confusion()) {
            return true;
        }

        return false;
    }

    void deleteOriginalGeometries()
    {
        std::stringstream stream;
        for (size_t j = 0; j < listOfGeoIds.size() - 1; j++) {
            stream << listOfGeoIds[j] << ",";
        }
        stream << listOfGeoIds[listOfGeoIds.size() - 1];
        try {
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometries([%s])", stream.str().c_str());
        }
        catch (const Base::Exception& e) {
            Base::Console().error("%s\n", e.what());
        }
    }

    void makeOffsetConstraint(std::vector<int>& listOfOffsetGeoIds)
    {
        SketchObject* Obj = sketchgui->getSketchObject();

        std::stringstream stream;
        stream << "conList = []\n";
        // We separate the constraints of new lines in case the construction lines are not needed.
        std::stringstream newLinesStream;
        newLinesStream << "conList2 = []\n";

        vCCO = generatevCC(listOfOffsetGeoIds);

        int geoIdCandidate1 {};
        int geoIdCandidate2 {};

        int newCurveCounter = 0;
        int prevCurveCounter = 0;
        std::vector<Part::Geometry*> geometriesToAdd;
        for (auto& curve : vCCO) {
            // Check if curve is closed. Note as we use pipe it should always be closed but in case
            // we enable 'Skin' in the future.
            bool closed = isCurveClosed(curve);
            bool atLeastOneLine = false;
            bool rerunFirstAfterThis = false;
            bool rerunningFirst = false;
            bool inTangentGroup = false;

            for (size_t j = 0; j < curve.size(); j++) {

                // Tangent constraint is constraining the offset already. So if there are tangents
                // we should not create the construction lines. Hence the code below.
                bool createLine = true;
                bool forceCreate = false;
                if (!inTangentGroup && (!closed || j != 0 || rerunningFirst)) {
                    createLine = true;
                    atLeastOneLine = true;
                }
                else {
                    // include case of j == 0 and closed curve, because if required the line
                    // will be made after last.
                    createLine = false;
                }

                if (j + 1 < curve.size()) {
                    inTangentGroup = areTangentCoincident(curve[j], curve[j + 1]);
                }
                else if (j == curve.size() - 1 && closed) {
                    // Case of last geoId for closed curves.
                    inTangentGroup = areTangentCoincident(curve[j], curve[0]);
                    if (inTangentGroup) {
                        if (!atLeastOneLine) {  // We need at least one line
                            createLine = true;
                            forceCreate = true;
                        }
                    }
                    else {
                        // We rerun the for at j=0 after this run to create line for j = 0.
                        rerunFirstAfterThis = true;
                    }
                }

                const Part::Geometry* geo = Obj->getGeometry(curve[j]);
                for (auto geoId : listOfGeoIds) {
                    // Check if geoId is the offsetted curve giving curve[j].
                    const Part::Geometry* geo2 = Obj->getGeometry(geoId);

                    if (isCircle(*geo) && isCircle(*geo2)) {
                        auto* circle = static_cast<const Part::GeomCircle*>(geo);
                        auto* circle2 = static_cast<const Part::GeomCircle*>(geo2);
                        Base::Vector3d p1 = circle->getCenter();
                        Base::Vector3d p2 = circle2->getCenter();
                        if ((p1 - p2).Length() < Precision::Confusion()) {
                            // coincidence of center
                            stream << "conList.append(Sketcher.Constraint('Coincident'," << curve[j]
                                   << ",3, " << geoId << ",3))\n";

                            // Create line between both circles.
                            auto* line = new Part::GeomLineSegment();
                            p1.x = p1.x + circle->getRadius();
                            p2.x = p2.x + circle2->getRadius();
                            line->setPoints(p1, p2);
                            GeometryFacade::setConstruction(line, true);
                            geometriesToAdd.push_back(line);
                            newCurveCounter++;
                            newLinesStream << "conList2.append(Sketcher.Constraint('Perpendicular',"
                                           << getHighestCurveIndex() + newCurveCounter << ", "
                                           << curve[j] << "))\n";
                            newLinesStream << "conList2.append(Sketcher.Constraint('PointOnObject',"
                                           << getHighestCurveIndex() + newCurveCounter << ",1, "
                                           << curve[j] << "))\n";
                            newLinesStream << "conList2.append(Sketcher.Constraint('PointOnObject',"
                                           << getHighestCurveIndex() + newCurveCounter << ",2, "
                                           << geoId << "))\n";

                            geoIdCandidate1 = curve[j];
                            geoIdCandidate2 = geoId;

                            break;
                        }
                    }
                    else if (isEllipse(*geo) && isEllipse(*geo2)) {
                        // same as circle but 2 lines
                    }
                    else if (isLineSegment(*geo) && isLineSegment(*geo2)) {
                        auto* lineSeg1 = static_cast<const Part::GeomLineSegment*>(geo);
                        auto* lineSeg2 = static_cast<const Part::GeomLineSegment*>(geo2);
                        Base::Vector3d p1[2], p2[2];
                        p1[0] = lineSeg1->getStartPoint();
                        p1[1] = lineSeg1->getEndPoint();
                        p2[0] = lineSeg2->getStartPoint();
                        p2[1] = lineSeg2->getEndPoint();
                        // if lines are parallel
                        if (((p1[1] - p1[0]) % (p2[1] - p2[0])).Length() < Precision::Intersection()) {
                            // If the lines are space by offsetLength distance
                            Base::Vector3d projectedP;
                            projectedP.ProjectToLine(p1[0] - p2[0], p2[1] - p2[0]);

                            if ((projectedP).Length() - fabs(offsetLength) < Precision::Confusion()) {
                                if (!forceCreate && !rerunningFirst) {
                                    stream << "conList.append(Sketcher.Constraint('Parallel',"
                                           << curve[j] << ", " << geoId << "))\n";
                                }

                                // We don't need a construction line if the line has a tangent at
                                // one end. Unless it's the first line that we're making.
                                if (createLine) {
                                    auto* line = new Part::GeomLineSegment();
                                    line->setPoints(p1[0], p1[0] + projectedP);
                                    GeometryFacade::setConstruction(line, true);
                                    geometriesToAdd.push_back(line);
                                    newCurveCounter++;

                                    newLinesStream
                                        << "conList2.append(Sketcher.Constraint('Perpendicular',"
                                        << getHighestCurveIndex() + newCurveCounter << ", "
                                        << curve[j] << "))\n";
                                    newLinesStream
                                        << "conList2.append(Sketcher.Constraint('PointOnObject',"
                                        << getHighestCurveIndex() + newCurveCounter << ",1, "
                                        << curve[j] << "))\n";
                                    newLinesStream
                                        << "conList2.append(Sketcher.Constraint('PointOnObject',"
                                        << getHighestCurveIndex() + newCurveCounter << ",2, "
                                        << geoId << "))\n";

                                    geoIdCandidate1 = curve[j];
                                    geoIdCandidate2 = geoId;
                                }
                                break;
                            }
                        }
                    }
                    else if (isArcOfCircle(*geo)) {
                        // multiple cases because arc join mode creates arcs or circle.
                        auto* arcOfCircle = static_cast<const Part::GeomArcOfCircle*>(geo);
                        Base::Vector3d p1 = arcOfCircle->getCenter();

                        if (isArcOfCircle(*geo2)) {
                            auto* arcOfCircle2 = static_cast<const Part::GeomArcOfCircle*>(geo2);
                            Base::Vector3d p2 = arcOfCircle2->getCenter();
                            Base::Vector3d p3 = arcOfCircle2->getStartPoint(true);
                            Base::Vector3d p4 = arcOfCircle2->getEndPoint(true);

                            if ((p1 - p2).Length() < Precision::Confusion()) {
                                // coincidence of center. Offset arc is the offset of an arc
                                stream << "conList.append(Sketcher.Constraint('Coincident',"
                                       << curve[j] << ",3, " << geoId << ",3))\n";
                                if (createLine) {
                                    // Create line between both circles.
                                    auto* line = new Part::GeomLineSegment();
                                    p1.x = p1.x + arcOfCircle->getRadius();
                                    p2.x = p2.x + arcOfCircle2->getRadius();
                                    line->setPoints(p1, p2);
                                    GeometryFacade::setConstruction(line, true);
                                    geometriesToAdd.push_back(line);
                                    newCurveCounter++;
                                    newLinesStream
                                        << "conList2.append(Sketcher.Constraint('Perpendicular',"
                                        << getHighestCurveIndex() + newCurveCounter << ", "
                                        << curve[j] << "))\n";
                                    newLinesStream
                                        << "conList2.append(Sketcher.Constraint('PointOnObject',"
                                        << getHighestCurveIndex() + newCurveCounter << ",1, "
                                        << curve[j] << "))\n";
                                    newLinesStream
                                        << "conList2.append(Sketcher.Constraint('PointOnObject',"
                                        << getHighestCurveIndex() + newCurveCounter << ",2, "
                                        << geoId << "))\n";

                                    geoIdCandidate1 = curve[j];
                                    geoIdCandidate2 = geoId;
                                }
                                break;
                            }
                            else if ((p1 - p3).Length() < Precision::Confusion()) {
                                // coincidence of center to startpoint. offset arc is created arc
                                // join
                                stream << "conList.append(Sketcher.Constraint('Coincident',"
                                       << curve[j] << ",3, " << geoId << ", 1))\n";

                                if (forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Radius',"
                                           << curve[j] << ", " << offsetLength << "))\n";
                                }
                                break;
                            }
                            else if ((p1 - p4).Length() < Precision::Confusion()) {
                                // coincidence of center to startpoint
                                stream << "conList.append(Sketcher.Constraint('Coincident',"
                                       << curve[j] << ",3, " << geoId << ", 2))\n";

                                if (forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Radius',"
                                           << curve[j] << ", " << offsetLength << "))\n";
                                }
                                break;
                            }
                        }
                        else if (isLineSegment(*geo2) || isBSplineCurve(*geo2)
                                 || geo2->is<Part::GeomArcOfConic>()) {
                            // cases where arc is created by arc join mode.
                            Base::Vector3d p2, p3;

                            if (getFirstSecondPoints(geoId, p2, p3)) {
                                bool startCoincidence = (p1 - p2).Length() < Precision::Confusion();
                                bool endCoincidence = (p1 - p3).Length() < Precision::Confusion();

                                if (startCoincidence || endCoincidence) {
                                    // coincidence of center to startpoint
                                    stream << "conList.append(Sketcher.Constraint('Coincident',"
                                           << curve[j] << ", 3, " << geoId << ", "
                                           << (startCoincidence ? 1 : 2) << "))\n";

                                    geoIdCandidate1 = curve[j];
                                    geoIdCandidate2 = geoId;

                                    break;
                                }
                            }
                        }
                    }
                    else if (isArcOfEllipse(*geo) && isArcOfEllipse(*geo2)) {
                        // const Part::GeomArcOfEllipse* arcOfEllipse = static_cast<const
                        // Part::GeomArcOfEllipse*>(geo2);
                    }
                    else if (isArcOfHyperbola(*geo) && isArcOfHyperbola(*geo2)) {
                        // const Part::GeomArcOfHyperbola* arcOfHyperbola = static_cast<const
                        // Part::GeomArcOfHyperbola*>(geo2);
                    }
                    else if (isArcOfParabola(*geo) && isArcOfParabola(*geo2)) {
                        // const Part::GeomArcOfParabola* arcOfParabola = static_cast<const
                        // Part::GeomArcOfParabola*>(geo2);
                    }
                    else if (isBSplineCurve(*geo) && isBSplineCurve(*geo2)) {
                    }
                }
                if (newCurveCounter != prevCurveCounter) {
                    prevCurveCounter = newCurveCounter;
                    if (newCurveCounter != 1) {
                        stream << "conList.append(Sketcher.Constraint('Equal',"
                               << getHighestCurveIndex() + newCurveCounter << ", "
                               << getHighestCurveIndex() + 1 << "))\n";
                    }
                }


                if (rerunningFirst) {
                    break;
                }

                if (rerunFirstAfterThis) {
                    j = -1;  // j will be incremented to 0 after new loop
                    rerunningFirst = true;
                }
            }
        }

        if (newCurveCounter >= 2) {
            stream << "conList.append(Sketcher.Constraint('Distance'," << getHighestCurveIndex() + 1
                   << ", " << fabs(offsetLength) << "))\n";

            Obj->addGeometry(std::move(geometriesToAdd));

            newLinesStream << Gui::Command::getObjectCmd(sketchgui->getObject())
                           << ".addConstraint(conList2)\n";
            newLinesStream << "del conList2\n";
            Gui::Command::doCommand(Gui::Command::Doc, newLinesStream.str().c_str());
        }
        else {
            // If there is a single construction line, then its not needed.
            const Part::Geometry* geo = Obj->getGeometry(geoIdCandidate1);

            if (isCircle(*geo)) {
                stream << "conList.append(Sketcher.Constraint('Distance'," << geoIdCandidate1
                       << ", " << geoIdCandidate2 << ", " << fabs(offsetLength) << "))\n";
            }
            else if (isLineSegment(*geo)) {
                stream << "conList.append(Sketcher.Constraint('Distance'," << geoIdCandidate1
                       << ", 1," << geoIdCandidate2 << ", " << fabs(offsetLength) << "))\n";
            }
            else if (isArcOfCircle(*geo)) {
                const Part::Geometry* geo2 = Obj->getGeometry(geoIdCandidate2);
                if (isArcOfCircle(*geo2)) {
                    stream << "conList.append(Sketcher.Constraint('Distance'," << geoIdCandidate1
                           << ", 1," << geoIdCandidate2 << ", 1, " << fabs(offsetLength) << "))\n";
                }
                else if (isLineSegment(*geo2)) {
                    stream << "conList.append(Sketcher.Constraint('Distance'," << geoIdCandidate1
                           << ", 3," << geoIdCandidate1 << ", 1, " << fabs(offsetLength) << "))\n";
                }
            }
        }

        stream << Gui::Command::getObjectCmd(sketchgui->getObject()) << ".addConstraint(conList)\n";
        stream << "del conList\n";
        Gui::Command::doCommand(Gui::Command::Doc, stream.str().c_str());
    }

    std::vector<std::vector<int>> generatevCC(std::vector<int>& listOfGeo)
    {
        // This function separates all the selected geometries into separate continuous curves.
        SketchObject* Obj = sketchgui->getSketchObject();
        std::vector<std::vector<int>> vcc;

        for (auto geoId : listOfGeo) {
            std::vector<int> vecOfGeoIds;
            const Part::Geometry* geo = Obj->getGeometry(geoId);
            if (isCircle(*geo) || isEllipse(*geo)) {
                vecOfGeoIds.push_back(geoId);
                vcc.push_back(vecOfGeoIds);
                continue;
            }

            bool inserted = false;
            int insertedIn = -1;
            for (size_t j = 0; j < vcc.size(); j++) {
                for (size_t k = 0; k < vcc[j].size(); k++) {
                    Base::Vector3d coincidentPoint;
                    if (!getCoincidentPoint(geoId, vcc[j][k], coincidentPoint)) {
                        continue;
                    }

                    // Don't chain edges at branch points (where 3+ edges meet)
                    if (isBranchPoint(coincidentPoint, listOfGeo)) {
                        continue;
                    }

                    if (inserted && insertedIn != int(j)) {
                        // if it's already inserted in another continuous curve then we need
                        // to merge both curves together. There're 2 cases, it could have
                        // been inserted at the end or at the beginning.
                        if (vcc[insertedIn][0] == geoId) {
                            // Two cases. Either the coincident is at the beginning or at
                            // the end.
                            if (k == 0) {
                                std::reverse(vcc[j].begin(), vcc[j].end());
                            }
                            vcc[j].insert(vcc[j].end(), vcc[insertedIn].begin(), vcc[insertedIn].end());
                            vcc.erase(vcc.begin() + insertedIn);
                        }
                        else {
                            if (k != 0) {  // ie k is  vcc[j].size()-1
                                std::reverse(vcc[j].begin(), vcc[j].end());
                            }
                            vcc[insertedIn].insert(vcc[insertedIn].end(), vcc[j].begin(), vcc[j].end());
                            vcc.erase(vcc.begin() + j);
                        }
                        j--;
                    }
                    else {
                        // we need to get the curves in the correct order.
                        if (k == vcc[j].size() - 1) {
                            vcc[j].push_back(geoId);
                        }
                        else {
                            // in this case k should actually be 0.
                            vcc[j].insert(vcc[j].begin() + k, geoId);
                        }
                        insertedIn = j;
                        inserted = true;
                    }
                    // printCCeVec();
                    break;
                }
            }
            if (!inserted) {
                vecOfGeoIds.push_back(geoId);
                vcc.push_back(vecOfGeoIds);
            }
        }
        return vcc;
    }

    void generateSourceWires()
    {
        vCC = generatevCC(listOfGeoIds);

        SketchObject* Obj = sketchgui->getSketchObject();

        for (auto& CC : vCC) {
            BRepBuilderAPI_MakeWire mkWire;
            for (auto& curveId : CC) {
                const Part::Geometry* pGeo = Obj->getGeometry(curveId);
                auto geoCopy = std::unique_ptr<Part::Geometry>(pGeo->copy());
                Part::Geometry* geo = geoCopy.get();
                geo->reverseIfReversed();  // make sure we don't have reversed conics

                // Use the normalized copy to create the edge for the wire
                mkWire.Add(TopoDS::Edge(geo->toShape()));
            }

            TopoDS_Wire wire = mkWire.Wire();

            // Fix orientation: ensure all closed wires are CCW relative to Sketch Plane (+Z)
            if (wire.Closed()) {
                BRepBuilderAPI_MakeFace mkFace(wire);
                if (mkFace.IsDone()) {
                    TopoDS_Face face = mkFace.Face();
                    BRepAdaptor_Surface surf(face);
                    if (surf.GetType() == GeomAbs_Plane) {
                        gp_Dir norm = surf.Plane().Axis().Direction();
                        if (norm.Z() < 0) {
                            wire.Reverse();
                        }
                    }
                }
            }

            // Here we make sure that if possible the first wire is not a single line.
            if (CC.size() == 1 && isLineSegment(*Obj->getGeometry(CC[0]))) {
                sourceWires.push_back(wire);
            }
            else {
                sourceWires.insert(sourceWires.begin(), wire);
                onlySingleLines = false;
            }
        }
    }

    void findOffsetLength()
    {
        double newOffsetLength = std::numeric_limits<double>::max();

        BRepBuilderAPI_MakeVertex mkVertex({endpoint.x, endpoint.y, 0.0});
        TopoDS_Vertex vertex = mkVertex.Vertex();
        for (auto& wire : sourceWires) {
            BRepExtrema_DistShapeShape distTool(wire, vertex);
            if (distTool.IsDone()) {
                double distance = distTool.Value();
                if (distance == std::min(distance, newOffsetLength)) {
                    newOffsetLength = distance;

                    gp_Pnt pnt = distTool.PointOnShape1(1);
                    pointOnSourceWire = Base::Vector2d(pnt.X(), pnt.Y());

                    // find direction
                    if (BRep_Tool::IsClosed(wire)) {
                        TopoDS_Face aFace = BRepBuilderAPI_MakeFace(wire);
                        BRepClass_FaceClassifier checkPoint(
                            aFace,
                            {endpoint.x, endpoint.y, 0.0},
                            Precision::Confusion()
                        );
                        if (checkPoint.State() == TopAbs_IN) {
                            newOffsetLength = -newOffsetLength;
                        }
                    }
                }
            }
        }

        if (newOffsetLength != std::numeric_limits<double>::max()) {
            offsetLength = newOffsetLength;
        }
    }

    bool getFirstSecondPoints(int geoId, Base::Vector3d& startPoint, Base::Vector3d& endPoint)
    {
        const Part::Geometry* geo = sketchgui->getSketchObject()->getGeometry(geoId);

        if (isLineSegment(*geo)) {
            const auto* line = static_cast<const Part::GeomLineSegment*>(geo);
            startPoint = line->getStartPoint();
            endPoint = line->getEndPoint();
            return true;
        }
        else if (isArcOfCircle(*geo) || isArcOfEllipse(*geo) || isArcOfHyperbola(*geo)
                 || isArcOfParabola(*geo)) {
            const auto* arcOfConic = static_cast<const Part::GeomArcOfConic*>(geo);
            startPoint = arcOfConic->getStartPoint(true);
            endPoint = arcOfConic->getEndPoint(true);
            return true;
        }
        else if (isBSplineCurve(*geo)) {
            const auto* bSpline = static_cast<const Part::GeomBSplineCurve*>(geo);
            startPoint = bSpline->getStartPoint();
            endPoint = bSpline->getEndPoint();
            return true;
        }
        return false;
    }

    CoincidencePointPos checkForCoincidence(int geoId1, int geoId2, bool tangentOnly = false)
    {
        // This function looks up for 2 coincidence between 2 edges (arc + line can have 2)
        SketchObject* Obj = sketchgui->getSketchObject();
        const std::vector<Constraint*>& vals = Obj->Constraints.getValues();
        CoincidencePointPos positions;
        positions.firstPos1 = PointPos::none;
        positions.secondPos1 = PointPos::none;
        positions.firstPos2 = PointPos::none;
        positions.secondPos2 = PointPos::none;
        bool firstCoincidenceFound = false;
        for (auto* cstr : vals) {
            if (((tangentOnly || cstr->Type != Coincident) && cstr->Type != Tangent)
                || cstr->FirstPos == PointPos::mid || cstr->FirstPos == PointPos::none
                || cstr->SecondPos == PointPos::mid || cstr->SecondPos == PointPos::none) {
                continue;
            }

            if ((cstr->First == geoId1 && cstr->Second == geoId2)
                || (cstr->First == geoId2 && cstr->Second == geoId1)) {
                if (!firstCoincidenceFound) {
                    positions.firstPos1 = cstr->First == geoId1 ? cstr->FirstPos : cstr->SecondPos;
                    positions.secondPos1 = cstr->First == geoId2 ? cstr->FirstPos : cstr->SecondPos;
                    firstCoincidenceFound = true;
                }
                else {
                    positions.firstPos2 = cstr->First == geoId1 ? cstr->FirstPos : cstr->SecondPos;
                    positions.secondPos2 = cstr->First == geoId2 ? cstr->FirstPos : cstr->SecondPos;
                    break;
                }
            }
        }
        return positions;
    }

    bool areCoincident(int geoId1, int geoId2)
    {
        // Instead of checking for constraints like so:
        // CoincidencePointPos ppc = checkForCoincidence(geoId1, geoId2);
        // return ppc.firstPos1 != PointPos::none;
        // we are going to check if the points are effectively coincident:

        Base::Vector3d p11, p12, p21, p22;
        if (!getFirstSecondPoints(geoId1, p11, p12) || !getFirstSecondPoints(geoId2, p21, p22)) {
            return false;
        }

        return (
            (p11 - p21).Length() < Precision::Confusion()
            || (p11 - p22).Length() < Precision::Confusion()
            || (p12 - p21).Length() < Precision::Confusion()
            || (p12 - p22).Length() < Precision::Confusion()
        );
    }

    // Returns the point where two edges meet (if any)
    bool getCoincidentPoint(int geoId1, int geoId2, Base::Vector3d& coincidentPoint)
    {
        Base::Vector3d p11, p12, p21, p22;
        if (!getFirstSecondPoints(geoId1, p11, p12) || !getFirstSecondPoints(geoId2, p21, p22)) {
            return false;
        }

        if ((p11 - p21).Length() < Precision::Confusion()) {
            coincidentPoint = p11;
            return true;
        }
        if ((p11 - p22).Length() < Precision::Confusion()) {
            coincidentPoint = p11;
            return true;
        }
        if ((p12 - p21).Length() < Precision::Confusion()) {
            coincidentPoint = p12;
            return true;
        }
        if ((p12 - p22).Length() < Precision::Confusion()) {
            coincidentPoint = p12;
            return true;
        }
        return false;
    }

    // Checks if a point is a branch point (3+ edges meeting there)
    bool isBranchPoint(const Base::Vector3d& point, const std::vector<int>& listOfGeo)
    {
        int edgeCount = 0;
        for (int geoId : listOfGeo) {
            Base::Vector3d p1, p2;
            if (getFirstSecondPoints(geoId, p1, p2)) {
                if ((point - p1).Length() < Precision::Confusion()
                    || (point - p2).Length() < Precision::Confusion()) {
                    edgeCount++;
                    if (edgeCount >= 3) {
                        return true;  // Early exit once we know it's a branch point
                    }
                }
            }
        }
        return false;
    }

    // Collect all branch points from the geometry list
    void collectBranchPoints()
    {
        branchPoints.clear();

        // Collect all endpoints
        std::vector<Base::Vector3d> allEndpoints;
        for (int geoId : listOfGeoIds) {
            Base::Vector3d p1, p2;
            if (getFirstSecondPoints(geoId, p1, p2)) {
                allEndpoints.push_back(p1);
                allEndpoints.push_back(p2);
            }
        }

        // For each unique endpoint, check if it's a branch point
        for (const auto& pt : allEndpoints) {
            // Skip if we already have this point
            bool alreadyAdded = false;
            for (const auto& bp : branchPoints) {
                if ((pt - bp).Length() < Precision::Confusion()) {
                    alreadyAdded = true;
                    break;
                }
            }
            if (alreadyAdded) {
                continue;
            }

            if (isBranchPoint(pt, listOfGeoIds)) {
                branchPoints.push_back(pt);
            }
        }
    }

    // Check if a point is near any known branch point
    bool isNearBranchPoint(const Base::Vector3d& point, double tolerance)
    {
        for (const auto& bp : branchPoints) {
            if ((point - bp).Length() < tolerance) {
                return true;
            }
        }
        return false;
    }

    // Get the angle of an edge at a branch point (direction pointing away from branch)
    double getEdgeAngleAtBranchPoint(int geoId, const Base::Vector3d& branchPt)
    {
        Base::Vector3d p1, p2;
        if (!getFirstSecondPoints(geoId, p1, p2)) {
            return 0.0;
        }

        // Determine which endpoint is at the branch point
        Base::Vector3d direction;
        if ((p1 - branchPt).Length() < Precision::Confusion()) {
            direction = p2 - p1;  // Direction from branch point to other end
        }
        else {
            direction = p1 - p2;  // Direction from branch point to other end
        }

        return atan2(direction.y, direction.x);
    }

    // Get edges connected to a branch point, sorted by angle
    std::vector<int> getEdgesSortedByAngle(const Base::Vector3d& branchPt)
    {
        std::vector<std::pair<double, int>> angleGeoIdPairs;

        for (int geoId : listOfGeoIds) {
            Base::Vector3d p1, p2;
            if (getFirstSecondPoints(geoId, p1, p2)) {
                if ((p1 - branchPt).Length() < Precision::Confusion()
                    || (p2 - branchPt).Length() < Precision::Confusion()) {
                    double angle = getEdgeAngleAtBranchPoint(geoId, branchPt);
                    angleGeoIdPairs.push_back({angle, geoId});
                }
            }
        }

        // Sort by angle
        std::sort(angleGeoIdPairs.begin(), angleGeoIdPairs.end());

        std::vector<int> sortedGeoIds;
        for (const auto& pair : angleGeoIdPairs) {
            sortedGeoIds.push_back(pair.second);
        }
        return sortedGeoIds;
    }

    // Create wires for adjacent pairs of edges at branch points
    std::vector<TopoDS_Wire> createBranchPairWires()
    {
        std::vector<TopoDS_Wire> pairWires;
        SketchObject* Obj = sketchgui->getSketchObject();

        for (const auto& branchPt : branchPoints) {
            std::vector<int> sortedEdges = getEdgesSortedByAngle(branchPt);

            if (sortedEdges.size() < 2) {
                continue;
            }

            gp_Pnt branchGp(branchPt.x, branchPt.y, branchPt.z);

            // Create wires for each adjacent pair (including wrap-around)
            for (size_t i = 0; i < sortedEdges.size(); i++) {
                int geoId1 = sortedEdges[i];
                int geoId2 = sortedEdges[(i + 1) % sortedEdges.size()];

                // Get the edges
                const Part::Geometry* pGeo1 = Obj->getGeometry(geoId1);
                auto geoCopy1 = std::unique_ptr<Part::Geometry>(pGeo1->copy());
                geoCopy1->reverseIfReversed();
                TopoDS_Edge edge1 = TopoDS::Edge(geoCopy1->toShape());

                const Part::Geometry* pGeo2 = Obj->getGeometry(geoId2);
                auto geoCopy2 = std::unique_ptr<Part::Geometry>(pGeo2->copy());
                geoCopy2->reverseIfReversed();
                TopoDS_Edge edge2 = TopoDS::Edge(geoCopy2->toShape());

                // Get vertices of edge1 to check orientation
                TopoDS_Vertex v1First, v1Last;
                TopExp::Vertices(edge1, v1First, v1Last);
                gp_Pnt p1First = BRep_Tool::Pnt(v1First);
                gp_Pnt p1Last = BRep_Tool::Pnt(v1Last);

                // Edge1 should end at branch point (wire goes: outer1 -> branch -> outer2)
                bool edge1EndsAtBranch = p1Last.Distance(branchGp) < Precision::Confusion();
                bool edge1StartsAtBranch = p1First.Distance(branchGp) < Precision::Confusion();

                if (edge1StartsAtBranch && !edge1EndsAtBranch) {
                    edge1.Reverse();
                }

                // Get vertices of edge2 to check orientation
                TopoDS_Vertex v2First, v2Last;
                TopExp::Vertices(edge2, v2First, v2Last);
                gp_Pnt p2First = BRep_Tool::Pnt(v2First);
                gp_Pnt p2Last = BRep_Tool::Pnt(v2Last);

                // Edge2 should start at branch point
                bool edge2StartsAtBranch = p2First.Distance(branchGp) < Precision::Confusion();
                bool edge2EndsAtBranch = p2Last.Distance(branchGp) < Precision::Confusion();

                if (edge2EndsAtBranch && !edge2StartsAtBranch) {
                    edge2.Reverse();
                }

                // Create wire with properly oriented edges
                BRepBuilderAPI_MakeWire mkWire;
                mkWire.Add(edge1);
                mkWire.Add(edge2);

                if (mkWire.IsDone()) {
                    pairWires.push_back(mkWire.Wire());
                }
            }
        }

        return pairWires;
    }

    bool areTangentCoincident(int geoId1, int geoId2)
    {
        CoincidencePointPos ppc = checkForCoincidence(geoId1, geoId2, true);
        return ppc.firstPos1 != PointPos::none;
    }

    bool isCurveClosed(std::vector<int>& curve)
    {
        bool closed = false;
        if (curve.size() > 2) {
            closed = areCoincident(curve[0], curve[curve.size() - 1]);
        }
        else if (curve.size() == 2) {
            // if only 2 elements, we need to check if they close end to end.
            CoincidencePointPos cpp = checkForCoincidence(curve[0], curve[curve.size() - 1]);
            closed = cpp.firstPos1 != PointPos::none && cpp.firstPos2 != PointPos::none;
        }
        return closed;
    }

    // debug only
    /*void printCCeVec()
    {
        for (size_t j = 0; j < vCC.size(); j++) {
            Base::Console().warning("curve %d{", j);
            for (size_t k = 0; k < vCC[j].size(); k++) {
                Base::Console().warning("%d, ", vCC[j][k]);
            }
            Base::Console().warning("}\n");
        }
    }*/
};

template<>
auto DSHOffsetControllerBase::getState(int labelindex) const
{
    switch (labelindex) {
        case OnViewParameter::First:
            return SelectMode::SeekFirst;
            break;
        default:
            THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template<>
void DSHOffsetController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        QStringList names = {
            QApplication::translate("Sketcher_CreateOffset", "Arc"),
            QApplication::translate("Sketcher_CreateOffset", "Intersection")
        };
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        toolWidget->setComboboxItemIcon(
            WCombobox::FirstCombo,
            0,
            Gui::BitmapFactory().iconFromTheme("Sketcher_OffsetArc")
        );
        toolWidget->setComboboxItemIcon(
            WCombobox::FirstCombo,
            1,
            Gui::BitmapFactory().iconFromTheme("Sketcher_OffsetIntersection")
        );

        toolWidget->setCheckboxLabel(
            WCheckbox::FirstBox,
            QApplication::translate("TaskSketcherTool_c1_offset", "Delete original geometries (U)")
        );
        toolWidget->setCheckboxLabel(
            WCheckbox::SecondBox,
            QApplication::translate("TaskSketcherTool_c2_offset", "Add offset constraint (J)")
        );
    }

    onViewParameters[OnViewParameter::First]->setLabelType(
        Gui::SoDatumLabel::DISTANCE,
        Gui::EditableDatumLabel::Function::Forced
    );
}

template<>
void DSHOffsetControllerBase::adaptDrawingToOnViewParameterChange(int labelindex, double value)
{
    switch (labelindex) {
        case OnViewParameter::First: {
            if (value == 0. && onViewParameters[OnViewParameter::First]->hasFinishedEditing) {
                // Do not accept 0, but only if user has finished editing the OVP.
                unsetOnViewParameter(onViewParameters[OnViewParameter::First].get());

                // reset offsetLengthSet so mouse can control the offset again
                handler->offsetLengthSet = false;

                Gui::NotifyUserError(
                    handler->sketchgui->getSketchObject(),
                    QT_TRANSLATE_NOOP("Notifications", "Invalid Value"),
                    QT_TRANSLATE_NOOP("Notifications", "Offset value can't be 0.")
                );
            }
            else {
                handler->offsetLengthSet = true;
                handler->offsetLength = value;
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHOffsetController::adaptDrawingToCheckboxChange(int checkboxindex, bool value)
{
    switch (checkboxindex) {
        case WCheckbox::FirstBox:
            handler->deleteOriginal = value;

            // Both options cannot be enabled at the same time.
            if (value && toolWidget->getCheckboxChecked(WCheckbox::SecondBox)) {
                toolWidget->setCheckboxChecked(WCheckbox::SecondBox, false);
            }
            break;

        case WCheckbox::SecondBox:
            handler->offsetConstraint = value;

            // Both options cannot be enabled at the same time.
            if (value && toolWidget->getCheckboxChecked(WCheckbox::FirstBox)) {
                toolWidget->setCheckboxChecked(WCheckbox::FirstBox, false);
            }
            break;
    }
}

/* doEnforceControlParameters : The tool validates after offset length is set. So we don't need to
 * enforce it. Besides it is hard to override onsketchpos such that it is at offsetLength from the
 * curve. As we do not override the pos, we need to use offsetLengthSet to prevent rewrite of
 * offsetLength.*/

template<>
void DSHOffsetController::adaptParameters(Base::Vector2d onSketchPos)
{
    Q_UNUSED(onSketchPos)

    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];

            if (!firstParam->isSet) {
                setOnViewParameterValue(OnViewParameter::First, handler->offsetLength);
            }

            Base::Vector3d dimensionEndpoint;
            if (handler->offsetLengthSet && firstParam->isSet) {
                // if user has typed a value, calculate correct endpoint based on typed value
                Base::Vector2d direction = handler->endpoint - handler->pointOnSourceWire;
                if (direction.Length() > Precision::Confusion()) {
                    direction.Normalize();
                    Base::Vector2d correctedEndpoint = handler->pointOnSourceWire
                        + direction * handler->offsetLength;
                    dimensionEndpoint = Base::Vector3d(correctedEndpoint.x, correctedEndpoint.y, 0.);
                }
                else {
                    dimensionEndpoint = Base::Vector3d(handler->endpoint.x, handler->endpoint.y, 0.);
                }
            }
            else {
                // use mouse pos when user hasn't typed a value
                dimensionEndpoint = Base::Vector3d(handler->endpoint.x, handler->endpoint.y, 0.);
            }

            firstParam->setPoints(
                dimensionEndpoint,
                Base::Vector3d(handler->pointOnSourceWire.x, handler->pointOnSourceWire.y, 0.)
            );
        } break;
        default:
            break;
    }
}

template<>
void DSHOffsetController::computeNextDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];

            if (firstParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerOffset_H

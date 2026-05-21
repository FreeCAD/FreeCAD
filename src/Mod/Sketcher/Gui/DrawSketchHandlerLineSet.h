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

#pragma once

#include <QApplication>

#include <Inventor/events/SoKeyboardEvent.h>
#include <boost/math/special_functions/fpclassify.hpp>

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/InputHint.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

#include "DrawSketchHandler.h"
#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "SnapManager.h"

using namespace Sketcher;

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerPolyLine;

namespace ConstructionMethods
{
enum class PolyLineConstructionMethod
{
    Line,
    Arc,
    End  // Must be the last one
};
}  // namespace ConstructionMethods

using DSHPolyLineController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerPolyLine,
    /*SelectModeT*/ StateMachines::TwoSeekEnd,
    /*PAutoConstraintSize =*/2,
    /*OnViewParametersT =*/OnViewParameters<4, 5>,  // NOLINT
    /*WidgetParametersT =*/WidgetParameters<0, 0>,  // NOLINT
    /*WidgetCheckboxesT =*/WidgetCheckboxes<1, 1>,  // NOLINT
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,  // NOLINT
    /*WidgetLineEditsT =*/WidgetLineEdits<0, 0>,    // NOLINT
    ConstructionMethods::PolyLineConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHPolyLineControllerBase = DSHPolyLineController::ControllerBase;

using DrawSketchHandlerPolyLineBase = DrawSketchControllableHandler<DSHPolyLineController>;


class DrawSketchHandlerPolyLine: public DrawSketchHandlerPolyLineBase
{
    friend DSHPolyLineController;
    friend DSHPolyLineControllerBase;

public:
    explicit DrawSketchHandlerPolyLine(ConstructionMethod constrMethod = ConstructionMethod::Line)
        : DrawSketchHandlerPolyLineBase(constrMethod)
        , prevCursorPos(Base::Vector2d())
        , resetSeekSecond(false)
        , resetEdge(true)
        , fillet(false)
        , previousDirectionAngle(0.0)
        , dirChangeAngle(0.0)
        , startAngle(0.0)
        , range(0.0)
        , angleToPrevious(0.0)
        , pos(PointPos::end)
        , capturedDirection(0.0, 0.0) {};
    ~DrawSketchHandlerPolyLine() override = default;

    void activated() override
    {
        DrawSketchHandlerPolyLineBase::activated();
        openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch polyline"));
    }

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        prevCursorPos = onSketchPos;

        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);

                seekAndRenderAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.F, 0.F));
            } break;
            case SelectMode::SeekSecond: {
                toolWidgetManager.drawDirectionAtCursor(onSketchPos, getLastPoint());

                double angle1 = (onSketchPos - getLastPoint()).Angle() - previousDirectionAngle;
                double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * std::numbers::pi;
                dirChangeAngle = abs(angle1 - dirChangeAngle) < abs(angle2 - dirChangeAngle)
                    ? angle1
                    : angle2;

                try {
                    CreateAndDrawShapeGeometry();
                }
                catch (const Base::ValueError&) {
                }  // equal points while hovering raise an objection that can be safely ignored

                Base::Vector2d vec;
                if (constructionMethod() == ConstructionMethod::Line) {
                    vec = onSketchPos - getLastPoint();
                }

                seekAndRenderAutoConstraint(sugConstraints[1], onSketchPos, vec);
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        commitCommand();
    }

    void generateAutoConstraints() override
    {
        // The auto constraints are already generated in canGoToNextMode
        removeRedundantAutoConstraints();
    }

    void createAutoConstraints() override
    {
        // The auto constraints are already generated in canGoToNextMode
    }

    std::string getToolName() const override
    {
        return "DSH_PolyLine";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Create_Lineset");
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
        return Gui::BitmapFactory().pixmap("Sketcher_CreatePolyline");
    }

    QString getToolWidgetText() const override
    {
        return QString(QObject::tr("Polyline parameters"));
    }

    bool canGoToNextMode() override
    {
        if (state() == SelectMode::SeekFirst) {
            points.push_back(prevCursorPos);
        }
        if (state() == SelectMode::SeekSecond) {
            // We create the geometry first.
            createShape(false);

            commandAddShapeGeometryAndConstraints();

            int geoId = getHighestCurveIndex();
            geoEltIds.push_back(GeoElementId(geoId, pos));

            // Add the OVP constraints.
            addStepControlConstraints();

            // Gui::Command::commitCommand();

            // We stay in SeekSecond unless the user closed the PolyLine.
            bool isClosed = false;
            PointPos ac1Pos = pos;

            if (geoEltIds.size() == 1) {  // first line
                auto& ac0 = sugConstraints[0];
                generateAutoConstraintsOnElement(ac0, geoId, PointPos::start);
            }
            else {
                // check if coincident with first point
                for (auto& ac : sugConstraints[1]) {
                    if (ac.Type != Sketcher::Coincident && ac.Type != Sketcher::Tangent) {
                        continue;
                    }

                    if (ac.GeoId == geoEltIds[0].GeoId && ac.PosId == Sketcher::PointPos::start) {
                        isClosed = true;
                        continue;
                    }

                    // The coincidence with first point may be indirect
                    const auto coincidents
                        = sketchgui->getSketchObject()->getAllCoincidentPoints(ac.GeoId, ac.PosId);

                    auto it = coincidents.find(geoEltIds[0].GeoId);
                    if (it != coincidents.end() && it->second == Sketcher::PointPos::start) {
                        isClosed = true;
                    }
                }
            }

            auto& ac1 = sugConstraints[1];
            generateAutoConstraintsOnElement(ac1, geoId, ac1Pos);
            ac1.clear();

            removeRedundantAutoConstraints();

            createGeneratedAutoConstraints(false);
            sugConstraints[0].clear();
            sugConstraints[1].clear();
            AutoConstraints.clear();

            auto obj = sketchgui->getSketchObject();
            obj->solve();

            if (fillet && geoEltIds.size() > 1) {
                int geoId1 = geoEltIds.back().GeoId;
                int geoId2 = geoEltIds[geoEltIds.size() - 2].GeoId;

                const Part::Geometry* newGeo = obj->getGeometry(geoId1);
                const Part::Geometry* prevGeo = obj->getGeometry(geoId2);

                PointPos newGeoPos = geoEltIds.back().Pos == PointPos::start ? PointPos::end
                                                                             : PointPos::start;
                PointPos prevGeoPos = geoEltIds[geoEltIds.size() - 2].Pos;

                double radius;
                Base::Vector3d refPnt1, refPnt2;
                getFilletData(refPnt1, refPnt2, radius, newGeo, prevGeo, newGeoPos, prevGeoPos);

                obj->fillet(geoId1, geoId2, refPnt1, refPnt2, radius, true, true);

                if (isConstructionMode()) {
                    int filletGeoId = geoId + 1;
                    Gui::cmdAppObjectArgs(obj, "toggleConstruction(%d) ", filletGeoId);
                }
            }

            Base::Vector3d p3d = obj->getPoint(geoEltIds.back().GeoId, geoEltIds.back().Pos);
            points.push_back(toVector2d(p3d));
            calculatePreviousDirectionAngle();

            if (!isClosed) {
                setAngleSnapping(true, getLastPoint());
                resetSeekSecond = true;
                resetEdge = true;
                // redraw the OVPs
                auto snapHandle = std::make_unique<SnapManager::SnapHandle>(nullptr, prevCursorPos);
                mouseMove(*snapHandle);
            }
            return isClosed;
        }
        return true;
    }

    void calculatePreviousDirectionAngle()
    {
        previousDirectionAngle = getPreviousDirection().Angle();
    }

    Base::Vector2d getPreviousDirection()
    {
        if (points.size() <= 1) {
            return Base::Vector2d(1., 0.);
        }

        int geoId = geoEltIds.back().GeoId;

        auto* geom = sketchgui->getSketchObject()->getGeometry(geoId);
        if (!geom || !isArcOfCircle(*geom)) {
            return (points[points.size() - 2] - points[points.size() - 1]).Normalize();
        }

        auto* obj = sketchgui->getSketchObject();
        Base::Vector2d start = toVector2d(obj->getPoint(geoId, PointPos::start));
        Base::Vector2d end = toVector2d(obj->getPoint(geoId, PointPos::end));
        Base::Vector2d center = toVector2d(obj->getPoint(geoId, PointPos::mid));

        Base::Vector2d dir;
        if (geoEltIds.back().Pos == PointPos::end) {
            dir = end - center;
            dir = Base::Vector2d(dir.y, -dir.x);
        }
        else {
            dir = start - center;
            dir = Base::Vector2d(-dir.y, dir.x);
        }

        return dir.Normalize();
    }

    Base::Vector2d getCurrentInitialDirection()
    {
        Base::Vector2d dir;
        if (constructionMethod() == ConstructionMethod::Line) {
            return prevCursorPos - getLastPoint();
        }

        dir = getPreviousDirection();
        dir.Rotate(angleToPrevious);


        return dir;
    }

    bool isPreviousArc()
    {
        if (!geoEltIds.empty()) {
            auto* geo = sketchgui->getSketchObject()->getGeometry(geoEltIds.back().GeoId);
            return geo->is<Part::GeomArcOfCircle>();
        }
        return false;
    }

    void angleSnappingControl() override
    {
        if (state() == SelectMode::SeekSecond) {
            setAngleSnapping(true, getLastPoint());
        }
        else {
            setAngleSnapping(false);
        }
    }

    void quit() override
    {
        if (state() != SelectMode::SeekSecond) {
            DrawSketchHandler::quit();
            return;
        }

        if (geoEltIds.size() <= 0) {
            // We don't want to finish() as that'll create auto-constraints
            handleContinuousMode();
            return;
        }

        setState(SelectMode::End);
        finish();
    }

    void rightButtonOrEsc() override
    {
        quit();
    }

    void onReset() override
    {
        abortCommand();
        tryAutoRecomputeIfNotSolve(sketchgui->getSketchObject());
        openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch polyline"));

        geoEltIds.clear();
        points.clear();

        previousDirectionAngle = 0.0;
        dirChangeAngle = 0.0;
        angleToPrevious = 0.0;
        pos = PointPos::end;

        capturedDirection = Base::Vector2d(0.0, 0.0);

        toolWidgetManager.resetControls();

        setConstructionMethod(ConstructionMethod::Line);
        ensureFocus();
    }

    void undoLastPoint()
    {
        // can only delete last pole/knot if it exists
        if (state() != SelectMode::SeekSecond) {
            return;
        }

        if (geoEltIds.empty()) {
            // this also exits b-spline creation if continuous mode is off
            quit();
            return;
        }

        // reverse the steps of press/release button
        try {
            auto* obj = sketchgui->getSketchObject();
            int delGeoId = geoEltIds.back().GeoId;
            int lastGeoId = getHighestCurveIndex();
            // 3 cases :
            // - no fillet, then delGeoId == lastGeoId
            // - fillet between 2 lines, then delGeoId + 2 == lastGeoId
            // - fillet other, then delGeoId + 1 == lastGeoId (no points are created)
            bool filletWasCreated = delGeoId < lastGeoId;
            bool pointWasCreated = delGeoId + 2 == lastGeoId;

            geoEltIds.pop_back();
            points.pop_back();

            // Start by removing last edge and fillet arc if any
            if (filletWasCreated) {
                obj->delGeometries({delGeoId, delGeoId + 1});
            }
            else {
                obj->delGeometry(delGeoId);
            }

            if (!geoEltIds.empty()) {
                // Move back the previous edge point
                int prevGeoId = geoEltIds.back().GeoId;
                PointPos prevPos = geoEltIds.back().Pos;
                obj->moveGeometry(prevGeoId, prevPos, toVector3d(points.back()));

                // Then transfert back the constraints
                if (pointWasCreated) {
                    int pointGeoId = getHighestCurveIndex();
                    // We must first remove the point on object constraint.
                    const auto& constraints = sketchgui->getSketchObject()->Constraints.getValues();
                    for (int i = constraints.size() - 1; i >= 0; --i) {
                        if (constraints[i]->Type != PointOnObject) {
                            continue;
                        }
                        int first = constraints[i]->getGeoId(0);
                        int second = constraints[i]->getGeoId(1);
                        bool case1 = first == pointGeoId && second == prevGeoId;
                        bool case2 = first == prevGeoId && second == pointGeoId;
                        if (case1 || case2) {
                            obj->delConstraint(i);
                            break;
                        }
                    }

                    obj->transferConstraints(pointGeoId, PointPos::start, prevGeoId, prevPos);

                    // Delete the point
                    obj->delGeometry(delGeoId);
                }
            }

            obj->solve();

            calculatePreviousDirectionAngle();

            setAngleSnapping(true, getLastPoint());

            auto snapHandle = std::make_unique<SnapManager::SnapHandle>(nullptr, prevCursorPos);
            mouseMove(*snapHandle);
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Error deleting last pole/knot")
            );
            // some commands might have already deleted some constraints/geometries but not
            // others
            abortCommand();

            sketchgui->getSketchObject()->solve();

            return;
        }
    }

private:
    Base::Vector2d prevCursorPos, center;

    std::vector<Base::Vector2d> points;
    std::vector<GeoElementId> geoEltIds;
    bool resetSeekSecond;
    bool resetEdge;
    bool fillet;
    double previousDirectionAngle, dirChangeAngle, startAngle, range, angleToPrevious;
    PointPos pos;

    // Direction tracking to check once OVP is locked
    Base::Vector2d capturedDirection;

    Base::Vector2d getLastPoint()
    {
        return points.empty() ? Base::Vector2d() : points.back();
    }

    double getArcCenter(Base::Vector2d& center, Base::Vector2d pos)
    {
        Base::Vector2d lastPoint = getLastPoint();
        Base::Vector2d Tangent = getCurrentInitialDirection();
        double theta = Tangent.GetAngle(pos - lastPoint);
        double radius = (pos - lastPoint).Length() / (2.0 * sin(theta));

        double x1 = lastPoint.x;
        double y1 = lastPoint.y;
        double x2 = x1 + Tangent.x;
        double y2 = y1 + Tangent.y;
        double x3 = pos.x;
        double y3 = pos.y;
        if ((x2 * y3 - x3 * y2) - (x1 * y3 - x3 * y1) + (x1 * y2 - x2 * y1) > 0) {
            radius *= -1;
        }
        if (std::isnan(radius) || std::isinf(radius)) {
            radius = 0.0;
        }

        center = lastPoint + Base::Vector2d(radius * Tangent.y, -radius * Tangent.x);
        return radius;
    }

    void getFilletData(
        Base::Vector3d& refPnt1,
        Base::Vector3d& refPnt2,
        double& radius,
        const Part::Geometry* newGeo,
        const Part::Geometry* prevGeo,
        PointPos newGeoPos,
        PointPos prevGeoPos
    )
    {
        Base::Vector2d lastPoint = getLastPoint();

        Base::Vector2d lastButOnePoint = points[points.size() - 2];  // if geoEltIds not empty we
                                                                     // have at least 2 points

        radius = min(
            (prevCursorPos - lastPoint).Length() * 0.3,
            (lastButOnePoint - lastPoint).Length() * 0.3
        );

        refPnt1 = toVector3d(lastPoint + (prevCursorPos - lastPoint).Normalize() * radius);
        refPnt2 = toVector3d(lastPoint + (lastButOnePoint - lastPoint).Normalize() * radius);

        if (isLineSegment(*newGeo) && isLineSegment(*prevGeo)) {
            // guess fillet radius
            auto* line1 = static_cast<const Part::GeomLineSegment*>(newGeo);
            auto* line2 = static_cast<const Part::GeomLineSegment*>(prevGeo);

            radius = Part::suggestFilletRadius(line1, line2, refPnt1, refPnt2);
        }
        double start, end, newAngle;
        double devAngle = 0.3;  // about 15deg
        if (isArcOfCircle(*newGeo)) {
            auto* arc = static_cast<const Part::GeomArcOfCircle*>(newGeo);
            // Get arc properties
            Base::Vector3d center = arc->getCenter();
            double radius = arc->getRadius();
            arc->getRange(start, end, true);

            double arcAngle = end - start;
            if (newGeoPos == PointPos::end) {
                newAngle = end - std::min(arcAngle, devAngle);
            }
            else {
                newAngle = start + std::min(arcAngle, devAngle);
            }

            refPnt1 = Base::Vector3d(
                center.x + radius * cos(newAngle),
                center.y + radius * sin(newAngle)
            );
            if (isLineSegment(*prevGeo)) {
                refPnt2 = toVector3d(lastPoint)
                    + (refPnt2 - toVector3d(lastPoint)).Normalize()
                        * (toVector3d(lastPoint) - refPnt1).Length();
            }
        }
        if (isArcOfCircle(*prevGeo)) {
            auto* arc = static_cast<const Part::GeomArcOfCircle*>(prevGeo);
            // Get arc properties
            Base::Vector3d center = arc->getCenter();
            double radius = arc->getRadius();
            arc->getRange(start, end, true);

            double arcAngle = end - start;
            if (prevGeoPos == PointPos::end) {
                newAngle = end - std::min(arcAngle, devAngle);
            }
            else {
                newAngle = start + std::min(arcAngle, devAngle);
            }

            refPnt2 = Base::Vector3d(
                center.x + radius * cos(newAngle),
                center.y + radius * sin(newAngle)
            );
            if (isLineSegment(*newGeo)) {
                refPnt1 = toVector3d(lastPoint)
                    + (refPnt1 - toVector3d(lastPoint)).Normalize()
                        * (toVector3d(lastPoint) - refPnt2).Length();
            }
        }
    }

    void createShape(bool onlyeditoutline) override
    {
        using std::numbers::pi;
        ShapeGeometry.clear();
        ShapeConstraints.clear();

        if (points.size() < 1) {
            return;
        }
        Base::Vector2d lastPoint = getLastPoint();
        Base::Vector2d currentDir = prevCursorPos - lastPoint;
        if (currentDir.Length() < Precision::Confusion()) {
            resetEdge = true;
            return;
        }

        pos = PointPos::end;
        PointPos posId = PointPos::start;
        Base::Vector2d prevDir = getPreviousDirection();

        Part::Geometry* newGeo = nullptr;
        if (constructionMethod() == ConstructionMethod::Line) {
            newGeo = addLineToShapeGeometry(
                toVector3d(points[points.size() - 1]),
                toVector3d(prevCursorPos),
                isConstructionMode()
            );
        }
        else {
            if (resetEdge) {
                resetEdge = false;

                // We check the direction of the first mouse move to determine tangent or
                // perpendicular
                int sign = (prevDir.x * (prevCursorPos.y - lastPoint.y)
                            - prevDir.y * (prevCursorPos.x - lastPoint.x))
                        > 0
                    ? 1
                    : -1;

                angleToPrevious = sign * (currentDir.GetAngle(prevDir) - pi);
                angleToPrevious = std::round(angleToPrevious / (pi * 0.5)) * (pi * 0.5);
            }

            double radius = getArcCenter(center, prevCursorPos);

            if (radius == 0.0) {
                // fall back to a line
                addLineToShapeGeometry(
                    toVector3d(points[points.size() - 1]),
                    toVector3d(prevCursorPos),
                    isConstructionMode()
                );
            }

            double rx = lastPoint.x - center.x;
            double ry = lastPoint.y - center.y;

            startAngle = atan2(ry, rx);

            double rxe = prevCursorPos.x - center.x;
            double rye = prevCursorPos.y - center.y;
            range = atan2(-rxe * ry + rye * rx, rxe * rx + rye * ry);
            if (boost::math::isnan(range) || boost::math::isinf(range)) {
                range = 0.f;
            }
            if (radius >= 0 && range > 0) {
                range -= 2 * pi;
            }
            if (radius < 0 && range < 0) {
                range += 2 * pi;
            }

            double endAngle = startAngle + range;
            double startAngleToUseHere = startAngle;
            if (radius < 0) {
                std::swap(startAngleToUseHere, endAngle);
                posId = PointPos::end;
                pos = PointPos::start;
            }

            newGeo = addArcToShapeGeometry(
                toVector3d(center),
                startAngleToUseHere,
                endAngle,
                fabs(radius),
                isConstructionMode()
            );

            // range is weirdly handled before. We fix the range for the OVP
            if (radius < 0) {
                range = -2 * pi + range;
            }
            else {
                range = 2 * pi + range;
            }
        }

        if (onlyeditoutline && !geoEltIds.empty() && fillet) {
            auto* obj = sketchgui->getSketchObject();
            const Part::Geometry* prevGeo = obj->getGeometry(geoEltIds.back().GeoId);

            double radius;
            Base::Vector3d refPnt1, refPnt2;
            getFilletData(refPnt1, refPnt2, radius, newGeo, prevGeo, posId, geoEltIds.back().Pos);

            int pos1 = 0;
            int pos2 = 0;
            bool reverse = false;
            std::unique_ptr<Part::GeomArcOfCircle> arc(
                Part::createFilletGeometry(newGeo, prevGeo, refPnt1, refPnt2, radius, pos1, pos2, reverse)
            );
            if (arc) {
                if (isLineSegment(*newGeo)) {
                    Base::Vector3d newStart = reverse ? arc->getStartPoint() : arc->getEndPoint();
                    auto* line = static_cast<Part::GeomLineSegment*>(newGeo);
                    line->setPoints(newStart, line->getEndPoint());
                }

                ShapeGeometry.emplace_back(std::move(arc));
            }
        }

        if (!onlyeditoutline && !geoEltIds.empty()) {
            // Add constraints.
            int geoId = geoEltIds.back().GeoId;
            int geoId2 = getHighestCurveIndex() + 1;

            if (constructionMethod() == ConstructionMethod::Line) {
                auto& ac1 = sugConstraints[1];
                auto newEnd = std::remove_if(ac1.begin(), ac1.end(), [&](const AutoConstraint& c) {
                    return (c.Type == Tangent && c.GeoId == geoId && c.PosId == geoEltIds.back().Pos);
                });
                bool erased = (newEnd != ac1.end());  // Check if anything was removed
                ac1.erase(newEnd, ac1.end());

                if (erased) {
                    addToShapeConstraints(Sketcher::Tangent, geoId, geoEltIds.back().Pos, geoId2, posId);
                }
                else {
                    addToShapeConstraints(Sketcher::Coincident, geoId, geoEltIds.back().Pos, geoId2, posId);
                }
            }
            else {
                double roundedByPi = std::round(angleToPrevious / (pi)) * pi;
                double roundedByHalfPi = std::round(angleToPrevious / (pi * 0.5)) * (pi * 0.5);

                if (fabs(angleToPrevious - roundedByPi) < Precision::Confusion()) {
                    addToShapeConstraints(Sketcher::Tangent, geoId, geoEltIds.back().Pos, geoId2, posId);
                }
                else if (fabs(angleToPrevious - roundedByHalfPi) < Precision::Confusion()) {
                    addToShapeConstraints(
                        Sketcher::Perpendicular,
                        geoId,
                        geoEltIds.back().Pos,
                        geoId2,
                        posId
                    );
                }
            }
        }
    }
};

template<>
auto DSHPolyLineControllerBase::getState(int labelindex) const
{
    switch (labelindex) {
        case OnViewParameter::First:
        case OnViewParameter::Second:
            return SelectMode::SeekFirst;
            break;
        case OnViewParameter::Third:
        case OnViewParameter::Fourth:
        case OnViewParameter::Fifth:
            return SelectMode::SeekSecond;
            break;
        default:
            THROWM(Base::ValueError, "Label index without an associated machine state")
    }
}

template<>
void DSHPolyLineController::thirdKeyShortcut()
{
    handler->undoLastPoint();
}

template<>
void DSHPolyLineController::fourthKeyShortcut()
{
    auto firstchecked = toolWidget->getCheckboxChecked(WCheckbox::FirstBox);
    toolWidget->setCheckboxChecked(WCheckbox::FirstBox, !firstchecked);
}

template<>
void DSHPolyLineController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        toolWidget->setNoticeVisible(true);
        toolWidget->setNoticeText(
            QApplication::translate("TaskSketcherTool_c1_PolyLine", "R undoes the last point")
        );

        QStringList names = {
            QApplication::translate("Sketcher_CreatePolyline", "Line"),
            QApplication::translate("Sketcher_CreatePolyline", "Arc")
        };
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        toolWidget->setCheckboxLabel(
            WCheckbox::FirstBox,
            QApplication::translate("TaskSketcherTool_c1_PolyLine", "Fillet (F)")
        );
        toolWidget->setCheckboxToolTip(
            WCheckbox::FirstBox,
            QApplication::translate(
                "TaskSketcherTool_c1_PolyLine",
                "Adds a fillet between the current and previous line"
            )
        );
        syncCheckboxToHandler(WCheckbox::FirstBox, handler->fillet);

        if (isConstructionMode()) {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateLine_Constr")
            );
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc_Constr")
            );
            toolWidget->setCheckboxIcon(
                WCheckbox::FirstBox,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateFillet_Constr")
            );
        }
        else {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateLine")
            );
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc")
            );
            toolWidget->setCheckboxIcon(
                WCheckbox::FirstBox,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateFillet")
            );
        }
    }

    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);

    Gui::SoDatumLabel::Type thirdType = handler->constructionMethod() == ConstructionMethod::Line
        ? Gui::SoDatumLabel::DISTANCE
        : Gui::SoDatumLabel::RADIUS;

    onViewParameters[OnViewParameter::Third]->setLabelType(
        thirdType,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
    onViewParameters[OnViewParameter::Fourth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );

    if (handler->constructionMethod() == ConstructionMethod::Arc) {
        onViewParameters[OnViewParameter::Fifth]->setLabelType(
            Gui::SoDatumLabel::ANGLE,
            Gui::EditableDatumLabel::Function::Dimensioning
        );
    }

    toolWidget->setCheckboxChecked(WCheckbox::FirstBox, handler->fillet);
}

template<>
void DSHPolyLineController::adaptDrawingToCheckboxChange(int checkboxindex, bool value)
{
    switch (checkboxindex) {
        case WCheckbox::FirstBox:
            handler->fillet = value;
            break;
    }

    handler->updateCursor();
}

template<>
void DSHPolyLineControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (onViewParameters[OnViewParameter::First]->isSet) {
                onSketchPos.x = onViewParameters[OnViewParameter::First]->getValue();
            }

            if (onViewParameters[OnViewParameter::Second]->isSet) {
                onSketchPos.y = onViewParameters[OnViewParameter::Second]->getValue();
            }
        } break;
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            if (handler->resetSeekSecond) {
                handler->resetSeekSecond = false;
                unsetOnViewParameter(thirdParam.get());
                unsetOnViewParameter(fourthParam.get());
                if (handler->constructionMethod() == ConstructionMethod::Arc) {
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Fifth].get());
                }
                setFocusToOnViewParameter(OnViewParameter::Third);
                return;
            }

            Base::Vector2d prevPoint = handler->getLastPoint();

            if (handler->constructionMethod() == ConstructionMethod::Line) {
                Base::Vector2d dir = onSketchPos - prevPoint;

                if (fourthParam->isSet) {
                    const double angle = handler->previousDirectionAngle
                        + Base::toRadians(fourthParam->getValue());
                    const Base::Vector2d ovpDir(cos(angle), sin(angle));
                    handler->capturedDirection = ovpDir;
                }
                else {
                    handler->capturedDirection = dir.Normalize();
                }

                if (thirdParam->isSet) {
                    if (dir.Length() < Precision::Confusion()) {
                        dir.x = 1.0;  // if direction null, default to (1,0)
                    }
                    double length = thirdParam->getValue();
                    if (length < Precision::Confusion() && thirdParam->hasFinishedEditing) {
                        unsetOnViewParameter(thirdParam.get());
                        handler->capturedDirection = Base::Vector2d(0.0, 0.0);
                        return;
                    }

                    onSketchPos = prevPoint + length * handler->capturedDirection;
                }
                else if (fourthParam->isSet) {
                    onSketchPos.ProjectToLine(onSketchPos - prevPoint, handler->capturedDirection);
                    onSketchPos += prevPoint;
                }

                if (thirdParam->hasFinishedEditing && fourthParam->hasFinishedEditing
                    && (onSketchPos - prevPoint).Length() < Precision::Confusion()) {
                    unsetOnViewParameter(thirdParam.get());
                    unsetOnViewParameter(fourthParam.get());
                    handler->capturedDirection = Base::Vector2d();
                }
            }
            else {
                if (thirdParam->isSet) {
                    double radius = thirdParam->getValue();
                    if (radius < Precision::Confusion()) {
                        unsetOnViewParameter(thirdParam.get());
                        return;
                    }

                    Base::Vector2d dir = handler->getPreviousDirection();
                    dir.Rotate(handler->angleToPrevious);

                    int sign = (dir.x * (onSketchPos.y - prevPoint.y)
                                - dir.y * (onSketchPos.x - prevPoint.x))
                            > 0
                        ? 1
                        : -1;

                    Base::Vector2d normal = sign * Base::Vector2d(-dir.y, dir.x);
                    Base::Vector2d center = prevPoint + radius * normal.Normalize();

                    dir = onSketchPos - center;
                    onSketchPos = center + radius * dir.Normalize();
                }
                if (fourthParam->isSet) {
                    Base::Vector2d center;
                    ;
                    double radius = handler->getArcCenter(center, onSketchPos);
                    int sign = radius < 0 ? -1 : 1;
                    radius = fabs(radius);
                    double range = Base::toRadians(fourthParam->getValue());
                    double angle = handler->startAngle + sign * range;
                    Base::Vector2d dir(1.0, 0.0);
                    dir.Rotate(angle);
                    onSketchPos = center + dir * radius;
                }
                if (onViewParameters[OnViewParameter::Fifth]->isSet) {
                    double angle = Base::toRadians(
                        onViewParameters[OnViewParameter::Fifth]->getValue()
                    );

                    if (handler->angleToPrevious != angle) {
                        handler->angleToPrevious = angle;
                    }
                }
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHPolyLineController::adaptParameters(Base::Vector2d onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (!firstParam->isSet) {
                setOnViewParameterValue(OnViewParameter::First, onSketchPos.x);
            }

            if (!secondParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Second, onSketchPos.y);
            }

            bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
            firstParam->setLabelAutoDistanceReverse(!sameSign);
            secondParam->setLabelAutoDistanceReverse(sameSign);
            firstParam->setPoints(Base::Vector3d(), toVector3d(onSketchPos));
            secondParam->setPoints(Base::Vector3d(), toVector3d(onSketchPos));
        } break;
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            Base::Vector2d prevPoint;
            if (!handler->points.empty()) {
                prevPoint = handler->getLastPoint();
            }
            if (handler->constructionMethod() == ConstructionMethod::Line) {
                Base::Vector3d start = toVector3d(prevPoint);
                Base::Vector3d end = toVector3d(onSketchPos);
                Base::Vector3d vec = end - start;

                if (!thirdParam->isSet) {
                    setOnViewParameterValue(OnViewParameter::Third, vec.Length());
                }

                double range = Base::toDegrees(handler->dirChangeAngle);
                if (!fourthParam->isSet) {
                    setOnViewParameterValue(OnViewParameter::Fourth, range, Base::Unit::Angle);
                }
                else if (vec.Length() > Precision::Confusion()) {
                    double ovpRange = fourthParam->getValue();

                    if (fabs(range - ovpRange) > Precision::Confusion()) {
                        setOnViewParameterValue(OnViewParameter::Fourth, range, Base::Unit::Angle);
                    }
                }

                thirdParam->setPoints(start, end);
                fourthParam->setPoints(start, Base::Vector3d());
                fourthParam->setLabelStartAngle(handler->previousDirectionAngle);
                fourthParam->setLabelRange(handler->dirChangeAngle);
            }
            else {
                auto& fifthParam = onViewParameters[OnViewParameter::Fifth];

                Base::Vector3d start = toVector3d(handler->center);
                Base::Vector3d end = toVector3d(onSketchPos);
                Base::Vector3d vec = end - start;

                if (!thirdParam->isSet) {
                    setOnViewParameterValue(OnViewParameter::Third, vec.Length());
                }

                double range = Base::toDegrees(handler->range);
                if (!fourthParam->isSet) {
                    setOnViewParameterValue(OnViewParameter::Fourth, range, Base::Unit::Angle);
                }

                if (!fifthParam->isSet) {
                    double angle = Base::toDegrees(handler->angleToPrevious);
                    setOnViewParameterValue(OnViewParameter::Fifth, angle, Base::Unit::Angle);
                }

                thirdParam->setPoints(start, end);
                fourthParam->setPoints(start, Base::Vector3d());
                fourthParam->setLabelStartAngle(handler->startAngle);
                fourthParam->setLabelRange(handler->range);

                fifthParam->setPoints(toVector3d(prevPoint), Base::Vector3d());
                fifthParam->setLabelStartAngle(handler->previousDirectionAngle);
                fifthParam->setLabelRange(handler->angleToPrevious);
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHPolyLineController::computeNextDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (firstParam->hasFinishedEditing && secondParam->hasFinishedEditing) {
                double x = onViewParameters[OnViewParameter::First]->getValue();
                double y = onViewParameters[OnViewParameter::Second]->getValue();
                handler->onButtonPressed(Base::Vector2d(x, y));
            }
        } break;
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            if (thirdParam->hasFinishedEditing && fourthParam->hasFinishedEditing) {
                handler->canGoToNextMode();  // its not going to next mode

                unsetOnViewParameter(onViewParameters[OnViewParameter::Third].get());
                unsetOnViewParameter(onViewParameters[OnViewParameter::Fourth].get());
                if (handler->constructionMethod() == ConstructionMethod::Arc) {
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Fifth].get());
                }
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHPolyLineController::doConstructionMethodChanged()
{
    // First segment cannot be an arc (yet?). So if user changed the mode, we roll that back.
    if (handler->constructionMethod() == ConstructionMethod::Arc) {
        if (handler->geoEltIds.empty()) {
            handler->setConstructionMethod(ConstructionMethod::Line);
            return;
        }
    }

    // Since line has 4 OVP but arc has 5, and because we are not reseting the whole tool,
    // we need to reset the OVP to have the correct number.
    resetOnViewParameters();

    configureToolWidget();

    setModeOnViewParameters();

    syncConstructionMethodComboboxToHandler();
}

template<>
bool DSHPolyLineControllerBase::resetOnConstructionMethodeChanged()
{
    return false;
}

template<>
void DSHPolyLineController::addStepConstraints()
{
    App::DocumentObject* obj = handler->sketchgui->getObject();

    int lastCurve = handler->getHighestCurveIndex();

    if (handler->geoEltIds.size() == 1) {
        auto x0 = onViewParameters[OnViewParameter::First]->getValue();
        auto y0 = onViewParameters[OnViewParameter::Second]->getValue();

        auto x0set = onViewParameters[OnViewParameter::First]->isSet;
        auto y0set = onViewParameters[OnViewParameter::Second]->isSet;

        auto constraintToOrigin = [&]() {
            ConstraintToAttachment(GeoElementId(lastCurve, PointPos::start), GeoElementId::RtPnt, x0, obj);
        };

        auto constraintx0 = [&]() {
            ConstraintToAttachment(GeoElementId(lastCurve, PointPos::start), GeoElementId::VAxis, x0, obj);
        };

        auto constrainty0 = [&]() {
            ConstraintToAttachment(GeoElementId(lastCurve, PointPos::start), GeoElementId::HAxis, y0, obj);
        };

        if (x0set && y0set && x0 == 0. && y0 == 0.) {
            constraintToOrigin();
        }
        else {
            if (x0set) {
                constraintx0();
            }

            if (y0set) {
                constrainty0();
            }
        }

        deactivateOnViewParameter(OnViewParameter::First);
        deactivateOnViewParameter(OnViewParameter::Second);
    }

    auto p3 = onViewParameters[OnViewParameter::Third]->getValue();
    auto p4 = onViewParameters[OnViewParameter::Fourth]->getValue();

    auto p3set = onViewParameters[OnViewParameter::Third]->isSet;
    auto p4set = onViewParameters[OnViewParameter::Fourth]->isSet;

    if (handler->constructionMethod() == ConstructionMethod::Line) {
        if (p3set) {
            Gui::cmdAppObjectArgs(
                obj,
                "addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
                lastCurve,
                fabs(p3)
            );
        }

        if (p4set) {
            if (handler->geoEltIds.size() > 1) {
                if (!handler->isPreviousArc()) {
                    int geoId2 = handler->geoEltIds[handler->geoEltIds.size() - 2].GeoId;
                    Constraint2LinesByAngle(lastCurve, geoId2, Base::toRadians(p4), obj);
                }
            }
            else {
                ConstraintLineByAngle(lastCurve, Base::toRadians(p4), obj);
            }
        }
    }
    else {
        if (p3set) {
            Gui::cmdAppObjectArgs(
                obj,
                "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                lastCurve,
                fabs(p3)
            );
        }

        if (p4set) {
            Gui::cmdAppObjectArgs(
                obj,
                "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
                lastCurve,
                fabs(Base::toRadians(p4))
            );
        }
    }
}

}  // namespace SketcherGui

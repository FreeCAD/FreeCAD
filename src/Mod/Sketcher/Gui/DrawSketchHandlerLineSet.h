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

class DrawSketchHandlerLineSet: public DrawSketchHandler
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerLineSet)

public:
    DrawSketchHandlerLineSet()
        : Mode(STATUS_SEEK_First)
        , SegmentMode(SEGMENT_MODE_Line)
        , TransitionMode(TRANSITION_MODE_Free)
        , SnapMode(SNAP_MODE_Free)
        , suppressTransition(false)
        , EditCurve(2)
        , firstCurve(-1)
        , previousCurve(-1)
        , firstPosId(Sketcher::PointPos::none)
        , previousPosId(Sketcher::PointPos::none)
        , startAngle(0)
        , endAngle(0)
        , arcRadius(0)
        , firstsegment(true)
    {}

    ~DrawSketchHandlerLineSet() override = default;

    /// mode table
    enum SELECT_MODE
    {
        STATUS_SEEK_First,
        STATUS_SEEK_Second,
        STATUS_Do,
        STATUS_Close
    };

    enum SEGMENT_MODE
    {
        SEGMENT_MODE_Arc,
        SEGMENT_MODE_Line
    };

    enum TRANSITION_MODE
    {
        TRANSITION_MODE_Free,
        TRANSITION_MODE_Tangent,
        TRANSITION_MODE_Perpendicular_L,
        TRANSITION_MODE_Perpendicular_R
    };

    enum SNAP_MODE
    {
        SNAP_MODE_Free,
        SNAP_MODE_45Degree
    };


    void registerPressedKey(bool pressed, int key) override
    {
        if (Mode == STATUS_SEEK_Second && key == SoKeyboardEvent::M && pressed
            && previousCurve != -1) {
            // loop through the following modes:
            // SEGMENT_MODE_Line, TRANSITION_MODE_Free / TRANSITION_MODE_Tangent
            // SEGMENT_MODE_Line, TRANSITION_MODE_Perpendicular_L
            // SEGMENT_MODE_Line, TRANSITION_MODE_Tangent / TRANSITION_MODE_Free
            // SEGMENT_MODE_Arc, TRANSITION_MODE_Tangent
            // SEGMENT_MODE_Arc, TRANSITION_MODE_Perpendicular_L
            // SEGMENT_MODE_Arc, TRANSITION_MODE_Perpendicular_R

            SnapMode = SNAP_MODE_Free;

            Base::Vector2d onSketchPos;
            if (SegmentMode == SEGMENT_MODE_Line) {
                onSketchPos = EditCurve[EditCurve.size() - 1];
            }
            else {
                onSketchPos = EditCurve[29];
            }

            const Part::Geometry* geom = sketchgui->getSketchObject()->getGeometry(previousCurve);

            if (SegmentMode == SEGMENT_MODE_Line) {
                switch (TransitionMode) {
                    case TRANSITION_MODE_Free:
                        if (geom->is<Part::GeomArcOfCircle>()) {  // 3rd mode
                            SegmentMode = SEGMENT_MODE_Arc;
                            TransitionMode = TRANSITION_MODE_Tangent;
                        }
                        else {  // 1st mode
                            TransitionMode = TRANSITION_MODE_Perpendicular_L;
                        }
                        break;
                    case TRANSITION_MODE_Perpendicular_L:  // 2nd mode
                        if (geom->is<Part::GeomArcOfCircle>()) {
                            TransitionMode = TRANSITION_MODE_Free;
                        }
                        else {
                            TransitionMode = TRANSITION_MODE_Tangent;
                        }
                        break;
                    case TRANSITION_MODE_Tangent:
                        if (geom->is<Part::GeomArcOfCircle>()) {  // 1st mode
                            TransitionMode = TRANSITION_MODE_Perpendicular_L;
                        }
                        else {  // 3rd mode
                            SegmentMode = SEGMENT_MODE_Arc;
                            TransitionMode = TRANSITION_MODE_Tangent;
                        }
                        break;
                    default:  // unexpected mode
                        TransitionMode = TRANSITION_MODE_Free;
                        break;
                }
            }
            else {
                switch (TransitionMode) {
                    case TRANSITION_MODE_Tangent:  // 4th mode
                        TransitionMode = TRANSITION_MODE_Perpendicular_L;
                        break;
                    case TRANSITION_MODE_Perpendicular_L:  // 5th mode
                        TransitionMode = TRANSITION_MODE_Perpendicular_R;
                        break;
                    default:  // 6th mode (Perpendicular_R) + unexpected mode
                        SegmentMode = SEGMENT_MODE_Line;
                        if (geom->is<Part::GeomArcOfCircle>()) {
                            TransitionMode = TRANSITION_MODE_Tangent;
                        }
                        else {
                            TransitionMode = TRANSITION_MODE_Free;
                        }
                        break;
                }
            }

            if (SegmentMode == SEGMENT_MODE_Line) {
                EditCurve.resize(TransitionMode == TRANSITION_MODE_Free ? 2 : 3);
            }
            else {
                EditCurve.resize(32);
            }
            auto snapHandle = std::make_unique<SnapManager::SnapHandle>(nullptr, onSketchPos);
            mouseMove(*snapHandle);  // trigger an update of EditCurve
        }
        else {
            DrawSketchHandler::registerPressedKey(pressed, key);
        }
    }

    void mouseMove(SnapManager::SnapHandle snapHandle) override
    {
        using std::numbers::pi;
        Base::Vector2d onSketchPos = snapHandle.compute();

        suppressTransition = false;
        if (Mode == STATUS_SEEK_First) {
            setPositionText(onSketchPos);
            seekAndRenderAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.f, 0.f));
        }
        else if (Mode == STATUS_SEEK_Second) {
            if (SegmentMode == SEGMENT_MODE_Line) {
                EditCurve[EditCurve.size() - 1] = onSketchPos;
                if (TransitionMode == TRANSITION_MODE_Tangent) {
                    Base::Vector2d Tangent(dirVec.x, dirVec.y);
                    EditCurve[1].ProjectToLine(EditCurve[2] - EditCurve[0], Tangent);
                    if (EditCurve[1] * Tangent < 0) {
                        EditCurve[1] = EditCurve[2];
                        suppressTransition = true;
                    }
                    else {
                        EditCurve[1] = EditCurve[0] + EditCurve[1];
                    }
                }
                else if (
                    TransitionMode == TRANSITION_MODE_Perpendicular_L
                    || TransitionMode == TRANSITION_MODE_Perpendicular_R
                ) {
                    Base::Vector2d Perpendicular(-dirVec.y, dirVec.x);
                    EditCurve[1].ProjectToLine(EditCurve[2] - EditCurve[0], Perpendicular);
                    EditCurve[1] = EditCurve[0] + EditCurve[1];
                }

                drawEdit(EditCurve);

                float length = (EditCurve[1] - EditCurve[0]).Length();
                float angle = (EditCurve[1] - EditCurve[0]).GetAngle(Base::Vector2d(1.f, 0.f));

                if (showCursorCoords()) {
                    SbString text;
                    std::string lengthString = lengthToDisplayFormat(length, 1);
                    std::string angleString = angleToDisplayFormat(angle * 180.0 / pi, 1);
                    text.sprintf(" (%s, %s)", lengthString.c_str(), angleString.c_str());
                    setPositionText(EditCurve[1], text);
                }

                if (TransitionMode == TRANSITION_MODE_Free) {
                    seekAndRenderAutoConstraint(sugConstr2, onSketchPos, onSketchPos - EditCurve[0]);
                }
            }
            else if (SegmentMode == SEGMENT_MODE_Arc) {

                if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
                    SnapMode = SNAP_MODE_45Degree;
                }
                else {
                    SnapMode = SNAP_MODE_Free;
                }

                Base::Vector2d Tangent;
                if (TransitionMode == TRANSITION_MODE_Tangent) {
                    Tangent = Base::Vector2d(dirVec.x, dirVec.y);
                }
                else if (TransitionMode == TRANSITION_MODE_Perpendicular_L) {
                    Tangent = Base::Vector2d(-dirVec.y, dirVec.x);
                }
                else if (TransitionMode == TRANSITION_MODE_Perpendicular_R) {
                    Tangent = Base::Vector2d(dirVec.y, -dirVec.x);
                }

                double theta = Tangent.GetAngle(onSketchPos - EditCurve[0]);

                arcRadius = (onSketchPos - EditCurve[0]).Length() / (2.0 * sin(theta));

                // At this point we need a unit normal vector pointing towards
                // the center of the arc we are drawing. Derivation of the formula
                // used here can be found at
                // http://people.richland.edu/james/lecture/m116/matrices/area.html
                double x1 = EditCurve[0].x;
                double y1 = EditCurve[0].y;
                double x2 = x1 + Tangent.x;
                double y2 = y1 + Tangent.y;
                double x3 = onSketchPos.x;
                double y3 = onSketchPos.y;
                if ((x2 * y3 - x3 * y2) - (x1 * y3 - x3 * y1) + (x1 * y2 - x2 * y1) > 0) {
                    arcRadius *= -1;
                }
                if (boost::math::isnan(arcRadius) || boost::math::isinf(arcRadius)) {
                    arcRadius = 0.f;
                }

                CenterPoint = EditCurve[0]
                    + Base::Vector2d(arcRadius * Tangent.y, -arcRadius * Tangent.x);

                double rx = EditCurve[0].x - CenterPoint.x;
                double ry = EditCurve[0].y - CenterPoint.y;

                startAngle = atan2(ry, rx);

                double rxe = onSketchPos.x - CenterPoint.x;
                double rye = onSketchPos.y - CenterPoint.y;
                double arcAngle = atan2(-rxe * ry + rye * rx, rxe * rx + rye * ry);
                if (boost::math::isnan(arcAngle) || boost::math::isinf(arcAngle)) {
                    arcAngle = 0.f;
                }
                if (arcRadius >= 0 && arcAngle > 0) {
                    arcAngle -= 2 * pi;
                }
                if (arcRadius < 0 && arcAngle < 0) {
                    arcAngle += 2 * pi;
                }

                if (SnapMode == SNAP_MODE_45Degree) {
                    arcAngle = round(arcAngle / (pi / 4)) * pi / 4;
                }

                endAngle = startAngle + arcAngle;

                for (int i = 1; i <= 29; i++) {
                    double angle = i * arcAngle / 29.0;
                    double dx = rx * cos(angle) - ry * sin(angle);
                    double dy = rx * sin(angle) + ry * cos(angle);
                    EditCurve[i] = Base::Vector2d(CenterPoint.x + dx, CenterPoint.y + dy);
                }

                EditCurve[30] = CenterPoint;
                EditCurve[31] = EditCurve[0];

                drawEdit(EditCurve);

                if (showCursorCoords()) {
                    SbString text;
                    std::string radiusString = lengthToDisplayFormat(std::abs(arcRadius), 1);
                    std::string angleString = angleToDisplayFormat(arcAngle * 180.0 / pi, 1);
                    text.sprintf(" (R%s, %s)", radiusString.c_str(), angleString.c_str());
                    setPositionText(onSketchPos, text);
                }

                seekAndRenderAutoConstraint(sugConstr2, onSketchPos, Base::Vector2d(0.f, 0.f));
            }
        }
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {

        if (Mode == STATUS_SEEK_First) {

            EditCurve[0] = onSketchPos;  // this may be overwritten if previousCurve is found

            virtualsugConstr1 = sugConstr1;  // store original autoconstraints.

            // here we check if there is a preselected point and
            // we set up a transition from the neighbouring segment.
            // (peviousCurve, previousPosId, dirVec, TransitionMode)
            for (unsigned int i = 0; i < sugConstr1.size(); i++) {
                if (sugConstr1[i].Type == Sketcher::Coincident) {
                    const Part::Geometry* geom = sketchgui->getSketchObject()->getGeometry(
                        sugConstr1[i].GeoId
                    );
                    if ((geom->is<Part::GeomLineSegment>() || geom->is<Part::GeomArcOfCircle>())
                        && (sugConstr1[i].PosId == Sketcher::PointPos::start
                            || sugConstr1[i].PosId == Sketcher::PointPos::end)) {
                        previousCurve = sugConstr1[i].GeoId;
                        previousPosId = sugConstr1[i].PosId;
                        updateTransitionData(
                            previousCurve,
                            previousPosId
                        );  // -> dirVec, EditCurve[0]
                        if (geom->is<Part::GeomArcOfCircle>()) {
                            TransitionMode = TRANSITION_MODE_Tangent;
                            SnapMode = SNAP_MODE_Free;
                        }
                        sugConstr1.erase(sugConstr1.begin() + i);  // actually we should clear the
                                                                   // vector completely
                        break;
                    }
                }
            }

            // remember our first point (even if we are doing a transition from a previous curve)
            firstCurve = getHighestCurveIndex() + 1;
            firstPosId = Sketcher::PointPos::start;

            if (SegmentMode == SEGMENT_MODE_Line) {
                EditCurve.resize(TransitionMode == TRANSITION_MODE_Free ? 2 : 3);
            }
            else if (SegmentMode == SEGMENT_MODE_Arc) {
                EditCurve.resize(32);
            }
            Mode = STATUS_SEEK_Second;
        }
        else if (Mode == STATUS_SEEK_Second) {
            // exit on clicking exactly at the same position (e.g. double click)
            if (onSketchPos == EditCurve[0]) {
                unsetCursor();
                resetPositionText();
                EditCurve.clear();
                drawEdit(EditCurve);

                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/Mod/Sketcher"
                );
                bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);

                if (continuousMode) {
                    // This code enables the continuous creation mode.
                    Mode = STATUS_SEEK_First;
                    SegmentMode = SEGMENT_MODE_Line;
                    TransitionMode = TRANSITION_MODE_Free;
                    SnapMode = SNAP_MODE_Free;
                    suppressTransition = false;
                    firstCurve = -1;
                    previousCurve = -1;
                    firstPosId = Sketcher::PointPos::none;
                    previousPosId = Sketcher::PointPos::none;
                    EditCurve.clear();
                    drawEdit(EditCurve);
                    EditCurve.resize(2);
                    applyCursor();
                    /* this is ok not to call to purgeHandler
                     * in continuous creation mode because the
                     * handler is destroyed by the quit() method on pressing the
                     * right button of the mouse */
                    return true;
                }
                else {
                    sketchgui->purgeHandler();  // no code after this line, Handler get deleted in
                                                // ViewProvider
                    return true;
                }
            }

            Mode = STATUS_Do;

            if (getPreselectPoint() != -1 && firstPosId != Sketcher::PointPos::none) {
                int GeoId;
                Sketcher::PointPos PosId;
                sketchgui->getSketchObject()->getGeoVertexIndex(getPreselectPoint(), GeoId, PosId);
                if (sketchgui->getSketchObject()
                        ->arePointsCoincident(GeoId, PosId, firstCurve, firstPosId)) {
                    Mode = STATUS_Close;
                }
            }
            else if (getPreselectCross() == 0 && firstPosId != Sketcher::PointPos::none) {
                // close line started at root point
                if (sketchgui->getSketchObject()
                        ->arePointsCoincident(-1, Sketcher::PointPos::start, firstCurve, firstPosId)) {
                    Mode = STATUS_Close;
                }
            }
        }

        updateHint();

        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_Do || Mode == STATUS_Close) {
            bool addedGeometry = true;
            if (SegmentMode == SEGMENT_MODE_Line) {
                // issue the geometry
                try {
                    // open the transaction
                    openCommand(QT_TRANSLATE_NOOP("Command", "Add line to sketch polyline"));
                    Gui::cmdAppObjectArgs(
                        sketchgui->getObject(),
                        "addGeometry(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)),%s)",
                        EditCurve[0].x,
                        EditCurve[0].y,
                        EditCurve[1].x,
                        EditCurve[1].y,
                        constructionModeAsBooleanText()
                    );
                }
                catch (const Base::Exception&) {
                    addedGeometry = false;
                    Gui::NotifyError(
                        sketchgui,
                        QT_TRANSLATE_NOOP("Notifications", "Error"),
                        QT_TRANSLATE_NOOP("Notifications", "Failed to add line")
                    );
                    abortCommand();
                }

                firstsegment = false;
            }
            else if (SegmentMode == SEGMENT_MODE_Arc) {  // We're dealing with an Arc
                if (!boost::math::isnormal(arcRadius)) {
                    Mode = STATUS_SEEK_Second;
                    return true;
                }

                try {
                    openCommand(QT_TRANSLATE_NOOP("Command", "Add arc to sketch polyline"));
                    Gui::cmdAppObjectArgs(
                        sketchgui->getObject(),
                        "addGeometry(Part.ArcOfCircle"
                        "(Part.Circle(App.Vector(%f,%f,0),App.Vector(0,0,1),%f),%f,%f),%s)",
                        CenterPoint.x,
                        CenterPoint.y,
                        std::abs(arcRadius),
                        std::min(startAngle, endAngle),
                        std::max(startAngle, endAngle),
                        constructionModeAsBooleanText()
                    );
                }
                catch (const Base::Exception&) {
                    addedGeometry = false;
                    Gui::NotifyError(
                        sketchgui,
                        QT_TRANSLATE_NOOP("Notifications", "Error"),
                        QT_TRANSLATE_NOOP("Notifications", "Failed to add arc")
                    );

                    abortCommand();
                }

                firstsegment = false;
            }

            int lastCurve = getHighestCurveIndex();
            // issue the constraint
            if (addedGeometry && (previousPosId != Sketcher::PointPos::none)) {
                Sketcher::PointPos lastStartPosId = (SegmentMode == SEGMENT_MODE_Arc
                                                     && startAngle > endAngle)
                    ? Sketcher::PointPos::end
                    : Sketcher::PointPos::start;
                Sketcher::PointPos lastEndPosId = (SegmentMode == SEGMENT_MODE_Arc
                                                   && startAngle > endAngle)
                    ? Sketcher::PointPos::start
                    : Sketcher::PointPos::end;
                // in case of a tangency constraint, the coincident constraint is redundant
                std::string constrType = "Coincident";
                if (!suppressTransition && previousCurve != -1) {
                    if (TransitionMode == TRANSITION_MODE_Tangent) {
                        constrType = "Tangent";
                    }
                    else if (
                        TransitionMode == TRANSITION_MODE_Perpendicular_L
                        || TransitionMode == TRANSITION_MODE_Perpendicular_R
                    ) {
                        constrType = "Perpendicular";
                    }
                }
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('%s',%i,%i,%i,%i)) ",
                    constrType.c_str(),
                    previousCurve,
                    static_cast<int>(previousPosId),
                    lastCurve,
                    static_cast<int>(lastStartPosId)
                );

                if (SnapMode == SNAP_MODE_45Degree && Mode != STATUS_Close) {
                    // -360, -315, -270, -225, -180, -135, -90, -45,  0, 45,  90, 135, 180, 225,
                    // 270, 315, 360
                    //  N/A,    a, perp,    a,  par,    a,perp,   a,N/A,  a,perp,   a, par, a,perp,
                    //  a, N/A

                    // #3974: if in radians, the printf %f defaults to six decimals, which leads to
                    // loss of precision
                    double arcAngle = abs(
                        round((endAngle - startAngle) / (std::numbers::pi / 4)) * 45
                    );  // in degrees

                    Gui::cmdAppObjectArgs(
                        sketchgui->getObject(),
                        "addConstraint(Sketcher.Constraint('Angle',%i,App.Units."
                        "Quantity('%f deg'))) ",
                        lastCurve,
                        arcAngle
                    );
                }
                if (Mode == STATUS_Close) {
                    // close the loop by constrain to the first curve point
                    Gui::cmdAppObjectArgs(
                        sketchgui->getObject(),
                        "addConstraint(Sketcher.Constraint('Coincident',%i,%i,%i,%i)) ",
                        lastCurve,
                        static_cast<int>(lastEndPosId),
                        firstCurve,
                        static_cast<int>(firstPosId)
                    );
                    firstsegment = true;
                }
                commitCommand();

                tryAutoRecomputeIfNotSolve(sketchgui->getObject<Sketcher::SketchObject>());
            }

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher"
            );
            bool avoidredundant = sketchgui->AvoidRedundant.getValue()
                && sketchgui->Autoconstraints.getValue();

            if (Mode == STATUS_Close) {

                if (avoidredundant) {
                    if (SegmentMode == SEGMENT_MODE_Line) {  // avoid redundant constraints.
                        if (sugConstr1.size() > 0) {
                            removeRedundantHorizontalVertical(
                                sketchgui->getObject<Sketcher::SketchObject>(),
                                sugConstr1,
                                sugConstr2
                            );
                        }
                        else {
                            removeRedundantHorizontalVertical(
                                sketchgui->getObject<Sketcher::SketchObject>(),
                                virtualsugConstr1,
                                sugConstr2
                            );
                        }
                    }
                }

                if (!sugConstr2.empty()) {
                    // exclude any coincidence constraints
                    std::vector<AutoConstraint> sugConstr;
                    for (unsigned int i = 0; i < sugConstr2.size(); i++) {
                        if (sugConstr2[i].Type != Sketcher::Coincident) {
                            sugConstr.push_back(sugConstr2[i]);
                        }
                    }
                    createAutoConstraints(sugConstr, getHighestCurveIndex(), Sketcher::PointPos::end);
                    sugConstr2.clear();
                }

                tryAutoRecomputeIfNotSolve(sketchgui->getObject<Sketcher::SketchObject>());

                unsetCursor();

                resetPositionText();
                EditCurve.clear();
                drawEdit(EditCurve);

                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/Mod/Sketcher"
                );
                bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);

                if (continuousMode) {
                    // This code enables the continuous creation mode.
                    Mode = STATUS_SEEK_First;
                    SegmentMode = SEGMENT_MODE_Line;
                    TransitionMode = TRANSITION_MODE_Free;
                    SnapMode = SNAP_MODE_Free;
                    suppressTransition = false;
                    firstCurve = -1;
                    previousCurve = -1;
                    firstPosId = Sketcher::PointPos::none;
                    previousPosId = Sketcher::PointPos::none;
                    EditCurve.clear();
                    drawEdit(EditCurve);
                    EditCurve.resize(2);
                    applyCursor();
                    /* this is ok not to call to purgeHandler
                     * in continuous creation mode because the
                     * handler is destroyed by the quit() method on pressing the
                     * right button of the mouse */
                }
                else {
                    sketchgui->purgeHandler();  // no code after this line, Handler get deleted in
                                                // ViewProvider
                }
            }
            else {
                commitCommand();

                // Add auto constraints
                if (!sugConstr1.empty()) {  // this is relevant only to the very first point
                    createAutoConstraints(sugConstr1, getHighestCurveIndex(), Sketcher::PointPos::start);
                    sugConstr1.clear();
                }


                if (avoidredundant) {
                    if (SegmentMode == SEGMENT_MODE_Line) {  // avoid redundant constraints.
                        if (sugConstr1.size() > 0) {
                            removeRedundantHorizontalVertical(
                                sketchgui->getObject<Sketcher::SketchObject>(),
                                sugConstr1,
                                sugConstr2
                            );
                        }
                        else {
                            removeRedundantHorizontalVertical(
                                sketchgui->getObject<Sketcher::SketchObject>(),
                                virtualsugConstr1,
                                sugConstr2
                            );
                        }
                    }
                }

                virtualsugConstr1 = sugConstr2;  // these are the initial constraints for the next
                                                 // iteration.

                if (!sugConstr2.empty()) {
                    createAutoConstraints(
                        sugConstr2,
                        getHighestCurveIndex(),
                        (SegmentMode == SEGMENT_MODE_Arc && startAngle > endAngle)
                            ? Sketcher::PointPos::start
                            : Sketcher::PointPos::end
                    );
                    sugConstr2.clear();
                }

                tryAutoRecomputeIfNotSolve(sketchgui->getObject<Sketcher::SketchObject>());

                // remember the vertex for the next rounds constraint..
                previousCurve = getHighestCurveIndex();
                previousPosId = (SegmentMode == SEGMENT_MODE_Arc && startAngle > endAngle)
                    ? Sketcher::PointPos::start
                    : Sketcher::PointPos::end;  // cw arcs are rendered in reverse

                // setup for the next line segment
                // calculate dirVec and EditCurve[0]
                updateTransitionData(previousCurve, previousPosId);

                applyCursor();
                Mode = STATUS_SEEK_Second;
                if (SegmentMode == SEGMENT_MODE_Arc) {
                    TransitionMode = TRANSITION_MODE_Tangent;
                    EditCurve.resize(3);
                    EditCurve[2] = EditCurve[0];
                }
                else {
                    TransitionMode = TRANSITION_MODE_Free;
                    EditCurve.resize(2);
                }
                SegmentMode = SEGMENT_MODE_Line;
                SnapMode = SNAP_MODE_Free;
                EditCurve[1] = EditCurve[0];
                auto snapHandle = std::make_unique<SnapManager::SnapHandle>(nullptr, onSketchPos);
                mouseMove(*snapHandle);  // trigger an update of EditCurve
            }
        }

        updateHint();

        return true;
    }

    void quit() override
    {
        // We must see if we need to create a B-spline before cancelling everything
        // and now just like any other Handler,

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher"
        );

        bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);

        if (firstsegment) {
            // user when right-clicking with no segment in really wants to exit
            DrawSketchHandler::quit();
        }
        else {

            if (!continuousMode) {
                DrawSketchHandler::quit();
            }
            else {
                // This code disregards existing data and enables the continuous creation mode.
                Mode = STATUS_SEEK_First;
                SegmentMode = SEGMENT_MODE_Line;
                TransitionMode = TRANSITION_MODE_Free;
                SnapMode = SNAP_MODE_Free;
                suppressTransition = false;
                firstCurve = -1;
                previousCurve = -1;
                firstPosId = Sketcher::PointPos::none;
                previousPosId = Sketcher::PointPos::none;
                firstsegment = true;
                EditCurve.clear();
                drawEdit(EditCurve);
                EditCurve.resize(2);
                applyCursor();
            }
        }
    }

private:
    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Create_Lineset");
    }

    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;

        // clang-format off
        return Gui::lookupHints<SELECT_MODE>(
            Mode,
            {
                {.state = STATUS_SEEK_First,
                 .hints =
                     {
                         {tr("%1 pick first point"), {MouseLeft}},
                     }},
                {.state = STATUS_SEEK_Second,
                 .hints =
                     {
                         {tr("%1 pick next point"), {MouseLeft}},
                         {tr("%1 finish"), {MouseRight}},
                         {tr("%1 switch mode"), {KeyM}},
                     }},
            });
        // clang-format on
    }

protected:
    SELECT_MODE Mode;
    SEGMENT_MODE SegmentMode;
    TRANSITION_MODE TransitionMode;
    SNAP_MODE SnapMode;
    bool suppressTransition;

    std::vector<Base::Vector2d> EditCurve;
    int firstCurve;
    int previousCurve;
    Sketcher::PointPos firstPosId;
    Sketcher::PointPos previousPosId;
    // the latter stores those constraints that a first point would have been given in absence of
    // the transition mechanism
    std::vector<AutoConstraint> sugConstr1, sugConstr2, virtualsugConstr1;

    Base::Vector2d CenterPoint;
    Base::Vector3d dirVec;
    double startAngle, endAngle, arcRadius;

    bool firstsegment;

    void updateTransitionData(int GeoId, Sketcher::PointPos PosId)
    {

        // Use updated startPoint/endPoint as autoconstraints can modify the position
        const Part::Geometry* geom = sketchgui->getSketchObject()->getGeometry(GeoId);
        if (geom->is<Part::GeomLineSegment>()) {
            const Part::GeomLineSegment* lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
            dirVec.Set(
                lineSeg->getEndPoint().x - lineSeg->getStartPoint().x,
                lineSeg->getEndPoint().y - lineSeg->getStartPoint().y,
                0.f
            );
            if (PosId == Sketcher::PointPos::start) {
                dirVec *= -1;
                EditCurve[0] = Base::Vector2d(lineSeg->getStartPoint().x, lineSeg->getStartPoint().y);
            }
            else {
                EditCurve[0] = Base::Vector2d(lineSeg->getEndPoint().x, lineSeg->getEndPoint().y);
            }
        }
        else if (geom->is<Part::GeomArcOfCircle>()) {
            const Part::GeomArcOfCircle* arcSeg = static_cast<const Part::GeomArcOfCircle*>(geom);
            if (PosId == Sketcher::PointPos::start) {
                EditCurve[0] = Base::Vector2d(
                    arcSeg->getStartPoint(/*emulateCCW=*/true).x,
                    arcSeg->getStartPoint(/*emulateCCW=*/true).y
                );
                dirVec = Base::Vector3d(0.f, 0.f, -1.0)
                    % (arcSeg->getStartPoint(/*emulateCCW=*/true) - arcSeg->getCenter());
            }
            else {
                EditCurve[0] = Base::Vector2d(
                    arcSeg->getEndPoint(/*emulateCCW=*/true).x,
                    arcSeg->getEndPoint(/*emulateCCW=*/true).y
                );
                dirVec = Base::Vector3d(0.f, 0.f, 1.0)
                    % (arcSeg->getEndPoint(/*emulateCCW=*/true) - arcSeg->getCenter());
            }
        }
        dirVec.Normalize();
    }
};

// ==================== New version of the polyline ==============================

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
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerPolyLine)

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

    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;

        const Gui::InputHint switchModeHint {
            constructionMethod() == ConstructionMethod::Line ? tr("%1 switch to arc")
                                                             : tr("%1 switch to line"),
            {KeyM}
        };
        const Gui::InputHint filletHint {tr("%1 toggle fillet"), {KeyF}};
        const Gui::InputHint undoHint {tr("%1 undo last point"), {KeyR}};

        return Gui::lookupHints<SelectMode>(
            state(),
            {
                {.state = SelectMode::SeekFirst,
                 .hints =
                     {
                         {tr("%1 pick first point"), {MouseLeft}},
                     }},
                {.state = SelectMode::SeekSecond,
                 .hints =
                     {
                         {tr("%1 pick next point"), {MouseLeft}},
                         {tr("%1 finish"), {MouseRight}},
                         switchModeHint,
                         filletHint,
                         undoHint,
                     }},
            });
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
        return QString(QObject::tr("Polyline Parameters"));
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

                if (!obj->noRecomputes) {
                    // obj->fillet() solves at the end only when obj->noRecomputes is set, but we
                    // need the solve even when AutoRecompute is on or the fillet won't appear.
                    // See https://github.com/FreeCAD/FreeCAD/issues/30625
                    obj->solve();
                }

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
                Gui::cmdAppObjectArgs(obj, "delGeometries([%d, %d])", delGeoId, delGeoId + 1);

                if (!geoEltIds.empty()) {
                    if (!obj->noRecomputes) {
                        // delGeometries do not call solve if !obj->noRecomputes (AutoRecompute =
                        // True) But we need to call solve before trying to moveGeometry or it cause
                        // the deleted geometry to reappear. See
                        // https://github.com/FreeCAD/FreeCAD/issues/30626
                        obj->solve();
                    }

                    // Move back the previous edge point
                    int prevGeoId = geoEltIds.back().GeoId;
                    PointPos prevPos = geoEltIds.back().Pos;
                    Gui::cmdAppObjectArgs(
                        obj,
                        "moveGeometry(%d,%d,App.Vector(%f,%f,0.0),0)",
                        prevGeoId,
                        static_cast<int>(prevPos),
                        points.back().x,
                        points.back().y
                    );

                    // Then transfer back the constraints
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
                                Gui::cmdAppObjectArgs(obj, "delConstraint(%d)", i);
                                break;
                            }
                        }

                        obj->transferConstraints(pointGeoId, PointPos::start, prevGeoId, prevPos);

                        // Delete the point
                        Gui::cmdAppObjectArgs(obj, "delGeometry(%d)", delGeoId);
                    }
                }
            }
            else {
                Gui::cmdAppObjectArgs(obj, "delGeometry(%d)", delGeoId);
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

    // Since line has 4 OVP but arc has 5, and because we are not resetting the whole tool,
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

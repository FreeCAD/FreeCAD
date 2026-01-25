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

#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/InputHint.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchHandler.h"
#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "SnapManager.h"


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
                else if (TransitionMode == TRANSITION_MODE_Perpendicular_L
                         || TransitionMode == TRANSITION_MODE_Perpendicular_R) {
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
                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add line to sketch polyline")
                    );
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
                    Gui::Command::abortCommand();
                }

                firstsegment = false;
            }
            else if (SegmentMode == SEGMENT_MODE_Arc) {  // We're dealing with an Arc
                if (!boost::math::isnormal(arcRadius)) {
                    Mode = STATUS_SEEK_Second;
                    return true;
                }

                try {
                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add arc to sketch polyline")
                    );
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

                    Gui::Command::abortCommand();
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
                    else if (TransitionMode == TRANSITION_MODE_Perpendicular_L
                             || TransitionMode == TRANSITION_MODE_Perpendicular_R) {
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
                Gui::Command::commitCommand();

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
                Gui::Command::commitCommand();

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
}  // namespace SketcherGui

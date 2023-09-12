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

#ifndef SKETCHERGUI_DrawSketchHandlerArc_H
#define SKETCHERGUI_DrawSketchHandlerArc_H

#include <Gui/Notifications.h>

#include "GeometryCreationMode.h"


using namespace std;

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerArc: public DrawSketchHandler
{
public:
    DrawSketchHandlerArc()
        : Mode(STATUS_SEEK_First)
        , EditCurve(2)
        , rx(0)
        , ry(0)
        , startAngle(0)
        , endAngle(0)
        , arcAngle(0)
    {}
    ~DrawSketchHandlerArc() override
    {}
    /// mode table
    enum SelectMode
    {
        STATUS_SEEK_First,  /**< enum value ----. */
        STATUS_SEEK_Second, /**< enum value ----. */
        STATUS_SEEK_Third,  /**< enum value ----. */
        STATUS_End
    };

    void mouseMove(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {
            setPositionText(onSketchPos);
            if (seekAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }
        }
        else if (Mode == STATUS_SEEK_Second) {
            double dx_ = onSketchPos.x - EditCurve[0].x;
            double dy_ = onSketchPos.y - EditCurve[0].y;
            for (int i = 0; i < 16; i++) {
                double angle = i * M_PI / 16.0;
                double dx = dx_ * cos(angle) + dy_ * sin(angle);
                double dy = -dx_ * sin(angle) + dy_ * cos(angle);
                EditCurve[1 + i] = Base::Vector2d(EditCurve[0].x + dx, EditCurve[0].y + dy);
                EditCurve[17 + i] = Base::Vector2d(EditCurve[0].x - dx, EditCurve[0].y - dy);
            }
            EditCurve[33] = EditCurve[1];

            // Display radius and start angle
            float radius = (onSketchPos - EditCurve[0]).Length();
            float angle = atan2f(dy_, dx_);

            if (showCursorCoords()) {
                SbString text;
                std::string radiusString = lengthToDisplayFormat(radius, 1);
                std::string angleString = angleToDisplayFormat(angle * 180.0 / M_PI, 1);
                text.sprintf(" (R%s, %s)", radiusString.c_str(), angleString.c_str());
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2, onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstr2);
                return;
            }
        }
        else if (Mode == STATUS_SEEK_Third) {
            double angle1 =
                atan2(onSketchPos.y - CenterPoint.y, onSketchPos.x - CenterPoint.x) - startAngle;
            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
            arcAngle = abs(angle1 - arcAngle) < abs(angle2 - arcAngle) ? angle1 : angle2;
            for (int i = 1; i <= 29; i++) {
                double angle = i * arcAngle / 29.0;
                double dx = rx * cos(angle) - ry * sin(angle);
                double dy = rx * sin(angle) + ry * cos(angle);
                EditCurve[i] = Base::Vector2d(CenterPoint.x + dx, CenterPoint.y + dy);
            }

            // Display radius and arc angle
            float radius = (onSketchPos - EditCurve[0]).Length();

            if (showCursorCoords()) {
                SbString text;
                std::string radiusString = lengthToDisplayFormat(radius, 1);
                std::string angleString = angleToDisplayFormat(arcAngle * 180.0 / M_PI, 1);
                text.sprintf(" (R%s, %s)", radiusString.c_str(), angleString.c_str());
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr3, onSketchPos, Base::Vector2d(0.0, 0.0))) {
                renderSuggestConstraintsCursor(sugConstr3);
                return;
            }
        }
        applyCursor();
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {
            CenterPoint = onSketchPos;
            EditCurve.resize(34);
            EditCurve[0] = onSketchPos;
            setAngleSnapping(true, EditCurve[0]);
            Mode = STATUS_SEEK_Second;
        }
        else if (Mode == STATUS_SEEK_Second) {
            EditCurve.resize(31);
            EditCurve[0] = onSketchPos;
            EditCurve[30] = CenterPoint;
            rx = EditCurve[0].x - CenterPoint.x;
            ry = EditCurve[0].y - CenterPoint.y;
            startAngle = atan2(ry, rx);
            arcAngle = 0.;
            Mode = STATUS_SEEK_Third;
        }
        else {
            EditCurve.resize(30);
            double angle1 =
                atan2(onSketchPos.y - CenterPoint.y, onSketchPos.x - CenterPoint.x) - startAngle;
            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
            arcAngle = abs(angle1 - arcAngle) < abs(angle2 - arcAngle) ? angle1 : angle2;
            if (arcAngle > 0) {
                endAngle = startAngle + arcAngle;
            }
            else {
                endAngle = startAngle;
                startAngle += arcAngle;
            }

            drawEdit(EditCurve);
            applyCursor();
            setAngleSnapping(false);
            Mode = STATUS_End;
        }

        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode == STATUS_End) {
            unsetCursor();
            resetPositionText();

            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc"));
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addGeometry(Part.ArcOfCircle"
                    "(Part.Circle(App.Vector(%f,%f,0),App.Vector(0,0,1),%f),%f,%f),%s)",
                    CenterPoint.x,
                    CenterPoint.y,
                    sqrt(rx * rx + ry * ry),
                    startAngle,
                    endAngle,
                    geometryCreationMode == Construction ? "True"
                                                         : "False");  // arcAngle > 0 ? 0 : 1);

                Gui::Command::commitCommand();
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Failed to add arc"));
                Gui::Command::abortCommand();
            }

            // Auto Constraint center point
            if (!sugConstr1.empty()) {
                createAutoConstraints(sugConstr1, getHighestCurveIndex(), Sketcher::PointPos::mid);
                sugConstr1.clear();
            }

            // Auto Constraint first picked point
            if (!sugConstr2.empty()) {
                createAutoConstraints(sugConstr2,
                                      getHighestCurveIndex(),
                                      (arcAngle > 0) ? Sketcher::PointPos::start
                                                     : Sketcher::PointPos::end);
                sugConstr2.clear();
            }

            // Auto Constraint second picked point
            if (!sugConstr3.empty()) {
                createAutoConstraints(sugConstr3,
                                      getHighestCurveIndex(),
                                      (arcAngle > 0) ? Sketcher::PointPos::end
                                                     : Sketcher::PointPos::start);
                sugConstr3.clear();
            }


            tryAutoRecomputeIfNotSolve(
                static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);
            if (continuousMode) {
                // This code enables the continuous creation mode.
                Mode = STATUS_SEEK_First;
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
        return true;
    }

private:
    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Create_Arc");
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    Base::Vector2d CenterPoint;
    double rx, ry, startAngle, endAngle, arcAngle;
    std::vector<AutoConstraint> sugConstr1, sugConstr2, sugConstr3;
};

class DrawSketchHandler3PointArc: public DrawSketchHandler
{
public:
    DrawSketchHandler3PointArc()
        : Mode(STATUS_SEEK_First)
        , EditCurve(2)
        , radius(0)
        , startAngle(0)
        , endAngle(0)
        , arcAngle(0)
        , arcPos1(Sketcher::PointPos::none)
        , arcPos2(Sketcher::PointPos::none)
    {}
    ~DrawSketchHandler3PointArc() override
    {}
    /// mode table
    enum SelectMode
    {
        STATUS_SEEK_First,  /**< enum value ----. */
        STATUS_SEEK_Second, /**< enum value ----. */
        STATUS_SEEK_Third,  /**< enum value ----. */
        STATUS_End
    };

    void mouseMove(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {
            setPositionText(onSketchPos);
            if (seekAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }
        }
        else if (Mode == STATUS_SEEK_Second) {
            CenterPoint = EditCurve[0] = (onSketchPos - FirstPoint) / 2 + FirstPoint;
            EditCurve[1] = EditCurve[33] = onSketchPos;
            radius = (onSketchPos - CenterPoint).Length();
            double lineAngle = GetPointAngle(CenterPoint, onSketchPos);

            // Build a 32 point circle ignoring already constructed points
            for (int i = 1; i <= 32; i++) {
                // Start at current angle
                double angle =
                    (i - 1) * 2 * M_PI / 32.0 + lineAngle;  // N point closed circle has N segments
                if (i != 1 && i != 17) {
                    EditCurve[i] = Base::Vector2d(CenterPoint.x + radius * cos(angle),
                                                  CenterPoint.y + radius * sin(angle));
                }
            }

            // Display radius and start angle
            // This lineAngle will report counter-clockwise from +X, not relatively
            if (showCursorCoords()) {
                SbString text;
                std::string radiusString = lengthToDisplayFormat(radius, 1);
                std::string angleString = angleToDisplayFormat(lineAngle * 180.0 / M_PI, 1);
                text.sprintf(" (R%s, %s)", radiusString.c_str(), angleString.c_str());
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2, onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstr2);
                return;
            }
        }
        else if (Mode == STATUS_SEEK_Third) {
            /*
            Centerline inverts when the arc flips sides.  Easily taken care of by replacing
            centerline with a point.  It happens because the direction the curve is being drawn
            reverses.
            */
            try {
                CenterPoint = EditCurve[30] =
                    Part::Geom2dCircle::getCircleCenter(FirstPoint, SecondPoint, onSketchPos);

                radius = (SecondPoint - CenterPoint).Length();

                double angle1 = GetPointAngle(CenterPoint, FirstPoint);
                double angle2 = GetPointAngle(CenterPoint, SecondPoint);
                double angle3 = GetPointAngle(CenterPoint, onSketchPos);

                // Always build arc counter-clockwise
                // Point 3 is between Point 1 and 2
                if (angle3 > min(angle1, angle2) && angle3 < max(angle1, angle2)) {
                    if (angle2 > angle1) {
                        EditCurve[0] = FirstPoint;
                        EditCurve[29] = SecondPoint;
                        arcPos1 = Sketcher::PointPos::start;
                        arcPos2 = Sketcher::PointPos::end;
                    }
                    else {
                        EditCurve[0] = SecondPoint;
                        EditCurve[29] = FirstPoint;
                        arcPos1 = Sketcher::PointPos::end;
                        arcPos2 = Sketcher::PointPos::start;
                    }
                    startAngle = min(angle1, angle2);
                    endAngle = max(angle1, angle2);
                    arcAngle = endAngle - startAngle;
                }
                // Point 3 is not between Point 1 and 2
                else {
                    if (angle2 > angle1) {
                        EditCurve[0] = SecondPoint;
                        EditCurve[29] = FirstPoint;
                        arcPos1 = Sketcher::PointPos::end;
                        arcPos2 = Sketcher::PointPos::start;
                    }
                    else {
                        EditCurve[0] = FirstPoint;
                        EditCurve[29] = SecondPoint;
                        arcPos1 = Sketcher::PointPos::start;
                        arcPos2 = Sketcher::PointPos::end;
                    }
                    startAngle = max(angle1, angle2);
                    endAngle = min(angle1, angle2);
                    arcAngle = 2 * M_PI - (startAngle - endAngle);
                }

                // Build a 30 point circle ignoring already constructed points
                for (int i = 1; i <= 28; i++) {
                    double angle =
                        startAngle + i * arcAngle / 29.0;  // N point arc has N-1 segments
                    EditCurve[i] = Base::Vector2d(CenterPoint.x + radius * cos(angle),
                                                  CenterPoint.y + radius * sin(angle));
                }

                if (showCursorCoords()) {
                    SbString text;
                    std::string radiusString = lengthToDisplayFormat(radius, 1);
                    std::string angleString = angleToDisplayFormat(arcAngle * 180.0 / M_PI, 1);
                    text.sprintf(" (R%s, %s)", radiusString.c_str(), angleString.c_str());
                    setPositionText(onSketchPos, text);
                }

                drawEdit(EditCurve);
                if (seekAutoConstraint(sugConstr3,
                                       onSketchPos,
                                       Base::Vector2d(0.0, 0.0),
                                       AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstr3);
                    return;
                }
            }
            catch (Base::ValueError& e) {
                e.ReportException();
            }
        }
        applyCursor();
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {
            // 32 point curve + center + endpoint
            EditCurve.resize(34);
            // 17 is circle halfway point (1+32/2)
            FirstPoint = EditCurve[17] = onSketchPos;

            Mode = STATUS_SEEK_Second;
        }
        else if (Mode == STATUS_SEEK_Second) {
            // 30 point arc and center point
            EditCurve.resize(31);
            SecondPoint = onSketchPos;

            Mode = STATUS_SEEK_Third;
        }
        else {
            EditCurve.resize(30);

            drawEdit(EditCurve);
            applyCursor();
            Mode = STATUS_End;
        }

        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        // Need to look at.  rx might need fixing.
        if (Mode == STATUS_End) {
            unsetCursor();
            resetPositionText();

            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc"));
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addGeometry(Part.ArcOfCircle"
                    "(Part.Circle(App.Vector(%f,%f,0),App.Vector(0,0,1),%f),%f,%f),%s)",
                    CenterPoint.x,
                    CenterPoint.y,
                    radius,
                    startAngle,
                    endAngle,
                    geometryCreationMode == Construction ? "True" : "False");

                Gui::Command::commitCommand();
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Failed to add arc"));
                Gui::Command::abortCommand();
            }

            // Auto Constraint first picked point
            if (!sugConstr1.empty()) {
                createAutoConstraints(sugConstr1, getHighestCurveIndex(), arcPos1);
                sugConstr1.clear();
            }

            // Auto Constraint second picked point
            if (!sugConstr2.empty()) {
                createAutoConstraints(sugConstr2, getHighestCurveIndex(), arcPos2);
                sugConstr2.clear();
            }

            // Auto Constraint third picked point
            if (!sugConstr3.empty()) {
                createAutoConstraints(sugConstr3, getHighestCurveIndex(), Sketcher::PointPos::none);
                sugConstr3.clear();
            }

            tryAutoRecomputeIfNotSolve(
                static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);
            if (continuousMode) {
                // This code enables the continuous creation mode.
                Mode = STATUS_SEEK_First;
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
        return true;
    }

private:
    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Create_3PointArc");
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    Base::Vector2d CenterPoint, FirstPoint, SecondPoint;
    double radius, startAngle, endAngle, arcAngle;
    std::vector<AutoConstraint> sugConstr1, sugConstr2, sugConstr3;
    Sketcher::PointPos arcPos1, arcPos2;
};

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerArc_H

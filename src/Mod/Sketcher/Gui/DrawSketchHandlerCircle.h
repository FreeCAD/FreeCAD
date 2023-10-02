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

#ifndef SKETCHERGUI_DrawSketchHandlerCircle_H
#define SKETCHERGUI_DrawSketchHandlerCircle_H

#include "GeometryCreationMode.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerCircle: public DrawSketchHandler
{
public:
    DrawSketchHandlerCircle()
        : Mode(STATUS_SEEK_First)
        , EditCurve(34)
    {}
    ~DrawSketchHandlerCircle() override
    {}
    /// mode table
    enum SelectMode
    {
        STATUS_SEEK_First,  /**< enum value ----. */
        STATUS_SEEK_Second, /**< enum value ----. */
        STATUS_Close
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
            double rx0 = onSketchPos.x - EditCurve[0].x;
            double ry0 = onSketchPos.y - EditCurve[0].y;
            for (int i = 0; i < 16; i++) {
                double angle = i * M_PI / 16.0;
                double rx = rx0 * cos(angle) + ry0 * sin(angle);
                double ry = -rx0 * sin(angle) + ry0 * cos(angle);
                EditCurve[1 + i] = Base::Vector2d(EditCurve[0].x + rx, EditCurve[0].y + ry);
                EditCurve[17 + i] = Base::Vector2d(EditCurve[0].x - rx, EditCurve[0].y - ry);
            }
            EditCurve[33] = EditCurve[1];

            // Display radius for user
            float radius = (onSketchPos - EditCurve[0]).Length();
            if (showCursorCoords()) {
                SbString text;
                std::string radiusString = lengthToDisplayFormat(radius, 1);
                text.sprintf(" (R%s)", radiusString.c_str());
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2,
                                   onSketchPos,
                                   onSketchPos - EditCurve[0],
                                   AutoConstraint::CURVE)) {
                renderSuggestConstraintsCursor(sugConstr2);
                return;
            }
        }
        applyCursor();
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {
            EditCurve[0] = onSketchPos;
            Mode = STATUS_SEEK_Second;
        }
        else {
            EditCurve[1] = onSketchPos;
            Mode = STATUS_Close;
        }
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode == STATUS_Close) {
            double rx = EditCurve[1].x - EditCurve[0].x;
            double ry = EditCurve[1].y - EditCurve[0].y;
            unsetCursor();
            resetPositionText();

            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch circle"));
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "addGeometry(Part.Circle"
                                      "(App.Vector(%f,%f,0),App.Vector(0,0,1),%f),%s)",
                                      EditCurve[0].x,
                                      EditCurve[0].y,
                                      sqrt(rx * rx + ry * ry),
                                      geometryCreationMode == Construction ? "True" : "False");

                Gui::Command::commitCommand();
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Failed to add circle"));
                Gui::Command::abortCommand();
            }

            // add auto constraints for the center point
            if (!sugConstr1.empty()) {
                createAutoConstraints(sugConstr1, getHighestCurveIndex(), Sketcher::PointPos::mid);
                sugConstr1.clear();
            }

            // add suggested constraints for circumference
            if (!sugConstr2.empty()) {
                createAutoConstraints(sugConstr2, getHighestCurveIndex(), Sketcher::PointPos::none);
                sugConstr2.clear();
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
                EditCurve.resize(34);
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
        return QString::fromLatin1("Sketcher_Pointer_Create_Circle");
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    std::vector<AutoConstraint> sugConstr1, sugConstr2;
};

class DrawSketchHandler3PointCircle: public DrawSketchHandler
{
public:
    DrawSketchHandler3PointCircle()
        : Mode(STATUS_SEEK_First)
        , EditCurve(2)
        , radius(1)
        , N(32.0)
    {}
    ~DrawSketchHandler3PointCircle() override
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
            if (seekAutoConstraint(sugConstr1,
                                   onSketchPos,
                                   Base::Vector2d(0.f, 0.f),
                                   AutoConstraint::CURVE)) {
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }
        }
        else if (Mode == STATUS_SEEK_Second || Mode == STATUS_SEEK_Third) {
            try {
                if (Mode == STATUS_SEEK_Second) {
                    CenterPoint = EditCurve[N + 1] = (onSketchPos - FirstPoint) / 2 + FirstPoint;
                }
                else {
                    CenterPoint = EditCurve[N + 1] =
                        Part::Geom2dCircle::getCircleCenter(FirstPoint, SecondPoint, onSketchPos);
                }
                radius = (onSketchPos - CenterPoint).Length();
                double lineAngle = GetPointAngle(CenterPoint, onSketchPos);

                // Build a N point circle
                for (int i = 1; i < N; i++) {
                    // Start at current angle
                    double angle =
                        i * 2 * M_PI / N + lineAngle;  // N point closed circle has N segments
                    EditCurve[i] = Base::Vector2d(CenterPoint.x + radius * cos(angle),
                                                  CenterPoint.y + radius * sin(angle));
                }
                // Beginning and end of curve should be exact
                EditCurve[0] = EditCurve[N] = onSketchPos;

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
                if (Mode == STATUS_SEEK_Second) {
                    if (seekAutoConstraint(sugConstr2,
                                           onSketchPos,
                                           Base::Vector2d(0.f, 0.f),
                                           AutoConstraint::CURVE)) {
                        renderSuggestConstraintsCursor(sugConstr2);
                        return;
                    }
                }
                else {
                    if (seekAutoConstraint(sugConstr3,
                                           onSketchPos,
                                           Base::Vector2d(0.f, 0.f),
                                           AutoConstraint::CURVE)) {
                        renderSuggestConstraintsCursor(sugConstr3);
                        return;
                    }
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
            // N point curve + center + endpoint
            EditCurve.resize(N + 2);
            FirstPoint = onSketchPos;

            Mode = STATUS_SEEK_Second;
        }
        else if (Mode == STATUS_SEEK_Second) {
            SecondPoint = onSketchPos;

            Mode = STATUS_SEEK_Third;
        }
        else {
            EditCurve.resize(N);

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
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch circle"));
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "addGeometry(Part.Circle"
                                      "(App.Vector(%f,%f,0),App.Vector(0,0,1),%f),%s)",
                                      CenterPoint.x,
                                      CenterPoint.y,
                                      radius,
                                      geometryCreationMode == Construction ? "True" : "False");

                Gui::Command::commitCommand();
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Failed to add circle"));
                Gui::Command::abortCommand();
            }

            // Auto Constraint first picked point
            if (!sugConstr1.empty()) {
                createAutoConstraints(sugConstr1, getHighestCurveIndex(), Sketcher::PointPos::none);
                sugConstr1.clear();
            }

            // Auto Constraint second picked point
            if (!sugConstr2.empty()) {
                createAutoConstraints(sugConstr2, getHighestCurveIndex(), Sketcher::PointPos::none);
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
        return QString::fromLatin1("Sketcher_Pointer_Create_3PointCircle");
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    Base::Vector2d CenterPoint, FirstPoint, SecondPoint;
    double radius, N;  // N should be even
    std::vector<AutoConstraint> sugConstr1, sugConstr2, sugConstr3;
};


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerCircle_H

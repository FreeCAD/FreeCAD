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

class DrawSketchHandlerArcOfEllipse: public DrawSketchHandler
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerArcOfEllipse)

public:
    DrawSketchHandlerArcOfEllipse()
        : Mode(SelectMode::First)
        , EditCurve(34)
        , startAngle(0)
        , endAngle(0)
        , arcAngle(0)
    {}

    ~DrawSketchHandlerArcOfEllipse() override = default;

    /// mode table
    enum class SelectMode
    {
        First,
        Second,
        Third,
        Fourth,
        End
    };

    void mouseMove(SnapManager::SnapHandle snapHandle) override
    {
        using std::numbers::pi;
        Base::Vector2d onSketchPos = snapHandle.compute();

        if (Mode == SelectMode::First) {
            setPositionText(onSketchPos);
            seekAndRenderAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.f, 0.f));  // TODO:
                                                                                             // ellipse
                                                                                             // prio 1
        }
        else if (Mode == SelectMode::Second) {
            double rx0 = onSketchPos.x - EditCurve[0].x;
            double ry0 = onSketchPos.y - EditCurve[0].y;
            for (int i = 0; i < 16; i++) {
                double angle = i * pi / 16.0;
                double rx1 = rx0 * cos(angle) + ry0 * sin(angle);
                double ry1 = -rx0 * sin(angle) + ry0 * cos(angle);
                EditCurve[1 + i] = Base::Vector2d(EditCurve[0].x + rx1, EditCurve[0].y + ry1);
                EditCurve[17 + i] = Base::Vector2d(EditCurve[0].x - rx1, EditCurve[0].y - ry1);
            }
            EditCurve[33] = EditCurve[1];

            // Display radius for user
            float radius = (onSketchPos - EditCurve[0]).Length();

            if (showCursorCoords()) {
                SbString text;
                std::string radiusString = lengthToDisplayFormat(radius, 1);
                text.sprintf(" (R%s, R%s)", radiusString.c_str(), radiusString.c_str());
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);
            seekAndRenderAutoConstraint(
                sugConstr2,
                onSketchPos,
                onSketchPos - centerPoint,
                AutoConstraint::CURVE
            );
        }
        else if (Mode == SelectMode::Third) {
            Base::Vector2d delta12 = axisPoint - centerPoint;

            double a = delta12.Length();

            Base::Vector2d aDir = delta12.Normalize();
            Base::Vector2d bDir(-aDir.y, aDir.x);

            Base::Vector2d delta13 = onSketchPos - centerPoint;
            Base::Vector2d delta13Prime(
                delta13.x * aDir.x + delta13.y * aDir.y,
                delta13.x * bDir.x + delta13.y * bDir.y
            );

            double cosT = max(-1.0, min(1.0, delta13Prime.x / a));
            double sinT = sqrt(max(0.0, 1 - cosT * cosT));

            double b = abs(delta13Prime.y) / sinT;
            if (sinT == 0.0) {
                b = 0.0;
                a = 0.0;
            }

            for (int i = 1; i < 16; i++) {
                double angle = i * pi / 16.0;
                double rx1 = a * cos(angle) * aDir.x + b * sin(angle) * bDir.x;
                double ry1 = a * cos(angle) * aDir.y + b * sin(angle) * bDir.y;
                EditCurve[1 + i] = Base::Vector2d(EditCurve[0].x + rx1, EditCurve[0].y + ry1);
                EditCurve[17 + i] = Base::Vector2d(EditCurve[0].x - rx1, EditCurve[0].y - ry1);
            }
            EditCurve[33] = EditCurve[1];
            EditCurve[17] = EditCurve[16];

            // Display radius for user
            if (showCursorCoords()) {
                SbString text;
                std::string aString = lengthToDisplayFormat(a, 1);
                std::string bString = lengthToDisplayFormat(b, 1);
                text.sprintf(" (R%s, R%s)", aString.c_str(), bString.c_str());
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);
            seekAndRenderAutoConstraint(sugConstr3, onSketchPos, Base::Vector2d(0.f, 0.f));
        }
        else if (Mode == SelectMode::Fourth) {  // here we differ from ellipse creation
            Base::Vector2d delta12 = axisPoint - centerPoint;

            double a = delta12.Length();

            Base::Vector2d aDir = delta12.Normalize();
            Base::Vector2d bDir(-aDir.y, aDir.x);

            Base::Vector2d delta13 = startingPoint - centerPoint;
            Base::Vector2d delta13Prime(
                delta13.x * aDir.x + delta13.y * aDir.y,
                delta13.x * bDir.x + delta13.y * bDir.y
            );

            double cosT = max(-1.0, min(1.0, delta13Prime.x / a));
            double sinT = sqrt(max(0.0, 1 - cosT * cosT));

            double b = abs(delta13Prime.y) / sinT;

            startAngle = atan2(delta13Prime.y / b, delta13Prime.x / a);

            Base::Vector2d delta14 = onSketchPos - centerPoint;
            Base::Vector2d delta14Prime(
                delta14.x * aDir.x + delta14.y * aDir.y,
                delta14.x * bDir.x + delta14.y * bDir.y
            );
            double angle1 = atan2(delta14Prime.y / b, delta14Prime.x / a) - startAngle;
            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * pi;

            arcAngle = abs(angle1 - arcAngle) < abs(angle2 - arcAngle) ? angle1 : angle2;

            for (int i = 0; i < 34; i++) {
                double angle = startAngle + i * arcAngle / 33.0;
                double rx1 = a * cos(angle) * aDir.x + b * sin(angle) * bDir.x;
                double ry1 = a * cos(angle) * aDir.y + b * sin(angle) * bDir.y;
                EditCurve[i] = Base::Vector2d(centerPoint.x + rx1, centerPoint.y + ry1);
            }

            // Display radii and angle for user
            if (showCursorCoords()) {
                SbString text;
                std::string aString = lengthToDisplayFormat(a, 1);
                std::string bString = lengthToDisplayFormat(b, 1);
                std::string angleString = angleToDisplayFormat(arcAngle * 180.0 / pi, 1);
                text.sprintf(" (R%s, R%s, %s)", aString.c_str(), bString.c_str(), angleString.c_str());
                setPositionText(onSketchPos, text);
            }

            if (onSketchPos != centerPoint) {
                drawEdit(EditCurve);
            }
            else {
                drawEdit(std::vector<Base::Vector2d>());
            }
            seekAndRenderAutoConstraint(sugConstr4, onSketchPos, Base::Vector2d(0.f, 0.f));
        }
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        using std::numbers::pi;

        if (Mode == SelectMode::First) {
            EditCurve[0] = onSketchPos;
            centerPoint = onSketchPos;
            setAngleSnapping(true, centerPoint);
            Mode = SelectMode::Second;
        }
        else if (Mode == SelectMode::Second
                 && (centerPoint - onSketchPos).Length() >= Precision::Confusion()) {
            EditCurve[1] = onSketchPos;
            axisPoint = onSketchPos;
            Mode = SelectMode::Third;
        }
        else if (Mode == SelectMode::Third && validThirdPoint(onSketchPos)) {
            startingPoint = onSketchPos;
            arcAngle = 0.;
            Mode = SelectMode::Fourth;
        }
        else if (Mode == SelectMode::Fourth && centerPoint != onSketchPos && arcAngle != 0
                 && abs(arcAngle) != 2 * pi) {
            endPoint = onSketchPos;

            setAngleSnapping(false);
            Mode = SelectMode::End;
        }

        updateHint();
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);

        using std::numbers::pi;

        if (Mode == SelectMode::End) {
            unsetCursor();
            resetPositionText();

            Base::Vector2d delta12 = axisPoint - centerPoint;

            double a = delta12.Length();

            Base::Vector2d aDir = delta12.Normalize();
            Base::Vector2d bDir(-aDir.y, aDir.x);

            Base::Vector2d delta13 = startingPoint - centerPoint;
            Base::Vector2d delta13Prime(
                delta13.x * aDir.x + delta13.y * aDir.y,
                delta13.x * bDir.x + delta13.y * bDir.y
            );

            double cosT = max(-1.0, min(1.0, delta13Prime.x / a));
            double sinT = sqrt(max(0.0, 1 - cosT * cosT));

            double b = abs(delta13Prime.y) / sinT;

            Base::Vector2d delta14 = endPoint - centerPoint;
            Base::Vector2d delta14Prime(
                delta14.x * aDir.x + delta14.y * aDir.y,
                delta14.x * bDir.x + delta14.y * bDir.y
            );
            double angle1 = atan2(delta14Prime.y / b, delta14Prime.x / a) - startAngle;
            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * pi;

            arcAngle = abs(angle1 - arcAngle) < abs(angle2 - arcAngle) ? angle1 : angle2;

            bool isOriginalArcCCW = true;

            if (arcAngle > 0) {
                endAngle = startAngle + arcAngle;
            }
            else {
                endAngle = startAngle;
                startAngle += arcAngle;
                isOriginalArcCCW = false;
            }

            Base::Vector2d majAxisDir, minAxisDir, minAxisPoint, majAxisPoint;
            // We always create a CCW ellipse, because we want our XY reference system to be in the
            // +X +Y direction Our normal will then always be in the +Z axis (local +Z axis of the
            // sketcher)

            if (a > b) {
                // force second semidiameter to be perpendicular to first semidiamater
                majAxisDir = axisPoint - centerPoint;
                Base::Vector2d perp(-majAxisDir.y, majAxisDir.x);
                perp.Normalize();
                perp.Scale(abs(b));
                minAxisPoint = centerPoint + perp;
                majAxisPoint = centerPoint + majAxisDir;
            }
            else {
                // force second semidiameter to be perpendicular to first semidiamater
                minAxisDir = axisPoint - centerPoint;
                Base::Vector2d perp(minAxisDir.y, -minAxisDir.x);
                perp.Normalize();
                perp.Scale(abs(b));
                majAxisPoint = centerPoint + perp;
                minAxisPoint = centerPoint + minAxisDir;
                endAngle += pi / 2;
                startAngle += pi / 2;
            }

            int currentgeoid = getHighestCurveIndex();

            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc of ellipse"));

                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addGeometry(Part.ArcOfEllipse"
                    "(Part.Ellipse(App.Vector(%f,%f,0),App.Vector(%f,%f,0),App."
                    "Vector(%f,%f,0)),%f,%f),%s)",
                    majAxisPoint.x,
                    majAxisPoint.y,
                    minAxisPoint.x,
                    minAxisPoint.y,
                    centerPoint.x,
                    centerPoint.y,
                    startAngle,
                    endAngle,
                    constructionModeAsBooleanText()
                );

                currentgeoid++;

                Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", currentgeoid);
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(
                    sketchgui,
                    QT_TRANSLATE_NOOP("Notifications", "Error"),
                    QT_TRANSLATE_NOOP("Notifications", "Failed to add arc of ellipse")
                );
                Gui::Command::abortCommand();

                tryAutoRecomputeIfNotSolve(sketchgui->getObject<Sketcher::SketchObject>());

                return false;
            }

            Gui::Command::commitCommand();

            // add auto constraints for the center point
            if (!sugConstr1.empty()) {
                createAutoConstraints(sugConstr1, currentgeoid, Sketcher::PointPos::mid);
                sugConstr1.clear();
            }

            // add suggested constraints for arc
            if (!sugConstr2.empty()) {
                createAutoConstraints(sugConstr2, currentgeoid, Sketcher::PointPos::none);
                sugConstr2.clear();
            }

            // add suggested constraints for start of arc
            if (!sugConstr3.empty()) {
                createAutoConstraints(
                    sugConstr3,
                    currentgeoid,
                    isOriginalArcCCW ? Sketcher::PointPos::start : Sketcher::PointPos::end
                );
                sugConstr3.clear();
            }

            // add suggested constraints for start of arc
            if (!sugConstr4.empty()) {
                createAutoConstraints(
                    sugConstr4,
                    currentgeoid,
                    isOriginalArcCCW ? Sketcher::PointPos::end : Sketcher::PointPos::start
                );
                sugConstr4.clear();
            }

            tryAutoRecomputeIfNotSolve(sketchgui->getObject<Sketcher::SketchObject>());

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher"
            );
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);
            if (continuousMode) {
                // This code enables the continuous creation mode.
                Mode = SelectMode::First;
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

        updateHint();

        return true;
    }

private:
    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Create_ArcOfEllipse");
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    Base::Vector2d centerPoint, axisPoint, startingPoint, endPoint;
    double startAngle, endAngle, arcAngle;
    std::vector<AutoConstraint> sugConstr1, sugConstr2, sugConstr3, sugConstr4;

private:
    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;

        return Gui::lookupHints<SelectMode>(
            Mode,
            {
                {.state = SelectMode::First,
                 .hints =
                     {
                         {tr("%1 pick ellipse center"), {MouseLeft}},
                     }},
                {.state = SelectMode::Second,
                 .hints =
                     {
                         {tr("%1 pick axis point"), {MouseLeft}},
                     }},
                {.state = SelectMode::Third,
                 .hints =
                     {
                         {tr("%1 pick arc start point"), {MouseLeft}},
                     }},
                {.state = SelectMode::Fourth,
                 .hints =
                     {
                         {tr("%1 pick arc end point"), {MouseLeft}},
                     }},
            });
    }

    bool validThirdPoint(Base::Vector2d onSketchPos)
    {
        Base::Vector2d delta12 = axisPoint - centerPoint;

        double a = delta12.Length();

        Base::Vector2d aDir = delta12.Normalize();
        Base::Vector2d bDir(-aDir.y, aDir.x);

        Base::Vector2d delta13 = onSketchPos - centerPoint;
        Base::Vector2d delta13Prime(
            delta13.x * aDir.x + delta13.y * aDir.y,
            delta13.x * bDir.x + delta13.y * bDir.y
        );

        double cosT = max(-1.0, min(1.0, delta13Prime.x / a));
        return cosT != -1.0 && cosT != 1.0 && delta13Prime.y != 0;
    }
};

}  // namespace SketcherGui

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

#ifndef SKETCHERGUI_DrawSketchHandlerArcOfHyperbola_H
#define SKETCHERGUI_DrawSketchHandlerArcOfHyperbola_H

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

class DrawSketchHandlerArcOfHyperbola: public DrawSketchHandler
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerArcOfHyperbola)

public:
    DrawSketchHandlerArcOfHyperbola()
        : Mode(SelectMode::First)
        , EditCurve(34)
        , arcAngle(0)
    {}

    ~DrawSketchHandlerArcOfHyperbola() override = default;
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
        Base::Vector2d onSketchPos = snapHandle.compute();
        if (Mode == SelectMode::First) {
            setPositionText(onSketchPos);
            seekAndRenderAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.f, 0.f));
        }
        else if (Mode == SelectMode::Second) {
            EditCurve[1] = onSketchPos;

            // Display radius for user
            float radius = (onSketchPos - centerPoint).Length();
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
                Base::Vector2d(0.f, 0.f),
                AutoConstraint::CURVE
            );
        }
        else if (Mode == SelectMode::Third) {
            // angle between the major axis of the hyperbola and the X axis
            Base::Vector2d delta12 = axisPoint - centerPoint;

            double a = delta12.Length();
            assert(
                a > Precision::Confusion()
                && "DrawSketchHandlerArcOfHyperbola: First and second point are at the same place"
            );

            Base::Vector2d aDir = delta12.Normalize();
            Base::Vector2d bDir(-aDir.y, aDir.x);

            Base::Vector2d delta13 = onSketchPos - centerPoint;
            Base::Vector2d delta13Prime(
                delta13.x * aDir.x + delta13.y * aDir.y,
                delta13.x * bDir.x + delta13.y * bDir.y
            );

            double denom = (delta13Prime.x * delta13Prime.x) / (a * a) - 1.0;
            double b = std::sqrt((delta13Prime.y * delta13Prime.y) / denom);

            if (denom <= Precision::Confusion() || b < Precision::Confusion()) {
                a = 0;
                b = 0;
            }

            double angleatpoint = atanh((delta13Prime.y * a) / (delta13Prime.x * b));

            for (int i = 16; i >= -16; i--) {
                // P(U) = O + MajRad*Cosh(U)*XDir + MinRad*Sinh(U)*YDir
                double angle = i * angleatpoint / 16.0;
                double rx = a * cosh(angle) * aDir.x + b * sinh(angle) * bDir.x;
                double ry = a * cosh(angle) * aDir.y + b * sinh(angle) * bDir.y;
                EditCurve[16 + i] = Base::Vector2d(centerPoint.x + rx, centerPoint.y + ry);
            }

            // Display radius for user
            if (showCursorCoords()) {
                SbString text;
                std::string aString = lengthToDisplayFormat(a, 1);
                std::string bString = lengthToDisplayFormat(b, 1);
                text.sprintf(" (R%s, R%s)", aString.c_str(), bString.c_str());
                setPositionText(onSketchPos, text);
            }

            if (denom > Precision::Confusion() && b > Precision::Confusion())
                drawEdit(EditCurve);
            else
                drawEdit(std::vector<Base::Vector2d>());

            seekAndRenderAutoConstraint(sugConstr3, onSketchPos, Base::Vector2d(0.f, 0.f));
        }
        else if (Mode == SelectMode::Fourth) {
            // angle between the major axis of the hyperbola and the X axis
            Base::Vector2d delta12 = axisPoint - centerPoint;

            double a = delta12.Length();
            assert(
                a > Precision::Confusion()
                && "DrawSketchHandlerArcOfHyperbola: First and second point are at the same place"
            );

            Base::Vector2d aDir = delta12.Normalize();
            Base::Vector2d bDir(-aDir.y, aDir.x);

            Base::Vector2d delta13 = startingPoint - centerPoint;
            Base::Vector2d delta13Prime(
                delta13.x * aDir.x + delta13.y * aDir.y,
                delta13.x * bDir.x + delta13.y * bDir.y
            );

            double denom = (delta13Prime.x * delta13Prime.x) / (a * a) - 1.0;
            double b = std::sqrt((delta13Prime.y * delta13Prime.y) / denom);

            if (denom <= Precision::Confusion()) {
                a = 0;
                b = 0;
            }

            double startAngle = atanh((delta13Prime.y * a) / (delta13Prime.x * b));

            Base::Vector2d delta14 = onSketchPos - centerPoint;
            Base::Vector2d delta14Prime(
                delta14.x * aDir.x + delta14.y * aDir.y,
                delta14.x * bDir.x + delta14.y * bDir.y
            );

            double angleatpoint = atanh((delta14Prime.y * a) / (delta14Prime.x * b));

            arcAngle = angleatpoint - startAngle;

            if (abs((delta14Prime.y * a) / (delta14Prime.x * b)) > 1) {
                arcAngle = 0;
            }

            for (int i = 0; i < 33; i++) {
                // P(U) = O + MajRad*Cosh(U)*XDir + MinRad*Sinh(U)*YDir
                double angle = startAngle + i * arcAngle / 32.0;
                double rx = a * cosh(angle) * aDir.x + b * sinh(angle) * bDir.x;
                double ry = a * cosh(angle) * aDir.y + b * sinh(angle) * bDir.y;
                EditCurve[i] = Base::Vector2d(centerPoint.x + rx, centerPoint.y + ry);
            }

            // Display radius for user
            if (showCursorCoords()) {
                SbString text;
                std::string aString = lengthToDisplayFormat(a, 1);
                std::string bString = lengthToDisplayFormat(b, 1);
                std::string arcAngleString = angleToDisplayFormat(arcAngle / 2 * 180, 1);
                text.sprintf(" (R%s, R%s, %s)", aString.c_str(), bString.c_str(), arcAngleString.c_str());
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);
            seekAndRenderAutoConstraint(sugConstr4, onSketchPos, Base::Vector2d(0.f, 0.f));
        }
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode == SelectMode::First) {
            EditCurve[0] = onSketchPos;
            centerPoint = onSketchPos;
            EditCurve.resize(2);
            Mode = SelectMode::Second;
        }
        else if (Mode == SelectMode::Second && 
            (centerPoint - onSketchPos).Length() > Precision::Confusion()) {
            EditCurve[1] = onSketchPos;
            axisPoint = onSketchPos;
            EditCurve.resize(33);
            Mode = SelectMode::Third;
        }
        else if (Mode == SelectMode::Third && validThirdPoint(onSketchPos)) {
            startingPoint = onSketchPos;
            arcAngle = 0.;
            Mode = SelectMode::Fourth;
        }
        else if (Mode == SelectMode::Fourth && arcAngle != 0) {
            endPoint = onSketchPos;

            Mode = SelectMode::End;
        }

        updateHint();
        return true;
    }

    bool releaseButton(Base::Vector2d /*onSketchPos*/) override
    {
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

            double denom = (delta13Prime.x * delta13Prime.x) / (a * a) - 1.0;
            double b = std::sqrt((delta13Prime.y * delta13Prime.y) / denom);

            double startAngle = atanh((delta13Prime.y * a) / (delta13Prime.x * b));

            Base::Vector2d delta14 = endPoint - centerPoint;
            Base::Vector2d delta14Prime(
                delta14.x * aDir.x + delta14.y * aDir.y,
                delta14.x * bDir.x + delta14.y * bDir.y
            );
            double endAngle = atanh((delta14Prime.y * a) / (delta14Prime.x * b));

            bool isOriginalArcCCW = true;

            if (endAngle < startAngle) {
                std::swap(startAngle, endAngle);
                isOriginalArcCCW = false;
            }

            Base::Vector2d minAxisPoint, majAxisPoint;
            // We always create a CCW hyperbola, because we want our XY reference system to be in
            // the +X +Y direction Our normal will then always be in the +Z axis (local +Z axis of
            // the sketcher)
            majAxisPoint = centerPoint + (aDir * a);
            minAxisPoint = centerPoint + (bDir * b);

            int currentgeoid = getHighestCurveIndex();

            try {

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc of hyperbola"));

                // Add arc of hyperbola, point and constrain point as focus2. We add focus2 for it
                // to balance the intrinsic focus1, in order to balance out the intrinsic invisible
                // focus1 when AOE is dragged by its center
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addGeometry(Part.ArcOfHyperbola"
                    "(Part.Hyperbola(App.Vector(%f,%f,0),App.Vector(%f,%f,0),App."
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
                    QT_TRANSLATE_NOOP("Notifications", "Cannot create arc of hyperbola")
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
                /* It is ok not to call to purgeHandler
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
        return QStringLiteral("Sketcher_Pointer_Create_ArcOfHyperbola");
    }

    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;

        return Gui::lookupHints<SelectMode>(
            Mode,
            {
                {.state = SelectMode::First,
                 .hints =
                     {
                         {tr("%1 pick center point"), {MouseLeft}},
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

        double denom = (delta13Prime.x * delta13Prime.x) / (a * a) - 1.0;
        double b = std::sqrt((delta13Prime.y * delta13Prime.y) / denom);

        return denom > Precision::Confusion() && b > Precision::Confusion();
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    Base::Vector2d centerPoint, axisPoint, startingPoint, endPoint;
    double arcAngle;
    std::vector<AutoConstraint> sugConstr1, sugConstr2, sugConstr3, sugConstr4;
};

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerArcOfHyperbola_H

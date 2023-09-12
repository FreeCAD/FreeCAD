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

#ifndef SKETCHERGUI_DrawSketchHandlerArcOfParabola_H
#define SKETCHERGUI_DrawSketchHandlerArcOfParabola_H

#include "GeometryCreationMode.h"

#include <Gui/Notifications.h>

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerArcOfParabola: public DrawSketchHandler
{
public:
    DrawSketchHandlerArcOfParabola()
        : Mode(STATUS_SEEK_First)
        , EditCurve(34)
        , startAngle(0)
        , endAngle(0)
        , arcAngle(0)
        , arcAngle_t(0)
    {}

    ~DrawSketchHandlerArcOfParabola() override = default;

    /// mode table
    enum SelectMode
    {
        STATUS_SEEK_First,  /**< enum value ----. */
        STATUS_SEEK_Second, /**< enum value ----. */
        STATUS_SEEK_Third,  /**< enum value ----. */
        STATUS_SEEK_Fourth, /**< enum value ----. */
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
            EditCurve[1] = onSketchPos;

            // Display radius for user
            float radius = (onSketchPos - focusPoint).Length();
            if (showCursorCoords()) {
                SbString text;
                std::string radiusString = lengthToDisplayFormat(radius, 1);
                text.sprintf(" (F%s)", radiusString.c_str());
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2, onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstr2);
                return;
            }
        }
        else if (Mode == STATUS_SEEK_Third) {
            double focal = (axisPoint - focusPoint).Length();
            double phi = atan2(focusPoint.y - axisPoint.y, focusPoint.x - axisPoint.x);

            // P(U) = O + U*U/(4.*F)*XDir + U*YDir
            //
            // pnt = Base::Vector3d(pnt0.x + angle * angle / 4 / focal * cos(phi) - angle *
            // sin(phi),
            //                      pnt0.y + angle * angle / 4 / focal * sin(phi) + angle *
            //                      cos(phi), 0.f);

            // This is the angle at cursor point
            double u = (cos(phi) * (onSketchPos.y - axisPoint.y)
                        - (onSketchPos.x - axisPoint.x) * sin(phi));

            for (int i = 15; i >= -15; i--) {
                double angle = i * u / 15;
                double rx = angle * angle / 4 / focal * cos(phi) - angle * sin(phi);
                double ry = angle * angle / 4 / focal * sin(phi) + angle * cos(phi);
                EditCurve[15 + i] = Base::Vector2d(axisPoint.x + rx, axisPoint.y + ry);
            }

            // Display radius for user
            if (showCursorCoords()) {
                SbString text;
                std::string focalString = lengthToDisplayFormat(focal, 1);
                text.sprintf(" (F%s)", focalString.c_str());
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);

            if (seekAutoConstraint(sugConstr3, onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstr3);
                return;
            }
        }
        else if (Mode == STATUS_SEEK_Fourth) {
            double focal = (axisPoint - focusPoint).Length();
            double phi = atan2(focusPoint.y - axisPoint.y, focusPoint.x - axisPoint.x);

            // P(U) = O + U*U/(4.*F)*XDir + U*YDir
            //
            // pnt = Base::Vector3d(pnt0.x + angle * angle / 4 / focal * cos(phi) - angle *
            // sin(phi),
            //                      pnt0.y + angle * angle / 4 / focal * sin(phi) + angle *
            //                      cos(phi), 0.f);

            // This is the angle at starting point
            double ustartpoint = (cos(phi) * (startingPoint.y - axisPoint.y)
                                  - (startingPoint.x - axisPoint.x) * sin(phi));

            double startValue = ustartpoint;

            double u = (cos(phi) * (onSketchPos.y - axisPoint.y)
                        - (onSketchPos.x - axisPoint.x) * sin(phi));


            arcAngle = u - startValue;

            if (!boost::math::isnan(arcAngle)) {
                EditCurve.resize(33);
                for (std::size_t i = 0; i < 33; i++) {
                    double angle = startValue + i * arcAngle / 32.0;
                    double rx = angle * angle / 4 / focal * cos(phi) - angle * sin(phi);
                    double ry = angle * angle / 4 / focal * sin(phi) + angle * cos(phi);
                    EditCurve[i] = Base::Vector2d(axisPoint.x + rx, axisPoint.y + ry);
                }

                if (showCursorCoords()) {
                    SbString text;
                    std::string focalString = lengthToDisplayFormat(focal, 1);
                    text.sprintf(" (F%s)", focalString.c_str());
                    setPositionText(onSketchPos, text);
                }
            }
            else {
                arcAngle = 0.;
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr4, onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstr4);
                return;
            }
        }

        applyCursor();
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {
            EditCurve[0] = onSketchPos;
            focusPoint = onSketchPos;
            EditCurve.resize(2);
            Mode = STATUS_SEEK_Second;
        }
        else if (Mode == STATUS_SEEK_Second) {
            EditCurve[1] = onSketchPos;
            axisPoint = onSketchPos;
            EditCurve.resize(31);
            Mode = STATUS_SEEK_Third;
        }
        else if (Mode == STATUS_SEEK_Third) {
            startingPoint = onSketchPos;
            arcAngle = 0.;
            arcAngle_t = 0.;
            Mode = STATUS_SEEK_Fourth;
        }
        else {  // Fourth
            endPoint = onSketchPos;
            Mode = STATUS_Close;
        }
        return true;
    }

    bool releaseButton(Base::Vector2d /*onSketchPos*/) override
    {
        if (Mode == STATUS_Close) {
            unsetCursor();
            resetPositionText();

            double phi = atan2(focusPoint.y - axisPoint.y, focusPoint.x - axisPoint.x);

            double ustartpoint = (cos(phi) * (startingPoint.y - axisPoint.y)
                                  - (startingPoint.x - axisPoint.x) * sin(phi));

            double uendpoint =
                (cos(phi) * (endPoint.y - axisPoint.y) - (endPoint.x - axisPoint.x) * sin(phi));

            double startAngle = ustartpoint;

            double endAngle = uendpoint;

            bool isOriginalArcCCW = true;

            if (arcAngle > 0) {
                endAngle = startAngle + arcAngle;
            }
            else {
                endAngle = startAngle;
                startAngle += arcAngle;
                isOriginalArcCCW = false;
            }

            int currentgeoid = getHighestCurveIndex();

            try {
                Gui::Command::openCommand(
                    QT_TRANSLATE_NOOP("Command", "Add sketch arc of Parabola"));

                // Add arc of parabola
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "addGeometry(Part.ArcOfParabola"
                                      "(Part.Parabola(App.Vector(%f,%f,0),App.Vector(%f,%f,0),App."
                                      "Vector(0,0,1)),%f,%f),%s)",
                                      focusPoint.x,
                                      focusPoint.y,
                                      axisPoint.x,
                                      axisPoint.y,
                                      startAngle,
                                      endAngle,
                                      geometryCreationMode == Construction ? "True" : "False");

                currentgeoid++;

                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "exposeInternalGeometry(%d)",
                                      currentgeoid);
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(
                    sketchgui,
                    QT_TRANSLATE_NOOP("Notifications", "Error"),
                    QT_TRANSLATE_NOOP("Notifications", "Cannot create arc of parabola"));
                Gui::Command::abortCommand();

                tryAutoRecomputeIfNotSolve(
                    static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

                return false;
            }

            Gui::Command::commitCommand();

            // add auto constraints for the focus point
            if (!sugConstr1.empty()) {
                createAutoConstraints(sugConstr1, currentgeoid + 1, Sketcher::PointPos::start);
                sugConstr1.clear();
            }

            // add suggested constraints for vertex point
            if (!sugConstr2.empty()) {
                createAutoConstraints(sugConstr2, currentgeoid, Sketcher::PointPos::mid);
                sugConstr2.clear();
            }

            // add suggested constraints for start of arc
            if (!sugConstr3.empty()) {
                createAutoConstraints(sugConstr3,
                                      currentgeoid,
                                      isOriginalArcCCW ? Sketcher::PointPos::start
                                                       : Sketcher::PointPos::end);
                sugConstr3.clear();
            }

            // add suggested constraints for start of arc
            if (!sugConstr4.empty()) {
                createAutoConstraints(sugConstr4,
                                      currentgeoid,
                                      isOriginalArcCCW ? Sketcher::PointPos::end
                                                       : Sketcher::PointPos::start);
                sugConstr4.clear();
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
        return true;
    }

private:
    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Create_ArcOfParabola");
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    Base::Vector2d focusPoint, axisPoint, startingPoint, endPoint;
    double startAngle, endAngle, arcAngle, arcAngle_t;
    std::vector<AutoConstraint> sugConstr1, sugConstr2, sugConstr3, sugConstr4;
};

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerArcOfParabola_H

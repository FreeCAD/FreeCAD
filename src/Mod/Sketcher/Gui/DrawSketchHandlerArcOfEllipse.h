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

#ifndef SKETCHERGUI_DrawSketchHandlerArcOfEllipse_H
#define SKETCHERGUI_DrawSketchHandlerArcOfEllipse_H

#include <Gui/Notifications.h>

#include "GeometryCreationMode.h"

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerArcOfEllipse: public DrawSketchHandler
{
public:
    DrawSketchHandlerArcOfEllipse()
        : Mode(STATUS_SEEK_First)
        , EditCurve(34)
        , rx(0)
        , ry(0)
        , startAngle(0)
        , endAngle(0)
        , arcAngle(0)
        , arcAngle_t(0)
    {}

    ~DrawSketchHandlerArcOfEllipse() override = default;

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
            if (seekAutoConstraint(sugConstr1,
                                   onSketchPos,
                                   Base::Vector2d(0.f, 0.f))) {  // TODO: ellipse prio 1
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }
        }
        else if (Mode == STATUS_SEEK_Second) {
            double rx0 = onSketchPos.x - EditCurve[0].x;
            double ry0 = onSketchPos.y - EditCurve[0].y;
            for (int i = 0; i < 16; i++) {
                double angle = i * M_PI / 16.0;
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
            if (seekAutoConstraint(sugConstr2,
                                   onSketchPos,
                                   onSketchPos - centerPoint,
                                   AutoConstraint::CURVE)) {
                renderSuggestConstraintsCursor(sugConstr2);
                return;
            }
        }
        else if (Mode == STATUS_SEEK_Third) {
            // angle between the major axis of the ellipse and the X axis
            double a = (EditCurve[1] - EditCurve[0]).Length();
            double phi = atan2(EditCurve[1].y - EditCurve[0].y, EditCurve[1].x - EditCurve[0].x);

            // This is the angle at cursor point
            double angleatpoint =
                acos((onSketchPos.x - EditCurve[0].x + (onSketchPos.y - EditCurve[0].y) * tan(phi))
                     / (a * (cos(phi) + tan(phi) * sin(phi))));
            double b = (onSketchPos.y - EditCurve[0].y - a * cos(angleatpoint) * sin(phi))
                / (sin(angleatpoint) * cos(phi));

            for (int i = 1; i < 16; i++) {
                double angle = i * M_PI / 16.0;
                double rx1 = a * cos(angle) * cos(phi) - b * sin(angle) * sin(phi);
                double ry1 = a * cos(angle) * sin(phi) + b * sin(angle) * cos(phi);
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
            if (seekAutoConstraint(sugConstr3, onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstr3);
                return;
            }
        }
        else if (Mode == STATUS_SEEK_Fourth) {  // here we differ from ellipse creation
            // angle between the major axis of the ellipse and the X axis
            double a = (axisPoint - centerPoint).Length();
            double phi = atan2(axisPoint.y - centerPoint.y, axisPoint.x - centerPoint.x);

            // This is the angle at cursor point
            double angleatpoint = acos(
                (startingPoint.x - centerPoint.x + (startingPoint.y - centerPoint.y) * tan(phi))
                / (a * (cos(phi) + tan(phi) * sin(phi))));
            double b = abs((startingPoint.y - centerPoint.y - a * cos(angleatpoint) * sin(phi))
                           / (sin(angleatpoint) * cos(phi)));

            double rxs = startingPoint.x - centerPoint.x;
            double rys = startingPoint.y - centerPoint.y;
            startAngle = atan2(a * (rys * cos(phi) - rxs * sin(phi)),
                               b * (rxs * cos(phi) + rys * sin(phi)));  // eccentric anomaly angle

            double angle1 = atan2(a
                                      * ((onSketchPos.y - centerPoint.y) * cos(phi)
                                         - (onSketchPos.x - centerPoint.x) * sin(phi)),
                                  b
                                      * ((onSketchPos.x - centerPoint.x) * cos(phi)
                                         + (onSketchPos.y - centerPoint.y) * sin(phi)))
                - startAngle;

            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
            arcAngle = abs(angle1 - arcAngle) < abs(angle2 - arcAngle) ? angle1 : angle2;

            for (int i = 0; i < 34; i++) {
                double angle = startAngle + i * arcAngle / 34.0;
                double rx1 = a * cos(angle) * cos(phi) - b * sin(angle) * sin(phi);
                double ry1 = a * cos(angle) * sin(phi) + b * sin(angle) * cos(phi);
                EditCurve[i] = Base::Vector2d(centerPoint.x + rx1, centerPoint.y + ry1);
            }
            //             EditCurve[33] = EditCurve[1];
            //             EditCurve[17] = EditCurve[16];

            // Display radii and angle for user
            if (showCursorCoords()) {
                SbString text;
                std::string aString = lengthToDisplayFormat(a, 1);
                std::string bString = lengthToDisplayFormat(b, 1);
                std::string angleString = angleToDisplayFormat(arcAngle * 180.0 / M_PI, 1);
                text.sprintf(" (R%s, R%s, %s)",
                             aString.c_str(),
                             bString.c_str(),
                             angleString.c_str());
                setPositionText(onSketchPos, text);
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
            centerPoint = onSketchPos;
            setAngleSnapping(true, centerPoint);
            Mode = STATUS_SEEK_Second;
        }
        else if (Mode == STATUS_SEEK_Second) {
            EditCurve[1] = onSketchPos;
            axisPoint = onSketchPos;
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

            setAngleSnapping(false);
            Mode = STATUS_Close;
        }
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode == STATUS_Close) {
            unsetCursor();
            resetPositionText();

            // angle between the major axis of the ellipse and the X axisEllipse
            double a = (axisPoint - centerPoint).Length();
            double phi = atan2(axisPoint.y - centerPoint.y, axisPoint.x - centerPoint.x);

            // This is the angle at cursor point
            double angleatpoint = acos(
                (startingPoint.x - centerPoint.x + (startingPoint.y - centerPoint.y) * tan(phi))
                / (a * (cos(phi) + tan(phi) * sin(phi))));
            double b = abs((startingPoint.y - centerPoint.y - a * cos(angleatpoint) * sin(phi))
                           / (sin(angleatpoint) * cos(phi)));

            double angle1 = atan2(a
                                      * ((endPoint.y - centerPoint.y) * cos(phi)
                                         - (endPoint.x - centerPoint.x) * sin(phi)),
                                  b
                                      * ((endPoint.x - centerPoint.x) * cos(phi)
                                         + (endPoint.y - centerPoint.y) * sin(phi)))
                - startAngle;

            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
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
                endAngle += M_PI / 2;
                startAngle += M_PI / 2;
                phi -= M_PI / 2;
                double t = a;
                a = b;
                b = t;  // swap a,b
            }

            int currentgeoid = getHighestCurveIndex();

            try {
                Gui::Command::openCommand(
                    QT_TRANSLATE_NOOP("Command", "Add sketch arc of ellipse"));

                Gui::cmdAppObjectArgs(sketchgui->getObject(),
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
                    QT_TRANSLATE_NOOP("Notifications", "Failed to add arc of ellipse"));
                Gui::Command::abortCommand();

                tryAutoRecomputeIfNotSolve(
                    static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

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
        return QString::fromLatin1("Sketcher_Pointer_Create_ArcOfEllipse");
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    Base::Vector2d centerPoint, axisPoint, startingPoint, endPoint;
    double rx, ry, startAngle, endAngle, arcAngle, arcAngle_t;
    std::vector<AutoConstraint> sugConstr1, sugConstr2, sugConstr3, sugConstr4;
};

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerArcOfEllipse_H

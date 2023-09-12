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
#ifndef SKETCHERGUI_DrawSketchHandlerLine_H
#define SKETCHERGUI_DrawSketchHandlerLine_H

#include <Gui/Notifications.h>

#include "GeometryCreationMode.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerLine: public DrawSketchHandler
{
public:
    DrawSketchHandlerLine()
        : Mode(STATUS_SEEK_First)
        , EditCurve(2)
    {}
    ~DrawSketchHandlerLine() override
    {}
    /// mode table
    enum SelectMode
    {
        STATUS_SEEK_First,  /**< enum value ----. */
        STATUS_SEEK_Second, /**< enum value ----. */
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
            float length = (onSketchPos - EditCurve[0]).Length();
            float angle = (onSketchPos - EditCurve[0]).GetAngle(Base::Vector2d(1.f, 0.f));
            if (showCursorCoords()) {
                SbString text;
                std::string lengthString = lengthToDisplayFormat(length, 1);
                std::string angleString = angleToDisplayFormat(angle * 180.0 / M_PI, 1);
                text.sprintf(" (%s, %s)", lengthString.c_str(), angleString.c_str());
                setPositionText(onSketchPos, text);
            }

            EditCurve[1] = onSketchPos;
            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2, onSketchPos, onSketchPos - EditCurve[0])) {
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

            setAngleSnapping(true, EditCurve[0]);
            Mode = STATUS_SEEK_Second;
        }
        else {
            EditCurve[1] = onSketchPos;
            drawEdit(EditCurve);
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
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch line"));
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addGeometry(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)),%s)",
                    EditCurve[0].x,
                    EditCurve[0].y,
                    EditCurve[1].x,
                    EditCurve[1].y,
                    geometryCreationMode == Construction ? "True" : "False");

                Gui::Command::commitCommand();
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Failed to add line"));

                Gui::Command::abortCommand();
            }

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool avoidredundant =
                sketchgui->AvoidRedundant.getValue() && sketchgui->Autoconstraints.getValue();

            if (avoidredundant) {
                removeRedundantHorizontalVertical(
                    static_cast<Sketcher::SketchObject*>(sketchgui->getObject()),
                    sugConstr1,
                    sugConstr2);
            }

            // add auto constraints for the line segment start
            if (!sugConstr1.empty()) {
                createAutoConstraints(sugConstr1,
                                      getHighestCurveIndex(),
                                      Sketcher::PointPos::start);
                sugConstr1.clear();
            }

            // add auto constraints for the line segment end
            if (!sugConstr2.empty()) {
                createAutoConstraints(sugConstr2, getHighestCurveIndex(), Sketcher::PointPos::end);
                sugConstr2.clear();
            }

            tryAutoRecomputeIfNotSolve(
                static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

            EditCurve.clear();
            drawEdit(EditCurve);

            bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);
            if (continuousMode) {
                // This code enables the continuous creation mode.
                Mode = STATUS_SEEK_First;
                EditCurve.resize(2);
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
        return QString::fromLatin1("Sketcher_Pointer_Create_Line");
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    std::vector<AutoConstraint> sugConstr1, sugConstr2;
};

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerLine_H

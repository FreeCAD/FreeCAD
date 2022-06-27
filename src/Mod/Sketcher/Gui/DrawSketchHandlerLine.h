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

#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

class DrawSketchHandlerLine: public DrawSketchHandler
{
public:
    DrawSketchHandlerLine():Mode(STATUS_SEEK_First),EditCurve(2){}
    virtual ~DrawSketchHandlerLine(){}
    /// mode table
    enum SelectMode {
        STATUS_SEEK_First,      /**< enum value ----. */
        STATUS_SEEK_Second,     /**< enum value ----. */
        STATUS_End
    };

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        if (Mode==STATUS_SEEK_First) {
            setPositionText(onSketchPos);
            if (seekAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }
        }
        else if (Mode==STATUS_SEEK_Second){
            float length = (onSketchPos - EditCurve[0]).Length();
            float angle = (onSketchPos - EditCurve[0]).GetAngle(Base::Vector2d(1.f,0.f));
            SbString text;
            text.sprintf(" (%.1f,%.1fdeg)", length, angle * 180 / M_PI);
            setPositionText(onSketchPos, text);

            EditCurve[1] = onSketchPos;
            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2, onSketchPos, onSketchPos - EditCurve[0])) {
                renderSuggestConstraintsCursor(sugConstr2);
                return;
            }
        }
        applyCursor();
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode==STATUS_SEEK_First){
            EditCurve[0] = onSketchPos;

            Mode = STATUS_SEEK_Second;
        }
        else {
            EditCurve[1] = onSketchPos;
            drawEdit(EditCurve);
            Mode = STATUS_End;
        }
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode==STATUS_End){
            unsetCursor();
            resetPositionText();

            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch line"));
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)),%s)",
                          EditCurve[0].x,EditCurve[0].y,EditCurve[1].x,EditCurve[1].y,
                          geometryCreationMode==Construction?"True":"False");

                Gui::Command::commitCommand();
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("Failed to add line: %s\n", e.what());
                Gui::Command::abortCommand();
            }

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool avoidredundant = sketchgui->AvoidRedundant.getValue()  && sketchgui->Autoconstraints.getValue();

            if(avoidredundant)
                removeRedundantHorizontalVertical(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()),sugConstr1,sugConstr2);

            // add auto constraints for the line segment start
            if (!sugConstr1.empty()) {
                createAutoConstraints(sugConstr1, getHighestCurveIndex(), Sketcher::PointPos::start);
                sugConstr1.clear();
            }

            // add auto constraints for the line segment end
            if (!sugConstr2.empty()) {
                createAutoConstraints(sugConstr2, getHighestCurveIndex(), Sketcher::PointPos::end);
                sugConstr2.clear();
            }

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

            EditCurve.clear();
            drawEdit(EditCurve);

            bool continuousMode = hGrp->GetBool("ContinuousCreationMode",true);
            if(continuousMode){
                // This code enables the continuous creation mode.
                Mode=STATUS_SEEK_First;
                EditCurve.resize(2);
                applyCursor();
                /* It is ok not to call to purgeHandler
                * in continuous creation mode because the
                * handler is destroyed by the quit() method on pressing the
                * right button of the mouse */
            }
            else{
                sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
            }
        }
        return true;
    }

private:

    virtual QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Create_Line");
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    std::vector<AutoConstraint> sugConstr1, sugConstr2;
};

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerLine_H


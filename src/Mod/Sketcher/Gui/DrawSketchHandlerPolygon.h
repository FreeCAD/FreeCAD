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

#ifndef SKETCHERGUI_DrawSketchHandlerPolygon_H
#define SKETCHERGUI_DrawSketchHandlerPolygon_H

#include <Gui/Notifications.h>

#include "GeometryCreationMode.h"
#include "SketcherRegularPolygonDialog.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerRegularPolygon: public DrawSketchHandler
{
public:
    explicit DrawSketchHandlerRegularPolygon(size_t nof_corners)
        : Corners(nof_corners)
        , AngleOfSeparation(2.0 * M_PI / static_cast<double>(Corners))
        , cos_v(cos(AngleOfSeparation))
        , sin_v(sin(AngleOfSeparation))
        , Mode(STATUS_SEEK_First)
        , EditCurve(Corners + 1)
    {}
    ~DrawSketchHandlerRegularPolygon() override
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
            EditCurve[0] = Base::Vector2d(onSketchPos.x, onSketchPos.y);
            EditCurve[Corners] = Base::Vector2d(onSketchPos.x, onSketchPos.y);

            Base::Vector2d dV = onSketchPos - StartPos;
            double rx = dV.x;
            double ry = dV.y;
            for (int i = 1; i < static_cast<int>(Corners); i++) {
                const double old_rx = rx;
                rx = cos_v * rx - sin_v * ry;
                ry = cos_v * ry + sin_v * old_rx;
                EditCurve[i] = Base::Vector2d(StartPos.x + rx, StartPos.y + ry);
            }

            // Display radius for user
            const float radius = dV.Length();
            const float angle = (180.0 / M_PI) * atan2(dV.y, dV.x);

            if (showCursorCoords()) {
                SbString text;
                std::string radiusString = lengthToDisplayFormat(radius, 1);
                std::string angleString = angleToDisplayFormat(angle, 1);
                text.sprintf(" (R%s, %s)", radiusString.c_str(), angleString.c_str());
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2, onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstr2);
                return;
            }
        }
        applyCursor();
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {
            StartPos = onSketchPos;
            Mode = STATUS_SEEK_Second;
        }
        else {
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
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add hexagon"));

            try {
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "import ProfileLib.RegularPolygon\n"
                                        "ProfileLib.RegularPolygon.makeRegularPolygon(%s,%i,App."
                                        "Vector(%f,%f,0),App.Vector(%f,%f,0),%s)",
                                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(),
                                        Corners,
                                        StartPos.x,
                                        StartPos.y,
                                        EditCurve[0].x,
                                        EditCurve[0].y,
                                        geometryCreationMode == Construction ? "True" : "False");

                Gui::Command::commitCommand();

                // add auto constraints at the center of the polygon
                if (!sugConstr1.empty()) {
                    createAutoConstraints(sugConstr1,
                                          getHighestCurveIndex(),
                                          Sketcher::PointPos::mid);
                    sugConstr1.clear();
                }

                // add auto constraints to the last side of the polygon
                if (!sugConstr2.empty()) {
                    createAutoConstraints(sugConstr2,
                                          getHighestCurveIndex() - 1,
                                          Sketcher::PointPos::end);
                    sugConstr2.clear();
                }

                tryAutoRecomputeIfNotSolve(
                    static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Failed to add polygon"));

                Gui::Command::abortCommand();

                tryAutoRecompute(static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
            }

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);

            if (continuousMode) {
                // This code enables the continuous creation mode.
                Mode = STATUS_SEEK_First;
                EditCurve.clear();
                drawEdit(EditCurve);
                EditCurve.resize(Corners + 1);
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
        return QString::fromLatin1("Sketcher_Pointer_Regular_Polygon");
    }

protected:
    const size_t Corners;
    const double AngleOfSeparation;
    const double cos_v, sin_v;
    SelectMode Mode;
    Base::Vector2d StartPos;
    std::vector<Base::Vector2d> EditCurve;
    std::vector<AutoConstraint> sugConstr1, sugConstr2;
};


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerPolygon_H

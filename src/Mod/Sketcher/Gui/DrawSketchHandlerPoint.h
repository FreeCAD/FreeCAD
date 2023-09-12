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

#ifndef SKETCHERGUI_DrawSketchHandlerPoint_H
#define SKETCHERGUI_DrawSketchHandlerPoint_H

#include <Gui/Notifications.h>

#include "GeometryCreationMode.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerPoint: public DrawSketchHandler
{
public:
    DrawSketchHandlerPoint()
        : selectionDone(false)
    {}
    ~DrawSketchHandlerPoint() override
    {}

    void mouseMove(Base::Vector2d onSketchPos) override
    {
        setPositionText(onSketchPos);
        if (seekAutoConstraint(sugConstr, onSketchPos, Base::Vector2d(0.f, 0.f))) {
            renderSuggestConstraintsCursor(sugConstr);
            return;
        }
        applyCursor();
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        EditPoint = onSketchPos;
        selectionDone = true;
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (selectionDone) {
            unsetCursor();
            resetPositionText();

            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch point"));
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                                      EditPoint.x,
                                      EditPoint.y);

                Gui::Command::commitCommand();
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Failed to add point"));

                Gui::Command::abortCommand();
            }

            // add auto constraints for the line segment start
            if (!sugConstr.empty()) {
                createAutoConstraints(sugConstr, getHighestCurveIndex(), Sketcher::PointPos::start);
                sugConstr.clear();
            }

            tryAutoRecomputeIfNotSolve(
                static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);
            if (continuousMode) {
                // This code enables the continuous creation mode.
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
        return QString::fromLatin1("Sketcher_Pointer_Create_Point");
    }

protected:
    bool selectionDone;
    Base::Vector2d EditPoint;
    std::vector<AutoConstraint> sugConstr;
};


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerPoint_H

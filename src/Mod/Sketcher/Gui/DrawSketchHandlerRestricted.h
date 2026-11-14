// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Ajinkya Dahale <dahale.a.p@gmail.com>              *
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

#include <algorithm>

#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/InputHint.h>
#include <Gui/Notifications.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchControllableHandler.h"
#include "DrawSketchDefaultWidgetController.h"

#include "Utils.h"

namespace SketcherGui
{

class DrawSketchHandlerRestrictedCurve;

namespace ConstructionMethods
{

enum class RestrictedCurveConstructionMethod
{
    BasisAndTwoPoints,
    End  // Must be the last one
};

}  // namespace ConstructionMethods

using DSHRestrictedCurveController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerRestrictedCurve,
    /*SelectModeT*/ StateMachines::TwoSeekEnd,
    /*PAutoConstraintSize =*/2,
    /*OnViewParametersT =*/OnViewParameters<2>,  // NOLINT
    /*WidgetParametersT =*/WidgetParameters<0>,  // NOLINT
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0>,  // NOLINT
    /*WidgetComboboxesT =*/WidgetComboboxes<0>,  // NOLINT
    /*WidgetLineEditsT =*/WidgetLineEdits<0>,
    ConstructionMethods::RestrictedCurveConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHRestrictedCurveControllerBase = DSHRestrictedCurveController::ControllerBase;

using DrawSketchHandlerRestrictedCurveBase
    = DrawSketchControllableHandler<DSHRestrictedCurveController>;

class DrawSketchHandlerRestrictedCurve: public DrawSketchHandlerRestrictedCurveBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerRestrictedCurve)

    friend DSHRestrictedCurveController;
    friend DSHRestrictedCurveControllerBase;

public:
    DrawSketchHandlerRestrictedCurve(
        int basisGeoId,
        ConstructionMethod constrMethod = ConstructionMethod::BasisAndTwoPoints
    )
        : DrawSketchHandlerRestrictedCurveBase(constrMethod)
        , basisGeoId(basisGeoId)
    {}

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        SketchObject* Obj = sketchgui->getSketchObject();
        const auto* basisCurve = Obj->getGeometry<Part::GeomCurve>(basisGeoId);
        if (state() == SelectMode::SeekFirst) {
            basisCurve->closestParameter(toVector3d(onSketchPos), firstParam);

            // TODO make original curve less conspicuous

            // TODO draw first point (take cues from knot insertion)
            Base::Vector3d pointOnCurve3d = basisCurve->value(firstParam);
            Base::Vector2d pointOnCurve = toVector2d(pointOnCurve3d);

            toolWidgetManager.drawPositionAtCursor(pointOnCurve);
        }
        else if (state() == SelectMode::SeekSecond) {
            basisCurve->closestParameter(toVector3d(onSketchPos), lastParam);
            // TODO make original curve less conspicuous
            Base::Vector2d firstPointOnCurve = toVector2d(basisCurve->value(firstParam));

            Base::Vector2d lastPointOnCurve = toVector2d(basisCurve->value(lastParam));

            // TODO draw second point (take cues from knot insertion)
            toolWidgetManager.drawPositionAtCursor(lastPointOnCurve);

            // TODO draw restricted curve
            // for now just draw a line segment
            drawEdit({firstPointOnCurve, lastPointOnCurve});
        }
    }

    void executeCommands() override
    {
        openCommand(QT_TRANSLATE_NOOP("Command", "Restricted Curve"));
        // TODO: use the generalized framework and `PythonConverter`

        std::string sketchObj = Gui::Command::getObjectCmd(sketchgui->getObject());
        Gui::Command::doCommand(Gui::Command::Doc, "ActiveSketch = %s\n", sketchObj.c_str());
        // TODO: Add python command with geoid (has to stay within sketcher)
        if (basisGeoId >= 0) {
            Gui::Command::doCommand(
                Gui::Command::Doc,
                "resc = Part.RestrictedCurve(ActiveSketch.Geometry[%d], %f, %f)",
                basisGeoId,
                firstParam,
                lastParam
            );
        }
        else {
            Gui::Command::doCommand(
                Gui::Command::Doc,
                "resc = Part.RestrictedCurve(ActiveSketch.ExternalGeo[%d], %f, %f)",
                -basisGeoId - 1,
                firstParam,
                lastParam
            );
        }
        Gui::Command::doCommand(Gui::Command::Doc, "rcGeoId = ActiveSketch.addGeometry(resc, False)");
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "ActiveSketch.addConstraint(Sketcher.Constraint('Restriction', %d, rcGeoId))",
            basisGeoId
        );
        Gui::Command::doCommand(Gui::Command::Doc, "ActiveSketch.setConstruction(%d, True)", basisGeoId);

        commitCommand();
    }

    std::string getToolName() const override
    {
        return "DSH_RestrictedCurve";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Create_Restricted_Curve");
    }

    std::unique_ptr<QWidget> createWidget() const override
    {
        return std::make_unique<SketcherToolDefaultWidget>();
    }

    bool isWidgetVisible() const override
    {
        return true;
    };

    QPixmap getToolIcon() const override
    {
        return Gui::BitmapFactory().pixmap("Sketcher_RestrictedCurve");
    }

    QString getToolWidgetText() const override
    {
        return QString(tr("Restricted Curve Parameters"));
    }

    void activated() override
    {
        DrawSketchDefaultHandler::activated();
        continuousMode = false;
        firstCurveCreated = getHighestCurveIndex() + 1;
    }

private:
    int basisGeoId, firstCurveCreated;
    double firstParam, lastParam;
};

template<>
auto DSHRestrictedCurveControllerBase::getState(int labelindex) const
{
    switch (labelindex) {
        case OnViewParameter::First:
            return SelectMode::SeekFirst;
            break;
        case OnViewParameter::Second:
            return SelectMode::SeekSecond;
            break;
        default:
            THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template<>
void DSHRestrictedCurveController::computeNextDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];

            if (firstParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::SeekSecond);
            }
        } break;
        case SelectMode::SeekSecond: {
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (secondParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}
}  // namespace SketcherGui

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

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/InputHint.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "GeometryCreationMode.h"

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

#include <vector>
#include <algorithm>

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerPoint;

using DSHPointController = DrawSketchController<
    DrawSketchHandlerPoint,
    StateMachines::OneSeekEnd,
    /*PAutoConstraintSize =*/1,
    /*OnViewParametersT =*/OnViewParameters<2>>;

using DrawSketchHandlerPointBase = DrawSketchControllableHandler<DSHPointController>;

class DrawSketchHandlerPoint: public DrawSketchHandlerPointBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerPoint)

    // Allow specialisations of controllers access to private members
    friend DSHPointController;

public:
    DrawSketchHandlerPoint() = default;
    ~DrawSketchHandlerPoint() override = default;

private:
    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;
        return {
            {tr("%1 place a point", "Sketcher Point: hint"), {MouseLeft}},
        };
    }

    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);

                editPoint = onSketchPos;

                seekAndRenderAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch point"));
            Gui::cmdAppObjectArgs(
                sketchgui->getObject(),
                "addGeometry(Part.Point(App.Vector(%f,%f,0)), %s)",
                editPoint.x,
                editPoint.y,
                isConstructionMode() ? "True" : "False"
            );

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to add point")
            );

            Gui::Command::abortCommand();
        }
    }

    void createAutoConstraints() override
    {

        if (!sugConstraints[0].empty()) {
            DrawSketchHandler::createAutoConstraints(
                sugConstraints[0],
                getHighestCurveIndex(),
                Sketcher::PointPos::start
            );
            sugConstraints[0].clear();
        }
    }

    std::string getToolName() const override
    {
        return "DSH_Point";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Create_Point");
    }

    std::unique_ptr<QWidget> createWidget() const override
    {
        return std::make_unique<SketcherToolDefaultWidget>();
    }

private:
    Base::Vector2d editPoint;
};

template<>
auto DSHPointController::getState(int labelindex) const
{
    switch (labelindex) {
        case OnViewParameter::First:
        case OnViewParameter::Second:
            return SelectMode::SeekFirst;
            break;
        default:
            THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template<>
void DSHPointController::configureOnViewParameters()
{
    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);
}

template<>
void DSHPointController::adaptDrawingToOnViewParameterChange(int labelindex, double value)
{
    switch (labelindex) {
        case OnViewParameter::First:
            handler->editPoint.x = value;
            break;
        case OnViewParameter::Second:
            handler->editPoint.y = value;
            break;
    }
    onViewParameters[OnViewParameter::First]->setPoints(
        Base::Vector3d(0., 0., 0.),
        Base::Vector3d(handler->editPoint.x, handler->editPoint.y, 0.)
    );
    onViewParameters[OnViewParameter::Second]->setPoints(
        Base::Vector3d(0., 0., 0.),
        Base::Vector3d(handler->editPoint.x, handler->editPoint.y, 0.)
    );
}

template<>
void DSHPointController::doEnforceControlParameters(Base::Vector2d& onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (firstParam->isSet) {
                onSketchPos.x = firstParam->getValue();
            }

            if (secondParam->isSet) {
                onSketchPos.y = secondParam->getValue();
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHPointController::adaptParameters(Base::Vector2d onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (!firstParam->isSet) {
                setOnViewParameterValue(OnViewParameter::First, onSketchPos.x);
            }

            if (!secondParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Second, onSketchPos.y);
            }

            bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
            firstParam->setLabelAutoDistanceReverse(!sameSign);
            secondParam->setLabelAutoDistanceReverse(sameSign);
            firstParam->setPoints(
                Base::Vector3d(0., 0., 0.),
                Base::Vector3d(onSketchPos.x, onSketchPos.y, 0.)
            );
            secondParam->setPoints(
                Base::Vector3d(0., 0., 0.),
                Base::Vector3d(onSketchPos.x, onSketchPos.y, 0.)
            );
        } break;
        default:
            break;
    }
}

template<>
void DSHPointController::computeNextDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (firstParam->hasFinishedEditing && secondParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::End);
                // handler->finish(); // Called by the change of mode
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHPointController::addConstraints()
{
    int firstCurve = handler->getHighestCurveIndex();

    auto x0 = onViewParameters[OnViewParameter::First]->getValue();
    auto y0 = onViewParameters[OnViewParameter::Second]->getValue();

    auto x0set = onViewParameters[OnViewParameter::First]->isSet;
    auto y0set = onViewParameters[OnViewParameter::Second]->isSet;

    using namespace Sketcher;

    if (x0set && y0set && x0 == 0. && y0 == 0.) {
        ConstraintToAttachment(
            GeoElementId(firstCurve, PointPos::start),
            GeoElementId::RtPnt,
            x0,
            handler->sketchgui->getObject()
        );
    }
    else {
        if (x0set) {
            ConstraintToAttachment(
                GeoElementId(firstCurve, PointPos::start),
                GeoElementId::VAxis,
                x0,
                handler->sketchgui->getObject()
            );
        }

        if (y0set) {
            ConstraintToAttachment(
                GeoElementId(firstCurve, PointPos::start),
                GeoElementId::HAxis,
                y0,
                handler->sketchgui->getObject()
            );
        }
    }
}

}  // namespace SketcherGui

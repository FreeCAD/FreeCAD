// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *   Copyright (c) 2026 Loke S. Haugsnes <Lokesh@live.no>                  *
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
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "SnapManager.h"


#include <Base/Tools.h>
#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <Mod/Part/App/Geometry2d.h>


#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

#include <vector>
#include <algorithm>

// Parabola governing equation:
// point(t) = origin + t*t/(4*focal)*XDir + t*YDir

namespace SketcherGui
{

class DrawSketchHandlerArcOfParabola;

using DSHArcOfParabolaController = DrawSketchController<
    DrawSketchHandlerArcOfParabola,
    StateMachines::FourSeekEnd,
    /*PAutoConstraintSize =*/4,
    /*OnViewParametersT =*/OnViewParameters<6>>;

using DrawSketchHandlerArcOfParabolaBase = DrawSketchControllableHandler<DSHArcOfParabolaController>;

class DrawSketchHandlerArcOfParabola: public DrawSketchHandlerArcOfParabolaBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerArcOfParabola)

    friend DSHArcOfParabolaController;

public:
    explicit DrawSketchHandlerArcOfParabola()
        : DrawSketchHandlerArcOfParabolaBase()
        , centerPoint({0.0, 0.0})
        , focusPoint({0.0, 0.0})
        , startPoint(0)
        , endPoint(0)
        , valid(true)
        , parabolaGeoId(Sketcher::GeoEnum::GeoUndef)
    {}

    ~DrawSketchHandlerArcOfParabola() override = default;

private:
    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;

        return Gui::lookupHints<SelectMode>(
            state(),
            {
                {.state = SelectMode::SeekFirst,
                 .hints =
                     {
                         {tr("%1 pick focus point"), {MouseLeft}},
                     }},
                {.state = SelectMode::SeekSecond,
                 .hints =
                     {
                         {tr("%1 pick axis point"), {MouseLeft}},
                     }},
                {.state = SelectMode::SeekThird,
                 .hints =
                     {
                         {tr("%1 pick starting point"), {MouseLeft}},
                     }},
                {.state = SelectMode::SeekFourth,
                 .hints =
                     {
                         {tr("%1 pick end point"), {MouseLeft}},
                     }},
            });
    }

    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        using std::numbers::pi;
        valid = true;
        switch (state()) {
            case SelectMode::SeekFirst: {
                focusPoint = onSketchPos;
                toolWidgetManager.drawPositionAtCursor(onSketchPos);
                seekAndRenderAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            case SelectMode::SeekSecond: {
                centerPoint = onSketchPos;
                if ((centerPoint - focusPoint).Length() < Precision::Confusion()) {
                    valid = false;
                    break;
                }

                toolWidgetManager.drawDirectionAtCursor(onSketchPos, focusPoint);
                seekAndRenderAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            case SelectMode::SeekThird: {
                Base::Vector2d delta23 = onSketchPos - centerPoint;
                startPoint = delta23.x * axis().Rotate(std::numbers::pi / 2).x
                    + delta23.y * axis().Rotate(std::numbers::pi / 2).y;

                toolWidgetManager.drawPositionAtCursor(onSketchPos);
                seekAndRenderAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            case SelectMode::SeekFourth: {
                Base::Vector2d delta24 = onSketchPos - centerPoint;
                endPoint = delta24.x * axis().Rotate(std::numbers::pi / 2).x
                    + delta24.y * axis().Rotate(std::numbers::pi / 2).y;

                if (abs(endPoint - startPoint) < Precision::Confusion()) {
                    valid = false;
                    break;
                }

                toolWidgetManager.drawPositionAtCursor(onSketchPos);
                seekAndRenderAutoConstraint(sugConstraints[3], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            default:
                return;
        }

        CreateAndDrawShapeGeometry();
    }

    void executeCommands() override
    {
        try {
            openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc of parabola"));

            parabolaGeoId = getHighestCurveIndex() + 1;

            createShape(true);

            commandAddShapeGeometryAndConstraints();

            Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", parabolaGeoId);

            commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to add arc of parabola")
            );

            abortCommand();
            THROWM(
                Base::RuntimeError,
                QT_TRANSLATE_NOOP(
                    "Notifications",
                    "Tool execution aborted"
                ) "\n"
            )  // This prevents constraints from being
               // applied on non existing geometry
        }
    }

    void generateAutoConstraints() override
    {
        auto& ac1 = sugConstraints[0];
        auto& ac2 = sugConstraints[1];
        auto& ac3 = sugConstraints[2];
        auto& ac4 = sugConstraints[3];

        if (!ac1.empty()) {
            generateAutoConstraintsOnElement(ac1, parabolaGeoId + 1, Sketcher::PointPos::start);
        }

        if (!ac2.empty()) {
            generateAutoConstraintsOnElement(ac2, parabolaGeoId, Sketcher::PointPos::mid);
        }

        if (!ac3.empty()) {
            generateAutoConstraintsOnElement(
                ac3,
                parabolaGeoId,
                (startPoint < endPoint) ? Sketcher::PointPos::start : Sketcher::PointPos::end
            );
        }

        if (!ac4.empty()) {
            generateAutoConstraintsOnElement(
                ac4,
                parabolaGeoId,
                (startPoint < endPoint) ? Sketcher::PointPos::end : Sketcher::PointPos::start
            );
        }

        // Ensure temporary autoconstraints do not generate a redundancy and that the geometry
        // parameters are accurate This is particularly important for adding widget mandated
        // constraints.
        removeRedundantAutoConstraints();
    }

    void createAutoConstraints() override
    {
        // execute python command to create autoconstraints
        createGeneratedAutoConstraints(true);

        sugConstraints[0].clear();
        sugConstraints[1].clear();
        sugConstraints[2].clear();
        sugConstraints[3].clear();
    }

    std::string getToolName() const override
    {
        return "DSH_ArcOfParabola";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Create_ArcOfParabola");
    }

    QPixmap getToolIcon() const override
    {
        return Gui::BitmapFactory().pixmap("Sketcher_CreateParabolic_Arc");
    }

    bool canGoToNextMode() override
    {
        if (!valid) {
            return false;
        }
        return true;
    }

    void angleSnappingControl() override
    {
        setAngleSnapping(false);
    }

    void createShape(bool onlyeditoutline) override
    {
        Q_UNUSED(onlyeditoutline);

        ShapeGeometry.clear();

        if (!valid) {
            return;
        }

        if (state() == SelectMode::SeekSecond) {
            addLineToShapeGeometry(toVector3d(focusPoint), toVector3d(centerPoint), isConstructionMode());
        }

        // starting point can be 0 but then the curve cant be displayed
        if (state() == SelectMode::SeekThird && abs(startPoint) > Precision::Confusion()) {
            addArcOfParabolaToShapeGeometry(
                toVector3d(axis()),
                toVector3d(centerPoint),
                focal(),
                startPoint,
                -startPoint,
                isConstructionMode()
            );
        }

        if (state() == SelectMode::SeekFourth || state() == SelectMode::End) {
            addArcOfParabolaToShapeGeometry(
                toVector3d(axis()),
                toVector3d(centerPoint),
                focal(),
                (startPoint < endPoint) ? startPoint : endPoint,
                (startPoint < endPoint) ? endPoint : startPoint,
                isConstructionMode()
            );
        }
    }

private:
    Base::Vector2d centerPoint, focusPoint;
    double startPoint, endPoint;
    bool valid;
    int parabolaGeoId;

    Base::Vector2d axis() const
    {
        return (focusPoint - centerPoint).Normalize();
    }

    double focal() const
    {
        return (focusPoint - centerPoint).Length();
    }
};

template<>
auto DSHArcOfParabolaController::getState(int labelindex) const
{
    switch (labelindex) {
        case OnViewParameter::First:
        case OnViewParameter::Second:
            return SelectMode::SeekFirst;
            break;
        case OnViewParameter::Third:
        case OnViewParameter::Fourth:
            return SelectMode::SeekSecond;
            break;
        case OnViewParameter::Fifth:
            return SelectMode::SeekThird;
            break;
        case OnViewParameter::Sixth:
            return SelectMode::SeekFourth;
            break;
        default:
            THROWM(Base::ValueError, "OnViewParameter index without an associated machine state")
    }
}

template<>
void DSHArcOfParabolaController::configureOnViewParameters()
{
    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);

    onViewParameters[OnViewParameter::Third]->setLabelType(
        Gui::SoDatumLabel::RADIUS,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
    onViewParameters[OnViewParameter::Fourth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );

    onViewParameters[OnViewParameter::Fifth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
    onViewParameters[OnViewParameter::Sixth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
}

template<>
void DSHArcOfParabolaController::doEnforceControlParameters(Base::Vector2d& onSketchPos)
{
    handler->updateDataAndDrawToPosition(
        onSketchPos
    );  // ensure that the member variables are updated to date values before enforcing parameters
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
        case SelectMode::SeekSecond: {
            auto& focalParam = onViewParameters[OnViewParameter::Third];
            auto& angleParam = onViewParameters[OnViewParameter::Fourth];

            if (!focalParam->isSet && !angleParam->isSet) {
                return;
            }

            double focal = handler->focal();
            auto axis = handler->axis();

            if (focalParam->isSet) {
                focal = focalParam->getValue();
                if (focal < Precision::Confusion() && focalParam->hasFinishedEditing) {
                    unsetOnViewParameter(focalParam.get());
                    return;
                }
            }

            if (angleParam->isSet) {
                double angle = Base::toRadians(angleParam->getValue());
                axis = {cos(angle), sin(angle)};
            }

            onSketchPos = handler->focusPoint + focal * -axis;
        } break;
        case SelectMode::SeekThird: {
            auto& startParam = onViewParameters[OnViewParameter::Fifth];

            if (!startParam->isSet) {
                return;
            }

            auto start = startParam->getValue();

            onSketchPos = handler->centerPoint + start * handler->axis().Rotate(std::numbers::pi / 2);
        } break;
        case SelectMode::SeekFourth: {
            auto& arcParam = onViewParameters[OnViewParameter::Sixth];

            if (!arcParam->isSet) {
                return;
            }

            double arc = -arcParam->getValue();

            if (std::abs(arc) < Precision::Confusion() && arcParam->hasFinishedEditing) {
                unsetOnViewParameter(arcParam.get());
                return;
            }

            double end = handler->endPoint;

            onSketchPos = handler->centerPoint + end * handler->axis().Rotate(std::numbers::pi / 2);
        } break;
        default:
            break;
    }
}

template<>
void DSHArcOfParabolaController::adaptParameters(Base::Vector2d onSketchPos)
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
            firstParam->setPoints(Base::Vector3d(), toVector3d(onSketchPos));
            secondParam->setPoints(Base::Vector3d(), toVector3d(onSketchPos));
        } break;
        case SelectMode::SeekSecond: {
            auto& focalParam = onViewParameters[OnViewParameter::Third];
            auto& angleParam = onViewParameters[OnViewParameter::Fourth];

            if (!focalParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Third, handler->focal());
            }

            if (!angleParam->isSet) {
                setOnViewParameterValue(
                    OnViewParameter::Fourth,
                    Base::toDegrees(handler->axis().Angle()),
                    Base::Unit::Angle
                );
            }

            Base::Vector3d start = toVector3d(handler->focusPoint);
            Base::Vector3d end = toVector3d(onSketchPos);

            focalParam->setPoints(start, end);
            angleParam->setPoints(start, Base::Vector3d());
            angleParam->setLabelRange(handler->axis().Angle());
        } break;
        case SelectMode::SeekThird: {
            auto& startAngleParam = onViewParameters[OnViewParameter::Fifth];

            if (!startAngleParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Fifth, handler->startPoint, Base::Unit::One);
            }

            Base::Vector3d start = toVector3d(handler->centerPoint);

            startAngleParam->setPoints(start, Base::Vector3d());
            startAngleParam->setLabelStartAngle(handler->axis().Angle());
            startAngleParam->setLabelRange(0);
        } break;
        case SelectMode::SeekFourth: {
            auto& arcAngleParam = onViewParameters[OnViewParameter::Sixth];

            if (!arcAngleParam->isSet) {
                setOnViewParameterValue(
                    OnViewParameter::Sixth,
                    handler->endPoint - handler->startPoint,
                    Base::Unit::One
                );
            }

            Base::Vector3d start = toVector3d(handler->centerPoint);

            arcAngleParam->setPoints(start, Base::Vector3d());
            arcAngleParam->setLabelStartAngle(handler->axis().Angle());
            arcAngleParam->setLabelRange(0);
        } break;
        default:
            break;
    }
}

template<>
void DSHArcOfParabolaController::computeNextDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (firstParam->hasFinishedEditing && secondParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::SeekSecond);
            }
        } break;
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            if (thirdParam->hasFinishedEditing && fourthParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::SeekThird);
            }
        } break;
        case SelectMode::SeekThird: {
            auto& fifthParam = onViewParameters[OnViewParameter::Fifth];

            if (fifthParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::SeekFourth);
            }
        } break;
        case SelectMode::SeekFourth: {
            auto& sixth = onViewParameters[OnViewParameter::Sixth];

            if (sixth->hasFinishedEditing) {
                handler->setNextState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHArcOfParabolaController::addConstraints()
{
    App::DocumentObject* obj = handler->sketchgui->getObject();

    int firstCurve = handler->parabolaGeoId;
    int focusPoint = firstCurve + 1;
    int line = firstCurve + 2;

    using namespace Sketcher;

    bool x0set = onViewParameters[OnViewParameter::First]->isSet;
    bool y0set = onViewParameters[OnViewParameter::Second]->isSet;
    bool focalSet = onViewParameters[OnViewParameter::Third]->isSet;
    bool axisSet = onViewParameters[OnViewParameter::Fourth]->isSet;
    // can't constrain the parameters.

    auto constraintx0 = [&]() {
        ConstraintToAttachment(
            GeoElementId(focusPoint, PointPos::mid),
            GeoElementId::VAxis,
            handler->focusPoint.x,
            obj
        );
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(
            GeoElementId(focusPoint, PointPos::mid),
            GeoElementId::HAxis,
            handler->focusPoint.y,
            obj
        );
    };

    auto constraintFocal = [&]() {
        Gui::cmdAppObjectArgs(
            obj,
            "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
            line,
            1,
            line,
            2,
            handler->focal()
        );
    };

    auto constraintAngle = [&]() {
        Gui::cmdAppObjectArgs(
            obj,
            "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
            line,
            handler->axis().Angle()
        );
    };

    if (handler->AutoConstraints.empty()) {  // No valid diagnosis. Every constraint can be added.
        if (x0set && y0set && handler->focusPoint.IsNull(Precision::Confusion())) {
            ConstraintToAttachment(GeoElementId(focusPoint, PointPos::mid), GeoElementId::RtPnt, 0., obj);
        }
        else {
            if (x0set) {
                constraintx0();
            }

            if (y0set) {
                constrainty0();
            }
        }

        if (focalSet) {
            constraintFocal();
        }
        if (axisSet) {
            constraintAngle();
        }
    }
    else {  // Valid diagnosis. Must check which constraints may be added.
        auto centerPointInfo = handler->getPointInfo(GeoElementId(focusPoint, PointPos::mid));

        if (x0set && centerPointInfo.isXDoF()) {
            constraintx0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition
            // get updated point position
            centerPointInfo = handler->getPointInfo(GeoElementId(focusPoint, PointPos::mid));
        }

        if (y0set && centerPointInfo.isYDoF()) {
            constrainty0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition
            // get updated point position
            centerPointInfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));
        }

        int lineDoFs = handler->getLineDoFs(line);

        if (focalSet && lineDoFs > 0) {
            constraintFocal();
            handler->diagnoseWithAutoConstraints();
            lineDoFs = handler->getLineDoFs(line);
        }

        if (axisSet && lineDoFs > 0) {
            constraintAngle();
        }
    }
}

}  // namespace SketcherGui

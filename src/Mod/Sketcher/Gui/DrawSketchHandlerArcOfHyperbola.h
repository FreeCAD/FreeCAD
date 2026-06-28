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

// Hyperbola governing equation:
// point(t) = origin + majRad*cosh(t)*majorDir + minRad*sinh(t)*minorDir
// Note that major radius can be less than minor radius, they are just the coefficients of the
// parametric equation. But the major direction is always the direction with respect to which way
// the hyperbola opens. This is different to how the more commonly known ellipse is defined.
// Angles are hyperbolic angles not the same as circular angles, look it up for more info

namespace SketcherGui
{

class DrawSketchHandlerArcOfHyperbola;

using DSHArcOfHyperbolaController = DrawSketchController<
    DrawSketchHandlerArcOfHyperbola,
    StateMachines::FourSeekEnd,
    /*PAutoConstraintSize =*/4,
    /*OnViewParametersT =*/OnViewParameters<7>>;

using DrawSketchHandlerArcOfHyperbolaBase = DrawSketchControllableHandler<DSHArcOfHyperbolaController>;

class DrawSketchHandlerArcOfHyperbola: public DrawSketchHandlerArcOfHyperbolaBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerArcOfHyperbola)

    friend DSHArcOfHyperbolaController;

public:
    explicit DrawSketchHandlerArcOfHyperbola()
        : DrawSketchHandlerArcOfHyperbolaBase()
        , centerPoint({0.0, 0.0})
        , axisPoint({0.0, 0.0})
        , minorRadius(0)
        , startAngle(0)
        , endAngle(0)
        , valid(true)
        , minorRadiusSet(false)
        , hyperbolaGeoId(Sketcher::GeoEnum::GeoUndef)
    {}

    ~DrawSketchHandlerArcOfHyperbola() override = default;

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
                         {tr("%1 pick center point"), {MouseLeft}},
                     }},
                {.state = SelectMode::SeekSecond,
                 .hints =
                     {
                         {tr("%1 pick axis point"), {MouseLeft}},
                     }},
                {.state = SelectMode::SeekThird,
                 .hints =
                     {
                         {tr("%1 pick arc start point"), {MouseLeft}},
                     }},
                {.state = SelectMode::SeekFourth,
                 .hints =
                     {
                         {tr("%1 pick arc end point"), {MouseLeft}},
                     }},
            });
    }

    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        using std::numbers::pi;
        valid = true;
        switch (state()) {
            case SelectMode::SeekFirst: {
                centerPoint = onSketchPos;
                toolWidgetManager.drawPositionAtCursor(onSketchPos);
                seekAndRenderAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            case SelectMode::SeekSecond: {
                axisPoint = onSketchPos;
                if ((axisPoint - centerPoint).Length() < Precision::Confusion()) {
                    valid = false;
                    break;
                }

                toolWidgetManager.drawDirectionAtCursor(onSketchPos, centerPoint);
                seekAndRenderAutoConstraint(
                    sugConstraints[1],
                    onSketchPos,
                    Base::Vector2d(0.f, 0.f),
                    AutoConstraint::CURVE
                );
            } break;
            case SelectMode::SeekThird: {
                double a = majorRadius();

                Base::Vector2d aDir = majorAxis();
                Base::Vector2d bDir = minorAxis();

                Base::Vector2d delta13 = onSketchPos - centerPoint;
                Base::Vector2d delta13Prime(
                    delta13.x * aDir.x + delta13.y * aDir.y,
                    delta13.x * bDir.x + delta13.y * bDir.y
                );

                if (!minorRadiusSet) {
                    if (abs(delta13Prime.y) < Precision::Confusion()) {
                        valid = false;
                        break;
                    }

                    double denom = (delta13Prime.x * delta13Prime.x) / (a * a) - 1.0;
                    if (denom < Precision::Confusion()) {
                        valid = false;
                        break;
                    }
                    minorRadius = std::sqrt((delta13Prime.y * delta13Prime.y) / denom);
                }
                startAngle = atanh((delta13Prime.y * a) / (delta13Prime.x * minorRadius));
                toolWidgetManager.drawPositionAtCursor(onSketchPos);
                seekAndRenderAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            case SelectMode::SeekFourth: {
                Base::Vector2d aDir = majorAxis();
                Base::Vector2d bDir = minorAxis();
                Base::Vector2d delta14 = onSketchPos - centerPoint;
                Base::Vector2d delta14Prime(
                    delta14.x * aDir.x + delta14.y * aDir.y,
                    delta14.x * bDir.x + delta14.y * bDir.y
                );

                if (std::abs((delta14Prime.y * majorRadius()) / (delta14Prime.x * minorRadius)) > 1) {
                    valid = false;
                    break;
                }
                endAngle = atanh((delta14Prime.y * majorRadius()) / (delta14Prime.x * minorRadius));
                if (abs(getArcAngle()) < Precision::Confusion()) {
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
            openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc of hyperbola"));

            hyperbolaGeoId = getHighestCurveIndex() + 1;

            createShape(true);

            commandAddShapeGeometryAndConstraints();

            Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", hyperbolaGeoId);

            commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to add arc of hyperbola")
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
            generateAutoConstraintsOnElement(ac1, hyperbolaGeoId, Sketcher::PointPos::mid);
        }

        if (!ac2.empty()) {
            generateAutoConstraintsOnElement(ac2, hyperbolaGeoId, Sketcher::PointPos::none);
        }

        if (!ac3.empty()) {
            generateAutoConstraintsOnElement(
                ac3,
                hyperbolaGeoId,
                (getArcAngle() > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end
            );
        }

        if (!ac4.empty()) {
            generateAutoConstraintsOnElement(
                ac4,
                hyperbolaGeoId,
                (getArcAngle() > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start
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
        return "DSH_ArcOfHyperbola";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Create_ArcOfHyperbola");
    }

    QPixmap getToolIcon() const override
    {
        return Gui::BitmapFactory().pixmap("Sketcher_CreateHyperbolic_Arc");
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
            addLineToShapeGeometry(toVector3d(centerPoint), toVector3d(axisPoint), isConstructionMode());
        }

        // Idea to help user see the boundary of the hyperbola
        // if (state() == SelectMode::SeekThird || state() == SelectMode::SeekFourth) {
        //    addLineToShapeGeometry(
        //        toVector3d(centerPoint),
        //        toVector3d(axisPoint + minorAxis() * minorRadius),
        //        true
        //    );
        //
        //    addLineToShapeGeometry(
        //        toVector3d(centerPoint),
        //        toVector3d(axisPoint - minorAxis() * minorRadius),
        //        true
        //    );
        //}

        if (state() == SelectMode::SeekThird && std::abs(startAngle) > Precision::Confusion()) {
            addArcOfHyperbolaToShapeGeometry(
                toVector3d(centerPoint),
                toVector3d(majorAxis()),
                majorRadius(),
                minorRadius,
                startAngle,
                -startAngle,
                isConstructionMode()
            );
        }

        if (state() == SelectMode::SeekFourth || state() == SelectMode::End) {
            addArcOfHyperbolaToShapeGeometry(
                toVector3d(centerPoint),
                toVector3d(majorAxis()),
                majorRadius(),
                minorRadius,
                (startAngle < endAngle) ? startAngle : endAngle,
                (startAngle < endAngle) ? endAngle : startAngle,
                isConstructionMode()
            );
        }
    }

private:
    Base::Vector2d centerPoint, axisPoint;
    double minorRadius, startAngle, endAngle;
    bool valid, minorRadiusSet;
    int hyperbolaGeoId;

    double majorRadius() const
    {
        return (centerPoint - axisPoint).Length();
    }

    Base::Vector2d majorAxis() const
    {
        return (axisPoint - centerPoint).Normalize();
    }

    Base::Vector2d minorAxis() const
    {
        return majorAxis().Rotate(std::numbers::pi / 2);
    }

    double getArcAngle() const noexcept
    {
        return endAngle - startAngle;
    }
};

template<>
auto DSHArcOfHyperbolaController::getState(int labelindex) const
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
        case OnViewParameter::Sixth:
            return SelectMode::SeekThird;
            break;
        case OnViewParameter::Seventh:
            return SelectMode::SeekFourth;
            break;
        default:
            THROWM(Base::ValueError, "OnViewParameter index without an associated machine state")
    }
}

template<>
void DSHArcOfHyperbolaController::configureOnViewParameters()
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
        Gui::SoDatumLabel::DISTANCE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );

    // not real angle but hyperbolic angle
    onViewParameters[OnViewParameter::Sixth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );

    onViewParameters[OnViewParameter::Seventh]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
}

template<>
void DSHArcOfHyperbolaController::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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
            auto& firstRadiusParam = onViewParameters[OnViewParameter::Third];
            auto& angleParam = onViewParameters[OnViewParameter::Fourth];

            if (!firstRadiusParam->isSet && !angleParam->isSet) {
                return;
            }

            double majorRadius = handler->majorRadius();
            auto majorAxis = handler->majorAxis();

            if (firstRadiusParam->isSet) {
                majorRadius = firstRadiusParam->getValue();
                if (majorRadius < Precision::Confusion() && firstRadiusParam->hasFinishedEditing) {
                    unsetOnViewParameter(firstRadiusParam.get());
                    return;
                }
            }

            if (angleParam->isSet) {
                double angle = Base::toRadians(angleParam->getValue());
                majorAxis = {cos(angle), sin(angle)};
            }

            onSketchPos = handler->centerPoint + majorRadius * majorAxis;
        } break;
        case SelectMode::SeekThird: {
            auto& secondRadiusParam = onViewParameters[OnViewParameter::Fifth];
            auto& startAngleParam = onViewParameters[OnViewParameter::Sixth];

            if (!secondRadiusParam->isSet && !startAngleParam->isSet) {
                return;
            }

            double minorRadius = handler->minorRadius;
            auto startAngle = handler->startAngle;

            if (secondRadiusParam->isSet) {
                minorRadius = secondRadiusParam->getValue();
                handler->minorRadiusSet = true;
                if (minorRadius < Precision::Confusion() && secondRadiusParam->hasFinishedEditing) {
                    handler->minorRadiusSet = false;
                    unsetOnViewParameter(secondRadiusParam.get());
                    return;
                }
            }

            if (startAngleParam->isSet) {
                startAngle = startAngleParam->getValue();
            }

            onSketchPos = handler->centerPoint
                + handler->majorRadius() * std::cosh(startAngle) * handler->majorAxis()
                + minorRadius * std::sinh(startAngle) * handler->minorAxis();
        } break;
        case SelectMode::SeekFourth: {
            auto& arcAngleParam = onViewParameters[OnViewParameter::Seventh];

            if (!arcAngleParam->isSet) {
                return;
            }

            double arcAngle = -arcAngleParam->getValue();

            double endAngle = handler->startAngle - arcAngle;

            double xPrime = handler->majorRadius() * std::cosh(endAngle);
            double yPrime = handler->minorRadius * std::sinh(endAngle);

            Base::Vector2d delta = handler->majorAxis() * xPrime + handler->minorAxis() * yPrime;

            onSketchPos = handler->centerPoint + delta;

        } break;
        default:
            break;
    }
}

template<>
void DSHArcOfHyperbolaController::adaptParameters(Base::Vector2d onSketchPos)
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
            auto& firstRadiusParam = onViewParameters[OnViewParameter::Third];
            auto& angleParam = onViewParameters[OnViewParameter::Fourth];

            if (!firstRadiusParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Third, handler->majorRadius());
            }

            if (!angleParam->isSet) {
                setOnViewParameterValue(
                    OnViewParameter::Fourth,
                    Base::toDegrees(handler->majorAxis().Angle()),
                    Base::Unit::Angle
                );
            }

            Base::Vector3d start = toVector3d(handler->centerPoint);
            Base::Vector3d end = toVector3d(onSketchPos);

            firstRadiusParam->setPoints(start, end);
            angleParam->setPoints(start, Base::Vector3d());
            angleParam->setLabelRange(handler->majorAxis().Angle());
        } break;
        case SelectMode::SeekThird: {
            auto& secondRadiusParam = onViewParameters[OnViewParameter::Fifth];
            auto& startAngleParam = onViewParameters[OnViewParameter::Sixth];

            if (!secondRadiusParam->isSet) {
                setOnViewParameterValue(
                    OnViewParameter::Fifth,
                    handler->valid ? handler->minorRadius : 0
                );
            }

            if (!startAngleParam->isSet) {
                setOnViewParameterValue(
                    OnViewParameter::Sixth,
                    handler->valid ? handler->startAngle : 0,
                    Base::Unit::One
                );
            }

            Base::Vector3d start = toVector3d(handler->axisPoint);

            startAngleParam->setPoints(start, Base::Vector3d());
            startAngleParam->setLabelStartAngle(handler->majorAxis().Angle());
            startAngleParam->setLabelRange(0);
            secondRadiusParam->setPoints(
                start,
                start
                    + toVector3d(
                        handler->minorAxis()
                        * (handler->minorRadius
                           * ((handler->startAngle > 0) - (handler->startAngle < 0)))
                    )
            );
            if (!handler->valid) {
                secondRadiusParam->setPoints(start, start);
            }
        } break;
        case SelectMode::SeekFourth: {
            auto& arcAngleParam = onViewParameters[OnViewParameter::Seventh];

            if (!arcAngleParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Seventh, handler->getArcAngle(), Base::Unit::One);
            }

            Base::Vector3d start = toVector3d(handler->axisPoint);

            arcAngleParam->setPoints(start, Base::Vector3d());
            arcAngleParam->setLabelStartAngle(handler->majorAxis().Angle());
            arcAngleParam->setLabelRange(0);
        } break;
        default:
            break;
    }
}

template<>
void DSHArcOfHyperbolaController::computeNextDrawSketchHandlerMode()
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
            auto& sixthParam = onViewParameters[OnViewParameter::Sixth];

            if (fifthParam->hasFinishedEditing && sixthParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::SeekFourth);
            }
        } break;
        case SelectMode::SeekFourth: {
            auto& seventh = onViewParameters[OnViewParameter::Seventh];

            if (seventh->hasFinishedEditing) {
                handler->setNextState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHArcOfHyperbolaController::addConstraints()
{
    App::DocumentObject* obj = handler->sketchgui->getObject();

    int firstCurve = handler->hyperbolaGeoId;
    int majorLine = firstCurve + 1;
    int minorLine = firstCurve + 2;
    // int point = firstCurve + 3;

    using namespace Sketcher;

    auto x0set = onViewParameters[OnViewParameter::First]->isSet;
    auto y0set = onViewParameters[OnViewParameter::Second]->isSet;
    auto majorRadiusSet = onViewParameters[OnViewParameter::Third]->isSet;
    auto majorAxisSet = onViewParameters[OnViewParameter::Fourth]->isSet;
    auto minorRadiusSet = onViewParameters[OnViewParameter::Fifth]->isSet;
    // can't constrain the hyperbolic angle.

    auto constraintx0 = [&]() {
        ConstraintToAttachment(
            GeoElementId(firstCurve, PointPos::mid),
            GeoElementId::VAxis,
            handler->centerPoint.x,
            obj
        );
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(
            GeoElementId(firstCurve, PointPos::mid),
            GeoElementId::HAxis,
            handler->centerPoint.y,
            obj
        );
    };

    auto constraintMajorRadius = [&]() {
        Gui::cmdAppObjectArgs(
            obj,
            "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
            firstCurve,
            3,
            majorLine,
            1,
            handler->majorRadius()
        );
    };

    auto constraintMajorAngle = [&]() {
        Gui::cmdAppObjectArgs(
            obj,
            "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
            majorLine,
            handler->majorAxis().Angle() - std::numbers::pi  // the major line is reversed compared
                                                             // to how the input looks like
        );
    };

    auto constraintMinorRadius = [&]() {
        Gui::cmdAppObjectArgs(
            obj,
            "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
            majorLine,
            1,
            minorLine,
            1,
            handler->minorRadius
        );
    };

    if (handler->AutoConstraints.empty()) {  // No valid diagnosis. Every constraint can be added.
        if (x0set && y0set && handler->centerPoint.IsNull(Precision::Confusion())) {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::RtPnt, 0., obj);
        }
        else {
            if (x0set) {
                constraintx0();
            }

            if (y0set) {
                constrainty0();
            }
        }

        if (majorRadiusSet) {
            constraintMajorRadius();
        }
        if (majorAxisSet) {
            constraintMajorAngle();
        }

        if (minorRadiusSet) {
            constraintMinorRadius();
        }
    }
    else {  // Valid diagnosis. Must check which constraints may be added.
        auto centerPointInfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));

        if (x0set && centerPointInfo.isXDoF()) {
            constraintx0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition
            // get updated point position
            centerPointInfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));
        }

        if (y0set && centerPointInfo.isYDoF()) {
            constrainty0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition
            // get updated point position
            centerPointInfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));
        }

        centerPointInfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));

        int majorDoFs = handler->getLineDoFs(majorLine);

        if (majorRadiusSet && majorDoFs > 0) {
            constraintMajorRadius();
            handler->diagnoseWithAutoConstraints();
            majorDoFs = handler->getLineDoFs(majorLine);
        }

        if (majorAxisSet && majorDoFs > 0) {
            constraintMajorAngle();
            handler->diagnoseWithAutoConstraints();
        }

        int minorDoFs = handler->getLineDoFs(minorLine);

        if (minorRadiusSet && minorDoFs > 0) {
            constraintMinorRadius();
        }
    }
}

}  // namespace SketcherGui

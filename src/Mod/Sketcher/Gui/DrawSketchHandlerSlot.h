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

#include <sstream>

#include <QApplication>

#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/InputHint.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"
#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp


class DrawSketchHandlerSlot;

using DSHSlotController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerSlot,
    StateMachines::ThreeSeekEnd,
    /*PAutoConstraintSize =*/2,
    /*OnViewParametersT =*/OnViewParameters<5>,   // NOLINT
    /*WidgetParametersT =*/WidgetParameters<0>,   // NOLINT
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0>,   // NOLINT
    /*WidgetComboboxesT =*/WidgetComboboxes<0>>;  // NOLINT

using DSHSlotControllerBase = DSHSlotController::ControllerBase;

using DrawSketchHandlerSlotBase = DrawSketchControllableHandler<DSHSlotController>;

class DrawSketchHandlerSlot: public DrawSketchHandlerSlotBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerSlot)

    friend DSHSlotController;
    friend DSHSlotControllerBase;

public:
    DrawSketchHandlerSlot()
        : radius(1.0)
        , length(0.0)
        , angle(0.0)
        , firstCurve(0)
        , capturedDirection(0.0, 0.0)
    {}

    ~DrawSketchHandlerSlot() override = default;

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
                         {tr("%1 pick slot start point"), {MouseLeft}},
                     }},
                {.state = SelectMode::SeekSecond,
                 .hints =
                     {
                         {tr("%1 pick slot end point"), {MouseLeft}},
                     }},
                {.state = SelectMode::SeekThird,
                 .hints =
                     {
                         {tr("%1 pick slot width"), {MouseLeft}},
                     }},
            });
    }

    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);

                startPoint = onSketchPos;

                seekAndRenderAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            case SelectMode::SeekSecond: {
                toolWidgetManager.drawDirectionAtCursor(onSketchPos, startPoint);

                secondPoint = onSketchPos;
                angle = (secondPoint - startPoint).Angle();
                length = (secondPoint - startPoint).Length();
                const double scale = 0.2;
                radius = length * scale;  // radius chosen at 1/5 of length

                CreateAndDrawShapeGeometry();

                seekAndRenderAutoConstraint(
                    sugConstraints[1],
                    onSketchPos,
                    secondPoint - startPoint,
                    AutoConstraint::VERTEX_NO_TANGENCY
                );
            } break;
            case SelectMode::SeekThird: {
                /*To follow the cursor, r should adapt depending on the position of the cursor. If
                cursor is 'between' the center points, then its distance to that line and not
                distance to the second center. A is "between" B and C if angle ?ABC and angle ?ACB
                are both less than or equal to ninety degrees. An angle ?ABC is greater than ninety
                degrees iff AB^2 + BC^2 < AC^2.*/

                double L1 = (onSketchPos - startPoint).Length();   // distance between first center
                                                                   // and onSketchPos
                double L2 = (onSketchPos - secondPoint).Length();  // distance between second center
                                                                   // and onSketchPos

                if ((L1 * L1 + length * length > L2 * L2) && (L2 * L2 + length * length > L1 * L1)) {
                    // distance of onSketchPos to the line StartPos-SecondPos
                    radius = (abs(
                                 (secondPoint.y - startPoint.y) * onSketchPos.x
                                 - (secondPoint.x - startPoint.x) * onSketchPos.y
                                 + secondPoint.x * startPoint.y - secondPoint.y * startPoint.x
                             ))
                        / length;
                }
                else {
                    radius = std::min(L1, L2);
                }

                toolWidgetManager.drawDoubleAtCursor(onSketchPos, radius);

                CreateAndDrawShapeGeometry();
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        firstCurve = getHighestCurveIndex() + 1;

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add slot"));

            createShape(false);

            commandAddShapeGeometryAndConstraints();

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to add slot")
            );

            Gui::Command::abortCommand();
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
        // alignment constraints needs to apply to the line not the arc.
        bool alignmentCstr = false;
        for (auto& ac : sugConstraints[1]) {
            if (ac.Type == Sketcher::Horizontal || ac.Type == Sketcher::Vertical
                || ac.Type == Sketcher::Perpendicular || ac.Type == Sketcher::Parallel) {
                ac.GeoId = firstCurve + 2;
                alignmentCstr = true;
            }
        }

        if (avoidRedundants && alignmentCstr) {
            removeRedundantHorizontalVertical(getSketchObject(), sugConstraints[0], sugConstraints[1]);
        }

        // add auto constraints for the center of 1st arc
        generateAutoConstraintsOnElement(
            sugConstraints[0],
            getHighestCurveIndex() - 3,
            Sketcher::PointPos::mid
        );

        generateAutoConstraintsOnElement(
            sugConstraints[1],
            getHighestCurveIndex() - 2,
            Sketcher::PointPos::mid
        );

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
    }

    std::string getToolName() const override
    {
        return "DSH_Slot";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Slot");
    }

    std::unique_ptr<QWidget> createWidget() const override
    {
        return std::make_unique<SketcherToolDefaultWidget>();
    }

    bool canGoToNextMode() override
    {
        if (state() == SelectMode::SeekSecond && length < Precision::Confusion()) {
            // Prevent validation of null slot.
            return false;
        }

        if (state() == SelectMode::SeekThird
            && (length < Precision::Confusion() || radius < Precision::Confusion())) {
            // Prevent validation of null slot.
            return false;
        }

        return true;
    }

    void angleSnappingControl() override
    {
        if (state() == SelectMode::SeekSecond) {
            setAngleSnapping(true, startPoint);
        }

        else {
            setAngleSnapping(false);
        }
    }

    void createShape(bool onlyeditoutline) override
    {
        using std::numbers::pi;

        ShapeGeometry.clear();

        if (length < Precision::Confusion() || radius < Precision::Confusion()) {
            return;
        }

        Part::GeomArcOfCircle* arc1 = addArcToShapeGeometry(
            toVector3d(startPoint),
            pi / 2 + angle,
            1.5 * pi + angle,
            radius,
            isConstructionMode()
        );

        Part::GeomArcOfCircle* arc2 = addArcToShapeGeometry(
            toVector3d(secondPoint),
            1.5 * pi + angle,
            pi / 2 + angle,
            radius,
            isConstructionMode()
        );

        Base::Vector3d p11 = arc1->getStartPoint();
        Base::Vector3d p12 = arc1->getEndPoint();
        Base::Vector3d p21 = arc2->getStartPoint();
        Base::Vector3d p22 = arc2->getEndPoint();

        addLineToShapeGeometry(p11, p22, isConstructionMode());

        addLineToShapeGeometry(p12, p21, isConstructionMode());

        if (!onlyeditoutline) {
            addToShapeConstraints(
                Sketcher::Tangent,
                firstCurve,
                Sketcher::PointPos::start,
                firstCurve + 2,
                Sketcher::PointPos::start
            );
            addToShapeConstraints(
                Sketcher::Tangent,
                firstCurve,
                Sketcher::PointPos::end,
                firstCurve + 3,
                Sketcher::PointPos::start
            );
            addToShapeConstraints(
                Sketcher::Tangent,
                firstCurve + 1,
                Sketcher::PointPos::end,
                firstCurve + 2,
                Sketcher::PointPos::end
            );
            addToShapeConstraints(
                Sketcher::Tangent,
                firstCurve + 1,
                Sketcher::PointPos::start,
                firstCurve + 3,
                Sketcher::PointPos::end
            );
            addToShapeConstraints(Sketcher::Equal, firstCurve, Sketcher::PointPos::none, firstCurve + 1);
        }
    }

private:
    Base::Vector2d startPoint, secondPoint;
    double radius, length, angle;
    int firstCurve;

    // Direction tracking to prevent OVP from flipping (issue #23459)
    Base::Vector2d capturedDirection;
};

template<>
auto DSHSlotControllerBase::getState(int labelindex) const
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
        default:
            THROWM(Base::ValueError, "OnViewParameter index without an associated machine state")
    }
}

template<>
void DSHSlotController::configureToolWidget()
{
    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);
    onViewParameters[OnViewParameter::Third]->setLabelType(
        Gui::SoDatumLabel::DISTANCE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
    onViewParameters[OnViewParameter::Fourth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
    onViewParameters[OnViewParameter::Fifth]->setLabelType(
        Gui::SoDatumLabel::RADIUS,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
}

template<>
void DSHSlotControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            Base::Vector2d dir = onSketchPos - handler->startPoint;
            if (dir.Length() < Precision::Confusion()) {
                dir.x = 1.0;  // if direction null, default to (1,0)
            }
            double length = dir.Length();

            if (fourthParam->isSet) {
                const double angle = Base::toRadians(fourthParam->getValue());
                const Base::Vector2d ovpDir(cos(angle), sin(angle));
                handler->capturedDirection = ovpDir;
            }
            else {
                handler->capturedDirection = dir.Normalize();
            }

            if (thirdParam->isSet) {
                length = thirdParam->getValue();
                if (length < Precision::Confusion() && thirdParam->hasFinishedEditing) {
                    unsetOnViewParameter(thirdParam.get());
                    handler->capturedDirection = Base::Vector2d(0.0, 0.0);
                    return;
                }

                onSketchPos = handler->startPoint + length * handler->capturedDirection;
            }
            else if (fourthParam->isSet) {
                onSketchPos.ProjectToLine(onSketchPos - handler->startPoint, handler->capturedDirection);
                onSketchPos += handler->startPoint;
            }
        } break;
        case SelectMode::SeekThird: {
            auto& fifthParam = onViewParameters[OnViewParameter::Fifth];

            if (fifthParam->isSet) {
                double radius = fifthParam->getValue();

                if (radius < Precision::Confusion() && fifthParam->hasFinishedEditing) {
                    unsetOnViewParameter(fifthParam.get());
                    return;
                }

                onSketchPos.x = handler->secondPoint.x + cos(handler->angle) * radius;
                onSketchPos.y = handler->secondPoint.y + sin(handler->angle) * radius;
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHSlotController::adaptParameters(Base::Vector2d onSketchPos)
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
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            Base::Vector3d start = toVector3d(handler->startPoint);
            Base::Vector3d end = toVector3d(handler->secondPoint);
            Base::Vector3d vec = end - start;

            if (!thirdParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Third, vec.Length());
            }

            double range = (handler->secondPoint - handler->startPoint).Angle();
            if (!fourthParam->isSet) {
                setOnViewParameterValue(
                    OnViewParameter::Fourth,
                    Base::toDegrees(range),
                    Base::Unit::Angle
                );
            }
            else if (vec.Length() > Precision::Confusion()) {
                double ovpRange = Base::toRadians(fourthParam->getValue());

                if (fabs(range - ovpRange) > Precision::Confusion()) {
                    setOnViewParameterValue(
                        OnViewParameter::Fourth,
                        Base::toDegrees(range),
                        Base::Unit::Angle
                    );
                }
            }

            thirdParam->setPoints(start, end);
            fourthParam->setPoints(start, Base::Vector3d());
            fourthParam->setLabelRange(range);
        } break;
        case SelectMode::SeekThird: {
            auto& fifthParam = onViewParameters[OnViewParameter::Fifth];

            if (!fifthParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Fifth, handler->radius);
            }

            Base::Vector3d labelSecondPoint = Base::Vector3d();
            labelSecondPoint.x = handler->secondPoint.x + cos(handler->angle) * handler->radius;
            labelSecondPoint.y = handler->secondPoint.y + sin(handler->angle) * handler->radius;

            fifthParam->setPoints(toVector3d(handler->secondPoint), labelSecondPoint);

        } break;
        default:
            break;
    }
}

template<>
void DSHSlotController::computeNextDrawSketchHandlerMode()
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
                handler->setNextState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHSlotController::addConstraints()
{
    App::DocumentObject* obj = handler->sketchgui->getObject();

    int firstCurve = handler->getHighestCurveIndex() - 3;

    auto x0 = onViewParameters[OnViewParameter::First]->getValue();
    auto y0 = onViewParameters[OnViewParameter::Second]->getValue();

    auto x0set = onViewParameters[OnViewParameter::First]->isSet;
    auto y0set = onViewParameters[OnViewParameter::Second]->isSet;
    auto lengthSet = onViewParameters[OnViewParameter::Third]->isSet;
    auto angleSet = onViewParameters[OnViewParameter::Fourth]->isSet;
    auto radiusSet = onViewParameters[OnViewParameter::Fifth]->isSet;

    using namespace Sketcher;

    auto constraintToOrigin = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::RtPnt, x0, obj);
    };

    auto constraintx0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::VAxis, x0, obj);
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::HAxis, y0, obj);
    };

    auto constraintLength = [&]() {
        Gui::cmdAppObjectArgs(
            obj,
            "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
            firstCurve,
            3,
            firstCurve + 1,
            3,
            handler->length
        );
    };

    auto constraintAngle = [&]() {
        ConstraintType lastType = handler->sugConstraints[1].empty()
            ? ConstraintType::None
            : handler->sugConstraints[1].back().Type;
        if (lastType != Sketcher::Horizontal && lastType != Sketcher::Vertical
            && lastType != Sketcher::Perpendicular && lastType != Sketcher::Parallel) {
            Gui::cmdAppObjectArgs(
                obj,
                "addConstraint(Sketcher.Constraint('Angle',%d,%d,%f)) ",
                Sketcher::GeoEnum::HAxis,
                firstCurve + 2,
                handler->angle
            );
        }
    };

    if (handler->AutoConstraints.empty()) {  // No valid diagnosis. Every constraint can be added.

        if (x0set && y0set && x0 == 0. && y0 == 0.) {
            constraintToOrigin();
        }
        else {
            if (x0set) {
                constraintx0();
            }

            if (y0set) {
                constrainty0();
            }
        }

        if (lengthSet) {
            constraintLength();
        }

        if (angleSet) {
            constraintAngle();
        }
    }
    else {  // Valid diagnosis. Must check which constraints may be added.
        auto startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));

        if (x0set && startpointinfo.isXDoF()) {
            constraintx0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition

            startpointinfo = handler->getPointInfo(
                GeoElementId(firstCurve, PointPos::start)
            );  // get updated point position
        }

        if (y0set && startpointinfo.isYDoF()) {
            constrainty0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition

            startpointinfo = handler->getPointInfo(
                GeoElementId(firstCurve, PointPos::start)
            );  // get updated point position
        }

        auto endpointinfo = handler->getPointInfo(GeoElementId(firstCurve + 1, PointPos::mid));

        int DoFs = startpointinfo.getDoFs();
        DoFs += endpointinfo.getDoFs();

        if (lengthSet && DoFs > 0) {
            constraintLength();
            DoFs--;
        }

        if (angleSet && DoFs > 0) {
            constraintAngle();
        }
    }

    // No auto constraint in seekThird.
    if (radiusSet) {
        Gui::cmdAppObjectArgs(
            obj,
            "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
            firstCurve,
            handler->radius
        );
    }
}

}  // namespace SketcherGui

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


#ifndef SKETCHERGUI_DrawSketchHandlerSlot_H
#define SKETCHERGUI_DrawSketchHandlerSlot_H

#include <sstream>

#include <QApplication>

#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

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

using DSHSlotController =
    DrawSketchDefaultWidgetController<DrawSketchHandlerSlot,
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
    friend DSHSlotController;
    friend DSHSlotControllerBase;

public:
    DrawSketchHandlerSlot()
        : radius(1.0)
        , length(0.0)
        , angle(0.0)
        , isHorizontal(false)
        , isVertical(false)
        , firstCurve(0)
    {}

    ~DrawSketchHandlerSlot() override = default;

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);

                startPoint = onSketchPos;

                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            } break;
            case SelectMode::SeekSecond: {
                toolWidgetManager.drawDirectionAtCursor(onSketchPos, startPoint);

                secondPoint = onSketchPos;
                angle = (secondPoint - startPoint).Angle();
                checkHorizontalVertical();
                length = (secondPoint - startPoint).Length();
                const double scale = 0.2;
                radius = length * scale;  // radius chosen at 1/5 of length

                CreateAndDrawShapeGeometry();

                if (seekAutoConstraint(sugConstraints[1],
                                       onSketchPos,
                                       secondPoint - startPoint,
                                       AutoConstraint::VERTEX_NO_TANGENCY)) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            } break;
            case SelectMode::SeekThird: {
                /*To follow the cursor, r should adapt depending on the position of the cursor. If
                cursor is 'between' the center points, then its distance to that line and not
                distance to the second center. A is "between" B and C if angle ?ABC and angle ?ACB
                are both less than or equal to ninety degrees. An angle ?ABC is greater than ninety
                degrees iff AB^2 + BC^2 < AC^2.*/

                double L1 = (onSketchPos - startPoint)
                                .Length();  // distance between first center and onSketchPos
                double L2 = (onSketchPos - secondPoint)
                                .Length();  // distance between second center and onSketchPos

                if ((L1 * L1 + length * length > L2 * L2)
                    && (L2 * L2 + length * length > L1 * L1)) {
                    // distance of onSketchPos to the line StartPos-SecondPos
                    radius = (abs((secondPoint.y - startPoint.y) * onSketchPos.x
                                  - (secondPoint.x - startPoint.x) * onSketchPos.y
                                  + secondPoint.x * startPoint.y - secondPoint.y * startPoint.x))
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
            Gui::NotifyError(sketchgui,
                             QT_TRANSLATE_NOOP("Notifications", "Error"),
                             QT_TRANSLATE_NOOP("Notifications", "Failed to add slot"));

            Gui::Command::abortCommand();
            THROWM(Base::RuntimeError,
                   QT_TRANSLATE_NOOP(
                       "Notifications",
                       "Tool execution aborted") "\n")  // This prevents constraints from being
                                                        // applied on non existing geometry
        }
    }

    void generateAutoConstraints() override
    {
        // add auto constraints for the center of 1st arc
        generateAutoConstraintsOnElement(sugConstraints[0],
                                         getHighestCurveIndex() - 3,
                                         Sketcher::PointPos::mid);

        generateAutoConstraintsOnElement(sugConstraints[1],
                                         getHighestCurveIndex() - 2,
                                         Sketcher::PointPos::mid);

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
        isHorizontal = false;
        isVertical = false;
    }

    std::string getToolName() const override
    {
        return "DSH_Slot";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Slot");
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
        ShapeGeometry.clear();

        if (length < Precision::Confusion() || radius < Precision::Confusion()) {
            return;
        }

        Part::GeomArcOfCircle* arc1 = addArcToShapeGeometry(toVector3d(startPoint),
                                                            M_PI / 2 + angle,
                                                            1.5 * M_PI + angle,
                                                            radius,
                                                            isConstructionMode());

        Part::GeomArcOfCircle* arc2 = addArcToShapeGeometry(toVector3d(secondPoint),
                                                            1.5 * M_PI + angle,
                                                            M_PI / 2 + angle,
                                                            radius,
                                                            isConstructionMode());

        Base::Vector3d p11 = arc1->getStartPoint();
        Base::Vector3d p12 = arc1->getEndPoint();
        Base::Vector3d p21 = arc2->getStartPoint();
        Base::Vector3d p22 = arc2->getEndPoint();

        addLineToShapeGeometry(p11, p22, isConstructionMode());

        addLineToShapeGeometry(p12, p21, isConstructionMode());

        if (!onlyeditoutline) {
            addToShapeConstraints(Sketcher::Tangent,
                                  firstCurve,
                                  Sketcher::PointPos::start,
                                  firstCurve + 2,
                                  Sketcher::PointPos::start);
            addToShapeConstraints(Sketcher::Tangent,
                                  firstCurve,
                                  Sketcher::PointPos::end,
                                  firstCurve + 3,
                                  Sketcher::PointPos::start);
            addToShapeConstraints(Sketcher::Tangent,
                                  firstCurve + 1,
                                  Sketcher::PointPos::end,
                                  firstCurve + 2,
                                  Sketcher::PointPos::end);
            addToShapeConstraints(Sketcher::Tangent,
                                  firstCurve + 1,
                                  Sketcher::PointPos::start,
                                  firstCurve + 3,
                                  Sketcher::PointPos::end);
            addToShapeConstraints(Sketcher::Equal,
                                  firstCurve,
                                  Sketcher::PointPos::none,
                                  firstCurve + 1);

            // Prevent duplicate with Autocontraint
            AutoConstraint lastCons = {Sketcher::None,
                                       Sketcher::GeoEnum::GeoUndef,
                                       Sketcher::PointPos::none};
            if (!sugConstraints[1].empty()) {
                lastCons = sugConstraints[1].back();
            }

            if (isHorizontal || isVertical) {
                addToShapeConstraints(isHorizontal ? Sketcher::Horizontal : Sketcher::Vertical,
                                      firstCurve + 3);


                if (lastCons.Type == Sketcher::Horizontal || lastCons.Type == Sketcher::Vertical) {
                    sugConstraints[1].pop_back();
                }
            }
            else {
                // If horizontal/vertical Autoconstraint suggested, applied it on first line
                // (rather than last arc)
                if (lastCons.Type == Sketcher::Horizontal || lastCons.Type == Sketcher::Vertical) {
                    sugConstraints[1].back().GeoId = firstCurve + 2;
                }
            }
        }
    }

    void checkHorizontalVertical()
    {
        isHorizontal = false;
        isVertical = false;

        if (fmod(fabs(angle), M_PI) < Precision::Confusion()) {
            isHorizontal = true;
        }
        else if (fmod(fabs(angle + M_PI / 2), M_PI) < Precision::Confusion()) {
            isVertical = true;
        }
    }

private:
    Base::Vector2d startPoint, secondPoint;
    double radius, length, angle;
    bool isHorizontal, isVertical;
    int firstCurve;
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
        Gui::EditableDatumLabel::Function::Dimensioning);
    onViewParameters[OnViewParameter::Fourth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning);
    onViewParameters[OnViewParameter::Fifth]->setLabelType(
        Gui::SoDatumLabel::RADIUS,
        Gui::EditableDatumLabel::Function::Dimensioning);
}

template<>
void DSHSlotControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (onViewParameters[OnViewParameter::First]->isSet) {
                onSketchPos.x = onViewParameters[OnViewParameter::First]->getValue();
            }

            if (onViewParameters[OnViewParameter::Second]->isSet) {
                onSketchPos.y = onViewParameters[OnViewParameter::Second]->getValue();
            }
        } break;
        case SelectMode::SeekSecond: {
            Base::Vector2d dir = onSketchPos - handler->startPoint;
            if (dir.Length() < Precision::Confusion()) {
                dir.x = 1.0;  // if direction null, default to (1,0)
            }
            double length = dir.Length();

            if (onViewParameters[OnViewParameter::Third]->isSet) {
                length = onViewParameters[OnViewParameter::Third]->getValue();
                if (length < Precision::Confusion()) {
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Third].get());
                    return;
                }

                onSketchPos = handler->startPoint + length * dir.Normalize();
            }

            if (onViewParameters[OnViewParameter::Fourth]->isSet) {
                double angle =
                    Base::toRadians(onViewParameters[OnViewParameter::Fourth]->getValue());
                onSketchPos.x = handler->startPoint.x + cos(angle) * length;
                onSketchPos.y = handler->startPoint.y + sin(angle) * length;
            }
        } break;
        case SelectMode::SeekThird: {
            if (onViewParameters[OnViewParameter::Fifth]->isSet) {
                double radius = onViewParameters[OnViewParameter::Fifth]->getValue();

                if (radius < Precision::Confusion()) {
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Fifth].get());
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
            if (!onViewParameters[OnViewParameter::First]->isSet) {
                setOnViewParameterValue(OnViewParameter::First, onSketchPos.x);
            }

            if (!onViewParameters[OnViewParameter::Second]->isSet) {
                setOnViewParameterValue(OnViewParameter::Second, onSketchPos.y);
            }

            bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
            onViewParameters[OnViewParameter::First]->setLabelAutoDistanceReverse(!sameSign);
            onViewParameters[OnViewParameter::Second]->setLabelAutoDistanceReverse(sameSign);
            onViewParameters[OnViewParameter::First]->setPoints(Base::Vector3d(),
                                                                toVector3d(onSketchPos));
            onViewParameters[OnViewParameter::Second]->setPoints(Base::Vector3d(),
                                                                 toVector3d(onSketchPos));
        } break;
        case SelectMode::SeekSecond: {
            Base::Vector3d start = toVector3d(handler->startPoint);
            Base::Vector3d end = toVector3d(handler->secondPoint);
            Base::Vector3d vec = end - start;

            if (!onViewParameters[OnViewParameter::Third]->isSet) {
                setOnViewParameterValue(OnViewParameter::Third, vec.Length());
            }

            double range = (handler->secondPoint - handler->startPoint).Angle();
            if (!onViewParameters[OnViewParameter::Fourth]->isSet) {
                setOnViewParameterValue(OnViewParameter::Fourth,
                                        Base::toDegrees(range),
                                        Base::Unit::Angle);
            }

            onViewParameters[OnViewParameter::Third]->setPoints(start, end);
            onViewParameters[OnViewParameter::Fourth]->setPoints(start, Base::Vector3d());
            onViewParameters[OnViewParameter::Fourth]->setLabelRange(range);
        } break;
        case SelectMode::SeekThird: {
            if (!onViewParameters[OnViewParameter::Fifth]->isSet) {
                setOnViewParameterValue(OnViewParameter::Fifth, handler->radius);
            }

            Base::Vector3d labelSecondPoint = Base::Vector3d();
            labelSecondPoint.x = handler->secondPoint.x + cos(handler->angle) * handler->radius;
            labelSecondPoint.y = handler->secondPoint.y + sin(handler->angle) * handler->radius;

            onViewParameters[OnViewParameter::Fifth]->setPoints(toVector3d(handler->secondPoint),
                                                                labelSecondPoint);

        } break;
        default:
            break;
    }
}

template<>
void DSHSlotController::doChangeDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (onViewParameters[OnViewParameter::First]->isSet
                && onViewParameters[OnViewParameter::Second]->isSet) {

                handler->setState(SelectMode::SeekSecond);
            }
        } break;
        case SelectMode::SeekSecond: {
            if (onViewParameters[OnViewParameter::Third]->isSet
                && onViewParameters[OnViewParameter::Fourth]->isSet) {

                handler->setState(SelectMode::SeekThird);
            }
        } break;
        case SelectMode::SeekThird: {
            if (onViewParameters[OnViewParameter::Fifth]->isSet) {

                handler->setState(SelectMode::End);
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
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid),
                               GeoElementId::RtPnt,
                               x0,
                               obj);
    };

    auto constraintx0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid),
                               GeoElementId::VAxis,
                               x0,
                               obj);
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid),
                               GeoElementId::HAxis,
                               y0,
                               obj);
    };

    auto constraintLength = [&]() {
        Gui::cmdAppObjectArgs(obj,
                              "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                              firstCurve,
                              3,
                              firstCurve + 1,
                              3,
                              handler->length);
    };

    auto constraintAngle = [&]() {
        if (!handler->isHorizontal && !handler->isVertical) {
            Gui::cmdAppObjectArgs(obj,
                                  "addConstraint(Sketcher.Constraint('Angle',%d,%d,%f)) ",
                                  Sketcher::GeoEnum::HAxis,
                                  firstCurve + 2,
                                  handler->angle);

            // Prevent duplicate with Autocontraint
            AutoConstraint lastCons = {Sketcher::None,
                                       Sketcher::GeoEnum::GeoUndef,
                                       Sketcher::PointPos::none};
            if (!handler->sugConstraints[1].empty()) {
                lastCons = handler->sugConstraints[1].back();
                if (lastCons.Type == Sketcher::Horizontal || lastCons.Type == Sketcher::Vertical) {
                    handler->AutoConstraints.pop_back();
                }
            }
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
                GeoElementId(firstCurve, PointPos::start));  // get updated point position
        }

        if (y0set && startpointinfo.isYDoF()) {
            constrainty0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition

            startpointinfo = handler->getPointInfo(
                GeoElementId(firstCurve, PointPos::start));  // get updated point position
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
        Gui::cmdAppObjectArgs(obj,
                              "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                              firstCurve,
                              handler->radius);
    }
}

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerSlot_H

// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Ondsel <development@ondsel.com>                    *
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

#include <QApplication>

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/InputHint.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerArcSlot;

namespace ConstructionMethods
{

enum class ArcSlotConstructionMethod
{
    ArcSlot,
    RectangleSlot,
    End  // Must be the last one
};

}  // namespace ConstructionMethods

using DSHArcSlotController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerArcSlot,
    StateMachines::FourSeekEnd,
    /*PAutoConstraintSize =*/3,                     // NOLINT
    /*OnViewParametersT =*/OnViewParameters<6, 6>,  // NOLINT
    /*WidgetParametersT =*/WidgetParameters<0, 0>,  // NOLINT
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,  // NOLINT
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,  // NOLINT
    ConstructionMethods::ArcSlotConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHArcSlotControllerBase = DSHArcSlotController::ControllerBase;

using DrawSketchHandlerArcSlotBase = DrawSketchControllableHandler<DSHArcSlotController>;

class DrawSketchHandlerArcSlot: public DrawSketchHandlerArcSlotBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerArcSlot)

    friend DSHArcSlotController;
    friend DSHArcSlotControllerBase;

public:
    explicit DrawSketchHandlerArcSlot(ConstructionMethod constrMethod = ConstructionMethod::ArcSlot)
        : DrawSketchHandlerArcSlotBase(constrMethod)
        , startAngle(0.)
        , startAngleBackup(0.)
        , endAngle(0.)
        , arcAngle(0.)
        , r(0.)
        , radius(0.)
        , angleReversed(false)
    {}

    ~DrawSketchHandlerArcSlot() override = default;

    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;

        const Gui::InputHint switchModeHint {.message = tr("%1 switch mode"), .sequences = {KeyM}};

        return Gui::lookupHints<SelectMode>(state(),
                                            {
                                                {.state = SelectMode::SeekFirst,
                                                 .hints =
                                                     {
                                                         {tr("%1 pick slot center"), {MouseLeft}},
                                                         switchModeHint,
                                                     }},
                                                {.state = SelectMode::SeekSecond,
                                                 .hints =
                                                     {
                                                         {tr("%1 pick slot radius"), {MouseLeft}},
                                                         switchModeHint,
                                                     }},
                                                {.state = SelectMode::SeekThird,
                                                 .hints =
                                                     {
                                                         {tr("%1 pick slot angle"), {MouseLeft}},
                                                         switchModeHint,
                                                     }},
                                                {.state = SelectMode::SeekFourth,
                                                 .hints =
                                                     {
                                                         {tr("%1 pick slot width"), {MouseLeft}},
                                                         switchModeHint,
                                                     }},
                                            });
    }

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);

                centerPoint = onSketchPos;

                seekAndRenderAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            case SelectMode::SeekSecond: {
                toolWidgetManager.drawDirectionAtCursor(onSketchPos, centerPoint);

                startPoint = onSketchPos;
                startAngle = (startPoint - centerPoint).Angle();
                startAngleBackup = startAngle;
                radius = (startPoint - centerPoint).Length();

                CreateAndDrawShapeGeometry();

                seekAndRenderAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            case SelectMode::SeekThird: {
                endPoint = centerPoint + (onSketchPos - centerPoint).Normalize() * radius;
                if (constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                    const double scale = 10;
                    r = radius / scale;
                }
                else {
                    const double scale = 1.2;
                    r = radius * scale;
                }

                startAngle = startAngleBackup;

                double angle1 = (onSketchPos - centerPoint).Angle() - startAngle;
                double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * std::numbers::pi;
                arcAngle = abs(angle1 - arcAngle) < abs(angle2 - arcAngle) ? angle1 : angle2;

                reverseIfNecessary();

                CreateAndDrawShapeGeometry();

                toolWidgetManager.drawDoubleAtCursor(onSketchPos, arcAngle, Base::Unit::Angle);

                seekAndRenderAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.0, 0.0));
            } break;
            case SelectMode::SeekFourth: {

                if (constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                    r = std::min(radius, fabs(radius - (onSketchPos - centerPoint).Length()));
                }
                else {
                    r = (onSketchPos - centerPoint).Length();
                }

                toolWidgetManager.drawDoubleAtCursor(onSketchPos, r);

                CreateAndDrawShapeGeometry();
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        try {
            createShape(false);

            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc slot"));

            commandAddShapeGeometryAndConstraints();

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to add arc slot")
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
        auto& ac2 = sugConstraints[1];
        auto& ac3 = sugConstraints[2];

        generateAutoConstraintsOnElement(
            sugConstraints[0],
            getHighestCurveIndex() - 3,
            Sketcher::PointPos::mid
        );  // add auto constraints for the center point

        if (constructionMethod() == ConstructionMethod::ArcSlot) {
            generateAutoConstraintsOnElement(ac2, getHighestCurveIndex() - 2, Sketcher::PointPos::mid);
            generateAutoConstraintsOnElement(ac3, getHighestCurveIndex() - 1, Sketcher::PointPos::mid);
        }
        else {
            generateAutoConstraintsOnElement(
                ac2,
                getHighestCurveIndex() - 3,
                (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end
            );
            generateAutoConstraintsOnElement(
                ac3,
                getHighestCurveIndex() - 3,
                (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start
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
    }

    std::string getToolName() const override
    {
        return "DSH_ArcSlot";
    }

    QString getCrosshairCursorSVGName() const override
    {
        if (constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
            return QStringLiteral("Sketcher_Pointer_Create_ArcSlot");
        }
        else {
            return QStringLiteral("Sketcher_Pointer_Create_RectangleSlot");
        }
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
        return Gui::BitmapFactory().pixmap("Sketcher_CreateArcSlot");
    }

    QString getToolWidgetText() const override
    {
        return QString(tr("Arc Slot Parameters"));
    }

    bool canGoToNextMode() override
    {
        // Prevent validation of null arc.
        if (state() == SelectMode::SeekSecond && radius < Precision::Confusion()) {
            return false;
        }
        if (state() == SelectMode::SeekThird && fabs(arcAngle) < Precision::Confusion()) {
            return false;
        }
        if (state() == SelectMode::SeekFourth) {
            if (constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                if (r < Precision::Confusion()) {
                    return false;
                }
            }
            else {
                if (fabs(radius - r) < Precision::Confusion()) {
                    return false;
                }
            }
        }
        return true;
    }

    void angleSnappingControl() override
    {
        if (state() == SelectMode::SeekSecond || state() == SelectMode::SeekThird) {
            setAngleSnapping(true, centerPoint);
        }
        else {
            setAngleSnapping(false);
        }
    }

    void createShape(bool onlyeditoutline) override
    {
        using std::numbers::pi;

        ShapeGeometry.clear();

        if (radius < Precision::Confusion()) {
            return;
        }

        if (state() == SelectMode::SeekSecond) {
            addCircleToShapeGeometry(toVector3d(centerPoint), radius, isConstructionMode());
        }
        else {
            if (fabs(arcAngle) < Precision::Confusion()) {
                return;
            }

            if (state() == SelectMode::SeekFourth && r < Precision::Confusion()) {
                return;
            }

            if (constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                addArcToShapeGeometry(
                    toVector3d(centerPoint),
                    startAngle,
                    endAngle,
                    radius + r,
                    isConstructionMode()
                );

                addArcToShapeGeometry(
                    toVector3d(startPoint),
                    angleReversed ? endAngle : startAngle + pi,
                    angleReversed ? endAngle + pi : startAngle + 2 * pi,
                    r,
                    isConstructionMode()
                );

                addArcToShapeGeometry(
                    toVector3d(endPoint),
                    angleReversed ? startAngle + pi : endAngle,
                    angleReversed ? startAngle + 2 * pi : pi + endAngle,
                    r,
                    isConstructionMode()
                );

                if (radius - r > Precision::Confusion()) {
                    addArcToShapeGeometry(
                        toVector3d(centerPoint),
                        startAngle,
                        endAngle,
                        radius - r,
                        isConstructionMode()
                    );
                }
            }
            else {
                Part::GeomArcOfCircle* arc1 = addArcToShapeGeometry(
                    toVector3d(centerPoint),
                    startAngle,
                    endAngle,
                    radius,
                    isConstructionMode()
                );

                Base::Vector3d p11 = arc1->getStartPoint();
                Base::Vector3d p12 = arc1->getEndPoint();

                if (r > Precision::Confusion()) {
                    auto arc2 = std::make_unique<Part::GeomArcOfCircle>();
                    arc2->setRadius(r);
                    arc2->setRange(startAngle, endAngle, true);
                    arc2->setCenter(toVector3d(centerPoint));
                    Sketcher::GeometryFacade::setConstruction(arc2.get(), isConstructionMode());

                    Base::Vector3d p21 = arc2->getStartPoint();
                    Base::Vector3d p22 = arc2->getEndPoint();

                    addLineToShapeGeometry(p11, p21, isConstructionMode());

                    addLineToShapeGeometry(p12, p22, isConstructionMode());

                    // arc2 is added last to make it easy if it does not exist
                    ShapeGeometry.push_back(std::move(arc2));
                }
                else {
                    addLineToShapeGeometry(p11, toVector3d(centerPoint), isConstructionMode());

                    addLineToShapeGeometry(p12, toVector3d(centerPoint), isConstructionMode());
                }
            }
        }

        if (!onlyeditoutline) {
            int firstCurve = getHighestCurveIndex() + 1;

            if (constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                bool allArcs = fabs(radius - r) > Precision::Confusion();

                Sketcher::PointPos pos1 = angleReversed ? Sketcher::PointPos::start
                                                        : Sketcher::PointPos::end;
                Sketcher::PointPos pos2 = angleReversed ? Sketcher::PointPos::end
                                                        : Sketcher::PointPos::start;

                if (allArcs) {
                    addToShapeConstraints(
                        Sketcher::Coincident,
                        firstCurve,
                        Sketcher::PointPos::mid,
                        firstCurve + 3,
                        Sketcher::PointPos::mid
                    );

                    addToShapeConstraints(Sketcher::Tangent, firstCurve + 3, pos1, firstCurve + 2, pos1);

                    addToShapeConstraints(Sketcher::Tangent, firstCurve + 3, pos2, firstCurve + 1, pos2);
                }
                else {
                    addToShapeConstraints(
                        Sketcher::Coincident,
                        firstCurve,
                        Sketcher::PointPos::mid,
                        firstCurve + 1,
                        pos2
                    );

                    addToShapeConstraints(
                        Sketcher::Coincident,
                        firstCurve,
                        Sketcher::PointPos::mid,
                        firstCurve + 2,
                        pos1
                    );
                }

                addToShapeConstraints(Sketcher::Tangent, firstCurve, pos1, firstCurve + 2, pos2);

                addToShapeConstraints(Sketcher::Tangent, firstCurve, pos2, firstCurve + 1, pos1);
            }
            else {
                bool allGeos = r > Precision::Confusion();

                addToShapeConstraints(
                    Sketcher::Perpendicular,
                    firstCurve,
                    Sketcher::PointPos::none,
                    firstCurve + 1,
                    Sketcher::PointPos::none
                );

                addToShapeConstraints(
                    Sketcher::Perpendicular,
                    firstCurve,
                    Sketcher::PointPos::none,
                    firstCurve + 2,
                    Sketcher::PointPos::none
                );


                addToShapeConstraints(
                    Sketcher::Coincident,
                    firstCurve,
                    Sketcher::PointPos::start,
                    firstCurve + 1,
                    Sketcher::PointPos::start
                );

                addToShapeConstraints(
                    Sketcher::Coincident,
                    firstCurve,
                    Sketcher::PointPos::end,
                    firstCurve + 2,
                    Sketcher::PointPos::start
                );

                if (allGeos) {
                    addToShapeConstraints(
                        Sketcher::Coincident,
                        firstCurve,
                        Sketcher::PointPos::mid,
                        firstCurve + 3,
                        Sketcher::PointPos::mid
                    );

                    addToShapeConstraints(
                        Sketcher::Coincident,
                        firstCurve + 3,
                        Sketcher::PointPos::start,
                        firstCurve + 1,
                        Sketcher::PointPos::end
                    );

                    addToShapeConstraints(
                        Sketcher::Coincident,
                        firstCurve + 3,
                        Sketcher::PointPos::end,
                        firstCurve + 2,
                        Sketcher::PointPos::end
                    );
                }
                else {
                    addToShapeConstraints(
                        Sketcher::Coincident,
                        firstCurve,
                        Sketcher::PointPos::mid,
                        firstCurve + 1,
                        Sketcher::PointPos::end
                    );

                    addToShapeConstraints(
                        Sketcher::Coincident,
                        firstCurve,
                        Sketcher::PointPos::mid,
                        firstCurve + 2,
                        Sketcher::PointPos::end
                    );
                }
            }
        }
    }

    void reverseIfNecessary()
    {
        if (arcAngle > 0) {
            endAngle = startAngle + arcAngle;
            angleReversed = false;
        }
        else {
            endAngle = startAngle;
            startAngle = startAngle + arcAngle;
            angleReversed = true;
        }
    }

private:
    Base::Vector2d centerPoint, startPoint, endPoint;
    double startAngle, startAngleBackup, endAngle, arcAngle, r, radius;
    bool angleReversed;
};

template<>
auto DSHArcSlotControllerBase::getState(int labelindex) const
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
void DSHArcSlotController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        QStringList names = {
            QApplication::translate("Sketcher_CreateArcSlot", "Arc ends"),
            QApplication::translate("Sketcher_CreateArcSlot", "Flat ends")
        };
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        if (isConstructionMode()) {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArcSlot_Constr")
            );
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangleSlot_Constr")
            );
        }
        else {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArcSlot")
            );
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangleSlot")
            );
        }
    }

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

    if (handler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
        onViewParameters[OnViewParameter::Sixth]->setLabelType(
            Gui::SoDatumLabel::RADIUS,
            Gui::EditableDatumLabel::Function::Dimensioning
        );
    }
    else {
        onViewParameters[OnViewParameter::Sixth]->setLabelType(
            Gui::SoDatumLabel::DISTANCE,
            Gui::EditableDatumLabel::Function::Dimensioning
        );
    }
}

template<>
void DSHArcSlotControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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

            auto dir = onSketchPos - handler->centerPoint;
            if (dir.Length() < Precision::Confusion()) {
                dir.x = 1.0;  // if direction null, default to (1,0)
            }
            double radius = dir.Length();

            if (thirdParam->isSet) {
                radius = thirdParam->getValue();
                if (radius < Precision::Confusion() && thirdParam->hasFinishedEditing) {
                    unsetOnViewParameter(thirdParam.get());
                    return;
                }

                onSketchPos = handler->centerPoint + radius * dir.Normalize();
            }

            if (fourthParam->isSet) {
                double angle = Base::toRadians(fourthParam->getValue());
                onSketchPos.x = handler->centerPoint.x + cos(angle) * radius;
                onSketchPos.y = handler->centerPoint.y + sin(angle) * radius;
            }
        } break;
        case SelectMode::SeekThird: {
            auto& fifthParam = onViewParameters[OnViewParameter::Fifth];

            if (fifthParam->isSet) {
                double arcAngle = Base::toRadians(fifthParam->getValue());
                if (fmod(fabs(arcAngle), 2 * std::numbers::pi) < Precision::Confusion()
                    && fifthParam->hasFinishedEditing) {
                    unsetOnViewParameter(fifthParam.get());
                }
                else {
                    handler->arcAngle = arcAngle;

                    double length = (onSketchPos - handler->centerPoint).Length();
                    double angle = handler->startAngleBackup + arcAngle;
                    onSketchPos.x = handler->centerPoint.x + cos(angle) * length;
                    onSketchPos.y = handler->centerPoint.y + sin(angle) * length;
                }
            }
        } break;
        case SelectMode::SeekFourth: {
            auto& sixthParam = onViewParameters[OnViewParameter::Sixth];

            if (sixthParam->isSet) {
                double radius2 = sixthParam->getValue();
                if (((fabs(radius2) < Precision::Confusion()
                      && handler->constructionMethod()
                          == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot)
                     || (fabs(handler->radius - radius2) < Precision::Confusion()
                         && handler->constructionMethod()
                             == DrawSketchHandlerArcSlot::ConstructionMethod::RectangleSlot))
                    && sixthParam->hasFinishedEditing) {
                    unsetOnViewParameter(sixthParam.get());
                }
                else {
                    onSketchPos = handler->centerPoint
                        + Base::Vector2d(handler->radius + radius2, 0.);
                }
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHArcSlotController::adaptParameters(Base::Vector2d onSketchPos)
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

            if (!thirdParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Third, handler->radius);
            }
            double range = Base::toDegrees(handler->startAngle);
            if (!fourthParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Fourth, range, Base::Unit::Angle);
            }

            Base::Vector3d start = toVector3d(handler->centerPoint);
            Base::Vector3d end = toVector3d(onSketchPos);

            thirdParam->setPoints(start, end);
            fourthParam->setPoints(start, Base::Vector3d());
            fourthParam->setLabelRange(handler->startAngle);
        } break;
        case SelectMode::SeekThird: {
            auto& fifthParam = onViewParameters[OnViewParameter::Fifth];

            double range = Base::toDegrees(handler->arcAngle);

            if (!fifthParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Fifth, range, Base::Unit::Angle);
            }

            Base::Vector3d start = toVector3d(handler->centerPoint);
            fifthParam->setPoints(start, Base::Vector3d());

            fifthParam->setLabelStartAngle(handler->startAngleBackup);
            fifthParam->setLabelRange(handler->arcAngle);
        } break;
        case SelectMode::SeekFourth: {
            auto& sixthParam = onViewParameters[OnViewParameter::Sixth];

            double dist = handler->r;
            if (handler->constructionMethod()
                == DrawSketchHandlerArcSlot::ConstructionMethod::RectangleSlot) {
                dist = (handler->r - handler->radius);
            }

            if (!sixthParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Sixth, dist);
            }

            Base::Vector3d start = toVector3d(handler->endPoint);
            Base::Vector3d end = start
                + (start - toVector3d(handler->centerPoint)).Normalize() * dist;

            sixthParam->setPoints(start, end);
        } break;
        default:
            break;
    }
}

template<>
void DSHArcSlotController::computeNextDrawSketchHandlerMode()
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
            auto& sixthParam = onViewParameters[OnViewParameter::Sixth];

            if (sixthParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHArcSlotController::addConstraints()
{
    App::DocumentObject* obj = handler->sketchgui->getObject();

    int firstCurve = handler->getHighestCurveIndex() - 3;
    using namespace Sketcher;

    auto x0 = onViewParameters[OnViewParameter::First]->getValue();
    auto y0 = onViewParameters[OnViewParameter::Second]->getValue();
    auto radius = onViewParameters[OnViewParameter::Third]->getValue();
    auto slotRadius = onViewParameters[OnViewParameter::Sixth]->getValue();

    auto x0set = onViewParameters[OnViewParameter::First]->isSet;
    auto y0set = onViewParameters[OnViewParameter::Second]->isSet;
    auto radiusSet = onViewParameters[OnViewParameter::Third]->isSet;
    auto arcAngleSet = onViewParameters[OnViewParameter::Fifth]->isSet;
    auto slotRadiusSet = onViewParameters[OnViewParameter::Sixth]->isSet;

    auto constraintx0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::VAxis, x0, obj);
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::HAxis, y0, obj);
    };

    auto constraintRadius = [&]() {
        if (handler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
            Gui::cmdAppObjectArgs(
                obj,
                "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                firstCurve,
                3,
                firstCurve + 2,
                3,
                fabs(radius)
            );
        }
        else {
            Gui::cmdAppObjectArgs(
                obj,
                "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                firstCurve,
                fabs(radius)
            );
        }
    };

    auto constraintArcAngle = [&]() {
        Gui::cmdAppObjectArgs(
            obj,
            "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
            firstCurve,
            fabs(handler->arcAngle)
        );
    };

    auto constraintSlotRadius = [&]() {
        if (handler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
            Gui::cmdAppObjectArgs(
                obj,
                "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                firstCurve + 2,
                fabs(slotRadius)
            );
        }
        else {
            Gui::cmdAppObjectArgs(
                obj,
                "addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
                firstCurve + 2,
                fabs(slotRadius)
            );
        }
    };

    if (handler->AutoConstraints.empty()) {  // No valid diagnosis. Every constraint can be added.
        if (x0set && y0set && x0 == 0. && y0 == 0.) {
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

        if (radiusSet) {
            constraintRadius();
        }

        if (arcAngleSet) {
            constraintArcAngle();
        }

        if (slotRadiusSet) {
            constraintSlotRadius();
        }
    }
    else {  // Valid diagnosis. Must check which constraints may be added.
        auto startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));

        if (x0set && startpointinfo.isXDoF()) {
            constraintx0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition
            // get updated point position
            startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));
        }

        if (y0set && startpointinfo.isYDoF()) {
            constrainty0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition
            // get updated point position
            startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));
        }


        startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::start));
        auto endpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::end));
        auto midpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));

        int DoFs = startpointinfo.getDoFs();
        DoFs += endpointinfo.getDoFs();
        DoFs += midpointinfo.getDoFs();

        if (radiusSet && DoFs > 0) {
            constraintRadius();
            DoFs--;
        }

        if (arcAngleSet && DoFs > 0) {
            constraintArcAngle();
        }

        startpointinfo = handler->getPointInfo(GeoElementId(firstCurve + 2, PointPos::start));
        endpointinfo = handler->getPointInfo(GeoElementId(firstCurve + 2, PointPos::end));

        DoFs = startpointinfo.getDoFs();
        DoFs += endpointinfo.getDoFs();

        if (handler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
            midpointinfo = handler->getPointInfo(GeoElementId(firstCurve + 2, PointPos::mid));
            DoFs += midpointinfo.getDoFs();
        }

        if (slotRadiusSet && DoFs > 0) {
            constraintSlotRadius();
        }
    }
}

}  // namespace SketcherGui

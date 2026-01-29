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

#include <Mod/Part/App/Geometry2d.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"
#include "GeometryCreationMode.h"
#include "Utils.h"

#include "CircleEllipseConstructionMethod.h"

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerCircle;

using DSHCircleController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerCircle,
    /*SelectModeT*/ StateMachines::ThreeSeekEnd,
    /*PAutoConstraintSize =*/3,
    /*OnViewParametersT =*/OnViewParameters<3, 6>,  // NOLINT
    /*WidgetParametersT =*/WidgetParameters<0, 0>,  // NOLINT
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,  // NOLINT
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,  // NOLINT
    ConstructionMethods::CircleEllipseConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHCircleControllerBase = DSHCircleController::ControllerBase;

using DrawSketchHandlerCircleBase = DrawSketchControllableHandler<DSHCircleController>;


class DrawSketchHandlerCircle: public DrawSketchHandlerCircleBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerCircle)

    friend DSHCircleController;
    friend DSHCircleControllerBase;

public:
    explicit DrawSketchHandlerCircle(ConstructionMethod constrMethod = ConstructionMethod::Center)
        : DrawSketchHandlerCircleBase(constrMethod)
        , radius(0.0)
        , isDiameter(true)
    {}
    ~DrawSketchHandlerCircle() override = default;

private:
    std::list<Gui::InputHint> getToolHints() const override
    {
        using State = std::pair<ConstructionMethod, SelectMode>;
        using enum Gui::InputHint::UserInput;

        const Gui::InputHint switchModeHint = {tr("%1 switch mode"), {KeyM}};

        return Gui::lookupHints<State>(
            {constructionMethod(), state()},
            {
                // Center method
                {.state = {ConstructionMethod::Center, SelectMode::SeekFirst},
                 .hints =
                     {
                         {tr("%1 pick circle center"), {MouseLeft}},
                         switchModeHint,
                     }},
                {.state = {ConstructionMethod::Center, SelectMode::SeekSecond},
                 .hints =
                     {
                         {tr("%1 pick rim point"), {MouseLeft}},
                         switchModeHint,
                     }},

                // ThreeRim method
                {.state = {ConstructionMethod::ThreeRim, SelectMode::SeekFirst},
                 .hints =
                     {
                         {tr("%1 pick first rim point"), {MouseLeft}},
                         switchModeHint,
                     }},
                {.state = {ConstructionMethod::ThreeRim, SelectMode::SeekSecond},
                 .hints =
                     {
                         {tr("%1 pick second rim point"), {MouseLeft}},
                         switchModeHint,
                     }},
                {.state = {ConstructionMethod::ThreeRim, SelectMode::SeekThird},
                 .hints =
                     {
                         {tr("%1 pick third rim point"), {MouseLeft}},
                         switchModeHint,
                     }},
            });
    }

    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);
                if (constructionMethod() == ConstructionMethod::Center) {
                    centerPoint = onSketchPos;

                    seekAndRenderAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d());
                }
                else {
                    firstPoint = onSketchPos;

                    seekAndRenderAutoConstraint(
                        sugConstraints[0],
                        onSketchPos,
                        Base::Vector2d(),
                        AutoConstraint::CURVE
                    );
                }
            } break;
            case SelectMode::SeekSecond: {
                if (constructionMethod() == ConstructionMethod::ThreeRim) {
                    centerPoint = (onSketchPos - firstPoint) / 2 + firstPoint;
                }
                secondPoint = onSketchPos;

                radius = (onSketchPos - centerPoint).Length();

                CreateAndDrawShapeGeometry();

                if (constructionMethod() == ConstructionMethod::Center) {
                    toolWidgetManager.drawDoubleAtCursor(onSketchPos, radius);
                }
                else {
                    toolWidgetManager.drawPositionAtCursor(onSketchPos);
                }

                seekAndRenderAutoConstraint(
                    sugConstraints[1],
                    onSketchPos,
                    constructionMethod() == ConstructionMethod::Center ? onSketchPos - centerPoint
                                                                       : Base::Vector2d(),
                    AutoConstraint::CURVE
                );
            } break;
            case SelectMode::SeekThird: {
                try {
                    if (areCollinear(firstPoint, secondPoint, onSketchPos)) {
                        // If points are collinear then we can't calculate the center.
                        return;
                    }

                    centerPoint
                        = Part::Geom2dCircle::getCircleCenter(firstPoint, secondPoint, onSketchPos);

                    radius = (onSketchPos - centerPoint).Length();

                    toolWidgetManager.drawPositionAtCursor(onSketchPos);

                    CreateAndDrawShapeGeometry();

                    seekAndRenderAutoConstraint(
                        sugConstraints[2],
                        onSketchPos,
                        Base::Vector2d(0.f, 0.f),
                        AutoConstraint::CURVE
                    );
                }
                catch (Base::ValueError& e) {
                    e.reportException();
                }
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        try {
            createShape(false);

            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch circle"));

            commandAddShapeGeometryAndConstraints();

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to add circle")
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
        int CircleGeoId = getHighestCurveIndex();

        if (constructionMethod() == ConstructionMethod::Center) {
            auto& ac1 = sugConstraints[0];
            auto& ac2 = sugConstraints[1];

            generateAutoConstraintsOnElement(
                ac1,
                CircleGeoId,
                Sketcher::PointPos::mid
            );  // add auto constraints for the center point
            generateAutoConstraintsOnElement(
                ac2,
                CircleGeoId,
                Sketcher::PointPos::none
            );  // add auto constraints for the edge
        }
        else {

            auto& ac1 = sugConstraints[0];
            auto& ac2 = sugConstraints[1];
            auto& ac3 = sugConstraints[2];

            generateAutoConstraintsOnElement(
                ac1,
                CircleGeoId,
                Sketcher::PointPos::none
            );  // add auto constraints for the first point
            generateAutoConstraintsOnElement(
                ac2,
                CircleGeoId,
                Sketcher::PointPos::none
            );  // add auto constraints for the second point
            generateAutoConstraintsOnElement(
                ac3,
                CircleGeoId,
                Sketcher::PointPos::none
            );  // add auto constraints for the second point
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
        return "DSH_Circle";
    }

    QString getCrosshairCursorSVGName() const override
    {
        if (constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::Center) {
            return QStringLiteral("Sketcher_Pointer_Create_Circle");
        }
        else {
            return QStringLiteral("Sketcher_Pointer_Create_3PointCircle");
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
        return Gui::BitmapFactory().pixmap("Sketcher_CreateCircle");
    }

    QString getToolWidgetText() const override
    {
        return QString(tr("Circle Parameters"));
    }

    bool canGoToNextMode() override
    {
        if (state() == SelectMode::SeekSecond && radius < Precision::Confusion()) {
            // Prevent validation of null circle.
            return false;
        }

        return true;
    }

    // reimplement because circle is 2 steps while 3rims is 3 steps
    void onButtonPressed(Base::Vector2d onSketchPos) override
    {
        this->updateDataAndDrawToPosition(onSketchPos);
        if (canGoToNextMode()) {
            if (state() == SelectMode::SeekSecond
                && constructionMethod() == ConstructionMethod::Center) {
                setState(SelectMode::End);
            }
            else {
                moveToNextMode();
            }
        }
    }

    void createShape(bool onlyeditoutline) override
    {
        Q_UNUSED(onlyeditoutline);

        ShapeGeometry.clear();

        if (radius < Precision::Confusion()) {
            return;
        }

        addCircleToShapeGeometry(toVector3d(centerPoint), radius, isConstructionMode());
    }

private:
    Base::Vector2d centerPoint, firstPoint, secondPoint;
    double radius;
    bool isDiameter;
};

template<>
auto DSHCircleControllerBase::getState(int labelindex) const
{

    if (handler->constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::Center) {
        switch (labelindex) {
            case OnViewParameter::First:
            case OnViewParameter::Second:
                return SelectMode::SeekFirst;
                break;
            case OnViewParameter::Third:
                return SelectMode::SeekSecond;
                break;
            default:
                THROWM(Base::ValueError, "OnViewParameter index without an associated machine state")
        }
    }
    else {  // ConstructionMethod::ThreeRim
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
            default:
                THROWM(Base::ValueError, "Label index without an associated machine state")
        }
    }
}

template<>
void DSHCircleController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        QStringList names = {
            QApplication::translate("Sketcher_CreateCircle", "Center"),
            QApplication::translate("Sketcher_CreateCircle", "3 rim points")
        };
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        if (isConstructionMode()) {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle_Constr")
            );
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointCircle_Constr")
            );
        }
        else {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle")
            );
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointCircle")
            );
        }


        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/dimensioning"
        );
        bool dimensioningDiameter = hGrp->GetBool("DimensioningDiameter", true);
        bool dimensioningRadius = hGrp->GetBool("DimensioningRadius", true);

        if (dimensioningRadius && !dimensioningDiameter) {
            handler->isDiameter = false;
        }
    }

    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);

    if (handler->constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::ThreeRim) {
        onViewParameters[OnViewParameter::Third]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
        onViewParameters[OnViewParameter::Fourth]->setLabelType(Gui::SoDatumLabel::DISTANCEY);

        onViewParameters[OnViewParameter::Fifth]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
        onViewParameters[OnViewParameter::Sixth]->setLabelType(Gui::SoDatumLabel::DISTANCEY);
    }
    else {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/dimensioning"
        );
        bool dimensioningDiameter = hGrp->GetBool("DimensioningDiameter", true);
        bool dimensioningRadius = hGrp->GetBool("DimensioningRadius", true);

        if (dimensioningRadius && !dimensioningDiameter) {
            onViewParameters[OnViewParameter::Third]->setLabelType(
                Gui::SoDatumLabel::RADIUS,
                Gui::EditableDatumLabel::Function::Dimensioning
            );
        }
        else {
            onViewParameters[OnViewParameter::Third]->setLabelType(
                Gui::SoDatumLabel::DIAMETER,
                Gui::EditableDatumLabel::Function::Dimensioning
            );
        }
    }
}

template<>
void DSHCircleControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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

            if (handler->constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::Center) {
                if (thirdParam->isSet) {
                    double radius = (handler->isDiameter ? 0.5 : 1) * thirdParam->getValue();
                    if (radius < Precision::Confusion() && thirdParam->hasFinishedEditing) {
                        unsetOnViewParameter(thirdParam.get());
                        return;
                    }

                    auto dir = (onSketchPos - handler->centerPoint);

                    if (dir.Length() < Precision::Confusion()) {
                        dir.x = 1.0;  // if direction null, default to (1,0)
                    }

                    onSketchPos = handler->centerPoint + radius * dir.Normalize();
                }
            }
            else {
                auto& fourthParam = onViewParameters[OnViewParameter::Fourth];
                if (thirdParam->isSet) {
                    onSketchPos.x = thirdParam->getValue();
                }

                if (fourthParam->isSet) {
                    onSketchPos.y = fourthParam->getValue();
                }

                if (thirdParam->hasFinishedEditing && fourthParam->hasFinishedEditing
                    && (onSketchPos - handler->firstPoint).Length() < Precision::Confusion()) {
                    unsetOnViewParameter(thirdParam.get());
                    unsetOnViewParameter(fourthParam.get());
                }
            }
        } break;
        case SelectMode::SeekThird: {  // 3 rims only
            auto& fifthParam = onViewParameters[OnViewParameter::Fifth];
            auto& sixthParam = onViewParameters[OnViewParameter::Sixth];

            if (fifthParam->isSet) {
                onSketchPos.x = fifthParam->getValue();
            }

            if (sixthParam->isSet) {
                onSketchPos.y = sixthParam->getValue();
            }
            if (fifthParam->hasFinishedEditing && sixthParam->hasFinishedEditing
                && areCollinear(handler->firstPoint, handler->secondPoint, onSketchPos)) {
                unsetOnViewParameter(fifthParam.get());
                unsetOnViewParameter(sixthParam.get());
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHCircleController::adaptParameters(Base::Vector2d onSketchPos)
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

            if (handler->constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::Center) {
                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/Mod/Sketcher/dimensioning"
                );
                bool dimDiameter = hGrp->GetBool("DimensioningDiameter", true);
                bool dimRadius = hGrp->GetBool("DimensioningRadius", true);
                bool useRadius = dimRadius && !dimDiameter;

                if (!thirdParam->isSet) {
                    double val = handler->radius * (useRadius ? 1 : 2);
                    setOnViewParameterValue(OnViewParameter::Third, val);
                }

                Base::Vector3d start = toVector3d(handler->centerPoint);
                Base::Vector3d end = toVector3d(onSketchPos);
                if (!useRadius) {
                    start = toVector3d(handler->centerPoint - (onSketchPos - handler->centerPoint));
                }

                thirdParam->setPoints(start, end);
            }
            else {
                auto& fourthParam = onViewParameters[OnViewParameter::Fourth];
                if (!thirdParam->isSet) {
                    setOnViewParameterValue(OnViewParameter::Third, onSketchPos.x);
                }

                if (!fourthParam->isSet) {
                    setOnViewParameterValue(OnViewParameter::Fourth, onSketchPos.y);
                }

                bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
                thirdParam->setLabelAutoDistanceReverse(!sameSign);
                fourthParam->setLabelAutoDistanceReverse(sameSign);
                thirdParam->setPoints(Base::Vector3d(), toVector3d(onSketchPos));
                fourthParam->setPoints(Base::Vector3d(), toVector3d(onSketchPos));
            }
        } break;
        case SelectMode::SeekThird: {  // 3 rims only
            auto& fifthParam = onViewParameters[OnViewParameter::Fifth];
            auto& sixthParam = onViewParameters[OnViewParameter::Sixth];

            if (!fifthParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Fifth, onSketchPos.x);
            }

            if (!sixthParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Sixth, onSketchPos.y);
            }

            bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
            fifthParam->setLabelAutoDistanceReverse(!sameSign);
            sixthParam->setLabelAutoDistanceReverse(sameSign);
            fifthParam->setPoints(Base::Vector3d(), toVector3d(onSketchPos));
            sixthParam->setPoints(Base::Vector3d(), toVector3d(onSketchPos));
        } break;
        default:
            break;
    }
}

template<>
void DSHCircleController::computeNextDrawSketchHandlerMode()
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

            if (thirdParam->hasFinishedEditing
                && handler->constructionMethod()
                    == DrawSketchHandlerCircle::ConstructionMethod::Center) {

                handler->setNextState(SelectMode::End);
            }
            else if (onViewParameters.size() > 3) {
                auto& fourthParam = onViewParameters[OnViewParameter::Fourth];
                if ((thirdParam->hasFinishedEditing || fourthParam->hasFinishedEditing)
                    && handler->constructionMethod()
                        == DrawSketchHandlerCircle::ConstructionMethod::ThreeRim) {

                    handler->setNextState(SelectMode::SeekThird);
                }
            }
        } break;
        case SelectMode::SeekThird: {
            auto& fifthParam = onViewParameters[OnViewParameter::Fifth];
            auto& sixthParam = onViewParameters[OnViewParameter::Sixth];

            if (fifthParam->hasFinishedEditing && sixthParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHCircleController::doConstructionMethodChanged()
{
    // Just update hints - combobox already handled by framework
    handler->updateHint();
}

template<>
void DSHCircleController::addConstraints()
{
    if (handler->constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::Center) {
        int firstCurve = handler->getHighestCurveIndex();

        auto x0 = onViewParameters[OnViewParameter::First]->getValue();
        auto y0 = onViewParameters[OnViewParameter::Second]->getValue();

        auto x0set = onViewParameters[OnViewParameter::First]->isSet;
        auto y0set = onViewParameters[OnViewParameter::Second]->isSet;
        auto radiusSet = onViewParameters[OnViewParameter::Third]->isSet;

        using namespace Sketcher;

        auto constraintx0 = [&]() {
            ConstraintToAttachment(
                GeoElementId(firstCurve, PointPos::mid),
                GeoElementId::VAxis,
                x0,
                handler->sketchgui->getObject()
            );
        };

        auto constrainty0 = [&]() {
            ConstraintToAttachment(
                GeoElementId(firstCurve, PointPos::mid),
                GeoElementId::HAxis,
                y0,
                handler->sketchgui->getObject()
            );
        };

        auto constraintradius = [&]() {
            if (handler->isDiameter) {
                Gui::cmdAppObjectArgs(
                    handler->sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Diameter',%d,%f)) ",
                    firstCurve,
                    handler->radius * 2
                );
            }
            else {
                Gui::cmdAppObjectArgs(
                    handler->sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                    firstCurve,
                    handler->radius
                );
            }

            const std::vector<Sketcher::Constraint*>& ConStr
                = handler->sketchgui->getSketchObject()->Constraints.getValues();
            int index = static_cast<int>(ConStr.size()) - 1;
            handler->moveConstraint(index, prevCursorPosition);
        };

        // NOTE: if AutoConstraints is empty, we can add constraints directly without any diagnose.
        // No diagnose was run.
        if (handler->AutoConstraints.empty()) {
            if (x0set) {
                constraintx0();
            }

            if (y0set) {
                constrainty0();
            }

            if (radiusSet) {
                constraintradius();
            }
        }
        else {  // There is a valid diagnose.
            auto startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));

            // if Autoconstraints is empty we do not have a diagnosed system and the parameter will
            // always be set
            if (x0set && startpointinfo.isXDoF()) {
                constraintx0();

                handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters
                                                         // after each constraint addition

                startpointinfo = handler->getPointInfo(
                    GeoElementId(firstCurve, PointPos::mid)
                );  // get updated point position
            }

            // if Autoconstraints is empty we do not have a diagnosed system and the parameter will
            // always be set
            if (y0set && startpointinfo.isYDoF()) {
                constrainty0();

                handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters
                                                         // after each constraint addition
            }

            auto edgeinfo = handler->getEdgeInfo(firstCurve);
            auto circle = static_cast<SolverGeometryExtension::Circle&>(edgeinfo);

            // if Autoconstraints is empty we do not have a diagnosed system and the parameter will
            // always be set
            if (radiusSet && circle.isRadiusDoF()) {
                constraintradius();
            }
        }
    }
    // No constraint possible for 3 rim circle.
}

}  // namespace SketcherGui

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


#ifndef SKETCHERGUI_DrawSketchHandlerCircle_H
#define SKETCHERGUI_DrawSketchHandlerCircle_H

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

#include <Mod/Part/App/Geometry2d.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"
#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

#include "CircleEllipseConstructionMethod.h"

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerCircle;

using DSHCircleController =
    DrawSketchDefaultWidgetController<DrawSketchHandlerCircle,
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
    friend DSHCircleController;
    friend DSHCircleControllerBase;

public:
    explicit DrawSketchHandlerCircle(ConstructionMethod constrMethod = ConstructionMethod::Center)
        : DrawSketchHandlerCircleBase(constrMethod)
        , radius(0.0)
    {}
    ~DrawSketchHandlerCircle() override = default;

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);
                if (constructionMethod() == ConstructionMethod::Center) {
                    centerPoint = onSketchPos;

                    if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d())) {
                        renderSuggestConstraintsCursor(sugConstraints[0]);
                        return;
                    }
                }
                else {
                    firstPoint = onSketchPos;

                    if (seekAutoConstraint(sugConstraints[0],
                                           onSketchPos,
                                           Base::Vector2d(),
                                           AutoConstraint::CURVE)) {
                        renderSuggestConstraintsCursor(sugConstraints[0]);
                        return;
                    }
                }
            } break;
            case SelectMode::SeekSecond: {
                if (constructionMethod() == ConstructionMethod::ThreeRim) {
                    centerPoint = (onSketchPos - firstPoint) / 2 + firstPoint;
                    secondPoint = onSketchPos;
                }

                radius = (onSketchPos - centerPoint).Length();

                CreateAndDrawShapeGeometry();

                if (constructionMethod() == ConstructionMethod::Center) {
                    toolWidgetManager.drawDoubleAtCursor(onSketchPos, radius);
                }
                else {
                    toolWidgetManager.drawPositionAtCursor(onSketchPos);
                }

                if (seekAutoConstraint(sugConstraints[1],
                                       onSketchPos,
                                       constructionMethod() == ConstructionMethod::Center
                                           ? onSketchPos - centerPoint
                                           : Base::Vector2d(),
                                       AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            } break;
            case SelectMode::SeekThird: {
                try {
                    if (areColinear(firstPoint, secondPoint, onSketchPos)) {
                        // If points are colinear then we can't calculate the center.
                        return;
                    }

                    centerPoint =
                        Part::Geom2dCircle::getCircleCenter(firstPoint, secondPoint, onSketchPos);

                    radius = (onSketchPos - centerPoint).Length();

                    toolWidgetManager.drawPositionAtCursor(onSketchPos);

                    CreateAndDrawShapeGeometry();

                    if (seekAutoConstraint(sugConstraints[2],
                                           onSketchPos,
                                           Base::Vector2d(0.f, 0.f),
                                           AutoConstraint::CURVE)) {
                        renderSuggestConstraintsCursor(sugConstraints[2]);
                        return;
                    }
                }
                catch (Base::ValueError& e) {
                    e.ReportException();
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
            Gui::NotifyError(sketchgui,
                             QT_TRANSLATE_NOOP("Notifications", "Error"),
                             QT_TRANSLATE_NOOP("Notifications", "Failed to add circle"));

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
        int CircleGeoId = getHighestCurveIndex();

        if (constructionMethod() == ConstructionMethod::Center) {
            auto& ac1 = sugConstraints[0];
            auto& ac2 = sugConstraints[1];

            generateAutoConstraintsOnElement(
                ac1,
                CircleGeoId,
                Sketcher::PointPos::mid);  // add auto constraints for the center point
            generateAutoConstraintsOnElement(
                ac2,
                CircleGeoId,
                Sketcher::PointPos::none);  // add auto constraints for the edge
        }
        else {

            auto& ac1 = sugConstraints[0];
            auto& ac2 = sugConstraints[1];
            auto& ac3 = sugConstraints[2];

            generateAutoConstraintsOnElement(
                ac1,
                CircleGeoId,
                Sketcher::PointPos::none);  // add auto constraints for the first point
            generateAutoConstraintsOnElement(
                ac2,
                CircleGeoId,
                Sketcher::PointPos::none);  // add auto constraints for the second point
            generateAutoConstraintsOnElement(
                ac3,
                CircleGeoId,
                Sketcher::PointPos::none);  // add auto constraints for the second point
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
            return QString::fromLatin1("Sketcher_Pointer_Create_Circle");
        }
        else {
            return QString::fromLatin1("Sketcher_Pointer_Create_3PointCircle");
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
        return QString(QObject::tr("Circle parameters"));
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
                THROWM(Base::ValueError,
                       "OnViewParameter index without an associated machine state")
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
        QStringList names = {QApplication::translate("Sketcher_CreateCircle", "Center"),
                             QApplication::translate("Sketcher_CreateCircle", "3 rim points")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        if (isConstructionMode()) {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle_Constr"));
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointCircle_Constr"));
        }
        else {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle"));
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointCircle"));
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
        onViewParameters[OnViewParameter::Third]->setLabelType(
            Gui::SoDatumLabel::RADIUS,
            Gui::EditableDatumLabel::Function::Dimensioning);
    }
}

template<>
void DSHCircleControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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
            if (handler->constructionMethod()
                == DrawSketchHandlerCircle::ConstructionMethod::Center) {
                if (onViewParameters[OnViewParameter::Third]->isSet) {
                    double radius = onViewParameters[OnViewParameter::Third]->getValue();
                    if (radius < Precision::Confusion()) {
                        unsetOnViewParameter(onViewParameters[OnViewParameter::Third].get());
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
                if (onViewParameters[OnViewParameter::Third]->isSet) {
                    onSketchPos.x = onViewParameters[OnViewParameter::Third]->getValue();
                }

                if (onViewParameters[OnViewParameter::Fourth]->isSet) {
                    onSketchPos.y = onViewParameters[OnViewParameter::Fourth]->getValue();
                }

                if (onViewParameters[OnViewParameter::Third]->isSet
                    && onViewParameters[OnViewParameter::Fourth]->isSet
                    && (onSketchPos - handler->firstPoint).Length() < Precision::Confusion()) {
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Third].get());
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Fourth].get());
                }
            }
        } break;
        case SelectMode::SeekThird: {  // 3 rims only
            if (onViewParameters[OnViewParameter::Fifth]->isSet) {
                onSketchPos.x = onViewParameters[OnViewParameter::Fifth]->getValue();
            }

            if (onViewParameters[OnViewParameter::Sixth]->isSet) {
                onSketchPos.y = onViewParameters[OnViewParameter::Sixth]->getValue();
            }
            if (onViewParameters[OnViewParameter::Fifth]->isSet
                && onViewParameters[OnViewParameter::Sixth]->isSet
                && areColinear(handler->firstPoint, handler->secondPoint, onSketchPos)) {
                unsetOnViewParameter(onViewParameters[OnViewParameter::Fifth].get());
                unsetOnViewParameter(onViewParameters[OnViewParameter::Sixth].get());
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
            if (handler->constructionMethod()
                == DrawSketchHandlerCircle::ConstructionMethod::Center) {
                if (!onViewParameters[OnViewParameter::Third]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Third, handler->radius);
                }

                Base::Vector3d start = toVector3d(handler->centerPoint);
                Base::Vector3d end = toVector3d(onSketchPos);

                onViewParameters[OnViewParameter::Third]->setPoints(start, end);
            }
            else {
                if (!onViewParameters[OnViewParameter::Third]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Third, onSketchPos.x);
                }

                if (!onViewParameters[OnViewParameter::Fourth]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Fourth, onSketchPos.y);
                }

                bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
                onViewParameters[OnViewParameter::Third]->setLabelAutoDistanceReverse(!sameSign);
                onViewParameters[OnViewParameter::Fourth]->setLabelAutoDistanceReverse(sameSign);
                onViewParameters[OnViewParameter::Third]->setPoints(Base::Vector3d(),
                                                                    toVector3d(onSketchPos));
                onViewParameters[OnViewParameter::Fourth]->setPoints(Base::Vector3d(),
                                                                     toVector3d(onSketchPos));
            }
        } break;
        case SelectMode::SeekThird: {  // 3 rims only
            if (!onViewParameters[OnViewParameter::Fifth]->isSet) {
                setOnViewParameterValue(OnViewParameter::Fifth, onSketchPos.x);
            }

            if (!onViewParameters[OnViewParameter::Sixth]->isSet) {
                setOnViewParameterValue(OnViewParameter::Sixth, onSketchPos.y);
            }

            bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
            onViewParameters[OnViewParameter::Fifth]->setLabelAutoDistanceReverse(!sameSign);
            onViewParameters[OnViewParameter::Sixth]->setLabelAutoDistanceReverse(sameSign);
            onViewParameters[OnViewParameter::Fifth]->setPoints(Base::Vector3d(),
                                                                toVector3d(onSketchPos));
            onViewParameters[OnViewParameter::Sixth]->setPoints(Base::Vector3d(),
                                                                toVector3d(onSketchPos));
        } break;
        default:
            break;
    }
}

template<>
void DSHCircleController::doChangeDrawSketchHandlerMode()
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
                && handler->constructionMethod()
                    == DrawSketchHandlerCircle::ConstructionMethod::Center) {

                handler->setState(SelectMode::End);
            }
            else if (onViewParameters[OnViewParameter::Third]->isSet
                     && onViewParameters[OnViewParameter::Fourth]->isSet
                     && handler->constructionMethod()
                         == DrawSketchHandlerCircle::ConstructionMethod::ThreeRim) {

                handler->setState(SelectMode::SeekThird);
            }
        } break;
        case SelectMode::SeekThird: {
            if (onViewParameters[OnViewParameter::Fifth]->isSet
                && onViewParameters[OnViewParameter::Sixth]->isSet) {

                handler->setState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
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
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid),
                                   GeoElementId::VAxis,
                                   x0,
                                   handler->sketchgui->getObject());
        };

        auto constrainty0 = [&]() {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid),
                                   GeoElementId::HAxis,
                                   y0,
                                   handler->sketchgui->getObject());
        };

        auto constraintradius = [&]() {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(),
                                  "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                                  firstCurve,
                                  handler->radius);
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
                    GeoElementId(firstCurve, PointPos::mid));  // get updated point position
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


#endif  // SKETCHERGUI_DrawSketchHandlerCircle_H

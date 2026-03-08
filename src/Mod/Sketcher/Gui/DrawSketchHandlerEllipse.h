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

#include <cmath>


#include <QApplication>

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"
#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"

#include "CircleEllipseConstructionMethod.h"

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

/* Ellipse ==============================================================================*/
class DrawSketchHandlerEllipse;

using DSHEllipseController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerEllipse,
    StateMachines::ThreeSeekEnd,
    /*PAutoConstraintSize =*/3,
    /*OnViewParametersT =*/OnViewParameters<5, 6>,  // NOLINT
    /*WidgetParametersT =*/WidgetParameters<0, 0>,  // NOLINT
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,  // NOLINT
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,  // NOLINT
    ConstructionMethods::CircleEllipseConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHEllipseControllerBase = DSHEllipseController::ControllerBase;

using DrawSketchHandlerEllipseBase = DrawSketchControllableHandler<DSHEllipseController>;

class DrawSketchHandlerEllipse: public DrawSketchHandlerEllipseBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerEllipse)

    friend DSHEllipseController;
    friend DSHEllipseControllerBase;

public:
    explicit DrawSketchHandlerEllipse(ConstructionMethod constrMethod = ConstructionMethod::Center)
        : DrawSketchHandlerEllipseBase(constrMethod)
        , firstRadius(0.0)
        , secondRadius(0.0)
        , majorRadius(0.0)
        , minorRadius(0.0)
        , ellipseGeoId(Sketcher::GeoEnum::GeoUndef)
    {}
    ~DrawSketchHandlerEllipse() override = default;

private:
    std::list<Gui::InputHint> getToolHints() const override
    {
        using State = std::pair<ConstructionMethod, SelectMode>;
        using enum Gui::InputHint::UserInput;

        const Gui::InputHint switchModeHint {tr("%1 switch mode"), {KeyM}};

        return Gui::lookupHints<State>(
            {constructionMethod(), state()},
            {
                // Center method
                {.state = {ConstructionMethod::Center, SelectMode::SeekFirst},
                 .hints =
                     {
                         {tr("%1 pick ellipse center"), {MouseLeft}},
                         switchModeHint,
                     }},
                {.state = {ConstructionMethod::Center, SelectMode::SeekSecond},
                 .hints =
                     {
                         {tr("%1 pick axis endpoint"), {MouseLeft}},
                         switchModeHint,
                     }},
                {.state = {ConstructionMethod::Center, SelectMode::SeekThird},
                 .hints =
                     {
                         {tr("%1 pick minor axis endpoint"), {MouseLeft}},
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

                    seekAndRenderAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f));
                }
                else {
                    apoapsis = onSketchPos;

                    seekAndRenderAutoConstraint(
                        sugConstraints[0],
                        onSketchPos,
                        Base::Vector2d(0.f, 0.f),
                        AutoConstraint::CURVE
                    );
                }
            } break;
            case SelectMode::SeekSecond: {
                periapsis = onSketchPos;

                calculateMajorAxisParameters();

                CreateAndDrawShapeGeometry();

                if (constructionMethod() == ConstructionMethod::Center) {
                    toolWidgetManager.drawDoubleAtCursor(onSketchPos, firstRadius);
                }
                else {
                    toolWidgetManager.drawPositionAtCursor(onSketchPos);
                }

                seekAndRenderAutoConstraint(
                    sugConstraints[1],
                    onSketchPos,
                    Base::Vector2d(0.f, 0.f),
                    AutoConstraint::CURVE
                );
            } break;
            case SelectMode::SeekThird: {
                calculateThroughPointMinorAxisParameters(onSketchPos);

                CreateAndDrawShapeGeometry();

                if (constructionMethod() == ConstructionMethod::Center) {
                    toolWidgetManager.drawWidthHeightAtCursor(onSketchPos, firstRadius, secondRadius);
                }
                else {
                    toolWidgetManager.drawPositionAtCursor(onSketchPos);
                }

                seekAndRenderAutoConstraint(
                    sugConstraints[2],
                    onSketchPos,
                    Base::Vector2d(0.f, 0.f),
                    AutoConstraint::CURVE
                );
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch ellipse"));

            ellipseGeoId = getHighestCurveIndex() + 1;

            createShape(false);

            commandAddShapeGeometryAndConstraints();

            // in the exceptional event that this may lead to a circle, do not
            // exposeInternalGeometry
            if (!ShapeGeometry.empty() && ShapeGeometry[0]->is<Part::GeomEllipse>()) {
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", ellipseGeoId);
            }

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to add ellipse")
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

        if (constructionMethod() == ConstructionMethod::Center) {

            auto& ac1 = sugConstraints[0];
            auto& ac2 = sugConstraints[1];
            auto& ac3 = sugConstraints[2];

            generateAutoConstraintsOnElement(
                ac1,
                ellipseGeoId,
                Sketcher::PointPos::mid
            );  // add auto constraints for the center point
            generateAutoConstraintsOnElement(
                ac2,
                ellipseGeoId,
                Sketcher::PointPos::none
            );  // add auto constraints for the edge
            generateAutoConstraintsOnElement(
                ac3,
                ellipseGeoId,
                Sketcher::PointPos::none
            );  // add auto constraints for the edge
        }
        else {

            auto& ac1 = sugConstraints[0];
            auto& ac2 = sugConstraints[1];
            auto& ac3 = sugConstraints[2];

            generateAutoConstraintsOnElement(
                ac1,
                ellipseGeoId,
                Sketcher::PointPos::none
            );  // add auto constraints for the first point
            generateAutoConstraintsOnElement(
                ac2,
                ellipseGeoId,
                Sketcher::PointPos::none
            );  // add auto constraints for the second point
            generateAutoConstraintsOnElement(
                ac3,
                ellipseGeoId,
                Sketcher::PointPos::none
            );  // add auto constraints for the edge
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
        return "DSH_Ellipse";
    }

    QString getCrosshairCursorSVGName() const override
    {
        if (constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            return QStringLiteral("Sketcher_Pointer_Create_EllipseByCenter");
        }
        else {
            return QStringLiteral("Sketcher_Pointer_Create_Ellipse_3points");
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
        return Gui::BitmapFactory().pixmap("Sketcher_CreateEllipseByCenter");
    }

    QString getToolWidgetText() const override
    {
        return QString(tr("Ellipse Parameters"));
    }

    bool canGoToNextMode() override
    {
        if (state() == SelectMode::SeekSecond && firstRadius < Precision::Confusion()) {
            // Prevent validation of null ellipse.
            return false;
        }

        if (state() == SelectMode::SeekThird
            && (firstRadius < Precision::Confusion() || secondRadius < Precision::Confusion())) {
            // Prevent validation of null ellipse.
            return false;
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

    void calculateMajorAxisParameters()
    {
        if (constructionMethod() == ConstructionMethod::ThreeRim) {
            centerPoint = (apoapsis - periapsis) / 2 + periapsis;
        }

        firstAxis = periapsis - centerPoint;
        firstRadius = firstAxis.Length();
    }

    void calculateThroughPointMinorAxisParameters(const Base::Vector2d& onSketchPos)
    {
        // we calculate the ellipse that will pass via the cursor as per de la Hire

        Base::Vector2d projx;
        projx.ProjectToLine(
            onSketchPos - centerPoint,
            firstAxis
        );  // projection onto the major axis

        auto projy = onSketchPos - centerPoint - projx;

        auto lprojx = projx.Length();  // Px = a cos t
        auto lprojy = projy.Length();  // Py = b sin t

        if (lprojx > firstRadius) {
            secondRadius = lprojy;
        }
        else {
            double t = std::acos(lprojx / firstRadius);
            if (t == 0.0) {
                secondRadius = 0.0;
            }
            else {
                secondRadius = lprojy / std::sin(t);  // b = Py / sin t
            }
        }

        secondAxis = projy.Normalize() * secondRadius;
    }

    void calculateMinorAxis(double minorradius)
    {
        // Find bPoint For that first we need the distance of onSketchPos to major axis.
        secondAxis = firstAxis.Perpendicular(false).Normalize() * minorRadius;
        secondRadius = minorradius;
    }

    void createShape(bool onlyeditoutline) override
    {
        Q_UNUSED(onlyeditoutline);

        ShapeGeometry.clear();

        Base::Vector2d majorAxis = firstAxis;
        majorRadius = firstRadius;
        if (state() == SelectMode::SeekSecond) {
            const double scale = 0.5;
            minorRadius = majorRadius * scale;
        }
        else {  // SelectMode::SeekThird or SelectMode::End
            minorRadius = secondRadius;

            if (secondRadius > firstRadius) {
                majorAxis = secondAxis;
                majorRadius = secondRadius;
                minorRadius = firstRadius;
            }
        }
        if (majorRadius < Precision::Confusion() || minorRadius < Precision::Confusion()) {
            return;
        }

        if (fabs(firstRadius - secondRadius) < Precision::Confusion()) {
            addCircleToShapeGeometry(toVector3d(centerPoint), firstRadius, isConstructionMode());
        }
        else {
            addEllipseToShapeGeometry(
                toVector3d(centerPoint),
                toVector3d(majorAxis),
                majorRadius,
                minorRadius,
                isConstructionMode()
            );
        }
    }

private:
    Base::Vector2d centerPoint,
        periapsis;            // Center Mode SeekFirst and SeekSecond, 3PointMode SeekFirst
    Base::Vector2d apoapsis;  // 3Point SeekSecond
    Base::Vector2d firstAxis, secondAxis;
    double firstRadius, secondRadius, majorRadius, minorRadius;
    int ellipseGeoId;
};

template<>
auto DSHEllipseControllerBase::getState(int parameterindex) const
{

    switch (parameterindex) {
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
            if (handler->constructionMethod() == ConstructionMethod::ThreeRim) {
                return SelectMode::SeekThird;
            }
            THROWM(Base::ValueError, "OnViewParameter index without an associated machine state")
            break;
        default:
            THROWM(Base::ValueError, "OnViewParameter index without an associated machine state")
    }
}

template<>
void DSHEllipseController::configureToolWidget()
{

    if (!init) {  // Code to be executed only upon initialisation
        QStringList names = {
            QApplication::translate("Sketcher_CreateEllipse", "Center"),
            QApplication::translate("Sketcher_CreateEllipse", "Axis endpoints")
        };
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        if (isConstructionMode()) {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipseByCenter_Constr")
            );
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse_3points_Constr")
            );
        }
        else {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipseByCenter")
            );
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse_3points")
            );
        }
    }

    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);

    if (handler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::ThreeRim) {
        onViewParameters[OnViewParameter::Third]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
        onViewParameters[OnViewParameter::Fourth]->setLabelType(Gui::SoDatumLabel::DISTANCEY);

        onViewParameters[OnViewParameter::Fifth]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
        onViewParameters[OnViewParameter::Sixth]->setLabelType(Gui::SoDatumLabel::DISTANCEY);
    }
    else {
        onViewParameters[OnViewParameter::Third]->setLabelType(
            Gui::SoDatumLabel::RADIUS,
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
}

template<>
void DSHEllipseControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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

            if (handler->constructionMethod()
                == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
                Base::Vector2d dir = onSketchPos - handler->centerPoint;
                if (dir.Length() < Precision::Confusion()) {
                    dir.x = 1.0;  // if direction null, default to (1,0)
                }
                double length = dir.Length();

                if (thirdParam->isSet) {
                    length = thirdParam->getValue();
                    if (length < Precision::Confusion() && thirdParam->hasFinishedEditing) {
                        unsetOnViewParameter(thirdParam.get());
                        return;
                    }

                    onSketchPos = handler->centerPoint + length * dir.Normalize();
                }

                if (fourthParam->isSet) {
                    double angle = Base::toRadians(fourthParam->getValue());
                    onSketchPos.x = handler->centerPoint.x + cos(angle) * length;
                    onSketchPos.y = handler->centerPoint.y + sin(angle) * length;
                }
            }
            else {
                if (thirdParam->isSet) {
                    onSketchPos.x = thirdParam->getValue();
                }

                if (fourthParam->isSet) {
                    onSketchPos.y = fourthParam->getValue();
                }

                if (thirdParam->hasFinishedEditing && fourthParam->hasFinishedEditing
                    && (onSketchPos - handler->apoapsis).Length() < Precision::Confusion()) {
                    unsetOnViewParameter(thirdParam.get());
                    unsetOnViewParameter(fourthParam.get());
                }
            }
        } break;
        case SelectMode::SeekThird: {
            auto& fifthParam = onViewParameters[OnViewParameter::Fifth];

            if (handler->constructionMethod()
                == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
                if (fifthParam->isSet) {
                    auto minorradius = fifthParam->getValue();
                    onSketchPos = handler->centerPoint
                        + (handler->periapsis - handler->centerPoint).Perpendicular(true).Normalize()
                            * minorradius;
                }
            }
            else {
                auto& sixthParam = onViewParameters[OnViewParameter::Sixth];
                if (fifthParam->isSet) {
                    onSketchPos.x = fifthParam->getValue();
                }

                if (sixthParam->isSet) {
                    onSketchPos.y = sixthParam->getValue();
                }

                if (fifthParam->hasFinishedEditing && sixthParam->hasFinishedEditing
                    && areCollinear(handler->apoapsis, handler->periapsis, onSketchPos)) {
                    unsetOnViewParameter(fifthParam.get());
                    unsetOnViewParameter(sixthParam.get());
                }
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHEllipseController::adaptParameters(Base::Vector2d onSketchPos)
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

            if (handler->constructionMethod()
                == DrawSketchHandlerEllipse::ConstructionMethod::Center) {

                auto vec = onSketchPos - handler->centerPoint;
                if (!thirdParam->isSet) {
                    setOnViewParameterValue(OnViewParameter::Third, vec.Length());
                }

                if (!fourthParam->isSet) {
                    double angle = vec.Length() > 0 ? Base::toDegrees(vec.Angle()) : 0;
                    setOnViewParameterValue(OnViewParameter::Fourth, angle, Base::Unit::Angle);
                }

                Base::Vector3d start = toVector3d(handler->centerPoint);
                Base::Vector3d end = toVector3d(onSketchPos);

                thirdParam->setPoints(start, end);

                fourthParam->setPoints(start, Base::Vector3d());
                fourthParam->setLabelRange((onSketchPos - handler->centerPoint).Angle());
            }
            else {
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
        case SelectMode::SeekThird: {
            auto& fifthParam = onViewParameters[OnViewParameter::Fifth];

            if (handler->constructionMethod()
                == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
                if (!fifthParam->isSet) {
                    setOnViewParameterValue(OnViewParameter::Fifth, handler->secondAxis.Length());
                }

                Base::Vector3d start = toVector3d(handler->centerPoint);
                Base::Vector3d end = toVector3d(handler->centerPoint + handler->secondAxis);

                fifthParam->setPoints(start, end);
            }
            else {
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
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHEllipseController::computeNextDrawSketchHandlerMode()
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

            if (handler->constructionMethod()
                == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
                if (fifthParam->hasFinishedEditing) {
                    handler->setNextState(SelectMode::End);
                }
            }
            else {
                auto& sixthParam = onViewParameters[OnViewParameter::Sixth];
                if (fifthParam->hasFinishedEditing && sixthParam->hasFinishedEditing) {
                    handler->setNextState(SelectMode::End);
                }
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHEllipseController::addConstraints()
{
    if (handler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
        int firstCurve = handler->ellipseGeoId;

        auto x0 = onViewParameters[OnViewParameter::First]->getValue();
        auto y0 = onViewParameters[OnViewParameter::Second]->getValue();
        auto angle = Base::toRadians(onViewParameters[OnViewParameter::Fourth]->getValue());

        auto x0set = onViewParameters[OnViewParameter::First]->isSet;
        auto y0set = onViewParameters[OnViewParameter::Second]->isSet;
        auto firstRadiusSet = onViewParameters[OnViewParameter::Third]->isSet;
        auto angleSet = onViewParameters[OnViewParameter::Fourth]->isSet;
        auto secondRadiusSet = onViewParameters[OnViewParameter::Fifth]->isSet;

        using namespace Sketcher;

        if (!handler->ShapeGeometry.empty() && handler->ShapeGeometry[0]->is<Part::GeomEllipse>()) {

            int firstLine = firstCurve + 1;   // this is always the major axis
            int secondLine = firstCurve + 2;  // this is always the minor axis

            if (handler->firstRadius < handler->secondRadius) {
                std::swap(firstLine, secondLine);
            }

            // NOTE: Because mouse positions are enforced by the widget, it is not possible to use
            // the last radius > first radius when widget parameters are enforced. Then firstLine
            // always goes with firstRadiusSet.

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

            auto constraintFirstRadius = [&]() {
                Gui::cmdAppObjectArgs(
                    handler->sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                    firstCurve,
                    3,
                    firstLine,
                    1,
                    handler->firstRadius
                );
            };

            auto constraintSecondRadius = [&]() {
                Gui::cmdAppObjectArgs(
                    handler->sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                    firstCurve,
                    3,
                    secondLine,
                    1,
                    handler->secondRadius
                );
            };

            auto constraintAngle = [&]() {
                Gui::cmdAppObjectArgs(
                    handler->sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
                    firstLine,
                    angle
                );
            };

            // NOTE: if AutoConstraints is empty, we can add constraints directly without any
            // diagnose. No diagnose was run.
            if (handler->AutoConstraints.empty()) {
                if (x0set) {
                    constraintx0();
                }

                if (y0set) {
                    constrainty0();
                }


                // this require to show internal geometry.
                if (firstRadiusSet) {
                    constraintFirstRadius();
                }

                // Todo: this makes the ellipse 'jump' because it's doing a 180 degree turn before
                // applying asked angle. Probably because start and end points of line are not in
                // the correct direction.
                if (angleSet) {
                    constraintAngle();
                }

                if (secondRadiusSet) {
                    constraintSecondRadius();
                }
            }
            else {  // There is a valid diagnose.
                auto centerpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter
                // will always be set
                if (x0set && centerpointinfo.isXDoF()) {
                    constraintx0();

                    handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters
                                                             // after each constraint addition

                    centerpointinfo = handler->getPointInfo(
                        GeoElementId(firstCurve, PointPos::mid)
                    );  // get updated point position
                }

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter
                // will always be set
                if (y0set && centerpointinfo.isYDoF()) {
                    constrainty0();

                    handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters
                                                             // after each constraint addition
                }

                // Major axis (it is not a solver parameter in the solver implementation)

                int leftDoFs = handler->getLineDoFs(firstLine);

                if (firstRadiusSet && leftDoFs > 0) {
                    constraintFirstRadius();
                    handler->diagnoseWithAutoConstraints();  // It is not a normal line as it is
                                                             // constrained by the Ellipse, so we need
                                                             // to recalculate after radius addition
                    leftDoFs = handler->getLineDoFs(firstLine);
                }

                if (angleSet && leftDoFs > 0) {
                    constraintAngle();
                    handler->diagnoseWithAutoConstraints();  // It is not a normal line as it is
                                                             // constrained by the Ellipse, so we need
                                                             // to recalculate after radius addition
                }

                // Minor axis (it is a solver parameter in the solver implementation)

                auto edgeinfo = handler->getEdgeInfo(firstCurve);
                auto ellipse = static_cast<SolverGeometryExtension::Ellipse&>(edgeinfo);

                if (secondRadiusSet && ellipse.isMinorRadiusDoF()) {
                    constraintSecondRadius();
                }
            }
        }
        else {  // it is a circle
            int firstCurve = handler->getHighestCurveIndex();

            auto x0 = toolWidget->getParameter(OnViewParameter::First);
            auto y0 = toolWidget->getParameter(OnViewParameter::Second);

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
                Gui::cmdAppObjectArgs(
                    handler->sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                    firstCurve,
                    handler->firstRadius
                );
            };

            // NOTE: if AutoConstraints is empty, we can add constraints directly without any
            // diagnose. No diagnose was run.
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

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter
                // will always be set
                if (x0set && startpointinfo.isXDoF()) {
                    constraintx0();

                    handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters
                                                             // after each constraint addition

                    startpointinfo = handler->getPointInfo(
                        GeoElementId(firstCurve, PointPos::mid)
                    );  // get updated point position
                }

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter
                // will always be set
                if (y0set && startpointinfo.isYDoF()) {
                    constrainty0();

                    handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters
                                                             // after each constraint addition
                }

                auto edgeinfo = handler->getEdgeInfo(firstCurve);
                auto circle = static_cast<SolverGeometryExtension::Circle&>(edgeinfo);

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter
                // will always be set
                if (radiusSet && circle.isRadiusDoF()) {
                    constraintradius();
                }
            }
        }
    }
    // No constraint possible for 3 rim ellipse.
}
}  // namespace SketcherGui

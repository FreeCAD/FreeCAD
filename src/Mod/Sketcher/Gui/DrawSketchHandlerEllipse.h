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


#ifndef SKETCHERGUI_DrawSketchHandlerEllipse_H
#define SKETCHERGUI_DrawSketchHandlerEllipse_H

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

using DSHEllipseController =
    DrawSketchDefaultWidgetController<DrawSketchHandlerEllipse,
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
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);

                if (constructionMethod() == ConstructionMethod::Center) {
                    centerPoint = onSketchPos;

                    if (seekAutoConstraint(sugConstraints[0],
                                           onSketchPos,
                                           Base::Vector2d(0.f, 0.f))) {
                        renderSuggestConstraintsCursor(sugConstraints[0]);
                        return;
                    }
                }
                else {
                    apoapsis = onSketchPos;

                    if (seekAutoConstraint(sugConstraints[0],
                                           onSketchPos,
                                           Base::Vector2d(0.f, 0.f),
                                           AutoConstraint::CURVE)) {
                        renderSuggestConstraintsCursor(sugConstraints[0]);
                        return;
                    }
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

                if (seekAutoConstraint(sugConstraints[1],
                                       onSketchPos,
                                       Base::Vector2d(0.f, 0.f),
                                       AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            } break;
            case SelectMode::SeekThird: {
                calculateThroughPointMinorAxisParameters(onSketchPos);

                CreateAndDrawShapeGeometry();

                if (constructionMethod() == ConstructionMethod::Center) {
                    toolWidgetManager.drawWidthHeightAtCursor(onSketchPos,
                                                              firstRadius,
                                                              secondRadius);
                }
                else {
                    toolWidgetManager.drawPositionAtCursor(onSketchPos);
                }

                if (seekAutoConstraint(sugConstraints[2],
                                       onSketchPos,
                                       Base::Vector2d(0.f, 0.f),
                                       AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstraints[2]);
                    return;
                }
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
            if (!ShapeGeometry.empty()
                && ShapeGeometry[0]->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "exposeInternalGeometry(%d)",
                                      ellipseGeoId);
            }

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(sketchgui,
                             QT_TRANSLATE_NOOP("Notifications", "Error"),
                             QT_TRANSLATE_NOOP("Notifications", "Failed to add ellipse"));

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

        if (constructionMethod() == ConstructionMethod::Center) {

            auto& ac1 = sugConstraints[0];
            auto& ac2 = sugConstraints[1];
            auto& ac3 = sugConstraints[2];

            generateAutoConstraintsOnElement(
                ac1,
                ellipseGeoId,
                Sketcher::PointPos::mid);  // add auto constraints for the center point
            generateAutoConstraintsOnElement(
                ac2,
                ellipseGeoId,
                Sketcher::PointPos::none);  // add auto constraints for the edge
            generateAutoConstraintsOnElement(
                ac3,
                ellipseGeoId,
                Sketcher::PointPos::none);  // add auto constraints for the edge
        }
        else {

            auto& ac1 = sugConstraints[0];
            auto& ac2 = sugConstraints[1];
            auto& ac3 = sugConstraints[2];

            generateAutoConstraintsOnElement(
                ac1,
                ellipseGeoId,
                Sketcher::PointPos::none);  // add auto constraints for the first point
            generateAutoConstraintsOnElement(
                ac2,
                ellipseGeoId,
                Sketcher::PointPos::none);  // add auto constraints for the second point
            generateAutoConstraintsOnElement(
                ac3,
                ellipseGeoId,
                Sketcher::PointPos::none);  // add auto constraints for the edge
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
            return QString::fromLatin1("Sketcher_Pointer_Create_EllipseByCenter");
        }
        else {
            return QString::fromLatin1("Sketcher_Pointer_Create_Ellipse_3points");
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
        return QString(QObject::tr("Ellipse parameters"));
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
        projx.ProjectToLine(onSketchPos - centerPoint,
                            firstAxis);  // projection onto the major axis

        auto projy = onSketchPos - centerPoint - projx;

        auto lprojx = projx.Length();  // Px = a cos t
        auto lprojy = projy.Length();  // Py = b sin t

        double t = std::acos(lprojx / firstRadius);

        secondRadius = lprojy / std::sin(t);  // b = Py / sin t

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
            addEllipseToShapeGeometry(toVector3d(centerPoint),
                                      toVector3d(majorAxis),
                                      majorRadius,
                                      minorRadius,
                                      isConstructionMode());
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
        QStringList names = {QApplication::translate("Sketcher_CreateEllipse", "Center"),
                             QApplication::translate("Sketcher_CreateEllipse", "Axis endpoints")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        if (isConstructionMode()) {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipseByCenter_Constr"));
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse_3points_Constr"));
        }
        else {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipseByCenter"));
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse_3points"));
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
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Fourth]->setLabelType(
            Gui::SoDatumLabel::ANGLE,
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Fifth]->setLabelType(
            Gui::SoDatumLabel::RADIUS,
            Gui::EditableDatumLabel::Function::Dimensioning);
    }
}

template<>
void DSHEllipseControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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
                == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
                Base::Vector2d dir = onSketchPos - handler->centerPoint;
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

                    onSketchPos = handler->centerPoint + length * dir.Normalize();
                }

                if (onViewParameters[OnViewParameter::Fourth]->isSet) {
                    double angle =
                        Base::toRadians(onViewParameters[OnViewParameter::Fourth]->getValue());
                    onSketchPos.x = handler->centerPoint.x + cos(angle) * length;
                    onSketchPos.y = handler->centerPoint.y + sin(angle) * length;
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
                    && (onSketchPos - handler->apoapsis).Length() < Precision::Confusion()) {
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Third].get());
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Fourth].get());
                }
            }
        } break;
        case SelectMode::SeekThird: {
            if (handler->constructionMethod()
                == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
                if (onViewParameters[OnViewParameter::Fifth]->isSet) {
                    auto minorradius = onViewParameters[OnViewParameter::Fifth]->getValue();
                    onSketchPos = handler->centerPoint
                        + (handler->periapsis - handler->centerPoint)
                                .Perpendicular(true)
                                .Normalize()
                            * minorradius;
                }
            }
            else {
                if (onViewParameters[OnViewParameter::Fifth]->isSet) {
                    onSketchPos.x = onViewParameters[OnViewParameter::Fifth]->getValue();
                }

                if (onViewParameters[OnViewParameter::Sixth]->isSet) {
                    onSketchPos.y = onViewParameters[OnViewParameter::Sixth]->getValue();
                }

                if (onViewParameters[OnViewParameter::Fifth]->isSet
                    && onViewParameters[OnViewParameter::Sixth]->isSet
                    && areColinear(handler->apoapsis, handler->periapsis, onSketchPos)) {
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Fifth].get());
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Sixth].get());
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
                == DrawSketchHandlerEllipse::ConstructionMethod::Center) {

                auto vec = onSketchPos - handler->centerPoint;
                if (!onViewParameters[OnViewParameter::Third]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Third, vec.Length());
                }

                if (!onViewParameters[OnViewParameter::Fourth]->isSet) {
                    double angle = vec.Length() > 0 ? Base::toDegrees(vec.Angle()) : 0;
                    setOnViewParameterValue(OnViewParameter::Fourth, angle, Base::Unit::Angle);
                }

                Base::Vector3d start = toVector3d(handler->centerPoint);
                Base::Vector3d end = toVector3d(onSketchPos);

                onViewParameters[OnViewParameter::Third]->setPoints(start, end);


                onViewParameters[OnViewParameter::Fourth]->setPoints(start, Base::Vector3d());
                onViewParameters[OnViewParameter::Fourth]->setLabelRange(
                    (onSketchPos - handler->centerPoint).Angle());
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
        case SelectMode::SeekThird: {
            if (handler->constructionMethod()
                == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
                if (!onViewParameters[OnViewParameter::Fifth]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Fifth, handler->secondAxis.Length());
                }

                Base::Vector3d start = toVector3d(handler->centerPoint);
                Base::Vector3d end = toVector3d(handler->centerPoint + handler->secondAxis);

                onViewParameters[OnViewParameter::Fifth]->setPoints(start, end);
            }
            else {
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
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHEllipseController::doChangeDrawSketchHandlerMode()
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
            if (handler->constructionMethod()
                == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
                if (onViewParameters[OnViewParameter::Fifth]->isSet) {

                    handler->setState(SelectMode::End);
                }
            }
            else {
                if (onViewParameters[OnViewParameter::Fifth]->isSet
                    && onViewParameters[OnViewParameter::Sixth]->isSet) {

                    handler->setState(SelectMode::End);
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

        if (!handler->ShapeGeometry.empty()
            && handler->ShapeGeometry[0]->getTypeId() == Part::GeomEllipse::getClassTypeId()) {

            int firstLine = firstCurve + 1;   // this is always the major axis
            int secondLine = firstCurve + 2;  // this is always the minor axis

            if (handler->firstRadius < handler->secondRadius) {
                std::swap(firstLine, secondLine);
            }

            // NOTE: Because mouse positions are enforced by the widget, it is not possible to use
            // the last radius > first radius when widget parameters are enforced. Then firstLine
            // always goes with firstRadiusSet.

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

            auto constraintFirstRadius = [&]() {
                Gui::cmdAppObjectArgs(
                    handler->sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                    firstCurve,
                    3,
                    firstLine,
                    1,
                    handler->firstRadius);
            };

            auto constraintSecondRadius = [&]() {
                Gui::cmdAppObjectArgs(
                    handler->sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                    firstCurve,
                    3,
                    secondLine,
                    1,
                    handler->secondRadius);
            };

            auto constraintAngle = [&]() {
                Gui::cmdAppObjectArgs(handler->sketchgui->getObject(),
                                      "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
                                      firstLine,
                                      angle);
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
                auto centerpointinfo =
                    handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter
                // will always be set
                if (x0set && centerpointinfo.isXDoF()) {
                    constraintx0();

                    handler
                        ->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters
                                                          // after each constraint addition

                    centerpointinfo = handler->getPointInfo(
                        GeoElementId(firstCurve, PointPos::mid));  // get updated point position
                }

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter
                // will always be set
                if (y0set && centerpointinfo.isYDoF()) {
                    constrainty0();

                    handler
                        ->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters
                                                          // after each constraint addition
                }

                // Major axis (it is not a solver parameter in the solver implementation)

                int leftDoFs = handler->getLineDoFs(firstLine);

                if (firstRadiusSet && leftDoFs > 0) {
                    constraintFirstRadius();
                    handler
                        ->diagnoseWithAutoConstraints();  // It is not a normal line as it is
                                                          // constrained by the Ellipse, so we need
                                                          // to recalculate after radius addition
                    leftDoFs = handler->getLineDoFs(firstLine);
                }

                if (angleSet && leftDoFs > 0) {
                    constraintAngle();
                    handler
                        ->diagnoseWithAutoConstraints();  // It is not a normal line as it is
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
                                      handler->firstRadius);
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
                auto startpointinfo =
                    handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter
                // will always be set
                if (x0set && startpointinfo.isXDoF()) {
                    constraintx0();

                    handler
                        ->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters
                                                          // after each constraint addition

                    startpointinfo = handler->getPointInfo(
                        GeoElementId(firstCurve, PointPos::mid));  // get updated point position
                }

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter
                // will always be set
                if (y0set && startpointinfo.isYDoF()) {
                    constrainty0();

                    handler
                        ->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters
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


#endif  // SKETCHERGUI_DrawSketchHandlerEllipse_H

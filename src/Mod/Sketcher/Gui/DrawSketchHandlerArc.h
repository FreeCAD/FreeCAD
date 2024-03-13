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


#ifndef SKETCHERGUI_DrawSketchHandlerArc_H
#define SKETCHERGUI_DrawSketchHandlerArc_H

#include <Base/Tools.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/CommandT.h>

#include <Mod/Part/App/Geometry2d.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"
#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"

#include "CircleEllipseConstructionMethod.h"

using namespace std;

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerArc;

using DSHArcController =
    DrawSketchDefaultWidgetController<DrawSketchHandlerArc,
                                      StateMachines::ThreeSeekEnd,
                                      /*PAutoConstraintSize =*/3,
                                      /*OnViewParametersT =*/OnViewParameters<5, 6>,  // NOLINT
                                      /*WidgetParametersT =*/WidgetParameters<0, 0>,  // NOLINT
                                      /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,  // NOLINT
                                      /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,  // NOLINT
                                      ConstructionMethods::CircleEllipseConstructionMethod,
                                      /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHArcControllerBase = DSHArcController::ControllerBase;

using DrawSketchHandlerArcBase = DrawSketchControllableHandler<DSHArcController>;

class DrawSketchHandlerArc: public DrawSketchHandlerArcBase
{
    friend DSHArcController;
    friend DSHArcControllerBase;

public:
    explicit DrawSketchHandlerArc(ConstructionMethod constrMethod = ConstructionMethod::Center)
        : DrawSketchHandlerArcBase(constrMethod)
        , radius(0.0)
        , startAngle(0.0)
        , endAngle(0.0)
        , arcAngle(0.0)
        , arcPos1(Sketcher::PointPos::none)
        , arcPos2(Sketcher::PointPos::none)
    {}

    ~DrawSketchHandlerArc() override = default;

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);

                if (constructionMethod() == ConstructionMethod::Center) {
                    centerPoint = onSketchPos;
                }
                else {
                    firstPoint = onSketchPos;
                }

                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            } break;
            case SelectMode::SeekSecond: {
                if (constructionMethod() == ConstructionMethod::Center) {
                    firstPoint = onSketchPos;
                    startAngle = (firstPoint - centerPoint).Angle();
                }
                else {
                    centerPoint = (onSketchPos - firstPoint) / 2 + firstPoint;
                    secondPoint = onSketchPos;
                }

                radius = (onSketchPos - centerPoint).Length();

                CreateAndDrawShapeGeometry();

                if (constructionMethod() == ConstructionMethod::Center) {
                    toolWidgetManager.drawDirectionAtCursor(onSketchPos, centerPoint);
                }
                else {
                    toolWidgetManager.drawPositionAtCursor(onSketchPos);
                }

                if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            } break;
            case SelectMode::SeekThird: {
                double startAngleBackup = startAngle;

                if (constructionMethod() == ConstructionMethod::Center) {
                    secondPoint = onSketchPos;
                    double angle1 = (onSketchPos - centerPoint).Angle() - startAngle;
                    double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
                    arcAngle = abs(angle1 - arcAngle) < abs(angle2 - arcAngle) ? angle1 : angle2;

                    if (arcAngle > 0) {
                        endAngle = startAngle + arcAngle;
                    }
                    else {
                        endAngle = startAngle;
                        startAngle = startAngle + arcAngle;
                    }
                }
                else {
                    if (areColinear(firstPoint, secondPoint, onSketchPos)) {
                        // If points are colinear then we can't calculate the center.
                        return;
                    }
                    centerPoint =
                        Part::Geom2dCircle::getCircleCenter(firstPoint, secondPoint, onSketchPos);
                    radius = (onSketchPos - centerPoint).Length();

                    double angle1 = (firstPoint - centerPoint).Angle();
                    double angle2 = (secondPoint - centerPoint).Angle();
                    double angle3 = (onSketchPos - centerPoint).Angle();

                    // Always build arc counter-clockwise
                    // Point 3 is between Point 1 and 2
                    if (angle3 > std::min(angle1, angle2) && angle3 < std::max(angle1, angle2)) {
                        if (angle2 > angle1) {
                            arcPos1 = Sketcher::PointPos::start;
                            arcPos2 = Sketcher::PointPos::end;
                        }
                        else {
                            arcPos1 = Sketcher::PointPos::end;
                            arcPos2 = Sketcher::PointPos::start;
                        }
                        startAngle = std::min(angle1, angle2);
                        endAngle = std::max(angle1, angle2);
                        arcAngle = endAngle - startAngle;
                    }
                    // Point 3 is not between Point 1 and 2
                    else {
                        if (angle2 > angle1) {
                            arcPos1 = Sketcher::PointPos::end;
                            arcPos2 = Sketcher::PointPos::start;
                        }
                        else {
                            arcPos1 = Sketcher::PointPos::start;
                            arcPos2 = Sketcher::PointPos::end;
                        }
                        startAngle = std::max(angle1, angle2);
                        endAngle = std::min(angle1, angle2);
                        arcAngle = 2 * M_PI - (startAngle - endAngle);
                    }
                }

                CreateAndDrawShapeGeometry();

                if (constructionMethod() == ConstructionMethod::Center) {
                    startAngle = startAngleBackup;
                }

                if (constructionMethod() == ConstructionMethod::Center) {
                    toolWidgetManager.drawDoubleAtCursor(onSketchPos, arcAngle, Base::Unit::Angle);
                    if (seekAutoConstraint(sugConstraints[2],
                                           onSketchPos,
                                           Base::Vector2d(0.0, 0.0))) {
                        renderSuggestConstraintsCursor(sugConstraints[2]);
                        return;
                    }
                }
                else {
                    toolWidgetManager.drawPositionAtCursor(onSketchPos);
                    if (seekAutoConstraint(sugConstraints[2],
                                           onSketchPos,
                                           Base::Vector2d(0.f, 0.f),
                                           AutoConstraint::CURVE)) {
                        renderSuggestConstraintsCursor(sugConstraints[2]);
                        return;
                    }
                }

            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {

        if (constructionMethod() == ConstructionMethod::Center) {
            if (arcAngle > 0) {
                endAngle = startAngle + arcAngle;
            }
            else {
                endAngle = startAngle;
                startAngle += arcAngle;
            }
        }

        try {
            createShape(false);

            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc"));

            commandAddShapeGeometryAndConstraints();

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception&) {
            /*Gui::NotifyError(sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to add arc"));*/

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
        int ArcGeoId = getHighestCurveIndex();

        auto& ac1 = sugConstraints[0];
        auto& ac2 = sugConstraints[1];
        auto& ac3 = sugConstraints[2];

        if (constructionMethod() == ConstructionMethod::Center) {
            generateAutoConstraintsOnElement(
                ac1,
                ArcGeoId,
                Sketcher::PointPos::mid);  // add auto constraints for the center point
            generateAutoConstraintsOnElement(ac2,
                                             ArcGeoId,
                                             (arcAngle > 0) ? Sketcher::PointPos::start
                                                            : Sketcher::PointPos::end);
            generateAutoConstraintsOnElement(ac3,
                                             ArcGeoId,
                                             (arcAngle > 0) ? Sketcher::PointPos::end
                                                            : Sketcher::PointPos::start);
        }
        else {
            generateAutoConstraintsOnElement(
                ac1,
                ArcGeoId,
                arcPos1);  // add auto constraints for the second picked point
            generateAutoConstraintsOnElement(
                ac2,
                ArcGeoId,
                arcPos2);  // add auto constraints for thesecond picked point
            generateAutoConstraintsOnElement(
                ac3,
                ArcGeoId,
                Sketcher::PointPos::none);  // add auto constraints for the third picked point
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
        return "DSH_Arc";
    }

    QString getCrosshairCursorSVGName() const override
    {
        if (constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
            return QString::fromLatin1("Sketcher_Pointer_Create_Arc");
        }
        else {  // constructionMethod == DrawSketchHandlerArc::ConstructionMethod::ThreeRim
            return QString::fromLatin1("Sketcher_Pointer_Create_3PointArc");
        }

        return QStringLiteral("None");
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
        return Gui::BitmapFactory().pixmap("Sketcher_CreateArc");
    }

    QString getToolWidgetText() const override
    {
        return QString(QObject::tr("Arc parameters"));
    }

    bool canGoToNextMode() override
    {
        if (state() == SelectMode::SeekSecond && radius < Precision::Confusion()) {
            // Prevent validation of null arc.
            return false;
        }
        return true;
    }

    void angleSnappingControl() override
    {
        if (constructionMethod() == ConstructionMethod::Center
            && (state() == SelectMode::SeekSecond || state() == SelectMode::SeekThird)) {
            setAngleSnapping(true, centerPoint);
        }
        else {
            setAngleSnapping(false);
        }
    }

    void createShape(bool onlyeditoutline) override
    {

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

            addArcToShapeGeometry(toVector3d(centerPoint),
                                  startAngle,
                                  endAngle,
                                  radius,
                                  isConstructionMode());
        }

        if (onlyeditoutline) {
            if (constructionMethod() == ConstructionMethod::Center) {
                if (state() == SelectMode::SeekThird) {
                    const double scale = 0.8;
                    addLineToShapeGeometry(
                        toVector3d(centerPoint),
                        Base::Vector3d(centerPoint.x + cos(startAngle) * scale * radius,
                                       centerPoint.y + sin(startAngle) * scale * radius,
                                       0.),
                        isConstructionMode());

                    addLineToShapeGeometry(
                        toVector3d(centerPoint),
                        Base::Vector3d(centerPoint.x + cos(endAngle) * scale * radius,
                                       centerPoint.y + sin(endAngle) * scale * radius,
                                       0.),
                        isConstructionMode());
                }
            }
            else {
                if (state() == SelectMode::SeekSecond) {
                    addLineToShapeGeometry(toVector3d(firstPoint),
                                           toVector3d(secondPoint),
                                           isConstructionMode());
                }
                else if (state() == SelectMode::SeekThird) {
                    const double scale = 0.8;
                    addLineToShapeGeometry(toVector3d(centerPoint),
                                           toVector3d(centerPoint)
                                               + (toVector3d(secondPoint) - toVector3d(centerPoint))
                                                   * scale,
                                           isConstructionMode());

                    addLineToShapeGeometry(toVector3d(centerPoint),
                                           toVector3d(centerPoint)
                                               + (toVector3d(firstPoint) - toVector3d(centerPoint))
                                                   * scale,
                                           isConstructionMode());
                }
            }
        }
    }

private:
    Base::Vector2d centerPoint, firstPoint, secondPoint;
    double radius, startAngle, endAngle, arcAngle;
    Sketcher::PointPos arcPos1, arcPos2;
};

template<>
auto DSHArcControllerBase::getState(int labelindex) const
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
        default:
            THROWM(Base::ValueError, "OnViewParameter index without an associated machine state")
    }
}

template<>
void DSHArcController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        QStringList names = {QApplication::translate("Sketcher_CreateArc", "Center"),
                             QApplication::translate("Sketcher_CreateArc", "3 rim points")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        if (isConstructionMode()) {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc_Constr"));
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointArc_Constr"));
        }
        else {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc"));
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointArc"));
        }
    }

    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);

    if (handler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::ThreeRim) {
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
            Gui::SoDatumLabel::ANGLE,
            Gui::EditableDatumLabel::Function::Dimensioning);
    }
}

template<>
void DSHArcControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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
            if (handler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
                Base::Vector2d dir = onSketchPos - handler->centerPoint;
                if (dir.Length() < Precision::Confusion()) {
                    dir.x = 1.0;  // if direction null, default to (1,0)
                }
                double radius = dir.Length();

                if (onViewParameters[OnViewParameter::Third]->isSet) {
                    radius = onViewParameters[OnViewParameter::Third]->getValue();
                    if (radius < Precision::Confusion()) {
                        unsetOnViewParameter(onViewParameters[OnViewParameter::Third].get());
                        return;
                    }

                    onSketchPos = handler->centerPoint + radius * dir.Normalize();
                }

                if (onViewParameters[OnViewParameter::Fourth]->isSet) {
                    double angle =
                        Base::toRadians(onViewParameters[OnViewParameter::Fourth]->getValue());
                    onSketchPos.x = handler->centerPoint.x + cos(angle) * radius;
                    onSketchPos.y = handler->centerPoint.y + sin(angle) * radius;
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
        case SelectMode::SeekThird: {
            if (handler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
                if (onViewParameters[OnViewParameter::Fifth]->isSet) {
                    double arcAngle =
                        Base::toRadians(onViewParameters[OnViewParameter::Fifth]->getValue());
                    if (fmod(fabs(arcAngle), 2 * M_PI) < Precision::Confusion()) {
                        unsetOnViewParameter(onViewParameters[OnViewParameter::Fifth].get());
                        return;
                    }
                    double angle = handler->startAngle + arcAngle;
                    onSketchPos.x = handler->centerPoint.x + cos(angle) * handler->radius;
                    onSketchPos.y = handler->centerPoint.y + sin(angle) * handler->radius;
                }
            }
            else {
                if (onViewParameters[OnViewParameter::Fifth]->isSet) {
                    onSketchPos.x = onViewParameters[OnViewParameter::Fifth]->getValue();
                }

                if (onViewParameters[OnViewParameter::Sixth]->isSet) {
                    onSketchPos.y = onViewParameters[OnViewParameter::Sixth]->getValue();
                }
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHArcController::adaptParameters(Base::Vector2d onSketchPos)
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
            if (handler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {

                if (!onViewParameters[OnViewParameter::Third]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Third, handler->radius);
                }
                double range = Base::toDegrees(handler->startAngle);
                if (!onViewParameters[OnViewParameter::Fourth]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Fourth, range, Base::Unit::Angle);
                }

                Base::Vector3d start = toVector3d(handler->centerPoint);
                Base::Vector3d end = toVector3d(onSketchPos);

                onViewParameters[OnViewParameter::Third]->setPoints(start, end);
                onViewParameters[OnViewParameter::Fourth]->setPoints(start, Base::Vector3d());
                onViewParameters[OnViewParameter::Fourth]->setLabelRange(handler->startAngle);
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
            if (handler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
                double range = Base::toDegrees(handler->arcAngle);

                if (!onViewParameters[OnViewParameter::Fifth]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Fifth, range, Base::Unit::Angle);
                }

                Base::Vector3d start = toVector3d(handler->centerPoint);
                onViewParameters[OnViewParameter::Fifth]->setPoints(start, Base::Vector3d());

                onViewParameters[OnViewParameter::Fifth]->setLabelStartAngle(handler->startAngle);
                onViewParameters[OnViewParameter::Fifth]->setLabelRange(handler->arcAngle);
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
void DSHArcController::doChangeDrawSketchHandlerMode()
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
            if (handler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
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
void DSHArcController::addConstraints()
{
    App::DocumentObject* obj = handler->sketchgui->getObject();

    int firstCurve = handler->getHighestCurveIndex();
    using namespace Sketcher;

    auto x0 = onViewParameters[OnViewParameter::First]->getValue();
    auto y0 = onViewParameters[OnViewParameter::Second]->getValue();
    auto p3 = onViewParameters[OnViewParameter::Third]->getValue();
    auto p4 = onViewParameters[OnViewParameter::Fourth]->getValue();

    auto x0set = onViewParameters[OnViewParameter::First]->isSet;
    auto y0set = onViewParameters[OnViewParameter::Second]->isSet;
    auto p3set = onViewParameters[OnViewParameter::Third]->isSet;
    auto p4set = onViewParameters[OnViewParameter::Fourth]->isSet;
    auto p5set = onViewParameters[OnViewParameter::Fifth]->isSet;


    PointPos pos1 =
        handler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center
        ? PointPos::mid
        : handler->arcPos1;

    auto constraintx0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, pos1), GeoElementId::VAxis, x0, obj);
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, pos1), GeoElementId::HAxis, y0, obj);
    };

    auto constraintp3radius = [&]() {
        Gui::cmdAppObjectArgs(obj,
                              "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                              firstCurve,
                              fabs(p3));
    };

    auto constraintp5angle = [&]() {
        Gui::cmdAppObjectArgs(obj,
                              "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
                              firstCurve,
                              fabs(handler->arcAngle));
    };

    auto constraintp3x = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, handler->arcPos2),
                               GeoElementId::VAxis,
                               p3,
                               obj);
    };

    auto constraintp4y = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, handler->arcPos2),
                               GeoElementId::HAxis,
                               p4,
                               obj);
    };


    if (handler->AutoConstraints.empty()) {  // No valid diagnosis. Every constraint can be added.
        if (x0set && y0set && x0 == 0. && y0 == 0.) {
            ConstraintToAttachment(GeoElementId(firstCurve, pos1), GeoElementId::RtPnt, 0., obj);
        }
        else {
            if (x0set) {
                constraintx0();
            }

            if (y0set) {
                constrainty0();
            }
        }

        if (handler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
            if (p3set) {
                constraintp3radius();
            }

            if (p5set) {
                constraintp5angle();
            }
        }
        else {
            if (p3set && p4set && p3 == 0. && p4 == 0.) {
                ConstraintToAttachment(GeoElementId(firstCurve, handler->arcPos2),
                                       GeoElementId::RtPnt,
                                       0.,
                                       obj);
            }
            else {
                if (p3set) {
                    constraintp3x();
                }

                if (p4set) {
                    constraintp4y();
                }
            }
        }
    }
    else {  // Valid diagnosis. Must check which constraints may be added.
        auto startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, pos1));

        if (x0set && startpointinfo.isXDoF()) {
            constraintx0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition
            // get updated point position
            startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, pos1));
        }

        if (y0set && startpointinfo.isYDoF()) {
            constrainty0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition
            // get updated point position
            startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, pos1));
        }


        if (handler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
            startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::start));
            auto endpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::end));
            auto midpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));

            int DoFs = startpointinfo.getDoFs();
            DoFs += endpointinfo.getDoFs();
            DoFs += midpointinfo.getDoFs();

            if (p3set && DoFs > 0) {
                constraintp3radius();
                DoFs--;
            }

            if (p5set && DoFs > 0) {
                constraintp5angle();
            }
        }
        else {
            auto endpointinfo = handler->getPointInfo(GeoElementId(firstCurve, handler->arcPos2));

            if (p3set && endpointinfo.isXDoF()) {
                constraintp3x();

                handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters
                                                         // after each constraint addition
                                                         // get updated point position
                startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, handler->arcPos1));
                endpointinfo = handler->getPointInfo(GeoElementId(firstCurve, handler->arcPos2));
            }

            if (p4set && endpointinfo.isYDoF()) {
                constraintp4y();
            }
        }
    }
}

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerArc_H

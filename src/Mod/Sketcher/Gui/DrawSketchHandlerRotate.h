/***************************************************************************
 *   Copyright (c) 2022 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>   *
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


#ifndef SKETCHERGUI_DrawSketchHandlerRotate_H
#define SKETCHERGUI_DrawSketchHandlerRotate_H

#include <QApplication>

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

using namespace Sketcher;

namespace SketcherGui
{

class DrawSketchHandlerRotate;

using DSHRotateController =
    DrawSketchDefaultWidgetController<DrawSketchHandlerRotate,
                                      StateMachines::ThreeSeekEnd,
                                      /*PAutoConstraintSize =*/0,
                                      /*OnViewParametersT =*/OnViewParameters<4>,
                                      /*WidgetParametersT =*/WidgetParameters<1>,
                                      /*WidgetCheckboxesT =*/WidgetCheckboxes<1>,
                                      /*WidgetComboboxesT =*/WidgetComboboxes<0>>;

using DSHRotateControllerBase = DSHRotateController::ControllerBase;

using DrawSketchHandlerRotateBase = DrawSketchControllableHandler<DSHRotateController>;

class DrawSketchHandlerRotate: public DrawSketchHandlerRotateBase
{
    friend DSHRotateController;
    friend DSHRotateControllerBase;

public:
    explicit DrawSketchHandlerRotate(std::vector<int> listOfGeoIds)
        : listOfGeoIds(listOfGeoIds)
        , deleteOriginal(false)
        , cloneConstraints(false)
        , length(0.0)
        , startAngle(0.0)
        , endAngle(0.0)
        , totalAngle(0.0)
        , individualAngle(0.0)
        , numberOfCopies(0)
    {}

    DrawSketchHandlerRotate(const DrawSketchHandlerRotate&) = delete;
    DrawSketchHandlerRotate(DrawSketchHandlerRotate&&) = delete;
    DrawSketchHandlerRotate& operator=(const DrawSketchHandlerRotate&) = delete;
    DrawSketchHandlerRotate& operator=(DrawSketchHandlerRotate&&) = delete;


    ~DrawSketchHandlerRotate() override = default;

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                centerPoint = onSketchPos;
            } break;
            case SelectMode::SeekSecond: {
                length = (onSketchPos - centerPoint).Length();
                startAngle = (onSketchPos - centerPoint).Angle();

                startPoint = onSketchPos;

                CreateAndDrawShapeGeometry();
            } break;
            case SelectMode::SeekThird: {
                endAngle = (onSketchPos - centerPoint).Angle();
                endpoint = centerPoint + length * Base::Vector2d(cos(endAngle), sin(endAngle));

                double angle1 = endAngle - startAngle;
                double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
                totalAngle = abs(angle1 - totalAngle) < abs(angle2 - totalAngle) ? angle1 : angle2;

                CreateAndDrawShapeGeometry();
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Rotate geometries"));

            createShape(false);

            commandAddShapeGeometryAndConstraints();

            if (deleteOriginal) {
                deleteOriginalGeos();
            }

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            e.ReportException();
            Gui::NotifyError(sketchgui,
                             QT_TRANSLATE_NOOP("Notifications", "Error"),
                             QT_TRANSLATE_NOOP("Notifications", "Failed to rotate"));

            Gui::Command::abortCommand();
            THROWM(Base::RuntimeError,
                   QT_TRANSLATE_NOOP(
                       "Notifications",
                       "Tool execution aborted") "\n")  // This prevents constraints from being
                                                        // applied on non existing geometry
        }
    }

    void createAutoConstraints() override
    {
        // none
    }

    std::string getToolName() const override
    {
        return "DSH_Rotate";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Create_Rotate");
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
        return Gui::BitmapFactory().pixmap("Sketcher_Rotate");
    }

    QString getToolWidgetText() const override
    {
        return QString(QObject::tr("Rotate parameters"));
    }

    void activated() override
    {
        DrawSketchDefaultHandler::activated();
        continuousMode = false;
    }

    bool canGoToNextMode() override
    {
        if (state() == SelectMode::SeekThird && fabs(totalAngle) < Precision::Confusion()) {
            // Prevent validation rotation of 0deg.
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

private:
    std::vector<int> listOfGeoIds;
    Base::Vector2d centerPoint, startPoint, endpoint;

    bool deleteOriginal, cloneConstraints;
    double length, startAngle, endAngle, totalAngle, individualAngle;
    int numberOfCopies;

    void deleteOriginalGeos()
    {
        std::stringstream stream;
        for (size_t j = 0; j < listOfGeoIds.size() - 1; j++) {
            stream << listOfGeoIds[j] << ",";
        }
        stream << listOfGeoIds[listOfGeoIds.size() - 1];
        try {
            Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                  "delGeometries([%s])",
                                  stream.str().c_str());
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("%s\n", e.what());
        }
    }

    void createShape(bool onlyeditoutline) override
    {
        SketchObject* Obj = sketchgui->getSketchObject();

        ShapeGeometry.clear();

        if (state() == SelectMode::SeekSecond) {
            if (length > Precision::Confusion()) {
                addLineToShapeGeometry(toVector3d(centerPoint),
                                       toVector3d(startPoint),
                                       isConstructionMode());
            }
            return;
        }

        int numberOfCopiesToMake = numberOfCopies;
        if (numberOfCopies == 0) {
            numberOfCopiesToMake = 1;
            deleteOriginal = true;
        }
        else {
            deleteOriginal = false;
        }

        individualAngle = totalAngle / numberOfCopiesToMake;

        for (int i = 1; i <= numberOfCopiesToMake; i++) {
            for (auto& geoId : listOfGeoIds) {
                const Part::Geometry* pGeo = Obj->getGeometry(geoId);
                auto geoUniquePtr = std::unique_ptr<Part::Geometry>(pGeo->copy());
                Part::Geometry* geo = geoUniquePtr.get();

                double angle = individualAngle * i;

                if (isCircle(*geo)) {
                    auto* circle = static_cast<Part::GeomCircle*>(geo);  // NOLINT
                    circle->setCenter(getRotatedPoint(circle->getCenter(), centerPoint, angle));
                }
                else if (isArcOfCircle(*geo)) {
                    auto* arcOfCircle = static_cast<Part::GeomArcOfCircle*>(geo);  // NOLINT
                    arcOfCircle->setCenter(
                        getRotatedPoint(arcOfCircle->getCenter(), centerPoint, angle));
                    double arcStartAngle, arcEndAngle;  // NOLINT
                    arcOfCircle->getRange(arcStartAngle, arcEndAngle, /*emulateCCWXY=*/true);
                    arcOfCircle->setRange(arcStartAngle + angle,
                                          arcEndAngle + angle,
                                          /*emulateCCWXY=*/true);
                }
                else if (isLineSegment(*geo)) {
                    auto* line = static_cast<Part::GeomLineSegment*>(geo);  // NOLINT
                    line->setPoints(getRotatedPoint(line->getStartPoint(), centerPoint, angle),
                                    getRotatedPoint(line->getEndPoint(), centerPoint, angle));
                }
                else if (isEllipse(*geo)) {
                    auto* ellipse = static_cast<Part::GeomEllipse*>(geo);  // NOLINT
                    ellipse->setCenter(getRotatedPoint(ellipse->getCenter(), centerPoint, angle));
                    ellipse->setMajorAxisDir(
                        getRotatedPoint(ellipse->getMajorAxisDir(), Base::Vector2d(0., 0.), angle));
                }
                else if (isArcOfEllipse(*geo)) {
                    auto* arcOfEllipse = static_cast<Part::GeomArcOfEllipse*>(geo);  // NOLINT
                    arcOfEllipse->setCenter(
                        getRotatedPoint(arcOfEllipse->getCenter(), centerPoint, angle));
                    arcOfEllipse->setMajorAxisDir(getRotatedPoint(arcOfEllipse->getMajorAxisDir(),
                                                                  Base::Vector2d(0., 0.),
                                                                  angle));
                }
                else if (isArcOfHyperbola(*geo)) {
                    auto* arcOfHyperbola = static_cast<Part::GeomArcOfHyperbola*>(geo);  // NOLINT
                    arcOfHyperbola->setCenter(
                        getRotatedPoint(arcOfHyperbola->getCenter(), centerPoint, angle));
                    arcOfHyperbola->setMajorAxisDir(
                        getRotatedPoint(arcOfHyperbola->getMajorAxisDir(),
                                        Base::Vector2d(0., 0.),
                                        angle));
                }
                else if (isArcOfParabola(*geo)) {
                    auto* arcOfParabola = static_cast<Part::GeomArcOfParabola*>(geo);  // NOLINT
                    arcOfParabola->setCenter(
                        getRotatedPoint(arcOfParabola->getCenter(), centerPoint, angle));
                    arcOfParabola->setAngleXU(arcOfParabola->getAngleXU() + angle);
                }
                else if (isBSplineCurve(*geo)) {
                    auto* bSpline = static_cast<Part::GeomBSplineCurve*>(geo);  // NOLINT
                    std::vector<Base::Vector3d> poles = bSpline->getPoles();
                    for (size_t p = 0; p < poles.size(); p++) {
                        poles[p] = getRotatedPoint(std::move(poles[p]), centerPoint, angle);
                    }
                    bSpline->setPoles(poles);
                }
                else if (isPoint(*geo)) {
                    auto* point = static_cast<Part::GeomPoint*>(geo);  // NOLINT
                    point->setPoint(getRotatedPoint(point->getPoint(), centerPoint, angle));
                }

                ShapeGeometry.emplace_back(std::move(geoUniquePtr));
            }
        }

        if (onlyeditoutline) {
            // Add the lines to show angle
            addLineToShapeGeometry(toVector3d(centerPoint), toVector3d(startPoint), true);

            addLineToShapeGeometry(toVector3d(centerPoint), toVector3d(endpoint), true);
        }
        else {
            int firstCurveCreated = getHighestCurveIndex() + 1;
            size_t size = listOfGeoIds.size();
            using namespace Sketcher;

            const std::vector<Constraint*>& vals = Obj->Constraints.getValues();
            // avoid applying equal several times if cloning distanceX and distanceY of the
            // same part.
            std::vector<int> geoIdsWhoAlreadyHasEqual = {};

            for (auto& cstr : vals) {
                int firstIndex = indexOfGeoId(listOfGeoIds, cstr->First);
                int secondIndex = indexOfGeoId(listOfGeoIds, cstr->Second);
                int thirdIndex = indexOfGeoId(listOfGeoIds, cstr->Third);

                for (int i = 0; i < numberOfCopiesToMake; i++) {
                    int firstIndexi = firstCurveCreated + firstIndex + static_cast<int>(size) * i;
                    int secondIndexi = firstCurveCreated + secondIndex + static_cast<int>(size) * i;
                    int thirdIndexi = firstCurveCreated + thirdIndex + static_cast<int>(size) * i;

                    auto newConstr = std::unique_ptr<Constraint>(cstr->copy());
                    newConstr->First = firstIndexi;

                    if ((cstr->Type == Symmetric || cstr->Type == Tangent
                         || cstr->Type == Perpendicular || cstr->Type == Angle)
                        && firstIndex >= 0 && secondIndex >= 0 && thirdIndex >= 0) {
                        newConstr->Second = secondIndexi;
                        newConstr->Third = thirdIndexi;
                    }
                    else if ((cstr->Type == Coincident || cstr->Type == Tangent
                              || cstr->Type == Symmetric || cstr->Type == Perpendicular
                              || cstr->Type == Parallel || cstr->Type == Equal
                              || cstr->Type == Angle || cstr->Type == PointOnObject
                              || cstr->Type == InternalAlignment)
                             && firstIndex >= 0 && secondIndex >= 0
                             && thirdIndex == GeoEnum::GeoUndef) {
                        newConstr->Second = secondIndexi;
                    }
                    else if ((cstr->Type == Radius || cstr->Type == Diameter
                              || cstr->Type == Weight)
                             && firstIndex >= 0) {
                        if (deleteOriginal || !cloneConstraints) {
                            newConstr->setValue(cstr->getValue());
                        }
                        else {
                            newConstr->Type = Equal;
                            newConstr->First = cstr->First;
                            newConstr->Second = firstIndexi;
                        }
                    }
                    else if ((cstr->Type == Distance || cstr->Type == DistanceX
                              || cstr->Type == DistanceY)
                             && firstIndex >= 0 && secondIndex >= 0) {
                        if (!deleteOriginal && cloneConstraints
                            && cstr->First == cstr->Second) {  // only line distances
                            if (indexOfGeoId(geoIdsWhoAlreadyHasEqual, secondIndexi) != -1) {
                                continue;
                            }
                            newConstr->Type = Equal;
                            newConstr->First = cstr->First;
                            newConstr->Second = secondIndexi;
                            geoIdsWhoAlreadyHasEqual.push_back(secondIndexi);
                        }
                        else if (cstr->Type == Distance) {
                            newConstr->Second = secondIndexi;
                        }
                        else {
                            // We should be able to handle cases where rotation is 90 or 180, but
                            // this is segfaulting. The same is reported in
                            // SketchObject::addSymmetric. There's apparently a problem with
                            // creation of DistanceX/Y. On top of the segfault the DistanceX/Y flips
                            // the new geometry.
                            /*if (cstr->Type == DistanceX || cstr->Type == DistanceY) {
                                //DistanceX/Y can be applied only if the rotation if 90 or 180.
                                if (fabs(fmod(individualAngle, M_PI)) < Precision::Confusion()) {
                                    // ok and nothing to do actually
                                }
                                else if (fabs(fmod(individualAngle, M_PI * 0.5)) <
                            Precision::Confusion()) { cstr->Type = cstr->Type == DistanceX ?
                            DistanceY : DistanceX;
                                }
                                else {
                                    // cannot apply for random angles
                                    continue;
                                }
                            }*/
                            // So for now we just ignore all DistanceX/Y
                            continue;
                        }
                    }
                    else if ((cstr->Type == Block) && firstIndex >= 0) {
                        newConstr->First = firstIndexi;
                    }
                    else {
                        continue;
                    }

                    ShapeConstraints.push_back(std::move(newConstr));
                }
            }
        }
    }

    Base::Vector3d
    getRotatedPoint(Base::Vector3d&& pointToRotate, const Base::Vector2d& centerPoint, double angle)
    {
        Base::Vector2d pointToRotate2D = Base::Vector2d(pointToRotate.x, pointToRotate.y);

        double initialAngle = (pointToRotate2D - centerPoint).Angle();
        double lengthToCenter = (pointToRotate2D - centerPoint).Length();

        pointToRotate2D = centerPoint
            + lengthToCenter * Base::Vector2d(cos(angle + initialAngle), sin(angle + initialAngle));


        pointToRotate.x = pointToRotate2D.x;
        pointToRotate.y = pointToRotate2D.y;

        return pointToRotate;
    }
};

template<>
auto DSHRotateControllerBase::getState(int labelindex) const
{
    switch (labelindex) {
        case OnViewParameter::First:
        case OnViewParameter::Second:
            return SelectMode::SeekFirst;
            break;
        case OnViewParameter::Third:
            return SelectMode::SeekSecond;
            break;
        case OnViewParameter::Fourth:
            return SelectMode::SeekThird;
            break;
        default:
            THROWM(Base::ValueError, "OnViewParameter index without an associated machine state")
    }
}

template<>
void DSHRotateController::firstKeyShortcut()
{
    auto value = toolWidget->getParameter(WParameter::First);
    toolWidget->setParameterWithoutPassingFocus(WParameter::First, value + 1);
}

template<>
void DSHRotateController::secondKeyShortcut()
{
    auto value = toolWidget->getParameter(WParameter::First);
    if (value > 0.0) {
        toolWidget->setParameterWithoutPassingFocus(WParameter::First, value - 1);
    }
}

template<>
void DSHRotateController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        toolWidget->setCheckboxLabel(
            WCheckbox::FirstBox,
            QApplication::translate("TaskSketcherTool_c1_offset", "Apply equal constraints"));
        toolWidget->setCheckboxToolTip(
            WCheckbox::FirstBox,
            QString::fromLatin1("<p>")
                + QApplication::translate("TaskSketcherTool_c1_offset",
                                          "If this option is selected dimensional constraints are "
                                          "excluded from the operation.\n"
                                          "Instead equal constraints are applied between the "
                                          "original objects and their copies.")
                + QString::fromLatin1("</p>"));
    }

    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);
    onViewParameters[OnViewParameter::Third]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning);
    onViewParameters[OnViewParameter::Fourth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning);

    toolWidget->setParameterLabel(
        WParameter::First,
        QApplication::translate("TaskSketcherTool_p4_rotate", "Copies (+'U'/ -'J')"));
    toolWidget->setParameter(OnViewParameter::First, 0.0);
    toolWidget->configureParameterUnit(OnViewParameter::First, Base::Unit());
    toolWidget->configureParameterMin(OnViewParameter::First, 0.0);     // NOLINT
    toolWidget->configureParameterMax(OnViewParameter::First, 9999.0);  // NOLINT
    toolWidget->configureParameterDecimals(OnViewParameter::First, 0);
}

template<>
void DSHRotateController::adaptDrawingToParameterChange(int parameterindex, double value)
{
    switch (parameterindex) {
        case WParameter::First:
            handler->numberOfCopies = floor(abs(value));
            break;
    }
}

template<>
void DSHRotateController::adaptDrawingToCheckboxChange(int checkboxindex, bool value)
{
    switch (checkboxindex) {
        case WCheckbox::FirstBox: {
            handler->cloneConstraints = value;
        } break;
    }
}

template<>
void DSHRotateControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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
            if (onViewParameters[OnViewParameter::Third]->isSet) {

                double arcAngle =
                    Base::toRadians(onViewParameters[OnViewParameter::Third]->getValue());
                if (fmod(fabs(arcAngle), 2 * M_PI) < Precision::Confusion()) {
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Third].get());
                    return;
                }
                onSketchPos.x = handler->centerPoint.x + 1;
                onSketchPos.y = handler->centerPoint.y;
            }
        } break;
        case SelectMode::SeekThird: {
            if (onViewParameters[OnViewParameter::Fourth]->isSet) {

                double arcAngle =
                    Base::toRadians(onViewParameters[OnViewParameter::Fourth]->getValue());
                if (fmod(fabs(arcAngle), 2 * M_PI) < Precision::Confusion()) {
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Fourth].get());
                    return;
                }

                onSketchPos.x = handler->centerPoint.x + cos((handler->startAngle + arcAngle));
                onSketchPos.y = handler->centerPoint.y + sin((handler->startAngle + arcAngle));
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHRotateController::adaptParameters(Base::Vector2d onSketchPos)
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
            double range = Base::toDegrees(handler->startAngle);
            if (!onViewParameters[OnViewParameter::Third]->isSet) {
                setOnViewParameterValue(OnViewParameter::Third, range, Base::Unit::Angle);
            }

            Base::Vector3d start = toVector3d(handler->centerPoint);

            onViewParameters[OnViewParameter::Third]->setPoints(start, Base::Vector3d());
            onViewParameters[OnViewParameter::Third]->setLabelRange(handler->startAngle);
        } break;
        case SelectMode::SeekThird: {
            double range = Base::toDegrees(handler->totalAngle);

            if (!onViewParameters[OnViewParameter::Fourth]->isSet) {
                setOnViewParameterValue(OnViewParameter::Fourth, range, Base::Unit::Angle);
            }

            Base::Vector3d start = toVector3d(handler->centerPoint);
            onViewParameters[OnViewParameter::Fourth]->setPoints(start, Base::Vector3d());

            onViewParameters[OnViewParameter::Fourth]->setLabelStartAngle(handler->startAngle);
            onViewParameters[OnViewParameter::Fourth]->setLabelRange(handler->totalAngle);
        } break;
        default:
            break;
    }
}

template<>
void DSHRotateController::doChangeDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (onViewParameters[OnViewParameter::First]->isSet
                && onViewParameters[OnViewParameter::Second]->isSet) {

                handler->setState(SelectMode::SeekSecond);
            }
        } break;
        case SelectMode::SeekSecond: {
            if (onViewParameters[OnViewParameter::Third]->isSet) {
                handler->totalAngle =
                    Base::toRadians(onViewParameters[OnViewParameter::Third]->getValue());

                handler->setState(SelectMode::End);
            }
        } break;
        case SelectMode::SeekThird: {
            if (onViewParameters[OnViewParameter::Fourth]->isSet) {

                handler->setState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerRotate_H

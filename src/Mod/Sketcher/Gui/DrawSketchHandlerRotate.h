// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <algorithm>

#include <QApplication>

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"
#include "SketcherTransformationExpressionHelper.h"

#include "Utils.h"

using namespace Sketcher;

namespace SketcherGui
{

class DrawSketchHandlerRotate;

using DSHRotateController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerRotate,
    StateMachines::ThreeSeekEnd,
    /*PAutoConstraintSize =*/0,
    /*OnViewParametersT =*/OnViewParameters<4>,
    /*WidgetParametersT =*/WidgetParameters<1>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<2>,
    /*WidgetComboboxesT =*/WidgetComboboxes<0>,
    /*WidgetLineEditsT =*/WidgetLineEdits<0>>;

using DSHRotateControllerBase = DSHRotateController::ControllerBase;

using DrawSketchHandlerRotateBase = DrawSketchControllableHandler<DSHRotateController>;

class DrawSketchHandlerRotate: public DrawSketchHandlerRotateBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerRotate)

    friend DSHRotateController;
    friend DSHRotateControllerBase;

public:
    explicit DrawSketchHandlerRotate(std::vector<int> listOfGeoIds)
        : listOfGeoIds(listOfGeoIds)
        , deleteOriginal(false)
        , cloneConstraints(false)
        , symmetric(false)
        , length(0.0)
        , startAngle(0.0)
        , endAngle(0.0)
        , totalAngle(0.0)
        , individualAngle(0.0)
        , numberOfCopies(1)
    {}

    DrawSketchHandlerRotate(const DrawSketchHandlerRotate&) = delete;
    DrawSketchHandlerRotate(DrawSketchHandlerRotate&&) = delete;
    DrawSketchHandlerRotate& operator=(const DrawSketchHandlerRotate&) = delete;
    DrawSketchHandlerRotate& operator=(DrawSketchHandlerRotate&&) = delete;


    ~DrawSketchHandlerRotate() override = default;

    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;

        return Gui::lookupHints<SelectMode>(
            state(),
            {
                {.state = SelectMode::SeekFirst,
                 .hints =
                     {
                         {tr("%1 pick center point", "Sketcher Rotate: hint"), {MouseLeft}},
                     }},
                {.state = SelectMode::SeekSecond,
                 .hints =
                     {
                         {tr("%1 set start angle", "Sketcher Rotate: hint"), {MouseLeft}},
                     }},
                {.state = SelectMode::SeekThird,
                 .hints =
                     {
                         {tr("%1 set rotation angle", "Sketcher Rotate: hint"), {MouseLeft}},
                     }},
            });
    }

private:
    static double getFullTurn()
    {
        return 2 * std::numbers::pi;
    }

    static bool isOneFullTurn(double angle)
    {
        return std::abs(std::abs(angle) - getFullTurn()) < Precision::Angular();
    }

    static double clampToFullTurn(double angle)
    {
        if (angle > getFullTurn()) {
            return getFullTurn();
        }

        if (angle < -getFullTurn()) {
            return -getFullTurn();
        }

        return angle;
    }

    static double angleForShapeCreation(double angle)
    {
        if (std::abs(angle) < Precision::Confusion()) {
            return getFullTurn();
        }

        return clampToFullTurn(angle);
    }

    static double closestContinuousAngle(double angle, double previousAngle)
    {
        if (std::abs(angle) < Precision::Confusion()) {
            return 0.0;
        }

        double closest = angle;
        auto useIfCloser = [&](double candidate) {
            if (std::abs(candidate) <= getFullTurn()
                && std::abs(candidate - previousAngle) < std::abs(closest - previousAngle)) {
                closest = candidate;
            }
        };

        useIfCloser(angle + getFullTurn());
        useIfCloser(angle - getFullTurn());

        return closest;
    }

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

                double angle = endAngle - startAngle;
                totalAngle = closestContinuousAngle(angle, totalAngle);

                CreateAndDrawShapeGeometry();
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        try {
            openCommand(QT_TRANSLATE_NOOP("Command", "Rotate geometries"));

            expressionHelper.storeOriginalExpressions(sketchgui->getSketchObject(), listOfGeoIds);

            createShape(false);

            commandAddShapeGeometryAndConstraints();

            expressionHelper.copyExpressionsToNewConstraints(
                sketchgui->getSketchObject(),
                listOfGeoIds,
                ShapeGeometry.size(),
                listOfGeoIds.empty()
                    ? 0
                    : static_cast<int>(ShapeGeometry.size() / listOfGeoIds.size()),
                1
            );

            if (deleteOriginal) {
                deleteOriginalGeos();
            }

            commitCommand();
        }
        catch (const Base::Exception& e) {
            e.reportException();
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to rotate")
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
        return QStringLiteral("Sketcher_Pointer_Create_Rotate");
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
        return QString(tr("Rotate Parameters"));
    }

    void activated() override
    {
        DrawSketchDefaultHandler::activated();
        continuousMode = false;
    }

    bool canGoToNextMode() override
    {
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

    bool deleteOriginal, cloneConstraints, symmetric;
    double length, startAngle, endAngle, totalAngle, individualAngle;
    int numberOfCopies;

    SketcherTransformationExpressionHelper expressionHelper;

    void deleteOriginalGeos()
    {
        std::stringstream stream;
        for (size_t j = 0; j < listOfGeoIds.size() - 1; j++) {
            stream << listOfGeoIds[j] << ",";
        }
        stream << listOfGeoIds[listOfGeoIds.size() - 1];
        try {
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometries([%s])", stream.str().c_str());
        }
        catch (const Base::Exception& e) {
            Base::Console().error("%s\n", e.what());
        }
    }

    void createShape(bool onlyeditoutline) override
    {
        SketchObject* Obj = sketchgui->getSketchObject();

        ShapeGeometry.clear();

        if (state() == SelectMode::SeekSecond) {
            if (length > Precision::Confusion()) {
                addLineToShapeGeometry(
                    toVector3d(centerPoint),
                    toVector3d(startPoint),
                    isConstructionMode()
                );
            }
            return;
        }

        int numberOfElements = std::max(numberOfCopies, 1);
        bool transformOriginal = numberOfElements == 1;
        int numberOfCopiesToMake =
            transformOriginal ? 1 : (symmetric ? numberOfElements / 2 : numberOfElements - 1);
        deleteOriginal = transformOriginal || (symmetric && numberOfElements % 2 == 0);

        double shapeAngle = angleForShapeCreation(totalAngle);

        double angleDivisor = deleteOriginal && symmetric && !transformOriginal
            ? numberOfCopiesToMake - 0.5
            : numberOfCopiesToMake;
        if (numberOfCopiesToMake > 0 && isOneFullTurn(shapeAngle)) {
            // Full-turn patterns use the total element count to avoid duplicating the original.
            angleDivisor = transformOriginal ? 1 : numberOfElements;
        }

        individualAngle = shapeAngle / angleDivisor;

        std::vector<double> copyFactors;
        copyFactors.reserve(
            transformOriginal ? 1 : (symmetric ? 2 * numberOfCopiesToMake : numberOfCopiesToMake)
        );
        for (int i = 1; i <= numberOfCopiesToMake; i++) {
            copyFactors.push_back(transformOriginal ? i : (deleteOriginal ? i - 0.5 : i));
        }
        if (symmetric && !transformOriginal) {
            for (int i = 1; i <= numberOfCopiesToMake; i++) {
                copyFactors.push_back(deleteOriginal ? 0.5 - i : -i);
            }
        }

        for (double copyFactor : copyFactors) {
            for (auto& geoId : listOfGeoIds) {
                const Part::Geometry* pGeo = Obj->getGeometry(geoId);
                auto geoUniquePtr = std::unique_ptr<Part::Geometry>(pGeo->copy());
                Part::Geometry* geo = geoUniquePtr.get();

                if (!onlyeditoutline) {
                    geo->reverseIfReversed();  // make sure we don't have reversed conics
                }

                double angle = individualAngle * copyFactor;

                Base::Matrix4D matrix(toVector3d(centerPoint), Base::Vector3d(0, 0, 1), angle);
                geo->transform(matrix);

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

                for (size_t i = 0; i < copyFactors.size(); i++) {
                    int firstIndexi = firstCurveCreated + firstIndex + static_cast<int>(size * i);
                    int secondIndexi = firstCurveCreated + secondIndex + static_cast<int>(size * i);
                    int thirdIndexi = firstCurveCreated + thirdIndex + static_cast<int>(size * i);

                    auto newConstr = std::unique_ptr<Constraint>(cstr->copy());
                    newConstr->First = firstIndexi;

                    if ((cstr->Type == Symmetric || cstr->Type == Tangent
                         || cstr->Type == Perpendicular || cstr->Type == Angle)
                        && firstIndex >= 0 && secondIndex >= 0 && thirdIndex >= 0) {
                        newConstr->Second = secondIndexi;
                        newConstr->Third = thirdIndexi;
                    }
                    else if (
                        (cstr->Type == Coincident || cstr->Type == Tangent
                         || cstr->Type == Symmetric || cstr->Type == Perpendicular
                         || cstr->Type == Parallel || cstr->Type == Equal || cstr->Type == Angle
                         || cstr->Type == PointOnObject || cstr->Type == InternalAlignment)
                        && firstIndex >= 0 && secondIndex >= 0 && thirdIndex == GeoEnum::GeoUndef
                    ) {
                        newConstr->Second = secondIndexi;
                    }
                    else if (
                        (cstr->Type == Radius || cstr->Type == Diameter || cstr->Type == Weight)
                        && firstIndex >= 0
                    ) {
                        if (deleteOriginal || !cloneConstraints) {
                            newConstr->setValue(cstr->getValue());
                        }
                        else {
                            newConstr->Type = Equal;
                            newConstr->First = cstr->First;
                            newConstr->Second = firstIndexi;
                        }
                    }
                    else if (
                        (cstr->Type == Distance || cstr->Type == DistanceX || cstr->Type == DistanceY)
                        && firstIndex >= 0
                    ) {
                        if (!deleteOriginal && cloneConstraints
                            && (cstr->First == cstr->Second || secondIndex < 0)) {  // only line
                                                                                    // distances
                            if (indexOfGeoId(geoIdsWhoAlreadyHasEqual, firstIndexi) != -1) {
                                continue;
                            }
                            newConstr->Type = Equal;
                            newConstr->First = cstr->First;
                            newConstr->Second = firstIndexi;
                            geoIdsWhoAlreadyHasEqual.push_back(firstIndexi);
                        }
                        else if (cstr->Type == Distance) {
                            if (secondIndex >= 0) {
                                newConstr->Second = secondIndexi;
                            }
                        }
                        else {
                            // We should be able to handle cases where rotation is 90 or 180, but
                            // this is segfaulting. The same is reported in
                            // SketchObject::addSymmetric. There's apparently a problem with
                            // creation of DistanceX/Y. On top of the segfault the DistanceX/Y flips
                            // the new geometry.
                            /*if (cstr->Type == DistanceX || cstr->Type == DistanceY) {
                                //DistanceX/Y can be applied only if the rotation if 90 or 180.
                                if (fabs(fmod(individualAngle, std::numbers::pi)) <
                            Precision::Confusion()) {
                                    // ok and nothing to do actually
                                }
                                else if (fabs(fmod(individualAngle, std::numbers::pi * 0.5)) <
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
    if (value > 1.0) {
        toolWidget->setParameterWithoutPassingFocus(WParameter::First, value - 1);
    }
}

template<>
void DSHRotateController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        toolWidget->setCheckboxLabel(
            WCheckbox::FirstBox,
            QApplication::translate("TaskSketcherTool_c1_offset", "Apply equal constraints")
        );
        toolWidget->setCheckboxToolTip(
            WCheckbox::FirstBox,
            QStringLiteral("<p>")
                + QApplication::translate(
                    "TaskSketcherTool_c1_offset",
                    "If this option is selected dimensional constraints are "
                    "excluded from the operation.\n"
                    "Instead equal constraints are applied between the "
                    "original objects and their copies."
                )
                + QStringLiteral("</p>")
        );
        toolWidget->setCheckboxLabel(
            WCheckbox::SecondBox,
            QApplication::translate("TaskSketcherTool_c2_rotate", "Symmetric")
        );
        toolWidget->setCheckboxToolTip(
            WCheckbox::SecondBox,
            QApplication::translate(
                "TaskSketcherTool_c2_rotate",
                "Distribute the elements symmetrically around the original position."
            )
        );
    }

    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);
    onViewParameters[OnViewParameter::Third]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
    onViewParameters[OnViewParameter::Fourth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );

    toolWidget->setParameterLabel(
        WParameter::First,
        QApplication::translate("TaskSketcherTool_p4_rotate", "Elements (+'U'/ -'J')")
    );
    toolWidget->setParameter(OnViewParameter::First, 1.0);
    toolWidget->configureParameterUnit(OnViewParameter::First, Base::Unit());
    toolWidget->configureParameterMin(OnViewParameter::First, 1.0);     // NOLINT
    toolWidget->configureParameterMax(OnViewParameter::First, 9999.0);  // NOLINT
    toolWidget->configureParameterDecimals(OnViewParameter::First, 0);
}

template<>
void DSHRotateController::adaptDrawingToParameterChange(int parameterindex, double value)
{
    switch (parameterindex) {
        case WParameter::First:
            handler->numberOfCopies = std::max(1, static_cast<int>(floor(abs(value))));
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
        case WCheckbox::SecondBox: {
            handler->symmetric = value;
        } break;
    }
}

template<>
void DSHRotateControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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

            if (thirdParam->isSet) {
                double arcAngle = Base::toRadians(thirdParam->getValue());
                double clampedAngle = DrawSketchHandlerRotate::clampToFullTurn(arcAngle);
                if (clampedAngle != arcAngle) {
                    setOnViewParameterValue(
                        OnViewParameter::Third,
                        Base::toDegrees(clampedAngle),
                        Base::Unit::Angle
                    );
                }

                onSketchPos.x = handler->centerPoint.x + 1;
                onSketchPos.y = handler->centerPoint.y;
            }
        } break;
        case SelectMode::SeekThird: {
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            if (fourthParam->isSet) {
                double arcAngle = Base::toRadians(fourthParam->getValue());
                double clampedAngle = DrawSketchHandlerRotate::clampToFullTurn(arcAngle);
                if (clampedAngle != arcAngle) {
                    setOnViewParameterValue(
                        OnViewParameter::Fourth,
                        Base::toDegrees(clampedAngle),
                        Base::Unit::Angle
                    );
                    arcAngle = clampedAngle;
                }

                handler->totalAngle = arcAngle;

                onSketchPos.x = handler->centerPoint.x + cos(handler->startAngle + arcAngle);
                onSketchPos.y = handler->centerPoint.y + sin(handler->startAngle + arcAngle);
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

            double range = Base::toDegrees(handler->startAngle);
            if (!thirdParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Third, range, Base::Unit::Angle);
            }

            Base::Vector3d start = toVector3d(handler->centerPoint);

            thirdParam->setPoints(start, Base::Vector3d());
            thirdParam->setLabelRange(handler->startAngle);
        } break;
        case SelectMode::SeekThird: {
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            double range = Base::toDegrees(handler->totalAngle);

            if (!fourthParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Fourth, range, Base::Unit::Angle);
            }

            Base::Vector3d start = toVector3d(handler->centerPoint);
            fourthParam->setPoints(start, Base::Vector3d());

            fourthParam->setLabelStartAngle(handler->startAngle);
            fourthParam->setLabelRange(handler->totalAngle);
        } break;
        default:
            break;
    }
}

template<>
void DSHRotateController::computeNextDrawSketchHandlerMode()
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

            if (thirdParam->hasFinishedEditing) {
                handler->totalAngle = DrawSketchHandlerRotate::clampToFullTurn(
                    Base::toRadians(thirdParam->getValue())
                );
                handler->setNextState(SelectMode::End);
            }
        } break;
        case SelectMode::SeekThird: {
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            if (fourthParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}


}  // namespace SketcherGui

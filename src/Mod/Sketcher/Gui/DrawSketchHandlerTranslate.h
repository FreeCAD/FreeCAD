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

#include <QApplication>
#include <map>

#include <Base/Tools.h>

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"
#include "SketcherTransformationExpressionHelper.h"

#include "GeometryCreationMode.h"
#include "Utils.h"


using namespace Sketcher;

namespace SketcherGui
{

class DrawSketchHandlerTranslate;

using DSHTranslateController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerTranslate,
    StateMachines::ThreeSeekEnd,
    /*PAutoConstraintSize =*/0,
    /*OnViewParametersT =*/OnViewParameters<6>,
    /*WidgetParametersT =*/WidgetParameters<2>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<1>,
    /*WidgetComboboxesT =*/WidgetComboboxes<0>>;

using DSHTranslateControllerBase = DSHTranslateController::ControllerBase;

using DrawSketchHandlerTranslateBase = DrawSketchControllableHandler<DSHTranslateController>;

class DrawSketchHandlerTranslate: public DrawSketchHandlerTranslateBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerTranslate)

    friend DSHTranslateController;
    friend DSHTranslateControllerBase;

public:
    explicit DrawSketchHandlerTranslate(std::vector<int> listOfGeoIds)
        : listOfGeoIds(listOfGeoIds)
        , deleteOriginal(false)
        , cloneConstraints(false)
        , numberOfCopies(0)
        , secondNumberOfCopies(1)
    {}

    DrawSketchHandlerTranslate(const DrawSketchHandlerTranslate&) = delete;
    DrawSketchHandlerTranslate(DrawSketchHandlerTranslate&&) = delete;
    DrawSketchHandlerTranslate& operator=(const DrawSketchHandlerTranslate&) = delete;
    DrawSketchHandlerTranslate& operator=(DrawSketchHandlerTranslate&&) = delete;

    ~DrawSketchHandlerTranslate() override = default;

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                referencePoint = onSketchPos;
            } break;
            case SelectMode::SeekSecond: {
                firstTranslationPoint = onSketchPos;

                firstTranslationVector = toVector3d(firstTranslationPoint - referencePoint);

                CreateAndDrawShapeGeometry();
            } break;
            case SelectMode::SeekThird: {
                secondTranslationPoint = onSketchPos;

                secondTranslationVector = toVector3d(secondTranslationPoint - referencePoint);

                CreateAndDrawShapeGeometry();
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Translate geometries"));

            expressionHelper.storeOriginalExpressions(sketchgui->getSketchObject(), listOfGeoIds);

            createShape(false);

            commandAddShapeGeometryAndConstraints();

            expressionHelper.copyExpressionsToNewConstraints(
                sketchgui->getSketchObject(),
                listOfGeoIds,
                ShapeGeometry.size(),
                numberOfCopies,
                secondNumberOfCopies
            );

            if (deleteOriginal) {
                deleteOriginalGeos();
            }

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            e.reportException();
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to translate")
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

    void createAutoConstraints() override
    {
        // none
    }

    std::string getToolName() const override
    {
        return "DSH_Translate";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Create_Translate");
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
        return Gui::BitmapFactory().pixmap("Sketcher_Translate");
    }

    QString getToolWidgetText() const override
    {
        return QString(tr("Translate Parameters"));
    }

    void onButtonPressed(Base::Vector2d onSketchPos) override
    {
        this->updateDataAndDrawToPosition(onSketchPos);

        if (state() == SelectMode::SeekSecond && secondNumberOfCopies == 1) {
            setState(SelectMode::End);
        }
        else {
            this->moveToNextMode();
        }
    }

    void activated() override
    {
        DrawSketchDefaultHandler::activated();
        continuousMode = false;
    }

    bool canGoToNextMode() override
    {
        if (state() == SelectMode::SeekSecond
            && firstTranslationVector.Length() < Precision::Confusion()) {
            // Prevent validation of null translation.
            return false;
        }
        if (state() == SelectMode::SeekThird
            && secondTranslationVector.Length() < Precision::Confusion() && secondNumberOfCopies > 1) {
            return false;
        }
        return true;
    }

    void angleSnappingControl() override
    {
        if (state() == SelectMode::SeekSecond || state() == SelectMode::SeekThird) {
            setAngleSnapping(true, referencePoint);
        }

        else {
            setAngleSnapping(false);
        }
    }

private:
    std::vector<int> listOfGeoIds;
    Base::Vector2d referencePoint, firstTranslationPoint, secondTranslationPoint;
    Base::Vector3d firstTranslationVector, secondTranslationVector;

    bool deleteOriginal, cloneConstraints;
    int numberOfCopies, secondNumberOfCopies;

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
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        ShapeGeometry.clear();

        int numberOfCopiesToMake = numberOfCopies;
        if (numberOfCopies == 0) {
            numberOfCopiesToMake = 1;
            deleteOriginal = 1;
        }
        else {
            deleteOriginal = 0;
        }

        for (int k = 0; k < secondNumberOfCopies; k++) {
            for (int i = 0; i <= numberOfCopiesToMake; i++) {
                if ((k == 0 && i == 0)) {
                    continue;
                }
                Base::Vector3d vec = firstTranslationVector * i + secondTranslationVector * k;

                for (auto& geoId : listOfGeoIds) {
                    const Part::Geometry* pGeo = Obj->getGeometry(geoId);
                    auto geoUniquePtr = std::unique_ptr<Part::Geometry>(pGeo->copy());
                    Part::Geometry* geo = geoUniquePtr.get();

                    if (isCircle(*geo)) {
                        Part::GeomCircle* circle = static_cast<Part::GeomCircle*>(geo);  // NOLINT
                        circle->setCenter(circle->getCenter() + vec);
                    }
                    else if (isArcOfCircle(*geo)) {
                        Part::GeomArcOfCircle* arc = static_cast<Part::GeomArcOfCircle*>(geo);  // NOLINT
                        arc->setCenter(arc->getCenter() + vec);
                    }
                    else if (isEllipse(*geo)) {
                        Part::GeomEllipse* ellipse = static_cast<Part::GeomEllipse*>(geo);  // NOLINT
                        ellipse->setCenter(ellipse->getCenter() + vec);
                    }
                    else if (isArcOfEllipse(*geo)) {
                        Part::GeomArcOfEllipse* aoe = static_cast<Part::GeomArcOfEllipse*>(geo);  // NOLINT
                        aoe->setCenter(aoe->getCenter() + vec);
                    }
                    else if (isArcOfHyperbola(*geo)) {
                        Part::GeomArcOfHyperbola* aoh = static_cast<Part::GeomArcOfHyperbola*>(
                            geo
                        );  // NOLINT
                        aoh->setCenter(aoh->getCenter() + vec);
                    }
                    else if (isArcOfParabola(*geo)) {
                        Part::GeomArcOfParabola* aop = static_cast<Part::GeomArcOfParabola*>(geo);  // NOLINT
                        aop->setCenter(aop->getCenter() + vec);
                    }
                    else if (isLineSegment(*geo)) {
                        auto* line = static_cast<Part::GeomLineSegment*>(geo);  // NOLINT
                        line->setPoints(line->getStartPoint() + vec, line->getEndPoint() + vec);
                    }
                    else if (isBSplineCurve(*geo)) {
                        auto* bSpline = static_cast<Part::GeomBSplineCurve*>(geo);  // NOLINT
                        std::vector<Base::Vector3d> poles = bSpline->getPoles();
                        for (size_t p = 0; p < poles.size(); p++) {
                            poles[p] = poles[p] + vec;
                        }
                        bSpline->setPoles(poles);
                    }
                    else if (isPoint(*geo)) {
                        auto* point = static_cast<Part::GeomPoint*>(geo);  // NOLINT
                        point->setPoint(point->getPoint() + vec);
                    }

                    ShapeGeometry.emplace_back(std::move(geoUniquePtr));
                }
            }
        }

        if (onlyeditoutline) {
            // Add the lines to show angle
            if (firstTranslationVector.Length() > Precision::Confusion()) {
                addLineToShapeGeometry(
                    toVector3d(referencePoint),
                    toVector3d(firstTranslationPoint),
                    true
                );
            }

            if (secondTranslationVector.Length() > Precision::Confusion()) {
                addLineToShapeGeometry(
                    toVector3d(referencePoint),
                    toVector3d(secondTranslationPoint),
                    true
                );
            }
        }
        else {
            int firstCurveCreated = getHighestCurveIndex() + 1;
            int size = static_cast<int>(listOfGeoIds.size());

            const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();
            // avoid applying equal several times if cloning distanceX and distanceY of the
            // same part.
            std::vector<int> geoIdsWhoAlreadyHasEqual = {};

            for (auto& cstr : vals) {
                int firstIndex = indexOfGeoId(listOfGeoIds, cstr->First);
                int secondIndex = indexOfGeoId(listOfGeoIds, cstr->Second);
                int thirdIndex = indexOfGeoId(listOfGeoIds, cstr->Third);

                for (int k = 0; k < secondNumberOfCopies; k++) {
                    for (int i = 0; i <= numberOfCopiesToMake; i++) {
                        if (k == 0 && i == 0) {
                            continue;
                        }

                        int firstIndexi = firstCurveCreated + firstIndex + size * (i - 1)
                            + size * (numberOfCopiesToMake + 1) * k;
                        int secondIndexi = firstCurveCreated + secondIndex + size * (i - 1)
                            + size * (numberOfCopiesToMake + 1) * k;
                        int thirdIndexi = firstCurveCreated + thirdIndex + size * (i - 1)
                            + size * (numberOfCopiesToMake + 1) * k;

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
                                  || cstr->Type == Parallel || cstr->Type == Equal || cstr->Type == Angle
                                  || cstr->Type == PointOnObject || cstr->Type == Horizontal
                                  || cstr->Type == Vertical || cstr->Type == InternalAlignment)
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
                            else {
                                newConstr->Second = secondIndexi;
                            }
                        }
                        else if ((cstr->Type == Block || cstr->Type == Horizontal
                                  || cstr->Type == Vertical)
                                 && firstIndex >= 0) {
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
    }


public:
    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;

        return Gui::lookupHints<SelectMode>(
            state(),
            {{.state = SelectMode::SeekFirst,
              .hints = {{tr("%1 pick reference point", "Sketcher Translate: hint"), {MouseLeft}}}},
             {.state = SelectMode::SeekSecond,
              .hints = {{tr("%1 set translation vector", "Sketcher Translate: hint"), {MouseLeft}}}},
             {.state = SelectMode::SeekThird,
              .hints
              = {{tr("%1 set second translation vector", "Sketcher Translate: hint"), {MouseLeft}}}}}
        );
    }
};

template<>
auto DSHTranslateControllerBase::getState(int labelindex) const
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
void DSHTranslateController::firstKeyShortcut()
{
    auto value = toolWidget->getParameter(WParameter::First);
    toolWidget->setParameterWithoutPassingFocus(WParameter::First, value + 1);
}

template<>
void DSHTranslateController::secondKeyShortcut()
{
    auto value = toolWidget->getParameter(WParameter::First);
    if (value > 0.0) {
        toolWidget->setParameterWithoutPassingFocus(WParameter::First, value - 1);
    }
}

template<>
void DSHTranslateController::thirdKeyShortcut()
{
    auto value = toolWidget->getParameter(WParameter::Second);
    toolWidget->setParameterWithoutPassingFocus(WParameter::Second, value + 1);
}

template<>
void DSHTranslateController::fourthKeyShortcut()
{
    auto value = toolWidget->getParameter(WParameter::Second);
    if (value > 1.0) {
        toolWidget->setParameterWithoutPassingFocus(WParameter::Second, value - 1);
    }
}

template<>
void DSHTranslateController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        toolWidget->setCheckboxLabel(
            WCheckbox::FirstBox,
            QApplication::translate("TaskSketcherTool_c1_translate", "Apply equal constraints")
        );
        toolWidget->setCheckboxToolTip(
            WCheckbox::FirstBox,
            QApplication::translate(
                "TaskSketcherTool_c1_translate",
                "If this option is selected dimensional constraints are "
                "excluded from the operation.\n"
                "Instead equal constraints are applied between the original "
                "objects and their copies."
            )
        );
    }

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
        Gui::SoDatumLabel::DISTANCE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
    onViewParameters[OnViewParameter::Sixth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );

    toolWidget->setParameterLabel(
        WParameter::First,
        QApplication::translate("TaskSketcherTool_p3_translate", "Copies (+'U'/-'J')")
    );
    toolWidget->setParameterLabel(
        WParameter::Second,
        QApplication::translate("TaskSketcherTool_p5_translate", "Rows (+'R'/-'F')")
    );

    toolWidget->setParameter(OnViewParameter::First, 0.0);
    toolWidget->setParameter(OnViewParameter::Second, 1.0);
    toolWidget->configureParameterUnit(OnViewParameter::First, Base::Unit());
    toolWidget->configureParameterUnit(OnViewParameter::Second, Base::Unit());
    toolWidget->configureParameterMin(OnViewParameter::First, 0.0);      // NOLINT
    toolWidget->configureParameterMin(OnViewParameter::Second, 0.0);     // NOLINT
    toolWidget->configureParameterMax(OnViewParameter::First, 9999.0);   // NOLINT
    toolWidget->configureParameterMax(OnViewParameter::Second, 9999.0);  // NOLINT
    toolWidget->configureParameterDecimals(OnViewParameter::First, 0);
    toolWidget->configureParameterDecimals(OnViewParameter::Second, 0);
}

template<>
void DSHTranslateController::adaptDrawingToParameterChange(int parameterindex, double value)
{
    switch (parameterindex) {
        case WParameter::First:
            handler->numberOfCopies = floor(abs(value));
            break;
        case WParameter::Second:
            handler->secondNumberOfCopies = floor(abs(value));
            break;
    }
}

template<>
void DSHTranslateController::adaptDrawingToCheckboxChange(int checkboxindex, bool value)
{
    switch (checkboxindex) {
        case WCheckbox::FirstBox: {
            handler->cloneConstraints = value;
        } break;
    }
}

template<>
void DSHTranslateControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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

            Base::Vector2d dir = onSketchPos - handler->referencePoint;
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

                onSketchPos = handler->referencePoint + length * dir.Normalize();
            }

            if (fourthParam->isSet) {
                double angle = Base::toRadians(fourthParam->getValue());
                onSketchPos.x = handler->referencePoint.x + cos(angle) * length;
                onSketchPos.y = handler->referencePoint.y + sin(angle) * length;
            }
        } break;
        case SelectMode::SeekThird: {
            auto& fifthParam = onViewParameters[OnViewParameter::Fifth];
            auto& sixthParam = onViewParameters[OnViewParameter::Sixth];

            Base::Vector2d dir = onSketchPos - handler->referencePoint;
            if (dir.Length() < Precision::Confusion()) {
                dir.x = 1.0;  // if direction null, default to (1,0)
            }
            double length = dir.Length();

            if (fifthParam->isSet) {
                length = fifthParam->getValue();
                if (length < Precision::Confusion() && fifthParam->hasFinishedEditing) {
                    unsetOnViewParameter(fifthParam.get());
                    return;
                }

                onSketchPos = handler->referencePoint + length * dir.Normalize();
            }

            if (sixthParam->isSet) {
                double angle = Base::toRadians(sixthParam->getValue());
                onSketchPos.x = handler->referencePoint.x + cos(angle) * length;
                onSketchPos.y = handler->referencePoint.y + sin(angle) * length;
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHTranslateController::adaptParameters(Base::Vector2d onSketchPos)
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
                double length = (onSketchPos - handler->referencePoint).Length();
                setOnViewParameterValue(OnViewParameter::Third, length);
            }

            Base::Vector2d vec2d = Base::Vector2d(
                handler->firstTranslationVector.x,
                handler->firstTranslationVector.y
            );
            double angle = vec2d.Angle();
            double range = angle * 180 / std::numbers::pi;

            if (!fourthParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Fourth, range, Base::Unit::Angle);
            }

            Base::Vector3d start = toVector3d(handler->referencePoint);
            Base::Vector3d end = toVector3d(onSketchPos);

            thirdParam->setPoints(start, end);
            fourthParam->setPoints(start, Base::Vector3d());
            fourthParam->setLabelRange(angle);
        } break;
        case SelectMode::SeekThird: {
            auto& fifthParam = onViewParameters[OnViewParameter::Fifth];
            auto& sixthParam = onViewParameters[OnViewParameter::Sixth];

            if (!fifthParam->isSet) {
                double length = (onSketchPos - handler->referencePoint).Length();
                setOnViewParameterValue(OnViewParameter::Fifth, length);
            }

            Base::Vector2d vec2d = Base::Vector2d(
                handler->secondTranslationVector.x,
                handler->secondTranslationVector.y
            );
            double angle = vec2d.Angle();
            double range = angle * 180 / std::numbers::pi;

            if (!sixthParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Sixth, range, Base::Unit::Angle);
            }

            Base::Vector3d start = toVector3d(handler->referencePoint);
            Base::Vector3d end = toVector3d(onSketchPos);

            fifthParam->setPoints(start, end);
            sixthParam->setPoints(start, Base::Vector3d());
            sixthParam->setLabelRange(angle);
        } break;
        default:
            break;
    }
}

template<>
void DSHTranslateController::computeNextDrawSketchHandlerMode()
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
                if (handler->secondNumberOfCopies == 1) {
                    handler->setNextState(SelectMode::End);
                }
                else {
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

}  // namespace SketcherGui

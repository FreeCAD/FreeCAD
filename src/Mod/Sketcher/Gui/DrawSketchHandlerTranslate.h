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


#ifndef SKETCHERGUI_DrawSketchHandlerTranslate_H
#define SKETCHERGUI_DrawSketchHandlerTranslate_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

#include <Mod/Sketcher/App/GeometryFacade.h>

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

//Todo: Add 2 more parameters to the tool widget so that we can use 8. Adding angles of translation vectors.

class DrawSketchHandlerTranslate;

namespace ConstructionMethods {
    enum class TranslateConstructionMethod {
        LinearArray,
        RectangularArray,
        End // Must be the last one
    };
}

using DrawSketchHandlerTranslateBase = DrawSketchDefaultWidgetHandler< DrawSketchHandlerTranslate,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 0,
    /*WidgetParametersT =*/WidgetParameters<4, 6>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<1, 1>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::TranslateConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerTranslate : public DrawSketchHandlerTranslateBase
{
    friend DrawSketchHandlerTranslateBase;

public:
    DrawSketchHandlerTranslate(std::vector<int> listOfGeoIds, ConstructionMethod constrMethod = ConstructionMethod::LinearArray) :
        DrawSketchHandlerTranslateBase(constrMethod)
        , snapMode(SnapMode::Free)
        , listOfGeoIds(listOfGeoIds)
        , firstTranslationVector(Base::Vector3d(0., 0., 0.))
        , secondTranslationVector(Base::Vector3d(0., 0., 0.))
        , deleteOriginal(false)
        , cloneConstraints(false)
        , numberOfCopies(0)
        , secondNumberOfCopies(1)

        {}
    virtual ~DrawSketchHandlerTranslate() = default;


    enum class SnapMode {
        Free,
        Snap5Degree
    };

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
            snapMode = SnapMode::Snap5Degree;
        else
            snapMode = SnapMode::Free;

        switch (state()) {
        case SelectMode::SeekFirst:
        {
            referencePoint = onSketchPos;
            if (snapMode == SnapMode::Snap5Degree) {
                getSnapPoint(referencePoint);
            }
            drawPositionAtCursor(onSketchPos);
        }
        break;
        case SelectMode::SeekSecond:
        {
            double length = (onSketchPos - referencePoint).Length();
            double angle = (onSketchPos - referencePoint).Angle();
            firstTranslationPoint = onSketchPos;

            if (snapMode == SnapMode::Snap5Degree) {
                if (getSnapPoint(firstTranslationPoint)) {
                    angle = (firstTranslationPoint - referencePoint).Angle();
                }
                else {
                    angle = round(angle / (M_PI / 36)) * M_PI / 36;
                    firstTranslationPoint = referencePoint + length * Base::Vector2d(cos(angle), sin(angle));
                }
            }

            firstTranslationVector.x = (firstTranslationPoint - referencePoint).x;
            firstTranslationVector.y = (firstTranslationPoint - referencePoint).y;

            //Draw geometries
            generateTranslatedGeos(false);

            SbString text;
            text.sprintf(" (%.1f, %.1fdeg)", length, angle * 180 / M_PI);
            setPositionText(firstTranslationPoint, text);
        }
        break;
        case SelectMode::SeekThird:
        {
            double length = (onSketchPos - referencePoint).Length();
            double angle = (onSketchPos - referencePoint).Angle();
            secondTranslationPoint = onSketchPos;

            if (snapMode == SnapMode::Snap5Degree) {
                if (getSnapPoint(secondTranslationPoint)) {
                    angle = (secondTranslationPoint - referencePoint).Angle();
                }
                else {
                    angle = round(angle / (M_PI / 36)) * M_PI / 36;
                    secondTranslationPoint = referencePoint + length * Base::Vector2d(cos(angle), sin(angle));
                }
            }

            secondTranslationVector.x = (secondTranslationPoint - referencePoint).x;
            secondTranslationVector.y = (secondTranslationPoint - referencePoint).y;

            //Draw geometries
            generateTranslatedGeos(false);

            SbString text;
            text.sprintf(" (%.1f, %.1fdeg)", length, angle * 180 / M_PI);
            setPositionText(secondTranslationPoint, text);
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        generateTranslatedGeos(/*CreateGeos*/ true);

        sketchgui->purgeHandler();
    }

    virtual void createAutoConstraints() override {
        //none
    }

    virtual std::string getToolName() const override {
        return "DSH_Translate";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Translate");
    }

    //reimplement because linear array is 2 steps while rectangular array is 3 steps
    virtual void onButtonPressed(Base::Vector2d onSketchPos) override {
        this->updateDataAndDrawToPosition(onSketchPos);
        if (state() == SelectMode::SeekSecond && constructionMethod() == ConstructionMethod::LinearArray) {
            setState(SelectMode::End);
        }
        else {
            this->moveToNextMode();
        }
    }

    virtual void activated() override
    {
        DrawSketchDefaultHandler::activated();
        firstCurveCreated = getHighestCurveIndex() + 1;
    }

private:
    SnapMode snapMode;
    std::vector<int> listOfGeoIds;
    Base::Vector2d referencePoint, firstTranslationPoint, secondTranslationPoint;
    Base::Vector3d firstTranslationVector, secondTranslationVector;

    bool deleteOriginal, cloneConstraints;
    int numberOfCopies, secondNumberOfCopies, firstCurveCreated;

    void generateTranslatedGeos(bool onReleaseButton) {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
        int numberOfCopiesToMake = numberOfCopies;
        if (numberOfCopies == 0) {
            numberOfCopiesToMake = 1;
            deleteOriginal = 1;
        }
        else {
            deleteOriginal = 0;
        }

        //Generate geos
        std::vector<Part::Geometry*> geometriesToAdd;
        for (int k = 0; k < secondNumberOfCopies; k++) {
            for (int i = 0; i <= numberOfCopiesToMake; i++) {
                if (!(k == 0 && i == 0)) {
                    for (size_t j = 0; j < listOfGeoIds.size(); j++) {
                        Part::Geometry* geo = Obj->getGeometry(listOfGeoIds[j])->copy();
                        Sketcher::GeometryFacade::setConstruction(geo, Sketcher::GeometryFacade::getConstruction(Obj->getGeometry(listOfGeoIds[j])));
                        if (geo->getTypeId() == Part::GeomConic::getClassTypeId()) {
                            Part::GeomConic* conic = static_cast<Part::GeomConic*>(geo);
                            conic->setCenter(conic->getCenter() + firstTranslationVector * i + secondTranslationVector * k);
                            geometriesToAdd.push_back(conic);
                        }
                        else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            Part::GeomLineSegment* line = static_cast<Part::GeomLineSegment*>(geo);
                            line->setPoints(line->getStartPoint() + firstTranslationVector * i + secondTranslationVector * k,
                                line->getEndPoint() + firstTranslationVector * i + secondTranslationVector * k);
                            geometriesToAdd.push_back(line);
                        }
                        else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                            Part::GeomBSplineCurve* bSpline = static_cast<Part::GeomBSplineCurve*>(geo);
                            std::vector<Base::Vector3d> poles = bSpline->getPoles();
                            for (size_t p = 0; p < poles.size(); p++) {
                                poles[p] = poles[p] + firstTranslationVector * i + secondTranslationVector * k;
                            }
                            bSpline->setPoles(poles);
                            geometriesToAdd.push_back(bSpline);
                        }
                    }
                }
            }
        }

        if (!onReleaseButton) {
            //Add the line to show angle
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            Base::Vector3d p1 = Base::Vector3d(referencePoint.x, referencePoint.y, 0.);
            Base::Vector3d p2 = Base::Vector3d(firstTranslationPoint.x, firstTranslationPoint.y, 0.);
            line->setPoints(p1, p2);
            geometriesToAdd.push_back(line);

            if (secondTranslationVector.Length() > Precision::Confusion()) {
                Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
                p1 = Base::Vector3d(referencePoint.x, referencePoint.y, 0.);
                p2 = Base::Vector3d(secondTranslationPoint.x, secondTranslationPoint.y, 0.);
                line2->setPoints(p1, p2);
                geometriesToAdd.push_back(line2);
            }

            //Draw geos
            drawEdit(geometriesToAdd);
        }
        else {
            //Creates geos
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Translate"));
            Obj->addGeometry(std::move(geometriesToAdd));

            //Create constrains
            const std::vector< Sketcher::Constraint* >& vals = Obj->Constraints.getValues();
            std::vector< Sketcher::Constraint* > newconstrVals(vals);
            std::vector<int> geoIdsWhoAlreadyHasEqual = {}; //avoid applying equal several times if cloning distanceX and distanceY of the same part.

            std::vector< Sketcher::Constraint* >::const_iterator itEnd = vals.end(); //we need vals.end before adding any constraints
            for (std::vector< Sketcher::Constraint* >::const_iterator it = vals.begin(); it != itEnd; ++it) {
                int firstIndex = indexInVec(listOfGeoIds, (*it)->First);
                int secondIndex = indexInVec(listOfGeoIds, (*it)->Second);
                int thirdIndex = indexInVec(listOfGeoIds, (*it)->Third);

                if (((*it)->Type == Sketcher::Symmetric
                    || (*it)->Type == Sketcher::Tangent
                    || (*it)->Type == Sketcher::Perpendicular)
                    && firstIndex >= 0 && secondIndex >= 0 && thirdIndex >= 0) {
                    for (int k = 0; k < secondNumberOfCopies; k++) {
                        for (int i = 0; i <= numberOfCopiesToMake; i++) {
                            if (!(k == 0 && i == 0)) {
                                Sketcher::Constraint* constNew = (*it)->copy();
                                constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                constNew->Third = firstCurveCreated + thirdIndex + listOfGeoIds.size() * i + listOfGeoIds.size() * numberOfCopiesToMake * k;
                                newconstrVals.push_back(constNew);
                            }
                        }
                    }
                }
                else if (((*it)->Type == Sketcher::Coincident
                    || (*it)->Type == Sketcher::Tangent
                    || (*it)->Type == Sketcher::Symmetric
                    || (*it)->Type == Sketcher::Perpendicular
                    || (*it)->Type == Sketcher::Parallel
                    || (*it)->Type == Sketcher::Equal
                    || (*it)->Type == Sketcher::Angle
                    || (*it)->Type == Sketcher::PointOnObject)
                    && firstIndex >= 0 && secondIndex >= 0 && thirdIndex == Sketcher::GeoEnum::GeoUndef) {
                    for (int k = 0; k < secondNumberOfCopies; k++) {
                        for (int i = 0; i <= numberOfCopiesToMake; i++) {
                            if (!(k == 0 && i == 0)) {
                                Sketcher::Constraint* constNew = (*it)->copy();
                                constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                newconstrVals.push_back(constNew);
                            }
                        }
                    }
                }
                else if (((*it)->Type == Sketcher::Radius
                    || (*it)->Type == Sketcher::Diameter)
                    && firstIndex >= 0) {
                    for (int k = 0; k < secondNumberOfCopies; k++) {
                        for (int i = 0; i <= numberOfCopiesToMake; i++) {
                            if (!(k == 0 && i == 0)) {
                                if (deleteOriginal || !cloneConstraints) {
                                    Sketcher::Constraint* constNew = (*it)->copy();
                                    constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                    newconstrVals.push_back(constNew);
                                }
                                else { //Clone constraint mode !
                                    Sketcher::Constraint* constNew = (*it)->copy();
                                    constNew->Type = Sketcher::Equal;// first is already (*it)->First
                                    constNew->isDriving = true;
                                    constNew->Second = firstCurveCreated + firstIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                    newconstrVals.push_back(constNew);
                                }
                            }
                        }
                    }
                }
                else if (((*it)->Type == Sketcher::Distance
                    || (*it)->Type == Sketcher::DistanceX
                    || (*it)->Type == Sketcher::DistanceY)
                    && firstIndex >= 0 && secondIndex >= 0) { //only line length because we can't apply equality between points.
                    for (int k = 0; k < secondNumberOfCopies; k++) {
                        for (int i = 0; i <= numberOfCopiesToMake; i++) {
                            if (!(k == 0 && i == 0)) {
                                if (deleteOriginal || !cloneConstraints || (*it)->First != (*it)->Second) {
                                    Sketcher::Constraint* constNew = (*it)->copy();
                                    constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                    constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                    newconstrVals.push_back(constNew);
                                }
                                else if (indexInVec(geoIdsWhoAlreadyHasEqual, firstCurveCreated + secondIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k) == -1) { //Clone constraint mode !
                                    Sketcher::Constraint* constNew = (*it)->copy();
                                    constNew->Type = Sketcher::Equal;
                                    constNew->isDriving = true;
                                    constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                    geoIdsWhoAlreadyHasEqual.push_back(constNew->Second);
                                    newconstrVals.push_back(constNew);
                                }
                            }
                        }
                    }
                }
            }
            if (newconstrVals.size() > vals.size())
                Obj->Constraints.setValues(std::move(newconstrVals));

            if (deleteOriginal) {
                std::stringstream stream;
                for (size_t j = 0; j < listOfGeoIds.size() - 1; j++) {
                    stream << listOfGeoIds[j] << ",";
                }
                stream << listOfGeoIds[listOfGeoIds.size() - 1];
                try {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometries([%s])", stream.str().c_str());
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("%s\n", e.what());
                }
            }
            Gui::Command::commitCommand();

            sketchgui->getSketchObject()->solve(true);
            sketchgui->draw(false, false); // Redraw
        }
    }

    bool getSnapPoint(Base::Vector2d& snapPoint) {
        int pointGeoId = Sketcher::GeoEnum::GeoUndef;
        Sketcher::PointPos pointPosId = Sketcher::PointPos::none;
        int VtId = getPreselectPoint();
        int CrsId = getPreselectCross();
        if (CrsId == 0) {
            pointGeoId = Sketcher::GeoEnum::RtPnt;
            pointPosId = Sketcher::PointPos::start;
        }
        else if (VtId >= 0) {
            sketchgui->getSketchObject()->getGeoVertexIndex(VtId, pointGeoId, pointPosId);
        }
        if (pointGeoId != Sketcher::GeoEnum::GeoUndef && pointGeoId < firstCurveCreated) {
            //don't want to snap to the point of a geometry which is being previewed!
            auto sk = static_cast<Sketcher::SketchObject*>(sketchgui->getObject());
            snapPoint.x = sk->getPoint(pointGeoId, pointPosId).x;
            snapPoint.y = sk->getPoint(pointGeoId, pointPosId).y;
            return true;
        }
        return false;
    }

    int indexInVec(std::vector<int> vec, int elem)
    {
        if (elem == Sketcher::GeoEnum::GeoUndef) {
            return Sketcher::GeoEnum::GeoUndef;
        }
        for (size_t i = 0; i < vec.size(); i++)
        {
            if (vec[i] == elem)
            {
                return i;
            }
        }
        return -1;
    }
};

template <> auto DrawSketchHandlerTranslateBase::ToolWidgetManager::getState(int parameterindex) const {
    switch (parameterindex) {
    case WParameter::First:
    case WParameter::Second:
        return SelectMode::SeekFirst;
        break;
    case WParameter::Third:
        return handler->state();
        break;
    case WParameter::Fourth:
        return SelectMode::SeekSecond;
        break;
    case WParameter::Fifth:
        return handler->state();
        break;
    case WParameter::Sixth:
        return SelectMode::SeekThird;
        break;
    default:
        THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template <> void DrawSketchHandlerTranslateBase::ToolWidgetManager::configureToolWidget() {
    if (!init) { // Code to be executed only upon initialisation
        QStringList names = { QStringLiteral("Linear array"), QStringLiteral("Rectangular array") };
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    auto dHandler = static_cast<DrawSketchHandlerTranslate*>(handler);
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_translate", "x of reference"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_translate", "y of reference"));

    if (dHandler->constructionMethod() == ConstructionMethod::LinearArray) {
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_translate", "Number of copies"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_translate", "Translation length"));
    }
    else {
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_translate", "First number of copies"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_translate", "First translation length"));
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("TaskSketcherTool_p5_translate", "Second number of copies"));
        toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("TaskSketcherTool_p6_translate", "Second translation length"));
    }

    toolWidget->setCheckboxLabel(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_translate", "Clone constraints"));

    toolWidget->setNoticeVisible(true);
    toolWidget->setNoticeText(QApplication::translate("Translate_1", "Select the reference point of the translation."));
}

template <> void DrawSketchHandlerTranslateBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    auto dHandler = static_cast<DrawSketchHandlerTranslate*>(handler);
    switch (parameterindex) {
    case WParameter::First:
        dHandler->referencePoint.x = value;
        break;
    case WParameter::Second:
        dHandler->referencePoint.y = value;
        break;
    case WParameter::Third:
        dHandler->numberOfCopies = floor(abs(value));
        break;
    case WParameter::Fifth:
        dHandler->secondNumberOfCopies = floor(abs(value));
        break;
    }
}

template <> void DrawSketchHandlerTranslateBase::ToolWidgetManager::onHandlerModeChanged() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
        toolWidget->setParameterFocus(WParameter::First);
        toolWidget->setNoticeText(QApplication::translate("Translate_1", "Select the reference point of the translation."));
        break;
    case SelectMode::SeekSecond:
        toolWidget->setParameterFocus(WParameter::Third);
        toolWidget->setNoticeText(QApplication::translate("Translate_2", "Select first translation point defining the first translation vector which starts from the reference point."));
        break;
    case SelectMode::SeekThird:
        toolWidget->setParameterFocus(WParameter::Fifth);
        toolWidget->setNoticeText(QApplication::translate("Translate_3", "Select second translation point defining the second translation vector which starts from the reference point."));
        break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerTranslateBase::ToolWidgetManager::adaptDrawingToCheckboxChange(int checkboxindex, bool value) {
    auto dHandler = static_cast<DrawSketchHandlerTranslate*>(handler);
    switch (checkboxindex) {
    case WCheckbox::FirstBox:
        dHandler->deleteOriginal = value;
        break;
    case WCheckbox::SecondBox:
        dHandler->cloneConstraints = value;
        break;
    }
    handler->updateDataAndDrawToPosition(prevCursorPosition);
    onHandlerModeChanged(); //re-focus/select spinbox
}

template <> void DrawSketchHandlerTranslateBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {
    auto dHandler = static_cast<DrawSketchHandlerTranslate*>(handler);

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Fourth)) {
            double length = (onSketchPos - dHandler->referencePoint).Length();
            onSketchPos = dHandler->referencePoint + (onSketchPos - dHandler->referencePoint) / length * toolWidget->getParameter(WParameter::Fourth);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Sixth)) {
            double length = (onSketchPos - dHandler->referencePoint).Length();
            onSketchPos = dHandler->referencePoint + (onSketchPos - dHandler->referencePoint) / length * toolWidget->getParameter(WParameter::Sixth);
        }
    }
    break;
    default:
        break;
    }
    prevCursorPosition = onSketchPos;
}

template <> void DrawSketchHandlerTranslateBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    auto dHandler = static_cast<DrawSketchHandlerTranslate*>(handler);

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (!toolWidget->isParameterSet(WParameter::Fourth))
            toolWidget->updateVisualValue(WParameter::Fourth, (onSketchPos - dHandler->referencePoint).Length());
    }
    break;
    case SelectMode::SeekThird:
    {
        if (!toolWidget->isParameterSet(WParameter::Sixth))
            toolWidget->updateVisualValue(WParameter::Sixth, (onSketchPos - dHandler->referencePoint).Length());
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerTranslateBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    auto dHandler = static_cast<DrawSketchHandlerTranslate*>(handler);
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->setState(SelectMode::SeekSecond);

            handler->updateDataAndDrawToPosition(prevCursorPosition);
        }
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third) ||
            toolWidget->isParameterSet(WParameter::Fourth)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            if (toolWidget->isParameterSet(WParameter::Third) &&
                toolWidget->isParameterSet(WParameter::Fourth)) {

                if (dHandler->constructionMethod() == ConstructionMethod::LinearArray) {
                    handler->setState(SelectMode::End);
                    handler->finish();
                }
                else {
                    handler->setState(SelectMode::SeekThird);
                }
            }
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Fifth) ||
            toolWidget->isParameterSet(WParameter::Sixth)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            if (toolWidget->isParameterSet(WParameter::Fifth) &&
                toolWidget->isParameterSet(WParameter::Sixth)) {

                handler->setState(SelectMode::End);
                handler->finish();
            }
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerTranslateBase::ToolWidgetManager::addConstraints() {
    //none
}

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerTranslate_H


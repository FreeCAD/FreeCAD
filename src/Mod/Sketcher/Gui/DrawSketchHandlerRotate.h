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


#ifndef SKETCHERGUI_DrawSketchHandlerRotate_H
#define SKETCHERGUI_DrawSketchHandlerRotate_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "Utils.h"

#include <Mod/Sketcher/App/GeometryFacade.h>

namespace SketcherGui {

// Rotate / circular pattern tool =======================================================
//TODO DrawSketchDefaultWidgetHandler is not a template!
class DrawSketchHandlerRotate;

using DrawSketchHandlerRotateBase = DrawSketchDefaultWidgetHandler< DrawSketchHandlerRotate,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 0,
    /*WidgetParametersT =*/WidgetParameters<4>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<1>,
    /*WidgetComboboxesT =*/WidgetComboboxes<0>>;

class DrawSketchHandlerRotate : public DrawSketchHandlerRotateBase
{
    friend DrawSketchHandlerRotateBase;

public:
    DrawSketchHandlerRotate(std::vector<int> listOfGeoIds)
        : snapMode(SnapMode::Free)
        , listOfGeoIds(listOfGeoIds)
        , deleteOriginal(false)
        , cloneConstraints(false)
        , numberOfCopies(0) {}

    virtual ~DrawSketchHandlerRotate() = default;

    enum class SnapMode {
        Free,
        Snap
    };

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
            snapMode = SnapMode::Snap;
        else
            snapMode = SnapMode::Free;

        switch (state()) {
        case SelectMode::SeekFirst:
        {
            centerPoint = onSketchPos;
            if (snapMode == SnapMode::Snap) {
                getSnapPoint(centerPoint);
            }
            drawPositionAtCursor(onSketchPos);
        }
        break;
        case SelectMode::SeekSecond:
        {
            length = (onSketchPos - centerPoint).Length();
            startAngle = (onSketchPos - centerPoint).Angle();

            startPoint = onSketchPos;

            if (snapMode == SnapMode::Snap) {
                if (getSnapPoint(startPoint)) {
                    startAngle = (startPoint - centerPoint).Angle();
                }
                else {
                    startAngle = round(startAngle / (M_PI / 36)) * M_PI / 36;
                    startPoint = centerPoint + length * Base::Vector2d(cos(startAngle), sin(startAngle));
                }
            }

            SbString text;
            text.sprintf(" (%.1f, %.1fdeg)", length, startAngle * 180 / M_PI);
            setPositionText(startPoint, text);

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            line->setPoints(Base::Vector3d(centerPoint.x, centerPoint.y, 0.), Base::Vector3d(startPoint.x, startPoint.y, 0.));
            geometriesToAdd.push_back(line);
            drawEdit(geometriesToAdd);

        }
        break;
        case SelectMode::SeekThird:
        {
            endAngle = (onSketchPos - centerPoint).Angle();
            endpoint = onSketchPos;

            if (snapMode == SnapMode::Snap) {
                if (getSnapPoint(endpoint)) {
                    endAngle = (endpoint - centerPoint).Angle();
                }
                else {
                    endAngle = round(endAngle / (M_PI / 36)) * M_PI / 36;
                    endpoint = centerPoint + length * Base::Vector2d(cos(endAngle), sin(endAngle));
                }
            }
            else {
                endpoint = centerPoint + length * Base::Vector2d(cos(endAngle), sin(endAngle));
            }
            double angle1 = atan2(endpoint.y - centerPoint.y,
                endpoint.x - centerPoint.x) - startAngle;
            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
            totalAngle = abs(angle1 - totalAngle) < abs(angle2 - totalAngle) ? angle1 : angle2;

            //generate the copies
            generateRotatedGeos(/*CreateGeos*/ false);
            sketchgui->draw(false, false); // Redraw

            SbString text;
            text.sprintf(" (%d copies, %.1fdeg)", numberOfCopies, totalAngle * 180 / M_PI);
            setPositionText(endpoint, text);
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        generateRotatedGeos(/*CreateGeos*/ true);

        sketchgui->purgeHandler();
    }

    virtual void createAutoConstraints() override {
        //none
    }

    virtual std::string getToolName() const override {
        return "DSH_Rotate";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Rotate");
    }

    virtual void activated() override
    {
        DrawSketchDefaultHandler::activated();
        firstCurveCreated = getHighestCurveIndex() + 1;
    }

private:
    SnapMode snapMode;
    std::vector<int> listOfGeoIds;
    Base::Vector2d centerPoint, startPoint, endpoint;

    bool deleteOriginal, cloneConstraints;
    double length, startAngle, endAngle, totalAngle, individualAngle;
    int numberOfCopies, firstCurveCreated;

    void generateRotatedGeos(bool onReleaseButton) {
        int numberOfCopiesToMake = numberOfCopies;
        if (numberOfCopies == 0) {
            numberOfCopiesToMake = 1;
            deleteOriginal = 1;
        }
        else {
            deleteOriginal = 0;
        }

        individualAngle = totalAngle / numberOfCopiesToMake;

        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        //Generate geos
        std::vector<Part::Geometry*> geometriesToAdd;
        for (int i = 1; i <= numberOfCopiesToMake; i++) {
            for (size_t j = 0; j < listOfGeoIds.size(); j++) {
                Part::Geometry* geo = Obj->getGeometry(listOfGeoIds[j])->copy();
                Sketcher::GeometryFacade::setConstruction(geo, Sketcher::GeometryFacade::getConstruction(Obj->getGeometry(listOfGeoIds[j])));
                if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                    Part::GeomCircle* circle = static_cast<Part::GeomCircle*>(geo);
                    circle->setCenter(getRotatedPoint(circle->getCenter(), centerPoint, individualAngle * i));
                    geometriesToAdd.push_back(circle);
                }
                else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                    Part::GeomArcOfCircle* arcOfCircle = static_cast<Part::GeomArcOfCircle*>(geo);
                    arcOfCircle->setCenter(getRotatedPoint(arcOfCircle->getCenter(), centerPoint, individualAngle * i));
                    double arcStartAngle, arcEndAngle;
                    arcOfCircle->getRange(arcStartAngle, arcEndAngle, /*emulateCCWXY=*/true);
                    arcOfCircle->setRange(arcStartAngle + individualAngle * i, arcEndAngle + individualAngle * i, /*emulateCCWXY=*/true);
                    geometriesToAdd.push_back(arcOfCircle);
                }
                else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                    Part::GeomLineSegment* line = static_cast<Part::GeomLineSegment*>(geo);
                    line->setPoints(getRotatedPoint(line->getStartPoint(), centerPoint, individualAngle * i),
                        getRotatedPoint(line->getEndPoint(), centerPoint, individualAngle * i));
                    geometriesToAdd.push_back(line);
                }
                else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                    Part::GeomEllipse* ellipse = static_cast<Part::GeomEllipse*>(geo);
                    ellipse->setCenter(getRotatedPoint(ellipse->getCenter(), centerPoint, individualAngle * i));
                    ellipse->setMajorAxisDir(getRotatedPoint(ellipse->getMajorAxisDir(), Base::Vector2d(0., 0.), individualAngle * i));
                    geometriesToAdd.push_back(ellipse);
                }
                else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                    Part::GeomArcOfEllipse* arcOfEllipse = static_cast<Part::GeomArcOfEllipse*>(geo);
                    arcOfEllipse->setCenter(getRotatedPoint(arcOfEllipse->getCenter(), centerPoint, individualAngle * i));
                    arcOfEllipse->setMajorAxisDir(getRotatedPoint(arcOfEllipse->getMajorAxisDir(), Base::Vector2d(0., 0.), individualAngle * i));
                    geometriesToAdd.push_back(arcOfEllipse);
                }
                else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                    Part::GeomArcOfHyperbola* arcOfHyperbola = static_cast<Part::GeomArcOfHyperbola*>(geo);
                    arcOfHyperbola->setCenter(getRotatedPoint(arcOfHyperbola->getCenter(), centerPoint, individualAngle * i));
                    arcOfHyperbola->setMajorAxisDir(getRotatedPoint(arcOfHyperbola->getMajorAxisDir(), Base::Vector2d(0., 0.), individualAngle * i));
                    geometriesToAdd.push_back(arcOfHyperbola);
                }
                else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                    Part::GeomArcOfParabola* arcOfParabola = static_cast<Part::GeomArcOfParabola*>(geo);

                    arcOfParabola->setCenter(getRotatedPoint(arcOfParabola->getCenter(), centerPoint, individualAngle * i));
                    arcOfParabola->setAngleXU(arcOfParabola->getAngleXU() + individualAngle * i);
                    geometriesToAdd.push_back(arcOfParabola);
                }
                else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                    Part::GeomBSplineCurve* bSpline = static_cast<Part::GeomBSplineCurve*>(geo);
                    std::vector<Base::Vector3d> poles = bSpline->getPoles();
                    for (size_t p = 0; p < poles.size(); p++) {
                        poles[p] = getRotatedPoint(poles[p], centerPoint, individualAngle * i);
                    }
                    bSpline->setPoles(poles);
                    geometriesToAdd.push_back(bSpline);
                }
            }
        }

        if (!onReleaseButton) {
            //Add the lines to show angle
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            Base::Vector3d p1 = Base::Vector3d(centerPoint.x, centerPoint.y, 0.);
            Base::Vector3d p2 = Base::Vector3d(startPoint.x, startPoint.y, 0.);
            line->setPoints(p1, p2);
            geometriesToAdd.push_back(line);

            Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
            p1 = Base::Vector3d(centerPoint.x, centerPoint.y, 0.);
            p2 = Base::Vector3d(endpoint.x, endpoint.y, 0.);
            line2->setPoints(p1, p2);
            geometriesToAdd.push_back(line2);

            //Draw geos
            drawEdit(geometriesToAdd);
        }
        else {
            //Creates geos
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Rotate"));
            Obj->addGeometry(std::move(geometriesToAdd));

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
                    for (int i = 0; i < numberOfCopiesToMake; i++) {
                        Sketcher::Constraint* constNew = (*it)->copy();
                        constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * i;
                        constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * i;
                        constNew->Third = firstCurveCreated + thirdIndex + listOfGeoIds.size() * i;
                        newconstrVals.push_back(constNew);
                    }
                }
                else if (((*it)->Type == Sketcher::Coincident
                    || (*it)->Type == Sketcher::Tangent
                    || (*it)->Type == Sketcher::Symmetric
                    || (*it)->Type == Sketcher::Perpendicular
                    || (*it)->Type == Sketcher::Parallel
                    || (*it)->Type == Sketcher::Equal
                    || (*it)->Type == Sketcher::PointOnObject)
                    && firstIndex >= 0 && secondIndex >= 0 && thirdIndex == Sketcher::GeoEnum::GeoUndef) {
                    for (int i = 0; i < numberOfCopiesToMake; i++) {
                        Sketcher::Constraint* constNew = (*it)->copy();
                        constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * i;
                        constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * i;
                        newconstrVals.push_back(constNew);
                    }
                }
                else if (((*it)->Type == Sketcher::Radius
                    || (*it)->Type == Sketcher::Diameter)
                    && firstIndex >= 0) {
                    for (int i = 0; i < numberOfCopiesToMake; i++) {
                        if (deleteOriginal || !cloneConstraints) {
                            Sketcher::Constraint* constNew = (*it)->copy();
                            constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * i;
                            newconstrVals.push_back(constNew);
                        }
                        else {
                            Sketcher::Constraint* constNew = (*it)->copy();
                            constNew->Type = Sketcher::Equal;// first is already (*it)->First
                            constNew->isDriving = true;
                            constNew->Second = firstCurveCreated + firstIndex + listOfGeoIds.size() * i;
                            newconstrVals.push_back(constNew);
                        }
                    }
                }
                else if (((*it)->Type == Sketcher::Distance
                    || (*it)->Type == Sketcher::DistanceX
                    || (*it)->Type == Sketcher::DistanceY)
                    && firstIndex >= 0 && secondIndex >= 0) { //only line length because we can't apply equality between points.
                    for (int i = 0; i < numberOfCopiesToMake; i++) {
                        if ((deleteOriginal || !cloneConstraints) && (*it)->Type == Sketcher::Distance) {
                            Sketcher::Constraint* constNew = (*it)->copy();
                            constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * i;
                            constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * i;
                            newconstrVals.push_back(constNew);
                        }
                        else if ((*it)->First == (*it)->Second && indexInVec(geoIdsWhoAlreadyHasEqual, firstCurveCreated + secondIndex + listOfGeoIds.size() * i) == -1) {
                            Sketcher::Constraint* constNew = (*it)->copy();
                            constNew->Type = Sketcher::Equal;// first is already (*it)->First
                            constNew->isDriving = true;
                            constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * i;
                            geoIdsWhoAlreadyHasEqual.push_back(constNew->Second);
                            newconstrVals.push_back(constNew);
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

    Base::Vector3d getRotatedPoint(Base::Vector3d pointToRotate, Base::Vector2d centerPoint, double angle) {
        Base::Vector2d pointToRotate2D;
        pointToRotate2D.x = pointToRotate.x;
        pointToRotate2D.y = pointToRotate.y;

        double initialAngle = (pointToRotate2D - centerPoint).Angle();
        double lengthToCenter = (pointToRotate2D - centerPoint).Length();

        pointToRotate2D = centerPoint + lengthToCenter * Base::Vector2d(cos(angle + initialAngle), sin(angle + initialAngle));


        pointToRotate.x = pointToRotate2D.x;
        pointToRotate.y = pointToRotate2D.y;

        return pointToRotate;
    }
};

template <> auto DrawSketchHandlerRotateBase::ToolWidgetManager::getState(int parameterindex) const {
    switch (parameterindex) {
    case WParameter::First:
    case WParameter::Second:
        return SelectMode::SeekFirst;
        break;
    case WParameter::Third:
        return SelectMode::SeekThird;
        break;
    case WParameter::Fourth:
        return handler->state();
        break;
    default:
        THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::configureToolWidget() {
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_rotate", "x of center"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_rotate", "y of center"));
    toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_rotate", "Total angle"));
    toolWidget->configureParameterUnit(WParameter::Third, Base::Unit::Angle);
    toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_rotate", "Number of copies"));
    toolWidget->setCheckboxLabel(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_rotate", "Clone constraints"));

    toolWidget->setNoticeVisible(true);
    toolWidget->setNoticeText(QApplication::translate("Rotate_1", "Select the center of the rotation."));
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    switch (parameterindex) {
    case WParameter::First:
        dHandler->centerPoint.x = value;
        break;
    case WParameter::Second:
        dHandler->centerPoint.y = value;
        break;
    case WParameter::Third:
        dHandler->totalAngle = value * M_PI / 180;
        break;
    case WParameter::Fourth:
        dHandler->numberOfCopies = floor(abs(value));
        break;
    }
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::onHandlerModeChanged() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
        toolWidget->setParameterFocus(WParameter::First);
        toolWidget->setNoticeText(QApplication::translate("Rotate_1", "Select the center of the rotation."));
        break;
    case SelectMode::SeekSecond:
        toolWidget->setParameterFocus(WParameter::Third);
        toolWidget->setNoticeText(QApplication::translate("Rotate_2", "Select a first point that will define the rotation angle with the next point."));
        break;
    case SelectMode::SeekThird:
        toolWidget->setParameterFocus(WParameter::Fifth);
        toolWidget->setNoticeText(QApplication::translate("Rotate_3", "Select the second point that will determine the rotation angle."));
        break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::adaptDrawingToCheckboxChange(int checkboxindex, bool value) {
    Q_UNUSED(checkboxindex)
    dHandler->cloneConstraints = value;

    handler->updateDataAndDrawToPosition(prevCursorPosition);
    onHandlerModeChanged(); //re-focus/select spinbox
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

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
        if (toolWidget->isParameterSet(WParameter::Third)) {
            dHandler->length = std::max(1.0, (onSketchPos - dHandler->centerPoint).Length()); //avoid nul length
            dHandler->startAngle = 0.0;
            dHandler->totalAngle = toolWidget->getParameter(WParameter::Third) * M_PI / 180;
            onSketchPos = dHandler->centerPoint + Base::Vector2d(dHandler->length, 0.0);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Third)) {
            dHandler->length = std::max(1.0, (onSketchPos - dHandler->centerPoint).Length()); //avoid nul length
            dHandler->startAngle = 0.0;
            dHandler->endAngle = toolWidget->getParameter(WParameter::Third) * M_PI / 180;
            dHandler->totalAngle = dHandler->endAngle;
            onSketchPos = dHandler->centerPoint + Base::Vector2d(cos(dHandler->totalAngle), sin(dHandler->totalAngle)) * dHandler->length;
        }
    }
    break;
    default:
        break;
    }
    lastWidgetEnforcedPosition = onSketchPos;
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekThird:
    {
        if (!toolWidget->isParameterSet(WParameter::Third))
            toolWidget->updateVisualValue(WParameter::Third, dHandler->totalAngle * 180 / M_PI);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
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
        if (toolWidget->isParameterSet(WParameter::Third)) {
            handler->updateDataAndDrawToPosition(prevCursorPosition);

            handler->setState(SelectMode::SeekThird);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Third) ||
            toolWidget->isParameterSet(WParameter::Fourth)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            if (toolWidget->isParameterSet(WParameter::Third) &&
                toolWidget->isParameterSet(WParameter::Fourth)) {

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

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::addConstraints() {
    //none
}


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerRotate_H


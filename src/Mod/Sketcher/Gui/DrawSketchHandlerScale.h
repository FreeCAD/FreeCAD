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


#ifndef SKETCHERGUI_DrawSketchHandlerScale_H
#define SKETCHERGUI_DrawSketchHandlerScale_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

class DrawSketchHandlerScale;

using DrawSketchHandlerScaleBase = DrawSketchDefaultWidgetHandler< DrawSketchHandlerScale,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 0,
    /*WidgetParametersT =*/WidgetParameters<3>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<1>,
    /*WidgetComboboxesT =*/WidgetComboboxes<0>>;

class DrawSketchHandlerScale : public DrawSketchHandlerScaleBase
{
    friend DrawSketchHandlerScaleBase;

public:
    DrawSketchHandlerScale(std::vector<int> listOfGeoIds)
        : snapMode(SnapMode::Free)
        , listOfGeoIds(listOfGeoIds)
        , deleteOriginal(false) {}
    virtual ~DrawSketchHandlerScale() {}

    enum class SnapMode {
        Free,
        Snap,
        Snap5Degree
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
            referencePoint = onSketchPos;
            if (snapMode == SnapMode::Snap5Degree) {
                getSnapPoint(referencePoint);
            }
            drawPositionAtCursor(onSketchPos);
        }
        break;
        case SelectMode::SeekSecond:
        {
            refLength = (onSketchPos - referencePoint).Length();

            startPoint = onSketchPos;

            if (snapMode == SnapMode::Snap) {
                if (getSnapPoint(startPoint)) {
                    refLength = (startPoint - referencePoint).Length();
                }
            }

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            line->setPoints(Base::Vector3d(referencePoint.x, referencePoint.y, 0.), Base::Vector3d(startPoint.x, startPoint.y, 0.));
            geometriesToAdd.push_back(line);
            drawEdit(geometriesToAdd);

            SbString text;
            text.sprintf(" (%.1f)", refLength);
            setPositionText(startPoint, text);
        }
        break;
        case SelectMode::SeekThird:
        {
            length = (onSketchPos - referencePoint).Length();

            endPoint = onSketchPos;

            if (snapMode == SnapMode::Snap) {
                if (getSnapPoint(endPoint)) {
                    length = (endPoint - referencePoint).Length();
                }
            }

            scaleFactor = length / refLength;

            //generate the copies
            generateScaledGeos(/*CreateGeos*/ false);

            SbString text;
            text.sprintf(" (%.1f)", length);
            setPositionText(endPoint, text);
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        generateScaledGeos(/*CreateGeos*/ true);

        sketchgui->purgeHandler();
    }

    virtual void createAutoConstraints() override {
        //none
    }

    virtual std::string getToolName() const override {
        return "DSH_Scale";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Scale");
    }

    virtual void activated() override
    {
        DrawSketchDefaultHandler::activated();
        firstCurveCreated = getHighestCurveIndex() + 1;
    }

private:
    SnapMode snapMode;
    std::vector<int> listOfGeoIds;
    Base::Vector2d referencePoint, startPoint, endPoint;
    bool deleteOriginal;
    double refLength, length, scaleFactor;
    int firstCurveCreated;
    Base::Vector2d centerPoint;

    void generateScaledGeos(bool onReleaseButton) {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        std::vector<Part::Geometry*> geometriesToAdd;
        for (size_t j = 0; j < listOfGeoIds.size(); j++) {
            Part::Geometry* geo = Obj->getGeometry(listOfGeoIds[j])->copy();
            Sketcher::GeometryFacade::setConstruction(geo, Sketcher::GeometryFacade::getConstruction(Obj->getGeometry(listOfGeoIds[j])));

            if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                Part::GeomCircle* circle = static_cast<Part::GeomCircle*>(geo);
                circle->setRadius(circle->getRadius() * scaleFactor);
                circle->setCenter(getScaledPoint(circle->getCenter(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(circle);
            }
            else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                Part::GeomArcOfCircle* arcOfCircle = static_cast<Part::GeomArcOfCircle*>(geo);
                arcOfCircle->setRadius(arcOfCircle->getRadius() * scaleFactor);
                arcOfCircle->setCenter(getScaledPoint(arcOfCircle->getCenter(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(arcOfCircle);
            }
            else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                Part::GeomLineSegment* line = static_cast<Part::GeomLineSegment*>(geo);
                line->setPoints(getScaledPoint(line->getStartPoint(), referencePoint, scaleFactor),
                    getScaledPoint(line->getEndPoint(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(line);
            }
            else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                Part::GeomEllipse* ellipse = static_cast<Part::GeomEllipse*>(geo);
                ellipse->setMajorRadius(ellipse->getMajorRadius() * scaleFactor);
                ellipse->setMinorRadius(ellipse->getMinorRadius() * scaleFactor);
                ellipse->setCenter(getScaledPoint(ellipse->getCenter(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(ellipse);
            }
            else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                Part::GeomArcOfEllipse* arcOfEllipse = static_cast<Part::GeomArcOfEllipse*>(geo);
                arcOfEllipse->setMajorRadius(arcOfEllipse->getMajorRadius() * scaleFactor);
                arcOfEllipse->setMinorRadius(arcOfEllipse->getMinorRadius() * scaleFactor);
                arcOfEllipse->setCenter(getScaledPoint(arcOfEllipse->getCenter(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(arcOfEllipse);
            }
            else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                Part::GeomArcOfHyperbola* arcOfHyperbola = static_cast<Part::GeomArcOfHyperbola*>(geo);
                arcOfHyperbola->setMajorRadius(arcOfHyperbola->getMajorRadius() * scaleFactor);
                arcOfHyperbola->setMinorRadius(arcOfHyperbola->getMinorRadius() * scaleFactor);
                arcOfHyperbola->setCenter(getScaledPoint(arcOfHyperbola->getCenter(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(arcOfHyperbola);
            }
            else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                Part::GeomArcOfParabola* arcOfParabola = static_cast<Part::GeomArcOfParabola*>(geo);
                //Todo: Problem with scale parabola end points.
                arcOfParabola->setFocal(arcOfParabola->getFocal() * scaleFactor);
                arcOfParabola->setCenter(getScaledPoint(arcOfParabola->getCenter(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(arcOfParabola);
            }
            else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                Part::GeomBSplineCurve* bSpline = static_cast<Part::GeomBSplineCurve*>(geo);
                std::vector<Base::Vector3d> poles = bSpline->getPoles();
                for (size_t p = 0; p < poles.size(); p++) {
                    poles[p] = getScaledPoint(poles[p], referencePoint, scaleFactor);
                }
                bSpline->setPoles(poles);
                geometriesToAdd.push_back(bSpline);
            }
        }

        if (!onReleaseButton) {
            //Add the lines to show lengths
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            Base::Vector3d p1 = Base::Vector3d(referencePoint.x, referencePoint.y, 0.);
            Base::Vector3d p2 = Base::Vector3d(startPoint.x, startPoint.y, 0.);
            line->setPoints(p1, p2);
            geometriesToAdd.push_back(line);

            Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
            p1 = Base::Vector3d(referencePoint.x, referencePoint.y, 0.);
            p2 = Base::Vector3d(endPoint.x, endPoint.y, 0.);
            line2->setPoints(p1, p2);
            geometriesToAdd.push_back(line2);

            //Draw geos
            drawEdit(geometriesToAdd);
        }
        else {
            //Creates geos
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Scale"));
            Obj->addGeometry(std::move(geometriesToAdd));

            //Create constraints
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
                    Sketcher::Constraint* constNew = (*it)->copy();
                    constNew->First = firstCurveCreated + firstIndex;
                    constNew->Second = firstCurveCreated + secondIndex;
                    constNew->Third = firstCurveCreated + thirdIndex;
                    newconstrVals.push_back(constNew);
                }
                else if (((*it)->Type == Sketcher::Coincident
                    || (*it)->Type == Sketcher::Tangent
                    || (*it)->Type == Sketcher::Symmetric
                    || (*it)->Type == Sketcher::Perpendicular
                    || (*it)->Type == Sketcher::Parallel
                    || (*it)->Type == Sketcher::Equal
                    || (*it)->Type == Sketcher::PointOnObject)
                    && firstIndex >= 0 && secondIndex >= 0 && thirdIndex == Sketcher::GeoEnum::GeoUndef) {
                    Sketcher::Constraint* constNew = (*it)->copy();
                    constNew->First = firstCurveCreated + firstIndex;
                    constNew->Second = firstCurveCreated + secondIndex;
                    newconstrVals.push_back(constNew);
                }
                else if (((*it)->Type == Sketcher::Radius
                    || (*it)->Type == Sketcher::Diameter)
                    && firstIndex >= 0) {
                    Sketcher::Constraint* constNew = (*it)->copy();
                    constNew->First = firstCurveCreated + firstIndex;
                    constNew->setValue(constNew->getValue() * scaleFactor);
                    newconstrVals.push_back(constNew);
                }
                else if (((*it)->Type == Sketcher::Distance
                    || (*it)->Type == Sketcher::DistanceX
                    || (*it)->Type == Sketcher::DistanceY)
                    && firstIndex >= 0 && secondIndex >= 0) {
                    Sketcher::Constraint* constNew = (*it)->copy();
                    constNew->First = firstCurveCreated + firstIndex;
                    constNew->Second = firstCurveCreated + secondIndex;
                    constNew->setValue(constNew->getValue() * scaleFactor);
                    newconstrVals.push_back(constNew);
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

    Base::Vector3d getScaledPoint(Base::Vector3d pointToScale, Base::Vector2d referencePoint, double scaleFactor) {
        Base::Vector2d pointToScale2D;
        pointToScale2D.x = pointToScale.x;
        pointToScale2D.y = pointToScale.y;
        pointToScale2D = (pointToScale2D - referencePoint) * scaleFactor + referencePoint;

        pointToScale.x = pointToScale2D.x;
        pointToScale.y = pointToScale2D.y;

        return pointToScale;
    }
};

template <> auto DrawSketchHandlerScaleBase::ToolWidgetManager::getState(int parameterindex) const {
    switch (parameterindex) {
    case WParameter::First:
    case WParameter::Second:
        return SelectMode::SeekFirst;
        break;
    case WParameter::Third:
        return SelectMode::SeekThird;
        break;
    default:
        THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::configureToolWidget() {
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_scale", "x of reference"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_scale", "y of reference"));
    toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_scale", "Scale factor"));

    toolWidget->setCheckboxLabel(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_scale", "Keep original geometries"));

    toolWidget->setNoticeVisible(true);
    toolWidget->setNoticeText(QApplication::translate("Scale_1", "Select the reference point of the scale."));
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    switch (parameterindex) {
    case WParameter::First:
        dHandler->centerPoint.x = value;
        break;
    case WParameter::Second:
        dHandler->centerPoint.y = value;
        break;
    case WParameter::Third:
        dHandler->scaleFactor = value;
        break;
    }
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::onHandlerModeChanged() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
        toolWidget->setParameterFocus(WParameter::First);
        toolWidget->setNoticeText(QApplication::translate("Scale_1", "Select the center of the rotation."));
        break;
    case SelectMode::SeekSecond:
        toolWidget->setParameterFocus(WParameter::Third);
        toolWidget->setNoticeText(QApplication::translate("Scale_2", "Select a point where distance from this point to reference point represent the reference length."));
        break;
    case SelectMode::SeekThird:
        toolWidget->setParameterFocus(WParameter::Third);
        toolWidget->setNoticeText(QApplication::translate("Scale_3", "Select a point where distance from this point to reference point represent the length defining scale factor (scale factor = length / reference length)."));
        break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::adaptDrawingToCheckboxChange(int checkboxindex, bool value) {
    Q_UNUSED(checkboxindex)
    dHandler->deleteOriginal = value;

    handler->updateDataAndDrawToPosition(prevCursorPosition);
    onHandlerModeChanged(); //re-focus/select spinbox
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

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
            dHandler->scaleFactor = toolWidget->getParameter(WParameter::Third);
            onSketchPos = dHandler->referencePoint + Base::Vector2d(1.0, 0.0); //just in case mouse is at referencePoint to avoid 0. length
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Third)) {
            dHandler->scaleFactor = toolWidget->getParameter(WParameter::Third);
            dHandler->startPoint = dHandler->referencePoint + Base::Vector2d(1.0, 0.0);
            dHandler->endPoint = dHandler->referencePoint + Base::Vector2d(dHandler->scaleFactor, 0.0);

            onSketchPos = dHandler->endPoint;
        }
    }
    break;
    default:
        break;
    }
    lastWidgetEnforcedPosition = onSketchPos;
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
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
            toolWidget->updateVisualValue(WParameter::Third, dHandler->scaleFactor);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
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
        if (toolWidget->isParameterSet(WParameter::Third)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            handler->setState(SelectMode::End);
            handler->finish();
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::addConstraints() {
    //none
}


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerScale_H


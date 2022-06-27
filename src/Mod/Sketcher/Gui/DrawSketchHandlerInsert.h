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


#ifndef SKETCHERGUI_DrawSketchHandlerInsert_H
#define SKETCHERGUI_DrawSketchHandlerInsert_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

/* Create Insert =====================================================*/
class DrawSketchHandlerInsert;

namespace ConstructionMethods {

    enum class InsertConstructionMethod {
        Box,
        Arc,
        End // Must be the last one
    };

}

using DrawSketchHandlerInsertBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerInsert,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 2,
    /*WidgetParametersT =*/WidgetParameters<3, 3>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::InsertConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerInsert : public DrawSketchHandlerInsertBase
{
    friend DrawSketchHandlerInsertBase;
public:
    DrawSketchHandlerInsert(int geoI, ConstructionMethod constrMethod = ConstructionMethod::Box) :
        DrawSketchHandlerInsertBase(constrMethod),
        reverseArc(false),
        geoId(geoI) {}

    virtual ~DrawSketchHandlerInsert() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);
        }
        break;
        case SelectMode::SeekSecond:
        {
            Base::Vector2d projectedPoint;
            projectedPoint.ProjectToLine(onSketchPos - startPoint, dirVec);
            projectedPoint = startPoint + projectedPoint;
            p1 = projectedPoint;
            startLength = (p1 - startPoint).Length();
            if (startLength > lineLength * 0.75) {
                boxLength = (lineLength - startLength) * 0.8;
            }
            else {
                boxLength = lineLength / 5;
            }

            SbString text;
            text.sprintf(" (%.1fL)", startLength);
            setPositionText(onSketchPos, text);

            if (constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
                p2 = onSketchPos;

                p3 = p2 + boxLength * dirVec;
                p4.ProjectToLine(p3 - startPoint, dirVec);
                p4 = startPoint + p4;

                drawEdit(createBoxGeometries());

                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }
            else {
                p2 = p1 + boxLength * dirVec;
                radius = boxLength / 2;
                centerPoint = p1 + radius * dirVec;

                startAngle = GetPointAngle(centerPoint, p1);
                endAngle = GetPointAngle(centerPoint, p2);

                //check if we need to reverse arc
                Base::Vector2d midArcPoint;
                midArcPoint.x = centerPoint.x + cos((startAngle + endAngle) / 2) * radius;
                midArcPoint.y = centerPoint.y + sin((startAngle + endAngle) / 2) * radius;
                int signOfMidPoint = getPointSideOfVector(midArcPoint, dirVec, startPoint);
                int signOfCurPos = getPointSideOfVector(onSketchPos, dirVec, startPoint);
                if ((signOfMidPoint != signOfCurPos && signOfMidPoint == 1) || (signOfMidPoint == signOfCurPos && signOfMidPoint == -1))
                    reverseArc = true;
                else
                    reverseArc = false;

                if (reverseArc)
                    std::swap(startAngle, endAngle);

                drawEdit(createArcGeometries());
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            Base::Vector2d projectedPtn;
            projectedPtn.ProjectToLine(onSketchPos - startPoint, dirVec);
            projectedPtn = startPoint + projectedPtn;

            if ((projectedPtn - startPoint).Length() > startLength) {
                if (constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
                    p3 = onSketchPos;
                    p4 = projectedPtn;
                    boxLength = (projectedPtn - p1).Length();
                    Base::Vector2d Perpendicular(-dirVec.y, dirVec.x);
                    p2.ProjectToLine(onSketchPos - p1, Perpendicular);
                    p2 = p1 + p2;

                    drawEdit(createBoxGeometries());
                }
                else {
                    boxLength = (projectedPtn - p1).Length() * 2;
                    centerPoint = onSketchPos;

                    p2 = p1 + boxLength * dirVec;
                    radius = (p1 - centerPoint).Length();

                    startAngle = GetPointAngle(centerPoint, p1);
                    endAngle = GetPointAngle(centerPoint, p2);

                    if (reverseArc)
                        std::swap(startAngle, endAngle);

                    drawEdit(createArcGeometries());
                }

                if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            }

            SbString text;
            text.sprintf(" (%.1fL)", boxLength);
            setPositionText(onSketchPos, text);
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        firstCurve = getHighestCurveIndex() + 1;

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch insert"));

            if (constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
                sketchgui->getSketchObject()->addGeometry(std::move(createBoxGeometries()));

                Gui::Command::doCommand(Gui::Command::Doc,
                    "conList = []\n"
                    "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                    "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                    "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                    "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                    "conList.append(Sketcher.Constraint('Parallel',%d,%d))\n"
                    "conList.append(Sketcher.Constraint('PerpendicularViaPoint',%d,%d,%d,2))\n"
                    "conList.append(Sketcher.Constraint('PerpendicularViaPoint',%d,%d,%d,1))\n"
                    "conList.append(Sketcher.Constraint('Tangent',%d,%d))\n"
                    "%s.addConstraint(conList)\n"
                    "del conList\n",
                    firstCurve, firstCurve + 1, // coincident1
                    firstCurve + 1, firstCurve + 2, // coincident2
                    firstCurve + 2, firstCurve + 3, // coincident3
                    firstCurve + 3, firstCurve + 4, // coincident4
                    firstCurve + 2, firstCurve, // Parallel
                    firstCurve, firstCurve + 1, firstCurve, // Perpendicular1
                    firstCurve + 3, firstCurve + 4, firstCurve + 4, // Perpendicular2
                    firstCurve, firstCurve + 4, // tangent
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch

                Gui::cmdAppObjectArgs(sketchgui->getObject(), "transferConstraints(%d,%d,%d,%d)",
                    geoId, 1, firstCurve, 1);
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "transferConstraints(%d,%d,%d,%d)",
                    geoId, 2, firstCurve + 4, 2);
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "transferConstraints(%d,%d,%d,%d)",
                    geoId, 0, firstCurve, 0);
            }
            else {
                sketchgui->getSketchObject()->addGeometry(std::move(createArcGeometries()));

                Gui::Command::doCommand(Gui::Command::Doc,
                    "conList = []\n"
                    "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,%i))\n"
                    "conList.append(Sketcher.Constraint('Coincident',%i,1,%i,%i))\n"
                    "conList.append(Sketcher.Constraint('Tangent',%d,%d))\n"
                    "%s.addConstraint(conList)\n"
                    "del conList\n",
                    firstCurve,     firstCurve + 1, reverseArc ? 2 : 1, // coincident1
                    firstCurve + 2, firstCurve + 1, reverseArc ? 1 : 2, // coincident2
                    firstCurve, firstCurve + 2, // colinear
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch

                Gui::cmdAppObjectArgs(sketchgui->getObject(), "transferConstraints(%d,%d,%d,%d)",
                    geoId, 1, firstCurve, 1);
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "transferConstraints(%d,%d,%d,%d)",
                    geoId, 2, firstCurve + 2, 2);
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "transferConstraints(%d,%d,%d,%d)",
                    geoId, 0, firstCurve, 0);
            }

            Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometry(%d)", geoId);
            firstCurve--;

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add insert: %s\n", e.what());
            Gui::Command::abortCommand();
        }
    }

    virtual void createAutoConstraints() override {

        if (constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
            // add auto constraints for the insert segment start
            if (!sugConstraints[0].empty()) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[0], firstCurve + 2, Sketcher::PointPos::start);
                sugConstraints[0].clear();
            }

            // add auto constraints for the insert segment end
            if (!sugConstraints[1].empty()) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], firstCurve + 2, Sketcher::PointPos::end);
                sugConstraints[1].clear();
            }
        }
        else {
            // add auto constraints for the insert segment end
            if (!sugConstraints[1].empty()) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], firstCurve + 1, Sketcher::PointPos::mid);
                sugConstraints[1].clear();
            }
        }

    }

    virtual std::string getToolName() const override {
        return "DSH_Insert";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Insert");
    }

    virtual void activated() override
    {
        DrawSketchDefaultHandler::activated();
        setLineGeo(geoId);
    }

    virtual void onButtonPressed(Base::Vector2d onSketchPos) override {
        if (state() == SelectMode::SeekFirst) {
            geoId = getPreselectCurve();
            setLineGeo(geoId);
        }
        else {
            DrawSketchDefaultHandler::onButtonPressed(onSketchPos);
        }
    }

private:
    bool reverseArc;
    int geoId, firstCurve;
    Base::Vector2d startPoint, endPoint, p1, p2, p3, p4, dirVec, centerPoint;
    double lineLength, boxLength, startLength, radius, startAngle, endAngle;

    void setLineGeo(int geoId) {
        if (geoId >= 0) {
            const Part::Geometry* geom = sketchgui->getSketchObject()->getGeometry(geoId);
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                //Hide GeoId line?

                const Part::GeomLineSegment* lineGeo = static_cast<const Part::GeomLineSegment*>(geom);
                startPoint.x = lineGeo->getStartPoint().x;
                startPoint.y = lineGeo->getStartPoint().y;
                endPoint.x = lineGeo->getEndPoint().x;
                endPoint.y = lineGeo->getEndPoint().y;
                lineLength = (endPoint - startPoint).Length();
                dirVec = (endPoint - startPoint) / lineLength;

                setState(SelectMode::SeekSecond);
            }
        }
    }

    std::vector<Part::Geometry*> createBoxGeometries() {
        std::vector<Part::Geometry*> geometriesToAdd;

        Part::GeomLineSegment* line1 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line3 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line4 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line5 = new Part::GeomLineSegment();

        line1->setPoints(Base::Vector3d(startPoint.x, startPoint.y, 0.), Base::Vector3d(p1.x, p1.y, 0.));
        line2->setPoints(Base::Vector3d(p1.x, p1.y, 0.), Base::Vector3d(p2.x, p2.y, 0.));
        line3->setPoints(Base::Vector3d(p2.x, p2.y, 0.), Base::Vector3d(p3.x, p3.y, 0.));
        line4->setPoints(Base::Vector3d(p3.x, p3.y, 0.), Base::Vector3d(p4.x, p4.y, 0.));
        line5->setPoints(Base::Vector3d(p4.x, p4.y, 0.), Base::Vector3d(endPoint.x, endPoint.y, 0.));

        Sketcher::GeometryFacade::setConstruction(line1, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line2, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line3, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line4, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line5, geometryCreationMode);

        geometriesToAdd.push_back(line1);
        geometriesToAdd.push_back(line2);
        geometriesToAdd.push_back(line3);
        geometriesToAdd.push_back(line4);
        geometriesToAdd.push_back(line5);

        return geometriesToAdd;
    }

    std::vector<Part::Geometry*> createArcGeometries() {
        std::vector<Part::Geometry*> geometriesToAdd;

        Part::GeomArcOfCircle* arc1 = new Part::GeomArcOfCircle();
        Part::GeomLineSegment* line1 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line2 = new Part::GeomLineSegment();

        arc1->setRadius(radius);
        arc1->setRange(startAngle, endAngle, true);
        arc1->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));

        line1->setPoints(Base::Vector3d(startPoint.x, startPoint.y, 0.), Base::Vector3d(p1.x, p1.y, 0.));
        line2->setPoints(Base::Vector3d(p2.x, p2.y, 0.), Base::Vector3d(endPoint.x, endPoint.y, 0.));

        Sketcher::GeometryFacade::setConstruction(arc1, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line1, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line2, geometryCreationMode);

        geometriesToAdd.push_back(line1);
        geometriesToAdd.push_back(arc1);
        geometriesToAdd.push_back(line2);

        return geometriesToAdd;
    }

    int getPointSideOfVector(Base::Vector2d pointToCheck, Base::Vector2d separatingVector, Base::Vector2d pointOnVector) {
        Base::Vector2d secondPointOnVec = pointOnVector + separatingVector;
        double d = (pointToCheck.x - pointOnVector.x) * (secondPointOnVec.y - pointOnVector.y)
            - (pointToCheck.y - pointOnVector.y) * (secondPointOnVec.x - pointOnVector.x);
        if (abs(d) < Precision::Confusion()) {
            return 0;
        }
        else if (d < 0) {
            return -1;
        }
        else {
            return 1;
        }
    }
};

template <> auto DrawSketchHandlerInsertBase::ToolWidgetManager::getState(int parameterindex) const {
    switch (parameterindex) {
    case WParameter::First:
    case WParameter::Second:
        return SelectMode::SeekSecond;
        break;
    case WParameter::Third:
        return SelectMode::SeekThird;
        break;
    default:
        THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::configureToolWidget() {
    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Box"), QStringLiteral("Arc")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    toolWidget->setParameterEnabled(WParameter::First, false);
    toolWidget->setParameterEnabled(WParameter::Second, false);
    toolWidget->setParameterEnabled(WParameter::Third, false);
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_insert", "Start distance"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_insert", "Insert length"));
    if (dHandler->constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box)
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_insert", "Insert depth"));
    else
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_insert", "Distance of center to line"));
}

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    switch (parameterindex) {
    case WParameter::First:
        dHandler->startLength = value;
        break;
    case WParameter::Second:
        dHandler->boxLength = value;
        break;
    }
}

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

    switch (handler->state()) {
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::First)) {
            dHandler->startLength = toolWidget->getParameter(WParameter::First);

            Base::Vector2d projectedPtn;
            projectedPtn.ProjectToLine(onSketchPos - dHandler->startPoint, dHandler->dirVec);
            projectedPtn = dHandler->startPoint + projectedPtn;

            onSketchPos = (dHandler->startPoint + dHandler->dirVec * dHandler->startLength) + (onSketchPos - projectedPtn);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
            if (toolWidget->isParameterSet(WParameter::Second)) {
                dHandler->boxLength = toolWidget->getParameter(WParameter::Second);
                dHandler->p4 = dHandler->p1 + dHandler->dirVec * dHandler->boxLength;

                Base::Vector2d projectedPtn;
                projectedPtn.ProjectToLine(onSketchPos - dHandler->startPoint, dHandler->dirVec);
                projectedPtn = dHandler->startPoint + projectedPtn;

                onSketchPos = dHandler->p4 + (onSketchPos - projectedPtn);
            }

            if (toolWidget->isParameterSet(WParameter::Third)) {
                double depth = toolWidget->getParameter(WParameter::Third);
                dHandler->p4.ProjectToLine(onSketchPos - dHandler->startPoint, dHandler->dirVec);
                dHandler->p4 = dHandler->startPoint + dHandler->p4;

                if (!dHandler->reverseArc)
                    onSketchPos = dHandler->p4 + (onSketchPos - dHandler->p4) / (onSketchPos - dHandler->p4).Length() * depth;
                else
                    onSketchPos = dHandler->p4 - (onSketchPos - dHandler->p4) / (onSketchPos - dHandler->p4).Length() * depth;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Second)) {
                dHandler->boxLength = toolWidget->getParameter(WParameter::Second);
                Base::Vector2d centerProjOnLine = dHandler->p1 + dHandler->dirVec * dHandler->boxLength / 2;

                Base::Vector2d projectedPtn;
                projectedPtn.ProjectToLine(onSketchPos - dHandler->startPoint, dHandler->dirVec);
                projectedPtn = dHandler->startPoint + projectedPtn;

                onSketchPos = centerProjOnLine + (onSketchPos - projectedPtn);
            }

            if (toolWidget->isParameterSet(WParameter::Third)) {
                double depth = toolWidget->getParameter(WParameter::Third);
                dHandler->p4.ProjectToLine(onSketchPos - dHandler->startPoint, dHandler->dirVec);
                dHandler->p4 = dHandler->startPoint + dHandler->p4;

                onSketchPos = dHandler->p4 + (onSketchPos - dHandler->p4) / (onSketchPos - dHandler->p4).Length() * depth;
            }
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    Q_UNUSED(onSketchPos)
    switch (handler->state()) {
    case SelectMode::SeekSecond:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, dHandler->startLength);
    }
    break;
    case SelectMode::SeekThird:
    {
        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, dHandler->boxLength);

        if (dHandler->constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, (dHandler->p1 - dHandler->p2).Length());
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Third)) {
                Base::Vector2d centerProjOnLine = dHandler->p1 + dHandler->dirVec * dHandler->boxLength / 2;
                if(abs(dHandler->startAngle - dHandler->endAngle) > M_PI)
                    toolWidget->updateVisualValue(WParameter::Third, (dHandler->centerPoint - centerProjOnLine).Length());
                else
                    toolWidget->updateVisualValue(WParameter::Third, -(dHandler->centerPoint - centerProjOnLine).Length());
            }
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::First)) {

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition);


            handler->setState(SelectMode::SeekThird);

        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if ((toolWidget->isParameterSet(WParameter::Second) ||
            toolWidget->isParameterSet(WParameter::Third))) {

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition);


            if (toolWidget->isParameterSet(WParameter::Second) &&
                toolWidget->isParameterSet(WParameter::Third)) {

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

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::addConstraints() {

    auto depth = toolWidget->getParameter(WParameter::Third);

    auto startLengthSet = toolWidget->isParameterSet(WParameter::First);
    auto insertLengthSet = toolWidget->isParameterSet(WParameter::Second);
    auto depthSet = toolWidget->isParameterSet(WParameter::Third);


    if (startLengthSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
            dHandler->firstCurve, 1, dHandler->firstCurve, 2, dHandler->startLength);


    if (dHandler->constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
        if (insertLengthSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                dHandler->firstCurve +2 , 1, dHandler->firstCurve + 2, 2, dHandler->boxLength);

        if (depthSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                dHandler->firstCurve + 1, 1, dHandler->firstCurve + 1, 2, depth);

    }
    else {
        if (insertLengthSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                dHandler->firstCurve, 2, dHandler->firstCurve + 2, 1, dHandler->boxLength);

        if (depthSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                dHandler->firstCurve + 1, 3, dHandler->firstCurve, 0, depth);
    }
}

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::onHandlerModeChanged() {
    switch (handler->state()) {
    case SelectMode::SeekSecond:
        toolWidget->setParameterEnabled(WParameter::First, true);
        toolWidget->setParameterEnabled(WParameter::Second, true);
        toolWidget->setParameterEnabled(WParameter::Third, true);
        toolWidget->setParameterFocus(WParameter::First);
        break;
    case SelectMode::SeekThird:
        toolWidget->setParameterFocus(WParameter::Second);
        break;
    default:
        break;
    }
}


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerInsert_H


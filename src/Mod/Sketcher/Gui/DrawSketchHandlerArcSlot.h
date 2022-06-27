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


#ifndef SKETCHERGUI_DrawSketchHandlerArcSlot_H
#define SKETCHERGUI_DrawSketchHandlerArcSlot_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

class DrawSketchHandlerArcSlot;

namespace ConstructionMethods {

    enum class ArcSlotConstructionMethod {
        ArcSlot,
        RectangleSlot,
        End // Must be the last one
    };

}

using DrawSketchHandlerArcSlotBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerArcSlot,
    StateMachines::FourSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 3,
    /*WidgetParametersT =*/WidgetParameters<6, 6>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::ArcSlotConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerArcSlot : public DrawSketchHandlerArcSlotBase
{
    friend DrawSketchHandlerArcSlotBase;

public:
    enum class SnapMode {
        Free,
        Snap5Degree
    };

    DrawSketchHandlerArcSlot(ConstructionMethod constrMethod = ConstructionMethod::ArcSlot) :
        DrawSketchHandlerArcSlotBase(constrMethod)
        , startAngle(0)
        , endAngle(0)
        , arcAngle(0) {}

    virtual ~DrawSketchHandlerArcSlot() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
            snapMode = SnapMode::Snap5Degree;
        else
            snapMode = SnapMode::Free;

        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);
            centerPoint = onSketchPos;

            if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[0]);
                return;
            }
        }
        break;
        case SelectMode::SeekSecond:
        {
            startPoint = onSketchPos;

            startAngle = GetPointAngle(centerPoint, startPoint);
            radius = (startPoint - centerPoint).Length();

            if (snapMode == SnapMode::Snap5Degree) {
                startAngle = round(startAngle / (M_PI / 36)) * M_PI / 36;
                startPoint = centerPoint + radius * Base::Vector2d(cos(startAngle), sin(startAngle));
            }

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomCircle* circle = new Part::GeomCircle();
            circle->setRadius(radius);
            circle->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
            geometriesToAdd.push_back(circle);

            //add line to show the snap at 5 degree.
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            line->setPoints(Base::Vector3d(centerPoint.x , centerPoint.y, 0.),
                Base::Vector3d(centerPoint.x + cos(startAngle) * 0.8 * radius, centerPoint.y + sin(startAngle) * 0.8 * radius, 0.));
            geometriesToAdd.push_back(line);

            drawEdit(geometriesToAdd);

            SbString text;
            text.sprintf(" (%.1fR,%.1fdeg)", (float)radius, (float)startAngle * 180 / M_PI);
            setPositionText(onSketchPos, text);

            if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[1]);
                return;
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            endPoint = centerPoint + (onSketchPos - centerPoint) / (onSketchPos - centerPoint).Length() * radius;
            r = radius / 10; //Auto radius to 1/10 of the arc radius

            double startAngleToDraw = startAngle;
            double angle1 = atan2(onSketchPos.y - centerPoint.y,
                onSketchPos.x - centerPoint.x) - startAngle;
            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
            arcAngle = abs(angle1 - arcAngle) < abs(angle2 - arcAngle) ? angle1 : angle2;

            if (snapMode == SnapMode::Snap5Degree) {
                arcAngle = round(arcAngle / (M_PI / 36)) * M_PI / 36;
                endPoint = centerPoint + radius * Base::Vector2d(cos(startAngle + arcAngle), sin(startAngle + arcAngle));
            }

            bool angleReversed = false;
            if (arcAngle > 0)
                endAngle = startAngle + arcAngle;
            else {
                endAngle = startAngle;
                startAngleToDraw = startAngle + arcAngle;
                angleReversed = true;
            }

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomArcOfCircle* arc1 = new Part::GeomArcOfCircle();
            arc1->setRange(startAngleToDraw, endAngle, true);
            arc1->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));

            if (constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                arc1->setRadius(radius - r);
                geometriesToAdd.push_back(arc1);

                Part::GeomArcOfCircle* arc2 = new Part::GeomArcOfCircle();
                arc2->setRadius(radius + r);
                arc2->setRange(startAngleToDraw, endAngle, true);
                arc2->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                geometriesToAdd.push_back(arc2);

                Part::GeomArcOfCircle* arc3 = new Part::GeomArcOfCircle();
                arc3->setRadius(r);
                arc3->setRange(M_PI + startAngleToDraw, 2 * M_PI + startAngleToDraw, true);
                if (angleReversed)
                    arc3->setCenter(Base::Vector3d(endPoint.x, endPoint.y, 0.));
                else
                    arc3->setCenter(Base::Vector3d(startPoint.x, startPoint.y, 0.));
                geometriesToAdd.push_back(arc3);

                Part::GeomArcOfCircle* arc4 = new Part::GeomArcOfCircle();
                arc4->setRadius(r);
                arc4->setRange(endAngle, M_PI + endAngle, true);
                if (angleReversed)
                    arc4->setCenter(Base::Vector3d(startPoint.x, startPoint.y, 0.));
                else
                    arc4->setCenter(Base::Vector3d(endPoint.x, endPoint.y, 0.));
                geometriesToAdd.push_back(arc4);
            }
            else {
                arc1->setRadius(radius);
                geometriesToAdd.push_back(arc1);
            }

            drawEdit(geometriesToAdd);

            SbString text;
            text.sprintf(" (%.1fR,%.1fdeg)", (float)radius, (float)arcAngle * 180 / M_PI);
            setPositionText(onSketchPos, text);

            if (seekAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.0, 0.0))) {
                renderSuggestConstraintsCursor(sugConstraints[2]);
                return;
            }
        }
        break;
        case SelectMode::SeekFourth:
        {
            double startAngleToDraw = startAngle;
            bool angleReversed = false;
            if (arcAngle > 0)
                endAngle = startAngle + arcAngle;
            else {
                endAngle = startAngle;
                startAngleToDraw = startAngle + arcAngle;
                angleReversed = true;
            }

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomArcOfCircle* arc1 = new Part::GeomArcOfCircle();
            arc1->setRange(startAngleToDraw, endAngle, true);
            arc1->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));

            if (constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                r = std::min(radius * 0.999, fabs(radius - (onSketchPos - centerPoint).Length()));
                arc1->setRadius(radius - r);
                geometriesToAdd.push_back(arc1);

                Part::GeomArcOfCircle* arc2 = new Part::GeomArcOfCircle();
                arc2->setRange(startAngleToDraw, endAngle, true);
                arc2->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                arc2->setRadius(radius + r);
                geometriesToAdd.push_back(arc2);

                Part::GeomArcOfCircle* arc3 = new Part::GeomArcOfCircle();
                arc3->setRange(M_PI + startAngleToDraw, 2 * M_PI + startAngleToDraw, true);
                if (angleReversed)
                    arc3->setCenter(Base::Vector3d(endPoint.x, endPoint.y, 0.));
                else
                    arc3->setCenter(Base::Vector3d(startPoint.x, startPoint.y, 0.));
                arc3->setRadius(r);
                geometriesToAdd.push_back(arc3);

                Part::GeomArcOfCircle* arc4 = new Part::GeomArcOfCircle();
                arc4->setRange(endAngle, M_PI + endAngle, true);
                if (angleReversed)
                    arc4->setCenter(Base::Vector3d(startPoint.x, startPoint.y, 0.));
                else
                    arc4->setCenter(Base::Vector3d(endPoint.x, endPoint.y, 0.));
                arc4->setRadius(r);
                geometriesToAdd.push_back(arc4);
            }
            else {
                r = std::max(0.001, (onSketchPos - centerPoint).Length());
                arc1->setRadius(radius);
                geometriesToAdd.push_back(arc1);

                Part::GeomArcOfCircle* arc2 = new Part::GeomArcOfCircle();
                arc2->setRange(startAngleToDraw, endAngle, true);
                arc2->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                arc2->setRadius(r);
                geometriesToAdd.push_back(arc2);

                Part::GeomLineSegment* line1 = new Part::GeomLineSegment();
                line1->setPoints(arc1->getStartPoint(), arc2->getStartPoint());
                geometriesToAdd.push_back(line1);

                Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
                line2->setPoints(arc1->getEndPoint(), arc2->getEndPoint());
                geometriesToAdd.push_back(line2);
            }

            drawEdit(geometriesToAdd);

            SbString text;
            text.sprintf(" (%.1fR)", (float)r);
            setPositionText(onSketchPos, text);
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        int firstCurve = getHighestCurveIndex() + 1;

        if (arcAngle > 0)
            endAngle = startAngle + arcAngle;
        else {
            endAngle = startAngle;
            startAngle = startAngle + arcAngle;
        }

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add slot"));
            if (constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                Gui::Command::doCommand(Gui::Command::Doc,
                    "geoList = []\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f, 0), App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f ,0), App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f ,0), App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f ,0), App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "%s.addGeometry(geoList, %s)\n"
                    "conList = []\n"
                    "conList.append(Sketcher.Constraint('Coincident', %i, 3, %i, 3))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, %i, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, %i, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, %i, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, %i, %i, %i))\n"
                    "%s.addConstraint(conList)\n"
                    "del geoList, conList\n",
                    centerPoint.x, centerPoint.y,         // center of the arc1
                    radius - r,                           // radius arc1
                    startAngle, endAngle,             // start and end angle of arc1
                    centerPoint.x, centerPoint.y,         // center of the arc2
                    radius + r,                           // radius arc2
                    startAngle, endAngle,             // start and end angle of arc2
                    startPoint.x, startPoint.y,           // center of the arc3
                    r,                                // radius arc3
                    (arcAngle > 0) ? startAngle + M_PI : endAngle,
                    (arcAngle > 0) ? startAngle + 2 * M_PI : endAngle + M_PI,    // start and end angle of arc3
                    endPoint.x, endPoint.y,               // center of the arc4
                    r,                                // radius arc4
                    (arcAngle > 0) ? endAngle : startAngle + M_PI,
                    (arcAngle > 0) ? endAngle + M_PI : startAngle + 2 * M_PI,        // start and end angle of arc4
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(), // the sketch
                    geometryCreationMode == Construction ? "True" : "False", // geometry as construction or not
                    firstCurve, firstCurve + 1,      // coicident1: mid of the two arcs
                    firstCurve, (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start,     //tangent1
                    firstCurve + 3, (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start,     //tangent1
                    firstCurve, (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end,     //tangent2
                    firstCurve + 2, (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end,     //tangent2
                    firstCurve + 1, (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start,     //tangent3
                    firstCurve + 3, (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end,     //tangent3
                    firstCurve + 1, (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end,     //tangent4
                    firstCurve + 2, (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start,     //tangent4
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch
            }
            else {
                std::vector<Part::Geometry*> geometriesToAdd;
                Part::GeomArcOfCircle* arc1 = new Part::GeomArcOfCircle();
                arc1->setRange(startAngle, endAngle, true);
                arc1->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                arc1->setRadius(radius);
                Sketcher::GeometryFacade::setConstruction(arc1, geometryCreationMode);
                geometriesToAdd.push_back(arc1);

                Part::GeomArcOfCircle* arc2 = new Part::GeomArcOfCircle();
                arc2->setRange(startAngle, endAngle, true);
                arc2->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                arc2->setRadius(r);
                Sketcher::GeometryFacade::setConstruction(arc2, geometryCreationMode);
                geometriesToAdd.push_back(arc2);

                Part::GeomLineSegment* line1 = new Part::GeomLineSegment();
                line1->setPoints(arc1->getStartPoint(), arc2->getStartPoint());
                Sketcher::GeometryFacade::setConstruction(line1, geometryCreationMode);
                geometriesToAdd.push_back(line1);

                Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
                line2->setPoints(arc1->getEndPoint(), arc2->getEndPoint());
                Sketcher::GeometryFacade::setConstruction(line2, geometryCreationMode);
                geometriesToAdd.push_back(line2);

                sketchgui->getSketchObject()->addGeometry(std::move(geometriesToAdd));

                Gui::Command::doCommand(Gui::Command::Doc,
                    "conList = []\n"
                    "conList.append(Sketcher.Constraint('Perpendicular', %i, 0, %i, 0))\n"
                    "conList.append(Sketcher.Constraint('Perpendicular', %i, 0, %i, 0))\n"
                    "conList.append(Sketcher.Constraint('Coincident', %i, 3, %i, 3))\n"
                    "conList.append(Sketcher.Constraint('Coincident', %i, 1, %i, 1))\n"
                    "conList.append(Sketcher.Constraint('Coincident', %i, 2, %i, 1))\n"
                    "conList.append(Sketcher.Constraint('Coincident', %i, 1, %i, 2))\n"
                    "conList.append(Sketcher.Constraint('Coincident', %i, 2, %i, 2))\n"
                    "%s.addConstraint(conList)\n"
                    "del conList\n",
                    firstCurve, firstCurve + 2,     // perpendicular1
                    firstCurve, firstCurve + 3,     // perpendicular2
                    firstCurve, firstCurve + 1,      // coicident1: mid of the two arcs
                    firstCurve, firstCurve + 2,     // coicident2
                    firstCurve, firstCurve + 3,     // coicident3
                    firstCurve + 1, firstCurve + 2, // coicident4
                    firstCurve + 1, firstCurve + 3, // coicident5
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch

            }
            Gui::Command::commitCommand();

        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add slot: %s\n", e.what());
            Gui::Command::abortCommand();

            tryAutoRecompute(static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
        }
    }

    virtual void createAutoConstraints() override {
        // Auto Constraint center point
        if (sugConstraints[0].size() > 0) {
            DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex() - 3, Sketcher::PointPos::mid);
            sugConstraints[0].clear();
        }

        if (constructionMethod() == ConstructionMethod::ArcSlot) {
            // Auto Constraint first picked point
            if (sugConstraints[1].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex() - 1, Sketcher::PointPos::mid);
                sugConstraints[1].clear();
            }

            // Auto Constraint second picked point
            if (sugConstraints[2].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[2], getHighestCurveIndex(), Sketcher::PointPos::mid);
                sugConstraints[2].clear();
            }
        }
        else {
            // Auto Constraint start point of first arc
            if (sugConstraints[1].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex() - 3, (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end);
                sugConstraints[1].clear();
            }

            // Auto Constraint end point of first arc
            if (sugConstraints[2].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[2], getHighestCurveIndex() - 3, (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start);
                sugConstraints[2].clear();
            }
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_ArcSlot";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        if(constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
            if (geometryCreationMode)
                return QString::fromLatin1("Sketcher_CreateArcSlot_Constr");
            else
                return QString::fromLatin1("Sketcher_CreateArcSlot");
        }
        else { // constructionMethod == DrawSketchHandlerArcSlot::ConstructionMethod::RectangleSlot
            if (geometryCreationMode)
                return QString::fromLatin1("Sketcher_CreateRectangleSlot_Constr");
            else
                return QString::fromLatin1("Sketcher_CreateRectangleSlot");
        }

        return QStringLiteral("None");
    }

private:
    SnapMode snapMode;
    Base::Vector2d centerPoint, startPoint, endPoint;
    double startAngle, endAngle, arcAngle, r, radius;
};

template <> auto DrawSketchHandlerArcSlotBase::ToolWidgetManager::getState(int parameterindex) const {
    switch (parameterindex) {
    case WParameter::First:
    case WParameter::Second:
        return SelectMode::SeekFirst;
        break;
    case WParameter::Third:
    case WParameter::Fourth:
        return SelectMode::SeekSecond;
        break;
    case WParameter::Fifth:
    case WParameter::Sixth:
        return SelectMode::SeekThird;
        break;
    default:
        THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template <> void DrawSketchHandlerArcSlotBase::ToolWidgetManager::configureToolWidget() {
    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Arc ends"), QStringLiteral("Flat ends")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_arcSlot", "x of center"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_arcSlot", "y of center"));
    toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_arcSlot", "Radius"));
    toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_arcSlot", "Start angle"));
    toolWidget->configureParameterUnit(WParameter::Fourth, Base::Unit::Angle);
    toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("TaskSketcherTool_p5_arcSlot", "Arc angle"));
    toolWidget->configureParameterUnit(WParameter::Fifth, Base::Unit::Angle);
    toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("TaskSketcherTool_p6_arcSlot", "Slot width"));

    toolWidget->setNoticeVisible(true);
    toolWidget->setNoticeText(QApplication::translate("TaskSketcherTool_notice_arcSlot", "Press Ctrl to snap angle at 5° steps."));
}

template <> void DrawSketchHandlerArcSlotBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    switch (parameterindex) {
    case WParameter::First:
        dHandler->centerPoint.x = value;
        break;
    case WParameter::Second:
        dHandler->centerPoint.y = value;
        break;
    case WParameter::Third:
        dHandler->radius = value;
        break;
    case WParameter::Fourth:
        dHandler->startAngle = value * M_PI / 180;
        break;
    case WParameter::Fifth:
        dHandler->arcAngle = value * M_PI / 180;
        break;
    case WParameter::Sixth:
        if (dHandler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot)
            dHandler->r = value/2;
        else
            dHandler->r = value;
        break;
    }
}

template <> void DrawSketchHandlerArcSlotBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

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
        double length = (onSketchPos - dHandler->centerPoint).Length();
        if (toolWidget->isParameterSet(WParameter::Third)) {
            dHandler->radius = toolWidget->getParameter(WParameter::Third);
            if (length != 0.) {
                onSketchPos = dHandler->centerPoint + (onSketchPos - dHandler->centerPoint) * dHandler->radius / length;
            }
        }
        if (toolWidget->isParameterSet(WParameter::Fourth)) {
            dHandler->startAngle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;
            onSketchPos.x = dHandler->centerPoint.x + cos(dHandler->startAngle) * length;
            onSketchPos.y = dHandler->centerPoint.y + sin(dHandler->startAngle) * length;
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Fifth)) {
            dHandler->arcAngle = toolWidget->getParameter(WParameter::Fifth) * M_PI / 180;
            double length = (onSketchPos - dHandler->centerPoint).Length();
            onSketchPos.x = dHandler->centerPoint.x + cos((dHandler->startAngle + dHandler->arcAngle)) * length;
            onSketchPos.y = dHandler->centerPoint.y + sin((dHandler->startAngle + dHandler->arcAngle)) * length;
        }
    }
    break;
    case SelectMode::SeekFourth:
    {
        if (toolWidget->isParameterSet(WParameter::Sixth)) {
            if (dHandler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                dHandler->r = toolWidget->getParameter(WParameter::Sixth) / 2;
            }
            else {
                dHandler->r = toolWidget->getParameter(WParameter::Sixth);
            }
            onSketchPos = dHandler->centerPoint + Base::Vector2d(dHandler->radius + dHandler->r, 0.);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerArcSlotBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
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
        if (!toolWidget->isParameterSet(WParameter::Third))
            toolWidget->updateVisualValue(WParameter::Third, dHandler->radius);

        if (!toolWidget->isParameterSet(WParameter::Fourth))
            toolWidget->updateVisualValue(WParameter::Fourth, dHandler->startAngle * 180 / M_PI, Base::Unit::Angle);
    }
    break;
    case SelectMode::SeekThird:
    {
        if (!toolWidget->isParameterSet(WParameter::Fifth))
            toolWidget->updateVisualValue(WParameter::Fifth, dHandler->arcAngle * 180 / M_PI, Base::Unit::Angle);
    }
    break;
    case SelectMode::SeekFourth:
    {
        if (!toolWidget->isParameterSet(WParameter::Sixth)) {
            if (dHandler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot)
                toolWidget->updateVisualValue(WParameter::Sixth, dHandler->r);
            else
                toolWidget->updateVisualValue(WParameter::Sixth, dHandler->r - dHandler->radius);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerArcSlotBase::ToolWidgetManager::addConstraints() {
    int firstCurve = handler->getHighestCurveIndex() - 3;
    using namespace Sketcher;

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);

    auto radiusSet = toolWidget->isParameterSet(WParameter::Third);
    auto arcAngleSet = toolWidget->isParameterSet(WParameter::Fifth);
    auto slotRadiusSet = toolWidget->isParameterSet(WParameter::Sixth);


    if (x0set && y0set && x0 == 0. && y0 == 0.) {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::RtPnt,
            x0, handler->sketchgui->getObject());
    }
    else {
        if (x0set)
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::VAxis,
                x0, handler->sketchgui->getObject());

        if (y0set)
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::HAxis,
                y0, handler->sketchgui->getObject());
    }

    if (radiusSet) {
        if (dHandler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                firstCurve, 3, firstCurve + 2, 3, dHandler->radius);
        else
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                firstCurve, dHandler->radius);
    }

    if (arcAngleSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ", firstCurve, fabs(dHandler->arcAngle));

    if (slotRadiusSet) {
        if(dHandler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                firstCurve + 2, dHandler->r);
        else
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
                firstCurve + 2, fabs(dHandler->radius - dHandler->r));
    }
}

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerArcSlot_H


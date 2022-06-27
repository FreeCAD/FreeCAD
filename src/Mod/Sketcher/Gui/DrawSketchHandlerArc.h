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


#include "GeometryCreationMode.h"
#include "Utils.h"

using namespace std;

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

class DrawSketchHandlerArc;

using DrawSketchHandlerArcBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerArc,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 3,
    /*WidgetParametersT =*/WidgetParameters<5, 6>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::CircleEllipseConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerArc : public DrawSketchHandlerArcBase
{
    friend DrawSketchHandlerArcBase;
public:
    enum SnapMode {
        Free,
        Snap5Degree
    };

    DrawSketchHandlerArc(ConstructionMethod constrMethod = ConstructionMethod::Center) :
        DrawSketchHandlerArcBase(constrMethod)
        , startAngle(0)
        , endAngle(0)
        , arcAngle(0) {}

    virtual ~DrawSketchHandlerArc() = default;

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
        }
        break;
        case SelectMode::SeekSecond:
        {
            if (constructionMethod() == ConstructionMethod::Center) {
                firstPoint = onSketchPos;
                double rx = firstPoint.x - centerPoint.x;
                double ry = firstPoint.y - centerPoint.y;
                startAngle = atan2(ry, rx);

                if (snapMode == SnapMode::Snap5Degree) {
                    startAngle = round(startAngle / (M_PI / 36)) * M_PI / 36;
                    firstPoint = centerPoint + radius * Base::Vector2d(cos(startAngle), sin(startAngle));
                }
            }
            else {
                centerPoint = (onSketchPos - firstPoint) / 2 + firstPoint;
                secondPoint = onSketchPos;
            }

            radius = (onSketchPos - centerPoint).Length();

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomCircle* circle = new Part::GeomCircle();
            circle->setRadius(radius);
            circle->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
            geometriesToAdd.push_back(circle);

            //add line to show the snap at 5 degree.
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            line->setPoints(Base::Vector3d(centerPoint.x, centerPoint.y, 0.),
                Base::Vector3d(centerPoint.x + cos(startAngle) * 0.8 * radius, centerPoint.y + sin(startAngle) * 0.8 * radius, 0.));
            geometriesToAdd.push_back(line);

            drawEdit(geometriesToAdd);

            double angle = GetPointAngle(centerPoint, onSketchPos);
            SbString text;
            text.sprintf(" (%.1fR,%.1fdeg)", (float)radius, (float)angle * 180 / M_PI);
            setPositionText(onSketchPos, text);

            if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[1]);
                return;
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            try
            {
                double startAngleToDraw = startAngle;
                if (constructionMethod() == ConstructionMethod::Center) {
                    double angle1 = atan2(onSketchPos.y - centerPoint.y,
                        onSketchPos.x - centerPoint.x) - startAngle;
                    double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
                    arcAngle = abs(angle1 - arcAngle) < abs(angle2 - arcAngle) ? angle1 : angle2;

                    if (snapMode == SnapMode::Snap5Degree) {
                        arcAngle = round(arcAngle / (M_PI / 36)) * M_PI / 36;
                    }

                    if (arcAngle > 0)
                        endAngle = startAngle + arcAngle;
                    else {
                        endAngle = startAngle;
                        startAngleToDraw = startAngle + arcAngle;
                    }
                }
                else {
                    /*Centerline inverts when the arc flips sides.  Easily taken care of by replacing
                    centerline with a point.  It happens because the direction the curve is being drawn
                    reverses.*/
                    centerPoint = Part::Geom2dCircle::getCircleCenter(firstPoint, secondPoint, onSketchPos);
                    radius = (onSketchPos - centerPoint).Length();

                    double angle1 = GetPointAngle(centerPoint, firstPoint);
                    double angle2 = GetPointAngle(centerPoint, secondPoint);
                    double angle3 = GetPointAngle(centerPoint, onSketchPos);

                    // Always build arc counter-clockwise
                    // Point 3 is between Point 1 and 2
                    if (angle3 > std::min(angle1, angle2) && angle3 < std::max(angle1, angle2)) {
                        if (angle2 > angle1) {
                            arcPos1 = Sketcher::PointPos::start;
                            arcPos2 = Sketcher::PointPos::end;
                        }
                        else {
                            swapPoints(firstPoint, secondPoint);
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
                            swapPoints(firstPoint, secondPoint);
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
                    startAngleToDraw = startAngle;
                }

                std::vector<Part::Geometry*> geometriesToAdd;
                Part::GeomArcOfCircle* arc = new Part::GeomArcOfCircle();
                arc->setRadius(radius);
                arc->setRange(startAngleToDraw, endAngle, true);
                arc->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                geometriesToAdd.push_back(arc);
                drawEdit(geometriesToAdd);

                SbString text;
                text.sprintf(" (%.1fR,%.1fdeg)", (float)radius, (float)arcAngle * 180 / M_PI);
                setPositionText(onSketchPos, text);

                if (constructionMethod() == ConstructionMethod::Center) {
                    if (seekAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.0, 0.0))) {
                        renderSuggestConstraintsCursor(sugConstraints[2]);
                        return;
                    }
                }
                else {
                    if (seekAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                        renderSuggestConstraintsCursor(sugConstraints[2]);
                        return;
                    }
                }
            }
            catch (Base::ValueError& e) {
                e.ReportException();
            }
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        unsetCursor();
        resetPositionText();

        if (constructionMethod() == ConstructionMethod::Center) {
            if (arcAngle > 0)
                endAngle = startAngle + arcAngle;
            else {
                endAngle = startAngle;
                startAngle += arcAngle;
            }
        }

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc"));
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.ArcOfCircle"
                "(Part.Circle(App.Vector(%f,%f,0),App.Vector(0,0,1),%f),%f,%f),%s)",
                centerPoint.x, centerPoint.y, radius,
                startAngle, endAngle,
                geometryCreationMode == Construction ? "True" : "False");

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add arc: %s\n", e.what());
            Gui::Command::abortCommand();
        }
    }

    virtual void createAutoConstraints() override {
        if (constructionMethod() == ConstructionMethod::Center) {
            // Auto Constraint center point
            if (sugConstraints[0].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex(), Sketcher::PointPos::mid);
                sugConstraints[0].clear();
            }

            // Auto Constraint first picked point
            if (sugConstraints[1].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex(), (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end);
                sugConstraints[1].clear();
            }

            // Auto Constraint second picked point
            if (sugConstraints[2].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[2], getHighestCurveIndex(), (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start);
                sugConstraints[2].clear();
            }
        }
        else {
            // Auto Constraint first picked point
            if (sugConstraints[0].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex(), arcPos1);
                sugConstraints[0].clear();
            }

            // Auto Constraint second picked point
            if (sugConstraints[1].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex(), arcPos2);
                sugConstraints[1].clear();
            }

            // Auto Constraint third picked point
            if (sugConstraints[2].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[2], getHighestCurveIndex(), Sketcher::PointPos::none);
                sugConstraints[2].clear();
            }
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_Arc";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        if (constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center)
            return QString::fromLatin1("Sketcher_Pointer_Create_Arc");
        else // constructionMethod == DrawSketchHandlerArc::ConstructionMethod::ThreeRim
            return QString::fromLatin1("Sketcher_Pointer_Create_3PointArc");

        return QStringLiteral("None");
    }

private:
    SnapMode snapMode;
    Base::Vector2d centerPoint, firstPoint, secondPoint;
    double radius, startAngle, endAngle, arcAngle;
    Sketcher::PointPos arcPos1, arcPos2;

    void swapPoints(Base::Vector2d& p1, Base::Vector2d& p2) {
        Base::Vector2d p3 = p1;
        p1 = p2;
        p2 = p3;
    }
};

template <> auto DrawSketchHandlerArcBase::ToolWidgetManager::getState(int parameterindex) const {
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

template <> void DrawSketchHandlerArcBase::ToolWidgetManager::configureToolWidget() {
    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Center"), QStringLiteral("3 rim points")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        syncConstructionMethodComboboxToHandler(); // in case the DSH was called with a specific construction method
    }

    if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_arc", "x of center"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_arc", "y of center"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_arc", "Radius"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_arc", "Start angle"));
        toolWidget->configureParameterUnit(WParameter::Fourth, Base::Unit::Angle);
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("TaskSketcherTool_p5_arc", "Arc angle"));
        toolWidget->configureParameterUnit(WParameter::Fifth, Base::Unit::Angle);

        toolWidget->setNoticeVisible(true);
        toolWidget->setNoticeText(QApplication::translate("TaskSketcherTool_p3_notice", "Press Ctrl to snap angles at 5° steps."));
    }
    else {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("ToolWidgetManager_p1", "x of 1st point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("ToolWidgetManager_p2", "y of 1st point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("ToolWidgetManager_p3", "x of 2nd point"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("ToolWidgetManager_p4", "y of 2nd point"));
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("ToolWidgetManager_p5", "x of 3rd point"));
        toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("ToolWidgetManager_p6", "y of 3rd point"));
    }
}

template <> void DrawSketchHandlerArcBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
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
        }
    }
    else { //if (constructionMethod == ConstructionMethod::ThreeRim)
        switch (parameterindex) {
        case WParameter::First:
            dHandler->firstPoint.x = value;
            break;
        case WParameter::Second:
            dHandler->firstPoint.y = value;
            break;
        case WParameter::Third:
            dHandler->secondPoint.x = value;
            break;
        case WParameter::Fourth:
            dHandler->secondPoint.y = value;
            break;
        }
    }
}

template <> void DrawSketchHandlerArcBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

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
        if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
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
        else {
            if (toolWidget->isParameterSet(WParameter::Third))
                onSketchPos.x = toolWidget->getParameter(WParameter::Third);

            if (toolWidget->isParameterSet(WParameter::Fourth))
                onSketchPos.y = toolWidget->getParameter(WParameter::Fourth);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
            if (toolWidget->isParameterSet(WParameter::Fifth)) {
                dHandler->arcAngle = toolWidget->getParameter(WParameter::Fifth) * M_PI / 180;
                double length = (onSketchPos - dHandler->centerPoint).Length();
                onSketchPos.x = dHandler->centerPoint.x + cos((dHandler->startAngle + dHandler->arcAngle)) * length;
                onSketchPos.y = dHandler->centerPoint.y + sin((dHandler->startAngle + dHandler->arcAngle)) * length;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Fifth))
                onSketchPos.x = toolWidget->getParameter(WParameter::Fifth);

            if (toolWidget->isParameterSet(WParameter::Sixth))
                onSketchPos.y = toolWidget->getParameter(WParameter::Sixth);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerArcBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
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
        if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, dHandler->radius);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, dHandler->startAngle * 180 / M_PI, Base::Unit::Angle);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, onSketchPos.y);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, dHandler->arcAngle * 180 / M_PI, Base::Unit::Angle);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Sixth))
                toolWidget->updateVisualValue(WParameter::Sixth, onSketchPos.y);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerArcBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
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

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition);

            if (toolWidget->isParameterSet(WParameter::Third) &&
                toolWidget->isParameterSet(WParameter::Fourth)) {

                handler->setState(SelectMode::SeekThird);

            }
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Fifth) &&
            dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition);

            handler->setState(SelectMode::End);
            handler->finish();
        }

        if ((toolWidget->isParameterSet(WParameter::Fifth) ||
            toolWidget->isParameterSet(WParameter::Sixth)) &&
            dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::ThreeRim) {

            doEnforceWidgetParameters(prevCursorPosition);
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

template <> void DrawSketchHandlerArcBase::ToolWidgetManager::addConstraints() {
    int firstCurve = handler->getHighestCurveIndex();
    using namespace Sketcher;

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);

    if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
        auto radiusSet = toolWidget->isParameterSet(WParameter::Third);
        auto arcAngleSet = toolWidget->isParameterSet(WParameter::Fifth);


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

        if (radiusSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ", firstCurve, dHandler->radius);

        if (arcAngleSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ", firstCurve, dHandler->arcAngle);
    }
    else {
        auto x1 = toolWidget->getParameter(WParameter::Third);
        auto y1 = toolWidget->getParameter(WParameter::Fourth);

        auto x1set = toolWidget->isParameterSet(WParameter::Third);
        auto y1set = toolWidget->isParameterSet(WParameter::Fourth);

        if (x0set && y0set && x0 == 0. && y0 == 0.) {
            ConstraintToAttachment(GeoElementId(firstCurve, dHandler->arcPos1), GeoElementId::RtPnt,
                x0, handler->sketchgui->getObject());
        }
        else {
            if (x0set)
                ConstraintToAttachment(GeoElementId(firstCurve, dHandler->arcPos1), GeoElementId::VAxis,
                    x0, handler->sketchgui->getObject());

            if (y0set)
                ConstraintToAttachment(GeoElementId(firstCurve, dHandler->arcPos1), GeoElementId::HAxis,
                    y0, handler->sketchgui->getObject());
        }
        if (x1set && y1set && x1 == 0. && y1 == 0.) {
            ConstraintToAttachment(GeoElementId(firstCurve, dHandler->arcPos2), GeoElementId::RtPnt,
                x1, handler->sketchgui->getObject());
        }
        else {
            if (x1set)
                ConstraintToAttachment(GeoElementId(firstCurve, dHandler->arcPos2), GeoElementId::VAxis,
                    x1, handler->sketchgui->getObject());

            if (y1set)
                ConstraintToAttachment(GeoElementId(firstCurve, dHandler->arcPos2), GeoElementId::HAxis,
                    y1, handler->sketchgui->getObject());
        }
    }
}

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerArc_H


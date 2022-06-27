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


#ifndef SKETCHERGUI_DrawSketchHandlerSlot_H
#define SKETCHERGUI_DrawSketchHandlerSlot_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp


class DrawSketchHandlerSlot;

using DrawSketchHandlerSlotBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerSlot,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 3,
    /*WidgetParametersT =*/WidgetParameters<5>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<0>>;

class DrawSketchHandlerSlot : public DrawSketchHandlerSlotBase
{
    friend DrawSketchHandlerSlotBase;
public:

    enum class SnapMode {
        Free,
        Snap5Degree
    };

    DrawSketchHandlerSlot() :
        radius(1)
        , angleIsSet(false), lengthIsSet(false)
        ,isHorizontal(false), isVertical(false) {}

    virtual ~DrawSketchHandlerSlot() = default;

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
            startPoint = onSketchPos;

            if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[0]);
                return;
            }
        }
        break;
        case SelectMode::SeekSecond:
        {
            secondPoint = onSketchPos;

            angle = GetPointAngle(startPoint, secondPoint);
            length = (secondPoint - startPoint).Length();
            radius = length / 5; //radius chosen at 1/5 of length

            if (!angleIsSet && snapMode == SnapMode::Snap5Degree) {
                angle = round(angle / (M_PI / 36)) * M_PI / 36;
                secondPoint = startPoint + length * Base::Vector2d(cos(angle), sin(angle));

                if (std::fmod(angle, M_PI) < Precision::Confusion())
                    isHorizontal = true;
                else if (std::fmod(angle, M_PI / 2) < Precision::Confusion())
                    isVertical = true;
            }

            drawEdit(createSlotGeometries());

            SbString text;
            text.sprintf(" (%.1fL)", length);
            setPositionText(onSketchPos, text);

            if ((isHorizontal || isVertical) && seekAutoConstraint(sugConstraints[1], onSketchPos, secondPoint - startPoint, AutoConstraint::VERTEX_NO_TANGENCY)) {
                renderSuggestConstraintsCursor(sugConstraints[1]);
                return;
            }
            else if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[1]);
                return;
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            /*To follow the cursor, r should adapt depending on the position of the cursor. If cursor is 'between' the center points,
            then its distance to that line and not distance to the second center.
            A is "between" B and C if angle ∠ABC and angle ∠ACB are both less than or equal to ninety degrees.
            An angle ∠ABC is greater than ninety degrees iff AB^2 + BC^2 < AC^2.*/

            double L1 = (onSketchPos - startPoint).Length();//distance between first center and onSketchPos
            double L2 = (onSketchPos - secondPoint).Length(); //distance between second center and onSketchPos

            if ((L1 * L1 + length * length > L2 * L2) && (L2 * L2 + length * length > L1 * L1)) {
                //distance of onSketchPos to the line StartPos-SecondPos
                radius = (abs((secondPoint.y - startPoint.y) * onSketchPos.x - (secondPoint.x - startPoint.x) * onSketchPos.y + secondPoint.x * startPoint.y - secondPoint.y * startPoint.x)) / length;
            }
            else {
                radius = std::min(L1, L2);
            }

            drawEdit(createSlotGeometries());

            SbString text;
            text.sprintf(" (%.1fR)", radius);
            setPositionText(onSketchPos, text);

            //Todo: we could add another auto constraint but we would need to know to which geometry to add it depending on mouse position.
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        int firstCurve = getHighestCurveIndex() + 1;

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add slot"));

            sketchgui->getSketchObject()->addGeometry(std::move(createSlotGeometries()));

            Gui::Command::doCommand(Gui::Command::Doc,
                "conList = []\n"
                "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, 1))\n"
                "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, 1))\n"
                "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, 2))\n"
                "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, 2))\n"
                "conList.append(Sketcher.Constraint('Equal', %i, %i))\n"
                "%s.addConstraint(conList)\n"
                "del conList\n",
                firstCurve, firstCurve + 2,     // tangent1
                firstCurve, firstCurve + 3,     // tangent2
                firstCurve + 1, firstCurve + 2, // tangent3
                firstCurve + 1, firstCurve + 3, // tangent4
                firstCurve, firstCurve + 1,     // equal constraint
                Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add slot: %s\n", e.what());
            Gui::Command::abortCommand();

            tryAutoRecompute(static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
        }
        angleIsSet = false;
        lengthIsSet = false;
    }

    virtual void createAutoConstraints() override {
        // add auto constraints at the center of the first arc
        if (sugConstraints[0].size() > 0) {
            DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex() - 3, Sketcher::PointPos::mid);
            sugConstraints[0].clear();
        }

        // add auto constraints at the center of the second arc
        if (sugConstraints[1].size() > 0) {
            if (isHorizontal || isVertical)
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex(), Sketcher::PointPos::none);
            else
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex() - 2, Sketcher::PointPos::mid);
            sugConstraints[1].clear();
        }
        isHorizontal = false;
        isVertical = false;
    }

    virtual std::string getToolName() const override {
        return "DSH_Slot";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Slot");
    }

private:
    SnapMode snapMode;
    Base::Vector2d startPoint, secondPoint;
    double radius, length, angle;
    bool angleIsSet, lengthIsSet, isHorizontal, isVertical;

    std::vector<Part::Geometry*> createSlotGeometries() {
        std::vector<Part::Geometry*> geometriesToAdd;

        Part::GeomArcOfCircle* arc1 = new Part::GeomArcOfCircle();
        arc1->setRadius(radius);
        arc1->setRange(M_PI / 2 + angle, 1.5 * M_PI + angle, true);
        arc1->setCenter(Base::Vector3d(startPoint.x, startPoint.y, 0.));
        Sketcher::GeometryFacade::setConstruction(arc1, geometryCreationMode);
        geometriesToAdd.push_back(arc1);

        Part::GeomArcOfCircle* arc2 = new Part::GeomArcOfCircle();
        arc2->setRadius(radius);
        arc2->setRange(1.5 * M_PI + angle, M_PI / 2 + angle, true);
        arc2->setCenter(Base::Vector3d(secondPoint.x, secondPoint.y, 0.));
        Sketcher::GeometryFacade::setConstruction(arc2, geometryCreationMode);
        geometriesToAdd.push_back(arc2);

        Part::GeomLineSegment* line1 = new Part::GeomLineSegment();
        line1->setPoints(arc1->getStartPoint(), arc2->getEndPoint());
        Sketcher::GeometryFacade::setConstruction(line1, geometryCreationMode);
        geometriesToAdd.push_back(line1);

        Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
        line2->setPoints(arc1->getEndPoint(), arc2->getStartPoint());
        Sketcher::GeometryFacade::setConstruction(line2, geometryCreationMode);
        geometriesToAdd.push_back(line2);

        return geometriesToAdd;
    }
};



template <> void DrawSketchHandlerSlotBase::ToolWidgetManager::configureToolWidget() {
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("ToolWidgetManager_p1", "x of 1st point"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("ToolWidgetManager_p2", "y of 1st point"));
    toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("ToolWidgetManager_p3", "Length"));
    toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("ToolWidgetManager_p4", "Angle to HAxis"));
    toolWidget->configureParameterUnit(WParameter::Fourth, Base::Unit::Angle);
    toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("ToolWidgetManager_p5", "Radius"));

    toolWidget->setNoticeVisible(true);
    toolWidget->setNoticeText(QApplication::translate("TaskSketcherTool_p3_notice", "Press Ctrl to snap angle at 5° steps."));
}

template <> void DrawSketchHandlerSlotBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {

    switch (parameterindex) {
    case WParameter::First:
        dHandler->startPoint.x = value;
        break;
    case WParameter::Second:
        dHandler->startPoint.y = value;
        break;
    case WParameter::Third:
        dHandler->length = value;
        dHandler->lengthIsSet = true;
        break;
    case WParameter::Fourth:
        dHandler->angle = value * M_PI / 180;
        dHandler->angleIsSet = true;
        break;
    }
}

template <> void DrawSketchHandlerSlotBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

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
        dHandler->length = (onSketchPos - dHandler->startPoint).Length();

        if (toolWidget->isParameterSet(WParameter::Third)) {
            dHandler->length = toolWidget->getParameter(WParameter::Third);
            Base::Vector2d v = onSketchPos - dHandler->startPoint;
            onSketchPos = dHandler->startPoint + v * dHandler->length / v.Length();
        }

        if (toolWidget->isParameterSet(WParameter::Fourth)) {
            dHandler->angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;
            onSketchPos.x = dHandler->startPoint.x + cos(dHandler->angle) * dHandler->length;
            onSketchPos.y = dHandler->startPoint.y + sin(dHandler->angle) * dHandler->length;
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Fifth)) {
            dHandler->radius = toolWidget->getParameter(WParameter::Fifth);
            onSketchPos.x = dHandler->secondPoint.x + cos(dHandler->angle) * dHandler->radius;
            onSketchPos.y = dHandler->secondPoint.y + sin(dHandler->angle) * dHandler->radius;
        }

    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerSlotBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
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
            toolWidget->updateVisualValue(WParameter::Third, dHandler->length);

        if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, dHandler->angle * 180 / M_PI, Base::Unit::Angle);
    }
    break;
    case SelectMode::SeekThird:
    {
        if (!toolWidget->isParameterSet(WParameter::Fifth))
            toolWidget->updateVisualValue(WParameter::Fifth, dHandler->radius);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerSlotBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->setState(SelectMode::SeekSecond);
        }
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third) ||
            toolWidget->isParameterSet(WParameter::Fourth)) {

            if (toolWidget->isParameterSet(WParameter::Third) &&
                toolWidget->isParameterSet(WParameter::Fourth)) {

                handler->setState(SelectMode::SeekThird);
            }
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Fifth)) {

            handler->setState(SelectMode::End);
            handler->finish();
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerSlotBase::ToolWidgetManager::addConstraints() {

    int firstCurve = handler->getHighestCurveIndex() - 3;

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);
    auto lengthSet = toolWidget->isParameterSet(WParameter::Third);
    auto angleSet = toolWidget->isParameterSet(WParameter::Fourth);
    auto radiusSet = toolWidget->isParameterSet(WParameter::Fifth);

    using namespace Sketcher;

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

    if (lengthSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
            firstCurve, 3, firstCurve + 1, 3, dHandler->length);

    if (angleSet) {
        if (fabs(dHandler->angle - M_PI) < Precision::Confusion() || fabs(dHandler->angle + M_PI) < Precision::Confusion() || fabs(dHandler->angle) < Precision::Confusion()) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Horizontal',%d)) ", firstCurve + 2);
        }
        else if (fabs(dHandler->angle - M_PI/2) < Precision::Confusion() || fabs(dHandler->angle + M_PI / 2) < Precision::Confusion()) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Vertical',%d)) ", firstCurve + 2);
        }
        else {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Angle',%d,%d,%f)) ",
                Sketcher::GeoEnum::HAxis, firstCurve + 2, dHandler->angle);
        }
    }

    if (radiusSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
            firstCurve, dHandler->radius);
}

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerSlot_H


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


#ifndef SKETCHERGUI_DrawSketchHandlerLine_H
#define SKETCHERGUI_DrawSketchHandlerLine_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

class DrawSketchHandlerLine;

namespace ConstructionMethods {

enum class LineConstructionMethod {
    OnePointLengthAngle,
    TwoPoints,
    End // Must be the last one
};

}

using DrawSketchHandlerLineBase = DrawSketchDefaultWidgetHandler<   DrawSketchHandlerLine,
                                                                    /*SelectModeT*/ StateMachines::TwoSeekEnd,
                                                                    /*PEditCurveSize =*/ 2,
                                                                    /*PAutoConstraintSize =*/ 2,
                                                                    /*WidgetParametersT =*/WidgetParameters<4, 4>,
                                                                    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
                                                                    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
                                                                    ConstructionMethods::LineConstructionMethod,
                                                                    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerLine: public DrawSketchHandlerLineBase
{
    friend DrawSketchHandlerLineBase; // allow DrawSketchHandlerRectangleBase specialisations access DrawSketchHandlerRectangle private members
public:
    DrawSketchHandlerLine(ConstructionMethod constrMethod = ConstructionMethod::OnePointLengthAngle): DrawSketchHandlerLineBase(constrMethod){};
    virtual ~DrawSketchHandlerLine() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch(state()) {
            case SelectMode::SeekFirst:
            {
                drawPositionAtCursor(onSketchPos);

                EditCurve[0] = onSketchPos;

                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f,0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }
            break;
            case SelectMode::SeekSecond:
            {
                drawDirectionAtCursor(onSketchPos, EditCurve[0]);

                EditCurve[1] = onSketchPos;

                drawEdit(EditCurve);

                if (seekAutoConstraint(sugConstraints[1], onSketchPos, onSketchPos - EditCurve[0])) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            }
            break;
            default:
                break;
        }
    }

    virtual void executeCommands() override {
        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch line"));
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)),%s)",
                        EditCurve[0].x,EditCurve[0].y,EditCurve[1].x,EditCurve[1].y,
                        geometryCreationMode==Construction?"True":"False");

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add line: %s\n", e.what());
            Gui::Command::abortCommand();
            THROWM(Base::RuntimeError, "Tool execution aborted\n") // This prevents constraints from being applied on non existing geometry
        }
    }

    virtual void generateAutoConstraints() override {
        int LineGeoId = getHighestCurveIndex();

        // Generate temporary autoconstraints (but do not actually add them to the sketch)
        if(avoidRedundants)
            removeRedundantHorizontalVertical(getSketchObject(),sugConstraints[0],sugConstraints[1]);

        auto & ac1 = sugConstraints[0];
        auto & ac2 = sugConstraints[1];

        generateAutoConstraintsOnElement(ac1, LineGeoId, Sketcher::PointPos::start);
        generateAutoConstraintsOnElement(ac2, LineGeoId, Sketcher::PointPos::end);

        // Ensure temporary autoconstraints do not generate a redundancy and that the geometry parameters are accurate
        // This is particularly important for adding widget mandated constraints.
        removeRedundantAutoConstraints();
    }

    virtual void createAutoConstraints() override {
        // execute python command to create autoconstraints
        createGeneratedAutoConstraints(true);

        sugConstraints[0].clear();
        sugConstraints[1].clear();
    }

    virtual std::string getToolName() const override {
        return "DSH_Line";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        if(constructionMethod() == ConstructionMethod::TwoPoints)
            return QString::fromLatin1("Sketcher_Pointer_Create_Line");
        else
            return QString::fromLatin1("Sketcher_Pointer_Create_Line_Polar");
    }

private:
    int LineGeoId;

};

// Function responsible for updating the DrawSketchHandler data members when widget parameters change
template <> void DrawSketchHandlerLineBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    if (handler->constructionMethod() == ConstructionMethod::OnePointLengthAngle) {
        switch(parameterindex) {
            case WParameter::First:
                handler->EditCurve[0].x = value;
                break;
            case WParameter::Second:
                handler->EditCurve[0].y = value;
                break;
            case WParameter::Third:
            {
                auto angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;
                auto endpoint = handler->EditCurve[0] + Base::Vector2d::FromPolar(value, angle);
                handler->EditCurve[1].x =  endpoint.x;
            }
                break;
            case WParameter::Fourth:
            {
                auto angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;
                auto endpoint = handler->EditCurve[0] + Base::Vector2d::FromPolar(value, angle);
                handler->EditCurve[1].y = endpoint.y;
            }
                break;
        }
    }
    else { // ConstructionMethod::TwoPoints
        switch(parameterindex) {
            case WParameter::First:
                handler->EditCurve[0].x = value;
                break;
            case WParameter::Second:
                handler->EditCurve[0].y = value;
                break;
            case WParameter::Third:
                handler->EditCurve[1].x = value;
                break;
            case WParameter::Fourth:
                handler->EditCurve[1].y = value;
                break;
        }
    }
}

template <> auto DrawSketchHandlerLineBase::ToolWidgetManager::getState(int parameterindex) const {
    switch (parameterindex) {
    case WParameter::First:
    case WParameter::Second:
        return SelectMode::SeekFirst;
        break;
    case WParameter::Third:
    case WParameter::Fourth:
        return SelectMode::SeekSecond;
        break;
    default:
        THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template <> void DrawSketchHandlerLineBase::ToolWidgetManager::configureToolWidget() {
    if (!init) { // Code to be executed only upon initialisation
        QStringList names = { QStringLiteral("Point, length, angle"), QStringLiteral("2 points") };
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    if (handler->constructionMethod() == ConstructionMethod::OnePointLengthAngle) {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_line", "x of 1st point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_line", "y of 1st point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_length_line", "Length"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_angle_line", "Angle"));
        toolWidget->configureParameterUnit(WParameter::Fourth, Base::Unit::Angle);
    }
    else {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_line", "x of 1st point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_line", "y of 1st point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_line", "x of 2nd point"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_line", "y of 2nd point"));
    }
}

template <> void DrawSketchHandlerLineBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {
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
        if (handler->constructionMethod() == ConstructionMethod::OnePointLengthAngle) {

            double length = (onSketchPos - dHandler->EditCurve[0]).Length();

            if (toolWidget->isParameterSet(WParameter::Third)) {
                length = toolWidget->getParameter(WParameter::Third);
                Base::Vector2d v = onSketchPos - dHandler->EditCurve[0];
                if( v.x < Precision::Confusion() && v.y < Precision::Confusion())
                    v.x = 1; // if direction cannot be determined, default to (1,0)
                onSketchPos = dHandler->EditCurve[0] + v * length / v.Length();
            }

            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                double angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;
                onSketchPos.x = dHandler->EditCurve[0].x + cos(angle) * length;
                onSketchPos.y = dHandler->EditCurve[0].y + sin(angle) * length;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Third)) {
                onSketchPos.x = toolWidget->getParameter(WParameter::Third);
            }
            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                onSketchPos.y = toolWidget->getParameter(WParameter::Fourth);
            }
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerLineBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
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
        if (handler->constructionMethod() == ConstructionMethod::OnePointLengthAngle) {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, (onSketchPos - handler->EditCurve[0]).Length());

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, (onSketchPos - handler->EditCurve[0]).Angle() * 180 / M_PI, Base::Unit::Angle);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth,onSketchPos.y);
        }
    }
    break;
    default:
        break;
    }
}

// Function responsible to add widget mandated constraints (it is executed before creating autoconstraints)
template <> void DrawSketchHandlerLineBase::ToolWidgetManager::addConstraints() {

    int firstCurve = handler->getHighestCurveIndex();

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);
    auto p3 = toolWidget->getParameter(WParameter::Third);
    auto p4 = toolWidget->getParameter(WParameter::Fourth);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);
    auto p3set = toolWidget->isParameterSet(WParameter::Third);
    auto p4set = toolWidget->isParameterSet(WParameter::Fourth);

    using namespace Sketcher;

    auto constraintx0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve,PointPos::start), GeoElementId::VAxis, x0, handler->sketchgui->getObject());
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve,PointPos::start), GeoElementId::HAxis, y0,  handler->sketchgui->getObject());
    };

    auto constraintp3length = [&]() {
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
                    firstCurve, p3);
    };

    auto constraintp3x = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::end), GeoElementId::VAxis, p3, handler->sketchgui->getObject());
    };

    auto constraintp4angle = [&]() {
        double angle = p4 / 180 * M_PI;
        if (fabs(angle - M_PI) < Precision::Confusion() || fabs(angle + M_PI) < Precision::Confusion() || fabs(angle) < Precision::Confusion()) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Horizontal',%d)) ", firstCurve);
        }
        else if (fabs(angle - M_PI / 2) < Precision::Confusion() || fabs(angle + M_PI / 2) < Precision::Confusion()) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Vertical',%d)) ", firstCurve);
        }
        else {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Angle',%d,%d,%f)) ",
                Sketcher::GeoEnum::HAxis, firstCurve, angle);
        }
    };

    auto constraintp4y = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::end), GeoElementId::HAxis, p4,  handler->sketchgui->getObject());
    };

    if(handler->AutoConstraints.empty()) { // No valid diagnosis. Every constraint can be added.
        if(x0set)
            constraintx0();

        if(y0set)
            constrainty0();

        if (handler->constructionMethod() == DrawSketchHandlerLine::ConstructionMethod::OnePointLengthAngle) {
            if (p3set)
                constraintp3length();

            if (p4set)
                constraintp4angle();
        }
        else {
            if(p3set)
                constraintp3x();

            if(p4set)
                constraintp4y();
        }
    }
    else { // Valid diagnosis. Must check which constraints may be added.
        auto startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::start));

        if(x0set && startpointinfo.isXDoF()) {
            constraintx0();

            handler->diagnoseWithAutoConstraints(); // ensure we have recalculated parameters after each constraint addition

            startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::start)); // get updated point position
        }

        if(y0set && startpointinfo.isYDoF()) {
            constrainty0();

            handler->diagnoseWithAutoConstraints(); // ensure we have recalculated parameters after each constraint addition

            startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::start)); // get updated point position
        }

        auto endpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::end));

        if (handler->constructionMethod() == DrawSketchHandlerLine::ConstructionMethod::OnePointLengthAngle) {

            int DoFs = startpointinfo.getDoFs();
            DoFs += endpointinfo.getDoFs();

            if (p3set && DoFs > 0) {
                constraintp3length();
                DoFs--;
            }

            if (p4set && DoFs > 0) {
                constraintp4angle();
            }
        }
        else {
            if(p3set && endpointinfo.isXDoF()) {
                constraintp3x();

                handler->diagnoseWithAutoConstraints(); // ensure we have recalculated parameters after each constraint addition

                startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::start)); // get updated point position
                endpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::end));
            }

            if(p4set && endpointinfo.isYDoF())
                constraintp4y();
        }
    }
}


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerLine_H


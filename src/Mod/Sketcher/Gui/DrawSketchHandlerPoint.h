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


#ifndef SKETCHERGUI_DrawSketchHandlerPoint_H
#define SKETCHERGUI_DrawSketchHandlerPoint_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

class DrawSketchHandlerPoint;

using DrawSketchHandlerPointBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerPoint,
    StateMachines::OneSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 1,
    /*WidgetParametersT =*/WidgetParameters<2>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<0>>;

class DrawSketchHandlerPoint : public DrawSketchHandlerPointBase
{
    friend DrawSketchHandlerPointBase;

public:

    DrawSketchHandlerPoint() = default;
    virtual ~DrawSketchHandlerPoint() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);

            editPoint = onSketchPos;

            if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[0]);
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
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch point"));
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                editPoint.x, editPoint.y);

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
                Base::Console().Error("Failed to add point: %s\n", e.what());
                Gui::Command::abortCommand();
            }
    }

    virtual void createAutoConstraints() override {

        if (!sugConstraints[0].empty()) {
            DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex(), Sketcher::PointPos::start);
            sugConstraints[0].clear();
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_Point";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Create_Point");
    }

private:
    Base::Vector2d editPoint;
};

template <> auto DrawSketchHandlerPointBase::ToolWidgetManager::getState(int parameterindex) const {
    switch (parameterindex) {
    case WParameter::First:
    case WParameter::Second:
        return SelectMode::SeekFirst;
        break;
    default:
        THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::configureToolWidget() {
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_point", "x of point"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_point", "y of point"));
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    switch (parameterindex) {
    case WParameter::First:
        dHandler->editPoint.x = value;
        break;
    case WParameter::Second:
        dHandler->editPoint.y = value;
        break;
    }
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

            handler->setState(SelectMode::End);
            handler->finish();
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::addConstraints() {
    int firstCurve = handler->getHighestCurveIndex();

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);

    using namespace Sketcher;

    if (x0set && y0set && x0 == 0. && y0 == 0.) {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::RtPnt,
            x0, handler->sketchgui->getObject());
    }
    else {
            if (x0set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::VAxis,
                    x0, handler->sketchgui->getObject());

            if (y0set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::HAxis,
                    y0, handler->sketchgui->getObject());
    }
}


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerPoint_H


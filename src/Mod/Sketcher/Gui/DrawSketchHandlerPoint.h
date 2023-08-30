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

#include <Gui/Notifications.h>
#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerPoint;

using DrawSketchHandlerPointBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerPoint,
    StateMachines::OneSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 1,
    /*OnViewParametersT =*/OnViewParameters<2>,
    /*WidgetParametersT =*/WidgetParameters<0>,
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
        catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                    QT_TRANSLATE_NOOP("Notifications", "Error"),
                    QT_TRANSLATE_NOOP("Notifications", "Failed to add point"));

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

template <> auto DrawSketchHandlerPointBase::ToolWidgetManager::getState(int labelindex) const {
    switch (labelindex) {
    case WLabel::First:
    case WLabel::Second:
        return SelectMode::SeekFirst;
        break;
    default:
        THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::beforeFirstMouseMove(Base::Vector2d onSketchPos) {
    onViewParameters[WLabel::First]->activate();
    onViewParameters[WLabel::Second]->activate();
    onViewParameters[WLabel::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[WLabel::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);
    bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
    onViewParameters[WLabel::First]->setLabelAutoDistanceReverse(!sameSign);
    onViewParameters[WLabel::Second]->setLabelAutoDistanceReverse(sameSign);
    onViewParameters[WLabel::First]->setPoints(Base::Vector3d(0., 0., 0.), Base::Vector3d(onSketchPos.x, onSketchPos.y, 0.));
    onViewParameters[WLabel::Second]->setPoints(Base::Vector3d(0., 0., 0.), Base::Vector3d(onSketchPos.x, onSketchPos.y, 0.));

    onViewParameters[WLabel::Second]->startEdit(onSketchPos.y);
    onViewParameters[WLabel::First]->startEdit(onSketchPos.x);
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::adaptDrawingToLabelChange(int labelindex, double value) {
    switch (labelindex) {
    case WLabel::First:
        dHandler->editPoint.x = value;
        break;
    case WLabel::Second:
        dHandler->editPoint.y = value;
        break;
    }
    onViewParameters[WLabel::First]->setPoints(Base::Vector3d(0., 0., 0.), Base::Vector3d(dHandler->editPoint.x, dHandler->editPoint.y, 0.));
    onViewParameters[WLabel::Second]->setPoints(Base::Vector3d(0., 0., 0.), Base::Vector3d(dHandler->editPoint.x, dHandler->editPoint.y, 0.));
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (onViewParameters[WLabel::First]->isSet)
            onSketchPos.x = onViewParameters[WLabel::First]->getValue();

        if (onViewParameters[WLabel::Second]->isSet)
            onSketchPos.y = onViewParameters[WLabel::Second]->getValue();
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
        if (!onViewParameters[WLabel::First]->isSet) {
            onViewParameters[WLabel::First]->setSpinboxValue(onSketchPos.x);
        }

        if (!onViewParameters[WLabel::Second]->isSet) {
            onViewParameters[WLabel::Second]->setSpinboxValue(onSketchPos.y);
        }

        bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
        onViewParameters[WLabel::First]->setLabelAutoDistanceReverse(!sameSign);
        onViewParameters[WLabel::Second]->setLabelAutoDistanceReverse(sameSign);
        onViewParameters[WLabel::First]->setPoints(Base::Vector3d(0., 0., 0.), Base::Vector3d(onSketchPos.x, onSketchPos.y, 0.));
        onViewParameters[WLabel::Second]->setPoints(Base::Vector3d(0., 0., 0.), Base::Vector3d(onSketchPos.x, onSketchPos.y, 0.));
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
        if (onViewParameters[WLabel::First]->isSet &&
            onViewParameters[WLabel::Second]->isSet) {

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

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::onHandlerModeChanged() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        //Do nothing.
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::addConstraints() {
    int firstCurve = handler->getHighestCurveIndex();

    auto x0 = onViewParameters[WLabel::First]->getValue();
    auto y0 = onViewParameters[WLabel::Second]->getValue();

    auto x0set = onViewParameters[WLabel::First]->isSet;
    auto y0set = onViewParameters[WLabel::Second]->isSet;

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


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerPoint_H

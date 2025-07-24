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

#ifndef SKETCHERGUI_DrawSketchHandlerNote_H
#define SKETCHERGUI_DrawSketchHandlerNote_H

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <QInputDialog>
#include <QLineEdit>

#include <Mod/Sketcher/App/SketchObject.h>

#include "GeometryCreationMode.h"

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerNote;

using DSHNoteController = DrawSketchController<DrawSketchHandlerNote,
                                               StateMachines::OneSeekEnd,
                                               /*PAutoConstraintSize =*/1,
                                               /*OnViewParametersT =*/OnViewParameters<2>>;

using DrawSketchHandlerNoteBase = DrawSketchControllableHandler<DSHNoteController>;

class DrawSketchHandlerNote: public DrawSketchHandlerNoteBase
{
    // Allow specialisations of controllers access to private members
    friend DSHNoteController;

public:
    DrawSketchHandlerNote() = default;
    ~DrawSketchHandlerNote() override = default;

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);

                editNote = onSketchPos;

                seekAndRenderAutoConstraint(sugConstraints[0],
                                            onSketchPos,
                                            Base::Vector2d(0.f, 0.f));
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        try {
            bool ok;
            QString text = QInputDialog::getText(nullptr,
                                                 QObject::tr("Create Note"),
                                                 QObject::tr("Note text:"),
                                                 QLineEdit::Normal,
                                                 QString(),
                                                 &ok);
            if (!ok || text.isEmpty()) {
                Gui::Command::abortCommand();
                return;
            }
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch note"));
            Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                  "addGeometry(Part.Note(App.Vector(%f,%f,0), '%s'), %s)",
                                  editNote.x,
                                  editNote.y,
                                  text.toStdString().c_str(),
                                  isConstructionMode() ? "True" : "False");
            Gui::Command::commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(sketchgui,
                             QT_TRANSLATE_NOOP("Notifications", "Error"),
                             QT_TRANSLATE_NOOP("Notifications", "Failed to add note"));

            Gui::Command::abortCommand();
        }
    }

    void createAutoConstraints() override
    {

        if (!sugConstraints[0].empty()) {
            DrawSketchHandler::createAutoConstraints(sugConstraints[0],
                                                     getHighestCurveIndex(),
                                                     Sketcher::PointPos::start);
            sugConstraints[0].clear();
        }
    }

    std::string getToolName() const override
    {
        return "DSH_Note";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Text");
    }

    std::unique_ptr<QWidget> createWidget() const override
    {
        return std::make_unique<SketcherToolDefaultWidget>();
    }

private:
    Base::Vector2d editNote;
};

template<>
auto DSHNoteController::getState(int labelindex) const
{
    switch (labelindex) {
        case OnViewParameter::First:
        case OnViewParameter::Second:
            return SelectMode::SeekFirst;
            break;
        default:
            THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template<>
void DSHNoteController::configureOnViewParameters()
{
    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);
}

template<>
void DSHNoteController::adaptDrawingToOnViewParameterChange(int labelindex, double value)
{
    switch (labelindex) {
        case OnViewParameter::First:
            handler->editNote.x = value;
            break;
        case OnViewParameter::Second:
            handler->editNote.y = value;
            break;
    }
    onViewParameters[OnViewParameter::First]->setPoints(
        Base::Vector3d(0., 0., 0.),
        Base::Vector3d(handler->editNote.x, handler->editNote.y, 0.));
    onViewParameters[OnViewParameter::Second]->setPoints(
        Base::Vector3d(0., 0., 0.),
        Base::Vector3d(handler->editNote.x, handler->editNote.y, 0.));
}

template<>
void DSHNoteController::doEnforceControlParameters(Base::Vector2d& onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (onViewParameters[OnViewParameter::First]->isSet) {
                onSketchPos.x = onViewParameters[OnViewParameter::First]->getValue();
            }

            if (onViewParameters[OnViewParameter::Second]->isSet) {
                onSketchPos.y = onViewParameters[OnViewParameter::Second]->getValue();
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHNoteController::adaptParameters(Base::Vector2d onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (!onViewParameters[OnViewParameter::First]->isSet) {
                setOnViewParameterValue(OnViewParameter::First, onSketchPos.x);
            }

            if (!onViewParameters[OnViewParameter::Second]->isSet) {
                setOnViewParameterValue(OnViewParameter::Second, onSketchPos.y);
            }

            bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
            onViewParameters[OnViewParameter::First]->setLabelAutoDistanceReverse(!sameSign);
            onViewParameters[OnViewParameter::Second]->setLabelAutoDistanceReverse(sameSign);
            onViewParameters[OnViewParameter::First]->setPoints(
                Base::Vector3d(0., 0., 0.),
                Base::Vector3d(onSketchPos.x, onSketchPos.y, 0.));
            onViewParameters[OnViewParameter::Second]->setPoints(
                Base::Vector3d(0., 0., 0.),
                Base::Vector3d(onSketchPos.x, onSketchPos.y, 0.));
        } break;
        default:
            break;
    }
}

template<>
void DSHNoteController::doChangeDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (onViewParameters[OnViewParameter::First]->isSet
                && onViewParameters[OnViewParameter::Second]->isSet) {

                handler->setState(SelectMode::End);
                // handler->finish(); // Called by the change of mode
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHNoteController::addConstraints()
{
    int firstCurve = handler->getHighestCurveIndex();

    auto x0 = onViewParameters[OnViewParameter::First]->getValue();
    auto y0 = onViewParameters[OnViewParameter::Second]->getValue();

    auto x0set = onViewParameters[OnViewParameter::First]->isSet;
    auto y0set = onViewParameters[OnViewParameter::Second]->isSet;

    using namespace Sketcher;

    if (x0set && y0set && x0 == 0. && y0 == 0.) {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start),
                               GeoElementId::RtPnt,
                               x0,
                               handler->sketchgui->getObject());
    }
    else {
        if (x0set) {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start),
                                   GeoElementId::VAxis,
                                   x0,
                                   handler->sketchgui->getObject());
        }

        if (y0set) {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start),
                                   GeoElementId::HAxis,
                                   y0,
                                   handler->sketchgui->getObject());
        }
    }
}

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerNote_H

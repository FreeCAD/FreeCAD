// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <QApplication>

#include <Gui/Notifications.h>
#include <Gui/Selection/SelectionFilter.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/MDIView.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchHandler.h"
#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "SnapManager.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class CarbonCopySelection: public Gui::SelectionFilterGate
{
    App::DocumentObject* object;

public:
    explicit CarbonCopySelection(App::DocumentObject* obj)
        : Gui::SelectionFilterGate(nullPointer())
        , object(obj)
    {}

    bool allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName) override
    {
        Q_UNUSED(sSubName);

        Sketcher::SketchObject* sketch = static_cast<Sketcher::SketchObject*>(object);
        sketch->setAllowOtherBody(
            QApplication::keyboardModifiers() == Qt::ControlModifier
            || QApplication::keyboardModifiers() == (Qt::ControlModifier | Qt::AltModifier)
        );
        sketch->setAllowUnaligned(
            QApplication::keyboardModifiers() == (Qt::ControlModifier | Qt::AltModifier)
        );

        this->notAllowedReason = "";
        Sketcher::SketchObject::eReasonList msg;
        // Reusing code: All good reasons not to allow a carbon copy
        bool xinv = false, yinv = false;
        if (!sketch->isCarbonCopyAllowed(pDoc, pObj, xinv, yinv, &msg)) {
            switch (msg) {
                case Sketcher::SketchObject::rlCircularReference:
                    this->notAllowedReason = QT_TR_NOOP(
                        "Carbon copy would cause a circular dependency."
                    );
                    break;
                case Sketcher::SketchObject::rlOtherDoc:
                    this->notAllowedReason = QT_TR_NOOP("This object is in another document.");
                    break;
                case Sketcher::SketchObject::rlOtherBody:
                    this->notAllowedReason = QT_TR_NOOP(
                        "This object belongs to another body. Hold "
                        "Ctrl to allow cross-references."
                    );
                    break;
                case Sketcher::SketchObject::rlOtherBodyWithLinks:
                    this->notAllowedReason = QT_TR_NOOP(
                        "This object belongs to another body and it contains external "
                        "geometry. Cross-reference not allowed."
                    );
                    break;
                case Sketcher::SketchObject::rlOtherPart:
                    this->notAllowedReason = QT_TR_NOOP("This object belongs to another part.");
                    break;
                case Sketcher::SketchObject::rlNonParallel:
                    this->notAllowedReason = QT_TR_NOOP(
                        "The selected sketch is not parallel to this sketch. Hold "
                        "Ctrl+Alt to allow non-parallel sketches."
                    );
                    break;
                case Sketcher::SketchObject::rlAxesMisaligned:
                    this->notAllowedReason = QT_TR_NOOP(
                        "The XY axes of the selected sketch do not have the same "
                        "direction as this sketch. Hold Ctrl+Alt to disregard it."
                    );
                    break;
                case Sketcher::SketchObject::rlOriginsMisaligned:
                    this->notAllowedReason = QT_TR_NOOP(
                        "The origin of the selected sketch is not aligned with the "
                        "origin of this sketch. Hold Ctrl+Alt to disregard it."
                    );
                    break;
                default:
                    break;
            }
            return false;
        }
        // Carbon copy only works on sketches that are not disallowed (e.g. would produce a circular
        // reference)
        return true;
    }
};

class DrawSketchHandlerCarbonCopy: public DrawSketchHandler
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerCarbonCopy)

public:
    DrawSketchHandlerCarbonCopy() = default;
    ~DrawSketchHandlerCarbonCopy() override
    {
        Gui::Selection().rmvSelectionGate();
    }

    void mouseMove(SnapManager::SnapHandle snapHandle) override
    {
        Q_UNUSED(snapHandle);
        if (Gui::Selection().getPreselection().pObjectName) {
            applyCursor();
        }
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        /* this is ok not to call to purgeHandler
         * in continuous creation mode because the
         * handler is destroyed by the quit() method on pressing the
         * right button of the mouse */
        return true;
    }

    bool onSelectionChanged(const Gui::SelectionChanges& msg) override
    {
        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            App::DocumentObject* obj = sketchgui->getObject()->getDocument()->getObject(
                msg.pObjectName
            );
            if (!obj) {
                throw Base::ValueError("Sketcher: Carbon Copy: Invalid object in selection");
            }

            std::string sketchArchType("Sketcher::SketchObjectPython");

            if (obj->is<Sketcher::SketchObject>() || sketchArchType == obj->getTypeId().getName()) {

                try {
                    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create a carbon copy"));
                    Gui::cmdAppObjectArgs(
                        sketchgui->getObject(),
                        "carbonCopy(\"%s\",%s)",
                        msg.pObjectName,
                        constructionModeAsBooleanText()
                    );

                    Gui::Command::commitCommand();

                    tryAutoRecomputeIfNotSolve(sketchgui->getObject<Sketcher::SketchObject>());

                    Gui::Selection().clearSelection();
                    /* this is ok not to call to purgeHandler
                     * in continuous creation mode because the
                     * handler is destroyed by the quit() method on pressing the
                     * right button of the mouse */
                }
                catch (const Base::Exception&) {
                    Gui::NotifyError(
                        sketchgui,
                        QT_TRANSLATE_NOOP("Notifications", "Error"),
                        QT_TRANSLATE_NOOP("Notifications", "Failed to add carbon copy")
                    );
                    Gui::Command::abortCommand();
                }
                return true;
            }
        }
        updateHint();
        return false;
    }

private:
    void activated() override
    {
        setAxisPickStyle(false);
        Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
        Gui::View3DInventorViewer* viewer;
        viewer = static_cast<Gui::View3DInventor*>(mdi)->getViewer();
        viewer->setSelectionEnabled(true);

        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new CarbonCopySelection(sketchgui->getObject()));
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_CarbonCopy");
    }

    void deactivated() override
    {
        Q_UNUSED(sketchgui);
        setAxisPickStyle(true);
    }

public:
    std::list<Gui::InputHint> getToolHints() const override
    {
        return {
            {tr("%1 pick sketch to copy", "Sketcher CarbonCopy: hint"),
             {Gui::InputHint::UserInput::MouseLeft}},
        };
    }
};

}  // namespace SketcherGui

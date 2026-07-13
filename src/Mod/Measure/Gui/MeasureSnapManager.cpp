// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Krrish777 <777krrish[at]gmail.com>                 *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include <cstring>
#include <vector>

#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Mod/Measure/App/MeasureSnap.h>

#include "MeasureSnapManager.h"
#include "MeasureSnapIndicator.h"

using namespace MeasureGui;

MeasureSnapManager::MeasureSnapManager()
    : mIndicator(std::make_unique<MeasureSnapIndicator>())
{
    // Let go of a closing document's scene graph so the overlay does not pin it alive.
    mDeleteDocConn = Gui::Application::Instance->signalDeleteDocument.connect(
        [this](const Gui::Document&) { mIndicator->detach(); }
    );
}

MeasureSnapManager::~MeasureSnapManager()
{
    mDeleteDocConn.disconnect();
}

void MeasureSnapManager::onPreselect(const Gui::SelectionChanges& msg)
{
    try {
        if (msg.Type == Gui::SelectionChanges::RmvPreselect) {
            mIndicator->hide();
            return;
        }
        // Hover can come from a non-active document's view, but the overlay attaches
        // to the active view; previewing there would draw into the wrong scene.
        Gui::Document* activeDoc = Gui::Application::Instance->activeDocument();
        if (!activeDoc || !activeDoc->getDocument() || !msg.pDocName
            || std::strcmp(activeDoc->getDocument()->getName(), msg.pDocName) != 0) {
            mIndicator->hide();
            return;
        }
        const TopoDS_Shape shape = Measure::MeasureSnap::resolveShape(msg.Object);
        const int flags = Measure::MeasureSnap::getAvailableSnapTypes(shape);
        // Mode is fixed to Auto until the panel exposes a snap-mode control.
        const Measure::MeasureSnapMode type
            = Measure::MeasureSnap::pickPreviewType(flags, Measure::MeasureSnapMode::Auto);
        mIndicator->show(Measure::MeasureSnap::previewPoints(shape, type), type);
    }
    catch (const Base::Exception& e) {
        Base::Console().log("MeasureSnapManager: %s\n", e.what());
    }
    catch (...) {
        Base::Console().log("MeasureSnapManager: unexpected error on preselect\n");
    }
}

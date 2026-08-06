// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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


#include "PreCompiled.h"

#include <App/Document.h>
#include <Gui/Command.h>
#include <Gui/Document.h>

#include "TaskDlgEditSketch3D.h"
#include "TaskSketcher3DTool.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;


TaskDlgEditSketch3D::TaskDlgEditSketch3D(ViewProviderSketch3D* view)
    : TaskDialog()
    , sketchView(view)
    , toolPanel(nullptr)
{
    assert(sketchView);

    toolPanel = new TaskSketcher3DTool(sketchView);
    Content.push_back(toolPanel);

    if (auto* obj = sketchView->getObject()) {
        associateToObject3dView(obj);
    }
}

TaskDlgEditSketch3D::~TaskDlgEditSketch3D() = default;

void TaskDlgEditSketch3D::open()
{}

bool TaskDlgEditSketch3D::accept()
{
    return true;
}

bool TaskDlgEditSketch3D::reject()
{
    if (!sketchView) {
        return true;
    }
    auto* obj = sketchView->getObject();
    if (!obj || !obj->getDocument()) {
        return true;
    }
    const std::string docName = obj->getDocument()->getName();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.getDocument('%s').resetEdit()", docName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.getDocument('%s').recompute()", docName.c_str());
    return true;
}

void TaskDlgEditSketch3D::autoClosedOnClosedView()
{
    reject();
}

QDialogButtonBox::StandardButtons TaskDlgEditSketch3D::getStandardButtons() const
{
    return QDialogButtonBox::Close;
}

#include "moc_TaskDlgEditSketch3D.cpp"

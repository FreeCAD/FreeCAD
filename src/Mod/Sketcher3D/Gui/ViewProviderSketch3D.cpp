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

#include <QMenu>

#include <Base/Console.h>
#include <Gui/Control.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ToolBarManager.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "TaskDlgEditSketch3D.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;

PROPERTY_SOURCE(Sketcher3DGui::ViewProviderSketch3D, PartGui::ViewProviderPart)

namespace
{
inline QStringList editModeToolbarNames() {
    return {QStringLiteral("Sketcher3D Edit")};
}

inline QStringList nonEditModeToolbarNames() {
    return {
        QStringLiteral("Structure"),
        QStringLiteral("Sketcher"),
    };
}
}  // namespace

ViewProviderSketch3D::ViewProviderSketch3D() = default;

ViewProviderSketch3D::~ViewProviderSketch3D() = default;

Sketcher3D::Sketch3DObject* ViewProviderSketch3D::getSketch3DObject() const
{
    return static_cast<Sketcher3D::Sketch3DObject*>(getObject());
}

void ViewProviderSketch3D::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    menu->addAction(tr("Edit 3D Sketch"), receiver, member);
    Gui::ViewProvider::setupContextMenu(menu, receiver, member);
}

bool ViewProviderSketch3D::setEdit(int ModNum)
{
    if (ModNum != ViewProviderSketch3D::Default) {
        return PartGui::ViewProviderPart::setEdit(ModNum);
    }

    if (Gui::Control().activeDialog()) {
        Base::Console().warning(
            "Sketcher3D: cannot enter edit mode while another task dialog is open.\n"
        );
        return false;
    }

    Gui::Selection().clearSelection();
    Gui::Selection().rmvPreselect();

    // save non edit toolbar state and switch to edit mode toolbars
    Gui::ToolBarManager::getInstance()->setState(
        nonEditModeToolbarNames(),
        Gui::ToolBarManager::State::SaveState
    );
    Gui::ToolBarManager::getInstance()->setState(
        editModeToolbarNames(),
        Gui::ToolBarManager::State::ForceAvailable
    );
    Gui::ToolBarManager::getInstance()->setState(
        nonEditModeToolbarNames(),
        Gui::ToolBarManager::State::ForceHidden
    );

    auto* dlg = new TaskDlgEditSketch3D(this);
    Gui::Control().showDialog(dlg);

    return true;
}

void ViewProviderSketch3D::unsetEdit(int ModNum)
{
    if (ModNum != ViewProviderSketch3D::Default) {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
        return;
    }

    Gui::ToolBarManager::getInstance()->setState(
        editModeToolbarNames(),
        Gui::ToolBarManager::State::RestoreDefault
    );
    Gui::ToolBarManager::getInstance()->setState(
        nonEditModeToolbarNames(),
        Gui::ToolBarManager::State::RestoreDefault
    );

    Gui::Control().closeDialog();
}

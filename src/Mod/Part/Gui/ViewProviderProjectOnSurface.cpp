// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2019 Manuel Apeltauer, direkt cnc-systeme GmbH          *
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QAction>
#include <QMenu>
#endif

#include "ViewProviderProjectOnSurface.h"
#include "DlgProjectionOnSurface.h"
#include <Gui/Control.h>


using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderProjectOnSurface, PartGui::ViewProviderPart)


ViewProviderProjectOnSurface::ViewProviderProjectOnSurface()
{
    const unsigned int color = 0x8ae23400;
    LineColor.setValue(color);
    ShapeAppearance.setDiffuseColor(color);
    PointColor.setValue(color);
    Transparency.setValue(0);
}

ViewProviderProjectOnSurface::~ViewProviderProjectOnSurface() = default;

void ViewProviderProjectOnSurface::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act = menu->addAction(QObject::tr("Edit projection"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));

    ViewProviderPart::setupContextMenu(menu, receiver, member);
}

bool ViewProviderProjectOnSurface::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        if (Gui::Control().activeDialog()) {
            return false;
        }

        if (auto feature = getObject<Part::ProjectOnSurface>()) {
            Gui::Control().showDialog(new TaskProjectOnSurface(feature));
            return true;
        }

        return false;
    }

    return ViewProviderPart::setEdit(ModNum);
}

void ViewProviderProjectOnSurface::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderPart::unsetEdit(ModNum);
    }
}

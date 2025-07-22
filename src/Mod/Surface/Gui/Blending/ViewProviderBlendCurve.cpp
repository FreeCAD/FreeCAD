/***************************************************************************
 *   Copyright (c) 2022 Matteo Grellier <matteogrellier@gmail.com>         *
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
#ifndef _PreComp_
#include <QMenu>
#endif

#include "ViewProviderBlendCurve.h"
#include "TaskBlendCurve.h"
#include <Gui/ActionFunction.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>

PROPERTY_SOURCE(SurfaceGui::ViewProviderBlendCurve, PartGui::ViewProviderSpline)

namespace SurfaceGui
{

QIcon ViewProviderBlendCurve::getIcon() const
{
    return Gui::BitmapFactory().pixmap("Surface_BlendCurve");
}

void ViewProviderBlendCurve::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    auto func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(
        QObject::tr("Edit %1").arg(QString::fromUtf8(getObject()->Label.getValue())));
    act->setData(QVariant((int)ViewProvider::Default));
    func->trigger(act, [this]() {
        this->startDefaultEditMode();
    });

    ViewProviderSpline::setupContextMenu(menu, receiver, member);
}

bool ViewProviderBlendCurve::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        if (Gui::Control().activeDialog()) {
            return false;
        }
        auto dlg = new TaskBlendCurve(this);
        Gui::Control().showDialog(dlg);
        return true;
    }

    return ViewProviderSpline::setEdit(ModNum);
}

void ViewProviderBlendCurve::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderSpline::unsetEdit(ModNum);
    }
}

}  // namespace SurfaceGui

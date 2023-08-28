/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QAction>
# include <QMenu>
#endif

#include <Gui/ActionFunction.h>
#include <Gui/Control.h>
#include <Mod/Part/App/PrimitiveFeature.h>

#include "ViewProviderPrimitive.h"
#include "DlgPrimitives.h"


using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderPrimitive, PartGui::ViewProviderPart)

ViewProviderPrimitive::ViewProviderPrimitive()
{
    extension.initExtension(this);
    extension.setIgnoreOverlayIcon(true);
}

ViewProviderPrimitive::~ViewProviderPrimitive() = default;

void ViewProviderPrimitive::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(QObject::tr("Edit %1").arg(QString::fromUtf8(getObject()->Label.getValue())));
    act->setData(QVariant((int)ViewProvider::Default));
    func->trigger(act, [this](){
        this->startDefaultEditMode();
    });

    ViewProviderPart::setupContextMenu(menu, receiver, member);
}

bool ViewProviderPrimitive::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        if (Gui::Control().activeDialog())
            return false;
        PartGui::TaskPrimitivesEdit* dlg
            = new PartGui::TaskPrimitivesEdit(dynamic_cast<Part::Primitive*>(getObject()));
        Gui::Control().showDialog(dlg);
        return true;
    }
    else {
        ViewProviderPart::setEdit(ModNum);
        return true;
    }
}

void ViewProviderPrimitive::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderPart::unsetEdit(ModNum);
    }
}

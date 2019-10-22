/****************************************************************************
 *   Copyright (c) 2019 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QMenu>
#endif

#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureSolid.h>
#include <Gui/BitmapFactory.h>
#include "ViewProviderSolid.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderSolid, PartGui::ViewProviderPart)

ViewProviderSolid::ViewProviderSolid()
{
    sPixmap = "PartDesign_Solid.svg";
}

void ViewProviderSolid::updateData(const App::Property* prop) {
    auto owner = Base::freecad_dynamic_cast<PartDesign::Solid>(getObject());
    if(owner) {
        if(prop == &owner->Active)
            signalChangeIcon();
    }
    inherited::updateData(prop);
}

QIcon ViewProviderSolid::getIcon(void) const {
    auto owner = Base::freecad_dynamic_cast<PartDesign::Solid>(getObject());
    if(owner && owner->Active.getValue())
        return Gui::BitmapFactory().pixmap("PartDesign_Solid_Active.svg");
    return inherited::getIcon();
}

void ViewProviderSolid::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Toggle active"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
}

bool ViewProviderSolid::setEdit(int ModNum)
{
    auto owner = Base::freecad_dynamic_cast<PartDesign::Solid>(getObject());
    if(!owner)
        return false;
    if (ModNum == ViewProvider::Default) {
        FCMD_OBJ_CMD(owner,"Active = " << (owner->Active.getValue()?"False":"True"));
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        return false;
    }
    return inherited::setEdit(ModNum);
}


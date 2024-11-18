/***************************************************************************
 *   Copyright (c) 2017 Stefan Tröger <stefantroeger@gmx.net>              *
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
#include <Mod/PartDesign/App/FeatureBase.h>

#include "ViewProviderBase.h"


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderBase,PartDesignGui::ViewProvider)

ViewProviderBase::ViewProviderBase()
{
    sPixmap = "PartDesign_BaseFeature.svg";
}

ViewProviderBase::~ViewProviderBase() = default;

bool ViewProviderBase::doubleClicked()
{
    // If the Placement is mutable then open the transform panel.
    // If the Placement can't be modified then just do nothing on double-click.
    PartDesign::FeatureBase* base = static_cast<PartDesign::FeatureBase*>(getObject());
    if (!base->Placement.testStatus(App::Property::Immutable) &&
        !base->Placement.testStatus(App::Property::ReadOnly) &&
        !base->Placement.testStatus(App::Property::Hidden)) {

        try {
            std::string Msg("Edit ");
            Msg += base->Label.getValue();
            Gui::Command::openCommand(Msg.c_str());
            FCMD_SET_EDIT(base);
        }
        catch (const Base::Exception&) {
            Gui::Command::abortCommand();
        }
        return true;
    }

    return false;
}

void ViewProviderBase::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    // If the Placement is mutable then show the context-menu of the base class.
    PartDesign::FeatureBase* base = static_cast<PartDesign::FeatureBase*>(getObject());
    if (!base->Placement.testStatus(App::Property::Immutable) &&
        !base->Placement.testStatus(App::Property::ReadOnly) &&
        !base->Placement.testStatus(App::Property::Hidden)) {
        PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
    }
}

bool ViewProviderBase::setEdit(int ModNum)
{
    PartDesign::FeatureBase* base = static_cast<PartDesign::FeatureBase*>(getObject());
    if (!base->Placement.testStatus(App::Property::Immutable) &&
        !base->Placement.testStatus(App::Property::ReadOnly) &&
        !base->Placement.testStatus(App::Property::Hidden)) {
        return PartGui::ViewProviderPart::setEdit(ModNum); // clazy:exclude=skipped-base-method
    }

    return false;
}

void ViewProviderBase::unsetEdit(int ModNum)
{
    PartGui::ViewProviderPart::unsetEdit(ModNum); // clazy:exclude=skipped-base-method
}

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
# include <QAction>
# include <QMenu>
# include <Inventor/nodes/SoGroup.h>
#endif

#include "ViewProviderSplit.h"
#include "ViewProviderBody.h"
#include <Base/Tools.h>
#include <Mod/PartDesign/App/FeatureSplit.h>
#include <Mod/PartDesign/App/FeatureSolid.h>
#include <Mod/PartDesign/App/Body.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/Document.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderSplit,PartDesignGui::ViewProvider)

ViewProviderSplit::ViewProviderSplit()
{
    sPixmap = "PartDesign_Split.svg";

    ADD_PROPERTY(Display,((long)0));
    static const char* DisplayEnum[] = {"Result","Solids",NULL};
    Display.setEnums(DisplayEnum);

    pcGroupChildren = new SoGroup;
}

ViewProviderSplit::~ViewProviderSplit()
{
}

SoGroup *ViewProviderSplit::getChildRoot() const {
    return pcGroupChildren;
}

void ViewProviderSplit::attach(App::DocumentObject* obj) {
    PartGui::ViewProviderPartExt::attach(obj);
    addDisplayMaskMode(pcGroupChildren, "Solids");
}

void ViewProviderSplit::onChanged(const App::Property* prop) {

    PartDesignGui::ViewProvider::onChanged(prop);

    if(prop == &Display) {
        if(Display.getValue() == 0) {
            auto vp = getBodyViewProvider();
            if(vp)
                setDisplayMode(vp->DisplayMode.getValueAsString());
            else
                setDisplayMode("Flat Lines");
        } else {
            setDisplayMaskMode("Solids");
            setDisplayMode("Solids");
        }
    }
}

bool ViewProviderSplit::canDropObject(App::DocumentObject *obj) const {
    auto owner = Base::freecad_dynamic_cast<PartDesign::Split>(getObject());
    return owner && owner->isToolAllowed(obj);
}

void ViewProviderSplit::dropObject(App::DocumentObject *obj) {
    auto owner = Base::freecad_dynamic_cast<PartDesign::Split>(getObject());
    if(!owner || !owner->isToolAllowed(obj))
        FC_THROWM(Base::RuntimeError, obj->getNameInDocument() << " cannot not be used as tool");
    auto tools = owner->Tools.getValues();
    tools.push_back(obj);
    Base::ObjectStatusLocker<App::Property::Status, App::Property>
        guard(App::Property::User3, &owner->Tools);
    owner->Tools.setValues(tools);
    obj->Visibility.setValue(false);
}

bool ViewProviderSplit::canDragObject(App::DocumentObject *obj) const {
    return !obj->isDerivedFrom(PartDesign::Solid::getClassTypeId());
}

void ViewProviderSplit::dragObject(App::DocumentObject *obj) {
    if(obj->isDerivedFrom(PartDesign::Solid::getClassTypeId()))
        FC_THROWM(Base::RuntimeError, "Cannot drag solid");
    auto owner = Base::freecad_dynamic_cast<PartDesign::Split>(getObject());
    if(owner) {
        auto tools = owner->Tools.getValues();
        tools.erase(std::remove(tools.begin(),tools.end(),obj),tools.end());
        if(owner->Tools.getSize() != (int)tools.size()) {
            owner->Tools.setValues(tools);
            obj->Visibility.setValue(true);
        }
    }
}

bool ViewProviderSplit::canDelete(App::DocumentObject* obj) const {
    return !obj->isDerivedFrom(PartDesign::Solid::getClassTypeId());
}

std::vector<App::DocumentObject*> ViewProviderSplit::claimChildren(void) const {
    auto owner = Base::freecad_dynamic_cast<PartDesign::Split>(getObject());
    if(owner) {
        auto children = owner->Tools.getValues();
        const auto &solids = owner->Solids.getValues();
        children.insert(children.end(),solids.begin(),solids.end());

        auto res = PartDesignGui::ViewProvider::claimChildren();
        children.insert(children.end(), res.begin(), res.end());
        return children;
    }
    return {};
}

std::vector<App::DocumentObject*> ViewProviderSplit::claimChildren3D(void) const {
    auto owner = Base::freecad_dynamic_cast<PartDesign::Split>(getObject());
    if(owner) 
        return owner->Solids.getValues();
    return {};
};

bool ViewProviderSplit::onDelete(const std::vector<std::string> &) {
    auto owner = Base::freecad_dynamic_cast<PartDesign::Split>(getObject());
    if(owner) {
        auto solids = owner->Solids.getValues();
        owner->Solids.setValues({});
        for(auto obj : solids) {
            if(obj && obj->getNameInDocument())
                obj->getDocument()->removeObject(obj->getNameInDocument());
        }
    }
    return true;
}

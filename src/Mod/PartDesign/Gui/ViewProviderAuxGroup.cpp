/****************************************************************************
 *   Copyright (c) 2020 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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
#endif

#include <Base/Tools.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Mod/PartDesign/App/AuxGroup.h>
#include <Mod/PartDesign/App/Body.h>

#include "ViewProviderAuxGroup.h"
#include "ViewProviderBody.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderAuxGroup, Gui::ViewProviderDocumentObject)

ViewProviderAuxGroup::ViewProviderAuxGroup()
{
    sPixmap = "PartDesign_Group.svg";
}

ViewProviderAuxGroup::~ViewProviderAuxGroup()
{
}

void ViewProviderAuxGroup::attach(App::DocumentObject* obj)
{
    inherited::attach(obj);
    auto group = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(obj);
    if (group) {
        switch (group->getGroupType()) {
        case PartDesign::AuxGroup::SketchGroup:
            sPixmap = "PartDesign_GroupSketch.svg";
            break;
        case PartDesign::AuxGroup::DatumGroup:
            sPixmap = "PartDesign_GroupDatum.svg";
            break;
        case PartDesign::AuxGroup::MiscGroup:
            sPixmap = "PartDesign_GroupMisc.svg";
            break;
        default:
            sPixmap = "PartDesign_Group.svg";
        }
    }
}

bool ViewProviderAuxGroup::canDropObject(App::DocumentObject *obj) const
{
    auto owner = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(getObject());
    auto body = owner->getBody();
    if (!body || !owner->isObjectAllowed(obj))
        return false;
    if (owner->getGroupType() == PartDesign::AuxGroup::OtherGroup) {
        if (PartDesign::Body::findBodyOf(obj) == body
                && !owner->Group.find(obj->getNameInDocument()))
            return true;
    }
    auto bodyVp = Gui::Application::Instance->getViewProvider(body);
    return bodyVp && bodyVp->canDropObject(obj);
}

bool ViewProviderAuxGroup::canDragAndDropObject(App::DocumentObject *obj) const
{
    auto owner = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(getObject());
    if (!owner)
        return false;
    auto body = owner->getBody();
    if (!body)
        return false;
    return PartDesign::Body::findBodyOf(obj) != body;
}

void ViewProviderAuxGroup::dropObject(App::DocumentObject *obj)
{
    auto owner = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(getObject());
    if (!owner)
        return;
    auto body = owner->getBody();
    if (!body || !owner->isObjectAllowed(obj))
        return;
    if (owner->getGroupType() == PartDesign::AuxGroup::OtherGroup) {
        if (PartDesign::Body::findBodyOf(obj) == body
                && !owner->Group.find(obj->getNameInDocument())) {
            auto children = owner->Group.getValues();
            children.push_back(obj);
            owner->Group.setValues(children);
            body->_GroupTouched.touch();
            return;
        }
    }
    auto bodyVp = Gui::Application::Instance->getViewProvider(owner->getBody());
    if (bodyVp && owner->isObjectAllowed(obj))
        bodyVp->dropObject(obj);
}

bool ViewProviderAuxGroup::canDragObject(App::DocumentObject *obj) const {
    auto owner = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(getObject());
    if (!owner)
        return false;
    auto bodyVp = Gui::Application::Instance->getViewProvider(owner->getBody());
    return bodyVp && bodyVp->canDragObject(obj);
}

void ViewProviderAuxGroup::dragObject(App::DocumentObject *obj) {
    auto owner = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(getObject());
    if (!owner)
        return;
    auto bodyVp = Gui::Application::Instance->getViewProvider(owner->getBody());
    if (bodyVp)
        bodyVp->dragObject(obj);
}

std::vector<App::DocumentObject*> ViewProviderAuxGroup::claimChildren(void) const {
    auto owner = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(getObject());
    if(owner)
        return owner->Group.getValues();
    return {};
}

bool ViewProviderAuxGroup::reorderObjects(const std::vector<App::DocumentObject*> &objs,
                                          App::DocumentObject *before)
{
    auto owner = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(getObject());
    if (!owner)
        return 0;
    auto bodyVp = Gui::Application::Instance->getViewProvider(owner->getBody());
    if (bodyVp)
        return bodyVp->reorderObjects(objs, before);
    return 0;
}

bool ViewProviderAuxGroup::canReorderObject(App::DocumentObject *obj,
                                            App::DocumentObject *before)
{
    auto owner = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(getObject());
    if (!owner)
        return false;
    auto bodyVp = Gui::Application::Instance->getViewProvider(owner->getBody());
    return bodyVp && bodyVp->canReplaceObject(obj, before);
}

void ViewProviderAuxGroup::updateData(const App::Property *prop)
{
    auto doc = getDocument()->getDocument();
    if (doc && !doc->isPerformingTransaction()
            && !doc->testStatus(App::Document::Restoring)) {
        auto owner = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(getObject());
        if (prop == &owner->Group) {
            auto body = owner->getBody();
            if (body)
                body->_GroupTouched.touch();
        }
    }
    inherited::updateData(prop);
}

bool ViewProviderAuxGroup::canDelete(App::DocumentObject* obj) const
{
    auto owner = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(getObject());
    if (owner) {
        auto bodyvp = Gui::Application::Instance->getViewProvider(owner->getBody());
        if (bodyvp)
            return bodyvp->canDelete(obj);
    }
    return true;
}

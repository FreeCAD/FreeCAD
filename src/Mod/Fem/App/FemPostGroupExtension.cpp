/***************************************************************************
 *   Copyright (c) 2024 Stefan Tröger <stefantroeger@gmx.net>              *
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
#include "FemPostGroupExtension.h"
#include <App/Document.h>

using namespace Fem;

EXTENSION_PROPERTY_SOURCE(Fem::FemPostGroupExtension, App::GroupExtension);

const char* FemPostGroupExtension::ModeEnums[] = {"Serial", "Parallel", nullptr};

FemPostGroupExtension::FemPostGroupExtension()
    : App::GroupExtension()
{

    initExtensionType(Fem::FemPostGroupExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY_TYPE(
        Mode,
        (long(0)),
        "Pipeline",
        App::Prop_None,
        "Selects the pipeline data transition mode.\n"
        "In serial, every filter gets the output of the previous one as input.\n"
        "In parallel, every filter gets the pipeline source as input.\n");

    Mode.setEnums(ModeEnums);
}

FemPostGroupExtension::~FemPostGroupExtension()
{}


void FemPostGroupExtension::initExtension(App::ExtensionContainer* obj)
{

    if (!obj->isDerivedFrom<FemPostObject>()) {
        throw Base::RuntimeError("FemPostGroupExtension can only be applied to FemPostObject");
    }

    App::GroupExtension::initExtension(obj);
}

void FemPostGroupExtension::extensionOnChanged(const App::Property* p)
{
    if (p == &Group) {
        if (!m_blockChange) {
            // sort the group, so that non filter objects are always on top (in case any object
            // using this extension allows those)
            auto objs = Group.getValues();
            std::sort(objs.begin(),
                      objs.end(),
                      [](const App::DocumentObject* lhs, const App::DocumentObject* rhs) {
                          int l = lhs->isDerivedFrom<FemPostFilter>() ? 0 : 1;
                          int r = rhs->isDerivedFrom<FemPostFilter>() ? 0 : 1;
                          return r < l;
                      });
            m_blockChange = true;
            Group.setValue(objs);
            m_blockChange = false;
        }
    }
    GroupExtension::extensionOnChanged(p);
}

std::vector<Fem::FemPostFilter*> FemPostGroupExtension::getFilter()
{
    // collect all other items that are not filters
    std::vector<Fem::FemPostFilter*> filters;
    for (auto& obj : Group.getValues()) {
        if (obj->isDerivedFrom<FemPostFilter>()) {
            filters.push_back(static_cast<FemPostFilter*>(obj));
        }
    }
    return filters;
}

App::DocumentObject* FemPostGroupExtension::getGroupOfObject(const App::DocumentObject* obj)
{
    for (auto o : obj->getInList()) {
        if (o->hasExtension(FemPostGroupExtension::getExtensionClassTypeId(), false)) {
            return o;
        }
    }

    return nullptr;
}

void FemPostGroupExtension::onExtendedUnsetupObject()
{
    // remove all children!
    auto document = getExtendedObject()->getDocument();
    for (const auto& obj : Group.getValues()) {
        document->removeObject(obj->getNameInDocument());
    }
}

bool FemPostGroupExtension::allowObject(App::DocumentObject* obj)
{
    // only filters may be added
    return obj->isDerivedFrom<FemPostFilter>();
}


void FemPostGroupExtension::recomputeChildren()
{
    for (const auto& obj : Group.getValues()) {
        obj->touch();

        if (obj->hasExtension(Fem::FemPostGroupExtension::getExtensionClassTypeId())) {
            obj->getExtension<Fem::FemPostGroupExtension>()->recomputeChildren();
        }
    }
}

FemPostObject* FemPostGroupExtension::getLastPostObject()
{

    if (Group.getValues().empty()) {
        return static_cast<FemPostObject*>(this->getExtendedObject());
    }

    return static_cast<FemPostObject*>(Group.getValues().back());
}

bool FemPostGroupExtension::holdsPostObject(FemPostObject* obj)
{
    return std::ranges::any_of(Group.getValues(), [obj](const auto& group_obj) {
        return group_obj == obj;
    });
}

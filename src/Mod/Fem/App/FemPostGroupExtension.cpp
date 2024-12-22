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

FemPostGroupExtension::FemPostGroupExtension() : App::GroupExtension() {

    initExtensionType(Fem::FemPostGroupExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY_TYPE(Mode,
                                (long(0)),
                                "Pipeline",
                                App::Prop_None,
                                "Selects the pipeline data transition mode.\n"
                                "In serial, every filter gets the output of the previous one as input.\n"
                                "In parallel, every filter gets the pipeline source as input.\n");

    EXTENSION_ADD_PROPERTY_TYPE(Filter,
                                (nullptr),
                                "Pipeline",
                                App::Prop_Transient,
                                "List of all filters");

    Mode.setEnums(ModeEnums);
}

FemPostGroupExtension::~FemPostGroupExtension() {

}


void FemPostGroupExtension::initExtension(App::ExtensionContainer* obj)
{

    if (!obj->isDerivedFrom(Fem::FemPostObject::getClassTypeId())) {
        throw Base::RuntimeError("FemPostGroupExtension can only be applied to FemPostObject");
    }

    App::GroupExtension::initExtension(obj);
}

void FemPostGroupExtension::extensionOnChanged(const App::Property* p)
{
    if(p == &Group) {
        if (!m_blockChange) {
            // make sure filter property holds all our filters (and non filters are in front)
            std::vector<App::DocumentObject*> filters;
            for(auto obj : Group.getValues()) {
                if(obj->isDerivedFrom(FemPostFilter::getClassTypeId())) {
                    filters.push_back(obj);
                }
            }

            // sort the group, so that non filter objects are always on top
            auto objs = Group.getValues();
            std::sort( objs.begin( ), objs.end( ), [ ]( const App::DocumentObject* lhs, const App::DocumentObject* rhs ){

                int l = lhs->isDerivedFrom(FemPostFilter::getClassTypeId()) ? 0 : 1;
                int r = rhs->isDerivedFrom(FemPostFilter::getClassTypeId()) ? 0 : 1;
                return r<l;
            });
            m_blockChange = true;
            Group.setValue(objs);
            Filter.setValue(filters);
            m_blockChange = false;
        }
    }
    else if (p == &Filter) {
        if (!m_blockChange) {
            // someone external changed filter, make sure group represents this

            // collect all filters
            std::vector<App::DocumentObject*> filters;
            for (auto obj : Filter.getValues()) {
                if (obj->isDerivedFrom(Fem::FemPostFilter::getClassTypeId())) {
                    filters.push_back(obj);
                }
            }

            //collect all other items that are not filters
            std::vector<App::DocumentObject*> objs;
            for (auto obj : Group.getValues()) {
                if (!obj->isDerivedFrom(Fem::FemPostFilter::getClassTypeId())) {
                    objs.push_back(obj);
                }
            }

            // set the properties correctly
            m_blockChange = true;
            objs.insert( objs.end(), filters.begin(), filters.end() );
            Filter.setValue(filters);
            Group.setValue(objs);
            m_blockChange = false;
        }
    }
    GroupExtension::extensionOnChanged(p);
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
    return obj->isDerivedFrom(Fem::FemPostFilter::getClassTypeId());
}


void FemPostGroupExtension::recomputeChildren()
{
    for (const auto& obj : Group.getValues()) {
        obj->touch();
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

    std::vector<App::DocumentObject*>::const_iterator it = Group.getValues().begin();
    for (; it != Group.getValues().end(); ++it) {

        if (*it == obj) {
            return true;
        }
    }
    return false;
}

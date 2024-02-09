/****************************************************************************
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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
#include <iostream>
#endif

#include "App/Part.h"
#include "VarSet.h"
#include "DocumentObject.h"
#include "PropertyStandard.h"

#include "Base/Console.h"

using namespace App;

PROPERTY_SOURCE(App::VarSet, App::DocumentObject)

FC_LOG_LEVEL_INIT("VarSet", true, true)

const bool HIDDEN = true;
const bool COPY_ON_CHANGE = true;
const bool READ_ONLY = true;


VarSet::VarSet() {
    ADD_PROPERTY_TYPE(Exposed, (false), 0, App::Prop_None, "Whether the variable set is exposed.");
    ADD_PROPERTY_TYPE(ReplacedBy, (nullptr), 0, App::Prop_None, "The VarSet that replaces this VarSet.");
    ADD_PROPERTY_TYPE(NamePropertyParent, (""), 0, App::Prop_None, "The name of the property of the parent if the variable set is exposed.");
    Exposed.setStatus(Property::Status::ReadOnly, true);
    ReplacedBy.setStatus(Property::Status::Hidden, true);
}

void VarSet::onBeforeChange(const Property* prop)
{
    DocumentObject::onBeforeChange(prop);
    if (prop == &NamePropertyParent && isExposed()) {
        previousNameParentProperty = NamePropertyParent.getValue();
    }
}

void VarSet::onChanged(const Property* prop)
{
    DocumentObject::onChanged(prop);
    if (prop == &Exposed) {
        exposedChanged();
    }
    if (prop == &NamePropertyParent && isExposed()) {
        renameVarSetPropertiesParent(previousNameParentProperty);
        previousNameParentProperty = nullptr;
    }
}

const char* VarSet::getViewProviderName() const
{
    return "Gui::ViewProviderVarSet";
}

void VarSet::enableExposed()
{
    Exposed.setStatus(Property::Status::ReadOnly, false);
}

void VarSet::disableExposed()
{
    Exposed.setValue(false);
    Exposed.setStatus(Property::Status::ReadOnly, true);
}

void VarSet::exposedChanged()
{
    if (!isRestoring()) {
        if (Exposed.getValue()) {
            addVarSetPropertiesParent();
        }
        else {
            removeVarSetPropertiesParent();
        }
    }
}

bool VarSet::isExposed() const
{
    return Exposed.getValue();
}

// TODO
bool VarSet::isEquivalent(VarSet* varSet) const
{
    return true;
}

void VarSet::onDocumentRestored()
{
    DocumentObject::onDocumentRestored();
    DocumentObject* parent = getFirstParent();
    if (parent && parent->isDerivedFrom<App::Part>()) {
        enableExposed();
    }
    else {
        disableExposed();
    }
}

const char* VarSet::getNameParentProperty()
{
    if (strcmp("", NamePropertyParent.getValue()) == 0) {
        NamePropertyParent.setValue(Label.getValue());
    }
    return NamePropertyParent.getValue();
}

void VarSet::renameVarSetPropertiesParent(const char *oldName)
{
    DocumentObject* parent = getParent();
    if (parent) {
        auto prop = dynamic_cast<App::PropertyVarSet*>(parent->getPropertyByName(oldName));
        if (prop) {
            VarSet* varSet = prop->getValue();
            assert(parent->removeDynamicProperty(oldName));
            assert(parent->removeDynamicProperty(getNameOriginalVarSetProperty(oldName).c_str()));

            createVarSetPropertiesParent(getNameParentProperty(), varSet);
        }
    }
    // there may be no parent, for example when copying
}

DocumentObject* VarSet::getParent()
{
    if (isExposed()) {
        DocumentObject* parent = getFirstParent();
        // It could be the case that we're copying an exposed VarSet to a place
        // where we have no valid parent, for example no App::Part.  In that
        // case disable the exposed and return nullptr.
        if (parent) {
            if (parent->isDerivedFrom<App::Part>()) {
                return parent;
            }
            else {
                disableExposed();
            }
        }
        else {
            disableExposed();
        }
    }
    return nullptr;
}

VarSet* VarSet::getParentVarSet()
{
    // get a parent if this VarSet is exposed
    DocumentObject* parent = getParent();
    if (parent) {
        const char* nameProp = getNameParentProperty();
        auto prop = dynamic_cast<App::PropertyVarSet*>(parent->getPropertyByName(nameProp));
        if (prop) {
            VarSet *parentVarSet = prop->getValue();
            if (parentVarSet != this) {
                return parentVarSet;
            }
        }
        else {
            throw Base::RuntimeError(std::string("Property ") + nameProp + " not available in " + getFullName());
        }
    }
    return nullptr;
}

void VarSet::createVarSetPropertyParent(DocumentObject* parent, const char* name,
                                        std::string& doc, DocumentObject* value,
                                        bool hidden, bool copyOnChange)
{
    auto prop = dynamic_cast<App::PropertyVarSet*>(
            parent->addDynamicProperty("App::PropertyVarSet",
                    name, "Expose", doc.c_str(), 0, !READ_ONLY, hidden));
    prop->setValue(value);
    if (copyOnChange) {
        prop->setStatus(Property::CopyOnChange, true);
    }
}

std::string VarSet::getNameOriginalVarSetProperty(const char* name) 
{
    return "_" + std::string(name) + "Original";
}

void VarSet::createVarSetPropertiesParent(const char* name, DocumentObject* value)
{
    DocumentObject* parent = getParent();
    if (parent) {
        std::string docExposed = "A link to a VarSet equivalent to " + std::string(name);
        createVarSetPropertyParent(parent, name, docExposed, value, !HIDDEN, COPY_ON_CHANGE);

        std::string docOriginal = "A link to the original VarSet";
        std::string nameOriginal = getNameOriginalVarSetProperty(name);
        createVarSetPropertyParent(parent, nameOriginal.c_str(), docOriginal, this, HIDDEN);
    }
    else { // a copy can go outside of a part, disable exposed
        disableExposed();
    }
}

void VarSet::addVarSetPropertiesParent()
{
    createVarSetPropertiesParent(getNameParentProperty(), this);
}

void VarSet::removeVarSetPropertiesParent()
{
    DocumentObject* parent = getParent();
    if (parent) {
        // the property may not be there when constructing the object
        const char *nameProperty = getNameParentProperty();
        parent->removeDynamicProperty(nameProperty);
        parent->removeDynamicProperty(getNameOriginalVarSetProperty(nameProperty).c_str());
    }
}

void VarSet::addReplacedVarSet(VarSet* varSet)
{
    ReplacedBy.setValue(varSet);
}


void VarSet::removeReplacedVarSet()
{
    ReplacedBy.setValue(nullptr);
}

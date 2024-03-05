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
const bool ADD = true;
const bool REMOVE = false;


VarSet::VarSet() {
    ADD_PROPERTY_TYPE(Exposed, (false), 0, App::Prop_None, "Whether the variable set is exposed.");
    ADD_PROPERTY_TYPE(ReplacedBy, (nullptr), 0, App::Prop_None, "The VarSet that replaces this VarSet.");
    ADD_PROPERTY_TYPE(NamePropertyParent, (""), 0, App::Prop_None, "The name of the property of the parent if the variable set is exposed.");
    Exposed.setStatus(Property::Status::ReadOnly, true);
    Exposed.setStatus(Property::Status::Immutable, true);
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
    Exposed.setStatus(Property::Status::Immutable, false);
}

void VarSet::disableExposed()
{
    Exposed.setValue(false);
    Exposed.setStatus(Property::Status::ReadOnly, true);
    Exposed.setStatus(Property::Status::Immutable, true);
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

bool VarSet::isEquivalent(VarSet* other) const
{
    // go only over dynamic properties and aim for an early exit
    std::vector<std::string> namesThis = getDynamicPropertyNames();
    std::vector<std::string> namesOther = other->getDynamicPropertyNames();

    // the sizes may differ on a copy giving a property _SourceUUID
    std::vector<std::string> &longer = namesThis.size() >= namesOther.size() ? namesThis : namesOther;

    for (const auto& name : longer) {
        if (name != "_SourceUUID") { // ignore this property
            Property* propThis = getPropertyByName(name.c_str());
            if (!propThis) {
                return false;
            }
            Property* propOther = other->getPropertyByName(name.c_str());
            if ((propOther == nullptr) || (propThis->getTypeId() != propOther->getTypeId())) {
                return false;
            }
        }
    }

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

/** @brief Get the name of the property of the VarSet in the parent.
 *
 * Get the name of the property of this varset in the parent.  If it is an
 * empty string, we default to the label of this VarSet.
 *
 * @return the name of the property of the VarSet in the parent.
 */
const char* VarSet::getNameVarSetPropertyParent()
{
    if (strcmp("", NamePropertyParent.getValue()) == 0) {
        NamePropertyParent.setValue(Label.getValue());
    }
    return NamePropertyParent.getValue();
}

// assumes that the VarSet is exposed
void VarSet::renameVarSetPropertiesParent(const char *oldName)
{
    DocumentObject* parent = getParentExposed();
    if (parent) {
        auto prop = dynamic_cast<App::PropertyVarSet*>(parent->getPropertyByName(oldName));
        if (prop) {
            VarSet* varSet = prop->getValue();
            assert(parent->removeDynamicProperty(oldName));
            assert(parent->removeDynamicProperty(getNameOriginalVarSetProperty(oldName).c_str()));

            createVarSetPropertiesParent(varSet);
        }
    }
    // there may be no parent, for example when copying
}

DocumentObject* VarSet::getParentExposed()
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
    DocumentObject* parent = getParentExposed();
    if (parent) {
        const char* nameProp = getNameVarSetPropertyParent();
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

// assumes that the VarSet is exposed
void VarSet::createVarSetPropertyParent(DocumentObject* parent, const char* name,
                                        std::string& doc, DocumentObject* value,
                                        bool hidden, bool copyOnChange)
{
    if (parent->getPropertyByName(name)) {
        FC_THROWM(Base::NameError, "Property " << parent->getFullName() << '.' << name << " already exists");
    }
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

void VarSet::createVarSetPropertiesParent(DocumentObject* varSet)
{
    const char* nameProperty = getNameVarSetPropertyParent();
    DocumentObject* parent = getParentExposed();
    if (parent) {
        std::string docExposed = "A link to a VarSet equivalent to " + std::string(nameProperty);
        createVarSetPropertyParent(parent, nameProperty, docExposed, varSet, !HIDDEN, COPY_ON_CHANGE);

        std::string docOriginal = "A link to the original VarSet";
        std::string nameOriginalVarSetProperty = getNameOriginalVarSetProperty(nameProperty);
        createVarSetPropertyParent(parent, nameOriginalVarSetProperty.c_str(), docOriginal, this, HIDDEN);
    }
    else { // a copy can go outside of a part, disable exposed
        disableExposed();
    }
}

void VarSet::addVarSetPropertiesParent()
{
    try {
        createVarSetPropertiesParent(this);
    }
    catch (Base::NameError &e) {
        // expose failure, set to false
        Exposed.setValue(false);
        throw e;
    }
}

void VarSet::removeVarSetPropertiesParent()
{
    DocumentObject* parent = getFirstParent();
    if (parent) {
        // the property may not be there when constructing the object
        const char *nameProperty = getNameVarSetPropertyParent();
        parent->removeDynamicProperty(nameProperty);
        parent->removeDynamicProperty(getNameOriginalVarSetProperty(nameProperty).c_str());
    }
}

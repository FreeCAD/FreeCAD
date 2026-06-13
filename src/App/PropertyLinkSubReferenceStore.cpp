// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD Project Association AISBL
// SPDX-FileNotice: Part of the FreeCAD project.
/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "PropertyLinks.h"

#include <Base/Exception.h>

#include "DocumentObject.h"
#include "ElementNamingUtils.h"

using namespace App;

std::size_t PropertyLinkSubReferenceStore::size() const
{
    return _references.size();
}

bool PropertyLinkSubReferenceStore::empty() const
{
    return _references.empty();
}

PropertyLinkSubReferenceStore::iterator PropertyLinkSubReferenceStore::begin()
{
    return _references.begin();
}

PropertyLinkSubReferenceStore::iterator PropertyLinkSubReferenceStore::end()
{
    return _references.end();
}

PropertyLinkSubReferenceStore::const_iterator PropertyLinkSubReferenceStore::begin() const
{
    return _references.begin();
}

PropertyLinkSubReferenceStore::const_iterator PropertyLinkSubReferenceStore::end() const
{
    return _references.end();
}

PropertyLinkSubReferenceStore::Reference&
PropertyLinkSubReferenceStore::operator[](std::size_t index)
{
    return _references[index];
}

const PropertyLinkSubReferenceStore::Reference&
PropertyLinkSubReferenceStore::operator[](std::size_t index) const
{
    return _references[index];
}

const std::vector<PropertyLinkSubReferenceStore::Reference>&
PropertyLinkSubReferenceStore::references() const
{
    return _references;
}

void PropertyLinkSubReferenceStore::set(std::vector<Reference>&& references)
{
    _references = std::move(references);
}

void PropertyLinkSubReferenceStore::set(std::vector<App::DocumentObject*> objects,
                                        std::vector<std::string> subNames,
                                        std::vector<PropertyLinkBase::ShadowSub> shadows)
{
    if (objects.size() != subNames.size()) {
        throw Base::ValueError("PropertyLinkSubReferenceStore: object and subelement counts differ");
    }

    if (shadows.size() != objects.size()) {
        shadows.clear();
        shadows.resize(objects.size());
    }

    _references.clear();
    _references.reserve(objects.size());
    for (std::size_t i = 0; i < objects.size(); ++i) {
        _references.push_back({objects[i], std::move(subNames[i]), std::move(shadows[i])});
    }
}

std::vector<App::DocumentObject*> PropertyLinkSubReferenceStore::objects() const
{
    std::vector<App::DocumentObject*> objects;
    objects.reserve(_references.size());
    for (const auto& reference : _references) {
        objects.push_back(reference.object);
    }
    return objects;
}

std::vector<std::string> PropertyLinkSubReferenceStore::subNames() const
{
    std::vector<std::string> subNames;
    subNames.reserve(_references.size());
    for (const auto& reference : _references) {
        subNames.push_back(reference.subName);
    }
    return subNames;
}

std::vector<std::string> PropertyLinkSubReferenceStore::subNames(bool newStyle) const
{
    std::vector<std::string> result;
    result.reserve(_references.size());
    std::string tmp;
    for (const auto& reference : _references) {
        result.push_back(getSubNameWithStyle(reference.subName, reference.shadow, newStyle, tmp));
    }
    return result;
}

std::vector<PropertyLinkBase::ShadowSub> PropertyLinkSubReferenceStore::shadows() const
{
    std::vector<PropertyLinkBase::ShadowSub> shadows;
    shadows.reserve(_references.size());
    for (const auto& reference : _references) {
        shadows.push_back(reference.shadow);
    }
    return shadows;
}

std::vector<PropertyLinkSubReferenceStore::Reference>
PropertyLinkSubReferenceStore::references(bool newStyle) const
{
    std::vector<Reference> result;
    result.reserve(_references.size());
    std::string tmp;
    for (const auto& reference : _references) {
        result.push_back({reference.object,
                          getSubNameWithStyle(reference.subName, reference.shadow, newStyle, tmp),
                          reference.shadow});
    }
    return result;
}

void PropertyLinkSubReferenceStore::clearShadows()
{
    for (auto& reference : _references) {
        reference.shadow = {};
    }
}

std::vector<std::size_t> PropertyLinkSubReferenceStore::attachedEntryIndices() const
{
    std::vector<std::size_t> indices;
    indices.reserve(_references.size());
    for (std::size_t i = 0; i < _references.size(); ++i) {
        if (auto* obj = _references[i].object; obj && obj->isAttachedToDocument()) {
            indices.push_back(i);
        }
    }
    return indices;
}

unsigned int PropertyLinkSubReferenceStore::getMemSize() const
{
    unsigned int size =
        static_cast<unsigned int>(_references.size() * sizeof(App::DocumentObject*));
    for (const auto& reference : _references) {
        size += static_cast<unsigned int>(reference.subName.size());
    }
    return size;
}

const std::string& PropertyLinkSubReferenceStore::getSubNameWithStyle(
    const std::string& subName,
    const PropertyLinkBase::ShadowSub& shadow,
    bool newStyle,
    std::string& tmp)
{
    if (!newStyle) {
        if (!shadow.oldName.empty()) {
            return shadow.oldName;
        }
    }
    else if (!shadow.newName.empty()) {
        if (Data::hasMissingElement(shadow.oldName.c_str())) {
            auto pos = shadow.newName.rfind('.');
            if (pos != std::string::npos) {
                tmp = shadow.newName.substr(0, pos + 1);
                tmp += shadow.oldName;
                return tmp;
            }
        }
        return shadow.newName;
    }
    return subName;
}

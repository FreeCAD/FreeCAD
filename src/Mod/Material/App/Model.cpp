/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <string>
#endif

#include <App/Application.h>

#include "Exceptions.h"
#include "Model.h"
#include "ModelLibrary.h"


using namespace Materials;

TYPESYSTEM_SOURCE(Materials::ModelProperty, Base::BaseClass)

ModelProperty::ModelProperty()
{}

ModelProperty::ModelProperty(const QString& name,
                             const QString& header,
                             const QString& type,
                             const QString& units,
                             const QString& url,
                             const QString& description)
    : _name(name)
    , _displayName(header)
    , _propertyType(type)
    , _units(units)
    , _url(url)
    , _description(description)
{}

ModelProperty::ModelProperty(const ModelProperty& other)
    : _name(other._name)
    , _displayName(other._displayName)
    , _propertyType(other._propertyType)
    , _units(other._units)
    , _url(other._url)
    , _description(other._description)
    , _inheritance(other._inheritance)
{
    for (auto it = other._columns.begin(); it != other._columns.end(); it++) {
        _columns.push_back(*it);
    }
}

const QString ModelProperty::getDisplayName() const
{
    if (_displayName.isEmpty()) {
        return getName();
    }
    return _displayName;
}

ModelProperty& ModelProperty::operator=(const ModelProperty& other)
{
    if (this == &other) {
        return *this;
    }

    _name = other._name;
    _displayName = other._displayName;
    _propertyType = other._propertyType;
    _units = other._units;
    _url = other._url;
    _description = other._description;
    _inheritance = other._inheritance;
    _columns.clear();
    for (auto it = other._columns.begin(); it != other._columns.end(); it++) {
        _columns.push_back(*it);
    }

    return *this;
}

bool ModelProperty::operator==(const ModelProperty& other) const
{
    if (this == &other) {
        return true;
    }

    return (_name == other._name) && (_displayName == other._displayName) && (_propertyType == other._propertyType)
        && (_units == other._units) && (_url == other._url) && (_description == other._description)
        && (_inheritance == other._inheritance);
}

TYPESYSTEM_SOURCE(Materials::Model, Base::BaseClass)

Model::Model()
{}

Model::Model(std::shared_ptr<ModelLibrary> library,
             ModelType type,
             const QString& name,
             const QString& directory,
             const QString& uuid,
             const QString& description,
             const QString& url,
             const QString& doi)
    : _library(library)
    , _type(type)
    , _name(name)
    , _directory(directory)
    , _uuid(uuid)
    , _description(description)
    , _url(url)
    , _doi(doi)
{}

ModelProperty& Model::operator[](const QString& key)
{
    try {
        return _properties.at(key);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

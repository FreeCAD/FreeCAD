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

    return (_name == other._name) && (_displayName == other._displayName)
        && (_propertyType == other._propertyType) && (_units == other._units)
        && (_url == other._url) && (_description == other._description)
        && (_inheritance == other._inheritance);
}

void ModelProperty::validate(const ModelProperty& other) const
{
    if (_name != other._name) {
        throw InvalidProperty("Model names don't match");
    }
    if (getDisplayName() != other.getDisplayName()) {
        Base::Console().log("Local display name '%s'\n", getDisplayName().toStdString().c_str());
        Base::Console().log("Remote display name '%s'\n",
                            other.getDisplayName().toStdString().c_str());
        throw InvalidProperty("Model display names don't match");
    }
    if (_propertyType != other._propertyType) {
        throw InvalidProperty("Model property types don't match");
    }
    if (_units != other._units) {
        throw InvalidProperty("Model units don't match");
    }
    if (_url != other._url) {
        throw InvalidProperty("Model URLs don't match");
    }
    if (_description != other._description) {
        throw InvalidProperty("Model descriptions don't match");
    }
    if (_inheritance != other._inheritance) {
        throw InvalidProperty("Model inheritance don't match");
    }

    if (_columns.size() != other._columns.size()) {
        Base::Console().log("Local property column count %d\n", _columns.size());
        Base::Console().log("Remote property column count %d\n", other._columns.size());
        throw InvalidProperty("Model property column counts don't match");
    }
    for (size_t i = 0; i < _columns.size(); i++) {
        _columns[i].validate(other._columns[i]);
    }
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

QString Model::getDirectory() const
{
    return _directory;
}

void Model::setDirectory(const QString& directory)
{
    _directory = directory;
}

QString Model::getFilename() const
{
    return _filename;
}

void Model::setFilename(const QString& filename)
{
    _filename = filename;
}

QString Model::getFilePath() const
{
    return QDir(_directory + QStringLiteral("/") + _filename).absolutePath();
}

ModelProperty& Model::operator[](const QString& key)
{
    try {
        return _properties.at(key);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

void Model::validate(Model& other) const
{
    try {
        _library->validate(*(other._library));
    }
    catch (const InvalidLibrary& e)
    {
        throw InvalidModel(e.what());
    }

    // std::map<QString, ModelProperty> _properties;
    if (_type != other._type) {
        throw InvalidModel("Model types don't match");
    }
    if (_name != other._name) {
        throw InvalidModel("Model names don't match");
    }
    if (_directory != other._directory) {
        throw InvalidModel("Model directories don't match");
    }
    if (!other._filename.isEmpty()) {
        throw InvalidModel("Remote filename is not empty");
    }
    if (_uuid != other._uuid) {
        throw InvalidModel("Model UUIDs don't match");
    }
    if (_description != other._description) {
        throw InvalidModel("Model descriptions don't match");
    }
    if (_url != other._url) {
        throw InvalidModel("Model URLs don't match");
    }
    if (_doi != other._doi) {
        throw InvalidModel("Model DOIs don't match");
    }
    if (_inheritedUuids != other._inheritedUuids) {
        throw InvalidModel("Model inherited UUIDs don't match");
    }

    // Need to compare properties
    if (_properties.size() != other._properties.size()) {
        throw InvalidModel("Model property counts don't match");
    }
    for (auto& property : _properties) {
        auto& remote = other._properties[property.first];
        property.second.validate(remote);
    }
}

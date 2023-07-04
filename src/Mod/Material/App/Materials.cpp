/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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
#ifndef _PreComp_
#endif

#include <App/Application.h>

#include "Materials.h"
#include "ModelManager.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::ModelData, Base::BaseClass)

ModelData::ModelData()
{

}

ModelData::ModelData(const std::string& name, const std::string& type,
                        const std::string& units, const std::string& url,
                        const std::string& description):
    _name(name), _type(type), _units(units), _url(url), _description(description)
{

}

ModelData::~ModelData()
{

}

TYPESYSTEM_SOURCE(Materials::Material, Base::BaseClass)

Material::Material()
{}

Material::Material(const MaterialLibrary &library, const std::string& directory,
            const std::string& uuid, const std::string& name) :
    _library(library), _uuid(uuid), _name(name)
{
    setDirectory(directory);
}

Material::Material(const MaterialLibrary &library, const QDir& directory,
            const std::string& uuid, const std::string& name) :
    _library(library), _directory(directory), _uuid(uuid), _name(name)
{}

/*
 *  Destroys the object and frees any allocated resources
 */
Material::~Material()
{
    // no need to delete child widgets, Qt does it all for us
}

void Material::addPhysical(const std::string &uuid)
{
    if (hasPhysicalModel(uuid))
        return;

    ModelManager *manager = ModelManager::getManager();

    try {
        const Model &model = manager->getModel(uuid);

        _physicalUuids.push_back(uuid);

        for (auto it = model.begin(); it != model.end(); it++)
        {
            std::string propertyName = it->first;
            ModelProperty property = it->second;

            _physical[propertyName] = ModelValueProperty(property);
        }
    } catch (ModelNotFound const &) {
    }
}

void Material::addAppearance(const std::string &uuid)
{
    if (hasAppearanceModel(uuid))
        return;

    ModelManager *manager = ModelManager::getManager();

    try {
        const Model &model = manager->getModel(uuid);

        _appearanceUuids.push_back(uuid);

        for (auto it = model.begin(); it != model.end(); it++)
        {
            std::string propertyName = it->first;
            ModelProperty property = it->second;

            _appearance[propertyName] = ModelValueProperty(property);
        }
    } catch (ModelNotFound const &) {
    }
}


void Material::setPhysicalValue(const std::string& name, const std::string &value)
{
    _physical[name].setValue(value); // may not be a string type
}

void Material::setPhysicalValue(const std::string& name, int value)
{
    _physical[name].setInt(value);
}

void Material::setPhysicalValue(const std::string& name, double value)
{
    _physical[name].setFloat(value);
}

void Material::setPhysicalValue(const std::string& name, const Base::Quantity value)
{
    _physical[name].setQuantity(value);
}

void Material::setAppearanceValue(const std::string& name, const std::string &value)
{
    _appearance[name].setValue(value); // may not be a string type
}

ModelValueProperty &Material::getPhysicalProperty(const std::string &name)
{
    try {
        return _physical.at(name);
    } catch (std::out_of_range const &) {
        throw PropertyNotFound();
    }
}

ModelValueProperty &Material::getAppearanceProperty(const std::string &name)
{
    try {
        return _appearance.at(name);
    } catch (std::out_of_range const &) {
        throw PropertyNotFound();
    }
}

const std::string Material::getPhysicalValue(const std::string &name) const
{
    try {
        return _physical.at(name).getValue();
    } catch (std::out_of_range const &) {
        throw PropertyNotFound();
    }
}

const std::string Material::getAppearanceValue(const std::string &name) const
{
    try {
        return _appearance.at(name).getValue();
    } catch (std::out_of_range const &) {
        throw PropertyNotFound();
    }
}

bool Material::hasPhysicalProperty(const std::string& name) const
{
    try {
        static_cast<void>(_physical.at(name));
    } catch (std::out_of_range const &) {
        return false;
    }
    return true;
}

bool Material::hasAppearanceProperty(const std::string& name) const
{
    try {
        static_cast<void>(_appearance.at(name));
    } catch (std::out_of_range const &) {
        return false;
    }
    return true;
}

bool Material::hasPhysicalModel(const std::string& uuid) const
{
    for (std::string modelUUID: _physicalUuids)
        if (modelUUID == uuid)
            return true;

    return false;
}

bool Material::hasAppearanceModel(const std::string& uuid) const
{
    for (std::string modelUUID: _appearanceUuids)
        if (modelUUID == uuid)
            return true;

    return false;
}

#include "moc_Materials.cpp"

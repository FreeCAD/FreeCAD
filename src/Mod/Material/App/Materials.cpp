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

ModelData::ModelData(const QString& name, const QString& type,
                        const QString& units, const QString& url,
                        const QString& description):
    _name(name), _type(type), _units(units), _url(url), _description(description)
{

}

ModelData::~ModelData()
{

}

TYPESYSTEM_SOURCE(Materials::Material, Base::BaseClass)

Material::Material()
{}

Material::Material(const MaterialLibrary &library, const QString& directory,
            const QString& uuid, const QString& name) :
    _library(library), _uuid(uuid), _name(name)
{
    setDirectory(directory);
}

Material::Material(const MaterialLibrary &library, const QDir& directory,
            const QString& uuid, const QString& name) :
    _library(library), _directory(directory), _uuid(uuid), _name(name)
{}

/*
 *  Destroys the object and frees any allocated resources
 */
Material::~Material()
{
    // no need to delete child widgets, Qt does it all for us
}

void Material::addPhysical(const QString &uuid)
{
    if (hasPhysicalModel(uuid))
        return;

    ModelManager *manager = ModelManager::getManager();

    try {
        const Model &model = manager->getModel(uuid);

        _physicalUuids.push_back(uuid);

        for (auto it = model.begin(); it != model.end(); it++)
        {
            QString propertyName = it->first;
            ModelProperty property = it->second;

            _physical[propertyName] = ModelValueProperty(property);
        }
    } catch (ModelNotFound const &) {
    }
}

void Material::addAppearance(const QString &uuid)
{
    if (hasAppearanceModel(uuid))
        return;

    ModelManager *manager = ModelManager::getManager();

    try {
        const Model &model = manager->getModel(uuid);

        _appearanceUuids.push_back(uuid);

        for (auto it = model.begin(); it != model.end(); it++)
        {
            QString propertyName = it->first;
            ModelProperty property = it->second;

            _appearance[propertyName] = ModelValueProperty(property);
        }
    } catch (ModelNotFound const &) {
    }
}


void Material::setPhysicalValue(const QString& name, const QString &value)
{
    _physical[name].setValue(value); // may not be a string type
}

void Material::setPhysicalValue(const QString& name, int value)
{
    _physical[name].setInt(value);
}

void Material::setPhysicalValue(const QString& name, double value)
{
    _physical[name].setFloat(value);
}

void Material::setPhysicalValue(const QString& name, const Base::Quantity value)
{
    _physical[name].setQuantity(value);
}

void Material::setAppearanceValue(const QString& name, const QString &value)
{
    _appearance[name].setValue(value); // may not be a string type
}

ModelValueProperty &Material::getPhysicalProperty(const QString &name)
{
    try {
        return _physical.at(name);
    } catch (std::out_of_range const &) {
        throw PropertyNotFound();
    }
}

ModelValueProperty &Material::getAppearanceProperty(const QString &name)
{
    try {
        return _appearance.at(name);
    } catch (std::out_of_range const &) {
        throw PropertyNotFound();
    }
}

const QString Material::getPhysicalValue(const QString &name) const
{
    try {
        return _physical.at(name).getValue();
    } catch (std::out_of_range const &) {
        throw PropertyNotFound();
    }
}

const QString Material::getAppearanceValue(const QString &name) const
{
    try {
        return _appearance.at(name).getValue();
    } catch (std::out_of_range const &) {
        throw PropertyNotFound();
    }
}

bool Material::hasPhysicalProperty(const QString& name) const
{
    try {
        static_cast<void>(_physical.at(name));
    } catch (std::out_of_range const &) {
        return false;
    }
    return true;
}

bool Material::hasAppearanceProperty(const QString& name) const
{
    try {
        static_cast<void>(_appearance.at(name));
    } catch (std::out_of_range const &) {
        return false;
    }
    return true;
}

bool Material::hasPhysicalModel(const QString& uuid) const
{
    for (QString modelUUID: _physicalUuids)
        if (modelUUID == uuid)
            return true;

    return false;
}

bool Material::hasAppearanceModel(const QString& uuid) const
{
    for (QString modelUUID: _appearanceUuids)
        if (modelUUID == uuid)
            return true;

    return false;
}

#include "moc_Materials.cpp"

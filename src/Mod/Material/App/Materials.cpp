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

TYPESYSTEM_SOURCE(Materials::MaterialLibrary, Base::BaseClass)

MaterialLibrary::MaterialLibrary()
{}

MaterialLibrary::MaterialLibrary(const std::string &libraryName, const QDir &dir, const std::string &icon):
    _name(libraryName), _directory(dir), _iconPath(icon)
{}

MaterialLibrary::~MaterialLibrary()
{
    // delete directory;
}

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

void Material::addModel(const std::string &uuid)
{
    ModelManager manager;

    const Model &model = manager.getModel(uuid);

    if (&model) {
        _modelUuids.push_back(uuid);

        for (auto it = model.begin(); it != model.end(); it++)
        {
            std::string propertyName = it->first;
            ModelProperty property = it->second;

            // ModelValueProperty* valuePtr = new ModelValueProperty(property);
            _properties[propertyName] = ModelValueProperty(property);
        }
    }
}


#include "moc_Materials.cpp"

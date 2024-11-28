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
#endif

#include "Exceptions.h"
#include "ExternalManager.h"
#include "MaterialLibrary.h"
#include "MaterialManagerExternal.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::MaterialManagerExternal, Base::BaseClass)

MaterialManagerExternal::MaterialManagerExternal()
{
}

void MaterialManagerExternal::cleanup()
{
}

void MaterialManagerExternal::refresh()
{
}

//=====
//
// Library management
//
//=====

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialManagerExternal::getLibraries()
{
    auto libraryList = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();
    auto externalLibraries = ExternalManager::getManager()->libraries();
    for (auto& entry : *externalLibraries) {
        auto libName = std::get<0>(entry);
        auto icon = std::get<1>(entry);
        auto readOnly = std::get<2>(entry);
        Base::Console().Log("Library name '%s', Icon '%s', readOnly %s\n",
                            libName.toStdString().c_str(),
                            icon.toStdString().c_str(),
                            readOnly ? "true" : "false");
        auto library = std::make_shared<MaterialLibrary>(libName, icon, readOnly);
        libraryList->push_back(library);
    }

    return libraryList;
}

void MaterialManagerExternal::createLibrary(const QString& libraryName,
                                            const QString& icon,
                                            bool readOnly)
{
    ExternalManager::getManager()->createLibrary(libraryName, icon, readOnly);
}

//=====
//
// Material management
//
//=====

void MaterialManagerExternal::addMaterial(const QString& libraryName,
                                          const QString& path,
                                          const std::shared_ptr<Material>& material)
{
    ExternalManager::getManager()->addMaterial(libraryName, path, material);
}

void MaterialManagerExternal::migrateMaterial(const QString& libraryName,
                                          const QString& path,
                                          const std::shared_ptr<Material>& material)
{
    ExternalManager::getManager()->migrateMaterial(libraryName, path, material);
}

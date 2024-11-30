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

#include <QMutexLocker>

#include "Exceptions.h"
#include "ExternalManager.h"
#include "MaterialLibrary.h"
#include "MaterialManagerExternal.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

QMutex MaterialManagerExternal::_mutex;
LRU::Cache<QString, std::shared_ptr<Material>> MaterialManagerExternal::_cache(100);

TYPESYSTEM_SOURCE(Materials::MaterialManagerExternal, Base::BaseClass)

MaterialManagerExternal::MaterialManagerExternal()
{
    initCache();
}

void MaterialManagerExternal::initCache()
{
    QMutexLocker locker(&_mutex);

    // ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
    //     "User parameter:BaseApp/Preferences/Mod/Material/Database");
    // auto cacheSize = hGrp->GetInt("MaterialCacheSize", 100);
    // _cache.capacity(cacheSize);
    _cache.capacity(100);

    _cache.monitor();
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

std::shared_ptr<Material> MaterialManagerExternal::getMaterial(const QString& uuid) const
{
    if (_cache.contains(uuid)) {
        return _cache.lookup(uuid);
    }
    try {
        auto material = ExternalManager::getManager()->getMaterial(uuid);
        _cache.emplace(uuid, material);
        return material;
    }
    catch (const MaterialNotFound& e) {
        _cache.emplace(uuid, nullptr);
        return nullptr;
    }
}

void MaterialManagerExternal::addMaterial(const QString& libraryName,
                                          const QString& path,
                                          const std::shared_ptr<Material>& material)
{
    _cache.erase(material->getUUID());
    ExternalManager::getManager()->addMaterial(libraryName, path, material);
}

void MaterialManagerExternal::migrateMaterial(const QString& libraryName,
                                          const QString& path,
                                          const std::shared_ptr<Material>& material)
{
    _cache.erase(material->getUUID());
    ExternalManager::getManager()->migrateMaterial(libraryName, path, material);
}

// SPDX-License-Identifier: LGPL-2.1-or-later

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


#include <App/Application.h>

#include "Exceptions.h"
#include "ExternalManager.h"
#include "MaterialLibrary.h"
#include "MaterialManagerExternal.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

std::mutex MaterialManagerExternal::_mutex;
LRU::Cache<std::string, std::shared_ptr<Material>>
    MaterialManagerExternal::_cache(DEFAULT_CACHE_SIZE);

TYPESYSTEM_SOURCE(Materials::MaterialManagerExternal, Base::BaseClass)

MaterialManagerExternal::MaterialManagerExternal()
{
    initCache();
}

void MaterialManagerExternal::initCache()
{
    std::lock_guard<std::mutex> locker(_mutex);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface");
    auto cacheSize = hGrp->GetInt("MaterialCacheSize", DEFAULT_CACHE_SIZE);
    _cache.capacity(cacheSize);

    _cache.monitor();
}

void MaterialManagerExternal::cleanup()
{}

void MaterialManagerExternal::refresh()
{
    resetCache();
}

//=====
//
// Library management
//
//=====

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialManagerExternal::getLibraries()
{
    auto libraryList = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();
    try {
        auto externalLibraries = ExternalManager::getManager()->libraries();
        for (auto& entry : *externalLibraries) {
            auto library = std::make_shared<MaterialLibrary>(*entry);
            libraryList->push_back(library);
        }
    }
    catch (const LibraryNotFound& e) {
    }
    catch (const ConnectionError& e) {
    }

    return libraryList;
}

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>
MaterialManagerExternal::getMaterialLibraries()
{
    auto libraryList = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();
    try {
        auto externalLibraries = ExternalManager::getManager()->materialLibraries();
        for (auto& entry : *externalLibraries) {
            auto library = std::make_shared<MaterialLibrary>(*entry);
            libraryList->push_back(library);
        }
    }
    catch (const LibraryNotFound& e) {
    }
    catch (const ConnectionError& e) {
    }

    return libraryList;
}

std::shared_ptr<MaterialLibrary> MaterialManagerExternal::getLibrary(const std::string& name) const
{
    try {
        auto lib = ExternalManager::getManager()->getLibrary(name);
        auto library = std::make_shared<MaterialLibrary>(*lib);
        return library;
    }
    catch (const LibraryNotFound& e) {
        throw LibraryNotFound(e);
    }
    catch (const ConnectionError& e) {
        throw LibraryNotFound(e.what());
    }
    catch (...) {
        throw LibraryNotFound("Unknown exception");
    }
}

void MaterialManagerExternal::createLibrary(const std::string& libraryName,
                                            const QByteArray& icon,
                                            bool readOnly)
{
    ExternalManager::getManager()->createLibrary(libraryName, icon, readOnly);
}

void MaterialManagerExternal::renameLibrary(const std::string& libraryName, const std::string& newName)
{
    ExternalManager::getManager()->renameLibrary(libraryName, newName);
}

void MaterialManagerExternal::changeIcon(const std::string& libraryName, const QByteArray& icon)
{
    ExternalManager::getManager()->changeIcon(libraryName, icon);
}

void MaterialManagerExternal::removeLibrary(const std::string& libraryName)
{
    ExternalManager::getManager()->removeLibrary(libraryName);
}

std::shared_ptr<std::vector<LibraryObject>>
MaterialManagerExternal::libraryMaterials(const std::string& libraryName)
{
    return ExternalManager::getManager()->libraryMaterials(libraryName);
}

std::shared_ptr<std::vector<LibraryObject>>
MaterialManagerExternal::libraryMaterials(const std::string& libraryName,
                                          const MaterialFilter& filter,
                                          const MaterialFilterOptions& options)
{
    return ExternalManager::getManager()->libraryMaterials(libraryName, filter, options);
}

//=====
//
// Folder management
//
//=====

void MaterialManagerExternal::createFolder(const MaterialLibrary& library,
                                           const std::string& path)
{
    ExternalManager::getManager()->createFolder(library.getName(), path);
}

void MaterialManagerExternal::renameFolder(const MaterialLibrary& library,
                                           const std::string& oldPath,
                                           const std::string& newPath)
{
    ExternalManager::getManager()->renameFolder(library.getName(), oldPath, newPath);
}

void MaterialManagerExternal::deleteRecursive(const MaterialLibrary& library,
                                              const std::string& path)
{
    ExternalManager::getManager()->deleteRecursive(library.getName(), path);
}

//=====
//
// Material management
//
//=====

std::shared_ptr<Material> MaterialManagerExternal::materialNotFound(const std::string& uuid) const
{
    // Setting the cache value to nullptr prevents repeated lookups
    _cache.emplace(uuid, nullptr);
    return nullptr;
}

std::shared_ptr<Material> MaterialManagerExternal::getMaterial(const std::string& uuid) const
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
        return materialNotFound(uuid);
    }
    catch (const ConnectionError& e) {
        return materialNotFound(uuid);
    }
    catch (...) {
        return materialNotFound(uuid);
    }
}

void MaterialManagerExternal::addMaterial(const std::string& libraryName,
                                          const std::string& path,
                                          const Material& material)
{
    _cache.erase(material.getUUID());
    ExternalManager::getManager()->addMaterial(libraryName, path, material);
}

void MaterialManagerExternal::migrateMaterial(const std::string& libraryName,
                                              const std::string& path,
                                              const Material& material)
{
    _cache.erase(material.getUUID());
    ExternalManager::getManager()->migrateMaterial(libraryName, path, material);
}

//=====
//
// Cache management
//
//=====

void MaterialManagerExternal::resetCache()
{
    _cache.clear();
}

double MaterialManagerExternal::materialHitRate()
{
    auto hitRate = _cache.stats().hit_rate();
    if (std::isnan(hitRate)) {
        return 0;
    }
    return hitRate;
}

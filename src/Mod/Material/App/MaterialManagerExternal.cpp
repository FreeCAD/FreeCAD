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

#include <App/Application.h>

#include "Exceptions.h"
#include "ExternalManager.h"
#include "MaterialLibrary.h"
#include "MaterialManagerExternal.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

QMutex MaterialManagerExternal::_mutex;
LRU::Cache<std::string, std::shared_ptr<Material>>
    MaterialManagerExternal::_cache(DEFAULT_CACHE_SIZE);

TYPESYSTEM_SOURCE(Materials::MaterialManagerExternal, Base::BaseClass)

MaterialManagerExternal::MaterialManagerExternal()
{
    initCache();
}

void MaterialManagerExternal::initCache()
{
    QMutexLocker locker(&_mutex);

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

std::shared_ptr<MaterialLibrary> MaterialManagerExternal::getLibrary(const QString& name) const
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
}

void MaterialManagerExternal::createLibrary(const QString& libraryName,
                                            const QString& icon,
                                            bool readOnly)
{
    ExternalManager::getManager()->createLibrary(libraryName, icon, readOnly);
}

std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
MaterialManagerExternal::libraryMaterials(const QString& libraryName)
{
    return ExternalManager::getManager()->libraryMaterials(libraryName);
}

std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
MaterialManagerExternal::libraryMaterials(const QString& libraryName,
                                          const std::shared_ptr<MaterialFilter>& filter,
                                          const MaterialFilterOptions& options)
{
    return ExternalManager::getManager()->libraryMaterials(libraryName, filter, options);
}

//=====
//
// Material management
//
//=====

std::shared_ptr<Material> MaterialManagerExternal::getMaterial(const QString& uuid) const
{
    if (_cache.contains(uuid.toStdString())) {
        return _cache.lookup(uuid.toStdString());
    }
    try {
        auto material = ExternalManager::getManager()->getMaterial(uuid);
        _cache.emplace(uuid.toStdString(), material);
        return material;
    }
    catch (const MaterialNotFound& e) {
        _cache.emplace(uuid.toStdString(), nullptr);
        return nullptr;
    }
    catch (const ConnectionError& e) {
        _cache.emplace(uuid.toStdString(), nullptr);
        return nullptr;
    }
}

void MaterialManagerExternal::addMaterial(const QString& libraryName,
                                          const QString& path,
                                          const std::shared_ptr<Material>& material)
{
    _cache.erase(material->getUUID().toStdString());
    ExternalManager::getManager()->addMaterial(libraryName, path, material);
}

void MaterialManagerExternal::migrateMaterial(const QString& libraryName,
                                              const QString& path,
                                              const std::shared_ptr<Material>& material)
{
    _cache.erase(material->getUUID().toStdString());
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

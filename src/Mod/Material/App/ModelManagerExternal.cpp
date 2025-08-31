/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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

#include "Model.h"
#include "ModelLoader.h"
#include "ModelManagerExternal.h"
#include "ExternalManager.h"

using namespace Materials;

QMutex ModelManagerExternal::_mutex;
LRU::Cache<std::string, std::shared_ptr<Model>> ModelManagerExternal::_cache(DEFAULT_CACHE_SIZE);

TYPESYSTEM_SOURCE(Materials::ModelManagerExternal, Base::BaseClass)

ModelManagerExternal::ModelManagerExternal()
{
    initCache();
}

void ModelManagerExternal::initCache()
{
    QMutexLocker locker(&_mutex);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface");
    auto cacheSize = hGrp->GetInt("ModelCacheSize", DEFAULT_CACHE_SIZE);
    _cache.capacity(cacheSize);

    _cache.monitor();
}

void ModelManagerExternal::cleanup()
{
}

void ModelManagerExternal::refresh()
{
    resetCache();
}

//=====
//
// Library management
//
//=====

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManagerExternal::getLibraries()
{
    auto libraryList = std::make_shared<std::list<std::shared_ptr<ModelLibrary>>>();
    try {
        auto externalLibraries = ExternalManager::getManager()->libraries();
        for (auto& entry : *externalLibraries) {
            auto library = std::make_shared<ModelLibrary>(*entry);
            libraryList->push_back(library);
        }
    }
    catch (const LibraryNotFound& e) {
    }
    catch (const ConnectionError& e) {
    }

    return libraryList;
}

std::shared_ptr<ModelLibrary> ModelManagerExternal::getLibrary(const QString& name) const
{
    try {
        auto lib = ExternalManager::getManager()->getLibrary(name);
        auto library = std::make_shared<ModelLibrary>(*lib);
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

void ModelManagerExternal::createLibrary(const QString& libraryName,
                                         const QByteArray& icon,
                                         bool readOnly)
{
    ExternalManager::getManager()->createLibrary(libraryName, icon, readOnly);
}

std::shared_ptr<std::vector<LibraryObject>>
ModelManagerExternal::libraryModels(const QString& libraryName)
{
    return ExternalManager::getManager()->libraryModels(libraryName);
}

//=====
//
// Model management
//
//=====

std::shared_ptr<Model> ModelManagerExternal::modelNotFound(const QString& uuid)
{
    // Setting the cache value to nullptr prevents repeated lookups
    _cache.emplace(uuid.toStdString(), nullptr);
    return nullptr;
}

std::shared_ptr<Model> ModelManagerExternal::getModel(const QString& uuid)
{
    if (_cache.contains(uuid.toStdString())) {
        return _cache.lookup(uuid.toStdString());
    }
    try
    {
        auto model = ExternalManager::getManager()->getModel(uuid);
        _cache.emplace(uuid.toStdString(), model);
        return model;
    }
    catch (const ModelNotFound& e) {
        return modelNotFound(uuid);
    }
    catch (const ConnectionError& e) {
        return modelNotFound(uuid);
    }
    catch (...) {
        return modelNotFound(uuid);
    }
}

std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> ModelManagerExternal::getModels()
{
    // TODO: Implement an external call
    return std::make_shared<std::map<QString, std::shared_ptr<Model>>>();
}

void ModelManagerExternal::addModel(const QString& libraryName,
                                    const QString& path,
                                    const Model& model)
{
    _cache.erase(model.getUUID().toStdString());
    ExternalManager::getManager()->addModel(libraryName, path, model);
}

void ModelManagerExternal::migrateModel(const QString& libraryName,
                                    const QString& path,
                                    const Model& model)
{
    _cache.erase(model.getUUID().toStdString());
    ExternalManager::getManager()->migrateModel(libraryName, path, model);
}

//=====
//
// Cache management
//
//=====

void ModelManagerExternal::resetCache()
{
    _cache.clear();
}

double ModelManagerExternal::modelHitRate()
{
    auto hitRate = _cache.stats().hit_rate();
    if (std::isnan(hitRate)) {
        return 0;
    }
    return hitRate;
}

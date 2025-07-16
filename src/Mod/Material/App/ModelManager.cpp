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

#include <QDirIterator>
#include <QMutexLocker>

#include <App/Application.h>
#include <Base/Console.h>

#include "Model.h"
#include "ModelLoader.h"
#include "ModelManager.h"

#include "ModelManagerLocal.h"
#if defined(BUILD_MATERIAL_EXTERNAL)
#include "ModelManagerExternal.h"
#endif

using namespace Materials;

TYPESYSTEM_SOURCE(Materials::ModelManager, Base::BaseClass)

QMutex ModelManager::_mutex;
bool ModelManager::_useExternal = false;
ModelManager* ModelManager::_manager = nullptr;
std::unique_ptr<ModelManagerLocal> ModelManager::_localManager;
#if defined(BUILD_MATERIAL_EXTERNAL)
std::unique_ptr<ModelManagerExternal> ModelManager::_externalManager;
#endif

ModelManager::ModelManager()
{
    _hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface");
    _useExternal = _hGrp->GetBool("UseExternal", false);
    _hGrp->Attach(this);
}

ModelManager::~ModelManager()
{
    _hGrp->Detach(this);
}

ModelManager& ModelManager::getManager()
{
    if (!_manager) {
        initManagers();
    }

    return *_manager;
}

void ModelManager::initManagers()
{
    QMutexLocker locker(&_mutex);

    if (!_manager) {
        // Can't use smart pointers for this since the constructor is private
        _manager = new ModelManager();
    }
    if (!_localManager) {
        _localManager = std::make_unique<ModelManagerLocal>();
    }

#if defined(BUILD_MATERIAL_EXTERNAL)
    if (!_externalManager) {
        _externalManager = std::make_unique<ModelManagerExternal>();
    }
#endif
}

void ModelManager::OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason)
{
    const ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);
    if (strcmp(Reason, "UseExternal") == 0) {
        Base::Console().log("Use external changed\n");
        _useExternal = rGrp.GetBool("UseExternal", false);
        // _dbManager->refresh();
    }
}

bool ModelManager::isModel(const QString& file)
{
    return ModelManagerLocal::isModel(file);
}

void ModelManager::cleanup()
{
    return ModelManagerLocal::cleanup();
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_externalManager) {
        _externalManager->cleanup();
    }
#endif
}

void ModelManager::refresh()
{
    _localManager->refresh();
}

//=====
//
// Library management
//
//=====

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManager::getLibraries()
{
    // External libraries take precedence over local libraries
    auto libMap = std::map<QString, std::shared_ptr<ModelLibrary>>();
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal) {
        auto remoteLibraries = _externalManager->getLibraries();
        for (auto& remote : *remoteLibraries) {
            libMap.try_emplace(remote->getName(), remote);
        }
    }
#endif
    auto localLibraries = _localManager->getLibraries();
    for (auto& local : *localLibraries) {
        libMap.try_emplace(local->getName(), local);
    }

    // Consolidate into a single list
    auto libraries = std::make_shared<std::list<std::shared_ptr<ModelLibrary>>>();
    for (auto libEntry : libMap) {
        libraries->push_back(libEntry.second);
    }

    return libraries;
}

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManager::getLocalLibraries()
{
    return _localManager->getLibraries();
}

void ModelManager::createLibrary([[maybe_unused]] const QString& libraryName,
                                 [[maybe_unused]] const QString& iconPath,
                                 [[maybe_unused]] bool readOnly)
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    auto icon = Materials::Library::getIcon(iconPath);
    _externalManager->createLibrary(libraryName, icon, readOnly);
#endif
}

std::shared_ptr<ModelLibrary> ModelManager::getLibrary(const QString& name) const
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal) {
        auto library = _externalManager->getLibrary(name);
        if (library) {
            return library;
        }
    }
#endif
    return _localManager->getLibrary(name);
}

void ModelManager::createLocalLibrary(const QString& libraryName,
                                      const QString& directory,
                                      const QString& icon,
                                      bool readOnly)
{
    _localManager->createLibrary(libraryName, directory, icon, readOnly);
}

void ModelManager::renameLibrary(const QString& libraryName, const QString& newName)
{
    _localManager->renameLibrary(libraryName, newName);
}

void ModelManager::changeIcon(const QString& libraryName, const QString& icon)
{
    _localManager->changeIcon(libraryName, icon);
}

void ModelManager::removeLibrary(const QString& libraryName)
{
    _localManager->removeLibrary(libraryName);
}

std::shared_ptr<std::vector<LibraryObject>>
ModelManager::libraryModels(const QString& libraryName)
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal) {
        try {
            auto models = _externalManager->libraryModels(libraryName);
            if (models) {
                return models;
            }
        }
        catch (const LibraryNotFound& e) {
        }
        catch (const InvalidModel& e) {
        }
    }
#endif
    return _localManager->libraryModels(libraryName);
}

bool ModelManager::isLocalLibrary([[maybe_unused]] const QString& libraryName)
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal) {
        try {
            auto lib = _externalManager->getLibrary(libraryName);
            if (lib) {
                return false;
            }
        }
        catch (const LibraryNotFound& e) {
        }
    }
#endif
    return true;
}

//=====
//
// Model management
//
//=====

std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> ModelManager::getModels()
{
    // External libraries take precedence over local libraries
    auto modelMap = std::make_shared<std::map<QString, std::shared_ptr<Model>>>();
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal) {
        auto remoteModels = _externalManager->getModels();
        for (auto& remote : *remoteModels) {
            modelMap->try_emplace(remote.first, remote.second);
        }
    }
#endif
    auto localModels = _localManager->getModels();
    for (auto& local : *localModels) {
        modelMap->try_emplace(local.first, local.second);
    }

    return modelMap;
}

std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> ModelManager::getLocalModels()
{
    return _localManager->getModels();
}

std::shared_ptr<Model> ModelManager::getModel(const QString& /*libraryName*/, const QString& uuid) const
{
    // TODO: Search a specific library
    return getModel(uuid);
}

std::shared_ptr<Model> ModelManager::getModel(const QString& uuid) const
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal) {
        auto model = _externalManager->getModel(uuid);
        if (model) {
            return model;
        }
    }
#endif
    // We really want to return the local model if not found, such as for User folder models
    return _localManager->getModel(uuid);
}

std::shared_ptr<Model> ModelManager::getModelByPath(const QString& path) const
{
    return _localManager->getModelByPath(path);
}

std::shared_ptr<Model> ModelManager::getModelByPath(const QString& path, const QString& lib) const
{
    return _localManager->getModelByPath(path, lib);
}

bool ModelManager::passFilter(ModelFilter filter, Model::ModelType modelType)
{
    switch (filter) {
        case ModelFilter_None:
            return true;

        case ModelFilter_Physical:
            return (modelType == Model::ModelType_Physical);

        case ModelFilter_Appearance:
            return (modelType == Model::ModelType_Appearance);
    }

    return false;
}

#if defined(BUILD_MATERIAL_EXTERNAL)
void ModelManager::migrateToExternal(const std::shared_ptr<Materials::ModelLibrary>& library)
{
    try {
        _externalManager->createLibrary(library->getName(),
                                        library->getIcon(),
                                        library->isReadOnly());
    }
    catch (const CreationError&) {
    }
    catch (const ConnectionError&) {
    }

    auto models = _localManager->libraryModels(library->getName());
    for (auto& it : *models) {
        auto uuid = it.getUUID();
        auto path = it.getPath();
        auto name = it.getName();
        Base::Console().log("\t('%s', '%s', '%s')\n",
                            uuid.toStdString().c_str(),
                            path.toStdString().c_str(),
                            name.toStdString().c_str());

        auto model = _localManager->getModel(uuid);
        _externalManager->migrateModel(library->getName(), path, *model);
    }
}

void ModelManager::validateMigration(const std::shared_ptr<Materials::ModelLibrary>& library)
{
    auto models = _localManager->libraryModels(library->getName());
    for (auto& it : *models) {
        auto uuid = it.getUUID();
        auto path = it.getPath();
        auto name = it.getName();
        Base::Console().log("\t('%s', '%s', '%s')\n",
                            uuid.toStdString().c_str(),
                            path.toStdString().c_str(),
                            name.toStdString().c_str());

        auto model = _localManager->getModel(uuid);
        auto externalModel = _externalManager->getModel(uuid);
        model->validate(*externalModel);
    }
}

// Cache stats
double ModelManager::modelHitRate()
{
    initManagers();
    return _externalManager->modelHitRate();
}
#endif

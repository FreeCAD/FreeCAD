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
        Base::Console().Log("Use external changed\n");
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
}

void ModelManager::refresh()
{
    _localManager->refresh();
}

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManager::getLibraries()
{
    return _localManager->getLibraries();
}

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManager::getLocalLibraries()
{
    return _localManager->getLibraries();
}

void ModelManager::createLibrary(const QString& libraryName, const QString& icon, bool readOnly)
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    _externalManager->createLibrary(libraryName, icon, readOnly);
#endif
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

std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
ModelManager::libraryModels(const QString& libraryName)
{
    return _localManager->libraryModels(libraryName);
}

bool ModelManager::isLocalLibrary(const QString& libraryName)
{
    return true;
}

std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> ModelManager::getModels()
{
    return _localManager->getModels();
}

std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> ModelManager::getLocalModels()
{
    return _localManager->getModels();
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

std::shared_ptr<ModelLibrary> ModelManager::getLibrary(const QString& name) const
{
    return _localManager->getLibrary(name);
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
    _externalManager->createLibrary(library->getName(),
                                    library->getIconPath(),
                                    library->isReadOnly());

    auto models = _localManager->libraryModels(library->getName());
    for (auto& tuple : *models) {
        auto uuid = std::get<0>(tuple);
        auto path = std::get<1>(tuple);
        auto name = std::get<2>(tuple);
        Base::Console().Log("\t('%s', '%s', '%s')\n",
                            uuid.toStdString().c_str(),
                            path.toStdString().c_str(),
                            name.toStdString().c_str());

        auto model = _localManager->getModel(uuid);
        _externalManager->migrateModel(library->getName(), path, model);
    }
}

void ModelManager::validateMigration(const std::shared_ptr<Materials::ModelLibrary>& library)
{
    auto models = _localManager->libraryModels(library->getName());
    for (auto& tuple : *models) {
        auto uuid = std::get<0>(tuple);
        auto path = std::get<1>(tuple);
        auto name = std::get<2>(tuple);
        Base::Console().Log("\t('%s', '%s', '%s')\n",
                            uuid.toStdString().c_str(),
                            path.toStdString().c_str(),
                            name.toStdString().c_str());

        auto model = _localManager->getModel(uuid);
        auto externalModel = _externalManager->getModel(uuid);
        model->validate(externalModel);
    }
}

// Cache stats
double ModelManager::modelHitRate()
{
    initManagers();
    return _externalManager->modelHitRate();
}
#endif

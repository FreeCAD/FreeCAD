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

using namespace Materials;

TYPESYSTEM_SOURCE(Materials::ModelManager, Base::BaseClass)

QMutex ModelManager::_mutex;
ModelManager* ModelManager::_manager = nullptr;
std::unique_ptr<ModelManagerLocal> ModelManager::_localManager;

ModelManager::ModelManager()
{}

ModelManager::~ModelManager()
{}

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
{}

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

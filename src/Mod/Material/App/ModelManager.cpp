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

#include <Base/Console.h>

#include "Model.h"
#include "ModelLoader.h"
#include "ModelManager.h"


using namespace Materials;

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManager::_libraryList = nullptr;
std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> ModelManager::_modelMap = nullptr;
QMutex ModelManager::_mutex;

TYPESYSTEM_SOURCE(Materials::ModelManager, Base::BaseClass)

ModelManager::ModelManager()
{
    initLibraries();
}

void ModelManager::initLibraries()
{
    QMutexLocker locker(&_mutex);

    if (_modelMap == nullptr) {
        _modelMap = std::make_shared<std::map<QString, std::shared_ptr<Model>>>();
        if (_libraryList == nullptr) {
            _libraryList = std::make_shared<std::list<std::shared_ptr<ModelLibrary>>>();
        }

        // Load the libraries
        ModelLoader loader(_modelMap, _libraryList);
    }
}

bool ModelManager::isModel(const QString& file)
{
    // if (!fs::is_regular_file(p))
    //     return false;
    // check file extension
    if (file.endsWith(QStringLiteral(".yml"))) {
        return true;
    }
    return false;
}

void ModelManager::cleanup()
{
    if (_libraryList) {
        _libraryList->clear();
    }

    if (_modelMap) {
        for (auto& it : *_modelMap) {
            // This is needed to resolve cyclic dependencies
            it.second->setLibrary(nullptr);
        }
        _modelMap->clear();
    }
}

void ModelManager::refresh()
{
    _modelMap->clear();
    _libraryList->clear();

    // Load the libraries
    ModelLoader loader(_modelMap, _libraryList);
}

std::shared_ptr<Model> ModelManager::getModel(const QString& uuid) const
{
    try {
        if (_modelMap == nullptr) {
            throw Uninitialized();
        }

        return _modelMap->at(uuid);
    }
    catch (std::out_of_range const&) {
        throw ModelNotFound();
    }
}

std::shared_ptr<Model> ModelManager::getModelByPath(const QString& path) const
{
    QString cleanPath = QDir::cleanPath(path);

    for (auto& library : *_libraryList) {
        if (cleanPath.startsWith(library->getDirectory())) {
            return library->getModelByPath(cleanPath);
        }
    }

    throw MaterialNotFound();
}

std::shared_ptr<Model> ModelManager::getModelByPath(const QString& path, const QString& lib) const
{
    auto library = getLibrary(lib);        // May throw LibraryNotFound
    return library->getModelByPath(path);  // May throw ModelNotFound
}

std::shared_ptr<ModelLibrary> ModelManager::getLibrary(const QString& name) const
{
    for (auto& library : *_libraryList) {
        if (library->getName() == name) {
            return library;
        }
    }

    throw LibraryNotFound();
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

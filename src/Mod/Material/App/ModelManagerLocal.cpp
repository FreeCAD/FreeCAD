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

#include <QDirIterator>
#include <QMutexLocker>

#include <Base/Console.h>

#include "Model.h"
#include "ModelLoader.h"
#include "ModelManagerLocal.h"

using namespace Materials;

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManagerLocal::_libraryList = nullptr;
std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> ModelManagerLocal::_modelMap = nullptr;
QMutex ModelManagerLocal::_mutex;


TYPESYSTEM_SOURCE(Materials::ModelManagerLocal, Base::BaseClass)

ModelManagerLocal::ModelManagerLocal()
{
    initLibraries();
}

void ModelManagerLocal::initLibraries()
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

bool ModelManagerLocal::isModel(const QString& file)
{
    // if (!fs::is_regular_file(p))
    //     return false;
    // check file extension
    if (file.endsWith(QString::fromStdString(".yml"))) {
        return true;
    }
    return false;
}

void ModelManagerLocal::cleanup()
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

void ModelManagerLocal::refresh()
{
    _modelMap->clear();
    _libraryList->clear();

    // Load the libraries
    ModelLoader loader(_modelMap, _libraryList);
}

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManagerLocal::getLibraries()
{
    return _libraryList;
}

void ModelManagerLocal::createLibrary(const QString& libraryName,
                                      const QString& directory,
                                      const QString& icon,
                                      bool readOnly)
{
    QDir dir;
    if (!dir.exists(directory)) {
        if (!dir.mkpath(directory)) {
            throw CreationError("Unable to create library path");
        }
    }

    auto modelLibrary = std::make_shared<ModelLibrary>(libraryName, directory, icon, readOnly);
    _libraryList->push_back(modelLibrary);

    // This needs to be persisted somehow
}

void ModelManagerLocal::renameLibrary(const QString& libraryName, const QString& newName)
{
    for (auto& library : *_libraryList) {
        if (library->getName() == libraryName) {
            library->setName(newName);
            return;
        }
    }

    throw LibraryNotFound();
}

void ModelManagerLocal::changeIcon(const QString& libraryName, const QString& icon)
{
    for (auto& library : *_libraryList) {
        if (library->getName() == libraryName) {
            library->setIconPath(icon);
            return;
        }
    }

    throw LibraryNotFound();
}

void ModelManagerLocal::removeLibrary(const QString& libraryName)
{
    for (auto& library : *_libraryList) {
        if (library->getName() == libraryName) {
            _libraryList->remove(library);

            // At this point we should rebuild the model map
            return;
        }
    }

    throw LibraryNotFound();
}

std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
ModelManagerLocal::libraryModels(const QString& libraryName)
{
    auto models = std::make_shared<std::vector<std::tuple<QString, QString, QString>>>();

    for (auto& it : *_modelMap) {
        // This is needed to resolve cyclic dependencies
        if (it.second->getLibrary()->getName() == libraryName) {
            models->push_back(
                std::tuple<QString, QString, QString>(it.first, it.second->getDirectory(), it.second->getName()));
        }
    }

    return models;
}

std::shared_ptr<Model> ModelManagerLocal::getModel(const QString& uuid) const
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

std::shared_ptr<Model> ModelManagerLocal::getModelByPath(const QString& path) const
{
    QString cleanPath = QDir::cleanPath(path);

    for (auto& library : *_libraryList) {
        if (cleanPath.startsWith(library->getDirectory())) {
            return library->getModelByPath(cleanPath);
        }
    }

    throw MaterialNotFound();
}

std::shared_ptr<Model> ModelManagerLocal::getModelByPath(const QString& path,
                                                         const QString& lib) const
{
    auto library = getLibrary(lib);        // May throw LibraryNotFound
    return library->getModelByPath(path);  // May throw ModelNotFound
}

std::shared_ptr<ModelLibrary> ModelManagerLocal::getLibrary(const QString& name) const
{
    for (auto& library : *_libraryList) {
        if (library->getName() == name) {
            return library;
        }
    }

    throw LibraryNotFound();
}

// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <QDirIterator>

#include <Base/Console.h>

#include "Model.h"
#include "ModelLoader.h"
#include "ModelManagerLocal.h"

using namespace Materials;

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManagerLocal::_libraryList = nullptr;
std::shared_ptr<std::map<std::string, std::shared_ptr<Model>>> ModelManagerLocal::_modelMap = nullptr;
std::mutex ModelManagerLocal::_mutex;


TYPESYSTEM_SOURCE(Materials::ModelManagerLocal, Base::BaseClass)

ModelManagerLocal::ModelManagerLocal()
{
    initLibraries();
}

void ModelManagerLocal::initLibraries()
{
    std::lock_guard<std::mutex> locker(_mutex);

    if (_modelMap == nullptr) {
        _modelMap = std::make_shared<std::map<std::string, std::shared_ptr<Model>>>();
        if (_libraryList == nullptr) {
            _libraryList = std::make_shared<std::list<std::shared_ptr<ModelLibrary>>>();
        }

        // Load the libraries
        ModelLoader loader(_modelMap, _libraryList);
    }
}

bool ModelManagerLocal::isModel(const std::string& file)
{
    // if (!fs::is_regular_file(p))
    //     return false;
    // check file extension
    return file.ends_with(".yml");
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

void ModelManagerLocal::createLibrary(const std::string& libraryName,
                                      const std::string& directory,
                                      const std::string& icon,
                                      bool readOnly)
{
    QDir dir;
    const QString libraryPath = QString::fromStdString(directory);
    if (!dir.exists(libraryPath)) {
        if (!dir.mkpath(libraryPath)) {
            throw CreationError("Unable to create library path");
        }
    }

    auto modelLibrary = std::make_shared<ModelLibraryLocal>(libraryName, directory, icon, readOnly);
    _libraryList->push_back(modelLibrary);
}

void ModelManagerLocal::renameLibrary(const std::string& libraryName, const std::string& newName)
{
    for (auto& library : *_libraryList) {
        if (library->isName(libraryName)) {
            library->setName(newName);
            return;
        }
    }

    throw LibraryNotFound();
}

void ModelManagerLocal::changeIcon(const std::string& libraryName, const std::string& icon)
{
    for (auto& library : *_libraryList) {
        if (library->isName(libraryName)) {
            library->setIcon(icon);
            return;
        }
    }

    throw LibraryNotFound();
}

void ModelManagerLocal::removeLibrary(const std::string& libraryName)
{
    for (auto& library : *_libraryList) {
        if (library->isName(libraryName)) {
            _libraryList->remove(library);

            // At this point we should rebuild the model map
            return;
        }
    }

    throw LibraryNotFound();
}

std::shared_ptr<std::vector<LibraryObject>>
ModelManagerLocal::libraryModels(const std::string& libraryName)
{
    auto models = std::make_shared<std::vector<LibraryObject>>();

    for (auto& it : *_modelMap) {
        // This is needed to resolve cyclic dependencies
        if (it.second->getLibrary()->isName(libraryName)) {
            models->push_back(
                LibraryObject(it.first, it.second->getDirectory(), it.second->getName()));
        }
    }

    return models;
}

std::shared_ptr<Model> ModelManagerLocal::getModel(const std::string& uuid) const
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

std::shared_ptr<Model> ModelManagerLocal::getModelByPath(const std::string& path) const
{
    const std::string cleanPath = Library::cleanPath(path);

    for (auto& library : *_libraryList) {
        if (library->isLocal()) {
            auto localLibrary = std::static_pointer_cast<Materials::ModelLibraryLocal> (library);
            if (localLibrary->startsWithDirectory(cleanPath)) {
                return localLibrary->getModelByPath(cleanPath);
            }
        }
    }

    throw ModelNotFound();
}

std::shared_ptr<Model> ModelManagerLocal::getModelByPath(const std::string& path,
                                                         const std::string& lib) const
{
    auto library = getLibrary(lib);        // May throw LibraryNotFound
    if (library->isLocal()) {
        auto localLibrary = std::static_pointer_cast<Materials::ModelLibraryLocal>(library);
        return localLibrary->getModelByPath(path);  // May throw ModelNotFound
    }

    throw ModelNotFound();
}

std::shared_ptr<ModelLibrary> ModelManagerLocal::getLibrary(const std::string& name) const
{
    for (auto& library : *_libraryList) {
        if (library->isName(name)) {
            return library;
        }
    }

    throw LibraryNotFound();
}

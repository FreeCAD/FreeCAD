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

#include "Exceptions.h"
#include "MaterialLoader.h"
#include "MaterialManager.h"
#include "ModelManager.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialManager::_libraryList =
    nullptr;
std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> MaterialManager::_materialMap =
    nullptr;
QMutex MaterialManager::_mutex;

TYPESYSTEM_SOURCE(Materials::MaterialManager, Base::BaseClass)

MaterialManager::MaterialManager()
{
    // TODO: Add a mutex or similar
    initLibraries();
}

void MaterialManager::initLibraries()
{
    QMutexLocker locker(&_mutex);

    if (_materialMap == nullptr) {
        // Load the models first
        ModelManager* manager = new ModelManager();
        Q_UNUSED(manager)

        delete manager;

        _materialMap = std::make_shared<std::map<QString, std::shared_ptr<Material>>>();

        if (_libraryList == nullptr) {
            _libraryList = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();
        }

        // Load the libraries
        MaterialLoader loader(_materialMap, _libraryList);
    }
}

void MaterialManager::saveMaterial(std::shared_ptr<MaterialLibrary> library,
                                   std::shared_ptr<Material> material,
                                   const QString& path,
                                   bool overwrite,
                                   bool saveAsCopy,
                                   bool saveInherited)
{
    auto newMaterial = library->saveMaterial(material, path, overwrite, saveAsCopy, saveInherited);
    (*_materialMap)[newMaterial->getUUID()] = newMaterial;
}

bool MaterialManager::isMaterial(const fs::path& p)
{
    if (!fs::is_regular_file(p)) {
        return false;
    }
    // check file extension
    if (p.extension() == ".FCMat") {
        return true;
    }
    return false;
}

bool MaterialManager::isMaterial(const QFileInfo& file)
{
    if (!file.isFile()) {
        return false;
    }
    // check file extension
    if (file.suffix() == QString::fromStdString("FCMat")) {
        return true;
    }
    return false;
}

std::shared_ptr<Material> MaterialManager::getMaterial(const QString& uuid) const
{
    try {
        return _materialMap->at(uuid);
    }
    catch (std::out_of_range& e) {
        throw MaterialNotFound();
    }
}

std::shared_ptr<Material> MaterialManager::getMaterialByPath(const QString& path) const
{
    QString cleanPath = QDir::cleanPath(path);

    for (auto library : *_libraryList) {
        // Base::Console().Log("MaterialManager::getMaterialByPath() Checking library '%s'->'%s'\n",
        //                     library->getName().toStdString().c_str(),
        //                     library->getDirectory().toStdString().c_str());


        if (cleanPath.startsWith(library->getDirectory())) {
            // Base::Console().Log("MaterialManager::getMaterialByPath() Library '%s'\n",
            //                     library->getDirectory().toStdString().c_str());
            // Base::Console().Log("MaterialManager::getMaterialByPath() Path '%s'\n",
            //                     cleanPath.toStdString().c_str());
            return library->getMaterialByPath(cleanPath);
        }
    }
    Base::Console().Log("MaterialManager::getMaterialByPath() Library not found for path '%s'\n",
                        cleanPath.toStdString().c_str());

    throw MaterialNotFound();
}

std::shared_ptr<Material> MaterialManager::getMaterialByPath(const QString& path,
                                                             const QString& lib) const
{
    auto library = getLibrary(lib);           // May throw LibraryNotFound
    return library->getMaterialByPath(path);  // May throw MaterialNotFound
}

bool MaterialManager::exists(const QString& uuid) const
{
    try {
        auto material = getMaterial(uuid);
        if (material.get() != nullptr) {
            return true;
        }
    }
    catch (const MaterialNotFound&) {
    }

    return false;
}

std::shared_ptr<Material> MaterialManager::getParent(std::shared_ptr<Material> material)
{
    if (material->getParentUUID().isEmpty()) {
        throw MaterialNotFound();
    }

    return getMaterial(material->getParentUUID());
}

bool MaterialManager::exists(std::shared_ptr<MaterialLibrary> library, const QString& uuid) const
{
    try {
        auto material = getMaterial(uuid);
        if (material.get() != nullptr) {
            return (*material->getLibrary() == *library);
        }
    }
    catch (const MaterialNotFound&) {
    }

    return false;
}

std::shared_ptr<MaterialLibrary> MaterialManager::getLibrary(const QString& name) const
{
    for (auto library : *_libraryList) {
        if (library->getName() == name) {
            return library;
        }
    }

    throw LibraryNotFound();
}

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialManager::getMaterialLibraries()
{
    if (_libraryList == nullptr) {
        if (_materialMap == nullptr) {
            _materialMap = std::make_shared<std::map<QString, std::shared_ptr<Material>>>();
        }
        _libraryList = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();

        // Load the libraries
        MaterialLoader loader(_materialMap, _libraryList);
    }
    return _libraryList;
}

std::shared_ptr<std::list<QString>>
MaterialManager::getMaterialFolders(std::shared_ptr<MaterialLibrary> library) const
{
    return MaterialLoader::getMaterialFolders(*library);
}

std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>
MaterialManager::materialsWithModel(QString uuid)
{
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> dict =
        std::make_shared<std::map<QString, std::shared_ptr<Material>>>();

    for (auto it = _materialMap->begin(); it != _materialMap->end(); it++) {
        QString key = it->first;
        auto material = it->second;

        if (material->hasModel(uuid)) {
            (*dict)[key] = material;
        }
    }

    return dict;
}

std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>
MaterialManager::materialsWithModelComplete(QString uuid)
{
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> dict =
        std::make_shared<std::map<QString, std::shared_ptr<Material>>>();

    for (auto it = _materialMap->begin(); it != _materialMap->end(); it++) {
        QString key = it->first;
        auto material = it->second;

        if (material->isModelComplete(uuid)) {
            (*dict)[key] = material;
        }
    }

    return dict;
}

void MaterialManager::dereference(std::shared_ptr<Material> material)
{
    MaterialLoader::dereference(_materialMap, material);
}

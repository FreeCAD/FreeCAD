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
#include "MaterialConfigLoader.h"
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
        auto manager = std::make_unique<ModelManager>();
        Q_UNUSED(manager)

        _materialMap = std::make_shared<std::map<QString, std::shared_ptr<Material>>>();

        if (_libraryList == nullptr) {
            _libraryList = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();
        }

        // Load the libraries
        MaterialLoader loader(_materialMap, _libraryList);
    }
}

void MaterialManager::saveMaterial(const std::shared_ptr<MaterialLibrary>& library,
                                   const std::shared_ptr<Material>& material,
                                   const QString& path,
                                   bool overwrite,
                                   bool saveAsCopy,
                                   bool saveInherited) const
{
    auto newMaterial = library->saveMaterial(material, path, overwrite, saveAsCopy, saveInherited);
    (*_materialMap)[newMaterial->getUUID()] = newMaterial;
}

bool MaterialManager::isMaterial(const fs::path& p) const
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

bool MaterialManager::isMaterial(const QFileInfo& file) const
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
    catch (std::out_of_range&) {
        throw MaterialNotFound();
    }
}

std::shared_ptr<Material> MaterialManager::getMaterialByPath(const QString& path) const
{
    QString cleanPath = QDir::cleanPath(path);

    for (auto& library : *_libraryList) {
        if (cleanPath.startsWith(library->getDirectory())) {
            try {
                return library->getMaterialByPath(cleanPath);
            } catch (const MaterialNotFound&) {}

            // See if it's a new file saved by the old editor
            {
                QMutexLocker locker(&_mutex);

                if (MaterialConfigLoader::isConfigStyle(path)) {
                    auto material = MaterialConfigLoader::getMaterialFromPath(library, path);
                    if (material) {
                        (*_materialMap)[material->getUUID()] = library->addMaterial(material, path);
                    }

                    return material;
                }
            }
        }
    }

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
        if (material) {
            return true;
        }
    }
    catch (const MaterialNotFound&) {
    }

    return false;
}

std::shared_ptr<Material>
MaterialManager::getParent(const std::shared_ptr<Material>& material) const
{
    if (material->getParentUUID().isEmpty()) {
        throw MaterialNotFound();
    }

    return getMaterial(material->getParentUUID());
}

bool MaterialManager::exists(const std::shared_ptr<MaterialLibrary>& library,
                             const QString& uuid) const
{
    try {
        auto material = getMaterial(uuid);
        if (material) {
            return (*material->getLibrary() == *library);
        }
    }
    catch (const MaterialNotFound&) {
    }

    return false;
}

std::shared_ptr<MaterialLibrary> MaterialManager::getLibrary(const QString& name) const
{
    for (auto& library : *_libraryList) {
        if (library->getName() == name) {
            return library;
        }
    }

    throw LibraryNotFound();
}

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>
MaterialManager::getMaterialLibraries() const
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
MaterialManager::getMaterialFolders(const std::shared_ptr<MaterialLibrary>& library) const
{
    return MaterialLoader::getMaterialFolders(*library);
}

std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>
MaterialManager::materialsWithModel(const QString& uuid) const
{
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> dict =
        std::make_shared<std::map<QString, std::shared_ptr<Material>>>();

    for (auto& it : *_materialMap) {
        QString key = it.first;
        auto material = it.second;

        if (material->hasModel(uuid)) {
            (*dict)[key] = material;
        }
    }

    return dict;
}

std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>
MaterialManager::materialsWithModelComplete(const QString& uuid) const
{
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> dict =
        std::make_shared<std::map<QString, std::shared_ptr<Material>>>();

    for (auto& it : *_materialMap) {
        QString key = it.first;
        auto material = it.second;

        if (material->isModelComplete(uuid)) {
            (*dict)[key] = material;
        }
    }

    return dict;
}

void MaterialManager::dereference() const
{
    // First clear the inheritences
    for (auto& it : *_materialMap) {
        auto material = it.second;
        material->clearDereferenced();
        material->clearInherited();
    }

    // Run the dereference again
    for (auto& it : *_materialMap) {
        dereference(it.second);
    }
}

void MaterialManager::dereference(std::shared_ptr<Material> material) const
{
    MaterialLoader::dereference(_materialMap, material);
}

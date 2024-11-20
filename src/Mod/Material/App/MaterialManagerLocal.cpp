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
#include <random>
#endif

#include <QDirIterator>
#include <QMutex>
#include <QMutexLocker>

#include <App/Application.h>
#include <App/Material.h>

#include "Exceptions.h"
#include "MaterialConfigLoader.h"
#include "MaterialLoader.h"
#include "MaterialManagerLocal.h"
#include "ModelManager.h"
#include "ModelUuids.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialManagerLocal::_libraryList =
    nullptr;
std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> MaterialManagerLocal::_materialMap =
    nullptr;
QMutex MaterialManagerLocal::_mutex;

TYPESYSTEM_SOURCE(Materials::MaterialManagerLocal, Base::BaseClass)

MaterialManagerLocal::MaterialManagerLocal()
{
    // TODO: Add a mutex or similar
    initLibraries();
}

void MaterialManagerLocal::initLibraries()
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

void MaterialManagerLocal::cleanup()
{
    QMutexLocker locker(&_mutex);

    if (_libraryList) {
        _libraryList->clear();
        _libraryList = nullptr;
    }

    if (_materialMap) {
        for (auto& it : *_materialMap) {
            // This is needed to resolve cyclic dependencies
            it.second->setLibrary(nullptr);
        }
        _materialMap->clear();
        _materialMap = nullptr;
    }
}

void MaterialManagerLocal::refresh()
{
    // This is very expensive and can be improved using observers?
    cleanup();
    initLibraries();
}

std::shared_ptr<std::vector<Library>> MaterialManagerLocal::getLibraries()
{
    auto libraries = std::shared_ptr<std::vector<Library>>();

    for (auto& library : *_libraryList) {
        libraries->push_back(
            Library(library->getName(), library->getIconPath(), library->isReadOnly()));
    }

    return libraries;
}

void MaterialManagerLocal::createLibrary(const QString& libraryName,
                                         const QString& directory,
                                         const QString& icon,
                                         bool readOnly)
{
    QDir dir;
    if (!dir.exists(directory)) {
        if (!dir.mkpath(directory)) {
            throw LibraryCreationError();
        }
    }

    auto materialLibrary = std::make_shared<MaterialLibrary>(libraryName, directory, icon, readOnly);
    _libraryList->push_back(materialLibrary);

    // This needs to be persisted somehow
}

void MaterialManagerLocal::renameLibrary(const QString& libraryName, const QString& newName)
{
    for (auto& library : *_libraryList) {
        if (library->getName() == libraryName) {
            library->setName(newName);
            return;
        }
    }

    throw LibraryNotFound();
}

void MaterialManagerLocal::changeIcon(const QString& libraryName, const QString& icon)
{
    for (auto& library : *_libraryList) {
        if (library->getName() == libraryName) {
            library->setIconPath(icon);
            return;
        }
    }

    throw LibraryNotFound();
}

void MaterialManagerLocal::removeLibrary(const QString& libraryName)
{
    for (auto& library : *_libraryList) {
        if (library->getName() == libraryName) {
            _libraryList->remove(library);

            // At this point we should rebuild the material map
            return;
        }
    }

    throw LibraryNotFound();
}

std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
MaterialManagerLocal::libraryMaterials(const QString& libraryName)
{
    auto materials = std::make_shared<std::vector<std::tuple<QString, QString, QString>>>();

    for (auto& it : *_materialMap) {
        // This is needed to resolve cyclic dependencies
        if (it.second->getLibrary()->getName() == libraryName) {
            materials->push_back(std::tuple<QString, QString, QString>(it.first,
                                                                       it.second->getDirectory(),
                                                                       it.second->getName()));
        }
    }

    return materials;
}

void MaterialManagerLocal::saveMaterial(const std::shared_ptr<MaterialLibrary>& library,
                                        const std::shared_ptr<Material>& material,
                                        const QString& path,
                                        bool overwrite,
                                        bool saveAsCopy,
                                        bool saveInherited) const
{
    auto newMaterial = library->saveMaterial(material, path, overwrite, saveAsCopy, saveInherited);
    (*_materialMap)[newMaterial->getUUID()] = newMaterial;
}

bool MaterialManagerLocal::isMaterial(const fs::path& p) const
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

bool MaterialManagerLocal::isMaterial(const QFileInfo& file) const
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

std::shared_ptr<Material> MaterialManagerLocal::getMaterial(const QString& uuid) const
{
    try {
        return _materialMap->at(uuid);
    }
    catch (std::out_of_range&) {
        throw MaterialNotFound();
    }
}

std::shared_ptr<Material> MaterialManagerLocal::getMaterialByPath(const QString& path) const
{
    QString cleanPath = QDir::cleanPath(path);

    for (auto& library : *_libraryList) {
        if (cleanPath.startsWith(library->getDirectory())) {
            try {
                return library->getMaterialByPath(cleanPath);
            }
            catch (const MaterialNotFound&) {
            }

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

    // Older workbenches may try files outside the context of a library
    {
        QMutexLocker locker(&_mutex);

        if (MaterialConfigLoader::isConfigStyle(path)) {
            auto material = MaterialConfigLoader::getMaterialFromPath(nullptr, path);

            return material;
        }
    }

    throw MaterialNotFound();
}

std::shared_ptr<Material> MaterialManagerLocal::getMaterialByPath(const QString& path,
                                                                  const QString& lib) const
{
    auto library = getLibrary(lib);           // May throw LibraryNotFound
    return library->getMaterialByPath(path);  // May throw MaterialNotFound
}

bool MaterialManagerLocal::exists(const QString& uuid) const
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

bool MaterialManagerLocal::exists(const std::shared_ptr<MaterialLibrary>& library,
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

std::shared_ptr<MaterialLibrary> MaterialManagerLocal::getLibrary(const QString& name) const
{
    for (auto& library : *_libraryList) {
        if (library->getName() == name) {
            return library;
        }
    }

    throw LibraryNotFound();
}

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>
MaterialManagerLocal::getMaterialLibraries() const
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

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>
MaterialManagerLocal::getLocalMaterialLibraries() const
{
    return getMaterialLibraries();
}

std::shared_ptr<std::list<QString>>
MaterialManagerLocal::getMaterialFolders(const std::shared_ptr<MaterialLibrary>& library) const
{
    return MaterialLoader::getMaterialFolders(*library);
}

std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>
MaterialManagerLocal::materialsWithModel(const QString& uuid) const
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
MaterialManagerLocal::materialsWithModelComplete(const QString& uuid) const
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

void MaterialManagerLocal::dereference() const
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

void MaterialManagerLocal::dereference(std::shared_ptr<Material> material) const
{
    MaterialLoader::dereference(_materialMap, material);
}

// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <random>

#include <QDirIterator>

#include <App/Application.h>
#include <App/Material.h>

#include "Exceptions.h"
#include "MaterialConfigLoader.h"
#include "MaterialFilter.h"
#include "MaterialLibrary.h"
#include "MaterialLoader.h"
#include "MaterialManagerLocal.h"
#include "ModelManager.h"
#include "ModelUuids.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>
    MaterialManagerLocal::_libraryList = nullptr;
std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> MaterialManagerLocal::_materialMap =
    nullptr;
std::mutex MaterialManagerLocal::_mutex;

TYPESYSTEM_SOURCE(Materials::MaterialManagerLocal, Base::BaseClass)

MaterialManagerLocal::MaterialManagerLocal()
{
    // TODO: Add a mutex or similar
    initLibraries();
}

void MaterialManagerLocal::initLibraries()
{
    std::lock_guard<std::mutex> locker(_mutex);

    if (_materialMap == nullptr) {
        // Load the models first
        ModelManager::getManager();

        _materialMap = std::make_shared<std::map<std::string, std::shared_ptr<Material>>>();

        if (_libraryList == nullptr) {
            _libraryList = getConfiguredLibraries();
        }

        // Load the libraries
        MaterialLoader loader(_materialMap, _libraryList);
    }
}

void MaterialManagerLocal::cleanup()
{
    std::lock_guard<std::mutex> locker(_mutex);

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

//=====
//
// Library management
//
//=====

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialManagerLocal::getLibraries()
{
    if (_libraryList == nullptr) {
        initLibraries();
    }
    return _libraryList;
}

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>
MaterialManagerLocal::getMaterialLibraries()
{
    if (_libraryList == nullptr) {
        initLibraries();
    }
    return _libraryList;
}

std::shared_ptr<MaterialLibrary> MaterialManagerLocal::getLibrary(const std::string& name) const
{
    for (auto& library : *_libraryList) {
        if (library->isLocal() && library->isName(name)) {
            return library;
        }
    }

    throw LibraryNotFound();
}

void MaterialManagerLocal::createLibrary(const std::string& libraryName,
                                         const std::string& directory,
                                         const std::string& iconPath,
                                         bool readOnly)
{
    QDir dir;
    const QString libraryPath = QString::fromStdString(directory);
    if (!dir.exists(libraryPath)) {
        if (!dir.mkpath(libraryPath)) {
            throw CreationError("Unable to create library path");
        }
    }

    auto materialLibrary =
        std::make_shared<MaterialLibraryLocal>(libraryName, directory, iconPath, readOnly);
    _libraryList->push_back(materialLibrary);

    // This needs to be persisted somehow
}

void MaterialManagerLocal::renameLibrary(const std::string& libraryName, const std::string& newName)
{
    for (auto& library : *_libraryList) {
        if (library->isLocal() && library->isName(libraryName)) {
            auto materialLibrary =
                std::dynamic_pointer_cast<Materials::MaterialLibraryLocal>(library);
            if (!materialLibrary) {
                throw LibraryNotFound();
            }
            materialLibrary->setName(newName);
            return;
        }
    }

    throw LibraryNotFound();
}

void MaterialManagerLocal::changeIcon(const std::string& libraryName, const QByteArray& icon)
{
    for (auto& library : *_libraryList) {
        if (library->isLocal() && library->isName(libraryName)) {
            auto materialLibrary =
                std::dynamic_pointer_cast<Materials::MaterialLibraryLocal>(library);
            if (!materialLibrary) {
                throw LibraryNotFound();
            }
            materialLibrary->setIcon(icon);
            return;
        }
    }

    throw LibraryNotFound();
}

void MaterialManagerLocal::removeLibrary(const std::string& libraryName)
{
    for (auto& library : *_libraryList) {
        if (library->isLocal() && library->isName(libraryName)) {
            _libraryList->remove(library);

            // At this point we should rebuild the material map
            return;
        }
    }

    throw LibraryNotFound();
}

std::shared_ptr<std::vector<LibraryObject>>
MaterialManagerLocal::libraryMaterials(const std::string& libraryName)
{
    auto materials = std::make_shared<std::vector<LibraryObject>>();

    for (auto& it : *_materialMap) {
        // This is needed to resolve cyclic dependencies
        auto library = it.second->getLibrary();
        if (library->isName(libraryName)) {
            materials->push_back(
                LibraryObject(it.first, it.second->getDirectory(), it.second->getName()));
        }
    }

    return materials;
}

bool MaterialManagerLocal::passFilter(const Material& material,
                                          const Materials::MaterialFilter& filter,
                                          const Materials::MaterialFilterOptions& options) const
{
    // filter out old format files
    if (material.isOldFormat() && !options.includeLegacy()) {
        return false;
    }

    // filter based on models
    return filter.modelIncluded(material);
}

std::shared_ptr<std::vector<LibraryObject>>
MaterialManagerLocal::libraryMaterials(const std::string& libraryName,
                                       const MaterialFilter& filter,
                                       const MaterialFilterOptions& options)
{
    auto materials = std::make_shared<std::vector<LibraryObject>>();

    for (auto& it : *_materialMap) {
        // This is needed to resolve cyclic dependencies
        auto library = it.second->getLibrary();
        if (library->isName(libraryName)) {
            if (passFilter(*it.second, filter, options)) {
                materials->push_back(
                    LibraryObject(it.first, it.second->getDirectory(), it.second->getName()));
            }
        }
    }

    return materials;
}

//=====
//
// Folder management
//
//=====

std::shared_ptr<std::list<std::string>>
MaterialManagerLocal::getMaterialFolders(const std::shared_ptr<MaterialLibraryLocal>& library) const
{
    // auto materialLibrary =
    //     reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);
    return MaterialLoader::getMaterialFolders(*library);
}

void MaterialManagerLocal::createFolder(const std::shared_ptr<MaterialLibraryLocal>& library,
                                        const std::string& path)
{
    library->createFolder(path);
}

void MaterialManagerLocal::renameFolder(const std::shared_ptr<MaterialLibraryLocal>& library,
                                        const std::string& oldPath,
                                        const std::string& newPath)
{
    library->renameFolder(oldPath, newPath);
}

void MaterialManagerLocal::deleteRecursive(const std::shared_ptr<MaterialLibraryLocal>& library,
                                           const std::string& path)
{
    library->deleteRecursive(path);
    dereference();
}

//=====
//
// Material management
//
//=====

std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>>
MaterialManagerLocal::getLocalMaterials() const
{
    return _materialMap;
}

std::shared_ptr<Material> MaterialManagerLocal::getMaterial(const std::string& uuid) const
{
    try {
        return _materialMap->at(uuid);
    }
    catch (std::out_of_range&) {
        throw MaterialNotFound();
    }
}

std::shared_ptr<Material> MaterialManagerLocal::getMaterialByPath(const std::string& path) const
{
    const std::string cleanPath = Library::cleanPath(path);

    for (auto& library : *_libraryList) {
        if (library->isLocal()) {
            auto materialLibrary =
                std::dynamic_pointer_cast<Materials::MaterialLibraryLocal>(library);
            if (!materialLibrary) {
                continue;
            }
            if (materialLibrary->startsWithDirectory(cleanPath)) {
                try {
                    return materialLibrary->getMaterialByPath(cleanPath);
                }
                catch (const MaterialNotFound&) {
                }

                // See if it's a new file saved by the old editor
                {
                    std::lock_guard<std::mutex> locker(_mutex);

                    if (MaterialConfigLoader::isConfigStyle(path)) {
                        auto material =
                            MaterialConfigLoader::getMaterialFromPath(materialLibrary, path);
                        if (material) {
                            (*_materialMap)[material->getUUID()] =
                                materialLibrary->addMaterial(material, path);
                        }

                        return material;
                    }
                }
            }
        }
    }

    // Older workbenches may try files outside the context of a library
    {
        std::lock_guard<std::mutex> locker(_mutex);

        if (MaterialConfigLoader::isConfigStyle(path)) {
            auto material = MaterialConfigLoader::getMaterialFromPath(nullptr, path);

            return material;
        }
    }

    throw MaterialNotFound();
}

std::shared_ptr<Material> MaterialManagerLocal::getMaterialByPath(const std::string& path,
                                                                  const std::string& lib) const
{
    auto library = getLibrary(lib);  // May throw LibraryNotFound
    if (library->isLocal()) {
        auto materialLibrary =
            std::dynamic_pointer_cast<Materials::MaterialLibraryLocal>(library);
        if (!materialLibrary) {
            throw LibraryNotFound();
        }
        return materialLibrary->getMaterialByPath(path);  // May throw MaterialNotFound
    }

    throw LibraryNotFound();
}

bool MaterialManagerLocal::exists(const std::string& uuid) const
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

bool MaterialManagerLocal::exists(const MaterialLibrary& library,
                                  const std::string& uuid) const
{
    try {
        auto material = getMaterial(uuid);
        if (material && material->getLibrary()) {
            auto materialLibrary =
                std::dynamic_pointer_cast<Materials::MaterialLibraryLocal>(
                    material->getLibrary());
            if (materialLibrary) {
                return (*materialLibrary == library);
            }
        }
    }
    catch (const MaterialNotFound&) {
    }

    return false;
}

void MaterialManagerLocal::remove(const std::string& uuid)
{
    _materialMap->erase(uuid);
}

void MaterialManagerLocal::saveMaterial(const std::shared_ptr<MaterialLibraryLocal>& library,
                                        const std::shared_ptr<Material>& material,
                                        const std::string& path,
                                        bool overwrite,
                                        bool saveAsCopy,
                                        bool saveInherited) const
{
    if (library->isLocal()) {
        auto newMaterial =
            library->saveMaterial(material, path, overwrite, saveAsCopy, saveInherited);
        (*_materialMap)[newMaterial->getUUID()] = newMaterial;
    }
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
    if (file.suffix() == "FCMat") {
        return true;
    }
    return false;
}

std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>>
MaterialManagerLocal::materialsWithModel(const std::string& uuid) const
{
    std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> dict =
        std::make_shared<std::map<std::string, std::shared_ptr<Material>>>();

    for (auto& it : *_materialMap) {
        const std::string& key = it.first;
        auto material = it.second;

        if (material->hasModel(uuid)) {
            (*dict)[key] = material;
        }
    }

    return dict;
}

std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>>
MaterialManagerLocal::materialsWithModelComplete(const std::string& uuid) const
{
    std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> dict =
        std::make_shared<std::map<std::string, std::shared_ptr<Material>>>();

    for (auto& it : *_materialMap) {
        const std::string& key = it.first;
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

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>
MaterialManagerLocal::getConfiguredLibraries()
{
    auto libraryList = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();

    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources");
    bool useBuiltInMaterials = param->GetBool("UseBuiltInMaterials", true);
    bool useMatFromModules = param->GetBool("UseMaterialsFromWorkbenches", true);
    bool useMatFromConfigDir = param->GetBool("UseMaterialsFromConfigDir", true);
    bool useMatFromCustomDir = param->GetBool("UseMaterialsFromCustomDir", true);

    if (useBuiltInMaterials) {
        const std::string resourceDir =
            App::Application::getResourceDir() + "/Mod/Material/Resources/Materials";
        auto libData =
            std::make_shared<MaterialLibraryLocal>("System",
                                                   resourceDir,
                                                   ":/icons/freecad.svg",
                                                   true);
        libraryList->push_back(libData);
    }

    if (useMatFromModules) {
        auto moduleParam = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules");
        for (auto& group : moduleParam->GetGroups()) {
            // auto module = moduleParam->GetGroup(group->GetGroupName());
            auto moduleName = group->GetGroupName();
            auto materialDir = group->GetASCII("ModuleDir", "");
            auto materialIcon = group->GetASCII("ModuleIcon", "");
            auto materialReadOnly = group->GetBool("ModuleReadOnly", true);

            if (!materialDir.empty()) {
                if (QDir(QString::fromStdString(materialDir)).exists()) {
                    auto libData = std::make_shared<MaterialLibraryLocal>(moduleName,
                                                                          materialDir,
                                                                          materialIcon,
                                                                          materialReadOnly);
                    libraryList->push_back(libData);
                }
            }
        }
    }

    if (useMatFromConfigDir) {
        const std::string resourceDir = App::Application::getUserAppDataDir() + "/Material";
        if (!resourceDir.empty()) {
            QDir materialDir(QString::fromStdString(resourceDir));
            if (!materialDir.exists()) {
                // Try creating the user dir if it doesn't exist
                if (!materialDir.mkpath(QString::fromStdString(resourceDir))) {
                    Base::Console().log("Unable to create user library '{}'\n", resourceDir);
                }
            }
            if (materialDir.exists()) {
                auto libData = std::make_shared<MaterialLibraryLocal>(
                    "User",
                    resourceDir,
                    ":/icons/preferences-general.svg",
                    false);
                libraryList->push_back(libData);
            }
        }
    }

    if (useMatFromCustomDir) {
        const std::string resourceDir = param->GetASCII("CustomMaterialsDir", "");
        if (!resourceDir.empty()) {
            if (QDir(QString::fromStdString(resourceDir)).exists()) {
                auto libData = std::make_shared<MaterialLibraryLocal>(
                    "Custom",
                    resourceDir,
                    ":/icons/user.svg",
                    false);
                libraryList->push_back(libData);
            }
        }
    }

    return libraryList;
}

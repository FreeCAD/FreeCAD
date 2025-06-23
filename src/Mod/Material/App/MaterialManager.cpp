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
#include "MaterialManager.h"
#if defined(BUILD_MATERIAL_EXTERNAL)
#include "MaterialManagerExternal.h"
#endif
#include "MaterialManagerLocal.h"
#include "ModelManager.h"
#include "ModelUuids.h"

#include <Base/Tools.h>


using namespace Materials;

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::MaterialManager, Base::BaseClass)

QMutex MaterialManager::_mutex;
bool MaterialManager::_useExternal = false;
MaterialManager* MaterialManager::_manager = nullptr;
std::unique_ptr<MaterialManagerLocal> MaterialManager::_localManager;
#if defined(BUILD_MATERIAL_EXTERNAL)
std::unique_ptr<MaterialManagerExternal> MaterialManager::_externalManager;
#endif

MaterialManager::MaterialManager()
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    _hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface");
    _useExternal = _hGrp->GetBool("UseExternal", false);
    _hGrp->Attach(this);
#else
    _useExternal = false;
#endif
}

MaterialManager::~MaterialManager()
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    _hGrp->Detach(this);
#endif
}

MaterialManager& MaterialManager::getManager()
{
    if (!_manager) {
        initManagers();
    }
    return *_manager;
}

void MaterialManager::initManagers()
{
    QMutexLocker locker(&_mutex);

    if (!_manager) {
        // Can't use smart pointers for this since the constructor is private
        _manager = new MaterialManager();
    }
    if (!_localManager) {
        _localManager = std::make_unique<MaterialManagerLocal>();
    }

#if defined(BUILD_MATERIAL_EXTERNAL)
    if (!_externalManager) {
        _externalManager = std::make_unique<MaterialManagerExternal>();
    }
#endif
}

void MaterialManager::OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason)
{
    const ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);
    if (strcmp(Reason, "UseExternal") == 0) {
        Base::Console().log("Use external changed\n");
        _useExternal = rGrp.GetBool("UseExternal", false);
        // _dbManager->refresh();
    }
}

void MaterialManager::cleanup()
{
    if (_localManager) {
        _localManager->cleanup();
    }
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_externalManager) {
        _externalManager->cleanup();
    }
#endif
}

void MaterialManager::refresh()
{
    _localManager->refresh();
}

//=====
//
// Defaults
//
//=====

std::shared_ptr<App::Material> MaterialManager::defaultAppearance()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    auto getColor = [hGrp](const char* parameter, Base::Color& color) {
        uint32_t packed = color.getPackedRGB();
        packed = hGrp->GetUnsigned(parameter, packed);
        color.setPackedRGB(packed);
        color.a = 1.0;  // The default color sets fully transparent, not opaque
    };
    auto intRandom = [](int min, int max) -> int {
        static std::mt19937 generator;
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(generator);
    };

    App::Material mat(App::Material::DEFAULT);
    bool randomColor = hGrp->GetBool("RandomColor", false);

    if (randomColor) {
        float red = static_cast<float>(intRandom(0, 255)) / 255.0F;
        float green = static_cast<float>(intRandom(0, 255)) / 255.0F;
        float blue = static_cast<float>(intRandom(0, 255)) / 255.0F;
        mat.diffuseColor = Base::Color(red, green, blue, 1.0);
    }
    else {
        getColor("DefaultShapeColor", mat.diffuseColor);
    }

    getColor("DefaultAmbientColor", mat.ambientColor);
    getColor("DefaultEmissiveColor", mat.emissiveColor);
    getColor("DefaultSpecularColor", mat.specularColor);

    long initialTransparency = hGrp->GetInt("DefaultShapeTransparency", 0);
    long initialShininess = hGrp->GetInt("DefaultShapeShininess", 90);
    mat.shininess = Base::fromPercent(initialShininess);
    mat.transparency = Base::fromPercent(initialTransparency);

    return std::make_shared<App::Material>(mat);
}

std::shared_ptr<Material> MaterialManager::defaultMaterial()
{
    MaterialManager manager;

    auto mat = defaultAppearance();
    auto material = getManager().getMaterial(defaultMaterialUUID());
    if (!material) {
        material = getManager().getMaterial(QStringLiteral("7f9fd73b-50c9-41d8-b7b2-575a030c1eeb"));
    }
    if (material->hasAppearanceModel(ModelUUIDs::ModelUUID_Rendering_Basic)) {
        material->getAppearanceProperty(QStringLiteral("DiffuseColor"))
            ->setColor(mat->diffuseColor);
        material->getAppearanceProperty(QStringLiteral("AmbientColor"))
            ->setColor(mat->ambientColor);
        material->getAppearanceProperty(QStringLiteral("EmissiveColor"))
            ->setColor(mat->emissiveColor);
        material->getAppearanceProperty(QStringLiteral("SpecularColor"))
            ->setColor(mat->specularColor);
        material->getAppearanceProperty(QStringLiteral("Transparency"))
            ->setFloat(mat->transparency);
        material->getAppearanceProperty(QStringLiteral("Shininess"))
            ->setFloat(mat->shininess);
    }

    return material;
}

QString MaterialManager::defaultMaterialUUID()
{
    // Make this a preference
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material");
    auto uuid = param->GetASCII("DefaultMaterial", "7f9fd73b-50c9-41d8-b7b2-575a030c1eeb");
    return QString::fromStdString(uuid);
}

//=====
//
// Library management
//
//=====

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialManager::getLibraries()
{
    // External libraries take precedence over local libraries
    auto libMap = std::map<QString, std::shared_ptr<MaterialLibrary>>();
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
    auto libraries = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();
    for (auto libEntry : libMap) {
        libraries->push_back(libEntry.second);
    }

    return libraries;
}

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>
MaterialManager::getLocalLibraries()
{
    return _localManager->getLibraries();
}

std::shared_ptr<MaterialLibrary> MaterialManager::getLibrary(const QString& name) const
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal) {
        try
        {
            auto lib = _externalManager->getLibrary(name);
            if (lib) {
                return lib;
            }
        }
        catch (const LibraryNotFound& e) {
        }
    }
#endif
    // We really want to return the local library if not found, such as for User folder models
    return _localManager->getLibrary(name);
}

void MaterialManager::createLibrary([[maybe_unused]] const QString& libraryName,
                                    [[maybe_unused]] const QString& iconPath,
                                    [[maybe_unused]] bool readOnly)
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal) {
        auto icon = Materials::Library::getIcon(iconPath);
        _externalManager->createLibrary(libraryName, icon, readOnly);
        return;
    }
#endif
    throw CreationError("Local library requires a path");
}

void MaterialManager::createLocalLibrary(const QString& libraryName,
                                         const QString& directory,
                                         const QString& iconPath,
                                         bool readOnly)
{
    _localManager->createLibrary(libraryName, directory, iconPath, readOnly);
}

void MaterialManager::renameLibrary(const QString& libraryName, const QString& newName)
{
    auto library = getLibrary(libraryName);
    if (library) {
#if defined(BUILD_MATERIAL_EXTERNAL)
        if (!library->isLocal()) {
            if (_useExternal) {
                _externalManager->renameLibrary(libraryName, newName);
                return;
            }

            throw Materials::RenameError();
        }
#endif
        _localManager->renameLibrary(libraryName, newName);
    }
}

void MaterialManager::changeIcon(const QString& libraryName, const QString& iconPath)
{
    auto icon = Materials::Library::getIcon(iconPath);
    _localManager->changeIcon(libraryName, icon);
}

void MaterialManager::removeLibrary(const QString& libraryName)
{
    _localManager->removeLibrary(libraryName);
}

std::shared_ptr<std::vector<LibraryObject>>
MaterialManager::libraryMaterials(const QString& libraryName, [[maybe_unused]] bool local)
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal && !local) {
        try {
            auto materials = _externalManager->libraryMaterials(libraryName);
            if (materials) {
                return materials;
            }
        }
        catch (const LibraryNotFound& e) {
        }
    }
#endif
    return _localManager->libraryMaterials(libraryName);
}

std::shared_ptr<std::vector<LibraryObject>>
MaterialManager::libraryMaterials(const QString& libraryName,
                                  const MaterialFilter& filter,
                                  const MaterialFilterOptions& options,
                                  [[maybe_unused]] bool local)
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal && !local) {
        try {
            auto materials = _externalManager->libraryMaterials(libraryName, filter, options);
            if (materials) {
                return materials;
            }
        }
        catch (const LibraryNotFound& e) {
        }
    }
#endif
    return _localManager->libraryMaterials(libraryName, filter, options);
}

#if defined(BUILD_MATERIAL_EXTERNAL)
bool MaterialManager::isLocalLibrary(const QString& libraryName)
{
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
    return true;
}
#else
bool MaterialManager::isLocalLibrary(const QString& /*libraryName*/)
{
    return true;
}
#endif

//=====
//
// Folder management
//
//=====

std::shared_ptr<std::list<QString>>
MaterialManager::getMaterialFolders(const std::shared_ptr<MaterialLibrary>& library) const
{
    if (library->isLocal()) {
        auto materialLibrary =
            reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);

        return _localManager->getMaterialFolders(materialLibrary);
    }

    return std::make_shared<std::list<QString>>();
}

void MaterialManager::createFolder(const std::shared_ptr<MaterialLibrary>& library,
                                   const QString& path)
{
    if (library->isLocal()) {
        auto materialLibrary =
            reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);

        _localManager->createFolder(materialLibrary, path);
    }
#if defined(BUILD_MATERIAL_EXTERNAL)
    else if (_useExternal) {
            _externalManager->createFolder(*library, path);
    }
    else {
        throw Materials::CreationError("External materials are not enabled");
    }
#endif
}

void MaterialManager::renameFolder(const std::shared_ptr<MaterialLibrary>& library,
                                   const QString& oldPath,
                                   const QString& newPath)
{
    if (library->isLocal()) {
        auto materialLibrary =
            reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);

        _localManager->renameFolder(materialLibrary, oldPath, newPath);
    }
#if defined(BUILD_MATERIAL_EXTERNAL)
    else if (_useExternal) {
        _externalManager->renameFolder(*library, oldPath, newPath);
    }
    else {
        throw Materials::RenameError("External materials are not enabled");
    }
#endif
}

void MaterialManager::deleteRecursive(const std::shared_ptr<MaterialLibrary>& library,
                                      const QString& path)
{
    if (library->isLocal()) {
        auto materialLibrary =
            reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);

        _localManager->deleteRecursive(materialLibrary, path);
    }
#if defined(BUILD_MATERIAL_EXTERNAL)
    else if (_useExternal) {
        _externalManager->deleteRecursive(*library, path);
    }
    else {
        throw Materials::DeleteError("External materials are not enabled");
    }
#endif
}

//=====
//
// Tree management
//
//=====

std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
MaterialManager::getMaterialTree(const MaterialLibrary& library,
                                 const Materials::MaterialFilter& filter) const
{
    MaterialFilterOptions options;
    return library.getMaterialTree(filter, options);
}

std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
MaterialManager::getMaterialTree(const MaterialLibrary& library,
                                 const Materials::MaterialFilter& filter,
                                 const MaterialFilterOptions& options) const
{
    return library.getMaterialTree(filter, options);
}

std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
MaterialManager::getMaterialTree(const MaterialLibrary& library) const
{
    Materials::MaterialFilter filter;
    MaterialFilterOptions options;
    return library.getMaterialTree(filter, options);
}

//=====
//
// Material management
//
//=====

std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>
MaterialManager::getLocalMaterials() const
{
    return _localManager->getLocalMaterials();
}

std::shared_ptr<Material> MaterialManager::getMaterial(const QString& uuid) const
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal) {
        auto material = _externalManager->getMaterial(uuid);
        if (material) {
            return material;
        }
    }
#endif
    // We really want to return the local material if not found, such as for User folder models
    return _localManager->getMaterial(uuid);
}

std::shared_ptr<Material> MaterialManager::getMaterial(const App::Material& material)
{
    MaterialManager manager;

    return manager.getMaterial(QString::fromStdString(material.uuid));
}

std::shared_ptr<Material> MaterialManager::getMaterialByPath(const QString& path) const
{
    return _localManager->getMaterialByPath(path);
}

std::shared_ptr<Material> MaterialManager::getMaterialByPath(const QString& path,
                                                             const QString& lib) const
{
    return _localManager->getMaterialByPath(path, lib);
}

std::shared_ptr<Material>
MaterialManager::getParent(const std::shared_ptr<Material>& material) const
{
    if (material->getParentUUID().isEmpty()) {
        throw MaterialNotFound();
    }

    return getMaterial(material->getParentUUID());
}

bool MaterialManager::exists(const QString& uuid) const
{
    return _localManager->exists(uuid);
}

bool MaterialManager::exists(const MaterialLibrary& library,
                             const QString& uuid) const
{
    if (library.isLocal()) {
        return _localManager->exists(library, uuid);
    }
    return false;
}

void MaterialManager::remove(const QString& uuid) const
{
    _localManager->remove(uuid);
}

void MaterialManager::saveMaterial(const std::shared_ptr<MaterialLibrary>& library,
                                   const std::shared_ptr<Material>& material,
                                   const QString& path,
                                   bool overwrite,
                                   bool saveAsCopy,
                                   bool saveInherited) const
{
    auto materialLibrary =
        reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);
    _localManager
        ->saveMaterial(materialLibrary, material, path, overwrite, saveAsCopy, saveInherited);
}

bool MaterialManager::isMaterial(const fs::path& p) const
{
    return _localManager->isMaterial(p);
}

bool MaterialManager::isMaterial(const QFileInfo& file) const
{
    return _localManager->isMaterial(file);
}

std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>
MaterialManager::materialsWithModel(const QString& uuid) const
{
    return _localManager->materialsWithModel(uuid);
}

std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>
MaterialManager::materialsWithModelComplete(const QString& uuid) const
{
    return _localManager->materialsWithModelComplete(uuid);
}

void MaterialManager::dereference() const
{
    _localManager->dereference();
}

void MaterialManager::dereference(std::shared_ptr<Material> material) const
{
    _localManager->dereference(material);
}

#if defined(BUILD_MATERIAL_EXTERNAL)
void MaterialManager::migrateToExternal(const std::shared_ptr<Materials::MaterialLibrary>& library)
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

    auto materials = _localManager->libraryMaterials(library->getName());
    for (auto& it : *materials) {
        auto uuid = it.getUUID();
        auto path = it.getPath();
        auto name = it.getName();
        Base::Console().log("\t('%s', '%s', '%s')\n",
                            uuid.toStdString().c_str(),
                            path.toStdString().c_str(),
                            name.toStdString().c_str());

        auto material = _localManager->getMaterial(uuid);
        if (!material->isOldFormat()) {
            _externalManager->migrateMaterial(library->getName(), path, *material);
        }
    }
}

void MaterialManager::validateMigration(const std::shared_ptr<Materials::MaterialLibrary>& library)
{
    auto materials = _localManager->libraryMaterials(library->getName());
    _externalManager->resetCache();
    for (auto& it : *materials) {
        auto uuid = it.getUUID();
        auto path = it.getPath();
        auto name = it.getName();
        Base::Console().log("\t('%s', '%s', '%s')\n",
                            uuid.toStdString().c_str(),
                            path.toStdString().c_str(),
                            name.toStdString().c_str());

        auto material = _localManager->getMaterial(uuid);
        if (!material->isOldFormat()) {
            auto externalMaterial = _externalManager->getMaterial(uuid);
            material->validate(*externalMaterial);
        }
    }
}

// Cache stats
double MaterialManager::materialHitRate()
{
    initManagers();
    return _externalManager->materialHitRate();
}
#endif

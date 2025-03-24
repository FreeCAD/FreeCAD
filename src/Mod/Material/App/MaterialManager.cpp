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
#include "MaterialManagerLocal.h"
#include "ModelManager.h"
#include "ModelUuids.h"

#include <Base/Tools.h>


using namespace Materials;

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::MaterialManager, Base::BaseClass)

QMutex MaterialManager::_mutex;
MaterialManager* MaterialManager::_manager = nullptr;
std::unique_ptr<MaterialManagerLocal> MaterialManager::_localManager;

MaterialManager::MaterialManager()
{}

MaterialManager::~MaterialManager()
{}

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
}

void MaterialManager::cleanup()
{
    if (_localManager) {
        _localManager->cleanup();
    }
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
    auto libraries = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();
    auto localLibraries = _localManager->getLibraries();
    for (auto& local : *localLibraries) {
        libraries->push_back(local);
    }

    return libraries;
}

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialManager::getLocalLibraries()
{
    return _localManager->getLibraries();
}

std::shared_ptr<MaterialLibrary> MaterialManager::getLibrary(const QString& name) const
{
    return _localManager->getLibrary(name);
}

void MaterialManager::createLibrary(const QString& libraryName, const QString& icon, bool readOnly)
{
    throw CreationError("Local library requires a path");
}

void MaterialManager::createLocalLibrary(const QString& libraryName,
                                         const QString& directory,
                                         const QString& icon,
                                         bool readOnly)
{
    _localManager->createLibrary(libraryName, directory, icon, readOnly);
}

void MaterialManager::renameLibrary(const QString& libraryName, const QString& newName)
{
    _localManager->renameLibrary(libraryName, newName);
}

void MaterialManager::changeIcon(const QString& libraryName, const QString& icon)
{
    _localManager->changeIcon(libraryName, icon);
}

void MaterialManager::removeLibrary(const QString& libraryName)
{
    _localManager->removeLibrary(libraryName);
}

std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
MaterialManager::libraryMaterials(const QString& libraryName)
{
    return _localManager->libraryMaterials(libraryName);
}

std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
MaterialManager::libraryMaterials(const QString& libraryName,
                                  const std::shared_ptr<MaterialFilter>& filter,
                                  const MaterialFilterOptions& options)
{
    return _localManager->libraryMaterials(libraryName, filter, options);
}

bool MaterialManager::isLocalLibrary(const QString& libraryName)
{
    return true;
}

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
}

void MaterialManager::deleteRecursive(const std::shared_ptr<MaterialLibrary>& library,
                                      const QString& path)
{
    if (library->isLocal()) {
        auto materialLibrary =
            reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);

        _localManager->deleteRecursive(materialLibrary, path);
    }
}

//=====
//
// Tree management
//
//=====

std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
MaterialManager::getMaterialTree(const std::shared_ptr<MaterialLibrary>& library,
                                 const std::shared_ptr<Materials::MaterialFilter>& filter) const
{
    MaterialFilterOptions options;
    return library->getMaterialTree(filter, options);
}

std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
MaterialManager::getMaterialTree(const std::shared_ptr<MaterialLibrary>& library,
                                 const std::shared_ptr<Materials::MaterialFilter>& filter,
                                 const MaterialFilterOptions& options) const
{
    return library->getMaterialTree(filter, options);
}

std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
MaterialManager::getMaterialTree(const std::shared_ptr<MaterialLibrary>& library) const
{
    std::shared_ptr<Materials::MaterialFilter> filter;
    MaterialFilterOptions options;
    return library->getMaterialTree(filter, options);
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

bool MaterialManager::exists(const std::shared_ptr<MaterialLibrary>& library,
                             const QString& uuid) const
{
    if (library->isLocal()) {
        auto materialLibrary =
            reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);

        return _localManager->exists(materialLibrary, uuid);
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

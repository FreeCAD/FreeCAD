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

#include <QMutex>
#include <QDirIterator>
#include <QMutexLocker>

#include <App/Application.h>
#include <App/Material.h>

#include "Exceptions.h"
#include "MaterialConfigLoader.h"
#include "MaterialLoader.h"
#include "MaterialManager.h"
#include "ModelManager.h"
#include "ModelUuids.h"


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

void MaterialManager::cleanup()
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

void MaterialManager::refresh()
{
    // This is very expensive and can be improved using observers?
    cleanup();
    initLibraries();
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

std::shared_ptr<App::Material> MaterialManager::defaultAppearance()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    auto getColor = [hGrp](const char* parameter, App::Color& color) {
        uint32_t packed = color.getPackedRGB();
        packed = hGrp->GetUnsigned(parameter, packed);
        color.setPackedRGB(packed);
        color.a = 1.0; // The default color sets fully transparent, not opaque
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
        mat.diffuseColor = App::Color(red, green, blue, 1.0);
    }
    else {
        getColor("DefaultShapeColor", mat.diffuseColor);
    }

    getColor("DefaultAmbientColor", mat.ambientColor);
    getColor("DefaultEmissiveColor", mat.emissiveColor);
    getColor("DefaultSpecularColor", mat.specularColor);

    long initialTransparency = hGrp->GetInt("DefaultShapeTransparency", 0);
    long initialShininess = hGrp->GetInt("DefaultShapeShininess", 90);
    mat.shininess = ((float)initialShininess / 100.0F);
    mat.transparency = ((float)initialTransparency / 100.0F);

    return std::make_shared<App::Material>(mat);
}

std::shared_ptr<Material> MaterialManager::defaultMaterial()
{
    MaterialManager manager;

    auto mat = defaultAppearance();
    auto material = manager.getMaterial(defaultMaterialUUID());
    if (!material) {
        material = manager.getMaterial(QLatin1String("7f9fd73b-50c9-41d8-b7b2-575a030c1eeb"));
    }
    if (material->hasAppearanceModel(ModelUUIDs::ModelUUID_Rendering_Basic)) {
        material->getAppearanceProperty(QLatin1String("DiffuseColor"))
            ->setColor(mat->diffuseColor);
        material->getAppearanceProperty(QLatin1String("AmbientColor"))
            ->setColor(mat->ambientColor);
        material->getAppearanceProperty(QLatin1String("EmissiveColor"))
            ->setColor(mat->emissiveColor);
        material->getAppearanceProperty(QLatin1String("SpecularColor"))
            ->setColor(mat->specularColor);
        material->getAppearanceProperty(QLatin1String("Transparency"))
            ->setFloat(mat->transparency);
        material->getAppearanceProperty(QLatin1String("Shininess"))
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

std::shared_ptr<Material> MaterialManager::getMaterial(const QString& uuid) const
{
    try {
        return _materialMap->at(uuid);
    }
    catch (std::out_of_range&) {
        throw MaterialNotFound();
    }
}

std::shared_ptr<Material> MaterialManager::getMaterial(const App::Material& material)
{
    MaterialManager manager;

    return manager.getMaterial(QString::fromStdString(material.uuid));
}

std::shared_ptr<Material> MaterialManager::getMaterialByPath(const QString& path) const
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

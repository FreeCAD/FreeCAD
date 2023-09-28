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

#include <QMutexLocker>

#include <App/Application.h>

#include "Exceptions.h"
#include "MaterialLoader.h"
#include "MaterialManager.h"
#include "ModelManager.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

std::shared_ptr<std::list<MaterialLibrary*>> MaterialManager::_libraryList = nullptr;
std::shared_ptr<std::map<QString, Material*>> MaterialManager::_materialMap = nullptr;
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

        _materialMap = std::make_shared<std::map<QString, Material*>>();

        if (_libraryList == nullptr) {
            _libraryList = std::make_shared<std::list<MaterialLibrary*>>();
        }

        // Load the libraries
        MaterialLoader loader(_materialMap, _libraryList);
    }
}

void MaterialManager::saveMaterial(MaterialLibrary* library,
                                   Material& material,
                                   const QString& path,
                                   bool saveAsCopy)
{
    Material* newMaterial = library->saveMaterial(material, path, saveAsCopy);

    try {
        Material* old = _materialMap->at(newMaterial->getUUID());
        if (old) {
            delete old;
        }
    }
    catch (const std::out_of_range&) {
    }

    (*_materialMap)[material.getUUID()] = newMaterial;
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

const Material& MaterialManager::getMaterial(const QString& uuid) const
{
    try {
        return *(_materialMap->at(uuid));
    }
    catch (std::out_of_range& e) {
        throw MaterialNotFound();
    }
}

const Material& MaterialManager::getMaterialByPath(const QString& path) const
{
    QString cleanPath = QDir::cleanPath(path);

    for (auto library : *_libraryList) {
        Base::Console().Log("MaterialManager::getMaterialByPath() Checking library '%s'->'%s'\n",
                            library->getName().toStdString().c_str(),
                            library->getDirectory().toStdString().c_str());


        if (cleanPath.startsWith(library->getDirectory())) {
            Base::Console().Log("MaterialManager::getMaterialByPath() Library '%s'\n",
                                library->getDirectory().toStdString().c_str());
            Base::Console().Log("MaterialManager::getMaterialByPath() Path '%s'\n",
                                cleanPath.toStdString().c_str());
            return library->getMaterialByPath(cleanPath);
        }
    }
    Base::Console().Log("MaterialManager::getMaterialByPath() Library not found for path '%s'\n",
                        cleanPath.toStdString().c_str());

    throw MaterialNotFound();
}

const Material& MaterialManager::getMaterialByPath(const QString& path, const QString& lib) const
{
    auto library = getLibrary(lib);           // May throw LibraryNotFound
    return library->getMaterialByPath(path);  // May throw MaterialNotFound
}

MaterialLibrary* MaterialManager::getLibrary(const QString& name) const
{
    for (auto library : *_libraryList) {
        if (library->getName() == name) {
            return library;
        }
    }

    throw LibraryNotFound();
}

std::shared_ptr<std::list<MaterialLibrary*>> MaterialManager::getMaterialLibraries()
{
    if (_libraryList == nullptr) {
        if (_materialMap == nullptr) {
            _materialMap = std::make_shared<std::map<QString, Material*>>();
        }
        _libraryList = std::make_shared<std::list<MaterialLibrary*>>();

        // Load the libraries
        MaterialLoader loader(_materialMap, _libraryList);
    }
    return _libraryList;
}

std::shared_ptr<std::map<QString, MaterialTreeNode*>>
MaterialManager::getMaterialTree(const MaterialLibrary& library) const
{
    std::shared_ptr<std::map<QString, MaterialTreeNode*>> materialTree =
        std::make_shared<std::map<QString, MaterialTreeNode*>>();

    for (auto it = _materialMap->begin(); it != _materialMap->end(); it++) {
        auto filename = it->first;
        auto material = it->second;

        if (material->getLibrary() == library) {
            fs::path path = material->getDirectory().toStdString();

            // Start at the root
            auto node = materialTree;
            for (auto itp = path.begin(); itp != path.end(); itp++) {
                if (QString::fromStdString(itp->string())
                        .endsWith(QString::fromStdString(".FCMat"))) {
                    MaterialTreeNode* child = new MaterialTreeNode();
                    child->setData(material);
                    (*node)[QString::fromStdString(itp->string())] = child;
                }
                else {
                    // Add the folder only if it's not already there
                    QString folderName = QString::fromStdString(itp->string());
                    std::shared_ptr<std::map<QString, MaterialTreeNode*>> mapPtr;
                    if (node->count(folderName) == 0) {
                        mapPtr = std::make_shared<std::map<QString, MaterialTreeNode*>>();
                        MaterialTreeNode* child = new MaterialTreeNode();
                        child->setFolder(mapPtr);
                        (*node)[folderName] = child;
                        node = mapPtr;
                    }
                    else {
                        node = (*node)[folderName]->getFolder();
                    }
                }
            }
        }
    }

    auto folderList = getMaterialFolders(library);
    for (auto folder : *folderList) {
        fs::path path = folder.toStdString();

        // Start at the root
        auto node = materialTree;
        for (auto itp = path.begin(); itp != path.end(); itp++) {
            // Add the folder only if it's not already there
            QString folderName = QString::fromStdString(itp->string());
            if (node->count(folderName) == 0) {
                std::shared_ptr<std::map<QString, MaterialTreeNode*>> mapPtr =
                    std::make_shared<std::map<QString, MaterialTreeNode*>>();
                MaterialTreeNode* child = new MaterialTreeNode();
                child->setFolder(mapPtr);
                (*node)[folderName] = child;
                node = mapPtr;
            }
            else {
                node = (*node)[folderName]->getFolder();
            }
        }
    }

    return materialTree;
}

std::shared_ptr<std::list<QString>>
MaterialManager::getMaterialFolders(const MaterialLibrary& library) const
{
    return MaterialLoader::getMaterialFolders(library);
}

std::shared_ptr<std::map<QString, Material*>> MaterialManager::materialsWithModel(QString uuid)
{
    std::shared_ptr<std::map<QString, Material*>> dict =
        std::make_shared<std::map<QString, Material*>>();

    for (auto it = _materialMap->begin(); it != _materialMap->end(); it++) {
        QString key = it->first;
        Material* material = it->second;

        if (material->hasModel(uuid)) {
            (*dict)[key] = material;
        }
    }

    return dict;
}

std::shared_ptr<std::map<QString, Material*>>
MaterialManager::materialsWithModelComplete(QString uuid)
{
    std::shared_ptr<std::map<QString, Material*>> dict =
        std::make_shared<std::map<QString, Material*>>();

    for (auto it = _materialMap->begin(); it != _materialMap->end(); it++) {
        QString key = it->first;
        Material* material = it->second;

        if (material->isModelComplete(uuid)) {
            (*dict)[key] = material;
        }
    }

    return dict;
}

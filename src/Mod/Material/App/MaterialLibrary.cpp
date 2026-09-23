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

#include <QDirIterator>
#include <QFileInfo>
#include <QVector>



#include <App/Application.h>

#include "MaterialFilter.h"
#include "MaterialLibrary.h"
#include "MaterialLoader.h"
#include "MaterialManager.h"
#include "Materials.h"
#include "ModelManager.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::MaterialLibrary, Base::BaseClass)

MaterialLibrary::MaterialLibrary(const std::string& libraryName, const std::string& icon, bool readOnly)
    : Library(libraryName, icon, readOnly)
{}

MaterialLibrary::MaterialLibrary(const std::string& libraryName,
                                 const std::string& dir,
                                 const std::string& icon,
                                 bool readOnly)
    : Library(libraryName, dir, icon, readOnly)
{}

MaterialLibrary::MaterialLibrary(const Library& library)
    : Library(library)
{}

std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>>
MaterialLibrary::getMaterialTree(const Materials::MaterialFilter& filter,
                                 const Materials::MaterialFilterOptions& options) const
{
    std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>> materialTree =
        std::make_shared<std::map<std::string, std::shared_ptr<MaterialTreeNode>>>();

    auto materials = MaterialManager::getManager().libraryMaterials(getName(), filter, options, isLocal());
    for (auto& it : *materials) {
        auto uuid = it.getUUID();
        auto path = it.getPath();
        auto filename = it.getName();

        std::vector<std::string> list = split(path, '/');

        // Start at the root
        std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>> node =
            materialTree;
        for (auto& itp : list) {
            if (!itp.empty()) {
                // Add the folder only if it's not already there
                if (!node->contains(itp)) {
                    auto mapPtr = std::make_shared<
                        std::map<std::string, std::shared_ptr<MaterialTreeNode>>>();
                    std::shared_ptr<MaterialTreeNode> child =
                        std::make_shared<MaterialTreeNode>();
                    child->setFolder(mapPtr);
                    child->setReadOnly(isReadOnly());
                    (*node)[itp] = child;
                    node = mapPtr;
                }
                else {
                    node = (*node)[itp]->getFolder();
                }
            }
        }
        std::shared_ptr<MaterialTreeNode> child = std::make_shared<MaterialTreeNode>();
        child->setUUID(uuid);
        child->setReadOnly(isReadOnly());
        if (isLocal()) {
            auto material = MaterialManager::getManager().getMaterial(uuid);
            child->setOldFormat(material->isOldFormat());
        }
        (*node)[filename] = child;
    }

    // // Empty folders aren't included in _materialPathMap, so we add them by looking at the file
    // // system
    // if (!filter || options.includeEmptyFolders()) {
    //     if (isLocal()) {
    //         auto& materialLibrary =
    //             *(reinterpret_cast<const Materials::MaterialLibraryLocal*>(this));
    //         auto folderList = MaterialLoader::getMaterialFolders(materialLibrary);
    //         for (auto& folder : *folderList) {
    //             std::vector<std::string> list = folder.split("/");

    //             // Start at the root
    //             auto node = materialTree;
    //             for (auto& itp : list) {
    //                 // Add the folder only if it's not already there
    //                 if (!node->contains(itp)) {
    //                     std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>>
    //                         mapPtr = std::make_shared<
    //                             std::map<std::string, std::shared_ptr<MaterialTreeNode>>>();
    //                     std::shared_ptr<MaterialTreeNode> child =
    //                         std::make_shared<MaterialTreeNode>();
    //                     child->setFolder(mapPtr);
    //                     (*node)[itp] = child;
    //                     node = mapPtr;
    //                 }
    //                 else {
    //                     node = (*node)[itp]->getFolder();
    //                 }
    //             }
    //         }
    //     }
    // }

    return materialTree;
}

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::MaterialLibraryLocal, Materials::MaterialLibrary)

MaterialLibraryLocal::MaterialLibraryLocal(const std::string& libraryName,
                                           const std::string& dir,
                                           const std::string& icon,
                                           bool readOnly)
    : MaterialLibrary(libraryName, dir, icon, readOnly)
    , _materialPathMap(std::make_unique<std::map<std::string, std::shared_ptr<Material>>>())
{
    setLocal(true);
}

void MaterialLibraryLocal::createFolder(const std::string& path)
{
    const QString filePath = QString::fromStdString(getLocalPath(path));

    QDir fileDir(filePath);
    if (!fileDir.exists()) {
        if (!fileDir.mkpath(filePath)) {
            Base::Console().error("Unable to create directory path '{}'\n", filePath.toStdString());
        }
    }
}

void MaterialLibraryLocal::renameFolder(const std::string& oldPath, const std::string& newPath)
{
    const QString filePath = QString::fromStdString(getLocalPath(oldPath));
    const QString newFilePath = QString::fromStdString(getLocalPath(newPath));

    QDir fileDir(filePath);
    if (fileDir.exists()) {
        if (!fileDir.rename(filePath, newFilePath)) {
            Base::Console().error("Unable to rename directory path '{}'\n",
                                  filePath.toStdString());
        }
    }

    updatePaths(oldPath, newPath);
}

void MaterialLibraryLocal::deleteRecursive(const std::string& path)
{
    if (isRoot(path)) {
        return;
    }

    const std::string filePath = getLocalPath(path);
    auto& manager = MaterialManager::getManager();

    QFileInfo info(QString::fromStdString(filePath));
    if (info.isDir()) {
        deleteDir(manager, filePath);
    }
    else {
        deleteFile(manager, filePath);
    }
}

// This accepts the filesystem path as returned from getLocalPath
void MaterialLibraryLocal::deleteDir(MaterialManager& manager, const std::string& path)
{
    // Remove the children first
    QDirIterator it(QString::fromStdString(path), QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    // Add paths to a list so there are no iterator errors
    std::vector<std::string> dirList;
    std::vector<std::string> fileList;
    while (it.hasNext()) {
        const auto pathname = it.next();
        QFileInfo file(pathname);
        if (file.isFile()) {
            fileList.push_back(pathname.toStdString());
        }
        else if (file.isDir()) {
            dirList.push_back(pathname.toStdString());
        }
    }

    // Remove the subdirs first
    for (const auto& dirPath : dirList) {
        deleteDir(manager, dirPath);
    }

    // Remove the files
    for (const auto& filePath : fileList) {
        deleteFile(manager, filePath);
    }

    // Finally, remove ourself
    QDir dir;
    if (!dir.rmdir(QString::fromStdString(path))) {
        throw DeleteError(path);
    }
}

// This accepts the filesystem path as returned from getLocalPath
void MaterialLibraryLocal::deleteFile(MaterialManager& manager, const std::string& path)
{
    if (QFile::remove(QString::fromStdString(path))) {
        // Remove from the map
        const std::string rPath = getRelativePath(path);
        try {
            auto material = getMaterialByPath(rPath);
            manager.remove(material->getUUID());
        }
        catch (const MaterialNotFound&) {
            Base::Console().log("Unable to remove file from materials list\n");
        }
        _materialPathMap->erase(rPath);
    }
    else {
        throw DeleteError("DeleteError: Unable to delete " + path);
    }
}

void MaterialLibraryLocal::updatePaths(const std::string& oldPath, const std::string& newPath)
{
    // Update the path map
    const std::string op = getRelativePath(oldPath);
    const std::string np = getRelativePath(newPath);
    std::unique_ptr<std::map<std::string, std::shared_ptr<Material>>> pathMap =
        std::make_unique<std::map<std::string, std::shared_ptr<Material>>>();
    for (auto& itp : *_materialPathMap) {
        std::string path = itp.first;
        if (path.starts_with(op)) {
            path = np + path.substr(op.size());
        }
        itp.second->setDirectory(path);
        (*pathMap)[path] = itp.second;
    }

    _materialPathMap = std::move(pathMap);
}

std::shared_ptr<Material>
MaterialLibraryLocal::saveMaterial(const std::shared_ptr<Material>& material,
                                   const std::string& path,
                                   bool overwrite,
                                   bool saveAsCopy,
                                   bool saveInherited)
{
    QFile file(QString::fromStdString(getLocalPath(path)));

    QFileInfo info(file);
    QDir fileDir(info.path());
    if (!fileDir.exists()) {
        if (!fileDir.mkpath(info.path())) {
            Base::Console().error("Unable to create directory path '{}'\n",
                                  info.path().toStdString());
        }
    }

    if (info.exists()) {
        if (!overwrite) {
            Base::Console().error("File already exists '{}'\n", info.path().toStdString());
            throw MaterialExists();
        }
    }

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setGenerateByteOrderMark(true);

        // Write the contents
        material->setName(
            info.fileName().remove(QLatin1String(".FCMat"), Qt::CaseInsensitive).toStdString());
        material->setLibrary(getptr());
        material->setDirectory(getRelativePath(path));
        material->save(stream, overwrite, saveAsCopy, saveInherited);
    }

    return addMaterial(material, path);
}

bool MaterialLibraryLocal::fileExists(const std::string& path) const
{
    return QFileInfo(QString::fromStdString(getLocalPath(path))).exists();
}

std::shared_ptr<Material>
MaterialLibraryLocal::addMaterial(const std::shared_ptr<Material>& material, const std::string& path)
{
    const std::string filePath = getRelativePath(path);
    const std::string filename =
        QFileInfo(QString::fromStdString(filePath)).fileName().toStdString();
    std::shared_ptr<Material> newMaterial = std::make_shared<Material>(*material);
    newMaterial->setLibrary(getptr());
    newMaterial->setDirectory(getLibraryPath(filePath, filename));
    newMaterial->setFilename(filename);

    (*_materialPathMap)[filePath] = newMaterial;

    return newMaterial;
}

std::shared_ptr<Material> MaterialLibraryLocal::getMaterialByPath(const std::string& path) const
{
    const std::string filePath = getRelativePath(path);

    auto search = _materialPathMap->find(filePath);
    if (search != _materialPathMap->end()) {
        return search->second;
    }

    throw MaterialNotFound();
}

std::string MaterialLibraryLocal::getUUIDFromPath(const std::string& path) const
{
    const std::string filePath = getRelativePath(path);

    auto search = _materialPathMap->find(filePath);
    if (search != _materialPathMap->end()) {
        return search->second->getUUID();
    }

    throw MaterialNotFound();
}

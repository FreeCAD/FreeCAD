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
#include <QDirIterator>
#include <QFileInfo>
#include <QVector>
#endif


#include <App/Application.h>

#include "MaterialFilter.h"
#include "MaterialLibrary.h"
#include "MaterialLoader.h"
#include "MaterialManager.h"
#include "Materials.h"
#include "ModelManager.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::MaterialLibrary, Materials::LibraryBase)

MaterialLibrary::MaterialLibrary(const QString& libraryName,
                                 const QString& dir,
                                 const QString& icon,
                                 bool readOnly)
    : LibraryBase(libraryName, dir, icon)
    , _readOnly(readOnly)
    , _materialPathMap(std::make_unique<std::map<QString, std::shared_ptr<Material>>>())
{}

void MaterialLibrary::createFolder(const QString& path)
{
    QString filePath = getLocalPath(path);

    QDir fileDir(filePath);
    if (!fileDir.exists()) {
        if (!fileDir.mkpath(filePath)) {
            Base::Console().Error("Unable to create directory path '%s'\n",
                                  filePath.toStdString().c_str());
        }
    }
}

// This accepts the filesystem path as returned from getLocalPath
void MaterialLibrary::deleteDir(MaterialManager& manager, const QString& path)
{
    // Remove the children first
    QDirIterator it(path, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    // Add paths to a list so there are no iterator errors
    QVector<QString> dirList;
    QVector<QString> fileList;
    while (it.hasNext()) {
        auto pathname = it.next();
        QFileInfo file(pathname);
        if (file.isFile()) {
            fileList.push_back(pathname);
        }
        else if (file.isDir()) {
            dirList.push_back(pathname);
        }
    }

    // Remove the subdirs first
    while (!dirList.isEmpty()) {
        QString dirPath = dirList.takeFirst();
        deleteDir(manager, dirPath);
    }

    // Remove the files
    while (!fileList.isEmpty()) {
        QString filePath = fileList.takeFirst();
        deleteFile(manager, filePath);
    }

    // Finally, remove ourself
    QDir dir;
    if (!dir.rmdir(path)) {
        throw DeleteError(path);
    }
}

// This accepts the filesystem path as returned from getLocalPath
void MaterialLibrary::deleteFile(MaterialManager& manager, const QString& path)
{
    if (QFile::remove(path)) {
        // Remove from the map
        QString rPath = getRelativePath(path);
        try {
            auto material = getMaterialByPath(rPath);
            manager.remove(material->getUUID());
        }
        catch (const MaterialNotFound&) {
            Base::Console().Log("Unable to remove file from materials list\n");
        }
        _materialPathMap->erase(rPath);
    }
    else {
        QString error = QStringLiteral("DeleteError: Unable to delete ") + path;
        throw DeleteError(error);
    }
}

void MaterialLibrary::deleteRecursive(const QString& path)
{
    if (isRoot(path)) {
        return;
    }

    QString filePath = getLocalPath(path);
    MaterialManager manager;

    QFileInfo info(filePath);
    if (info.isDir()) {
        deleteDir(manager, filePath);
    }
    else {
        deleteFile(manager, filePath);
    }
}

void MaterialLibrary::updatePaths(const QString& oldPath, const QString& newPath)
{
    // Update the path map
    QString op = getRelativePath(oldPath);
    QString np = getRelativePath(newPath);
    std::unique_ptr<std::map<QString, std::shared_ptr<Material>>> pathMap =
        std::make_unique<std::map<QString, std::shared_ptr<Material>>>();
    for (auto& itp : *_materialPathMap) {
        QString path = itp.first;
        if (path.startsWith(op)) {
            path = np + path.remove(0, op.size());
        }
        itp.second->setDirectory(path);
        (*pathMap)[path] = itp.second;
    }

    _materialPathMap = std::move(pathMap);
}

void MaterialLibrary::renameFolder(const QString& oldPath, const QString& newPath)
{
    QString filePath = getLocalPath(oldPath);
    QString newFilePath = getLocalPath(newPath);

    QDir fileDir(filePath);
    if (fileDir.exists()) {
        if (!fileDir.rename(filePath, newFilePath)) {
            Base::Console().Error("Unable to rename directory path '%s'\n",
                                  filePath.toStdString().c_str());
        }
    }

    updatePaths(oldPath, newPath);
}

std::shared_ptr<Material> MaterialLibrary::saveMaterial(const std::shared_ptr<Material>& material,
                                                        const QString& path,
                                                        bool overwrite,
                                                        bool saveAsCopy,
                                                        bool saveInherited)
{
    QString filePath = getLocalPath(path);
    QFile file(filePath);

    QFileInfo info(file);
    QDir fileDir(info.path());
    if (!fileDir.exists()) {
        if (!fileDir.mkpath(info.path())) {
            Base::Console().Error("Unable to create directory path '%s'\n",
                                  info.path().toStdString().c_str());
        }
    }

    if (info.exists()) {
        if (!overwrite) {
            Base::Console().Error("File already exists '%s'\n", info.path().toStdString().c_str());
            throw MaterialExists();
        }
    }

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        stream.setCodec("UTF-8");
#endif
        stream.setGenerateByteOrderMark(true);

        // Write the contents
        material->setName(info.baseName());
        material->setLibrary(getptr());
        material->setDirectory(getRelativePath(path));
        material->save(stream, overwrite, saveAsCopy, saveInherited);
    }

    return addMaterial(material, path);
}

bool MaterialLibrary::fileExists(const QString& path) const
{
    QString filePath = getLocalPath(path);
    QFileInfo info(filePath);

    return info.exists();
}

std::shared_ptr<Material> MaterialLibrary::addMaterial(const std::shared_ptr<Material>& material,
                                                       const QString& path)
{
    QString filePath = getRelativePath(path);
    std::shared_ptr<Material> newMaterial = std::make_shared<Material>(*material);
    newMaterial->setLibrary(getptr());
    newMaterial->setDirectory(filePath);

    (*_materialPathMap)[filePath] = newMaterial;

    return newMaterial;
}

std::shared_ptr<Material> MaterialLibrary::getMaterialByPath(const QString& path) const
{
    QString filePath = getRelativePath(path);
    try {
        auto material = _materialPathMap->at(filePath);
        return material;
    }
    catch (std::out_of_range&) {
        throw MaterialNotFound();
    }
}

QString MaterialLibrary::getUUIDFromPath(const QString& path) const
{
    QString filePath = getRelativePath(path);
    try {
        auto material = _materialPathMap->at(filePath);
        return material->getUUID();
    }
    catch (std::out_of_range&) {
        throw MaterialNotFound();
    }
}

bool MaterialLibrary::materialInTree(const std::shared_ptr<Material>& material,
                                     const std::shared_ptr<Materials::MaterialFilter>& filter,
                                     const Materials::MaterialFilterOptions& options) const
{
    if (!filter) {
        // If there's no filter we always include
        return true;
    }

    // filter out old format files
    if (material->isOldFormat() && !options.includeLegacy()) {
        return false;
    }

    // filter based on models
    return filter->modelIncluded(material);
}

std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
MaterialLibrary::getMaterialTree(const std::shared_ptr<Materials::MaterialFilter>& filter,
                                 const Materials::MaterialFilterOptions& options) const
{
    std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>> materialTree =
        std::make_shared<std::map<QString, std::shared_ptr<MaterialTreeNode>>>();

    for (auto& it : *_materialPathMap) {
        auto filename = it.first;
        auto material = it.second;

        if (materialInTree(material, filter, options)) {
            QStringList list = filename.split(QStringLiteral("/"));

            // Start at the root
            std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>> node =
                materialTree;
            for (auto& itp : list) {
                if (itp.endsWith(QStringLiteral(".FCMat"))) {
                    std::shared_ptr<MaterialTreeNode> child = std::make_shared<MaterialTreeNode>();
                    child->setData(material);
                    (*node)[itp] = child;
                }
                else {
                    // Add the folder only if it's not already there
                    if (node->count(itp) == 0) {
                        auto mapPtr = std::make_shared<
                            std::map<QString, std::shared_ptr<MaterialTreeNode>>>();
                        std::shared_ptr<MaterialTreeNode> child =
                            std::make_shared<MaterialTreeNode>();
                        child->setFolder(mapPtr);
                        (*node)[itp] = child;
                        node = mapPtr;
                    }
                    else {
                        node = (*node)[itp]->getFolder();
                    }
                }
            }
        }
    }

    // Empty folders aren't included in _materialPathMap, so we add them by looking at the file
    // system
    if (!filter || options.includeEmptyFolders()) {
        auto folderList = MaterialLoader::getMaterialFolders(*this);
        for (auto& folder : *folderList) {
            QStringList list = folder.split(QStringLiteral("/"));

            // Start at the root
            auto node = materialTree;
            for (auto& itp : list) {
                // Add the folder only if it's not already there
                if (node->count(itp) == 0) {
                    std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>> mapPtr =
                        std::make_shared<std::map<QString, std::shared_ptr<MaterialTreeNode>>>();
                    std::shared_ptr<MaterialTreeNode> child = std::make_shared<MaterialTreeNode>();
                    child->setFolder(mapPtr);
                    (*node)[itp] = child;
                    node = mapPtr;
                }
                else {
                    node = (*node)[itp]->getFolder();
                }
            }
        }
    }

    return materialTree;
}

TYPESYSTEM_SOURCE(Materials::MaterialExternalLibrary, Materials::MaterialLibrary)

MaterialExternalLibrary::MaterialExternalLibrary(const QString& libraryName,
                                                 const QString& dir,
                                                 const QString& icon,
                                                 bool readOnly)
    : MaterialLibrary(libraryName, dir, icon, readOnly)
{}

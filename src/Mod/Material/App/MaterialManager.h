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

#ifndef MATERIAL_MATERIALMANAGER_H
#define MATERIAL_MATERIALMANAGER_H

#include <memory>

#include <QMutex>

#include <boost/filesystem.hpp>

#include <Mod/Material/MaterialGlobal.h>

#include "FolderTree.h"
#include "Materials.h"

#include "MaterialLibrary.h"

namespace fs = boost::filesystem;

namespace Materials
{

class MaterialsExport MaterialManager: public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    MaterialManager();
    ~MaterialManager() override = default;

    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> getMaterials()
    {
        return _materialMap;
    }
    std::shared_ptr<Material> getMaterial(const QString& uuid) const;
    std::shared_ptr<Material> getMaterialByPath(const QString& path) const;
    std::shared_ptr<Material> getMaterialByPath(const QString& path, const QString& library) const;
    std::shared_ptr<Material> getParent(std::shared_ptr<Material> material);
    std::shared_ptr<MaterialLibrary> getLibrary(const QString& name) const;
    bool exists(const QString& uuid) const;
    bool exists(std::shared_ptr<MaterialLibrary> library, const QString& uuid) const;

    // Library management
    static std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> getMaterialLibraries();
    std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
    getMaterialTree(std::shared_ptr<MaterialLibrary> library) const
    {
        return library->getMaterialTree();
    }
    std::shared_ptr<std::list<QString>>
    getMaterialFolders(std::shared_ptr<MaterialLibrary> library) const;
    void createFolder(std::shared_ptr<MaterialLibrary> library, const QString& path)
    {
        library->createFolder(path);
    }
    void renameFolder(std::shared_ptr<MaterialLibrary> library,
                      const QString& oldPath,
                      const QString& newPath)
    {
        library->renameFolder(oldPath, newPath);
    }
    void deleteRecursive(std::shared_ptr<MaterialLibrary> library, const QString& path)
    {
        library->deleteRecursive(path);
    }
    void remove(const QString& uuid)
    {
        _materialMap->erase(uuid);
    }

    void saveMaterial(std::shared_ptr<MaterialLibrary> library,
                      std::shared_ptr<Material> material,
                      const QString& path,
                      bool overwrite,
                      bool saveAsCopy,
                      bool saveInherited);

    static bool isMaterial(const fs::path& p);
    static bool isMaterial(const QFileInfo& file);

    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> materialsWithModel(QString uuid);
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>
    materialsWithModelComplete(QString uuid);
    void dereference(std::shared_ptr<Material> material);

private:
    static std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> _libraryList;
    static std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> _materialMap;
    static QMutex _mutex;

    static void initLibraries();
};

}  // namespace Materials

#endif  // MATERIAL_MATERIALMANAGER_H

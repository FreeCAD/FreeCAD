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

#include <filesystem>

#include <Mod/Material/MaterialGlobal.h>

#include "FolderTree.h"
#include "Materials.h"

#include "MaterialLibrary.h"
#include "MaterialFilter.h"

namespace fs = std::filesystem;

class QMutex;

namespace App
{
class Material;
}

namespace Materials
{

class MaterialsExport MaterialManager: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialManager();
    ~MaterialManager() override = default;

    static void cleanup();
    static void refresh();
    static std::shared_ptr<App::Material> defaultAppearance();
    static std::shared_ptr<Material> defaultMaterial();
    static QString defaultMaterialUUID();

    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> getMaterials() const
    {
        return _materialMap;
    }
    std::shared_ptr<Material> getMaterial(const QString& uuid) const;
    static std::shared_ptr<Material> getMaterial(const App::Material& material);
    std::shared_ptr<Material> getMaterialByPath(const QString& path) const;
    std::shared_ptr<Material> getMaterialByPath(const QString& path, const QString& library) const;
    std::shared_ptr<Material> getParent(const std::shared_ptr<Material>& material) const;
    std::shared_ptr<MaterialLibrary> getLibrary(const QString& name) const;
    bool exists(const QString& uuid) const;
    bool exists(const std::shared_ptr<MaterialLibrary>& library, const QString& uuid) const;

    // Library management
    std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> getMaterialLibraries() const;
    std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
    getMaterialTree(const std::shared_ptr<MaterialLibrary>& library,
                    const std::shared_ptr<Materials::MaterialFilter>& filter) const
    {
        MaterialFilterOptions options;
        return library->getMaterialTree(filter, options);
    }
    std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
    getMaterialTree(const std::shared_ptr<MaterialLibrary>& library,
                    const std::shared_ptr<Materials::MaterialFilter>& filter,
                    const MaterialFilterOptions& options) const
    {
        return library->getMaterialTree(filter, options);
    }
    std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
    getMaterialTree(const std::shared_ptr<MaterialLibrary>& library) const
    {
        std::shared_ptr<Materials::MaterialFilter> filter;
        MaterialFilterOptions options;
        return library->getMaterialTree(filter, options);
    }
    std::shared_ptr<std::list<QString>>
    getMaterialFolders(const std::shared_ptr<MaterialLibrary>& library) const;
    void createFolder(const std::shared_ptr<MaterialLibrary>& library, const QString& path) const
    {
        library->createFolder(path);
    }
    void renameFolder(const std::shared_ptr<MaterialLibrary>& library,
                      const QString& oldPath,
                      const QString& newPath) const
    {
        library->renameFolder(oldPath, newPath);
    }
    void deleteRecursive(const std::shared_ptr<MaterialLibrary>& library, const QString& path) const
    {
        library->deleteRecursive(path);
        dereference();
    }
    void remove(const QString& uuid) const
    {
        _materialMap->erase(uuid);
    }

    void saveMaterial(const std::shared_ptr<MaterialLibrary>& library,
                      const std::shared_ptr<Material>& material,
                      const QString& path,
                      bool overwrite,
                      bool saveAsCopy,
                      bool saveInherited) const;

    bool isMaterial(const fs::path& p) const;
    bool isMaterial(const QFileInfo& file) const;

    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>
    materialsWithModel(const QString& uuid) const;
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>
    materialsWithModelComplete(const QString& uuid) const;
    void dereference(std::shared_ptr<Material> material) const;
    void dereference() const;

private:
    static std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> _libraryList;
    static std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> _materialMap;
    static QMutex _mutex;

    static void initLibraries();
};

}  // namespace Materials

#endif  // MATERIAL_MATERIALMANAGER_H

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

#pragma once

#include <memory>

#include <filesystem>

#include <Base/Parameter.h>
#include <Mod/Material/MaterialGlobal.h>

#include "FolderTree.h"
#include "Materials.h"

#include "MaterialFilter.h"
#include "MaterialLibrary.h"

namespace fs = std::filesystem;

class QMutex;

namespace App
{
class Material;
}

namespace Materials
{
class MaterialManagerExternal;
class MaterialManagerLocal;
class MaterialFilter;
class MaterialFilterOptions;

class MaterialsExport MaterialManager: public Base::BaseClass, ParameterGrp::ObserverType
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ~MaterialManager() override;

    static MaterialManager& getManager();

    static void cleanup();
    static void refresh();

    // Defaults
    static std::shared_ptr<App::Material> defaultAppearance();
    static std::shared_ptr<Material> defaultMaterial();
    static QString defaultMaterialUUID();

    // Library management
    bool useExternal() const { return _useExternal; }
    std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> getLibraries();
    std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> getLocalLibraries();
    std::shared_ptr<MaterialLibrary> getLibrary(const QString& name) const;
    void createLibrary(const QString& libraryName,
                       const QString& iconPath,
                       bool readOnly = true);
    void createLocalLibrary(const QString& libraryName,
                            const QString& directory,
                            const QString& iconPath,
                            bool readOnly = true);
    void renameLibrary(const QString& libraryName, const QString& newName);
    void changeIcon(const QString& libraryName, const QString& iconPath);
    void removeLibrary(const QString& libraryName);
    std::shared_ptr<std::vector<LibraryObject>>
    libraryMaterials(const QString& libraryName, bool local = false);
    std::shared_ptr<std::vector<LibraryObject>>
    libraryMaterials(const QString& libraryName,
                     const MaterialFilter& filter,
                     const MaterialFilterOptions& options,
                     bool local = false);
    bool isLocalLibrary(const QString& libraryName);

    // Folder management
    std::shared_ptr<std::list<QString>>
    getMaterialFolders(const std::shared_ptr<MaterialLibrary>& library) const;
    void createFolder(const std::shared_ptr<MaterialLibrary>& library, const QString& path);
    void renameFolder(const std::shared_ptr<MaterialLibrary>& library,
                      const QString& oldPath,
                      const QString& newPath);
    void deleteRecursive(const std::shared_ptr<MaterialLibrary>& library, const QString& path);

    // Tree management
    std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
    getMaterialTree(const MaterialLibrary& library,
                    const Materials::MaterialFilter& filter) const;
    std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
    getMaterialTree(const MaterialLibrary& library,
                    const Materials::MaterialFilter& filter,
                    const MaterialFilterOptions& options) const;
    std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
    getMaterialTree(const MaterialLibrary& library) const;

    // Material management
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> getLocalMaterials() const;
    std::shared_ptr<Material> getMaterial(const QString& uuid) const;
    static std::shared_ptr<Material> getMaterial(const App::Material& material);
    std::shared_ptr<Material> getMaterialByPath(const QString& path) const;
    std::shared_ptr<Material> getMaterialByPath(const QString& path, const QString& library) const;
    std::shared_ptr<Material> getParent(const std::shared_ptr<Material>& material) const;
    bool exists(const QString& uuid) const;
    bool exists(const MaterialLibrary& library, const QString& uuid) const;
    void remove(const QString& uuid) const;

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

    /// Observer message from the ParameterGrp
    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;

#if defined(BUILD_MATERIAL_EXTERNAL)
    void migrateToExternal(const std::shared_ptr<Materials::MaterialLibrary>& library);
    void validateMigration(const std::shared_ptr<Materials::MaterialLibrary>& library);

    // Cache functions
    static double materialHitRate();
#endif

private:
    MaterialManager();

    FC_DISABLE_COPY_MOVE(MaterialManager);

    static void initManagers();

    static MaterialManager* _manager;

#if defined(BUILD_MATERIAL_EXTERNAL)
    static std::unique_ptr<MaterialManagerExternal> _externalManager;
#endif
    static std::unique_ptr<MaterialManagerLocal> _localManager;
    static QMutex _mutex;
    static bool _useExternal;

    ParameterGrp::handle _hGrp;
};

}  // namespace Materials
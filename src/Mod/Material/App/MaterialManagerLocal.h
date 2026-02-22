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

#include <Mod/Material/MaterialGlobal.h>

#include "FolderTree.h"
#include "Materials.h"

namespace fs = std::filesystem;

class QMutex;

namespace App
{
class Material;
}

namespace Materials
{

class LibraryObject;
class MaterialLibrary;
class MaterialLibraryLocal;
class MaterialFilter;
class MaterialFilterOptions;

class MaterialsExport MaterialManagerLocal: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialManagerLocal();
    ~MaterialManagerLocal() override = default;

    static void cleanup();
    static void refresh();

    // Library management
    std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> getLibraries();
    std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> getMaterialLibraries();
    std::shared_ptr<MaterialLibrary> getLibrary(const QString& name) const;
    void createLibrary(const QString& libraryName,
                       const QString& directory,
                       const QString& iconPath,
                       bool readOnly = true);
    void renameLibrary(const QString& libraryName, const QString& newName);
    void changeIcon(const QString& libraryName, const QByteArray& icon);
    void removeLibrary(const QString& libraryName);
    std::shared_ptr<std::vector<LibraryObject>>
    libraryMaterials(const QString& libraryName);
    std::shared_ptr<std::vector<LibraryObject>>
    libraryMaterials(const QString& libraryName,
                     const MaterialFilter& filter,
                     const MaterialFilterOptions& options);

    // Folder management
    std::shared_ptr<std::list<QString>>
    getMaterialFolders(const std::shared_ptr<MaterialLibraryLocal>& library) const;
    void createFolder(const std::shared_ptr<MaterialLibraryLocal>& library, const QString& path);
    void renameFolder(const std::shared_ptr<MaterialLibraryLocal>& library,
                      const QString& oldPath,
                      const QString& newPath);
    void deleteRecursive(const std::shared_ptr<MaterialLibraryLocal>& library, const QString& path);

    // Material management
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> getLocalMaterials() const;
    std::shared_ptr<Material> getMaterial(const QString& uuid) const;
    std::shared_ptr<Material> getMaterialByPath(const QString& path) const;
    std::shared_ptr<Material> getMaterialByPath(const QString& path, const QString& library) const;
    bool exists(const QString& uuid) const;
    bool exists(const MaterialLibrary& library, const QString& uuid) const;
    void remove(const QString& uuid);

    void saveMaterial(const std::shared_ptr<MaterialLibraryLocal>& library,
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

protected:
    static std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> getConfiguredLibraries();
    bool passFilter(const Material& material,
                    const Materials::MaterialFilter& filter,
                    const Materials::MaterialFilterOptions& options) const;

private:
    static std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> _libraryList;
    static std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> _materialMap;
    static QMutex _mutex;

    static void initLibraries();
};

}  // namespace Materials
// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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
#include <lru/lru.hpp>

#include <Mod/Material/MaterialGlobal.h>

#include <QMutex>

#include "FolderTree.h"
#include "Materials.h"

class QMutex;

namespace App
{
class Material;
}

namespace Materials
{

class LibraryObject;
class MaterialLibrary;
class MaterialLibraryExternal;
class MaterialFilter;
class MaterialFilterOptions;

class MaterialsExport MaterialManagerExternal: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialManagerExternal();
    ~MaterialManagerExternal() override = default;

    static void cleanup();
    void refresh();

    static const int DEFAULT_CACHE_SIZE = 100;

    // Library management
    std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> getLibraries();
    std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> getMaterialLibraries();
    std::shared_ptr<MaterialLibrary> getLibrary(const std::string& name) const;
    void createLibrary(const std::string& libraryName,
                       const QByteArray& icon,
                       bool readOnly = true);
    void renameLibrary(const std::string& libraryName, const std::string& newName);
    void changeIcon(const std::string& libraryName, const QByteArray& icon);
    void removeLibrary(const std::string& libraryName);
    std::shared_ptr<std::vector<LibraryObject>>
    libraryMaterials(const std::string& libraryName);
    std::shared_ptr<std::vector<LibraryObject>>
    libraryMaterials(const std::string& libraryName,
                     const MaterialFilter& filter,
                     const MaterialFilterOptions& options);

    // Folder management
    void createFolder(const MaterialLibrary& library, const std::string& path);
    void
    renameFolder(const MaterialLibrary& library, const std::string& oldPath, const std::string& newPath);
    void deleteRecursive(const MaterialLibrary& library, const std::string& path);

    // Material management
    std::shared_ptr<Material> getMaterial(const std::string& uuid) const;
    void addMaterial(const std::string& libraryName,
                     const std::string& path,
                     const Material& material);
    void migrateMaterial(const std::string& libraryName,
                     const std::string& path,
                     const Material& material);

    // Cache functions
    void resetCache();
    double materialHitRate();

private:
    static void initCache();
    std::shared_ptr<Material> materialNotFound(const std::string& uuid) const;

    static QMutex _mutex;

    // Older platforms (Ubuntu 20.04) can't use QString as the index
    // due to a lack of a move constructor
    static LRU::Cache<std::string, std::shared_ptr<Material>> _cache;
};

}  // namespace Materials
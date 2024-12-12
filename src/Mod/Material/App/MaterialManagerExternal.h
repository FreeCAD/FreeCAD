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

#ifndef MATERIAL_MATERIALMANAGEREXTERNAl_H
#define MATERIAL_MATERIALMANAGEREXTERNAl_H

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
    std::shared_ptr<MaterialLibrary> getLibrary(const QString& name) const;
    void createLibrary(const QString& libraryName, const QString& icon, bool readOnly = true);
    std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
    libraryMaterials(const QString& libraryName);
    std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
    libraryMaterials(const QString& libraryName,
                     const std::shared_ptr<MaterialFilter>& filter,
                     const MaterialFilterOptions& options);

    // Folder management

    // Material management
    std::shared_ptr<Material> getMaterial(const QString& uuid) const;
    void addMaterial(const QString& libraryName,
                     const QString& path,
                     const std::shared_ptr<Material>& material);
    void migrateMaterial(const QString& libraryName,
                     const QString& path,
                     const std::shared_ptr<Material>& material);

    // Cache functions
    void resetCache();
    double materialHitRate();

private:
    static void initCache();

    static QMutex _mutex;

    // Older platforms (Ubuntu 20.04) can't use QString as the index
    // due to a lack of a move constructor
    static LRU::Cache<std::string, std::shared_ptr<Material>> _cache;
};

}  // namespace Materials

#endif  // MATERIAL_MATERIALMANAGEREXTERNAl_H
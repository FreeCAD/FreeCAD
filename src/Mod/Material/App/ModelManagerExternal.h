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

#ifndef MATERIAL_MODELMANAGEREXTERNAL_H
#define MATERIAL_MODELMANAGEREXTERNAL_H

#include <memory>
#include <lru/lru.hpp>

#include <Mod/Material/MaterialGlobal.h>

#include <QMutex>

#include "Exceptions.h"
#include "FolderTree.h"
#include "Model.h"
#include "ModelLibrary.h"

namespace Materials
{

class MaterialsExport ModelManagerExternal: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ModelManagerExternal();
    ~ModelManagerExternal() override = default;

    static void cleanup();
    void refresh();

    static const int DEFAULT_CACHE_SIZE = 100;

    // Library management
    std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> getLibraries();
    void createLibrary(const QString& libraryName,
                       const QString& icon,
                       bool readOnly = true);

    // Model management
    std::shared_ptr<Model> getModel(const QString& uuid);
    void
    addModel(const QString& libraryName, const QString& path, const std::shared_ptr<Model>& model);
    void
    migrateModel(const QString& libraryName, const QString& path, const std::shared_ptr<Model>& model);

    // Cache functions
    void resetCache();
    double modelHitRate();

private:
    static void initCache();

    static QMutex _mutex;

    // Older platforms (Ubuntu 20.04) can't use QString as the index
    // due to a lack of a move constructor
    static LRU::Cache<std::string, std::shared_ptr<Model>> _cache;
};

}  // namespace Materials

#endif  // MATERIAL_MODELMANAGEREXTERNAL_H
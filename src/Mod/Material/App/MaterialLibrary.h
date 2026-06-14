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

#include <QDir>
#include <QString>
#include <QVariant>

#include <Base/BaseClass.h>
#include <Mod/Material/MaterialGlobal.h>

#include "Library.h"
#include "Materials.h"
#include "Model.h"
#include "ModelLibrary.h"

namespace Materials
{

class Material;
class MaterialManager;
class MaterialFilter;
class MaterialFilterOptions;

class MaterialsExport MaterialLibrary
    : public Library,
      public std::enable_shared_from_this<MaterialLibrary>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialLibrary() = default;
    MaterialLibrary(const QString& libraryName, const QString& icon, bool readOnly = true);
    MaterialLibrary(const QString& libraryName,
                    const QString& dir,
                    const QString& iconPath,
                    bool readOnly = true);
    MaterialLibrary(const Library& library);
    MaterialLibrary(const MaterialLibrary&) = delete;
    ~MaterialLibrary() override = default;

    virtual std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
    getMaterialTree(const Materials::MaterialFilter& filter,
                    const Materials::MaterialFilterOptions& options) const;

    // Use this to get a shared_ptr for *this
    std::shared_ptr<MaterialLibrary> getptr()
    {
        return shared_from_this();
    }
};

class MaterialsExport MaterialLibraryLocal: public MaterialLibrary
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialLibraryLocal() = default;
    MaterialLibraryLocal(const QString& libraryName,
                         const QString& dir,
                         const QString& iconPath,
                         bool readOnly = true);
    ~MaterialLibraryLocal() override = default;

    void createFolder(const QString& path);
    void renameFolder(const QString& oldPath, const QString& newPath);
    void deleteRecursive(const QString& path);

    std::shared_ptr<Material> saveMaterial(const std::shared_ptr<Material>& material,
                                           const QString& path,
                                           bool overwrite,
                                           bool saveAsCopy,
                                           bool saveInherited);
    bool fileExists(const QString& path) const;
    std::shared_ptr<Material> addMaterial(const std::shared_ptr<Material>& material,
                                          const QString& path);
    std::shared_ptr<Material> getMaterialByPath(const QString& path) const;

    bool operator==(const MaterialLibrary& library) const
    {
        return library.isLocal() ? Library::operator==(library) : false;
    }
    bool operator!=(const MaterialLibrary& library) const
    {
        return !operator==(library);
    }

    bool operator==(const MaterialLibraryLocal& library) const
    {
        return Library::operator==(library);
    }
    bool operator!=(const MaterialLibraryLocal& library) const
    {
        return !operator==(library);
    }

protected:
    void deleteDir(MaterialManager& manager, const QString& path);
    void deleteFile(MaterialManager& manager, const QString& path);
    void updatePaths(const QString& oldPath, const QString& newPath);

    QString getUUIDFromPath(const QString& path) const;

    std::unique_ptr<std::map<QString, std::shared_ptr<Material>>> _materialPathMap;
};

}  // namespace Materials

Q_DECLARE_METATYPE(std::shared_ptr<Materials::MaterialLibrary>)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::MaterialLibraryLocal>)
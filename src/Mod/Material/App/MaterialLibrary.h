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

#ifndef MATERIAL_MATERIALLIBRARY_H
#define MATERIAL_MATERIALLIBRARY_H

#include <memory>

#include <QDir>
#include <QString>
#include <QVariant>

#include <Base/BaseClass.h>
#include <Mod/Material/MaterialGlobal.h>

#include "Materials.h"
#include "Model.h"
#include "ModelLibrary.h"

namespace Materials
{

class Material;
class MaterialManager;
class MaterialFilter;
class MaterialFilterOptions;

class MaterialsExport MaterialLibrary: public LibraryBase,
                                       public std::enable_shared_from_this<MaterialLibrary>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialLibrary() = default;
    MaterialLibrary(const MaterialLibrary&) = delete;
    MaterialLibrary(const QString& libraryName,
                    const QString& dir,
                    const QString& icon,
                    bool readOnly = true);
    ~MaterialLibrary() override = default;

    bool operator==(const MaterialLibrary& library) const
    {
        return LibraryBase::operator==(library);
    }
    bool operator!=(const MaterialLibrary& library) const
    {
        return !operator==(library);
    }
    std::shared_ptr<Material> getMaterialByPath(const QString& path) const;

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
    std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>
    getMaterialTree(const std::shared_ptr<Materials::MaterialFilter>& filter,
                    const Materials::MaterialFilterOptions& options) const;

    bool isReadOnly() const
    {
        return _readOnly;
    }

    // Use this to get a shared_ptr for *this
    std::shared_ptr<MaterialLibrary> getptr()
    {
        return shared_from_this();
    }

protected:
    void deleteDir(MaterialManager& manager, const QString& path);
    void deleteFile(MaterialManager& manager, const QString& path);

    void updatePaths(const QString& oldPath, const QString& newPath);
    QString getUUIDFromPath(const QString& path) const;
    bool materialInTree(const std::shared_ptr<Material>& material,
                        const std::shared_ptr<Materials::MaterialFilter>& filter,
                        const Materials::MaterialFilterOptions& options) const;

    bool _readOnly;
    std::unique_ptr<std::map<QString, std::shared_ptr<Material>>> _materialPathMap;
};

class MaterialsExport MaterialExternalLibrary: public MaterialLibrary
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialExternalLibrary() = default;
    MaterialExternalLibrary(const QString& libraryName,
                            const QString& dir,
                            const QString& icon,
                            bool readOnly = true);
    ~MaterialExternalLibrary() override = default;
};

}  // namespace Materials

Q_DECLARE_METATYPE(std::shared_ptr<Materials::MaterialLibrary>)

#endif  // MATERIAL_MATERIALLIBRARY_H

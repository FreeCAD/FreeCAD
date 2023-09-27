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

#include <Mod/Material/MaterialGlobal.h>

#include <QMutex>

#include <boost/filesystem.hpp>

#include "FolderTree.h"
#include "Materials.h"

namespace fs = boost::filesystem;

namespace Materials
{

typedef FolderTreeNode<Material> MaterialTreeNode;

class MaterialsExport MaterialManager: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialManager();
    ~MaterialManager() override = default;

    std::shared_ptr<std::map<QString, Material*>> getMaterials()
    {
        return _materialMap;
    }
    const Material& getMaterial(const QString& uuid) const;
    const Material& getMaterialByPath(const QString& path) const;
    const Material& getMaterialByPath(const QString& path, const QString& library) const;
    MaterialLibrary* getLibrary(const QString& name) const;

    // Library management
    static std::shared_ptr<std::list<MaterialLibrary*>> getMaterialLibraries();
    std::shared_ptr<std::map<QString, MaterialTreeNode*>>
    getMaterialTree(const MaterialLibrary& library) const;
    std::shared_ptr<std::list<QString>> getMaterialFolders(const MaterialLibrary& library) const;
    void createPath(MaterialLibrary* library, const QString& path)
    {
        library->createPath(path);
    }
    void saveMaterial(MaterialLibrary* library,
                      Material& material,
                      const QString& path,
                      bool saveAsCopy = true);

    static bool isMaterial(const fs::path& p);

    std::shared_ptr<std::map<QString, Material*>> materialsWithModel(QString uuid);
    std::shared_ptr<std::map<QString, Material*>> materialsWithModelComplete(QString uuid);

private:
    static std::shared_ptr<std::list<MaterialLibrary*>> _libraryList;
    static std::shared_ptr<std::map<QString, Material*>> _materialMap;
    static QMutex _mutex;

    static void initLibraries();
};

}  // namespace Materials

#endif  // MATERIAL_MATERIALMANAGER_H

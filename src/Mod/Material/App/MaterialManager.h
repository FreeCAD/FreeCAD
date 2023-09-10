/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef MATERIAL_MATERIALMANAGER_H
#define MATERIAL_MATERIALMANAGER_H

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
    TYPESYSTEM_HEADER();

public:
    MaterialManager();
    virtual ~MaterialManager() = default;

    std::map<QString, Material*>* getMaterials()
    {
        return _materialMap;
    }
    const Material& getMaterial(const QString& uuid) const;
    const Material& getMaterialByPath(const QString& path) const;
    const Material& getMaterialByPath(const QString& path, const QString& library) const;
    MaterialLibrary* getLibrary(const QString& name) const;

    // Library management
    static std::list<MaterialLibrary*>* getMaterialLibraries();
    std::map<QString, MaterialTreeNode*>* getMaterialTree(const MaterialLibrary& library);
    std::list<QString>* getMaterialFolders(const MaterialLibrary& library);
    void createPath(MaterialLibrary* library, const QString& path)
    {
        library->createPath(path);
    }
    void saveMaterial(MaterialLibrary* library,
                      Material& material,
                      const QString& path,
                      bool saveAsCopy = true);

    static bool isMaterial(const fs::path& p);

    std::map<QString, Material*>* materialsWithModel(QString uuid);
    std::map<QString, Material*>* materialsWithModelComplete(QString uuid);

private:
    static std::list<MaterialLibrary*>* _libraryList;
    static std::map<QString, Material*>* _materialMap;
    static QMutex _mutex;

    static void initLibraries();
};

}// namespace Materials

#endif// MATERIAL_MATERIALMANAGER_H

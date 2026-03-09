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

#include <Mod/Material/MaterialGlobal.h>

#include <QMutex>

#include "Exceptions.h"
#include "FolderTree.h"
#include "Model.h"
#include "ModelLibrary.h"

namespace Materials
{

class MaterialsExport ModelManagerLocal: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ModelManagerLocal();
    ~ModelManagerLocal() override = default;

    static void cleanup();
    void refresh();

    std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> getLibraries();
    void createLibrary(const QString& libraryName,
                       const QString& directory,
                       const QString& icon,
                       bool readOnly = true);
    void renameLibrary(const QString& libraryName, const QString& newName);
    void changeIcon(const QString& libraryName, const QString& icon);
    void removeLibrary(const QString& libraryName);
    std::shared_ptr<std::vector<LibraryObject>>
    libraryModels(const QString& libraryName);

    std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> getModels()
    {
        return _modelMap;
    }
    std::shared_ptr<std::map<QString, std::shared_ptr<ModelTreeNode>>>
    getModelTree(std::shared_ptr<ModelLibrary> library, ModelFilter filter = ModelFilter_None) const
    {
        return library->getModelTree(filter);
    }
    std::shared_ptr<Model> getModel(const QString& uuid) const;
    std::shared_ptr<Model> getModelByPath(const QString& path) const;
    std::shared_ptr<Model> getModelByPath(const QString& path, const QString& lib) const;
    std::shared_ptr<ModelLibrary> getLibrary(const QString& name) const;

    static bool isModel(const QString& file);

private:
    static void initLibraries();

    static std::shared_ptr<std::list<std::shared_ptr<ModelLibraryLocal>>> _libraryList;
    static std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> _modelMap;
    static QMutex _mutex;
};

}  // namespace Materials
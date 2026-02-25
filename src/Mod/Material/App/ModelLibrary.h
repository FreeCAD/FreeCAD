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

#include <QDir>
#include <QString>

#include <Base/BaseClass.h>
#include <Base/Quantity.h>

#include <Mod/Material/MaterialGlobal.h>

#include "Library.h"
#include "MaterialValue.h"
#include "Model.h"

namespace Materials
{

class MaterialsExport ModelLibrary: public Library,
                                    public std::enable_shared_from_this<ModelLibrary>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ModelLibrary();
    ModelLibrary(const Library& library);
    ModelLibrary(const QString& libraryName,
                 const QString& dir,
                 const QString& iconPath,
                 bool readOnly = true);
    ModelLibrary(const ModelLibrary& other) = delete;
    ~ModelLibrary() override = default;

    std::shared_ptr<std::map<QString, std::shared_ptr<ModelTreeNode>>>
    getModelTree(ModelFilter filter) const;

    // Use this to get a shared_ptr for *this
    std::shared_ptr<ModelLibrary> getptr()
    {
        return shared_from_this();
    }
};

class MaterialsExport ModelLibraryLocal: public ModelLibrary
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ModelLibraryLocal();
    ModelLibraryLocal(const Library& other);
    ModelLibraryLocal(const QString& libraryName,
                      const QString& dir,
                      const QString& iconPath,
                      bool readOnly = true);
    ModelLibraryLocal(const ModelLibraryLocal& other) = delete;
    ~ModelLibraryLocal() override = default;

    bool operator==(const ModelLibrary& library) const
    {
        return Library::operator==(library);
    }
    bool operator!=(const ModelLibrary& library) const
    {
        return !operator==(library);
    }
    std::shared_ptr<Model> getModelByPath(const QString& path) const;

    std::shared_ptr<Model> addModel(const Model& model, const QString& path);

private:

    std::unique_ptr<std::map<QString, std::shared_ptr<Model>>> _modelPathMap;
};

}  // namespace Materials

Q_DECLARE_METATYPE(std::shared_ptr<Materials::ModelLibrary>)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::ModelLibraryLocal>)
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

#ifndef MATERIAL_MODELMANAGER_H
#define MATERIAL_MODELMANAGER_H

#include <memory>

#include <Mod/Material/MaterialGlobal.h>

#include <QMutex>

#include "Exceptions.h"
#include "FolderTree.h"
#include "Model.h"
#include "ModelLibrary.h"

namespace Materials
{

class MaterialsExport ModelManager: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ModelManager();
    ~ModelManager() override = default;

    static void cleanup();
    void refresh();

    std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> getModelLibraries()
    {
        return _libraryList;
    }
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
    static bool passFilter(ModelFilter filter, Model::ModelType modelType);

private:
    static void initLibraries();

    static std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> _libraryList;
    static std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> _modelMap;
    static QMutex _mutex;
};

}  // namespace Materials

#endif  // MATERIAL_MODELMANAGER_H

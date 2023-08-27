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

#ifndef MATERIAL_MODELMANAGER_H
#define MATERIAL_MODELMANAGER_H

#include <boost/filesystem.hpp>

#include "Exceptions.h"
#include "Model.h"
#include "FolderTree.h"

namespace fs = boost::filesystem;

namespace Materials {

typedef FolderTreeNode<Model> ModelTreeNode;

class MaterialsExport ModelManager : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    enum ModelFilter
    {
        ModelFilter_None,
        ModelFilter_Physical,
        ModelFilter_Appearance
    };

    virtual ~ModelManager();

    static ModelManager *getManager();

    void refresh();

    std::list<ModelLibrary*> *getModelLibraries() { return _libraryList; }
    std::map<QString, Model*> *getModels() { return _modelMap; }
    std::map<QString, ModelTreeNode*>* getModelTree(const ModelLibrary &library, ModelFilter filter=ModelFilter_None);
    const Model &getModel(const QString& uuid) const;
    const Model &getModelByPath(const QString &path) const;
    const Model &getModelByPath(const QString &path, const QString &libraryPath) const;

    static bool isModel(const fs::path& p);
    bool passFilter(ModelFilter filter, Model::ModelType modelType) const;

private:
    ModelManager();

    static ModelManager *manager;
    static std::list<ModelLibrary*> *_libraryList;
    static std::map<QString, Model*> *_modelMap;
};

} // namespace Materials

#endif // MATERIAL_MODELMANAGER_H

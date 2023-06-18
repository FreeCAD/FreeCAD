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

namespace fs = boost::filesystem;

namespace Materials {

class ModelTreeNode
{
public:
    enum NodeType {
        ModelNode,
        FolderNode
    };

    explicit ModelTreeNode();
    virtual ~ModelTreeNode();

    NodeType getType(void) const { return _type; }
    void setType(NodeType type) { _type = type; }

    const std::map<std::string, ModelTreeNode*> *getFolder(void) const { return _folder; }
    std::map<std::string, ModelTreeNode*> *getFolder(void) { return _folder; }
    const Model *getModel(void) const { return _model; }

    void setData(std::map<std::string, ModelTreeNode*> *folder);
    void setData(const Model *model);

private:
    NodeType    _type;
    std::map<std::string, ModelTreeNode*> *_folder;
    const Model *_model;
};

class MaterialsExport ModelManager : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    virtual ~ModelManager();

    static ModelManager *getManager();

    void refresh();
    
    std::list<ModelLibrary*> *getModelLibraries() { return _libraryList; }
    std::map<std::string, Model*> *getModels() { return _modelMap; }
    std::map<std::string, ModelTreeNode*>* getModelTree(const ModelLibrary &library);
    const Model &getModel(const std::string& uuid) const;
    const Model &getModelByPath(const std::string &path) const;
    const Model &getModelByPath(const std::string &path, const std::string &libraryPath) const;

    static bool isModel(const fs::path& p);

private:
    explicit ModelManager();

    static ModelManager *manager;
    static std::list<ModelLibrary*> *_libraryList;
    static std::map<std::string, Model*> *_modelMap;
};

} // namespace Materials

#endif // MATERIAL_MODELMANAGER_H

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

#ifndef MATERIAL_MODELLOADER_H
#define MATERIAL_MODELLOADER_H

#include <QDir>
#include <QString>
#include <yaml-cpp/yaml.h>

#include "Model.h"

namespace Materials {

class ModelEntry
{
public:
    explicit ModelEntry(const ModelLibrary &library, const std::string &baseName, const std::string &modelName, const QDir &dir, 
        const std::string &modelUuid, const YAML::Node &modelData);
    virtual ~ModelEntry();

    const ModelLibrary &getLibrary() const { return _library; }
    const std::string &getBase() const { return _base; }
    const std::string &getName() const { return _name; }
    const QDir &getDirectory() const { return _directory; }
    const std::string &getUUID() const { return _uuid; }
    const YAML::Node &getModel() const { return _model; }
    YAML::Node *getModelPtr() { return &_model; }
    bool getDereferenced() const { return _dereferenced; }

    void markDereferenced() { _dereferenced = true; }

private:
    explicit ModelEntry();

    ModelLibrary _library;
    std::string _base;
    std::string _name;
    QDir _directory;
    std::string _uuid;
    YAML::Node _model;
    bool _dereferenced;
};

class ModelLoader
{
public:
    explicit ModelLoader(std::map<std::string, Model*> *modelMap, std::list<ModelLibrary*> *libraryList);
    virtual ~ModelLoader();

    std::list<ModelLibrary*> *getModelLibraries();
    static const std::string getUUIDFromPath(const std::string &path);

private:
    explicit ModelLoader();

    void addToTree(ModelEntry* model);
    void showYaml(const YAML::Node& yaml) const;
    void dereference(ModelEntry* parent, const ModelEntry* child);
    void dereference(ModelEntry* model);
    ModelEntry *getModelFromPath(const ModelLibrary &library, const std::string &path) const;
    void addLibrary(ModelLibrary* model);
    void loadLibrary(const ModelLibrary &library);
    void loadLibraries(void);
    static std::map<std::string, ModelEntry*> *_modelEntryMap;
    std::map<std::string, Model*> *_modelMap;
    std::list<ModelLibrary*> *_libraryList;
};

} // namespace Materials

#endif // MATERIAL_MODELLOADER_H

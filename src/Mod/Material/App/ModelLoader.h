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

namespace Materials
{

class ModelEntry
{
public:
    explicit ModelEntry(const ModelLibrary& library,
                        const QString& baseName,
                        const QString& modelName,
                        const QString& dir,
                        const QString& modelUuid,
                        const YAML::Node& modelData);
    virtual ~ModelEntry() = default;

    const ModelLibrary& getLibrary() const
    {
        return _library;
    }
    const QString getBase() const
    {
        return _base;
    }
    const QString getName() const
    {
        return _name;
    }
    const QString getDirectory() const
    {
        return _directory;
    }
    const QString getUUID() const
    {
        return _uuid;
    }
    const YAML::Node& getModel() const
    {
        return _model;
    }
    YAML::Node* getModelPtr()
    {
        return &_model;
    }
    bool getDereferenced() const
    {
        return _dereferenced;
    }

    void markDereferenced()
    {
        _dereferenced = true;
    }

private:
    ModelEntry();

    ModelLibrary _library;
    QString _base;
    QString _name;
    QString _directory;
    QString _uuid;
    YAML::Node _model;
    bool _dereferenced;
};

class ModelLoader
{
public:
    explicit ModelLoader(std::map<QString, Model*>* modelMap,
                         std::list<ModelLibrary*>* libraryList);
    virtual ~ModelLoader() = default;

    static const QString getUUIDFromPath(const QString& path);

private:
    ModelLoader();

    void getModelLibraries();
    QString
    yamlValue(const YAML::Node& node, const std::string& key, const std::string& defaultValue);
    void addToTree(ModelEntry* model, std::map<std::pair<QString, QString>, QString>* inheritances);
    void showYaml(const YAML::Node& yaml) const;
    void dereference(const QString& uuid,
                     ModelEntry* parent,
                     const ModelEntry* child,
                     std::map<std::pair<QString, QString>, QString>* inheritances);
    void dereference(ModelEntry* model,
                     std::map<std::pair<QString, QString>, QString>* inheritances);
    ModelEntry* getModelFromPath(const ModelLibrary& library, const QString& path) const;
    void addLibrary(ModelLibrary* model);
    void loadLibrary(const ModelLibrary& library);
    void loadLibraries(void);
    static std::map<QString, ModelEntry*>* _modelEntryMap;
    std::map<QString, Model*>* _modelMap;
    std::list<ModelLibrary*>* _libraryList;
};

}// namespace Materials

#endif// MATERIAL_MODELLOADER_H

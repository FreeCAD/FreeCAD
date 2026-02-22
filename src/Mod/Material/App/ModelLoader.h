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
#include <yaml-cpp/yaml.h>

#include "Model.h"
#include "ModelLibrary.h"

namespace Materials
{

class ModelEntry
{
public:
    ModelEntry(const std::shared_ptr<ModelLibraryLocal>& library,
               const QString& baseName,
               const QString& modelName,
               const QString& dir,
               const QString& modelUuid,
               const YAML::Node& modelData);
    virtual ~ModelEntry() = default;

    std::shared_ptr<ModelLibraryLocal> getLibrary() const
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

    std::shared_ptr<ModelLibraryLocal> _library;
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
    ModelLoader(std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> modelMap,
                std::shared_ptr<std::list<std::shared_ptr<ModelLibraryLocal>>> libraryList);
    virtual ~ModelLoader() = default;

    static const QString getUUIDFromPath(const QString& path);

private:
    ModelLoader();

    void getModelLibraries();
    QString
    yamlValue(const YAML::Node& node, const std::string& key, const std::string& defaultValue);
    void addToTree(std::shared_ptr<ModelEntry> model,
                   std::map<std::pair<QString, QString>, QString>* inheritances);
    void showYaml(const YAML::Node& yaml) const;
    void dereference(const QString& uuid,
                     std::shared_ptr<ModelEntry> parent,
                     std::shared_ptr<ModelEntry> child,
                     std::map<std::pair<QString, QString>, QString>* inheritances);
    void dereference(std::shared_ptr<ModelEntry> model,
                     std::map<std::pair<QString, QString>, QString>* inheritances);
    std::shared_ptr<ModelEntry> getModelFromPath(std::shared_ptr<ModelLibrary> library,
                                                 const QString& path) const;
    void addLibrary(std::shared_ptr<ModelLibraryLocal> model);
    void loadLibrary(std::shared_ptr<ModelLibraryLocal> library);
    void loadLibraries();

    static std::unique_ptr<std::map<QString, std::shared_ptr<ModelEntry>>> _modelEntryMap;
    std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> _modelMap;
    std::shared_ptr<std::list<std::shared_ptr<ModelLibraryLocal>>> _libraryList;
};

}  // namespace Materials
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

#include "Materials.h"
#include "trim.h"

namespace Materials
{
class MaterialLibrary;
class MaterialLibraryLocal;

class MaterialEntry
{
public:
    MaterialEntry() = default;
    MaterialEntry(const std::shared_ptr<MaterialLibraryLocal>& library,
                  const QString& modelName,
                  const QString& dir,
                  const QString& modelUuid);
    virtual ~MaterialEntry() = default;

    virtual void
    addToTree(std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> materialMap) = 0;

    std::shared_ptr<MaterialLibraryLocal> getLibrary() const
    {
        return _library;
    }
    QString getName() const
    {
        return _name;
    }
    QString getDirectory() const
    {
        return _directory;
    }
    QString getUUID() const
    {
        return _uuid;
    }

protected:
    std::shared_ptr<MaterialLibraryLocal> _library;
    QString _name;
    QString _directory;
    QString _uuid;
};

class MaterialYamlEntry: public MaterialEntry
{
public:
    MaterialYamlEntry(const std::shared_ptr<MaterialLibraryLocal>& library,
                      const QString& modelName,
                      const QString& dir,
                      const QString& modelUuid,
                      const YAML::Node& modelData);
    ~MaterialYamlEntry() override = default;

    void
    addToTree(std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> materialMap) override;

    const YAML::Node& getModel() const
    {
        return _model;
    }
    YAML::Node* getModelPtr()
    {
        return &_model;
    }

private:
    MaterialYamlEntry();

    static QString
    yamlValue(const YAML::Node& node, const std::string& key, const std::string& defaultValue);
    static std::shared_ptr<QList<QVariant>> readList(const YAML::Node& node,
                                                     bool isImageList = false);
    static std::shared_ptr<QList<QVariant>> readImageList(const YAML::Node& node);
    static std::shared_ptr<Array2D> read2DArray(const YAML::Node& node, int columns);
    static std::shared_ptr<Array3D> read3DArray(const YAML::Node& node, int columns);

    YAML::Node _model;
};

class MaterialLoader
{
public:
    MaterialLoader(const std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>& materialMap,
                   const std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>& libraryList);
    ~MaterialLoader() = default;

    static std::shared_ptr<std::list<QString>>
    getMaterialFolders(const MaterialLibraryLocal& library);
    static void showYaml(const YAML::Node& yaml);
    static void
    dereference(const std::shared_ptr<std::map<QString, std::shared_ptr<Material>>>& materialMap,
                const std::shared_ptr<Material>& material);
    static std::shared_ptr<MaterialEntry>
    getMaterialFromYAML(const std::shared_ptr<MaterialLibraryLocal>& library,
                        YAML::Node& yamlroot,
                        const QString& path);

private:
    MaterialLoader();

    void addToTree(std::shared_ptr<MaterialEntry> model);
    void dereference(const std::shared_ptr<Material>& material);
    std::shared_ptr<MaterialEntry>
    getMaterialFromPath(const std::shared_ptr<MaterialLibraryLocal>& library, const QString& path) const;
    void addLibrary(const std::shared_ptr<MaterialLibraryLocal>& model);
    void loadLibrary(const std::shared_ptr<MaterialLibraryLocal>& library);
    void loadLibraries(
        const std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>& libraryList);

    static std::unique_ptr<std::map<QString, std::shared_ptr<MaterialEntry>>> _materialEntryMap;
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> _materialMap;
    std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> _libraryList;
};

}  // namespace Materials
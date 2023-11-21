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

#ifndef MATERIAL_MATERIALLOADER_H
#define MATERIAL_MATERIALLOADER_H

#include <memory>

#include <QDir>
#include <QString>
#include <yaml-cpp/yaml.h>

#include "Materials.h"
#include "trim.h"

namespace Materials
{

class MaterialEntry
{
public:
    MaterialEntry() = default;
    MaterialEntry(const std::shared_ptr<MaterialLibrary>& library,
                  const QString& modelName,
                  const QString& dir,
                  const QString& modelUuid);
    virtual ~MaterialEntry() = default;

    virtual void
    addToTree(std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> materialMap) = 0;

    std::shared_ptr<MaterialLibrary> getLibrary() const
    {
        return _library;
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

protected:
    std::shared_ptr<MaterialLibrary> _library;
    QString _name;
    QString _directory;
    QString _uuid;
};

class MaterialYamlEntry: public MaterialEntry
{
public:
    MaterialYamlEntry(const std::shared_ptr<MaterialLibrary>& library,
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

    QString
    yamlValue(const YAML::Node& node, const std::string& key, const std::string& defaultValue);
    std::shared_ptr<QList<QVariant>> readList(const YAML::Node& node);
    std::shared_ptr<Material2DArray> read2DArray(const YAML::Node& node);
    std::shared_ptr<Material3DArray> read3DArray(const YAML::Node& node);

    YAML::Node _model;
};

class MaterialLoader
{
public:
    MaterialLoader(std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> materialMap,
                   std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> libraryList);
    ~MaterialLoader() = default;

    std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> getMaterialLibraries();
    static std::shared_ptr<std::list<QString>> getMaterialFolders(const MaterialLibrary& library);
    static void showYaml(const YAML::Node& yaml);
    static void
    dereference(std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> materialMap,
                std::shared_ptr<Material> material);
    std::shared_ptr<MaterialEntry> getMaterialFromYAML(std::shared_ptr<MaterialLibrary> library,
                                                       YAML::Node& yamlroot,
                                                       const QString& path) const;

private:
    MaterialLoader();

    void addToTree(std::shared_ptr<MaterialEntry> model);
    void dereference(std::shared_ptr<Material> material);
    std::shared_ptr<MaterialEntry> getMaterialFromPath(std::shared_ptr<MaterialLibrary> library,
                                                       const QString& path) const;
    void addLibrary(std::shared_ptr<MaterialLibrary> model);
    void loadLibrary(std::shared_ptr<MaterialLibrary> library);
    void loadLibraries();

    static std::unique_ptr<std::map<QString, std::shared_ptr<MaterialEntry>>> _materialEntryMap;
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> _materialMap;
    std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> _libraryList;
};

}  // namespace Materials

#endif  // MATERIAL_MATERIALLOADER_H

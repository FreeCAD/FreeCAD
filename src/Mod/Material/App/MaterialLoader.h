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
    MaterialEntry();
    explicit MaterialEntry(const MaterialLibrary& library,
                           const QString& modelName,
                           const QString& dir,
                           const QString& modelUuid);
    virtual ~MaterialEntry() = default;

    virtual void addToTree(std::shared_ptr<std::map<QString, Material*>> materialMap) = 0;

    const MaterialLibrary& getLibrary() const
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
    MaterialLibrary _library;
    QString _name;
    QString _directory;
    QString _uuid;
};

class MaterialYamlEntry: public MaterialEntry
{
public:
    explicit MaterialYamlEntry(const MaterialLibrary& library,
                               const QString& modelName,
                               const QString& dir,
                               const QString& modelUuid,
                               const YAML::Node& modelData);
    ~MaterialYamlEntry() override = default;

    void addToTree(std::shared_ptr<std::map<QString, Material*>> materialMap) override;

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

    YAML::Node _model;
};

class MaterialLoader
{
public:
    explicit MaterialLoader(std::shared_ptr<std::map<QString, Material*>> materialMap,
                            std::shared_ptr<std::list<MaterialLibrary*>> libraryList);
    virtual ~MaterialLoader();

    std::shared_ptr<std::list<MaterialLibrary*>> getMaterialLibraries();
    static std::shared_ptr<std::list<QString>> getMaterialFolders(const MaterialLibrary& library);
    static void showYaml(const YAML::Node& yaml);

private:
    MaterialLoader();

    void addToTree(MaterialEntry* model);
    void dereference(Material* material);
    MaterialEntry* getMaterialFromPath(MaterialLibrary& library, const QString& path) const;
    void addLibrary(MaterialLibrary* model);
    void loadLibrary(MaterialLibrary& library);
    void loadLibraries();
    static std::unique_ptr<std::map<QString, MaterialEntry*>> _materialEntryMap;
    std::shared_ptr<std::map<QString, Material*>> _materialMap;
    std::shared_ptr<std::list<MaterialLibrary*>> _libraryList;
};

}  // namespace Materials

#endif  // MATERIAL_MATERIALLOADER_H

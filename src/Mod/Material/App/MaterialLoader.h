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

    virtual void addToTree(std::map<QString, Material*>* materialMap) = 0;

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

    void addToTree(std::map<QString, Material*>* materialMap) override;

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
    explicit MaterialLoader(std::map<QString, Material*>* materialMap,
                            std::list<MaterialLibrary*>* libraryList);
    virtual ~MaterialLoader();

    std::list<MaterialLibrary*>* getMaterialLibraries();
    static std::list<QString>* getMaterialFolders(const MaterialLibrary& library);
    static void showYaml(const YAML::Node& yaml);

private:
    MaterialLoader();

    void addToTree(MaterialEntry* model);
    void dereference(Material* material);
    MaterialEntry* getMaterialFromPath(MaterialLibrary& library, const QString& path) const;
    void addLibrary(MaterialLibrary* model);
    void loadLibrary(MaterialLibrary& library);
    void loadLibraries(void);
    static std::map<QString, MaterialEntry*>* _materialEntryMap;
    std::map<QString, Material*>* _materialMap;
    std::list<MaterialLibrary*>* _libraryList;
};

}// namespace Materials

#endif// MATERIAL_MATERIALLOADER_H

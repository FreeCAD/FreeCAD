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

namespace Materials {

class MaterialEntry
{
public:
    explicit MaterialEntry(const MaterialLibrary &library, const std::string &modelName, const QDir &dir, 
        const std::string &modelUuid, const YAML::Node &modelData);
    virtual ~MaterialEntry();

    const MaterialLibrary &getLibrary() const { return _library; }
    const std::string &getName() const { return _name; }
    const QDir &getDirectory() const { return _directory; }
    const std::string &getUUID() const { return _uuid; }
    const YAML::Node &getModel() const { return _model; }
    YAML::Node *getModelPtr() { return &_model; }
    const bool getDereferenced() const { return _dereferenced; }

    void markDereferenced() { _dereferenced = true; }

private:
    explicit MaterialEntry();

    MaterialLibrary _library;
    std::string _name;
    QDir _directory;
    std::string _uuid;
    YAML::Node _model;
    bool _dereferenced;
};

class MaterialLoader
{
public:
    explicit MaterialLoader(std::map<std::string, Material*> *modelMap, std::list<MaterialLibrary*> *libraryList);
    virtual ~MaterialLoader();

    std::list<MaterialLibrary*>* getMaterialLibraries();
    static const std::string getUUIDFromPath(const std::string &path);

private:
    explicit MaterialLoader();

    std::string yamlValue(const YAML::Node& node, const std::string& key,
                                          const std::string& defaultValue);
    void addToTree(MaterialEntry* model);
    void showYaml(const YAML::Node& yaml) const;
    void dereference(MaterialEntry* parent, const MaterialEntry* child);
    void dereference(MaterialEntry* model);
    MaterialEntry *getMaterialFromPath(const MaterialLibrary &library, const std::string &path) const;
    void addLibrary(MaterialLibrary* model);
    void loadLibrary(const MaterialLibrary &library);
    void loadLibraries(void);
    static std::map<std::string, MaterialEntry*> *_MaterialEntryMap;
    std::map<std::string, Material*> *_modelMap;
    std::list<MaterialLibrary*> *_libraryList;
};

} // namespace Materials

#endif // MATERIAL_MATERIALLOADER_H

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

#ifndef MATERIAL_MODEL_H
#define MATERIAL_MODEL_H

#include <QDir>
#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>

namespace fs = boost::filesystem;

namespace Material {

class Model
{
public:
    explicit Model(const std::string &baseName, const std::string &modelName, const QDir &dir, 
        const std::string &modelUuid, const YAML::Node &modelData);
    virtual ~Model();

    const std::string &getBase() const
    {
        return base;
    }
    const std::string &getName() const
    {
        return name;
    }
    const QDir &getDirectory() const
    {
        return directory;
    }
    const std::string &getUUID() const
    {
        return uuid;
    }
    const YAML::Node &getModel() const
    {
        return model;
    }
    YAML::Node *getModelPtr()
    {
        return &model;
    }
    const bool getDereferenced() const
    {
        return dereferenced;
    }
    void markDereferenced()
    {
        dereferenced = true;
    }

private:
    explicit Model();
    std::string base;
    std::string name;
    QDir directory;
    std::string uuid;
    YAML::Node model;
    bool dereferenced;
};

class LibraryEntry
{
public:
    explicit LibraryEntry(const std::string &libraryName, const QDir &dir, const std::string &icon);
    virtual ~LibraryEntry();

    const std::string &getName() const
    {
        return name;
    }
    const QDir &getDirectory() const
    {
        return directory;
    }
    const std::string &getIconPath() const
    {
        return iconPath;
    }

private:
    explicit LibraryEntry();
    std::string name;
    QDir directory;
    std::string iconPath;
};

class MaterialExport ModelManager
{
public:
    static ModelManager *getManager();

private:
    explicit ModelManager();
    virtual ~ModelManager();

    void showYaml(const YAML::Node& yaml) const;
    void dereference(Model* parent, const Model* child);
    void dereference(Model* model);
    Model *getModelFromPath(const std::string &path) const;
    void showLibEntry(const std::string& checkpoint, const QDir &dir, const LibraryEntry& entry) const;
    void addModel(LibraryEntry* model);
    void loadLibrary(const LibraryEntry &library);
    void loadLibraries(void);
    std::list<LibraryEntry*> *getModelLibraries();
    static ModelManager *manager;
    static std::list<LibraryEntry*> *libraries;
    static std::map<std::string, Model*> *modelMap;
};

} // namespace Material

#endif // MATERIAL_MODEL_H

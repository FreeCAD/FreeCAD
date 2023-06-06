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
#include <QString>
#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>

namespace fs = boost::filesystem;

namespace Material {

class Model
{
public:
    explicit Model() {}
    explicit Model(const std::string &baseName, const std::string &modelName, const QDir &dir, 
        const std::string &modelUuid, const YAML::Node &modelData);
    virtual ~Model();

    const std::string &getBase() const
    {
        return _base;
    }
    const std::string &getName() const
    {
        return _name;
    }
    const QDir &getDirectory() const
    {
        return _directory;
    }
    const std::string &getUUID() const
    {
        return _uuid;
    }
    const YAML::Node &getModel() const
    {
        return _model;
    }
    YAML::Node *getModelPtr()
    {
        return &_model;
    }
    const bool getDereferenced() const
    {
        return _dereferenced;
    }
    void markDereferenced()
    {
        _dereferenced = true;
    }

    void setBase(const std::string& base)
    {
        _base = base;
    }
    void setName(const std::string& name)
    {
        _name = name;
    }
    void setDirectory(const std::string& directory)
    {
        _directory = QDir(QString::fromStdString(directory));
    }
    void setUUID(const std::string& uuid)
    {
        _uuid = uuid;
    }

    bool operator==(const Model& m) const
    {
        return _uuid == m._uuid;
    }
    bool operator!=(const Model& m) const
    {
        return !operator==(m);
    }

private:
    std::string _base;
    std::string _name;
    QDir _directory;
    std::string _uuid;
    YAML::Node _model;
    bool _dereferenced;
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

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

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}

class MaterialEntry
{
public:
    explicit MaterialEntry();
    explicit MaterialEntry(const MaterialLibrary &library, const std::string &modelName, const QDir &dir, 
        const std::string &modelUuid);
    virtual ~MaterialEntry();

    virtual void addToTree(std::map<std::string, Material*> *materialMap, std::map<std::string, Material*> *_materialPathMap) = 0;

    const MaterialLibrary &getLibrary() const { return _library; }
    const std::string &getName() const { return _name; }
    const QDir &getDirectory() const { return _directory; }
    const std::string &getUUID() const { return _uuid; }
    bool getDereferenced() const { return _dereferenced; }

    void markDereferenced() { _dereferenced = true; }

protected:
    MaterialLibrary _library;
    std::string _name;
    QDir _directory;
    std::string _uuid;
    bool _dereferenced;
};

class MaterialYamlEntry : public MaterialEntry
{
public:
    explicit MaterialYamlEntry(const MaterialLibrary &library, const std::string &modelName, const QDir &dir, 
        const std::string &modelUuid, const YAML::Node &modelData);
    ~MaterialYamlEntry() override;

    void addToTree(std::map<std::string, Material*> *materialMap, std::map<std::string, Material*> *_materialPathMap) override;

    const YAML::Node &getModel() const { return _model; }
    YAML::Node *getModelPtr() { return &_model; }

private:
    explicit MaterialYamlEntry();

    std::string yamlValue(const YAML::Node& node, const std::string& key,
                                          const std::string& defaultValue);

    YAML::Node _model;
};

class MaterialLoader
{
public:
    explicit MaterialLoader(std::map<std::string, Material*> *materialMap, std::map<std::string, Material*> *materialPathMap, std::list<MaterialLibrary*> *libraryList);
    virtual ~MaterialLoader();

    std::list<MaterialLibrary*>* getMaterialLibraries();
    static void showYaml(const YAML::Node& yaml);

private:
    explicit MaterialLoader();

    void addToTree(MaterialEntry* model);
    void dereference(MaterialEntry* parent, const MaterialEntry* child);
    void dereference(MaterialEntry* model);
    MaterialEntry *getMaterialFromPath(const MaterialLibrary &library, const std::string &path) const;
    void addLibrary(MaterialLibrary* model);
    void loadLibrary(const MaterialLibrary &library);
    void loadLibraries(void);
    static std::map<std::string, MaterialEntry*> *_materialEntryMap;
    std::map<std::string, Material*> *_materialMap;
    std::map<std::string, Material*> *_materialPathMap;
    std::list<MaterialLibrary*> *_libraryList;
};

} // namespace Materials

#endif // MATERIAL_MATERIALLOADER_H

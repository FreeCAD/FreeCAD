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

#include <Base/BaseClass.h>
#include <QDir>
#include <QString>

namespace Material {

class MaterialExport ModelLibrary : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    explicit ModelLibrary();
    explicit ModelLibrary(const std::string &libraryName, const QDir &dir, const std::string &icon);
    virtual ~ModelLibrary();

    const std::string &getName() const { return _name; }
    const QDir &getDirectory() const { return _directory; }
    const std::string getDirectoryPath() const { return _directory.absolutePath().toStdString(); }
    const std::string &getIconPath() const { return _iconPath; }

private:
    std::string _name;
    QDir _directory;
    std::string _iconPath;
};

class MaterialExport ModelProperty : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    explicit ModelProperty();
    explicit ModelProperty(const std::string& name, const std::string& type,
                           const std::string& units, const std::string& url,
                           const std::string& description);
    virtual ~ModelProperty();

    const std::string &getName() const {  return _name; }
    const std::string &getType() const {  return _type; }
    const std::string &getUnits() const {  return _units; }
    const std::string &getURL() const {  return _url; }
    const std::string &getDescription() const {  return _description; }

    void setName(const std::string& name) { _name = name; }
    void setType(const std::string& type) { _type = type; }
    void setUnits(const std::string& units) { _units = units; }
    void setURL(const std::string& url) { _url = url; }
    void setDescription(const std::string& description) { _description = description; }

private:
    std::string _name;
    std::string _type;
    std::string _units;
    std::string _url;
    std::string _description;
};

class MaterialExport Model : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    enum ModelType {
        MODEL,
        APPEARANCE_MODEL
    };

    explicit Model();
    explicit Model(const ModelLibrary &library, ModelType type, const std::string &name, const QDir &directory, 
        const std::string &uuid, const std::string& description, const std::string& url,
        const std::string& doi);
    virtual ~Model();

    const ModelLibrary &getLibrary() const { return _library; }
    const std::string getBase() const { return (_type == MODEL) ? "Model" : "AppearanceModel"; }
    const std::string &getName() const { return _name; }
    ModelType getType() const { return _type; }
    const QDir &getDirectory() const { return _directory; }
    const std::string getDirectoryPath() const { return _directory.absolutePath().toStdString(); }
    const std::string getRelativePath() const { return _library.getDirectory().relativeFilePath(_directory.absolutePath()).toStdString(); }
    const std::string &getUUID() const { return _uuid; }
    const std::string &getDescription() const { return _description; }
    const std::string &getURL() const { return _url; }
    const std::string &getDOI() const { return _doi; }

    void setLibrary(const ModelLibrary &library) { _library = library; }
    void setType(ModelType type) { _type = type; }
    void setName(const std::string& name) { _name = name; }
    void setDirectory(const std::string& directory) { _directory = QDir(QString::fromStdString(directory)); }
    void setUUID(const std::string& uuid) { _uuid = uuid; }
    void setDescription(const std::string& description) { _description = description; }
    void setURL(const std::string& url) { _url = url; }
    void setDOI(const std::string& doi) { _doi = doi; }

    void addInheritance(const std::string &uuid) { _inheritedUuids.push_back(uuid); }
    const std::vector<std::string> &getInheritance() const { return _inheritedUuids; }

    bool operator==(const Model& m) const
    {
        return _uuid == m._uuid;
    }
    bool operator!=(const Model& m) const
    {
        return !operator==(m);
    }

    ModelProperty& operator[] (const std::string& key) { return _properties.at(key); } // Throw exception if not found
    void addProperty(ModelProperty &property) { _properties[property.getName()] = property; }

    using iterator=typename std::map<std::string, ModelProperty>::iterator;
    using const_iterator=typename std::map<std::string, ModelProperty>::const_iterator;
    iterator begin() { return _properties.begin(); }
    const_iterator begin() const noexcept { return _properties.begin(); }
    iterator end() noexcept { return _properties.end(); }
    const_iterator end() const noexcept { return _properties.end(); }
    const_iterator cbegin() const noexcept { return _properties.cbegin(); }
    const_iterator cend() const noexcept { return _properties.cend(); }

private:
    ModelLibrary _library;
    ModelType _type;
    std::string _name;
    QDir _directory;
    std::string _uuid;
    std::string _description;
    std::string _url;
    std::string _doi;
    std::vector<std::string> _inheritedUuids;
    std::map<std::string, ModelProperty> _properties;
};

} // namespace Material

#endif // MATERIAL_MODEL_H

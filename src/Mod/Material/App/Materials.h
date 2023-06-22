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

#ifndef MATERIAL_MATERIALS_H
#define MATERIAL_MATERIALS_H

#include <Base/BaseClass.h>
#include <QDir>
#include <QString>

#include "Model.h"
#include "MaterialLibrary.h"

namespace fs = boost::filesystem;

namespace Materials {

class MaterialsExport ModelData : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    explicit ModelData();
    explicit ModelData(const std::string& name, const std::string& type,
                           const std::string& units, const std::string& url,
                           const std::string& description);
    virtual ~ModelData();

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

class MaterialsExport Material : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    explicit Material();
    explicit Material(const MaterialLibrary &library, const std::string& directory,
                      const std::string& uuid, const std::string& name);
    explicit Material(const MaterialLibrary &library, const QDir& directory,
                      const std::string& uuid, const std::string& name);
    virtual ~Material();

    const MaterialLibrary &getLibrary() const { return _library; }
    const QDir &getDirectory() const { return _directory; }
    const std::string getDirectoryPath() const { return _directory.absolutePath().toStdString(); }
    const std::string getRelativePath() const { return _library.getDirectory().relativeFilePath(_directory.absolutePath()).toStdString(); }
    const std::string &getUUID() const { return _uuid; }
    const std::string &getVersion() const { return _version; }
    const std::string &getName() const { return _name; }
    const std::string &getAuthorAndLicense() const { return _authorAndLicense; }
    const std::string &getParentUUID() const { return _parentUuid; }
    const std::string &getDescription() const { return _description; }
    const std::string &getURL() const { return _url; }
    const std::string &getReference() const { return _reference; }
    const std::list<std::string> &getTags() const { return _tags; }
    const std::vector<std::string> *getPhysicalModels() const { return &_physicalUuids; }
    const std::vector<std::string> *getAppearanceModels() const { return &_appearanceUuids; }

    void setLibrary(const MaterialLibrary &library) { _library = library; }
    void setDirectory(const std::string& directory) { _directory = QDir(QString::fromStdString(directory)); }
    void setDirectory(const QDir &directory) { _directory = directory; }
    void setUUID(const std::string& uuid) { _uuid = uuid; }
    void setVersion(const std::string& uuid) { _version = uuid; }
    void setName(const std::string& name) { _name = name; }
    void setAuthorAndLicense(const std::string& authorAndLicense) { _authorAndLicense = authorAndLicense; }
    void setParentUUID(const std::string& uuid) { _parentUuid = uuid; }
    void setDescription(const std::string& description) { _description = description; }
    void setURL(const std::string& url) { _url = url; }
    void setReference(const std::string& reference) { _reference = reference; }
    void addTag(const std::string& tag) { Q_UNUSED(tag); }
    void removeTag(const std::string& tag) { Q_UNUSED(tag); }
    void addPhysical(const std::string& uuid);
    void addAppearance(const std::string& uuid);
    
    void setPhysicalValue(const std::string& name, const std::string &value);
    void setPhysicalValue(const QString& name, const QString &value)
    {
        setPhysicalValue(name.toStdString(), value.toStdString());
    }
    void setPhysicalValue(const std::string& name, int value);
    void setPhysicalValue(const std::string& name, double value);
    void setPhysicalValue(const std::string& name, const Base::Quantity value);

    void setAppearanceValue(const std::string& name, const std::string &value);
    void setAppearanceValue(const QString& name, const QString &value)
    {
        setAppearanceValue(name.toStdString(), value.toStdString());
    }

    const std::string getPhysicalValue(const std::string &name) const;
    const std::string getAppearanceValue(const std::string &name) const;
    bool hasPhysicalProperty(const std::string& name) const;
    bool hasPhysicalProperty(const QString& name) const { return hasPhysicalProperty(name.toStdString()); }
    bool hasAppearanceProperty(const std::string& name) const;
    bool hasAppearanceProperty(const QString& name) const { return hasAppearanceProperty(name.toStdString()); }
    bool hasPhysicalModel(const std::string& uuid) const;
    bool hasAppearanceModel(const std::string& uuid) const;

private:
    MaterialLibrary _library;
    QDir _directory;
    std::string _uuid;
    std::string _version;
    std::string _name;
    std::string _authorAndLicense;
    std::string _parentUuid;
    std::string _description;
    std::string _url;
    std::string _reference;
    std::list<std::string> _tags;
    std::vector<std::string> _physicalUuids;
    std::vector<std::string> _appearanceUuids;
    std::map<std::string, ModelValueProperty> _physical;
    std::map<std::string, ModelValueProperty> _appearance;

};

} // namespace Materials

#endif // MATERIAL_MATERIALS_H

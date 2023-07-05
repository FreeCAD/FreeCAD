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
    explicit ModelData(const QString& name, const QString& type,
                           const QString& units, const QString& url,
                           const QString& description);
    virtual ~ModelData();

    const QString &getName() const {  return _name; }
    const QString &getType() const {  return _type; }
    const QString &getUnits() const {  return _units; }
    const QString &getURL() const {  return _url; }
    const QString &getDescription() const {  return _description; }

    void setName(const QString& name) { _name = name; }
    void setType(const QString& type) { _type = type; }
    void setUnits(const QString& units) { _units = units; }
    void setURL(const QString& url) { _url = url; }
    void setDescription(const QString& description) { _description = description; }

private:
    QString _name;
    QString _type;
    QString _units;
    QString _url;
    QString _description;
};

class MaterialsExport Material : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    explicit Material();
    explicit Material(const MaterialLibrary &library, const QString& directory,
                      const QString& uuid, const QString& name);
    explicit Material(const MaterialLibrary &library, const QDir& directory,
                      const QString& uuid, const QString& name);
    virtual ~Material();

    const MaterialLibrary &getLibrary() const { return _library; }
    const QDir &getDirectory() const { return _directory; }
    const QString getDirectoryPath() const { return _directory.absolutePath(); }
    const QString getRelativePath() const { return _library.getDirectory().relativeFilePath(_directory.absolutePath()); }
    const QString &getUUID() const { return _uuid; }
    const QString &getVersion() const { return _version; }
    const QString &getName() const { return _name; }
    const QString &getAuthorAndLicense() const { return _authorAndLicense; }
    const QString &getParentUUID() const { return _parentUuid; }
    const QString &getDescription() const { return _description; }
    const QString &getURL() const { return _url; }
    const QString &getReference() const { return _reference; }
    const std::list<QString> &getTags() const { return _tags; }
    const std::vector<QString> *getPhysicalModels() const { return &_physicalUuids; }
    const std::vector<QString> *getAppearanceModels() const { return &_appearanceUuids; }

    void setLibrary(const MaterialLibrary &library) { _library = library; }
    void setDirectory(const QString& directory) { _directory = QDir(directory); }
    void setDirectory(const QDir &directory) { _directory = directory; }
    void setUUID(const QString& uuid) { _uuid = uuid; }
    void setVersion(const QString& uuid) { _version = uuid; }
    void setName(const QString& name) { _name = name; }
    void setAuthorAndLicense(const QString& authorAndLicense) { _authorAndLicense = authorAndLicense; }
    void setParentUUID(const QString& uuid) { _parentUuid = uuid; }
    void setDescription(const QString& description) { _description = description; }
    void setURL(const QString& url) { _url = url; }
    void setReference(const QString& reference) { _reference = reference; }
    void addTag(const QString& tag) { Q_UNUSED(tag); }
    void removeTag(const QString& tag) { Q_UNUSED(tag); }
    void addPhysical(const QString& uuid);
    void addAppearance(const QString& uuid);
    
    void setPhysicalValue(const QString& name, const QString &value);
    void setPhysicalValue(const QString& name, int value);
    void setPhysicalValue(const QString& name, double value);
    void setPhysicalValue(const QString& name, const Base::Quantity value);

    void setAppearanceValue(const QString& name, const QString &value);

    ModelValueProperty &getPhysicalProperty(const QString &name);
    ModelValueProperty &getAppearanceProperty(const QString &name);
    const QString getPhysicalValue(const QString &name) const;
    const QString getAppearanceValue(const QString &name) const;
    bool hasPhysicalProperty(const QString& name) const;
    bool hasAppearanceProperty(const QString& name) const;
    bool hasPhysicalModel(const QString& uuid) const;
    bool hasAppearanceModel(const QString& uuid) const;

private:
    MaterialLibrary _library;
    QDir _directory;
    QString _uuid;
    QString _version;
    QString _name;
    QString _authorAndLicense;
    QString _parentUuid;
    QString _description;
    QString _url;
    QString _reference;
    std::list<QString> _tags;
    std::vector<QString> _physicalUuids;
    std::vector<QString> _appearanceUuids;
    std::map<QString, ModelValueProperty> _physical;
    std::map<QString, ModelValueProperty> _appearance;

};

} // namespace Materials

Q_DECLARE_METATYPE(Materials::Material *)

#endif // MATERIAL_MATERIALS_H

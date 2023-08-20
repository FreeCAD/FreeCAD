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

class MaterialsExport MaterialProperty : public ModelProperty
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit MaterialProperty();
    explicit MaterialProperty(const ModelProperty &property);
    virtual ~MaterialProperty();

    MaterialValue::ValueType getType(void) const { return _valuePtr->getType(); }

    const QString getModelUUID(void) const;
    const QVariant getValue(void) const;
    bool isNull() const { return _valuePtr->isNull(); }
    MaterialValue* getMaterialValue(void);
    const QString getString(void) const;
    bool getBoolean(void) const;
    int getInt(void) const;
    double getFloat(void) const;
    const Base::Quantity& getQuantity(void) const;
    const QString getURL(void) const;

    MaterialProperty &getColumn(int column);
    MaterialValue::ValueType getColumnType(int column) const;
    QString getColumnUnits(int column) const;
    QVariant getColumnNull(int column) const;

    void setModelUUID(const QString& uuid);
    void setPropertyType(const QString& type) override;
    void setValue(const QVariant& value);
    void setValue(const QString& value);
    void setString(const QString& value);
    void setBoolean(bool value);
    void setBoolean(int value);
    void setBoolean(const QString& value);
    void setInt(int value);
    void setInt(const QString& value);
    void setFloat(double value);
    void setFloat(const QString& value);
    void setQuantity(const Base::Quantity& value);
    void setQuantity(double value, const QString& units);
    void setQuantity(const QString& value);
    void setURL(const QString& value);

protected:
    void setType(const QString& type);
    // void setType(MaterialValue::ValueType type) { _valueType = type; }

    void addColumn(MaterialProperty &column) { _columns.push_back(column); }

private:
    QString _modelUUID;
    MaterialValue* _valuePtr;
    std::vector<MaterialProperty> _columns;
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
    const QString getUUID() const { return _uuid; }
    const QString getName() const { return _name; }
    const QString getAuthorAndLicense() const { return _authorAndLicense; }
    const QString getParentUUID() const { return _parentUuid; }
    const QString getDescription() const { return _description; }
    const QString getURL() const { return _url; }
    const QString getReference() const { return _reference; }
    const std::list<QString> &getTags() const { return _tags; }
    const std::vector<QString> *getPhysicalModels() const { return &_physicalUuids; }
    const std::vector<QString> *getAppearanceModels() const { return &_appearanceUuids; }

    void setLibrary(const MaterialLibrary &library) { _library = library; }
    void setDirectory(const QString& directory) { _directory = QDir(directory); }
    void setDirectory(const QDir &directory) { _directory = directory; }
    void setUUID(const QString& uuid) { _uuid = uuid; }
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

    MaterialProperty &getPhysicalProperty(const QString &name);
    const MaterialProperty &getPhysicalProperty(const QString &name) const;
    MaterialProperty &getAppearanceProperty(const QString &name);
    const MaterialProperty &getAppearanceProperty(const QString &name) const;
    const QString getPhysicalValue(const QString &name) const;
    const QString getAppearanceValue(const QString &name) const;
    bool hasPhysicalProperty(const QString& name) const;
    bool hasAppearanceProperty(const QString& name) const;

    // Test if the model is defined, and if values are provided for all properties
    bool hasModel(const QString& uuid) const;
    bool hasPhysicalModel(const QString& uuid) const;
    bool hasAppearanceModel(const QString& uuid) const;
    bool isModelComplete(const QString& uuid) const { return isPhysicalModelComplete(uuid) || isAppearanceModelComplete(uuid); }
    bool isPhysicalModelComplete(const QString& uuid) const;
    bool isAppearanceModelComplete(const QString& uuid) const;

    const std::map<QString, MaterialProperty> &getPhysicalProperties() const { return _physical; }
    const std::map<QString, MaterialProperty> &getAppearanceProperties() const { return _appearance; }

    bool getDereferenced() const { return _dereferenced; }
    void markDereferenced() { _dereferenced = true; }

protected:
    void addModel(const QString& uuid);

private:
    MaterialLibrary _library;
    QDir _directory;
    QString _uuid;
    QString _name;
    QString _authorAndLicense;
    QString _parentUuid;
    QString _description;
    QString _url;
    QString _reference;
    std::list<QString> _tags;
    std::vector<QString> _physicalUuids;
    std::vector<QString> _appearanceUuids;
    std::vector<QString> _allUuids; // Includes inherited models
    std::map<QString, MaterialProperty> _physical;
    std::map<QString, MaterialProperty> _appearance;
    bool _dereferenced;
};

} // namespace Materials

Q_DECLARE_METATYPE(Materials::Material *)

#endif // MATERIAL_MATERIALS_H

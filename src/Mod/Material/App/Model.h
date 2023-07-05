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
#include <Base/Quantity.h>
#include <QDir>
#include <QString>

namespace Materials {

class MaterialsExport ModelLibrary : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    explicit ModelLibrary();
    explicit ModelLibrary(const QString &libraryName, const QDir &dir, const QString &icon);
    virtual ~ModelLibrary();

    const QString &getName() const { return _name; }
    const QDir &getDirectory() const { return _directory; }
    const QString getDirectoryPath() const { return _directory.absolutePath(); }
    const QString &getIconPath() const { return _iconPath; }
    bool operator==(const ModelLibrary& library) const
    {
        return (_name == library._name) && (_directory == library._directory);
    }
    bool operator!=(const ModelLibrary& library) const { return !operator==(library); }

private:
    QString _name;
    QDir _directory;
    QString _iconPath;
};

class MaterialsExport ModelProperty : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    explicit ModelProperty();
    explicit ModelProperty(const QString& name, const QString& type,
                           const QString& units, const QString& url,
                           const QString& description);
    virtual ~ModelProperty();

    const QString &getName() const {  return _name; }
    const QString &getPropertyType() const {  return _propertyType; }
    const QString &getUnits() const {  return _units; }
    const QString &getURL() const {  return _url; }
    const QString &getDescription() const {  return _description; }
    const QString &getInheritance() const { return _inheritance; }
    bool isInherited() const { return (_inheritance.length() > 0); }

    void setName(const QString& name) { _name = name; }
    virtual void setPropertyType(const QString& type) { _propertyType = type; }
    void setUnits(const QString& units) { _units = units; }
    void setURL(const QString& url) { _url = url; }
    void setDescription(const QString& description) { _description = description; }
    void setInheritance(const QString &uuid) { _inheritance = uuid; }

    void addColumn(ModelProperty &column) { _columns.push_back(column); }
    const std::vector<ModelProperty> &columns() const { return _columns; }

private:
    QString _name;
    QString _propertyType;
    QString _units;
    QString _url;
    QString _description;
    QString _inheritance;
    std::vector<ModelProperty> _columns;
};

class MaterialsExport ModelValueProperty : public ModelProperty
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit ModelValueProperty();
    explicit ModelValueProperty(const ModelProperty &property);
    virtual ~ModelValueProperty();

    enum ValueType {
        None = 0,
        String = 1,
        Boolean = 2,
        Int = 3,
        Float = 4,
        Quantity = 5,
        Distribution = 6,
        List = 7,
        Array2D = 8,
        Array3D = 9,
        Color = 10,
        Image = 11,
        File = 12,
        URL = 13
    };

    ValueType getType(void) const { return _valueType; }

    const QString& getModelUUID(void) const;
    const QString getValue(void) const;
    const QString& getString(void) const;
    bool getBoolean(void) const;
    int getInt(void) const;
    double getFloat(void) const;
    const Base::Quantity& getQuantity(void) const;
    const QString& getURL(void) const;

    ModelValueProperty &getColumn(int column);

    void setModelUUID(const QString& uuid);
    void setPropertyType(const QString& type) override;
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
    void setType(ValueType type) { _valueType = type; }

    void addColumn(ModelValueProperty &column) { _columns.push_back(column); }

private:
    QString _modelUUID;
    ValueType _valueType;
    QString _valueString;
    bool _valueBoolean;
    int _valueInt;
    double _valueFloat;
    Base::Quantity _valueQuantity;
    std::vector<ModelValueProperty> _columns;
};

class MaterialsExport Model : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    enum ModelType {
        ModelType_Physical,
        ModelType_Appearance
    };

    explicit Model();
    explicit Model(const ModelLibrary &library, ModelType type, const QString &name, const QDir &directory, 
        const QString &uuid, const QString& description, const QString& url,
        const QString& doi);
    virtual ~Model();

    const ModelLibrary &getLibrary() const { return _library; }
    const QString getBase() const { return (_type == ModelType_Physical) ? QString::fromStdString("Model") : QString::fromStdString("AppearanceModel"); }
    const QString &getName() const { return _name; }
    ModelType getType() const { return _type; }
    const QDir &getDirectory() const { return _directory; }
    const QString getDirectoryPath() const { return _directory.absolutePath(); }
    const QString getRelativePath() const { return _library.getDirectory().relativeFilePath(_directory.absolutePath()); }
    const QString &getUUID() const { return _uuid; }
    const QString &getDescription() const { return _description; }
    const QString &getURL() const { return _url; }
    const QString &getDOI() const { return _doi; }

    void setLibrary(const ModelLibrary &library) { _library = library; }
    void setType(ModelType type) { _type = type; }
    void setName(const QString& name) { _name = name; }
    void setDirectory(const QString& directory) { _directory = QDir(directory); }
    void setUUID(const QString& uuid) { _uuid = uuid; }
    void setDescription(const QString& description) { _description = description; }
    void setURL(const QString& url) { _url = url; }
    void setDOI(const QString& doi) { _doi = doi; }

    void addInheritance(const QString &uuid) { _inheritedUuids.push_back(uuid); }
    const std::vector<QString> &getInheritance() const { return _inheritedUuids; }

    bool operator==(const Model& m) const
    {
        return _uuid == m._uuid;
    }
    bool operator!=(const Model& m) const { return !operator==(m); }

    ModelProperty& operator[](const QString& key);
    void addProperty(ModelProperty &property) { _properties[property.getName()] = property; }

    using iterator=typename std::map<QString, ModelProperty>::iterator;
    using const_iterator=typename std::map<QString, ModelProperty>::const_iterator;
    iterator begin() { return _properties.begin(); }
    const_iterator begin() const noexcept { return _properties.begin(); }
    iterator end() noexcept { return _properties.end(); }
    const_iterator end() const noexcept { return _properties.end(); }
    const_iterator cbegin() const noexcept { return _properties.cbegin(); }
    const_iterator cend() const noexcept { return _properties.cend(); }

private:
    ModelLibrary _library;
    ModelType _type;
    QString _name;
    QDir _directory;
    QString _uuid;
    QString _description;
    QString _url;
    QString _doi;
    std::vector<QString> _inheritedUuids;
    std::map<QString, ModelProperty> _properties;
};

} // namespace Materials

#endif // MATERIAL_MODEL_H

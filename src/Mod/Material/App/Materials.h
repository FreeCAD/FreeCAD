/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef MATERIAL_MATERIALS_H
#define MATERIAL_MATERIALS_H

#include <Mod/Material/MaterialGlobal.h>

#include <Base/BaseClass.h>
#include <QDir>
#include <QString>
#include <QTextStream>

#include <App/Application.h>

#include "MaterialLibrary.h"
#include "Model.h"

namespace fs = boost::filesystem;

namespace Materials
{

class MaterialsExport MaterialProperty: public ModelProperty
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialProperty();
    explicit MaterialProperty(const ModelProperty& property);
    explicit MaterialProperty(const MaterialProperty& property);
    ~MaterialProperty() override = default;

    MaterialValue::ValueType getType() const
    {
        return _valuePtr->getType();
    }

    const QString getModelUUID() const;
    const QVariant getValue() const;
    bool isNull() const
    {
        return _valuePtr->isNull();
    }
    std::shared_ptr<MaterialValue> getMaterialValue();
    const std::shared_ptr<MaterialValue> getMaterialValue() const;
    const QString getString() const;
    bool getBoolean() const;
    int getInt() const;
    double getFloat() const;
    const Base::Quantity& getQuantity() const;
    const QString getURL() const;

    MaterialProperty& getColumn(int column);
    const MaterialProperty& getColumn(int column) const;
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

    MaterialProperty& operator=(const MaterialProperty& other);

protected:
    void setType(const QString& type);
    // void setType(MaterialValue::ValueType type) { _valueType = type; }

    void addColumn(MaterialProperty& column)
    {
        _columns.push_back(column);
    }

private:
    QString _modelUUID;
    std::shared_ptr<MaterialValue> _valuePtr;
    std::vector<MaterialProperty> _columns;
};

class MaterialsExport Material: public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    enum ModelEdit
    {
        ModelEdit_None,   // No change
        ModelEdit_Alter,  // Existing values are changed
        ModelEdit_Extend  // New values added
    };

    Material();
    explicit Material(const MaterialLibrary& library,
                      const QString& directory,
                      const QString& uuid,
                      const QString& name);
    explicit Material(const Material& other);
    virtual ~Material();

    const MaterialLibrary& getLibrary() const
    {
        return _library;
    }
    const QString getDirectory() const
    {
        return _directory;
    }
    const QString getUUID() const
    {
        return _uuid;
    }
    const QString getName() const
    {
        return _name;
    }
    const QString getAuthorAndLicense() const
    {
        return _authorAndLicense;
    }
    const QString getParentUUID() const
    {
        return _parentUuid;
    }
    const QString getDescription() const
    {
        return _description;
    }
    const QString getURL() const
    {
        return _url;
    }
    const QString getReference() const
    {
        return _reference;
    }
    ModelEdit getEditState() const
    {
        return _editState;
    }
    const std::list<QString>& getTags() const
    {
        return _tags;
    }
    const std::vector<QString>* getPhysicalModels() const
    {
        return &_physicalUuids;
    }
    const std::vector<QString>* getAppearanceModels() const
    {
        return &_appearanceUuids;
    }

    void setLibrary(const MaterialLibrary& library)
    {
        _library = library;
    }
    void setDirectory(const QString& directory)
    {
        Base::Console().Log("Materials::setDirectory(%s)\n", directory.toStdString().c_str());
        _directory = directory;
    }
    void setUUID(const QString& uuid)
    {
        _uuid = uuid;
    }
    void setName(const QString& name)
    {
        _name = name;
    }
    void setAuthorAndLicense(const QString& authorAndLicense)
    {
        _authorAndLicense = authorAndLicense;
    }
    void setParentUUID(const QString& uuid)
    {
        _parentUuid = uuid;
    }
    void setDescription(const QString& description)
    {
        _description = description;
    }
    void setURL(const QString& url)
    {
        _url = url;
    }
    void setReference(const QString& reference)
    {
        _reference = reference;
    }
    void setEditState(ModelEdit newState);
    void setEditStateAlter()
    {
        setEditState(ModelEdit_Alter);
    }
    void setEditStateExtend()
    {
        setEditState(ModelEdit_Extend);
    }
    void setPhysicalEditState(const QString& name);
    void setAppearanceEditState(const QString& name);
    void resetEditState()
    {
        _editState = ModelEdit_None;
    }
    void addTag(const QString& tag)
    {
        Q_UNUSED(tag);
    }
    void removeTag(const QString& tag)
    {
        Q_UNUSED(tag);
    }
    void addPhysical(const QString& uuid);
    void addAppearance(const QString& uuid);
    void newUuid();

    void setPhysicalValue(const QString& name, const QString& value);
    void setPhysicalValue(const QString& name, int value);
    void setPhysicalValue(const QString& name, double value);
    void setPhysicalValue(const QString& name, const Base::Quantity value);

    void setAppearanceValue(const QString& name, const QString& value);

    MaterialProperty& getPhysicalProperty(const QString& name);
    const MaterialProperty& getPhysicalProperty(const QString& name) const;
    MaterialProperty& getAppearanceProperty(const QString& name);
    const MaterialProperty& getAppearanceProperty(const QString& name) const;
    const QVariant getPhysicalValue(const QString& name) const;
    const QString getPhysicalValueString(const QString& name) const;
    const QVariant getAppearanceValue(const QString& name) const;
    const QString getAppearanceValueString(const QString& name) const;
    bool hasPhysicalProperty(const QString& name) const;
    bool hasAppearanceProperty(const QString& name) const;

    // Test if the model is defined, and if values are provided for all properties
    bool hasModel(const QString& uuid) const;
    bool hasPhysicalModel(const QString& uuid) const;
    bool hasAppearanceModel(const QString& uuid) const;
    bool isModelComplete(const QString& uuid) const
    {
        return isPhysicalModelComplete(uuid) || isAppearanceModelComplete(uuid);
    }
    bool isPhysicalModelComplete(const QString& uuid) const;
    bool isAppearanceModelComplete(const QString& uuid) const;

    const std::map<QString, MaterialProperty>& getPhysicalProperties() const
    {
        return _physical;
    }
    const std::map<QString, MaterialProperty>& getAppearanceProperties() const
    {
        return _appearance;
    }

    bool getDereferenced() const
    {
        return _dereferenced;
    }
    void markDereferenced()
    {
        _dereferenced = true;
    }

    void save(QTextStream& stream, bool saveAsCopy);

    Material& operator=(const Material& other);

protected:
    void addModel(const QString& uuid);

    const QVariant getValue(const std::map<QString, MaterialProperty>& propertyList,
                            const QString& name) const;
    const QString getValueString(const std::map<QString, MaterialProperty>& propertyList,
                                 const QString& name) const;

    void saveGeneral(QTextStream& stream) const;
    void saveInherits(QTextStream& stream) const;
    void saveModels(QTextStream& stream) const;
    void saveAppearanceModels(QTextStream& stream) const;

private:
    MaterialLibrary _library;
    QString _directory;
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
    std::vector<QString> _allUuids;  // Includes inherited models
    std::map<QString, MaterialProperty> _physical;
    std::map<QString, MaterialProperty> _appearance;
    bool _dereferenced;
    ModelEdit _editState;
};

}  // namespace Materials

Q_DECLARE_METATYPE(Materials::Material*)

#endif  // MATERIAL_MATERIALS_H

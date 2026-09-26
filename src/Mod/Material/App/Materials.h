// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <QTextStream>

#include <App/Application.h>
#include <Base/Color.h>
#include <App/Material.h>
#include <Base/BaseClass.h>

#include <Mod/Material/MaterialGlobal.h>

#include "MaterialValue.h"
#include "Model.h"

namespace Materials
{

class MaterialLibrary;

class MaterialsExport MaterialProperty: public ModelProperty
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialProperty();
    MaterialProperty(const MaterialProperty& other);
    explicit MaterialProperty(const ModelProperty& other, std::string modelUUID);
    explicit MaterialProperty(const std::shared_ptr<MaterialProperty>& other);
    ~MaterialProperty() override = default;

    MaterialValue::ValueType getType() const
    {
        return _valuePtr->getType();
    }

    const std::string& getModelUUID() const
    {
        return _modelUUID;
    }

    QVariant getValue();
    QVariant getValue() const;
    QList<QVariant> getList()
    {
        return _valuePtr->getList();
    }
    QList<QVariant> getList() const
    {
        return _valuePtr->getList();
    }
    bool isNull() const
    {
        return _valuePtr->isNull();
    }
    bool isEmpty() const
    {
        return _valuePtr->isEmpty();
    }
    std::shared_ptr<MaterialValue> getMaterialValue();
    std::shared_ptr<MaterialValue> getMaterialValue() const;
    std::string getString() const;
    std::string getYAMLString() const;
    std::string getDictionaryString() const;  // Non-localized string
    bool getBoolean() const
    {
        return getValue().toBool();
    }
    int getInt() const
    {
        return getValue().toInt();
    }
    double getFloat() const
    {
        return getValue().toFloat();
    }
    const Base::Quantity& getQuantity() const;
    std::string getURL() const
    {
        return getValue().toString().toStdString();
    }
    Base::Color getColor() const;

    MaterialProperty& getColumn(int column);
    const MaterialProperty& getColumn(int column) const;
    MaterialValue::ValueType getColumnType(int column) const;
    std::string getColumnUnits(int column) const;
    QVariant getColumnNull(int column) const;
    const std::vector<MaterialProperty>& getColumns() const
    {
        return _columns;
    }

    void setModelUUID(std::string uuid);
    void setPropertyType(std::string type) override;
    void setValue(const QVariant& value);
    void setValue(const std::string& value);
    void setValue(const std::shared_ptr<MaterialValue>& value);
    void setString(const std::string& value);
    void setBoolean(bool value);
    void setBoolean(int value);
    void setBoolean(const std::string& value);
    void setInt(int value);
    void setInt(const std::string& value);
    void setFloat(double value);
    void setFloat(const std::string& value);
    void setQuantity(const Base::Quantity& value);
    void setQuantity(double value, const std::string& units);
    void setQuantity(const std::string& value);
    void setList(const QList<QVariant>& value);
    void setURL(const std::string& value);
    void setColor(const Base::Color& value);

    MaterialProperty& operator=(const MaterialProperty& other);
    friend QTextStream& operator<<(QTextStream& output, const MaterialProperty& property);

    bool operator==(const MaterialProperty& other) const;
    bool operator!=(const MaterialProperty& other) const
    {
        return !operator==(other);
    }

    void validate(const MaterialProperty& other) const;

    // Define precision for displaying floating point values
    static int const PRECISION;

protected:
    void setType(const std::string& type);
    // void setType(MaterialValue::ValueType type) { _valueType = type; }
    void copyValuePtr(const std::shared_ptr<MaterialValue>& value);

    void addColumn(MaterialProperty& column)
    {
        _columns.push_back(column);
    }

private:
    std::string _modelUUID;
    std::shared_ptr<MaterialValue> _valuePtr;
    std::vector<MaterialProperty> _columns;
};

class MaterialsExport Material: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum ModelEdit
    {
        ModelEdit_None,   // No change
        ModelEdit_Alter,  // Existing values are changed
        ModelEdit_Extend  // New values added
    };

    Material();
    Material(const std::shared_ptr<MaterialLibrary>& library,
             std::string directory,
             std::string uuid,
             std::string name);
    Material(const Material& other);
    ~Material() override = default;

    std::shared_ptr<MaterialLibrary> getLibrary() const
    {
        return _library;
    }
    const std::string& getDirectory() const;
    const std::string& getFilename() const;
    std::string getFilePath() const;
    const std::string& getUUID() const
    {
        return _uuid;
    }
    const std::string& getName() const
    {
        return _name;
    }
    std::string getAuthorAndLicense() const;
    const std::string& getAuthor() const
    {
        return _author;
    }
    const std::string& getLicense() const
    {
        return _license;
    }
    const std::string& getParentUUID() const
    {
        return _parentUuid;
    }
    const std::string& getDescription() const
    {
        return _description;
    }
    const std::string& getURL() const
    {
        return _url;
    }
    const std::string& getReference() const
    {
        return _reference;
    }
    ModelEdit getEditState() const
    {
        return _editState;
    }
    const std::set<std::string>& getTags() const
    {
        return _tags;
    }
    const std::set<std::string>* getPhysicalModels() const
    {
        return &_physicalUuids;
    }
    const std::set<std::string>* getAppearanceModels() const
    {
        return &_appearanceUuids;
    }

    App::Material getMaterialAppearance() const;

    void setLibrary(const std::shared_ptr<MaterialLibrary>& library)
    {
        _library = library;
    }
    void setDirectory(std::string directory);
    void setFilename(std::string filename);
    void setUUID(std::string uuid)
    {
        _uuid = std::move(uuid);
    }
    void setName(std::string name);
    void setAuthor(std::string author);
    void setLicense(std::string license);
    void setParentUUID(std::string uuid);
    void setDescription(std::string description);
    void setURL(std::string url);
    void setReference(std::string reference);

    void setEditState(ModelEdit newState);
    void setEditStateAlter()
    {
        setEditState(ModelEdit_Alter);
    }
    void setEditStateExtend()
    {
        setEditState(ModelEdit_Extend);
    }
    void setPropertyEditState(const std::string& name);
    void setPhysicalEditState(const std::string& name);
    void setAppearanceEditState(const std::string& name);
    void resetEditState()
    {
        _editState = ModelEdit_None;
    }
    void addTag(std::string tag);
    void removeTag(const std::string& tag);
    bool hasTag(const std::string& tag)
    {
        return _tags.contains(tag);
    }
    void addPhysical(const std::string& uuid);
    void removePhysical(const std::string& uuid);
    void addAppearance(const std::string& uuid);
    void removeAppearance(const std::string& uuid);
    void clearModels();
    void clearInherited();
    void newUuid();

    void setPhysicalValue(const std::string& name, const std::string& value);
    void setPhysicalValue(const std::string& name, int value);
    void setPhysicalValue(const std::string& name, double value);
    void setPhysicalValue(const std::string& name, const Base::Quantity& value);
    void setPhysicalValue(const std::string& name, const std::shared_ptr<MaterialValue>& value);
    void setPhysicalValue(const std::string& name, const std::shared_ptr<QList<QVariant>>& value);
    void setPhysicalValue(const std::string& name, const QVariant& value);

    void setAppearanceValue(const std::string& name, const std::string& value);
    void setAppearanceValue(const std::string& name, const std::shared_ptr<MaterialValue>& value);
    void setAppearanceValue(const std::string& name, const std::shared_ptr<QList<QVariant>>& value);
    void setAppearanceValue(const std::string& name, const QVariant& value);

    void setValue(const std::string& name, const std::string& value);
    void setValue(const std::string& name, const QVariant& value);
    void setValue(const std::string& name, const std::shared_ptr<MaterialValue>& value);

    /*
     * Legacy values are thosed contained in old format files that don't fit in the new
     * property format. It should not be used as a catch all for defining a property with
     * no model.
     *
     * These values are transient and will not be saved.
     */
    void setLegacyValue(const std::string& name, const std::string& value);

    std::shared_ptr<MaterialProperty> getPhysicalProperty(const std::string& name);
    std::shared_ptr<MaterialProperty> getPhysicalProperty(const std::string& name) const;
    std::shared_ptr<MaterialProperty> getAppearanceProperty(const std::string& name);
    std::shared_ptr<MaterialProperty> getAppearanceProperty(const std::string& name) const;
    std::shared_ptr<MaterialProperty> getProperty(const std::string& name);
    std::shared_ptr<MaterialProperty> getProperty(const std::string& name) const;
    QVariant getPhysicalValue(const std::string& name) const;
    Base::Quantity getPhysicalQuantity(const std::string& name) const;
    std::string getPhysicalValueString(const std::string& name) const;
    QVariant getAppearanceValue(const std::string& name) const;
    Base::Quantity getAppearanceQuantity(const std::string& name) const;
    std::string getAppearanceValueString(const std::string& name) const;
    bool hasPhysicalProperty(const std::string& name) const;
    bool hasAppearanceProperty(const std::string& name) const;
    bool hasNonLegacyProperty(const std::string& name) const;
    bool hasLegacyProperty(const std::string& name) const;
    bool hasLegacyProperties() const;
    bool hasPhysicalProperties() const;
    bool hasAppearanceProperties() const;

    // Test if the model is defined, and if values are provided for all properties
    bool hasModel(const std::string& uuid) const;
    bool hasPhysicalModel(const std::string& uuid) const;
    bool hasAppearanceModel(const std::string& uuid) const;
    bool isInherited(const std::string& uuid) const;
    bool isModelComplete(const std::string& uuid) const
    {
        return isPhysicalModelComplete(uuid) || isAppearanceModelComplete(uuid);
    }
    bool isPhysicalModelComplete(const std::string& uuid) const;
    bool isAppearanceModelComplete(const std::string& uuid) const;

    std::map<std::string, std::shared_ptr<MaterialProperty>>& getPhysicalProperties()
    {
        return _physical;
    }
    const std::map<std::string, std::shared_ptr<MaterialProperty>>& getPhysicalProperties() const
    {
        return _physical;
    }
    std::map<std::string, std::shared_ptr<MaterialProperty>>& getAppearanceProperties()
    {
        return _appearance;
    }
    const std::map<std::string, std::shared_ptr<MaterialProperty>>& getAppearanceProperties() const
    {
        return _appearance;
    }
    std::map<std::string, std::string>& getLegacyProperties()
    {
        return _legacy;
    }

    std::string getModelByName(const std::string& name) const;

    bool getDereferenced() const
    {
        return _dereferenced;
    }
    void markDereferenced()
    {
        _dereferenced = true;
    }
    void clearDereferenced()
    {
        _dereferenced = false;
    }
    bool isOldFormat() const
    {
        return _oldFormat;
    }
    void setOldFormat(bool isOld)
    {
        _oldFormat = isOld;
    }

    /*
     * Normalize models by removing any inherited models
     */
    static std::vector<std::string> normalizeModels(const std::vector<std::string>& models);

    /*
     * Set or change the base material for the current material, updating the properties as
     * required.
     */
    void updateInheritance(const std::string& parent);
    /*
     * Return a list of models that are defined in the parent material but not in this one
     */
    std::vector<std::string> inheritedMissingModels(const Material& parent) const;
    /*
     * Return a list of models that are defined in this model but not the parent
     */
    std::vector<std::string> inheritedAddedModels(const Material& parent) const;
    /*
     * Return a list of properties that have different values from the parent material
     */
    void inheritedPropertyDiff(const std::string& parent);

    void save(QTextStream& stream, bool overwrite, bool saveAsCopy, bool saveInherited);

    /*
     * Assignment operator
     */
    Material& operator=(const Material& other);

    /*
     * Set the appearance properties
     */
    Material& operator=(const App::Material& other);

    bool operator==(const Material& other) const
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId() && _uuid == other._uuid;
    }

    void validate(Material& other) const;

protected:
    void addModel(const std::string& uuid);
    static void removeUUID(std::set<std::string>& uuidList, const std::string& uuid);

    static QVariant
    getValue(const std::map<std::string, std::shared_ptr<MaterialProperty>>& propertyList,
             const std::string& name);
    static std::string
    getValueString(const std::map<std::string, std::shared_ptr<MaterialProperty>>& propertyList,
                   const std::string& name);

    bool modelChanged(const Material& parent,
                      const Model& model) const;
    bool modelAppearanceChanged(const Material& parent,
                                const Model& model) const;
    void saveGeneral(QTextStream& stream) const;
    void saveInherits(QTextStream& stream) const;
    void saveModels(QTextStream& stream, bool saveInherited) const;
    void saveAppearanceModels(QTextStream& stream, bool saveInherited) const;

private:
    std::shared_ptr<MaterialLibrary> _library;
    std::string _directory;
    std::string _filename;
    std::string _uuid;
    std::string _name;
    std::string _author;
    std::string _license;
    std::string _parentUuid;
    std::string _description;
    std::string _url;
    std::string _reference;
    std::set<std::string> _tags;
    std::set<std::string> _physicalUuids;
    std::set<std::string> _appearanceUuids;
    std::set<std::string> _allUuids;  // Includes inherited models
    std::map<std::string, std::shared_ptr<MaterialProperty>> _physical;
    std::map<std::string, std::shared_ptr<MaterialProperty>> _appearance;
    std::map<std::string, std::string> _legacy;
    bool _dereferenced;
    bool _oldFormat;
    ModelEdit _editState;
};

inline QTextStream& operator<<(QTextStream& output, const MaterialProperty& property)
{
    output << QString::fromStdString(MaterialValue::escapeString(property.getName())) << ":"
           << QString::fromStdString(property.getYAMLString());
    return output;
}

using MaterialTreeNode = FolderTreeNode<Material>;

}  // namespace Materials

Q_DECLARE_METATYPE(Materials::Material*)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::Material>)
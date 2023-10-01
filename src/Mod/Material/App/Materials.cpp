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

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <QMetaType>
#include <QUuid>

#include <App/Application.h>
#include <Gui/MetaTypes.h>

#include "MaterialManager.h"
#include "Materials.h"
#include "ModelManager.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::MaterialProperty, Materials::ModelProperty)

MaterialProperty::MaterialProperty()
{
    _valuePtr = std::make_shared<MaterialValue>(MaterialValue::None);
}

MaterialProperty::MaterialProperty(const ModelProperty& property)
    : ModelProperty(property)
    , _valuePtr(nullptr)
{
    setType(getPropertyType());
    auto columns = property.getColumns();
    for (std::vector<ModelProperty>::const_iterator it = columns.begin(); it != columns.end();
         it++) {
        MaterialProperty prop(*it);
        addColumn(prop);
    }

    if (_valuePtr->getType() == MaterialValue::Array2D) {
        std::static_pointer_cast<Material2DArray>(_valuePtr)->setDefault(getColumnNull(0));
    }
    else if (_valuePtr->getType() == MaterialValue::Array3D) {
        std::static_pointer_cast<Material3DArray>(_valuePtr)->setDefault(getColumnNull(0));
    }
}

MaterialProperty::MaterialProperty(const MaterialProperty& other)
    : ModelProperty(other)
{
    _modelUUID = other._modelUUID;
    if (other._valuePtr != nullptr) {
        _valuePtr = std::make_shared<MaterialValue>(*(other._valuePtr));
    }
    else {
        _valuePtr = nullptr;
    }

    for (auto it = other._columns.begin(); it != other._columns.end(); it++) {
        _columns.push_back(*it);
    }
}

// MaterialProperty::~MaterialProperty()
// {}

void MaterialProperty::setModelUUID(const QString& uuid)
{
    _modelUUID = uuid;
}

const QVariant MaterialProperty::getValue() const
{
    return _valuePtr->getValue();
}

std::shared_ptr<MaterialValue> MaterialProperty::getMaterialValue()
{
    return _valuePtr;
}

const std::shared_ptr<MaterialValue> MaterialProperty::getMaterialValue() const
{
    return _valuePtr;
}

const QString MaterialProperty::getString() const
{
    if (getType() == MaterialValue::Quantity) {
        Base::Quantity quantity = getValue().value<Base::Quantity>();
        return quantity.getUserString();
    }
    return getValue().toString();
}

void MaterialProperty::setPropertyType(const QString& type)
{
    ModelProperty::setPropertyType(type);
    setType(type);
}

void MaterialProperty::setType(const QString& type)
{
    if (type == QString::fromStdString("String")) {
        _valuePtr = std::make_shared<MaterialValue>(MaterialValue::String);
    }
    else if (type == QString::fromStdString("Boolean")) {
        _valuePtr = std::make_shared<MaterialValue>(MaterialValue::Boolean);
    }
    else if (type == QString::fromStdString("Integer")) {
        _valuePtr = std::make_shared<MaterialValue>(MaterialValue::Integer);
    }
    else if (type == QString::fromStdString("Float")) {
        _valuePtr = std::make_shared<MaterialValue>(MaterialValue::Float);
    }
    else if (type == QString::fromStdString("URL")) {
        _valuePtr = std::make_shared<MaterialValue>(MaterialValue::URL);
    }
    else if (type == QString::fromStdString("Quantity")) {
        _valuePtr = std::make_shared<MaterialValue>(MaterialValue::Quantity);
    }
    else if (type == QString::fromStdString("Color")) {
        _valuePtr = std::make_shared<MaterialValue>(MaterialValue::Color);
    }
    else if (type == QString::fromStdString("File")) {
        _valuePtr = std::make_shared<MaterialValue>(MaterialValue::File);
    }
    else if (type == QString::fromStdString("Image")) {
        _valuePtr = std::make_shared<MaterialValue>(MaterialValue::Image);
    }
    else if (type == QString::fromStdString("List")) {
        _valuePtr = std::make_shared<MaterialValue>(MaterialValue::List);
    }
    else if (type == QString::fromStdString("2DArray")) {
        _valuePtr = std::make_shared<Material2DArray>();
    }
    else if (type == QString::fromStdString("3DArray")) {
        _valuePtr = std::make_shared<Material3DArray>();
    }
    else {
        // Error. Throw something
        _valuePtr = std::make_shared<MaterialValue>(MaterialValue::None);
        std::string stringType = type.toStdString();
        std::string name = getName().toStdString();
        throw UnknownValueType();
    }
}

MaterialProperty& MaterialProperty::getColumn(int column)
{
    try {
        return _columns.at(column);
    }
    catch (std::out_of_range const&) {
        throw InvalidColumn();
    }
}

const MaterialProperty& MaterialProperty::getColumn(int column) const
{
    try {
        return _columns.at(column);
    }
    catch (std::out_of_range const&) {
        throw InvalidColumn();
    }
}

MaterialValue::ValueType MaterialProperty::getColumnType(int column) const
{
    try {
        return _columns.at(column).getType();
    }
    catch (std::out_of_range const&) {
        throw InvalidColumn();
    }
}

QString MaterialProperty::getColumnUnits(int column) const
{
    try {
        return _columns.at(column).getUnits();
    }
    catch (std::out_of_range const&) {
        throw InvalidColumn();
    }
}

QVariant MaterialProperty::getColumnNull(int column) const
{
    MaterialValue::ValueType valueType = getColumnType(column);

    switch (valueType) {
        case MaterialValue::Quantity: {
            Base::Quantity q = Base::Quantity(0, getColumnUnits(column));
            return QVariant::fromValue(q);
        }

        case MaterialValue::Float:
        case MaterialValue::Integer:
            return QVariant(0);

        default:
            break;
    }

    return QVariant(QString());
}

void MaterialProperty::setValue(const QVariant& value)
{
    // _valueType = MaterialValue::String;
    _valuePtr->setValue(value);
}

void MaterialProperty::setValue(const QString& value)
{
    if (_valuePtr->getType() == MaterialValue::Boolean) {
        setBoolean(value);
    }
    else if (_valuePtr->getType() == MaterialValue::Integer) {
        setInt(value);
    }
    else if (_valuePtr->getType() == MaterialValue::Float) {
        setFloat(value);
    }
    else if (_valuePtr->getType() == MaterialValue::URL) {
        setURL(value);
    }
    else if (_valuePtr->getType() == MaterialValue::Quantity) {
        // Base::Console().Log("\tParse quantity '%s'\n", value.toStdString().c_str());
        try {
            setQuantity(Base::Quantity::parse(value));
        }
        catch (const Base::ParserError& e) {
            Base::Console().Log("Error '%s'\n", e.what());
            // Save as a string
            setString(value);
        }
    }
    else {
        setString(value);
    }
}

void MaterialProperty::setString(const QString& value)
{
    // _valueType = MaterialValue::String;
    _valuePtr->setValue(QVariant(value));
}

void MaterialProperty::setBoolean(bool value)
{
    // _valueType = MaterialValue::Boolean;
    _valuePtr->setValue(QVariant(value));
}

void MaterialProperty::setBoolean(int value)
{
    // _valueType = MaterialValue::Boolean;
    _valuePtr->setValue(QVariant(value != 0));
}

void MaterialProperty::setBoolean(const QString& value)
{
    // _valueType = MaterialValue::Boolean;
    bool boolean;
    std::string val = value.toStdString();
    if ((val == "true") || (val == "True")) {
        boolean = true;
    }
    else if ((val == "false") || (val == "False")) {
        boolean = false;
    }
    else {
        boolean = (std::stoi(val) != 0);
    }

    setBoolean(boolean);
}

void MaterialProperty::setInt(int value)
{
    _valuePtr->setValue(QVariant(value));
}

void MaterialProperty::setInt(const QString& value)
{
    _valuePtr->setValue(value.toInt());
}

void MaterialProperty::setFloat(double value)
{
    _valuePtr->setValue(QVariant(value));
}

void MaterialProperty::setFloat(const QString& value)
{
    _valuePtr->setValue(QVariant(value.toFloat()));
}

void MaterialProperty::setQuantity(const Base::Quantity& value)
{
    _valuePtr->setValue(QVariant(QVariant::fromValue(value)));
}

void MaterialProperty::setQuantity(double value, const QString& units)
{
    setQuantity(Base::Quantity(value, units));
}

void MaterialProperty::setQuantity(const QString& value)
{
    setQuantity(Base::Quantity::parse(value));
}

void MaterialProperty::setURL(const QString& value)
{
    _valuePtr->setValue(QVariant(value));
}

MaterialProperty& MaterialProperty::operator=(const MaterialProperty& other)
{
    if (this == &other) {
        return *this;
    }

    ModelProperty::operator=(other);
    _modelUUID = other._modelUUID;

    if (other._valuePtr != nullptr) {
        _valuePtr = std::make_shared<MaterialValue>(*(other._valuePtr));
    }
    else {
        _valuePtr = nullptr;
    }

    _columns.clear();
    for (auto it = other._columns.begin(); it != other._columns.end(); it++) {
        _columns.push_back(*it);
    }

    return *this;
}

TYPESYSTEM_SOURCE(Materials::Material, Base::BaseClass)

Material::Material()
    : _dereferenced(false)
{}

Material::Material(const MaterialLibrary& library,
                   const QString& directory,
                   const QString& uuid,
                   const QString& name)
    : _library(library)
    , _uuid(uuid)
    , _name(name)
    , _dereferenced(false)
    , _editState(ModelEdit_None)
{
    setDirectory(directory);
}

Material::Material(const Material& other)
    : _library(other._library)
    , _directory(other._directory)
    , _uuid(other._uuid)
    , _name(other._name)
    , _authorAndLicense(other._authorAndLicense)
    , _parentUuid(other._parentUuid)
    , _description(other._description)
    , _url(other._url)
    , _reference(other._reference)
    , _dereferenced(other._dereferenced)
    , _editState(other._editState)
{
    for (auto it = other._tags.begin(); it != other._tags.end(); it++) {
        _tags.push_back(*it);
    }
    for (auto it = other._physicalUuids.begin(); it != other._physicalUuids.end(); it++) {
        _physicalUuids.push_back(*it);
    }
    for (auto it = other._appearanceUuids.begin(); it != other._appearanceUuids.end(); it++) {
        _appearanceUuids.push_back(*it);
    }
    for (auto it = other._allUuids.begin(); it != other._allUuids.end(); it++) {
        _allUuids.push_back(*it);
    }
    for (auto it = other._physical.begin(); it != other._physical.end(); it++) {
        _physical[it->first] = MaterialProperty(it->second);
    }
    for (auto it = other._appearance.begin(); it != other._appearance.end(); it++) {
        _appearance[it->first] = MaterialProperty(it->second);
    }
}

/*
 *  Destroys the object and frees any allocated resources
 */
Material::~Material()
{
    // no need to delete child widgets, Qt does it all for us
}

void Material::addModel(const QString& uuid)
{
    for (QString modelUUID : _allUuids) {
        if (modelUUID == uuid) {
            return;
        }
    }

    _allUuids.push_back(uuid);

    ModelManager manager;

    try {
        const Model& model = manager.getModel(uuid);
        auto inheritance = model.getInheritance();
        for (auto inherits = inheritance.begin(); inherits != inheritance.end(); inherits++) {
            addModel(*inherits);
        }
    }
    catch (ModelNotFound const&) {
    }
}

void Material::setEditState(ModelEdit newState)
{
    if (newState == ModelEdit_Extend) {
        if (_editState != ModelEdit_Alter) {
            _editState = newState;
        }
    }
    else if (newState == ModelEdit_Alter) {
        _editState = newState;
    }
}

void Material::addPhysical(const QString& uuid)
{
    if (hasPhysicalModel(uuid)) {
        return;
    }

    ModelManager manager;

    try {
        const Model& model = manager.getModel(uuid);

        _physicalUuids.push_back(uuid);
        addModel(uuid);
        setEditStateExtend();

        for (auto it = model.begin(); it != model.end(); it++) {
            QString propertyName = it->first;
            ModelProperty property = static_cast<ModelProperty>(it->second);

            try {
                _physical[propertyName] = MaterialProperty(property);
            }
            catch (const UnknownValueType&) {
                Base::Console().Error("Property '%s' has unknown type '%s'. Ignoring\n",
                                      property.getName().toStdString().c_str(),
                                      property.getPropertyType().toStdString().c_str());
            }
        }
    }
    catch (ModelNotFound const&) {
    }
}

void Material::addAppearance(const QString& uuid)
{
    if (hasAppearanceModel(uuid)) {
        return;
    }

    ModelManager manager;

    try {
        const Model& model = manager.getModel(uuid);

        _appearanceUuids.push_back(uuid);
        addModel(uuid);
        setEditStateExtend();

        for (auto it = model.begin(); it != model.end(); it++) {
            QString propertyName = it->first;
            ModelProperty property = static_cast<ModelProperty>(it->second);

            _appearance[propertyName] = MaterialProperty(property);
        }
    }
    catch (ModelNotFound const&) {
    }
}

void Material::setPhysicalEditState(const QString& name)
{
    if (getPhysicalProperty(name).isNull()) {
        setEditStateExtend();
    }
    else {
        setEditStateAlter();
    }
}

void Material::setAppearanceEditState(const QString& name)
{
    if (getAppearanceProperty(name).isNull()) {
        setEditStateExtend();
    }
    else {
        setEditStateAlter();
    }
}

void Material::setPhysicalValue(const QString& name, const QString& value)
{
    setPhysicalEditState(name);

    _physical[name].setValue(value);  // may not be a string type
}

void Material::setPhysicalValue(const QString& name, int value)
{
    setPhysicalEditState(name);

    _physical[name].setInt(value);
}

void Material::setPhysicalValue(const QString& name, double value)
{
    setPhysicalEditState(name);

    _physical[name].setFloat(value);
}

void Material::setPhysicalValue(const QString& name, const Base::Quantity value)
{
    setPhysicalEditState(name);

    _physical[name].setQuantity(value);
}

void Material::setAppearanceValue(const QString& name, const QString& value)
{
    setAppearanceEditState(name);

    _appearance[name].setValue(value);  // may not be a string type
}

MaterialProperty& Material::getPhysicalProperty(const QString& name)
{
    try {
        return _physical.at(name);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

const MaterialProperty& Material::getPhysicalProperty(const QString& name) const
{
    try {
        return _physical.at(name);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

MaterialProperty& Material::getAppearanceProperty(const QString& name)
{
    try {
        return _appearance.at(name);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

const MaterialProperty& Material::getAppearanceProperty(const QString& name) const
{
    try {
        return _appearance.at(name);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

const QVariant Material::getValue(const std::map<QString, MaterialProperty>& propertyList,
                                  const QString& name) const
{
    try {
        return propertyList.at(name).getValue();
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

const QString Material::getValueString(const std::map<QString, MaterialProperty>& propertyList,
                                       const QString& name) const
{
    try {
        if (propertyList.at(name).getType() == MaterialValue::Quantity) {
            auto value = propertyList.at(name).getValue();
            if (value.isNull()) {
                return QString();
            }
            return value.value<Base::Quantity>().getUserString();
        }
        return propertyList.at(name).getValue().toString();
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

const QVariant Material::getPhysicalValue(const QString& name) const
{
    return getValue(_physical, name);
}

const QString Material::getPhysicalValueString(const QString& name) const
{
    return getValueString(_physical, name);
}

const QVariant Material::getAppearanceValue(const QString& name) const
{
    return getValue(_appearance, name);
}

const QString Material::getAppearanceValueString(const QString& name) const
{
    return getValueString(_appearance, name);
}

bool Material::hasPhysicalProperty(const QString& name) const
{
    try {
        static_cast<void>(_physical.at(name));
    }
    catch (std::out_of_range const&) {
        return false;
    }
    return true;
}

bool Material::hasAppearanceProperty(const QString& name) const
{
    try {
        static_cast<void>(_appearance.at(name));
    }
    catch (std::out_of_range const&) {
        return false;
    }
    return true;
}

bool Material::hasModel(const QString& uuid) const
{
    for (QString modelUUID : _allUuids) {
        if (modelUUID == uuid) {
            return true;
        }
    }

    return false;
}

bool Material::hasPhysicalModel(const QString& uuid) const
{
    if (!hasModel(uuid)) {
        return false;
    }

    ModelManager manager;

    try {
        const Model& model = manager.getModel(uuid);
        if (model.getType() == Model::ModelType_Physical) {
            return true;
        }
    }
    catch (ModelNotFound const&) {
    }

    return false;
}

bool Material::hasAppearanceModel(const QString& uuid) const
{
    if (!hasModel(uuid)) {
        return false;
    }

    ModelManager manager;

    try {
        const Model& model = manager.getModel(uuid);
        if (model.getType() == Model::ModelType_Appearance) {
            return true;
        }
    }
    catch (ModelNotFound const&) {
    }

    return false;
}

bool Material::isPhysicalModelComplete(const QString& uuid) const
{
    if (!hasPhysicalModel(uuid)) {
        return false;
    }

    ModelManager manager;

    try {
        const Model& model = manager.getModel(uuid);
        for (auto it = model.begin(); it != model.end(); it++) {
            QString propertyName = it->first;
            const MaterialProperty& property = getPhysicalProperty(propertyName);

            if (property.isNull()) {
                return false;
            }
        }
    }
    catch (ModelNotFound const&) {
        return false;
    }

    return true;
}

bool Material::isAppearanceModelComplete(const QString& uuid) const
{
    if (!hasAppearanceModel(uuid)) {
        return false;
    }

    ModelManager manager;

    try {
        const Model& model = manager.getModel(uuid);
        for (auto it = model.begin(); it != model.end(); it++) {
            QString propertyName = it->first;
            const MaterialProperty& property = getAppearanceProperty(propertyName);

            if (property.isNull()) {
                return false;
            }
        }
    }
    catch (ModelNotFound const&) {
        return false;
    }

    return true;
}

void Material::saveGeneral(QTextStream& stream) const
{
    stream << "General:\n";
    stream << "  UUID: \"" << _uuid << "\"\n";
    stream << "  Name: \"" << _name << "\"\n";
    if (!_authorAndLicense.isEmpty()) {
        stream << "  AuthorAndLicense: \"" << _authorAndLicense << "\"\n";
    }
    if (!_description.isEmpty()) {
        stream << "  Description: \"" << _description << "\"\n";
    }
    if (!_url.isEmpty()) {
        stream << "  SourceURL: \"" << _url << "\"\n";
    }
    if (!_reference.isEmpty()) {
        stream << "  ReferenceSource: \"" << _reference << "\"\n";
    }
}

void Material::saveInherits(QTextStream& stream) const
{
    if (!_parentUuid.isEmpty()) {
        MaterialManager manager;

        stream << "Inherits:\n";
        stream << "  " << manager.getMaterial(_parentUuid).getName() << ":\n";
        stream << "    UUID: \"" << _parentUuid << "\"\n";
    }
}

void Material::saveModels(QTextStream& stream) const
{
    if (!_physical.empty()) {
        ModelManager modelManager;

        stream << "Models:\n";
        for (auto itm = _physicalUuids.begin(); itm != _physicalUuids.end(); itm++) {
            auto model = modelManager.getModel(*itm);
            stream << "  " << model.getName() << ":\n";
            stream << "    UUID: \"" << model.getUUID() << "\"\n";
            for (auto itp = model.begin(); itp != model.end(); itp++) {
                QString propertyName = itp->first;
                const MaterialProperty& property = getPhysicalProperty(propertyName);

                if (!property.isNull()) {
                    stream << "    " << propertyName << ": \""
                           << getPhysicalValueString(propertyName) << "\"\n";
                }
            }
        }
    }
}

void Material::saveAppearanceModels(QTextStream& stream) const
{
    if (!_appearance.empty()) {
        ModelManager modelManager;

        stream << "AppearanceModels:\n";
        for (auto itm = _appearanceUuids.begin(); itm != _appearanceUuids.end(); itm++) {
            auto model = modelManager.getModel(*itm);
            stream << "  " << model.getName() << ":\n";
            stream << "    UUID: \"" << model.getUUID() << "\"\n";
            for (auto itp = model.begin(); itp != model.end(); itp++) {
                QString propertyName = itp->first;
                const MaterialProperty& property = getAppearanceProperty(propertyName);

                if (!property.isNull()) {
                    stream << "    " << propertyName << ": \""
                           << getAppearanceValueString(propertyName) << "\"\n";
                }
            }
        }
    }
}

void Material::newUuid()
{
    _uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void Material::save(QTextStream& stream, bool saveAsCopy)
{
    Q_UNUSED(saveAsCopy)

    stream << "# File created by FreeCAD\n";
    saveGeneral(stream);
    saveInherits(stream);
    saveModels(stream);
    saveAppearanceModels(stream);
}

Material& Material::operator=(const Material& other)
{
    if (this == &other) {
        return *this;
    }

    _library = other._library;
    _directory = other._directory;
    _uuid = other._uuid;
    _name = other._name;
    _authorAndLicense = other._authorAndLicense;
    _parentUuid = other._parentUuid;
    _description = other._description;
    _url = other._url;
    _reference = other._reference;
    _dereferenced = other._dereferenced;

    _tags.clear();
    for (auto it = other._tags.begin(); it != other._tags.end(); it++) {
        _tags.push_back(*it);
    }
    _physicalUuids.clear();
    for (auto it = other._physicalUuids.begin(); it != other._physicalUuids.end(); it++) {
        _physicalUuids.push_back(*it);
    }
    _appearanceUuids.clear();
    for (auto it = other._appearanceUuids.begin(); it != other._appearanceUuids.end(); it++) {
        _appearanceUuids.push_back(*it);
    }
    _allUuids.clear();
    for (auto it = other._allUuids.begin(); it != other._allUuids.end(); it++) {
        _allUuids.push_back(*it);
    }
    _physical.clear();
    for (auto it = other._physical.begin(); it != other._physical.end(); it++) {
        _physical[it->first] = MaterialProperty(it->second);
    }
    _appearance.clear();
    for (auto it = other._appearance.begin(); it != other._appearance.end(); it++) {
        _appearance[it->first] = MaterialProperty(it->second);
    }

    return *this;
}

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

#include "Materials.h"

#include "MaterialLibrary.h"
#include "MaterialManager.h"
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
        std::static_pointer_cast<Material2DArray>(_valuePtr)->setDefault(getColumnNull(0), false);
    }
    else if (_valuePtr->getType() == MaterialValue::Array3D) {
        std::static_pointer_cast<Material3DArray>(_valuePtr)->setDefault(getColumnNull(0), false);
    }
}

void MaterialProperty::copyValuePtr(std::shared_ptr<MaterialValue> value)
{
    if (value->getType() == MaterialValue::Array2D) {
        _valuePtr =
            std::make_shared<Material2DArray>(*(std::static_pointer_cast<Material2DArray>(value)));
    }
    else if (value->getType() == MaterialValue::Array3D) {
        _valuePtr =
            std::make_shared<Material3DArray>(*(std::static_pointer_cast<Material3DArray>(value)));
    }
    else {
        _valuePtr = std::make_shared<MaterialValue>(*value);
    }
}

MaterialProperty::MaterialProperty(const MaterialProperty& other)
    : ModelProperty(other)
{
    _modelUUID = other._modelUUID;
    copyValuePtr(other._valuePtr);

    for (auto it = other._columns.begin(); it != other._columns.end(); it++) {
        _columns.push_back(*it);
    }
}

MaterialProperty::MaterialProperty(std::shared_ptr<MaterialProperty> other)
    : MaterialProperty(*other)
{}

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
    if (isNull()) {
        return QString();
    }
    if (getType() == MaterialValue::Quantity) {
        Base::Quantity quantity = getValue().value<Base::Quantity>();
        return quantity.getUserString();
    }
    else if (getType() == MaterialValue::Float) {
        auto value = getValue();
        if (value.isNull()) {
            return QString();
        }
        return QString(QString::fromStdString("%1")).arg(value.toFloat(), 0, 'g', 6);
    }
    return getValue().toString();
}

const QString MaterialProperty::getYAMLString() const
{
    return _valuePtr->getYAMLString();
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
    else if (_valuePtr->getType() == MaterialValue::Array2D) {
        //_valuePtr->setValue(QVariant(std::make_shared<Material2DArray>()));
    }
    else if (_valuePtr->getType() == MaterialValue::Array3D) {
        //_valuePtr = std::make_shared<Material3DArray>();
    }
    else if (_valuePtr->getType() == MaterialValue::Quantity) {
        // Base::Console().Log("\tParse quantity '%s'\n", value.toStdString().c_str());
        try {
            setQuantity(Base::Quantity::parse(value));
        }
        catch (const Base::ParserError& e) {
            Base::Console().Log("MaterialProperty::setValue Error '%s' - '%s'\n",
                                e.what(),
                                value.toStdString().c_str());
            // Save as a string
            setString(value);
        }
    }
    else {
        setString(value);
    }
}

void MaterialProperty::setValue(std::shared_ptr<MaterialValue> value)
{
    _valuePtr = value;
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
    copyValuePtr(other._valuePtr);

    _columns.clear();
    for (auto it = other._columns.begin(); it != other._columns.end(); it++) {
        _columns.push_back(*it);
    }

    return *this;
}

bool MaterialProperty::operator==(const MaterialProperty& other) const
{
    if (this == &other) {
        return true;
    }

    if (ModelProperty::operator==(other)) {
        return (*_valuePtr == *other._valuePtr);
    }
    return false;
}

TYPESYSTEM_SOURCE(Materials::Material, Base::BaseClass)

Material::Material()
    : _dereferenced(false)
    , _editState(ModelEdit_None)
{}

Material::Material(std::shared_ptr<MaterialLibrary> library,
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
    , _author(other._author)
    , _license(other._license)
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
        MaterialProperty prop(it->second);
        _physical[it->first] = std::make_shared<MaterialProperty>(prop);
    }
    for (auto it = other._appearance.begin(); it != other._appearance.end(); it++) {
        MaterialProperty prop(it->second);
        _appearance[it->first] = std::make_shared<MaterialProperty>(prop);
    }
}

const QString Material::getAuthorAndLicense() const
{
    QString authorAndLicense;

    // Combine the author and license field for backwards compatibility
    if (!_author.isNull()) {
        authorAndLicense = _author;
        if (!_license.isNull()) {
            authorAndLicense += QString::fromStdString(" ") + _license;
        }
    }
    else if (!_license.isNull()) {
        authorAndLicense = _license;
    }

    return _license;
}

void Material::addModel(const QString& uuid)
{
    for (QString modelUUID : _allUuids) {
        if (modelUUID == uuid) {
            return;
        }
    }

    _allUuids << uuid;

    ModelManager manager;

    try {
        auto model = manager.getModel(uuid);
        auto inheritance = model->getInheritance();
        for (auto inherits = inheritance.begin(); inherits != inheritance.end(); inherits++) {
            addModel(*inherits);
        }
    }
    catch (ModelNotFound const&) {
    }
}

void Material::removeModel(const QString& uuid)
{
    Q_UNUSED(uuid);
}

void Material::clearModels()
{
    _physicalUuids.clear();
    _appearanceUuids.clear();
    _allUuids.clear();
    _physical.clear();
    _appearance.clear();
}

void Material::setName(const QString& name)
{
    _name = name;
    setEditStateExtend();
}

void Material::setAuthor(const QString& author)
{
    _author = author;
    setEditStateExtend();
}

void Material::setLicense(const QString& license)
{
    _license = license;
    setEditStateExtend();
}

void Material::setParentUUID(const QString& uuid)
{
    _parentUuid = uuid;
    setEditStateExtend();
}

void Material::setDescription(const QString& description)
{
    _description = description;
    setEditStateExtend();
}

void Material::setURL(const QString& url)
{
    _url = url;
    setEditStateExtend();
}

void Material::setReference(const QString& reference)
{
    _reference = reference;
    setEditStateExtend();
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

void Material::removeUUID(QStringList& uuidList, const QString& uuid)
{
    uuidList.removeAll(uuid);
}

void Material::addPhysical(const QString& uuid)
{
    if (hasPhysicalModel(uuid)) {
        return;
    }

    ModelManager manager;

    try {
        auto model = manager.getModel(uuid);

        auto& inheritance = model->getInheritance();
        for (auto it = inheritance.begin(); it != inheritance.end(); it++) {
            // Inherited models may already have the properties, so just
            // remove the uuid
            removeUUID(_physicalUuids, *it);
        }

        _physicalUuids.push_back(uuid);
        addModel(uuid);
        setEditStateExtend();

        for (auto it = model->begin(); it != model->end(); it++) {
            QString propertyName = it->first;
            if (!hasPhysicalProperty(propertyName)) {
                ModelProperty property = static_cast<ModelProperty>(it->second);

                try {
                    _physical[propertyName] = std::make_shared<MaterialProperty>(property);
                }
                catch (const UnknownValueType&) {
                    Base::Console().Error("Property '%s' has unknown type '%s'. Ignoring\n",
                                          property.getName().toStdString().c_str(),
                                          property.getPropertyType().toStdString().c_str());
                }
            }
        }
    }
    catch (ModelNotFound const&) {
    }
}

void Material::removePhysical(const QString& uuid)
{
    if (!hasPhysicalModel(uuid)) {
        return;
    }

    // If it's an inherited model, do nothing
    bool inherited = true;
    for (auto it = _physicalUuids.begin(); it != _physicalUuids.end(); it++) {
        if (*it == uuid) {
            inherited = false;
            break;
        }
    }
    if (inherited) {
        return;
    }

    ModelManager manager;

    try {
        auto model = manager.getModel(uuid);

        auto& inheritance = model->getInheritance();
        for (auto it = inheritance.begin(); it != inheritance.end(); it++) {
            removeUUID(_physicalUuids, *it);
            removeUUID(_allUuids, *it);
        }
        removeUUID(_physicalUuids, uuid);
        removeUUID(_allUuids, uuid);

        for (auto it = model->begin(); it != model->end(); it++) {
            _physical.erase(it->first);
        }

        setEditStateAlter();
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
        auto model = manager.getModel(uuid);

        auto& inheritance = model->getInheritance();
        for (auto it = inheritance.begin(); it != inheritance.end(); it++) {
            // Inherited models may already have the properties, so just
            // remove the uuid
            removeUUID(_appearanceUuids, *it);
        }

        _appearanceUuids.push_back(uuid);
        addModel(uuid);
        setEditStateExtend();

        for (auto it = model->begin(); it != model->end(); it++) {
            QString propertyName = it->first;
            if (!hasAppearanceProperty(propertyName)) {
                ModelProperty property = static_cast<ModelProperty>(it->second);

                _appearance[propertyName] = std::make_shared<MaterialProperty>(property);
            }
        }
    }
    catch (ModelNotFound const&) {
    }
}

void Material::removeAppearance(const QString& uuid)
{
    if (!hasAppearanceModel(uuid)) {
        return;
    }

    // If it's an inherited model, do nothing
    bool inherited = true;
    for (auto it = _appearanceUuids.begin(); it != _appearanceUuids.end(); it++) {
        if (*it == uuid) {
            inherited = false;
            break;
        }
    }
    if (inherited) {
        return;
    }

    ModelManager manager;

    try {
        auto model = manager.getModel(uuid);

        auto& inheritance = model->getInheritance();
        for (auto it = inheritance.begin(); it != inheritance.end(); it++) {
            removeUUID(_appearanceUuids, *it);
            removeUUID(_allUuids, *it);
        }
        removeUUID(_appearanceUuids, uuid);
        removeUUID(_allUuids, uuid);

        for (auto it = model->begin(); it != model->end(); it++) {
            _appearance.erase(it->first);
        }

        setEditStateAlter();
    }
    catch (ModelNotFound const&) {
    }
}

void Material::setPropertyEditState(const QString& name)
{
    if (hasPhysicalProperty(name)) {
        setPhysicalEditState(name);
    }
    else if (hasAppearanceProperty(name)) {
        setAppearanceEditState(name);
    }
}

void Material::setPhysicalEditState(const QString& name)
{
    if (getPhysicalProperty(name)->isNull()) {
        setEditStateExtend();
    }
    else {
        setEditStateAlter();
    }
}

void Material::setAppearanceEditState(const QString& name)
{
    if (getAppearanceProperty(name)->isNull()) {
        setEditStateExtend();
    }
    else {
        setEditStateAlter();
    }
}

void Material::setPhysicalValue(const QString& name, const QString& value)
{
    setPhysicalEditState(name);

    _physical[name]->setValue(value);  // may not be a string type, conversion may be required
}

void Material::setPhysicalValue(const QString& name, int value)
{
    setPhysicalEditState(name);

    _physical[name]->setInt(value);
}

void Material::setPhysicalValue(const QString& name, double value)
{
    setPhysicalEditState(name);

    _physical[name]->setFloat(value);
}

void Material::setPhysicalValue(const QString& name, const Base::Quantity value)
{
    setPhysicalEditState(name);

    _physical[name]->setQuantity(value);
}

void Material::setPhysicalValue(const QString& name, std::shared_ptr<MaterialValue> value)
{
    setPhysicalEditState(name);

    _physical[name]->setValue(value);
}

void Material::setAppearanceValue(const QString& name, const QString& value)
{
    setAppearanceEditState(name);

    _appearance[name]->setValue(value);  // may not be a string type, conversion may be required
}

void Material::setAppearanceValue(const QString& name, std::shared_ptr<MaterialValue> value)
{
    setAppearanceEditState(name);

    _appearance[name]->setValue(value);
}

std::shared_ptr<MaterialProperty> Material::getPhysicalProperty(const QString& name)
{
    try {
        return _physical.at(name);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

const std::shared_ptr<MaterialProperty> Material::getPhysicalProperty(const QString& name) const
{
    try {
        return _physical.at(name);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

std::shared_ptr<MaterialProperty> Material::getAppearanceProperty(const QString& name)
{
    try {
        return _appearance.at(name);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

const std::shared_ptr<MaterialProperty> Material::getAppearanceProperty(const QString& name) const
{
    try {
        return _appearance.at(name);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

const QVariant
Material::getValue(const std::map<QString, std::shared_ptr<MaterialProperty>>& propertyList,
                   const QString& name) const
{
    try {
        return propertyList.at(name)->getValue();
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

const QString
Material::getValueString(const std::map<QString, std::shared_ptr<MaterialProperty>>& propertyList,
                         const QString& name) const
{
    try {
        auto property = propertyList.at(name);
        if (property->isNull()) {
            return QString();
        }
        if (property->getType() == MaterialValue::Quantity) {
            auto value = property->getValue();
            if (value.isNull()) {
                return QString();
            }
            return value.value<Base::Quantity>().getUserString();
        }
        else if (property->getType() == MaterialValue::Float) {
            auto value = property->getValue();
            if (value.isNull()) {
                return QString();
            }
            return QString(QString::fromStdString("%1")).arg(value.toFloat(), 0, 'g', 6);
        }
        return property->getValue().toString();
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

const QVariant Material::getPhysicalValue(const QString& name) const
{
    return getValue(_physical, name);
}

const Base::Quantity Material::getPhysicalQuantity(const QString& name) const
{
    return getValue(_physical, name).value<Base::Quantity>();
}

const QString Material::getPhysicalValueString(const QString& name) const
{
    return getValueString(_physical, name);
}

const QVariant Material::getAppearanceValue(const QString& name) const
{
    return getValue(_appearance, name);
}

const Base::Quantity Material::getAppearanceQuantity(const QString& name) const
{
    return getValue(_appearance, name).value<Base::Quantity>();
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
    return _allUuids.contains(uuid);
}

bool Material::hasPhysicalModel(const QString& uuid) const
{
    if (!hasModel(uuid)) {
        return false;
    }

    ModelManager manager;

    try {
        auto model = manager.getModel(uuid);
        if (model->getType() == Model::ModelType_Physical) {
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
        auto model = manager.getModel(uuid);
        if (model->getType() == Model::ModelType_Appearance) {
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
        auto model = manager.getModel(uuid);
        for (auto it = model->begin(); it != model->end(); it++) {
            QString propertyName = it->first;
            auto property = getPhysicalProperty(propertyName);

            if (property->isNull()) {
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
        auto model = manager.getModel(uuid);
        for (auto it = model->begin(); it != model->end(); it++) {
            QString propertyName = it->first;
            auto property = getAppearanceProperty(propertyName);

            if (property->isNull()) {
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
    if (!_author.isEmpty()) {
        stream << "  Author: \"" << _author << "\"\n";
    }
    if (!_license.isEmpty()) {
        stream << "  License: \"" << _license << "\"\n";
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

        try {
            auto material = manager.getMaterial(_parentUuid);

            stream << "Inherits:\n";
            stream << "  " << material->getName() << ":\n";
            stream << "    UUID: \"" << _parentUuid << "\"\n";
        }
        catch (const MaterialNotFound&) {
        }
    }
}

bool Material::modelChanged(const std::shared_ptr<Material> parent,
                            const std::shared_ptr<Model> model) const
{
    for (auto itp = model->begin(); itp != model->end(); itp++) {
        QString propertyName = itp->first;
        auto property = getPhysicalProperty(propertyName);
        try {
            auto parentProperty = parent->getPhysicalProperty(propertyName);

            if (*property != *parentProperty) {
                return true;
            }
        }
        catch (const PropertyNotFound&) {
            return true;
        }
    }

    return false;
}

bool Material::modelAppearanceChanged(const std::shared_ptr<Material> parent,
                                      const std::shared_ptr<Model> model) const
{
    for (auto itp = model->begin(); itp != model->end(); itp++) {
        QString propertyName = itp->first;
        auto property = getAppearanceProperty(propertyName);
        try {
            auto parentProperty = parent->getAppearanceProperty(propertyName);

            if (*property != *parentProperty) {
                return true;
            }
        }
        catch (const PropertyNotFound&) {
            return true;
        }
    }

    return false;
}

void Material::saveModels(QTextStream& stream, bool saveInherited) const
{
    if (!_physical.empty()) {
        ModelManager modelManager;
        MaterialManager materialManager;

        bool inherited = saveInherited && (_parentUuid.size() > 0);
        std::shared_ptr<Material> parent;
        if (inherited) {
            try {
                parent = materialManager.getMaterial(_parentUuid);
            }
            catch (const MaterialNotFound&) {
                inherited = false;
            }
        }

        bool headerPrinted = false;
        for (auto itm = _physicalUuids.begin(); itm != _physicalUuids.end(); itm++) {
            auto model = modelManager.getModel(*itm);
            if (!inherited || modelChanged(parent, model)) {
                if (!headerPrinted) {
                    stream << "Models:\n";
                    headerPrinted = true;
                }
                stream << "  " << model->getName() << ":\n";
                stream << "    UUID: \"" << model->getUUID() << "\"\n";
                for (auto itp = model->begin(); itp != model->end(); itp++) {
                    QString propertyName = itp->first;
                    std::shared_ptr<MaterialProperty> property = getPhysicalProperty(propertyName);
                    std::shared_ptr<MaterialProperty> parentProperty;
                    try {
                        if (inherited) {
                            parentProperty = parent->getPhysicalProperty(propertyName);
                        }
                    }
                    catch (const PropertyNotFound&) {
                        Base::Console().Log("Material::saveModels Property not found '%s'\n",
                                            propertyName.toStdString().c_str());
                    }

                    if (!inherited || (*property != *parentProperty)) {
                        if (!property->isNull()) {
                            stream << "    " << *property << "\n";
                        }
                    }
                }
            }
        }
    }
}

void Material::saveAppearanceModels(QTextStream& stream, bool saveInherited) const
{
    if (!_appearance.empty()) {
        ModelManager modelManager;
        MaterialManager materialManager;

        bool inherited = saveInherited && (_parentUuid.size() > 0);
        std::shared_ptr<Material> parent;
        if (inherited) {
            try {
                parent = materialManager.getMaterial(_parentUuid);
            }
            catch (const MaterialNotFound&) {
                inherited = false;
            }
        }

        bool headerPrinted = false;
        for (auto itm = _appearanceUuids.begin(); itm != _appearanceUuids.end(); itm++) {
            auto model = modelManager.getModel(*itm);
            if (!inherited || modelAppearanceChanged(parent, model)) {
                if (!headerPrinted) {
                    stream << "AppearanceModels:\n";
                    headerPrinted = true;
                }
                stream << "  " << model->getName() << ":\n";
                stream << "    UUID: \"" << model->getUUID() << "\"\n";
                for (auto itp = model->begin(); itp != model->end(); itp++) {
                    QString propertyName = itp->first;
                    std::shared_ptr<MaterialProperty> property =
                        getAppearanceProperty(propertyName);
                    std::shared_ptr<MaterialProperty> parentProperty;
                    try {
                        if (inherited) {
                            parentProperty = parent->getAppearanceProperty(propertyName);
                        }
                    }
                    catch (const PropertyNotFound&) {
                    }

                    if (!inherited || (*property != *parentProperty)) {
                        if (!property->isNull()) {
                            stream << "    " << *property << "\n";
                        }
                    }
                }
            }
        }
    }
}

void Material::newUuid()
{
    _uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString Material::getModelByName(const QString& name) const
{
    ModelManager manager;

    for (auto it = _allUuids.begin(); it != _allUuids.end(); it++) {
        try {
            auto model = manager.getModel(*it);
            if (model->getName() == name) {
                return *it;
            }
        }
        catch (ModelNotFound const&) {
        }
    }

    return QString();
}

void Material::save(QTextStream& stream, bool saveAsCopy, bool saveInherited)
{
    if (saveInherited && !saveAsCopy) {
        // Check to see if we're an original or if we're already in the list of models
        MaterialManager materialManager;
        if (materialManager.exists(_uuid)) {
            // Make a new version based on the current
            setParentUUID(_uuid);
            newUuid();
        }
    }

    if (saveAsCopy) {
        // Save it in the same format as the parent
        if (_parentUuid.isEmpty()) {
            saveInherited = false;
        }
        else {
            saveInherited = true;
        }
    }

    stream << "---\n";
    stream << "# File created by " << QString::fromStdString(App::Application::Config()["ExeName"])
           << " " << QString::fromStdString(App::Application::Config()["ExeVersion"])
           << " Revision: " << QString::fromStdString(App::Application::Config()["BuildRevision"])
           << "\n";
    saveGeneral(stream);
    if (saveInherited) {
        saveInherits(stream);
    }
    saveModels(stream, saveInherited);
    saveAppearanceModels(stream, saveInherited);
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
    _author = other._author;
    _license = other._license;
    _parentUuid = other._parentUuid;
    _description = other._description;
    _url = other._url;
    _reference = other._reference;
    _dereferenced = other._dereferenced;
    _editState = other._editState;

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

    // Create copies of the properties rather than modify the originals
    _physical.clear();
    for (auto it = other._physical.begin(); it != other._physical.end(); it++) {
        MaterialProperty prop(it->second);
        _physical[it->first] = std::make_shared<MaterialProperty>(prop);
    }
    _appearance.clear();
    for (auto it = other._appearance.begin(); it != other._appearance.end(); it++) {
        MaterialProperty prop(it->second);
        _appearance[it->first] = std::make_shared<MaterialProperty>(prop);
    }

    return *this;
}

/*
 * Normalize models by removing any inherited models
 */
QStringList Material::normalizeModels(const QStringList& models)
{
    QStringList normalized;

    ModelManager manager;

    for (auto uuid : models) {
        auto model = manager.getModel(uuid);

        bool found = false;
        for (auto childUuid : models) {
            if (uuid != childUuid) {
                auto childModel = manager.getModel(childUuid);
                if (childModel->inherits(childUuid)) {
                    // We're an inherited model
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            normalized << uuid;
        }
    }

    return normalized;
}

/*
 * Set or change the base material for the current material, updating the properties as
 * required.
 */
void Material::updateInheritance(const QString& parent)
{}

/*
 * Return a list of models that are defined in the parent material but not in this one
 */
QStringList Material::inheritedMissingModels(const Material& parent)
{
    QStringList missing;
    for (auto uuid : parent._allUuids) {
        if (!hasModel(uuid)) {
            missing << uuid;
        }
    }

    return normalizeModels(missing);
}

/*
 * Return a list of models that are defined in this model but not the parent
 */
QStringList Material::inheritedAddedModels(const Material& parent)
{
    QStringList added;
    for (auto uuid : _allUuids) {
        if (!parent.hasModel(uuid)) {
            added << uuid;
        }
    }

    return normalizeModels(added);
}

/*
 * Return a list of properties that have different values from the parent material
 */
void Material::inheritedPropertyDiff(const QString& parent)
{}

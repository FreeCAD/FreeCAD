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

#include <App/Application.h>
#include <Base/Quantity.h>
#include <Gui/MetaTypes.h>

#include "Exceptions.h"
#include "MaterialValue.h"


using namespace Materials;

/* TRANSLATOR Material::MaterialValue */

TYPESYSTEM_SOURCE(Materials::MaterialValue, Base::BaseClass)

MaterialValue::MaterialValue()
    : _valueType(None)
{
    this->setInitialValue(None);
}

MaterialValue::MaterialValue(const MaterialValue& other)
    : _valueType(other._valueType)
    , _value(other._value)
{}

MaterialValue::MaterialValue(ValueType type)
    : _valueType(type)
{
    this->setInitialValue(None);
}

MaterialValue::MaterialValue(ValueType type, ValueType inherited)
    : _valueType(type)
{
    this->setInitialValue(inherited);
}

MaterialValue& MaterialValue::operator=(const MaterialValue& other)
{
    if (this == &other) {
        return *this;
    }

    _valueType = other._valueType;
    _value = other._value;

    return *this;
}

bool MaterialValue::operator==(const MaterialValue& other) const
{
    if (this == &other) {
        return true;
    }

    return (_valueType == other._valueType) && (_value == other._value);
}

void MaterialValue::setInitialValue(ValueType inherited)
{
    if (_valueType == String) {
        _value = QVariant(static_cast<QVariant::Type>(QMetaType::QString));
    }
    else if (_valueType == Boolean) {
        _value = QVariant(static_cast<QVariant::Type>(QMetaType::Bool));
    }
    else if (_valueType == Integer) {
        _value = QVariant(static_cast<QVariant::Type>(QMetaType::Int));
    }
    else if (_valueType == Float) {
        _value = QVariant(static_cast<QVariant::Type>(QMetaType::Float));
    }
    else if (_valueType == URL) {
        _value = QVariant(static_cast<QVariant::Type>(QMetaType::QString));
    }
    else if (_valueType == Quantity) {
        Base::Quantity q;
        q.setInvalid();
        _value = QVariant::fromValue(q);
    }
    else if (_valueType == Color) {
        _value = QVariant(static_cast<QVariant::Type>(QMetaType::QString));
    }
    else if (_valueType == File) {
        _value = QVariant(static_cast<QVariant::Type>(QMetaType::QString));
    }
    else if (_valueType == Image) {
        _value = QVariant(static_cast<QVariant::Type>(QMetaType::QString));
    }
    else if (_valueType == List) {
        _value = QVariant(static_cast<QVariant::Type>(QMetaType::QString));
    }
    else if (_valueType == Array2D) {
        if (_valueType != inherited) {
            throw InvalidMaterialType("Initializing a regular material value as a 2D Array");
        }

        _value = QVariant();  // Uninitialized default value
    }
    else if (_valueType == Array3D) {
        if (_valueType != inherited) {
            throw InvalidMaterialType("Initializing a regular material value as a 3D Array");
        }

        _value = QVariant();  // Uninitialized default value
    }
    else {
        // Default is to set the type to None and leave the variant uninitialized
        _valueType = None;
        _value = QVariant();
    }
}

bool MaterialValue::isNull() const
{
    if (_value.isNull()) {
        return true;
    }

    if (_valueType == Quantity) {
        return !_value.value<Base::Quantity>().isValid();
    }

    return false;
}

const QString MaterialValue::getYAMLString() const
{
    QString yaml = QString::fromStdString("\"");
    if (!isNull()) {
        if (getType() == MaterialValue::Quantity) {
            Base::Quantity quantity = getValue().value<Base::Quantity>();
            yaml += quantity.getUserString();
        }
        else if (getType() == MaterialValue::Float) {
            auto value = getValue();
            if (!value.isNull()) {
                yaml += QString(QString::fromStdString("%1")).arg(value.toFloat(), 0, 'g', 6);
            }
        }
        else {
            yaml += getValue().toString();
        }
    }
    yaml += QString::fromStdString("\"");
    return yaml;
}

//===

TYPESYSTEM_SOURCE(Materials::Material2DArray, Materials::MaterialValue)

Material2DArray::Material2DArray()
    : MaterialValue(Array2D, Array2D)
    , _defaultSet(false)
{
    // Initialize separatelt to prevent recursion
    // setType(Array2D);
}

Material2DArray::Material2DArray(const Material2DArray& other)
    : MaterialValue(other)
    , _defaultSet(other._defaultSet)
{
    deepCopy(other);
}

Material2DArray& Material2DArray::operator=(const Material2DArray& other)
{
    if (this == &other) {
        return *this;
    }

    MaterialValue::operator=(other);
    _defaultSet = other._defaultSet;

    deepCopy(other);

    return *this;
}

void Material2DArray::deepCopy(const Material2DArray& other)
{
    // Deep copy
    for (auto row : other._rows) {
        std::vector<QVariant> v;
        for (auto col : *row) {
            QVariant newVariant(col);
            v.push_back(newVariant);
        }
        addRow(std::make_shared<std::vector<QVariant>>(v));
    }
}

bool Material2DArray::isNull() const
{
    return rows() <= 0;
}

const QVariant Material2DArray::getDefault() const
{
    if (_defaultSet) {
        return _value;
    }

    return QVariant();
}

std::shared_ptr<std::vector<QVariant>> Material2DArray::getRow(int row) const
{
    try {
        return _rows.at(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidRow();
    }
}

std::shared_ptr<std::vector<QVariant>> Material2DArray::getRow(int row)
{
    try {
        return _rows.at(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidRow();
    }
}

void Material2DArray::addRow(std::shared_ptr<std::vector<QVariant>> row)
{
    _rows.push_back(row);
}

void Material2DArray::insertRow(int index, std::shared_ptr<std::vector<QVariant>> row)
{
    _rows.insert(_rows.begin() + index, row);
}

void Material2DArray::deleteRow(int row)
{
    if (static_cast<std::size_t>(row) >= _rows.size() || row < 0) {
        throw InvalidRow();
    }
    _rows.erase(_rows.begin() + row);
}

void Material2DArray::setValue(int row, int column, const QVariant& value)
{
    if (row >= rows()) {
        throw InvalidIndex();
    }

    auto val = getRow(row);
    try {
        val->at(column) = value;
    }
    catch (const std::out_of_range&) {
        throw InvalidIndex();
    }
}

const QVariant Material2DArray::getValue(int row, int column) const
{
    try {
        auto val = getRow(row);
        try {
            return val->at(column);
        }
        catch (std::out_of_range const&) {
            throw InvalidIndex();
        }
    }
    catch (const InvalidRow&) {
        throw InvalidIndex();
    }
}

void Material2DArray::dumpRow(std::shared_ptr<std::vector<QVariant>> row) const
{
    Base::Console().Log("row: ");
    for (auto column : *row) {
        Base::Console().Log("'%s' ", column.toString().toStdString().c_str());
    }
    Base::Console().Log("\n");
}

void Material2DArray::dump() const
{
    for (auto row : _rows) {
        dumpRow(row);
    }
}

const QString Material2DArray::getYAMLString() const
{
    if (isNull()) {
        return QString();
    }

    // Set the correct indentation. 9 chars in this case
    QString pad;
    pad.fill(QChar::fromLatin1(' '), 9);

    // Save the default value
    QString yaml = QString::fromStdString("\n      - \"");
    Base::Quantity quantity = getDefault().value<Base::Quantity>();
    yaml += quantity.getUserString();
    yaml += QString::fromStdString("\"\n");

    // Next the array contents
    yaml += QString::fromStdString("      - [");
    bool firstRow = true;
    for (auto row : _rows) {
        if (!firstRow) {
            // Each row is on its own line, padded for correct indentation
            yaml += QString::fromStdString(",\n") + pad;
        }
        else {
            firstRow = false;
        }
        yaml += QString::fromStdString("[");

        bool first = true;
        for (auto column : *row) {
            if (!first) {
                // TODO: Fix for arrays with too many columns to fit on a single line
                yaml += QString::fromStdString(", ");
            }
            else {
                first = false;
            }
            yaml += QString::fromStdString("\"");
            Base::Quantity quantity = column.value<Base::Quantity>();
            yaml += quantity.getUserString();
            yaml += QString::fromStdString("\"");
        }

        yaml += QString::fromStdString("]");
    }
    yaml += QString::fromStdString("]");
    return yaml;
}

//===

TYPESYSTEM_SOURCE(Materials::Material3DArray, Materials::MaterialValue)

Material3DArray::Material3DArray()
    : MaterialValue(Array3D, Array3D)
    , _defaultSet(false)
    , _currentDepth(0)
{
    // Initialize separatelt to prevent recursion
    // setType(Array3D);
}

bool Material3DArray::isNull() const
{
    return depth() <= 0;
}

const QVariant Material3DArray::getDefault() const
{
    return _value;
}

const std::shared_ptr<std::vector<std::shared_ptr<std::vector<Base::Quantity>>>>&
Material3DArray::getTable(const Base::Quantity& depth) const
{
    for (auto it = _rowMap.begin(); it != _rowMap.end(); it++) {
        if (std::get<0>(*it) == depth) {
            return std::get<1>(*it);
        }
    }

    throw InvalidDepth();
}

const std::shared_ptr<std::vector<std::shared_ptr<std::vector<Base::Quantity>>>>&
Material3DArray::getTable(int depthIndex) const
{
    try {
        return std::get<1>(_rowMap.at(depthIndex));
    }
    catch (std::out_of_range const&) {
        throw InvalidDepth();
    }
}

const std::shared_ptr<std::vector<Base::Quantity>> Material3DArray::getRow(int depth, int row) const
{
    try {
        return getTable(depth)->at(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidRow();
    }
}

std::shared_ptr<std::vector<Base::Quantity>> Material3DArray::getRow(int row) const
{
    // Check if we can convert otherwise throw error
    return getRow(_currentDepth, row);
}

std::shared_ptr<std::vector<Base::Quantity>> Material3DArray::getRow(int depth, int row)
{
    try {
        return getTable(depth)->at(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidRow();
    }
}

std::shared_ptr<std::vector<Base::Quantity>> Material3DArray::getRow(int row)
{
    return getRow(_currentDepth, row);
}

void Material3DArray::addRow(int depth, std::shared_ptr<std::vector<Base::Quantity>> row)
{
    try {
        getTable(depth)->push_back(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidRow();
    }
}

void Material3DArray::addRow(std::shared_ptr<std::vector<Base::Quantity>> row)
{
    addRow(_currentDepth, row);
}

int Material3DArray::addDepth(int depth, Base::Quantity value)
{
    if (depth == this->depth()) {
        // Append to the end
        return addDepth(value);
    }
    else if (depth > this->depth()) {
        throw InvalidDepth();
    }
    auto rowVector = std::make_shared<std::vector<std::shared_ptr<std::vector<Base::Quantity>>>>();
    auto entry = std::make_pair(value, rowVector);
    _rowMap.insert(_rowMap.begin() + depth, entry);

    return depth;
}

int Material3DArray::addDepth(Base::Quantity value)
{
    auto rowVector = std::make_shared<std::vector<std::shared_ptr<std::vector<Base::Quantity>>>>();
    auto entry = std::make_pair(value, rowVector);
    _rowMap.push_back(entry);

    return depth() - 1;
}

void Material3DArray::deleteDepth(int depth)
{
    deleteRows(depth);  // This may throw an InvalidDepth
    _rowMap.erase(_rowMap.begin() + depth);
}

void Material3DArray::insertRow(int depth,
                                int row,
                                std::shared_ptr<std::vector<Base::Quantity>> rowData)
{
    try {
        auto table = getTable(depth);
        // auto it = table->begin();
        // std::advance(it, row);
        table->insert(table->begin() + row, rowData);
    }
    catch (std::out_of_range const&) {
        throw InvalidRow();
    }
}

void Material3DArray::insertRow(int row, std::shared_ptr<std::vector<Base::Quantity>> rowData)
{
    insertRow(_currentDepth, row, rowData);
}

void Material3DArray::deleteRow(int depth, int row)
{
    auto table = getTable(depth);
    if (static_cast<std::size_t>(row) >= table->size() || row < 0) {
        throw InvalidRow();
    }
    table->erase(table->begin() + row);
}

void Material3DArray::deleteRow(int row)
{
    deleteRow(_currentDepth, row);
}

void Material3DArray::deleteRows(int depth)
{
    auto table = getTable(depth);
    table->clear();
}

void Material3DArray::deleteRows()
{
    deleteRows(_currentDepth);
}

int Material3DArray::rows(int depth) const
{
    if (depth < 0 || (depth == 0 && this->depth() == 0)) {
        return 0;
    }

    return getTable(depth)->size();
}

int Material3DArray::columns(int depth) const
{
    if (depth < 0 || (depth == 0 && this->depth() == 0)) {
        return 0;
    }
    if (rows() == 0) {
        return 0;
    }

    return getTable(depth)->at(0)->size();
}

void Material3DArray::setValue(int depth, int row, int column, const Base::Quantity& value)
{
    auto val = getRow(depth, row);
    try {
        val->at(column) = value;
    }
    catch (std::out_of_range const&) {
        throw InvalidColumn();
    }
}

void Material3DArray::setValue(int row, int column, const Base::Quantity& value)
{
    setValue(_currentDepth, row, column, value);
}

void Material3DArray::setDepthValue(int depth, const Base::Quantity& value)
{
    try {
        auto oldRows = getTable(depth);
        _rowMap.at(depth) = std::pair(value, oldRows);
    }
    catch (std::out_of_range const&) {
        throw InvalidRow();
    }
}

void Material3DArray::setDepthValue(const Base::Quantity& value)
{
    setDepthValue(_currentDepth, value);
}


const Base::Quantity Material3DArray::getValue(int depth, int row, int column) const
{
    auto val = getRow(depth, row);
    try {
        return val->at(column);
    }
    catch (std::out_of_range const&) {
        throw InvalidColumn();
    }
}

const Base::Quantity Material3DArray::getValue(int row, int column) const
{
    return getValue(_currentDepth, row, column);
}

const Base::Quantity Material3DArray::getDepthValue(int depth) const
{
    try {
        return std::get<0>(_rowMap.at(depth));
    }
    catch (std::out_of_range const&) {
        throw InvalidRow();
    }
}

int Material3DArray::currentDepth() const
{
    return _currentDepth;
}

void Material3DArray::setCurrentDepth(int depth)
{
    if (depth < 0 || _rowMap.size() == 0) {
        _currentDepth = 0;
    }
    else if (static_cast<std::size_t>(depth) >= _rowMap.size()) {
        _currentDepth = _rowMap.size() - 1;
    }
    else {
        _currentDepth = depth;
    }
}

const QString Material3DArray::getYAMLString() const
{
    if (isNull()) {
        return QString();
    }

    // Set the correct indentation. 7 chars + name length
    QString pad;
    pad.fill(QChar::fromLatin1(' '), 9);

    // Save the default value
    QString yaml = QString::fromStdString("\n      - \"");
    Base::Quantity quantity = getDefault().value<Base::Quantity>();
    yaml += quantity.getUserString();
    yaml += QString::fromStdString("\"\n");

    // Next the array contents
    yaml += QString::fromStdString("      - [");
    for (int depth = 0; depth < this->depth(); depth++) {
        if (depth > 0) {
            // Each row is on its own line, padded for correct indentation
            yaml += QString::fromStdString(",\n") + pad;
        }

        yaml += QString::fromStdString("\"");
        auto value = getDepthValue(depth).getUserString();
        yaml += value;
        yaml += QString::fromStdString("\": [");

        QString pad2;
        pad2.fill(QChar::fromLatin1(' '), 14 + value.length());

        bool firstRow = true;
        auto rows = getTable(depth);
        for (auto row : *rows) {
            if (!firstRow) {
                // Each row is on its own line, padded for correct indentation
                yaml += QString::fromStdString(",\n") + pad2;
            }
            else {
                firstRow = false;
            }
            yaml += QString::fromStdString("[");

            bool first = true;
            for (auto column : *row) {
                if (!first) {
                    // TODO: Fix for arrays with too many columns to fit on a single line
                    yaml += QString::fromStdString(", ");
                }
                else {
                    first = false;
                }
                yaml += QString::fromStdString("\"");
                // Base::Quantity quantity = column.value<Base::Quantity>();
                yaml += column.getUserString();
                yaml += QString::fromStdString("\"");
            }

            yaml += QString::fromStdString("]");
        }
        yaml += QString::fromStdString("]");
    }
    yaml += QString::fromStdString("]");
    return yaml;
}

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
#include <QMetaType>
#include <QRegularExpression>
#endif

#include <App/Application.h>
#include <Base/QtTools.h>
#include <Base/Quantity.h>
#include <Gui/MetaTypes.h>

#include "Exceptions.h"
#include "MaterialValue.h"


using namespace Materials;

/* TRANSLATOR Material::MaterialValue */

TYPESYSTEM_SOURCE(Materials::MaterialValue, Base::BaseClass)

QMap<QString, MaterialValue::ValueType> MaterialValue::_typeMap {
    {QString::fromStdString("String"), String},
    {QString::fromStdString("Boolean"), Boolean},
    {QString::fromStdString("Integer"), Integer},
    {QString::fromStdString("Float"), Float},
    {QString::fromStdString("Quantity"), Quantity},
    {QString::fromStdString("Distribution"), Distribution},
    {QString::fromStdString("List"), List},
    {QString::fromStdString("2DArray"), Array2D},
    {QString::fromStdString("3DArray"), Array3D},
    {QString::fromStdString("Color"), Color},
    {QString::fromStdString("Image"), Image},
    {QString::fromStdString("File"), File},
    {QString::fromStdString("URL"), URL},
    {QString::fromStdString("MultiLineString"), MultiLineString},
    {QString::fromStdString("FileList"), FileList},
    {QString::fromStdString("ImageList"), ImageList},
    {QString::fromStdString("SVG"), SVG}};

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

QString MaterialValue::escapeString(const QString& source)
{
    QString res = source;
    res.replace(QString::fromStdString("\\"), QString::fromStdString("\\\\"));
    res.replace(QString::fromStdString("\""), QString::fromStdString("\\\""));
    return res;
}

MaterialValue::ValueType MaterialValue::mapType(const QString& stringType)
{
    // If not found, return None
    return _typeMap.value(stringType, None);
}

void MaterialValue::setInitialValue(ValueType inherited)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (_valueType == String || _valueType == MultiLineString || _valueType == SVG) {
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
    else if (_valueType == Color) {
        _value = QVariant(static_cast<QVariant::Type>(QMetaType::QString));
    }
    else if (_valueType == File) {
        _value = QVariant(static_cast<QVariant::Type>(QMetaType::QString));
    }
    else if (_valueType == Image) {
        _value = QVariant(static_cast<QVariant::Type>(QMetaType::QString));
    }
#else
    if (_valueType == String || _valueType == MultiLineString || _valueType == SVG) {
        _value = QVariant(QMetaType(QMetaType::QString));
    }
    else if (_valueType == Boolean) {
        _value = QVariant(QMetaType(QMetaType::Bool));
    }
    else if (_valueType == Integer) {
        _value = QVariant(QMetaType(QMetaType::Int));
    }
    else if (_valueType == Float) {
        _value = QVariant(QMetaType(QMetaType::Float));
    }
    else if (_valueType == URL) {
        _value = QVariant(QMetaType(QMetaType::QString));
    }
    else if (_valueType == Color) {
        _value = QVariant(QMetaType(QMetaType::QString));
    }
    else if (_valueType == File) {
        _value = QVariant(QMetaType(QMetaType::QString));
    }
    else if (_valueType == Image) {
        _value = QVariant(QMetaType(QMetaType::QString));
    }
#endif
    else if (_valueType == Quantity) {
        Base::Quantity qu;
        qu.setInvalid();
        _value = QVariant::fromValue(qu);
    }
    else if (_valueType == List || _valueType == FileList || _valueType == ImageList) {
        auto list = QList<QVariant>();
        _value = QVariant::fromValue(list);
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

void MaterialValue::setList(const QList<QVariant>& value)
{
    _value = QVariant::fromValue(value);
}

bool MaterialValue::isNull() const
{
    if (_value.isNull()) {
        return true;
    }

    if (_valueType == Quantity) {
        return !_value.value<Base::Quantity>().isValid();
    }

    if (_valueType == List || _valueType == FileList || _valueType == ImageList) {
        return _value.value<QList<QVariant>>().isEmpty();
    }

    return false;
}

QString MaterialValue::getYAMLStringImage() const
{
    QString yaml;
    yaml = QString::fromStdString(" |-2");
    QString base64 = getValue().toString();
    while (!base64.isEmpty()) {
        yaml += QString::fromStdString("\n      ") + base64.left(74);
        base64.remove(0, 74);
    }
    return yaml;
}

QString MaterialValue::getYAMLStringList() const
{
    QString yaml;
    for (auto& it : getList()) {
        yaml += QString::fromStdString("\n      - \"") + escapeString(it.toString())
            + QString::fromStdString("\"");
    }
    return yaml;
}

QString MaterialValue::getYAMLStringImageList() const
{
    QString yaml;
    for (auto& it : getList()) {
        yaml += QString::fromStdString("\n      - |-2");
        QString base64 = it.toString();
        while (!base64.isEmpty()) {
            yaml += QString::fromStdString("\n        ") + base64.left(72);
            base64.remove(0, 72);
        }
    }
    return yaml;
}

QString MaterialValue::getYAMLStringMultiLine() const
{
    QString yaml;
    yaml = QString::fromStdString(" |2");
    auto list =
        getValue().toString().split(QRegularExpression(QString::fromStdString("[\r\n]")), Qt::SkipEmptyParts);
    for (auto& it : list) {
        yaml += QString::fromStdString("\n      ") + it;
    }
    return yaml;
}

QString MaterialValue::getYAMLString() const
{
    QString yaml;
    if (!isNull()) {
        if (getType() == MaterialValue::Image) {
            return getYAMLStringImage();
        }
        if (getType() == MaterialValue::List || getType() == MaterialValue::FileList) {
            return getYAMLStringList();
        }
        if (getType() == MaterialValue::ImageList) {
            return getYAMLStringImageList();
        }
        if (getType() == MaterialValue::MultiLineString || getType() == MaterialValue::SVG) {
            return getYAMLStringMultiLine();
        }
        if (getType() == MaterialValue::Quantity) {
            auto quantity = getValue().value<Base::Quantity>();
            yaml += quantity.getUserString();
        }
        else if (getType() == MaterialValue::Float) {
            auto value = getValue();
            if (!value.isNull()) {
                yaml += QString::fromLatin1("%1").arg(value.toFloat(), 0, 'g', 6);
            }
        }
        else if (getType() == MaterialValue::List) {
            for (auto& it : getList()) {
                yaml += QLatin1String("\n      - \"") + escapeString(it.toString())
                    + QLatin1String("\"");
            }
            return yaml;
        }
        else {
            yaml += getValue().toString();
        }
    }
    yaml = QLatin1String(" \"") + escapeString(yaml) + QLatin1String("\"");
    return yaml;
}

//===

TYPESYSTEM_SOURCE(Materials::Material2DArray, Materials::MaterialValue)

Material2DArray::Material2DArray()
    : MaterialValue(Array2D, Array2D)
    , _columns(0)
{
    // Initialize separatelt to prevent recursion
    // setType(Array2D);
}

Material2DArray::Material2DArray(const Material2DArray& other)
    : MaterialValue(other)
    , _columns(other._columns)
{
    deepCopy(other);
}

Material2DArray& Material2DArray::operator=(const Material2DArray& other)
{
    if (this == &other) {
        return *this;
    }

    MaterialValue::operator=(other);
    _columns = other._columns;

    deepCopy(other);

    return *this;
}

void Material2DArray::deepCopy(const Material2DArray& other)
{
    // Deep copy
    for (auto& row : other._rows) {
        QList<QVariant> vv;
        for (auto& col : *row) {
            QVariant newVariant(col);
            vv.push_back(newVariant);
        }
        addRow(std::make_shared<QList<QVariant>>(vv));
    }
}

bool Material2DArray::isNull() const
{
    return rows() <= 0;
}

void Material2DArray::validateRow(int row) const
{
    if (row < 0 || row >= rows()) {
        throw InvalidIndex();
    }
}

void Material2DArray::validateColumn(int column) const
{
    if (column < 0 || column >= columns()) {
        throw InvalidIndex();
    }
}

std::shared_ptr<QList<QVariant>> Material2DArray::getRow(int row) const
{
    validateRow(row);

    try {
        return _rows.at(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

std::shared_ptr<QList<QVariant>> Material2DArray::getRow(int row)
{
    validateRow(row);

    try {
        return _rows.at(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

void Material2DArray::addRow(const std::shared_ptr<QList<QVariant>>& row)
{
    _rows.push_back(row);
}

void Material2DArray::insertRow(int index, const std::shared_ptr<QList<QVariant>>& row)
{
    _rows.insert(_rows.begin() + index, row);
}

void Material2DArray::deleteRow(int row)
{
    if (row >= static_cast<int>(_rows.size()) || row < 0) {
        throw InvalidIndex();
    }
    _rows.erase(_rows.begin() + row);
}

void Material2DArray::setValue(int row, int column, const QVariant& value)
{
    validateRow(row);
    validateColumn(column);

    auto val = getRow(row);
    try {
        val->replace(column, value);
    }
    catch (const std::out_of_range&) {
        throw InvalidIndex();
    }
}

QVariant Material2DArray::getValue(int row, int column) const
{
    validateColumn(column);

    auto val = getRow(row);
    try {
        return val->at(column);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

void Material2DArray::dumpRow(const std::shared_ptr<QList<QVariant>>& row)
{
    Base::Console().Log("row: ");
    for (auto& column : *row) {
        Base::Console().Log("'%s' ", column.toString().toStdString().c_str());
    }
    Base::Console().Log("\n");
}

void Material2DArray::dump() const
{
    for (auto& row : _rows) {
        dumpRow(row);
    }
}

QString Material2DArray::getYAMLString() const
{
    if (isNull()) {
        return QString();
    }

    // Set the correct indentation. 9 chars in this case
    QString pad;
    pad.fill(QChar::fromLatin1(' '), 9);

    // Save the array contents
    QString yaml = QString::fromStdString("\n      - [");
    bool firstRow = true;
    for (auto& row : _rows) {
        if (!firstRow) {
            // Each row is on its own line, padded for correct indentation
            yaml += QString::fromStdString(",\n") + pad;
        }
        else {
            firstRow = false;
        }
        yaml += QString::fromStdString("[");

        bool first = true;
        for (auto& column : *row) {
            if (!first) {
                // TODO: Fix for arrays with too many columns to fit on a single line
                yaml += QString::fromStdString(", ");
            }
            else {
                first = false;
            }
            yaml += QString::fromStdString("\"");
            auto quantity = column.value<Base::Quantity>();
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
    , _currentDepth(0)
    , _columns(0)
{
    // Initialize separatelt to prevent recursion
    // setType(Array3D);
}

bool Material3DArray::isNull() const
{
    return depth() <= 0;
}

void Material3DArray::validateDepth(int level) const
{
    if (level < 0 || level >= depth()) {
        throw InvalidIndex();
    }
}

void Material3DArray::validateColumn(int column) const
{
    if (column < 0 || column >= columns()) {
        throw InvalidIndex();
    }
}

void Material3DArray::validateRow(int level, int row) const
{
    validateDepth(level);

    if (row < 0 || row >= rows(level)) {
        throw InvalidIndex();
    }
}

const std::shared_ptr<QList<std::shared_ptr<QList<Base::Quantity>>>>&
Material3DArray::getTable(const Base::Quantity& depth) const
{
    for (auto& it : _rowMap) {
        if (std::get<0>(it) == depth) {
            return std::get<1>(it);
        }
    }

    throw InvalidIndex();
}

const std::shared_ptr<QList<std::shared_ptr<QList<Base::Quantity>>>>&
Material3DArray::getTable(int depthIndex) const
{
    try {
        return std::get<1>(_rowMap.at(depthIndex));
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

std::shared_ptr<QList<Base::Quantity>> Material3DArray::getRow(int depth, int row) const
{
    validateRow(depth, row);

    try {
        return getTable(depth)->at(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

std::shared_ptr<QList<Base::Quantity>> Material3DArray::getRow(int row) const
{
    // Check if we can convert otherwise throw error
    return getRow(_currentDepth, row);
}

std::shared_ptr<QList<Base::Quantity>> Material3DArray::getRow(int depth, int row)
{
    validateRow(depth, row);

    try {
        return getTable(depth)->at(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

std::shared_ptr<QList<Base::Quantity>> Material3DArray::getRow(int row)
{
    return getRow(_currentDepth, row);
}

void Material3DArray::addRow(int depth, const std::shared_ptr<QList<Base::Quantity>>& row)
{
    try {
        getTable(depth)->push_back(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

void Material3DArray::addRow(const std::shared_ptr<QList<Base::Quantity>>& row)
{
    addRow(_currentDepth, row);
}

int Material3DArray::addDepth(int depth, const Base::Quantity& value)
{
    if (depth == this->depth()) {
        // Append to the end
        return addDepth(value);
    }
    if (depth > this->depth()) {
        throw InvalidIndex();
    }
    auto rowVector = std::make_shared<QList<std::shared_ptr<QList<Base::Quantity>>>>();
    auto entry = std::make_pair(value, rowVector);
    _rowMap.insert(_rowMap.begin() + depth, entry);

    return depth;
}

int Material3DArray::addDepth(const Base::Quantity& value)
{
    auto rowVector = std::make_shared<QList<std::shared_ptr<QList<Base::Quantity>>>>();
    auto entry = std::make_pair(value, rowVector);
    _rowMap.push_back(entry);

    return depth() - 1;
}

void Material3DArray::deleteDepth(int depth)
{
    deleteRows(depth);  // This may throw an InvalidIndex
    _rowMap.erase(_rowMap.begin() + depth);
}

void Material3DArray::insertRow(int depth,
                                int row,
                                const std::shared_ptr<QList<Base::Quantity>>& rowData)
{
    try {
        auto table = getTable(depth);
        table->insert(table->begin() + row, rowData);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

void Material3DArray::insertRow(int row, const std::shared_ptr<QList<Base::Quantity>>& rowData)
{
    insertRow(_currentDepth, row, rowData);
}

void Material3DArray::deleteRow(int depth, int row)
{
    auto table = getTable(depth);
    if (row >= static_cast<int>(table->size()) || row < 0) {
        throw InvalidIndex();
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
    validateDepth(depth);

    return getTable(depth)->size();
}

void Material3DArray::setValue(int depth, int row, int column, const Base::Quantity& value)
{
    validateRow(depth, row);
    validateColumn(column);

    auto val = getRow(depth, row);
    try {
        val->replace(column, value);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
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
        _rowMap.replace(depth, std::pair(value, oldRows));
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

void Material3DArray::setDepthValue(const Base::Quantity& value)
{
    setDepthValue(_currentDepth, value);
}


Base::Quantity Material3DArray::getValue(int depth, int row, int column) const
{
    // getRow validates depth and row. Do that first
    auto val = getRow(depth, row);
    validateColumn(column);

    try {
        return val->at(column);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

Base::Quantity Material3DArray::getValue(int row, int column) const
{
    return getValue(_currentDepth, row, column);
}

Base::Quantity Material3DArray::getDepthValue(int depth) const
{
    validateDepth(depth);

    try {
        return std::get<0>(_rowMap.at(depth));
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

int Material3DArray::currentDepth() const
{
    return _currentDepth;
}

void Material3DArray::setCurrentDepth(int depth)
{
    validateDepth(depth);

    if (depth < 0 || _rowMap.empty()) {
        _currentDepth = 0;
    }
    else if (depth >= static_cast<int>(_rowMap.size())) {
        _currentDepth = _rowMap.size() - 1;
    }
    else {
        _currentDepth = depth;
    }
}

QString Material3DArray::getYAMLString() const
{
    if (isNull()) {
        return QString();
    }

    // Set the correct indentation. 7 chars + name length
    QString pad;
    pad.fill(QChar::fromLatin1(' '), 9);

    // Save the array contents
    QString yaml = QString::fromStdString("\n      - [");
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
        for (auto& row : *rows) {
            if (!firstRow) {
                // Each row is on its own line, padded for correct indentation
                yaml += QString::fromStdString(",\n") + pad2;
            }
            else {
                firstRow = false;
            }
            yaml += QString::fromStdString("[");

            bool first = true;
            for (auto& column : *row) {
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

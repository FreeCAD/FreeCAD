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
#include <Base/Quantity.h>
#include <Gui/MetaTypes.h>

#include "Exceptions.h"
#include "MaterialValue.h"


using namespace Materials;

/* TRANSLATOR Material::MaterialValue */

TYPESYSTEM_SOURCE(Materials::MaterialValue, Base::BaseClass)

QMap<QString, MaterialValue::ValueType> MaterialValue::_typeMap {
    {QStringLiteral("String"), String},
    {QStringLiteral("Boolean"), Boolean},
    {QStringLiteral("Integer"), Integer},
    {QStringLiteral("Float"), Float},
    {QStringLiteral("Quantity"), Quantity},
    {QStringLiteral("Distribution"), Distribution},
    {QStringLiteral("List"), List},
    {QStringLiteral("2DArray"), Array2D},
    {QStringLiteral("3DArray"), Array3D},
    {QStringLiteral("Color"), Color},
    {QStringLiteral("Image"), Image},
    {QStringLiteral("File"), File},
    {QStringLiteral("URL"), URL},
    {QStringLiteral("MultiLineString"), MultiLineString},
    {QStringLiteral("FileList"), FileList},
    {QStringLiteral("ImageList"), ImageList},
    {QStringLiteral("SVG"), SVG}};

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

void MaterialValue::validate(const MaterialValue& other) const
{
    if (_valueType != other._valueType) {
        throw InvalidProperty("Material property value types don't match");
    }
    if (_valueType == Quantity) {
        auto q1 = _value.value<Base::Quantity>();
        auto q2 = other._value.value<Base::Quantity>();
        if (q1.isValid()) {
            if (!q2.isValid()) {
                throw InvalidProperty("Invalid remote Material property quantity value");
            }
            if (q1.getUserString() != q2.getUserString()) {
                // Direct comparisons of the quantities may have precision issues
                // throw InvalidProperty("Material property quantity values don't match");
            }
        }
        else {
            if (q2.isValid()) {
                throw InvalidProperty("Remote Material property quantity should not have a value");
            }
        }
    }
    else if (_valueType == Array2D) {
        auto a1 = static_cast<const Materials::Array2D*>(this);
        auto a2 = static_cast<const Materials::Array2D*>(&other);
        a1->validate(*a2);
    }
    else if (_valueType == Array3D) {
        auto a1 = static_cast<const Materials::Array3D*>(this);
        auto a2 = static_cast<const Materials::Array3D*>(&other);
        a1->validate(*a2);
    }
    else if (!(_value.isNull() && other._value.isNull()) && (_value != other._value)) {
        throw InvalidProperty("Material property values don't match");
    }
}

QString MaterialValue::escapeString(const QString& source)
{
    QString res = source;
    res.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    res.replace(QStringLiteral("\""), QStringLiteral("\\\""));
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
    return isEmpty();
}

bool MaterialValue::isEmpty() const
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
    yaml = QStringLiteral(" |-2");
    QString base64 = getValue().toString();
    while (!base64.isEmpty()) {
        yaml += QStringLiteral("\n      ") + base64.left(74);
        base64.remove(0, 74);
    }
    return yaml;
}

QString MaterialValue::getYAMLStringList() const
{
    QString yaml;
    for (auto& it : getList()) {
        yaml += QStringLiteral("\n      - \"") + escapeString(it.toString())
            + QStringLiteral("\"");
    }
    return yaml;
}

QString MaterialValue::getYAMLStringImageList() const
{
    QString yaml;
    for (auto& it : getList()) {
        yaml += QStringLiteral("\n      - |-2");
        QString base64 = it.toString();
        while (!base64.isEmpty()) {
            yaml += QStringLiteral("\n        ") + base64.left(72);
            base64.remove(0, 72);
        }
    }
    return yaml;
}

QString MaterialValue::getYAMLStringMultiLine() const
{
    QString yaml;
    yaml = QStringLiteral(" |2");
    auto list =
        getValue().toString().split(QRegularExpression(QStringLiteral("[\r\n]")), Qt::SkipEmptyParts);
    for (auto& it : list) {
        yaml += QStringLiteral("\n      ") + it;
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
            yaml += QString::fromStdString(quantity.getUserString());
        }
        else if (getType() == MaterialValue::Float) {
            auto value = getValue();
            if (!value.isNull()) {
                yaml += QStringLiteral("%1").arg(value.toFloat(), 0, 'g', 6);
            }
        }
        else if (getType() == MaterialValue::List) {
            for (auto& it : getList()) {
                yaml += QStringLiteral("\n      - \"") + escapeString(it.toString())
                    + QStringLiteral("\"");
            }
            return yaml;
        }
        else {
            yaml += getValue().toString();
        }
    }
    yaml = QStringLiteral(" \"") + escapeString(yaml) + QStringLiteral("\"");
    return yaml;
}

const Base::QuantityFormat MaterialValue::getQuantityFormat()
{
    return Base::QuantityFormat(Base::QuantityFormat::NumberFormat::Default, PRECISION);
}

//===

TYPESYSTEM_SOURCE(Materials::Array2D, Materials::MaterialValue)

Array2D::Array2D()
    : MaterialValue(MaterialValue::Array2D, MaterialValue::Array2D)
    , _columns(0)
{
    // Initialize separatelt to prevent recursion
    // setType(Array2D);
}

Array2D::Array2D(const Array2D& other)
    : MaterialValue(other)
    , _columns(other._columns)
{
    deepCopy(other);
}

Array2D& Array2D::operator=(const Array2D& other)
{
    if (this == &other) {
        return *this;
    }

    MaterialValue::operator=(other);
    _columns = other._columns;

    deepCopy(other);

    return *this;
}

void Array2D::deepCopy(const Array2D& other)
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

bool Array2D::isNull() const
{
    return isEmpty();
}

bool Array2D::isEmpty() const
{
    return rows() <= 0;
}

void Array2D::validateRow(int row) const
{
    if (row < 0 || row >= rows()) {
        throw InvalidIndex();
    }
}

void Array2D::validateColumn(int column) const
{
    if (column < 0 || column >= columns()) {
        throw InvalidIndex();
    }
}

void Array2D::validate(const Array2D& other) const
{
    if (rows() != other.rows()) {
        Base::Console().log("Local row count %d, remote %d\n", rows(), other.rows());
        throw InvalidProperty("Material property value row counts don't match");
    }
    if (columns() != other.columns()) {
        Base::Console().log("Local column count %d, remote %d\n", columns(), other.columns());
        throw InvalidProperty("Material property value column counts don't match");
    }
    try {
        for (int i = 0; i < rows(); i++) {
            for (int j = 0; j < columns(); j++) {
                if (getValue(i, j) != other.getValue(i, j)) {
                    throw InvalidProperty("Material property values don't match");
                }
            }
        }
    }
    catch (const InvalidIndex&) {
        throw InvalidProperty("Material property value invalid array index");
    }
}

std::shared_ptr<QList<QVariant>> Array2D::getRow(int row) const
{
    validateRow(row);

    try {
        return _rows.at(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

std::shared_ptr<QList<QVariant>> Array2D::getRow(int row)
{
    validateRow(row);

    try {
        return _rows.at(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

void Array2D::addRow(const std::shared_ptr<QList<QVariant>>& row)
{
    _rows.push_back(row);
}

void Array2D::insertRow(int index, const std::shared_ptr<QList<QVariant>>& row)
{
    _rows.insert(_rows.begin() + index, row);
}

void Array2D::deleteRow(int row)
{
    if (row >= static_cast<int>(_rows.size()) || row < 0) {
        throw InvalidIndex();
    }
    _rows.erase(_rows.begin() + row);
}

void Array2D::setRows(int rowCount)
{
    while (rows() < rowCount) {
        auto row = std::make_shared<QList<QVariant>>();
        for (int i = 0; i < columns(); i++) {
            row->append(QVariant());
        }
        addRow(row);
    }
}

void Array2D::setValue(int row, int column, const QVariant& value)
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

QVariant Array2D::getValue(int row, int column) const
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

void Array2D::dumpRow(const std::shared_ptr<QList<QVariant>>& row)
{
    Base::Console().log("row: ");
    for (auto& column : *row) {
        Base::Console().log("'%s' ", column.toString().toStdString().c_str());
    }
    Base::Console().log("\n");
}

void Array2D::dump() const
{
    for (auto& row : _rows) {
        dumpRow(row);
    }
}

QString Array2D::getYAMLString() const
{
    if (isNull()) {
        return QString();
    }

    // Set the correct indentation. 9 chars in this case
    QString pad;
    pad.fill(QChar::fromLatin1(' '), 9);

    // Save the array contents
    QString yaml = QStringLiteral("\n      - [");
    bool firstRow = true;
    for (auto& row : _rows) {
        if (!firstRow) {
            // Each row is on its own line, padded for correct indentation
            yaml += QStringLiteral(",\n") + pad;
        }
        else {
            firstRow = false;
        }
        yaml += QStringLiteral("[");

        bool first = true;
        for (auto& column : *row) {
            if (!first) {
                // TODO: Fix for arrays with too many columns to fit on a single line
                yaml += QStringLiteral(", ");
            }
            else {
                first = false;
            }
            yaml += QStringLiteral("\"");
            auto quantity = column.value<Base::Quantity>();
            yaml += QString::fromStdString(quantity.getUserString());
            yaml += QStringLiteral("\"");
        }

        yaml += QStringLiteral("]");
    }
    yaml += QStringLiteral("]");
    return yaml;
}

//===

TYPESYSTEM_SOURCE(Materials::Array3D, Materials::MaterialValue)

Array3D::Array3D()
    : MaterialValue(MaterialValue::Array3D, MaterialValue::Array3D)
    , _currentDepth(0)
    , _columns(0)
{
    // Initialize separatelt to prevent recursion
    // setType(Array3D);
}

Array3D::Array3D(const Array3D& other)
    : MaterialValue(other)
    , _currentDepth(other._currentDepth)
    , _columns(other._columns)
{
    deepCopy(other);
}

Array3D& Array3D::operator=(const Array3D& other)
{
    if (this == &other) {
        return *this;
    }

    MaterialValue::operator=(other);
    _columns = other._columns;
    _currentDepth = other._currentDepth;

    deepCopy(other);

    return *this;
}

void Array3D::deepCopy(const Array3D& other)
{
    // Deep copy
    _rowMap.clear();
    for (auto& depthTable : other._rowMap) {
        auto depth = addDepth(depthTable.first);
        auto rows = depthTable.second;
        for (auto row : *rows) {
            auto newRow = std::make_shared<QList<Base::Quantity>>();
            for (auto column : *row) {
                newRow->append(column);
            }
            addRow(depth, newRow);
        }
    }
}

bool Array3D::isNull() const
{
    return isEmpty();
}

bool Array3D::isEmpty() const
{
    return depth() <= 0;
}

void Array3D::validateDepth(int level) const
{
    if (level < 0 || level >= depth()) {
        throw InvalidIndex();
    }
}

void Array3D::validateColumn(int column) const
{
    if (column < 0 || column >= columns()) {
        throw InvalidIndex();
    }
}

void Array3D::validateRow(int level, int row) const
{
    validateDepth(level);

    if (row < 0 || row >= rows(level)) {
        throw InvalidIndex();
    }
}

void Array3D::validate(const Array3D& other) const
{
    if (depth() != other.depth()) {
        throw InvalidProperty("Material property value row counts don't match");
    }
    if (columns() != other.columns()) {
        throw InvalidProperty("Material property value column counts don't match");
    }
}

const std::shared_ptr<QList<std::shared_ptr<QList<Base::Quantity>>>>&
Array3D::getTable(const Base::Quantity& depth) const
{
    for (auto& it : _rowMap) {
        if (std::get<0>(it) == depth) {
            return std::get<1>(it);
        }
    }

    throw InvalidIndex();
}

const std::shared_ptr<QList<std::shared_ptr<QList<Base::Quantity>>>>&
Array3D::getTable(int depthIndex) const
{
    try {
        return std::get<1>(_rowMap.at(depthIndex));
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

std::shared_ptr<QList<Base::Quantity>> Array3D::getRow(int depth, int row) const
{
    validateRow(depth, row);

    try {
        return getTable(depth)->at(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

std::shared_ptr<QList<Base::Quantity>> Array3D::getRow(int row) const
{
    // Check if we can convert otherwise throw error
    return getRow(_currentDepth, row);
}

std::shared_ptr<QList<Base::Quantity>> Array3D::getRow(int depth, int row)
{
    validateRow(depth, row);

    try {
        return getTable(depth)->at(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

std::shared_ptr<QList<Base::Quantity>> Array3D::getRow(int row)
{
    return getRow(_currentDepth, row);
}

void Array3D::addRow(int depth, const std::shared_ptr<QList<Base::Quantity>>& row)
{
    try {
        getTable(depth)->push_back(row);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

void Array3D::addRow(const std::shared_ptr<QList<Base::Quantity>>& row)
{
    addRow(_currentDepth, row);
}

int Array3D::addDepth(int depth, const Base::Quantity& value)
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

int Array3D::addDepth(const Base::Quantity& value)
{
    auto rowVector = std::make_shared<QList<std::shared_ptr<QList<Base::Quantity>>>>();
    auto entry = std::make_pair(value, rowVector);
    _rowMap.push_back(entry);

    return depth() - 1;
}

void Array3D::deleteDepth(int depth)
{
    deleteRows(depth);  // This may throw an InvalidIndex
    _rowMap.erase(_rowMap.begin() + depth);
}

void Array3D::setDepth(int depthCount)
{
    Base::Quantity dummy;
    dummy.setInvalid();
    while (depth() < depthCount) {
        addDepth(dummy);
    }
}

void Array3D::insertRow(int depth,
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

void Array3D::insertRow(int row, const std::shared_ptr<QList<Base::Quantity>>& rowData)
{
    insertRow(_currentDepth, row, rowData);
}

void Array3D::deleteRow(int depth, int row)
{
    auto table = getTable(depth);
    if (row >= static_cast<int>(table->size()) || row < 0) {
        throw InvalidIndex();
    }
    table->erase(table->begin() + row);
}

void Array3D::deleteRow(int row)
{
    deleteRow(_currentDepth, row);
}

void Array3D::deleteRows(int depth)
{
    auto table = getTable(depth);
    table->clear();
}

void Array3D::deleteRows()
{
    deleteRows(_currentDepth);
}

int Array3D::rows(int depth) const
{
    if (depth < 0 || (depth == 0 && this->depth() == 0)) {
        return 0;
    }
    validateDepth(depth);

    return getTable(depth)->size();
}

void Array3D::setRows(int depth, int rowCount)
{
    Base::Quantity dummy;
    dummy.setInvalid();

    while (rows(depth) < rowCount) {
        auto row = std::make_shared<QList<Base::Quantity>>();
        for (int i = 0; i < columns(); i++) {
            row->append(dummy);
        }
        addRow(depth, row);
    }
}

void Array3D::setValue(int depth, int row, int column, const Base::Quantity& value)
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

void Array3D::setValue(int row, int column, const Base::Quantity& value)
{
    setValue(_currentDepth, row, column, value);
}

void Array3D::setDepthValue(int depth, const Base::Quantity& value)
{
    try {
        auto oldRows = getTable(depth);
        _rowMap.replace(depth, std::pair(value, oldRows));
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

void Array3D::setDepthValue(const Base::Quantity& value)
{
    setDepthValue(_currentDepth, value);
}


Base::Quantity Array3D::getValue(int depth, int row, int column) const
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

Base::Quantity Array3D::getValue(int row, int column) const
{
    return getValue(_currentDepth, row, column);
}

Base::Quantity Array3D::getDepthValue(int depth) const
{
    validateDepth(depth);

    try {
        return std::get<0>(_rowMap.at(depth));
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

int Array3D::currentDepth() const
{
    return _currentDepth;
}

void Array3D::setCurrentDepth(int depth)
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

QString Array3D::getYAMLString() const
{
    if (isNull()) {
        return QString();
    }

    // Set the correct indentation. 7 chars + name length
    QString pad;
    pad.fill(QChar::fromLatin1(' '), 9);

    // Save the array contents
    QString yaml = QStringLiteral("\n      - [");
    for (int depth = 0; depth < this->depth(); depth++) {
        if (depth > 0) {
            // Each row is on its own line, padded for correct indentation
            yaml += QStringLiteral(",\n") + pad;
        }

        yaml += QStringLiteral("\"");
        auto value = QString::fromStdString(getDepthValue(depth).getUserString());
        yaml += value;
        yaml += QStringLiteral("\": [");

        QString pad2;
        pad2.fill(QChar::fromLatin1(' '), 14 + value.length());

        bool firstRow = true;
        auto rows = getTable(depth);
        for (auto& row : *rows) {
            if (!firstRow) {
                // Each row is on its own line, padded for correct indentation
                yaml += QStringLiteral(",\n") + pad2;
            }
            else {
                firstRow = false;
            }
            yaml += QStringLiteral("[");

            bool first = true;
            for (auto& column : *row) {
                if (!first) {
                    // TODO: Fix for arrays with too many columns to fit on a single line
                    yaml += QStringLiteral(", ");
                }
                else {
                    first = false;
                }
                yaml += QStringLiteral("\"");
                // Base::Quantity quantity = column.value<Base::Quantity>();
                yaml += QString::fromStdString(column.getUserString());
                yaml += QStringLiteral("\"");
            }

            yaml += QStringLiteral("]");
        }
        yaml += QStringLiteral("]");
    }
    yaml += QStringLiteral("]");
    return yaml;
}

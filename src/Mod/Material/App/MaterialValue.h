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

#include <memory>

#include <QList>
#include <QMetaType>
#include <QVariant>

#include <Gui/MetaTypes.h>

#include <Mod/Material/MaterialGlobal.h>

namespace Materials
{

class MaterialsExport MaterialValue: public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    enum ValueType
    {
        None = 0,
        String = 1,
        Boolean = 2,
        Integer = 3,
        Float = 4,
        Quantity = 5,
        Distribution = 6,
        List = 7,
        Array2D = 8,
        Array3D = 9,
        Color = 10,
        Image = 11,
        File = 12,
        URL = 13,
        MultiLineString = 14,
        FileList = 15,
        ImageList = 16,
        SVG
    };
    MaterialValue();
    explicit MaterialValue(const MaterialValue& other);
    explicit MaterialValue(ValueType type);
    virtual ~MaterialValue() = default;

    MaterialValue& operator=(const MaterialValue& other);
    virtual bool operator==(const MaterialValue& other) const;
    bool operator!=(const MaterialValue& other) const
    {
        return !operator==(other);
    }

    ValueType getType() const
    {
        return _valueType;
    }

    QVariant getValue() const
    {
        return _value;
    }
    QList<QVariant> getList()
    {
        return _value.value<QList<QVariant>>();
    }
    const QList<QVariant> getList() const
    {
        return _value.value<QList<QVariant>>();
    }
    virtual bool isNull() const;
    virtual bool isEmpty() const;

    virtual const QVariant getValueAt(const QVariant& value) const
    {
        Q_UNUSED(value);
        return _value;
    }
    void setValue(const QVariant& value)
    {
        _value = value;
    }
    void setList(const QList<QVariant>& value);

    virtual QString getYAMLString() const;
    static QString escapeString(const QString& source);
    static ValueType mapType(const QString& stringType);

    static const Base::QuantityFormat getQuantityFormat();

    // The precision is based on the value from the original materials editor
    static const int PRECISION = 6;

    void validate(const MaterialValue& other) const;

protected:
    MaterialValue(ValueType type, ValueType inherited);

    void setType(ValueType type)
    {
        _valueType = type;
    }
    void setInitialValue(ValueType inherited);

    QString getYAMLStringImage() const;
    QString getYAMLStringList() const;
    QString getYAMLStringImageList() const;
    QString getYAMLStringMultiLine() const;

    ValueType _valueType;
    QVariant _value;

private:
    static QMap<QString, ValueType> _typeMap;
};

class MaterialsExport Array2D: public MaterialValue
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Array2D();
    Array2D(const Array2D& other);
    ~Array2D() override = default;

    Array2D& operator=(const Array2D& other);

    bool isNull() const override;
    bool isEmpty() const override;

    const QList<std::shared_ptr<QList<QVariant>>>& getArray() const
    {
        return _rows;
    }

    void validateRow(int row) const;
    void validateColumn(int column) const;
    void validate(const Array2D& other) const;

    std::shared_ptr<QList<QVariant>> getRow(int row) const;
    std::shared_ptr<QList<QVariant>> getRow(int row);
    int rows() const
    {
        return _rows.size();
    }
    int columns() const
    {
        return _columns;
    }
    void setColumns(int size)
    {
        _columns = size;
    }
    void addRow(const std::shared_ptr<QList<QVariant>>& row);
    void insertRow(int index, const std::shared_ptr<QList<QVariant>>& row);
    void deleteRow(int row);
    void setRows(int rowCount);

    void setValue(int row, int column, const QVariant& value);
    QVariant getValue(int row, int column) const;

    QString getYAMLString() const override;

protected:
    void deepCopy(const Array2D& other);

    QList<std::shared_ptr<QList<QVariant>>> _rows;
    int _columns;

private:
    static void dumpRow(const std::shared_ptr<QList<QVariant>>& row);
    void dump() const;
};

class MaterialsExport Array3D: public MaterialValue
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Array3D();
    Array3D(const Array3D& other);
    ~Array3D() override = default;

    Array3D& operator=(const Array3D& other);

    bool isNull() const override;
    bool isEmpty() const override;

    const QList<
        std::pair<Base::Quantity, std::shared_ptr<QList<std::shared_ptr<QList<Base::Quantity>>>>>>&
    getArray() const
    {
        return _rowMap;
    }

    void validateDepth(int level) const;
    void validateColumn(int column) const;
    void validateRow(int level, int row) const;
    void validate(const Array3D& other) const;

    const std::shared_ptr<QList<std::shared_ptr<QList<Base::Quantity>>>>&
    getTable(const Base::Quantity& depth) const;
    const std::shared_ptr<QList<std::shared_ptr<QList<Base::Quantity>>>>&
    getTable(int depthIndex) const;
    std::shared_ptr<QList<Base::Quantity>> getRow(int depth, int row) const;
    std::shared_ptr<QList<Base::Quantity>> getRow(int row) const;
    std::shared_ptr<QList<Base::Quantity>> getRow(int depth, int row);
    std::shared_ptr<QList<Base::Quantity>> getRow(int row);
    void addRow(int depth, const std::shared_ptr<QList<Base::Quantity>>& row);
    void addRow(const std::shared_ptr<QList<Base::Quantity>>& row);
    int addDepth(int depth, const Base::Quantity& value);
    int addDepth(const Base::Quantity& value);
    void deleteDepth(int depth);
    void insertRow(int depth, int row, const std::shared_ptr<QList<Base::Quantity>>& rowData);
    void insertRow(int row, const std::shared_ptr<QList<Base::Quantity>>& rowData);
    void deleteRow(int depth, int row);
    void deleteRow(int row);
    void deleteRows(int depth);
    void deleteRows();
    int depth() const
    {
        return _rowMap.size();
    }
    int rows(int depth) const;
    int rows() const
    {
        return rows(_currentDepth);
    }
    int columns() const
    {
        return _columns;
    }
    void setColumns(int size)
    {
        _columns = size;
    }
    void setDepth(int depthCount);
    void setRows(int depth, int rowCount);

    void setValue(int depth, int row, int column, const Base::Quantity& value);
    void setValue(int row, int column, const Base::Quantity& value);
    void setDepthValue(int depth, const Base::Quantity& value);
    void setDepthValue(const Base::Quantity& value);
    Base::Quantity getValue(int depth, int row, int column) const;
    Base::Quantity getValue(int row, int column) const;
    Base::Quantity getDepthValue(int depth) const;

    int currentDepth() const;
    void setCurrentDepth(int depth);

    QString getYAMLString() const override;

protected:
    void deepCopy(const Array3D& other);

    QList<std::pair<Base::Quantity, std::shared_ptr<QList<std::shared_ptr<QList<Base::Quantity>>>>>>
        _rowMap;
    int _currentDepth;
    int _columns;
};

}  // namespace Materials

Q_DECLARE_METATYPE(Materials::MaterialValue)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::Array2D>)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::Array3D>)
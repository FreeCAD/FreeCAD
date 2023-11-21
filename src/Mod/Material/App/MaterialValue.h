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

#ifndef MATERIAL_MATERIALVALUE_H
#define MATERIAL_MATERIALVALUE_H

#include <memory>

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
        MultiLineString = 14
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

    virtual const QString getYAMLString() const;
    static QString escapeString(const QString& source);

protected:
    MaterialValue(ValueType type, ValueType inherited);

    void setType(ValueType type)
    {
        _valueType = type;
    }
    void setInitialValue(ValueType inherited);

    ValueType _valueType;
    QVariant _value;
};

class MaterialsExport Material2DArray: public MaterialValue
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Material2DArray();
    Material2DArray(const Material2DArray& other);
    ~Material2DArray() override = default;

    Material2DArray& operator=(const Material2DArray& other);

    bool isNull() const override;

    void setDefault(MaterialValue value, bool markSet = true)
    {
        _value = value.getValue();
        _defaultSet = markSet;
    }
    void setDefault(const QVariant& value, bool markSet = true)
    {
        _value = value;
        _defaultSet = markSet;
    }
    void setDefault(const Base::Quantity& value, bool markSet = true)
    {
        _value = QVariant::fromValue(value);
        _defaultSet = markSet;
    }
    const QVariant getDefault() const;
    bool defaultSet() const
    {
        return _defaultSet;
    }

    std::shared_ptr<std::vector<QVariant>> getRow(int row) const;
    std::shared_ptr<std::vector<QVariant>> getRow(int row);
    int rows() const
    {
        return _rows.size();
    }
    int columns() const
    {
        if (rows() == 0) {
            return 0;
        }

        return _rows.at(0)->size();
    }
    void addRow(std::shared_ptr<std::vector<QVariant>> row);
    void insertRow(int index, std::shared_ptr<std::vector<QVariant>> row);
    void deleteRow(int row);

    void setValue(int row, int column, const QVariant& value);
    const QVariant getValue(int row, int column) const;

    const QString getYAMLString() const override;

protected:
    void deepCopy(const Material2DArray& other);

    std::vector<std::shared_ptr<std::vector<QVariant>>> _rows;
    bool _defaultSet;

private:
    void dumpRow(std::shared_ptr<std::vector<QVariant>> row) const;
    void dump() const;
};

class MaterialsExport Material3DArray: public MaterialValue
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Material3DArray();
    ~Material3DArray() override = default;

    bool isNull() const override;

    void setDefault(MaterialValue value, bool markSet = true)
    {
        _value = value.getValue();
        _defaultSet = markSet;
    }
    void setDefault(const QVariant& value, bool markSet = true)
    {
        _value = value;
        _defaultSet = markSet;
    }
    void setDefault(const Base::Quantity& value, bool markSet = true)
    {
        _value = QVariant::fromValue(value);
        _defaultSet = markSet;
    }
    const QVariant getDefault() const;
    bool defaultSet() const
    {
        return _defaultSet;
    }

    const std::shared_ptr<std::vector<std::shared_ptr<std::vector<Base::Quantity>>>>&
    getTable(const Base::Quantity& depth) const;
    const std::shared_ptr<std::vector<std::shared_ptr<std::vector<Base::Quantity>>>>&
    getTable(int depthIndex) const;
    const std::shared_ptr<std::vector<Base::Quantity>> getRow(int depth, int row) const;
    std::shared_ptr<std::vector<Base::Quantity>> getRow(int row) const;
    std::shared_ptr<std::vector<Base::Quantity>> getRow(int depth, int row);
    std::shared_ptr<std::vector<Base::Quantity>> getRow(int row);
    void addRow(int depth, std::shared_ptr<std::vector<Base::Quantity>> row);
    void addRow(std::shared_ptr<std::vector<Base::Quantity>> row);
    int addDepth(int depth, Base::Quantity value);
    int addDepth(Base::Quantity value);
    void deleteDepth(int depth);
    void insertRow(int depth, int row, std::shared_ptr<std::vector<Base::Quantity>> rowData);
    void insertRow(int row, std::shared_ptr<std::vector<Base::Quantity>> rowData);
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
    int columns(int depth) const;
    int columns() const
    {
        return columns(_currentDepth);
    }

    void setValue(int depth, int row, int column, const Base::Quantity& value);
    void setValue(int row, int column, const Base::Quantity& value);
    void setDepthValue(int depth, const Base::Quantity& value);
    void setDepthValue(const Base::Quantity& value);
    const Base::Quantity getValue(int depth, int row, int column) const;
    const Base::Quantity getValue(int row, int column) const;
    const Base::Quantity getDepthValue(int depth) const;

    int currentDepth() const;
    void setCurrentDepth(int depth);

    const QString getYAMLString() const override;

protected:
    std::vector<
        std::pair<Base::Quantity,
                  std::shared_ptr<std::vector<std::shared_ptr<std::vector<Base::Quantity>>>>>>
        _rowMap;
    bool _defaultSet;
    int _currentDepth;
};

}  // namespace Materials

Q_DECLARE_METATYPE(Materials::MaterialValue)
// Q_DECLARE_METATYPE(Materials::Material2DArray)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::Material2DArray>)
// Q_DECLARE_METATYPE(Materials::Material3DArray)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::Material3DArray>)

#endif  // MATERIAL_MATERIALVALUE_H

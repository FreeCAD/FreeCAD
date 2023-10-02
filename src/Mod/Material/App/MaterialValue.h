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

#include <Mod/Material/MaterialGlobal.h>

#include <QVariant>

namespace Materials
{

class MaterialsExport MaterialValue
{
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
        URL = 13
    };
    MaterialValue();
    explicit MaterialValue(ValueType type);
    virtual ~MaterialValue() = default;

    ValueType getType() const
    {
        return _valueType;
    }

    const QVariant getValue() const
    {
        return _value;
    }
    bool isNull() const
    {
        return _value.isNull();
    }
    virtual const QVariant getValueAt(const QVariant& value) const
    {
        Q_UNUSED(value);
        return _value;
    }
    void setValue(const QVariant& value)
    {
        _value = value;
    }

protected:
    ValueType _valueType;
    QVariant _value;

    void setType(ValueType type)
    {
        _valueType = type;
    }
};

class MaterialsExport Material2DArray: public MaterialValue
{
public:
    Material2DArray();
    ~Material2DArray() override = default;

    void setDefault(MaterialValue value)
    {
        _value = value.getValue();
        _defaultSet = true;
    }
    void setDefault(const QVariant& value)
    {
        _value = value;
        _defaultSet = true;
    }
    MaterialValue getDefault() const;
    bool defaultSet() const
    {
        return _defaultSet;
    }

    const std::vector<QVariant>* getRow(int row) const;
    std::vector<QVariant>* getRow(int row);
    int rows() const
    {
        return _rows.size();
    }
    void addRow(std::vector<QVariant>* row);
    void insertRow(int index, std::vector<QVariant>* row);
    void deleteRow(int row);

    void setValue(int row, int column, const QVariant& value);
    const QVariant getValue(int row, int column) const;

protected:
    std::vector<std::vector<QVariant>*> _rows;
    bool _defaultSet;

private:
    void dumpRow(const std::vector<QVariant>& row) const;
    void dump() const;
};

class MaterialsExport Material3DArray: public MaterialValue
{
public:
    Material3DArray();
    ~Material3DArray() override = default;

    void setDefault(MaterialValue value)
    {
        _value = value.getValue();
        _defaultSet = true;
    }
    void setDefault(const QVariant& value)
    {
        _value = value;
        _defaultSet = true;
    }
    MaterialValue getDefault() const;
    bool defaultSet() const
    {
        return _defaultSet;
    }

    const std::vector<std::vector<QVariant>*>& getTable(const QVariant& depth) const;
    const std::vector<QVariant>& getRow(const QVariant& depth, int row) const;
    const std::vector<QVariant>& getRow(int row) const;
    std::vector<QVariant>& getRow(const QVariant& depth, int row);
    std::vector<QVariant>& getRow(int row);
    void addRow(const QVariant& depth, std::vector<QVariant>* row);
    void deleteRow(const QVariant& depth, int row);
    void deleteRows(int depth);
    int depth() const
    {
        return _rowMap.size();
    }
    int rows(const QVariant& depth) const
    {
        return getTable(depth).size();
    }

    void setValue(const QVariant& depth, int row, int column, const QVariant& value);
    void setValue(int row, int column, const QVariant& value);
    const QVariant getValue(const QVariant& depth, int row, int column);
    const QVariant getValue(int row, int column);

protected:
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    std::map<QVariant, std::vector<std::vector<QVariant>*>> _rowMap;
#else
    struct variant_comp
    {
        bool operator()(const QVariant& var1,
                        const QVariant& var2) const
        {
            return QVariant::compare(var1, var2) == QPartialOrdering::Less;
        }
    };
    std::map<QVariant, std::vector<std::vector<QVariant>*>, variant_comp> _rowMap;
#endif

    bool _defaultSet;
};

}  // namespace Materials

Q_DECLARE_METATYPE(Materials::MaterialValue)
Q_DECLARE_METATYPE(Materials::Material2DArray)
Q_DECLARE_METATYPE(Materials::Material3DArray)

#endif  // MATERIAL_MATERIALVALUE_H

/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <App/Application.h>

#include "MaterialValue.h"
#include "Exceptions.h"


using namespace Materials;

/* TRANSLATOR Material::MaterialValue */

MaterialValue::MaterialValue() :
    _valueType(None)
{}

MaterialValue::MaterialValue(ValueType type) :
    _valueType(type)
{}

MaterialValue::~MaterialValue()
{}

//===

Material2DArray::Material2DArray() :
    MaterialValue(Array2D)
{}

Material2DArray::~Material2DArray()
{}

MaterialValue Material2DArray::getDefault() const
{
    MaterialValue ret(_valueType);
    ret.setValue(_value);
    return ret;
}

const std::vector<QVariant> *Material2DArray::getRow(int row) const
{
    try {
        return _rows.at(row);
    } catch (std::out_of_range const &) {
        throw InvalidRow();
    }
}

std::vector<QVariant> *Material2DArray::getRow(int row)
{
    try {
        return _rows.at(row);
    } catch (std::out_of_range const &) {
        throw InvalidRow();
    }

}

void Material2DArray::addRow(std::vector<QVariant> *row)
{
    _rows.push_back(row);
}

void Material2DArray::insertRow(int index, std::vector<QVariant> *row)
{
    _rows.insert(_rows.begin() + index, row);
}

void Material2DArray::deleteRow(int row)
{
    if (row >= _rows.size() || row < 0)
        throw InvalidRow();
    _rows.erase(_rows.begin() + row);
}

void Material2DArray::setValue(int row, int column,  const QVariant &value)
{
    if (row >= rows())
        throw InvalidIndex();

    std::vector<QVariant> *val = getRow(row);
    try
    {
        val->at(column) = value;
    }
    catch(const std::out_of_range&)
    {
        throw InvalidIndex();
    }
}

const QVariant &Material2DArray::getValue(int row, int column)
{
    try
    {
        auto val = getRow(row);
        try {
            return val->at(column);
        } catch (std::out_of_range const &) {
            throw InvalidIndex();
        }
    }
    catch(const InvalidRow &)
    {
        throw InvalidIndex();
    }
}

void Material2DArray::dumpRow(const std::vector<QVariant> &row) const
{
    Base::Console().Log("row: ");
    for (auto column: row)
    {
        Base::Console().Log("'%s' ", 
            column.toString().toStdString().c_str());
    }
    Base::Console().Log("\n");
}

void Material2DArray::dump() const
{
    for (auto row: _rows)
    {
        dumpRow(*row);
    }
}

//===

Material3DArray::Material3DArray() :
    MaterialValue(Array3D)
{}

Material3DArray::~Material3DArray()
{}

MaterialValue Material3DArray::getDefault() const
{
    MaterialValue ret(_valueType);
    ret.setValue(_value);
    return ret;
}

const std::vector<QVariant> &Material3DArray::getRow(const QString &depth, int row) const
{
    try {
        return *(_rowMap.at(depth).at(row));
    } catch (std::out_of_range const &) {
        throw InvalidRow();
    }
}

const std::vector<QVariant> &Material3DArray::getRow(int row) const
{
    return getRow(getDefault().getValue().toString(), row);
}

std::vector<QVariant> &Material3DArray::getRow(const QString & depth, int row)
{
    try {
        return *(_rowMap.at(depth).at(row));
    } catch (std::out_of_range const &) {
        throw InvalidRow();
    }
}

std::vector<QVariant> &Material3DArray::getRow(int row)
{
    return getRow(getDefault().getValue().toString(), row);
}

void Material3DArray::addRow(const QString & depth, std::vector<QVariant> *row)
{

}

void Material3DArray::deleteRow(const QString & depth, int row)
{

}

void Material3DArray::deleteRows(int depth)
{

}

void Material3DArray::setValue(const QString & depth, int row, int column,  const QVariant &value)
{

}

void Material3DArray::setValue(int row, int column,  const QVariant &value)
{

}

const QVariant &Material3DArray::getValue(const QString & depth, int row, int column)
{
    auto val = getRow(depth, row);
    try {
        return val.at(column);
    } catch (std::out_of_range const &) {
        throw InvalidColumn();
    }
}

const QVariant &Material3DArray::getValue(int row, int column)
{
    return getValue(getDefault().getValue().toString(), row, column);

}


#include "moc_MaterialValue.cpp"

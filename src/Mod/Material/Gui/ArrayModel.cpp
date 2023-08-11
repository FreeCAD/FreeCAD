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
# include <QMessageBox>
#endif

#include <QMetaType>
#include <Gui/MetaTypes.h>
#include <Gui/MainWindow.h>

#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/Exceptions.h>
#include "ArrayModel.h"


using namespace MatGui;

/* TRANSLATOR MatGui::ArrayModel */

AbstractArrayModel::AbstractArrayModel(QObject *parent) :
    QAbstractTableModel(parent)
{}

AbstractArrayModel::~AbstractArrayModel()
{}

//===


Array2DModel::Array2DModel(Materials::MaterialProperty *property, Materials::Material2DArray *value, QObject *parent) :
    AbstractArrayModel(parent),
    _property(property),
    _value(value)
{}

Array2DModel::~Array2DModel()
{}

int Array2DModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0; // No children

    return _value->rows() + 1; // Will always have 1 empty row
}

bool Array2DModel::newRow(const QModelIndex& index) const
{
    return (index.row() == _value->rows());
}

int Array2DModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return _property->columns();
}

QVariant Array2DModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        try
        {
            return _value->getValue(index.row(), index.column());
        }
        catch(const Materials::InvalidIndex &)
        {
        }

        try
        {
            auto column = _property->getColumnType(index.column());
            if (column == Materials::MaterialValue::Quantity)
            {
                Base::Quantity q = Base::Quantity(0, _property->getColumnUnits(index.column()));
                return QVariant::fromValue(q);
            }
        }
        catch(const Materials::InvalidColumn &)
        {
        }

        return QString();
    }

    return QVariant();
}

QVariant Array2DModel::headerData(int section, Qt::Orientation orientation,
                    int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal)
        {
            auto column = _property->getColumn(section);
            return QVariant(column.getName());
        } else if (orientation == Qt::Vertical) {
            // Vertical header
            if (section == (rowCount() - 1))
                return QVariant(QString::fromStdString("*"));
            return QVariant(section + 1);
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

bool Array2DModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(role);
    
    if (index.row() == _value->rows())
    {
        insertRows(index.row(), 1);
    }
    _value->setValue(index.row(), index.column(), value);

    Q_EMIT dataChanged(index, index);
    return true;
}

Qt::ItemFlags Array2DModel::flags(const QModelIndex& index) const
{
    return (QAbstractTableModel::flags(index) | Qt::ItemIsEditable);
}


// Resizing functions
bool Array2DModel::insertRows(int row, int count, const QModelIndex& parent)
{
    beginInsertRows(parent, row, row + count - 1);

    int columns = columnCount();
    for (int i = 0; i < count; i++)
    {
        std::vector<QVariant>* rowPtr = new std::vector<QVariant>();
        for (int j = 0; j < columns; j++)
        {
            rowPtr->push_back(_property->getColumnNull(j));
        }

        _value->insertRow(row, rowPtr);
    }

    endInsertRows();

    return false;
}

bool Array2DModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    endRemoveRows();

    return false;
}

bool Array2DModel::insertColumns(int column, int count, const QModelIndex& parent)
{
    Q_UNUSED(column);
    Q_UNUSED(count);
    Q_UNUSED(parent);

    return false;
}

bool Array2DModel::removeColumns(int column, int count, const QModelIndex& parent)
{
    Q_UNUSED(column);
    Q_UNUSED(count);
    Q_UNUSED(parent);

    return false;
}
//===


Array3DDepthModel::Array3DDepthModel(Materials::MaterialProperty *property, Materials::Material3DArray *value, QObject *parent) :
    AbstractArrayModel(parent),
    _property(property),
    _value(value)
{}

Array3DDepthModel::~Array3DDepthModel()
{}

int Array3DDepthModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0; // No children

    return _value->depth() + 1; // Will always have 1 empty row
}

bool Array3DDepthModel::newRow(const QModelIndex& index) const
{
    return (index.row() == _value->depth());
}

QVariant Array3DDepthModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        try
        {
            return _value->getValue(index.row(), index.column());
        }
        catch(const Materials::InvalidIndex &)
        {
        }

        try
        {
            auto column = _property->getColumnType(index.column());
            if (column == Materials::MaterialValue::Quantity)
            {
                Base::Quantity q = Base::Quantity(0, _property->getColumnUnits(index.column()));
                return QVariant::fromValue(q);
            }
        }
        catch(const Materials::InvalidColumn &)
        {
        }

        return QString();
    }

    return QVariant();
}

QVariant Array3DDepthModel::headerData(int section, Qt::Orientation orientation,
                    int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal)
        {
            auto column = _property->getColumn(section);
            return QVariant(column.getName());
        } else if (orientation == Qt::Vertical) {
            // Vertical header
            if (section == (rowCount() - 1))
                return QVariant(QString::fromStdString("*"));
            return QVariant(section + 1);
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

bool Array3DDepthModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(role);
    
    if (index.row() == _value->depth())
    {
        insertRows(index.row(), 1);
    }
    _value->setValue(index.row(), index.column(), value);

    Q_EMIT dataChanged(index, index);
    return true;
}

Qt::ItemFlags Array3DDepthModel::flags(const QModelIndex& index) const
{
    return (QAbstractTableModel::flags(index) | Qt::ItemIsEditable);
}


// Resizing functions
bool Array3DDepthModel::insertRows(int row, int count, const QModelIndex& parent)
{
    beginInsertRows(parent, row, row + count - 1);

    int columns = columnCount();
    for (int i = 0; i < count; i++)
    {
        std::vector<QVariant>* rowPtr = new std::vector<QVariant>();
        for (int j = 0; j < columns; j++)
        {
            rowPtr->push_back(_property->getColumnNull(j));
        }

        // _value->insertRow(row, rowPtr);
    }

    endInsertRows();

    return false;
}

bool Array3DDepthModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    endRemoveRows();

    return false;
}

bool Array3DDepthModel::insertColumns(int column, int count, const QModelIndex& parent)
{
    Q_UNUSED(column);
    Q_UNUSED(count);
    Q_UNUSED(parent);

    return false;
}

bool Array3DDepthModel::removeColumns(int column, int count, const QModelIndex& parent)
{
    Q_UNUSED(column);
    Q_UNUSED(count);
    Q_UNUSED(parent);

    return false;
}

//===

#include "moc_ArrayModel.cpp"

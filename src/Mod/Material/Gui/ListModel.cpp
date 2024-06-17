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
#endif

#include <Base/Console.h>
#include <Gui/MainWindow.h>
#include <Gui/MetaTypes.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/Materials.h>

#include "ListModel.h"


using namespace MatGui;

/* TRANSLATOR MatGui::ArrayModel */

ListModel::ListModel()
{}

ListModel::ListModel(std::shared_ptr<Materials::MaterialProperty> property,
                     QList<QVariant>& value,
                     QObject* parent)
    : QAbstractListModel(parent)
    , _property(property)
    , _valuePtr(&value)
{}

int ListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;  // No children
    }

    return _valuePtr->size() + 1;  // Will always have 1 empty row
}

bool ListModel::newRow(const QModelIndex& index) const
{
    return (index.row() == _valuePtr->size());
}

void ListModel::deleteRow(const QModelIndex& index)
{
    removeRows(index.row(), 1);
    Q_EMIT dataChanged(index, index);
}

QVariant ListModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole) {
        if (index.row() < _valuePtr->size()) {
            return _valuePtr->at(index.row());
        }
    }

    return QVariant();
}

QVariant ListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QAbstractListModel::headerData(section, orientation, role);
}

bool ListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(role);

    if (newRow(index)) {
        insertRows(index.row(), 1);
    }
    (*_valuePtr)[index.row()] = value;

    Q_EMIT dataChanged(index, index);
    return true;
}

Qt::ItemFlags ListModel::flags(const QModelIndex& index) const
{
    return (QAbstractListModel::flags(index) | Qt::ItemIsEditable);
}


// Resizing functions
bool ListModel::insertRows(int row, int count, const QModelIndex& parent)
{
    beginInsertRows(parent, row, row + count - 1);

    QVariant newRow = QString();
    while (count--) {
        _valuePtr->insert(row, newRow);
    }

    endInsertRows();

    return true;
}

bool ListModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    while (count--) {
        _valuePtr->removeAt(row);
    }

    endRemoveRows();

    return true;
}

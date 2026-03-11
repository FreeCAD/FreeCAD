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

#include <QAbstractListModel>
#include <QDialog>
#include <QList>
#include <QStandardItem>
#include <QVariant>

#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/Model.h>

namespace MatGui
{

class ListModel: public QAbstractListModel
{
public:
    ListModel();
    ListModel(std::shared_ptr<Materials::MaterialProperty> property,
              QList<QVariant>& value,
              QObject* parent = nullptr);
    ~ListModel() override = default;

    // Overridden virtual functions
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool newRow(const QModelIndex& index) const;
    void deleteRow(const QModelIndex& index);
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Resizing functions
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

private:
    std::shared_ptr<Materials::MaterialProperty> _property;
    QList<QVariant>* _valuePtr;
};

}  // namespace MatGui
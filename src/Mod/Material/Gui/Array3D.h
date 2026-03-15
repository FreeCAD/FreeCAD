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

#include <QAction>
#include <QDialog>
#include <QStandardItem>
#include <QTableView>

#include <Mod/Material/App/Materials.h>

namespace MatGui
{

class Ui_Array3D;

class Array3D: public QDialog
{
    Q_OBJECT

public:
    Array3D(const QString& propertyName,
            const std::shared_ptr<Materials::Material>& material,
            QWidget* parent = nullptr);
    ~Array3D() override = default;

    void onRowsInserted(const QModelIndex& parent, int first, int last);
    void onRowsRemoved(const QModelIndex& parent, int first, int last);
    void onDataChanged(const QModelIndex& topLeft,
                       const QModelIndex& bottomRight,
                       const QVector<int>& roles = QVector<int>());
    void onSelectDepth(const QItemSelection& selected, const QItemSelection& deselected);
    bool onSplitter(QEvent* e);
    void onDepthDelete(bool checked);
    int confirmDepthDelete();
    void deleteDepthSelected();
    void on2DDelete(bool checked);
    int confirm2dDelete();
    void delete2DSelected();
    void onDepthContextMenu(const QPoint& pos);
    void on2DContextMenu(const QPoint& pos);

    void onOk(bool checked);
    void onCancel(bool checked);

private:
    std::unique_ptr<Ui_Array3D> ui;
    std::shared_ptr<Materials::Material> _material;
    std::shared_ptr<Materials::MaterialProperty> _property;
    std::shared_ptr<Materials::Array3D> _value;

    QAction _deleteDepthAction;
    QAction _delete2DAction;

    bool newDepthRow(const QModelIndex& index);
    bool new2DRow(const QModelIndex& index);
    void setDepthColumnWidth(QTableView* table);
    void setDepthColumnDelegate(QTableView* table);
    void setupDepthArray();
    void setColumnWidths(QTableView* table);
    void setColumnDelegates(QTableView* table);
    void setupArray();
    void update2DArray();
};

}  // namespace MatGui
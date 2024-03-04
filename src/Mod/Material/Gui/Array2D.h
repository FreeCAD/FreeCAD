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

#ifndef MATGUI_ARRAY2D_H
#define MATGUI_ARRAY2D_H

#include <memory>

#include <QAbstractTableModel>
#include <QAction>
#include <QDialog>
#include <QPoint>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>

#include <Mod/Material/App/Model.h>

#include "ArrayModel.h"

namespace MatGui
{

class Ui_Array2D;

class Array2D: public QDialog
{
    Q_OBJECT

public:
    Array2D(const QString& propertyName,
            const std::shared_ptr<Materials::Material>& material,
            QWidget* parent = nullptr);
    ~Array2D() override = default;

    void onDataChanged(const QModelIndex& topLeft,
                       const QModelIndex& bottomRight,
                       const QVector<int>& roles = QVector<int>());
    void onDelete(bool checked);
    void onContextMenu(const QPoint& pos);

    void accept() override;
    void reject() override;

private:
    std::unique_ptr<Ui_Array2D> ui;
    std::shared_ptr<Materials::Material> _material;
    std::shared_ptr<Materials::MaterialProperty> _property;
    std::shared_ptr<Materials::Material2DArray> _value;

    QAction _deleteAction;

    void setColumnWidths(QTableView* table);
    void setColumnDelegates(QTableView* table);
    void setupArray();

    bool newRow(const QModelIndex& index);
    int confirmDelete();
    void deleteSelected();
};

}  // namespace MatGui

#endif  // MATGUI_ARRAY2D_H

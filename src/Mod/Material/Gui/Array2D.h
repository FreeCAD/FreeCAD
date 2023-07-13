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

#ifndef MATGUI_ARRAY2D_H
#define MATGUI_ARRAY2D_H

#include <QDialog>
#include <QStandardItem>
#include <QTableView>
#include <QAbstractTableModel>

#include <Mod/Material/App/Model.h>

namespace MatGui {

class Ui_Array2D;

class Array2DModel : public QAbstractTableModel
{
public:
    Array2DModel(Materials::MaterialProperty *property = nullptr, Materials::Material2DArray *value = nullptr, QObject *parent = nullptr);
    ~Array2DModel() override;

    // Overriden virtual functions
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Resizing functions
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;
private:
    Materials::MaterialProperty *_property;
    Materials::Material2DArray *_value;

};

class Array2D : public QDialog
{
    Q_OBJECT

public:
    explicit Array2D(const QString &propertyName, Materials::Material *material, QWidget* parent = nullptr);
    ~Array2D() override;

    void accept() override;
    void reject() override;

private:
    std::unique_ptr<Ui_Array2D> ui;
    Materials::MaterialProperty *_property;
    Materials::Material2DArray *_value;

    void setupDefault();
    void setHeaders(QStandardItemModel *model);
    void setColumnWidths(QTableView *table);
    void setColumnDelegates(QTableView *table);
    void setupArray();
};

} // namespace MatGui

#endif // MATGUI_ARRAY2D_H

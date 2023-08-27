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

#ifndef MATGUI_ARRAYMODEL_H
#define MATGUI_ARRAYMODEL_H

#include <QDialog>
#include <QStandardItem>
#include <QTableView>
#include <QAbstractTableModel>

#include <Mod/Material/App/Model.h>

namespace MatGui {

class AbstractArrayModel : public QAbstractTableModel
{
public:
    explicit AbstractArrayModel(QObject *parent = nullptr);
    ~AbstractArrayModel() override;

    virtual bool newRow(const QModelIndex& index) const = 0;
};

class Array2DModel : public AbstractArrayModel
{
public:
    explicit Array2DModel(Materials::MaterialProperty *property = nullptr, Materials::Material2DArray *value = nullptr, QObject *parent = nullptr);
    ~Array2DModel() override;

    // Overridden virtual functions
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool newRow(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
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

class Array3DDepthModel : public AbstractArrayModel
{
public:
    explicit Array3DDepthModel(Materials::MaterialProperty *property = nullptr, Materials::Material3DArray *value = nullptr, QObject *parent = nullptr);
    ~Array3DDepthModel() override;

    // Overridden virtual functions
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool newRow(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override { Q_UNUSED(parent) return 1; }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
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
    Materials::Material3DArray *_value;

};

class Array3DModel : public AbstractArrayModel
{
public:
    explicit Array3DModel(Materials::MaterialProperty *property = nullptr, Materials::Material3DArray *value = nullptr, QObject *parent = nullptr);
    ~Array3DModel() override;

    // Overridden virtual functions
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool newRow(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
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
    Materials::Material3DArray *_value;

};

} // namespace MatGui

#endif // MATGUI_ARRAYMODEL_H

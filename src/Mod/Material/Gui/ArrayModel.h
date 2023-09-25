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

#ifndef MATGUI_ARRAYMODEL_H
#define MATGUI_ARRAYMODEL_H

#include <QAbstractTableModel>
#include <QDialog>
#include <QStandardItem>
#include <QTableView>

#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/Model.h>

namespace MatGui
{

class AbstractArrayModel: public QAbstractTableModel
{
public:
    explicit AbstractArrayModel(QObject* parent = nullptr);
    ~AbstractArrayModel() override = default;

    virtual bool newRow(const QModelIndex& index) const = 0;
};

class Array2DModel: public AbstractArrayModel
{
public:
    explicit Array2DModel(const Materials::MaterialProperty* property = nullptr,
                          std::shared_ptr<Materials::Material2DArray> value = nullptr,
                          QObject* parent = nullptr);
    ~Array2DModel() override = default;

    // Overridden virtual functions
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool newRow(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Resizing functions
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;

private:
    const Materials::MaterialProperty* _property;
    std::shared_ptr<Materials::Material2DArray> _value;
};

class Array3DDepthModel: public AbstractArrayModel
{
public:
    explicit Array3DDepthModel(const Materials::MaterialProperty* property = nullptr,
                               std::shared_ptr<Materials::Material3DArray> value = nullptr,
                               QObject* parent = nullptr);
    ~Array3DDepthModel() override = default;

    // Overridden virtual functions
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool newRow(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return 1;
    }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Resizing functions
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;

private:
    const Materials::MaterialProperty* _property;
    std::shared_ptr<Materials::Material3DArray> _value;
};

class Array3DModel: public AbstractArrayModel
{
public:
    explicit Array3DModel(const Materials::MaterialProperty* property = nullptr,
                          std::shared_ptr<Materials::Material3DArray> value = nullptr,
                          QObject* parent = nullptr);
    ~Array3DModel() override = default;

    // Overridden virtual functions
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool newRow(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Resizing functions
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;

private:
    const Materials::MaterialProperty* _property;
    std::shared_ptr<Materials::Material3DArray> _value;
};

}  // namespace MatGui

#endif  // MATGUI_ARRAYMODEL_H

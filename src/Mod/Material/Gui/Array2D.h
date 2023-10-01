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

#include <QAbstractTableModel>
#include <QDialog>
#include <QStandardItem>
#include <QTableView>

#include "ArrayModel.h"
#include <Mod/Material/App/Model.h>

namespace MatGui
{

class Ui_Array2D;

class Array2D: public QDialog
{
    Q_OBJECT

public:
    explicit Array2D(const QString& propertyName,
                     Materials::Material* material,
                     QWidget* parent = nullptr);
    ~Array2D() override = default;

    void defaultValueChanged(const Base::Quantity& value);

    void accept() override;
    void reject() override;

private:
    std::unique_ptr<Ui_Array2D> ui;
    const Materials::MaterialProperty* _property;
    std::shared_ptr<Materials::Material2DArray> _value;

    void setupDefault();
    void setHeaders(QStandardItemModel* model);
    void setColumnWidths(QTableView* table);
    void setColumnDelegates(QTableView* table);
    void setupArray();
};

}  // namespace MatGui

#endif  // MATGUI_ARRAY2D_H

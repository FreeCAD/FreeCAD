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
    Materials::MaterialProperty* _property;
    Materials::Material2DArray* _value;

    void setupDefault();
    void setHeaders(QStandardItemModel* model);
    void setColumnWidths(QTableView* table);
    void setColumnDelegates(QTableView* table);
    void setupArray();
};

}// namespace MatGui

#endif// MATGUI_ARRAY2D_H

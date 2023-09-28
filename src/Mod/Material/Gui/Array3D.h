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

#ifndef MATGUI_ARRAY3D_H
#define MATGUI_ARRAY3D_H

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
    explicit Array3D(const QString& propertyName,
                     Materials::Material* material,
                     QWidget* parent = nullptr);
    ~Array3D() override = default;

    void defaultValueChanged(const Base::Quantity& value);
    bool onSplitter(QEvent* e);

    void onOk(bool checked);
    void onCancel(bool checked);

private:
    std::unique_ptr<Ui_Array3D> ui;
    const Materials::MaterialProperty* _property;
    std::shared_ptr<Materials::Material3DArray> _value;

    void setupDefault();
    void setDepthColumnWidth(QTableView* table);
    void setDepthColumnDelegate(QTableView* table);
    void setupDepthArray();
    void setColumnWidths(QTableView* table);
    void setColumnDelegates(QTableView* table);
    void setupArray();
};

}  // namespace MatGui

#endif  // MATGUI_ARRAY3D_H

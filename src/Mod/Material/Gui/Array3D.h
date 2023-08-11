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

#ifndef MATGUI_ARRAY3D_H
#define MATGUI_ARRAY3D_H

#include <QDialog>
#include <QStandardItem>

namespace MatGui {

class Ui_Array3D;

class Array3D : public QDialog
{
    Q_OBJECT

public:
    explicit Array3D(const QString &propertyName, Materials::Material *material, QWidget* parent = nullptr);
    ~Array3D() override;

    void defaultValueChanged(const Base::Quantity &value);

    void onOk(bool checked);
    void onCancel(bool checked);

private:
    std::unique_ptr<Ui_Array3D> ui;
    Materials::MaterialProperty *_property;
    Materials::Material3DArray *_value;

    void setupDefault();
    void setDepthColumnWidth(QTableView *table);
    void setDepthColumnDelegate(QTableView *table);
    void setupDepthArray();
};

} // namespace MatGui

#endif // MATGUI_ARRAY3D_H

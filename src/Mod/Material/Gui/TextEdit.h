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

class Ui_TextEdit;

class TextEdit: public QDialog
{
    Q_OBJECT

public:
    TextEdit(const QString& propertyName,
             const std::shared_ptr<Materials::Material>& material,
             QWidget* parent = nullptr);
    ~TextEdit() override = default;

    void accept() override;
    void reject() override;

private:
    std::unique_ptr<Ui_TextEdit> ui;
    std::shared_ptr<Materials::Material> _material;
    std::shared_ptr<Materials::MaterialProperty> _property;
    QString _value;
};

}  // namespace MatGui
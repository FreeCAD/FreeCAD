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

#include <QDialog>
#include <QDir>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <QSvgWidget>
#include <QTreeView>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/ModelManager.h>

namespace MatGui
{

class ArrayDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    ArrayDelegate(Materials::MaterialValue::ValueType type = Materials::MaterialValue::None,
                  const QString& units = QString(),
                  QObject* parent = nullptr);
    virtual ~ArrayDelegate() = default;

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& styleOption,
                          const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;

private:
    Materials::MaterialValue::ValueType _type;
    QString _units;

    QWidget* createWidget(QWidget* parent, const QVariant& item) const;
};

}  // namespace MatGui
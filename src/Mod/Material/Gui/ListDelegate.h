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

#include "BaseDelegate.h"

namespace MatGui
{

class ListDelegate: public BaseDelegate
{
    Q_OBJECT
public:
    ListDelegate(Materials::MaterialValue::ValueType type = Materials::MaterialValue::None,
                 const QString& units = QString(),
                 QObject* parent = nullptr);
    virtual ~ListDelegate() = default;

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

protected:
    Materials::MaterialValue::ValueType getType(const QModelIndex& index) const override
    {
        Q_UNUSED(index)
        return _type;
    }
    QString getUnits(const QModelIndex& index) const override
    {
        Q_UNUSED(index)
        return _units;
    }
    QVariant getValue(const QModelIndex& index) const override;
    void setValue(QAbstractItemModel* model,
                  const QModelIndex& index,
                  const QVariant& value) const override;
    void notifyChanged(const QAbstractItemModel* model, const QModelIndex& index) const override;

private:
    Materials::MaterialValue::ValueType _type;
    QString _units;
};

}  // namespace MatGui
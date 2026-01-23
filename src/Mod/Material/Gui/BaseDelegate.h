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

#include <Base/Color.h>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/ModelManager.h>

namespace MatGui
{

using Base::Color;

class BaseDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    BaseDelegate(QObject* parent = nullptr);
    virtual ~BaseDelegate() = default;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& styleOption,
                          const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor,
                      QAbstractItemModel* model,
                      const QModelIndex& index) const override;

    // Q_SIGNALS:
    /** Emits this signal when a property has changed */
    // void propertyChange(const QModelIndex& index, const QString value);

protected:
    virtual Materials::MaterialValue::ValueType getType(const QModelIndex& index) const = 0;
    virtual QString getUnits(const QModelIndex& index) const = 0;
    virtual QVariant getValue(const QModelIndex& index) const = 0;
    virtual void
    setValue(QAbstractItemModel* model, const QModelIndex& index, const QVariant& value) const = 0;
    virtual void notifyChanged(const QAbstractItemModel* model, const QModelIndex& index) const = 0;

    QString getStringValue(const QModelIndex& index) const;
    Color parseColor(const QString& color) const;

    void paintQuantity(QPainter* painter,
                       const QStyleOptionViewItem& option,
                       const QModelIndex& index) const;
    void paintImage(QPainter* painter,
                    const QStyleOptionViewItem& option,
                    const QModelIndex& index) const;
    void
    paintSVG(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void paintColor(QPainter* painter,
                    const QStyleOptionViewItem& option,
                    const QModelIndex& index) const;
    void paintList(QPainter* painter,
                   const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;
    void paintMultiLineString(QPainter* painter,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const;
    void paintArray(QPainter* painter,
                    const QStyleOptionViewItem& option,
                    const QModelIndex& index) const;

    virtual bool newRow(const QAbstractItemModel* model, const QModelIndex& index) const;
    QWidget* createWidget(QWidget* parent, const QVariant& item, const QModelIndex& index) const;
};

}  // namespace MatGui
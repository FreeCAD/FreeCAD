/****************************************************************************
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QApplication>
# include <QModelIndex>
# include <QPainter>
#endif

#include <Base/Tools.h>

#include "PropertyManagerItemDelegate.h"
#include "PropertyItemDelegate2.h"
#include "PropertyManagerItem.h"
#include "propertyeditor/PropertyItem.h"


FC_LOG_LEVEL_INIT("PropertyView", true, true)

using namespace Gui::PropertyEditor;


PropertyManagerItemDelegate::PropertyManagerItemDelegate(QObject* parent)
    : PropertyItemDelegate2(parent)
{
}

void PropertyManagerItemDelegate::paintPropertyItem(QPainter *painter, QStyleOptionViewItem &option,
                                                    const QStyleOptionViewItem &opt,
                                                    const QModelIndex &index) const {
    auto property = static_cast<PropertyItem*>(index.internalPointer());
    
    if (property && property->isSeparator()) {
        QColor color = option.palette.color(QPalette::BrightText);
        QObject* par = parent();
        if (par) {
            QVariant value = par->property("groupTextColor");
            if (value.canConvert<QColor>()) {
                color = value.value<QColor>();
            }
        }
        option.palette.setColor(QPalette::Text, color);
        option.font.setBold(true);

        // Since the group item now parents all the property items and can be
        // collapsed, it makes sense to have some selection visual clue for it.
        //
        // option.state &= ~QStyle::State_Selected;
    }
    else if (index.column() == 1) {
        option.state &= ~QStyle::State_Selected;
    }

    option.state &= ~QStyle::State_HasFocus;

    if (property && property->isSeparator()) {
        QBrush brush = option.palette.dark();
        QObject* par = parent();
        if (par) {
            QVariant value = par->property("groupBackground");
            if (value.canConvert<QBrush>())
                brush = value.value<QBrush>();
        }
        painter->fillRect(option.rect, brush);
    }

    QPen savedPen = painter->pen();

    QItemDelegate::paint(painter, option, index);

    QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &opt, qobject_cast<QWidget*>(parent())));
    painter->setPen(QPen(color));
    if (index.column() == 1 || !(property && property->isSeparator())) {
        int right = (option.direction == Qt::LeftToRight) ? option.rect.right() : option.rect.left();
        painter->drawLine(right, option.rect.y(), right, option.rect.bottom());
    }
    painter->drawLine(option.rect.x(), option.rect.bottom(),
            option.rect.right(), option.rect.bottom());
    painter->setPen(savedPen);
}

void PropertyManagerItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    QStyleOptionViewItem option = opt;

    auto propertyItem = static_cast<PropertyItem*>(index.internalPointer());
    if (propertyItem) {
        auto documentItem = dynamic_cast<DocumentItem*>(propertyItem);
        auto objectItem = dynamic_cast<DocumentObjectItem*>(propertyItem);

        if (documentItem || objectItem) {
            option.font.setBold(true);
            option.state &= ~QStyle::State_HasFocus;
            QPen savedPen = painter->pen();

            QItemDelegate::paint(painter, option, index);
        }
        else {
            paintPropertyItem(painter, option, opt, index);
        }
    }
    else {
        paintPropertyItem(painter, option, opt, index);
    }
}

#include "moc_PropertyManagerItemDelegate.cpp"

/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen (eivind@kvedalen.name)             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This file is based on the Qt spreadsheet example code.                *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QLineEdit>
#include <QPainter>
#endif

#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Mod/Spreadsheet/App/Sheet.h>

#include "LineEdit.h"
#include "SpreadsheetDelegate.h"


FC_LOG_LEVEL_INIT("Spreadsheet", true, true)

using namespace Spreadsheet;
using namespace SpreadsheetGui;

SpreadsheetDelegate::SpreadsheetDelegate(Spreadsheet::Sheet* _sheet, QWidget* parent)
    : QStyledItemDelegate(parent)
    , sheet(_sheet)
{}

QWidget* SpreadsheetDelegate::createEditor(QWidget* parent,
                                           const QStyleOptionViewItem&,
                                           const QModelIndex& index) const
{
    App::CellAddress addr(index.row(), index.column());
    App::Range range(addr, addr);
    if (sheet && sheet->getCellBinding(range)) {
        FC_ERR("Bound cell " << addr.toString() << " cannot be edited");
        return nullptr;
    }

    SpreadsheetGui::LineEdit* editor = new SpreadsheetGui::LineEdit(parent);
    editor->setDocumentObject(sheet);
    connect(editor,
            &SpreadsheetGui::LineEdit::finishedWithKey,
            this,
            &SpreadsheetDelegate::onEditorFinishedWithKey);
    return editor;
}

void SpreadsheetDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
    if (edit) {
        edit->setText(index.model()->data(index, Qt::EditRole).toString());
        return;
    }
}

void SpreadsheetDelegate::setModelData(QWidget* editor,
                                       QAbstractItemModel* model,
                                       const QModelIndex& index) const
{
    QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
    if (edit) {
        model->setData(index, edit->text());
        return;
    }
}

void SpreadsheetDelegate::onEditorFinishedWithKey(int key, Qt::KeyboardModifiers modifiers)
{
    Q_EMIT finishedWithKey(key, modifiers);
}

QSize SpreadsheetDelegate::sizeHint(const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return {};
}

static inline void drawBorder(QPainter* painter,
                              const QStyleOptionViewItem& option,
                              unsigned flags,
                              QColor color,
                              Qt::PenStyle style)
{
    if (!flags) {
        return;
    }
    QPen pen(color);
    pen.setWidth(2);
    pen.setStyle(style);
    painter->setPen(pen);

    QRect rect = option.rect.adjusted(1, 1, 0, 0);
    if (flags == Sheet::BorderAll) {
        painter->drawRect(rect.adjusted(0, 0, -1, -1));
        return;
    }
    if (flags & Sheet::BorderLeft) {
        painter->drawLine(rect.topLeft(), rect.bottomLeft());
    }
    if (flags & Sheet::BorderTop) {
        painter->drawLine(rect.topLeft(), rect.topRight());
    }
    if (flags & Sheet::BorderRight) {
        painter->drawLine(rect.topRight(), rect.bottomRight());
    }
    if (flags & Sheet::BorderBottom) {
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    }
}

void SpreadsheetDelegate::paint(QPainter* painter,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);
    if (!sheet) {
        return;
    }
    App::CellAddress addr(index.row(), index.column());
    drawBorder(painter, option, sheet->getCellBindingBorder(addr), Qt::blue, Qt::SolidLine);
    drawBorder(painter, option, sheet->getCopyOrCutBorder(addr, true), Qt::green, Qt::DashLine);
    drawBorder(painter, option, sheet->getCopyOrCutBorder(addr, false), Qt::red, Qt::DashLine);
}

#include "moc_SpreadsheetDelegate.cpp"

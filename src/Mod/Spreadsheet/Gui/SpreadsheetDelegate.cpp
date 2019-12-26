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
# include <QLineEdit>
# include <QPainter>
#endif

#include "SpreadsheetDelegate.h"
#include "LineEdit.h"
#include <App/DocumentObject.h>
#include <Mod/Spreadsheet/App/Sheet.h>
#include <Gui/ExpressionCompleter.h>
#include "DlgBindSheet.h"

using namespace Spreadsheet;
using namespace SpreadsheetGui;

SpreadsheetDelegate::SpreadsheetDelegate(Spreadsheet::Sheet * _sheet, QWidget *parent)
    : QStyledItemDelegate(parent)
    , sheet(_sheet)
{
}

QWidget *SpreadsheetDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &,
                                          const QModelIndex &index) const
{
    App::CellAddress addr(index.row(),index.column());
    App::Range range(addr,addr);
    if(sheet && sheet->getCellBinding(range)) {
        DlgBindSheet dlg(sheet,{range},parent);
        dlg.exec();
        return 0;
    }

    SpreadsheetGui::LineEdit *editor = new SpreadsheetGui::LineEdit(parent);
    editor->setIndex(index);

    editor->setDocumentObject(sheet);
    connect(editor, SIGNAL(returnPressed()), this, SLOT(commitAndCloseEditor()));
    return editor;
}

void SpreadsheetDelegate::commitAndCloseEditor()
{
    Gui::ExpressionLineEdit *editor = qobject_cast<Gui::ExpressionLineEdit *>(sender());
    if (editor->completerActive()) {
        editor->hideCompleter();
        return;
    }

    // See https://forum.freecadweb.org/viewtopic.php?f=3&t=41694
    // It looks like the slot commitAndCloseEditor() is not needed any more and even
    // causes a crash when doing so because the LineEdit is still accessed after its destruction.
    //Q_EMIT commitData(editor);
    //Q_EMIT closeEditor(editor);
}

void SpreadsheetDelegate::setEditorData(QWidget *editor,
    const QModelIndex &index) const
{
    QLineEdit *edit = qobject_cast<QLineEdit*>(editor);
    if (edit) {
        edit->setText(index.model()->data(index, Qt::EditRole).toString());
        return;
    }
}

void SpreadsheetDelegate::setModelData(QWidget *editor,
    QAbstractItemModel *model, const QModelIndex &index) const
{
    QLineEdit *edit = qobject_cast<QLineEdit *>(editor);
    if (edit) {
        model->setData(index, edit->text());
        return;
    }
}

QSize SpreadsheetDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize();
}

void SpreadsheetDelegate::paint(QPainter *painter,
        const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    QStyledItemDelegate::paint(painter, option, index);
    if(!sheet)
        return;
    unsigned flags = sheet->getCellBindingBorder(App::CellAddress(index.row(),index.column()));
    if(!flags)
        return;
    QPen pen(Qt::blue);
    pen.setWidth(1);
    pen.setStyle(Qt::SolidLine);
    painter->setPen(pen);
    if(flags == Sheet::BorderAll) {
        painter->drawRect(option.rect);
        return;
    }
    if(flags & Sheet::BorderLeft) 
        painter->drawLine(option.rect.topLeft(), option.rect.bottomLeft());
    if(flags & Sheet::BorderTop) 
        painter->drawLine(option.rect.topLeft(), option.rect.topRight());
    if(flags & Sheet::BorderRight) 
        painter->drawLine(option.rect.topRight(), option.rect.bottomRight());
    if(flags & Sheet::BorderBottom) 
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
}

#include "moc_SpreadsheetDelegate.cpp"


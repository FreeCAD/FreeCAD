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
#include <QItemDelegate>
#include <QLineEdit>
#endif

#include "SpreadsheetDelegate.h"
#include "LineEdit.h"
#include <App/DocumentObject.h>
#include <Mod/Spreadsheet/App/Sheet.h>
#include <Gui/ExpressionCompleter.h>

using namespace SpreadsheetGui;

SpreadsheetDelegate::SpreadsheetDelegate(Spreadsheet::Sheet * _sheet, QWidget *parent)
    : QItemDelegate(parent)
    , sheet(_sheet)
{
}

QWidget *SpreadsheetDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &,
                                          const QModelIndex &index) const
{
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

    Q_EMIT commitData(editor);
    Q_EMIT closeEditor(editor);
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

#include "moc_SpreadsheetDelegate.cpp"


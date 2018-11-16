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
#include <QPushButton>
#include <QComboBox>
#endif

#include <Base/Tools.h>
#include "../App/Cell.h"
#include "SpreadsheetDelegate.h"
#include "LineEdit.h"
#include <App/DocumentObject.h>
#include <Mod/Spreadsheet/App/Sheet.h>
#include <Gui/ExpressionCompleter.h>

using namespace Spreadsheet;
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
    auto cell = sheet->getCell(App::CellAddress(index.row(),index.column()));
    if(cell && !cell->hasException()) {
        switch(cell->getEditMode()) {
        case Cell::EditButton: {
            auto button = new QPushButton(parent);
            connect(button, SIGNAL(clicked()), this, SLOT(commitAndCloseEditor()));
            return button;
        } 
        case Cell::EditCombo: {
            auto combo = new QComboBox(parent);
            connect(combo, SIGNAL(activated(const QString &)), this, SLOT(commitAndCloseEditor()));
            return combo;
        }
        default:
            break;
        }
    }

    SpreadsheetGui::TextEdit *editor = new SpreadsheetGui::TextEdit(parent);
    editor->setIndex(index);

    editor->setDocumentObject(sheet);
    connect(editor, SIGNAL(returnPressed()), this, SLOT(commitAndCloseEditor()));
    return editor;
}

void SpreadsheetDelegate::commitAndCloseEditor()
{
    Base::FlagToggler<> flag(commiting);
    Gui::ExpressionTextEdit *editor = qobject_cast<Gui::ExpressionTextEdit *>(sender());
    if(editor) {
        if (editor->completerActive()) {
            editor->hideCompleter();
            return;
        }
        Q_EMIT commitData(editor);
        Q_EMIT closeEditor(editor);
        return;
    }
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if(button) {
        Q_EMIT commitData(button);
        return;
    }
    QComboBox *combo = qobject_cast<QComboBox*>(sender());
    if(button) {
        Q_EMIT commitData(combo);
        Q_EMIT closeEditor(combo);
        return;
    }
}

void SpreadsheetDelegate::setEditorData(QWidget *editor,
    const QModelIndex &index) const
{
    QVariant data = index.model()->data(index, Qt::EditRole);
    TextEdit *edit = qobject_cast<TextEdit*>(editor);
    if (edit) {
        edit->setPlainText(data.toString());
        return;
    }
    QPushButton *button = qobject_cast<QPushButton*>(editor);
    if(button) {
        button->setText(data.toString());
        return;
    }
    QComboBox *combo = qobject_cast<QComboBox*>(editor);
    if(combo && (QMetaType::Type)data.type()==QMetaType::QStringList) {
        QStringList list = data.toStringList();
        QString txt = list.front();
        list.pop_front();
        combo->clear();
        combo->addItems(list);
        int index = combo->findText(txt);
        if(index>=0)
            combo->setCurrentIndex(index);
    }
}

void SpreadsheetDelegate::setModelData(QWidget *editor,
    QAbstractItemModel *model, const QModelIndex &index) const
{
    TextEdit *edit = qobject_cast<TextEdit *>(editor);
    if (edit) {
        model->setData(index, edit->toPlainText());
        return;
    }
    QPushButton *button = qobject_cast<QPushButton*>(editor);
    if(button) {
        // For button widget, make sure we are triggered by user clicking not
        // something else, like lost of focus.
        if(commiting)
            model->setData(index, QString());
        return;
    }
    QComboBox *combo = qobject_cast<QComboBox*>(editor);
    if(combo) {
        model->setData(index, combo->currentText());
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


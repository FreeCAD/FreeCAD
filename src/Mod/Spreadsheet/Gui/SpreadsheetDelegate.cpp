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
# include <QItemDelegate>
# include <QLineEdit>
# include <QPushButton>
# include <QComboBox>
# include <QPainter>
# include <QCheckBox>
# include <QHBoxLayout>
#endif

#include <Base/Tools.h>
#include <Base/Console.h>
#include "../App/Cell.h"
#include "SpreadsheetDelegate.h"
#include "LineEdit.h"
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <Mod/Spreadsheet/App/Sheet.h>
#include <Gui/ExpressionCompleter.h>
#include <Gui/QuantitySpinBox.h>
#include "DlgBindSheet.h"

FC_LOG_LEVEL_INIT("Spreadsheet",true,true)

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
        FC_ERR("Bound cell " << addr.toString() << " cannot be edited");
        return 0;
    }
    auto cell = sheet->getCell(App::CellAddress(index.row(),index.column()));
    if(cell && !cell->hasException()) {
        switch(cell->getEditMode()) {
        case Cell::EditButton: {
            auto button = new QPushButton(parent);
            connect(button, SIGNAL(clicked()), this, SLOT(commitAndCloseEditor()));
            return button;
        } 
        case Cell::EditLabel: {
            auto editor = new SpreadsheetGui::TextEdit(parent);
            if(cell->isPersistentEditMode())
                editor->setObjectName(QLatin1String("persistent"));
            else
                editor->setObjectName(QLatin1String("label"));
            editor->setContextMenuPolicy(Qt::NoContextMenu);
            editor->setIndex(index);
            connect(editor, SIGNAL(returnPressed()), this, SLOT(commitAndCloseEditor()));
            return editor;
        }
        case Cell::EditCombo: {
            auto combo = new QComboBox(parent);
            if(cell->isPersistentEditMode())
                combo->setObjectName(QLatin1String("persistent"));
            combo->setContextMenuPolicy(Qt::NoContextMenu);
            connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(commitAndCloseEditor()));
            return combo;
        }
        case Cell::EditQuantity: {
            auto spinbox = new Gui::QuantitySpinBox(parent);
            spinbox->ignoreSizeHint(true);
            if(cell->isPersistentEditMode())
                spinbox->setContextMenuPolicy(Qt::NoContextMenu);
            connect(spinbox, SIGNAL(editingFinished()), this, SLOT(commitAndCloseEditor()));
            return spinbox;
        }
        case Cell::EditCheckBox: {
            QWidget *widget = new QWidget(parent);
            QCheckBox *checkbox = new QCheckBox();
            checkbox->setObjectName(QLatin1String("checkbox"));
            QHBoxLayout *layout = new QHBoxLayout(widget);
            layout->addWidget(checkbox);
            layout->setAlignment(Qt::AlignCenter);
            layout->setContentsMargins(0,0,0,0);
            connect(checkbox, SIGNAL(clicked()), this, SLOT(commitAndCloseEditor()));
            return widget;
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
    Base::FlagToggler<> flag(committing);
    Gui::ExpressionTextEdit *editor = qobject_cast<Gui::ExpressionTextEdit *>(sender());
    if(editor) {
        if(!updating)
            Q_EMIT commitData(editor);
        if(editor->objectName().isEmpty())
            Q_EMIT closeEditor(editor);
        return;
    }
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if(button) {
        if(!updating)
            Q_EMIT commitData(button);
        return;
    }
    QComboBox *combo = qobject_cast<QComboBox*>(sender());
    if(combo) {
        if(!updating)
            Q_EMIT commitData(combo);
        if(combo->objectName().isEmpty())
            Q_EMIT closeEditor(combo);
        return;
    }
    Gui::QuantitySpinBox *spinbox = qobject_cast<Gui::QuantitySpinBox*>(sender());
    if(spinbox) {
        if(!updating)
            Q_EMIT commitData(spinbox);
        return;
    }
    QCheckBox *checkbox = qobject_cast<QCheckBox*>(sender());
    if(checkbox) {
        if(!updating)
            Q_EMIT commitData(checkbox->parentWidget());
        return;
    }
}

void SpreadsheetDelegate::setEditorData(QWidget *editor,
    const QModelIndex &index) const
{
    Base::StateLocker guard(updating);

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
    if(combo) {
        combo->clear();
        QList<QVariant> list = data.toList();
        if(list.size()>1) {
            QString txt = list.front().toString();
            int idx = list.front().toInt();
            QStringList items;
            for(int i=1; i<list.size(); ++i)
                items.append(list[i].toString());
            combo->addItems(items);
            if(idx <= 0)
                idx = combo->findText(txt);
            else
                --idx;
            if(idx>=0)
                combo->setCurrentIndex(idx);
            else if (!syncCombo) {
                Base::StateLocker guard(syncCombo);
                // Cannot find a matching current index for the combo. So we
                // update the the data to sync with the current index
                QList<QVariant> data;
                data.append(combo->currentText());
                data.append(combo->currentIndex()+1);
                App::CellAddress address(index.row(), index.column());
                sheet->editCell(address, data);
                sheet->recomputeFeature();
            }
        }
        return;
    }
    Gui::QuantitySpinBox *spinbox = qobject_cast<Gui::QuantitySpinBox*>(editor);
    if(spinbox) {
        try {
            Base::Quantity q;
            auto map = data.toHash();
            auto iter = map.find(QString::fromLatin1("value"));
            if(iter == map.end())
                q = qvariant_cast<Base::Quantity>(data);
            else
                q = qvariant_cast<Base::Quantity>(iter.value());
            spinbox->setValue(q);

            iter = map.find(QString::fromLatin1("step"));
            if(iter != map.end())
                spinbox->setSingleStep(iter.value().toDouble());

            iter = map.find(QString::fromLatin1("max"));
            if(iter != map.end())
                spinbox->setMaximum(iter.value().toDouble());

            iter = map.find(QString::fromLatin1("min"));
            if(iter != map.end())
                spinbox->setMinimum(iter.value().toDouble());

            iter = map.find(QString::fromLatin1("unit"));
            if(iter != map.end()) {
                double scale = 1;
                auto iter2 = map.find(QString::fromLatin1("scale"));
                if(iter2 != map.end()) {
                    scale = iter2.value().toDouble();
                    if(scale == 0.0)
                        scale = 1;
                }
                spinbox->setDisplayUnit(iter.value().toString(), scale);
            }

        } catch (Base::Exception &e) {
            e.ReportException();
            FC_ERR("Failed to setup quantity edit: " << e.what());
        }
        return;
    }

    auto checkbox = editor->findChild<QCheckBox*>(QLatin1String("checkbox"));
    if(checkbox) {
        auto list = data.toList();
        if(list.isEmpty())
            checkbox->setChecked(data.toBool());
        else {
            if(list.size()>0)
                checkbox->setChecked(list[0].toBool());
            if(list.size()>1)
                checkbox->setText(list[1].toString());
        }
        return;
    }
}

void SpreadsheetDelegate::setModelData(QWidget *editor,
    QAbstractItemModel *model, const QModelIndex &index) const
{
    if(updating)
        return;

    TextEdit *edit = qobject_cast<TextEdit *>(editor);
    if (edit) {
        model->setData(index, edit->toPlainText());
        return;
    }
    QPushButton *button = qobject_cast<QPushButton*>(editor);
    if(button) {
        // For button widget, make sure we are triggered by user clicking not
        // something else, like lost of focus.
        if(committing)
            model->setData(index, QVariant());
        return;
    }
    QComboBox *combo = qobject_cast<QComboBox*>(editor);
    if(combo) {
        QList<QVariant> data;
        data.append(combo->currentText());
        data.append(combo->currentIndex()+1);
        model->setData(index, data);
        return;
    }
    Gui::QuantitySpinBox *spinbox = qobject_cast<Gui::QuantitySpinBox*>(editor);
    if(spinbox) {
        model->setData(index, QVariant::fromValue(spinbox->value()));
        return;
    }
    QCheckBox *checkbox = qobject_cast<QCheckBox*>(editor);
    if(!checkbox)
        checkbox = editor->findChild<QCheckBox*>(QLatin1String("checkbox"));
    if(checkbox) {
        model->setData(index, checkbox->isChecked());
        return;
    }
}

QSize SpreadsheetDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize();
}

static inline void drawBorder(QPainter *painter, const QStyleOptionViewItem &option,
        unsigned flags, QColor color, Qt::PenStyle style)
{
    if(!flags)
        return;
    QPen pen(color);
    pen.setWidth(2);
    pen.setStyle(style);
    painter->setPen(pen);

    QRect rect = option.rect.adjusted(1,1,0,0);
    if(flags == Sheet::BorderAll) {
        painter->drawRect(rect);
        return;
    }
    if(flags & Sheet::BorderLeft) 
        painter->drawLine(rect.topLeft(), rect.bottomLeft());
    if(flags & Sheet::BorderTop) 
        painter->drawLine(rect.topLeft(), rect.topRight());
    if(flags & Sheet::BorderRight) 
        painter->drawLine(rect.topRight(), rect.bottomRight());
    if(flags & Sheet::BorderBottom) 
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
}

void SpreadsheetDelegate::paint(QPainter *painter,
        const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    QStyledItemDelegate::paint(painter, option, index);
    if(!sheet)
        return;
    App::CellAddress addr(index.row(), index.column());
    drawBorder(painter, option, sheet->getCellBindingBorder(addr), Qt::blue, Qt::SolidLine);
    drawBorder(painter, option, sheet->getCopyOrCutBorder(addr,true), Qt::green, Qt::DashLine);
    drawBorder(painter, option, sheet->getCopyOrCutBorder(addr,false), Qt::red, Qt::DashLine);
}

#include "moc_SpreadsheetDelegate.cpp"


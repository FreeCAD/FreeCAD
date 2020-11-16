/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
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
# include <QApplication>
# include <QModelIndex>
# include <QPainter>
#endif

#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include "PropertyItemDelegate.h"
#include "PropertyItem.h"
#include "PropertyEditor.h"
#include "View3DInventorViewer.h"
#include "Tree.h"

FC_LOG_LEVEL_INIT("PropertyView",true,true)

using namespace Gui::PropertyEditor;


PropertyItemDelegate::PropertyItemDelegate(QObject* parent)
    : QItemDelegate(parent), expressionEditor(0)
    , pressed(false), changed(false)
{
}

PropertyItemDelegate::~PropertyItemDelegate()
{
}

QSize PropertyItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QSize size = QItemDelegate::sizeHint(option, index);
    size += QSize(0, 5);
    return size;
}

void PropertyItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    QStyleOptionViewItem option = opt;

    PropertyItem *property = static_cast<PropertyItem*>(index.internalPointer());

    if (property && property->isSeparator()) {
        QColor color = option.palette.color(QPalette::BrightText);
        QObject* par = parent();
        if (par) {
            QVariant value = par->property("groupTextColor");
            if (value.canConvert<QColor>())
                color = value.value<QColor>();
        }
        option.palette.setColor(QPalette::Text, color);
        option.font.setBold(true);

        // Since the group item now parents all the property items and can be
        // collpased, it makes sense to have some selection visual clue for it.
        //
        // option.state &= ~QStyle::State_Selected;
    }
    else if (index.column() == 1) {
        option.state &= ~QStyle::State_Selected;
    }

    option.state &= ~QStyle::State_HasFocus;

    QObject* par = parent();
    if (par) {
        if (property && property->isSeparator()) {
            QBrush brush = option.palette.dark();
            QVariant value = par->property("groupBackground");
            if (value.canConvert<QBrush>())
                brush = value.value<QBrush>();
            painter->fillRect(option.rect, brush);
        }
#if QT_VERSION  >= 0x050000
        else if (!(option.features & QStyleOptionViewItem::Alternate)) {
            QVariant value = par->property("itemBackground");
            if (value.canConvert<QBrush>()) {
                QBrush brush = value.value<QBrush>();
                if (brush.color().alpha() != 0)
                    painter->fillRect(option.rect, brush);
            }
        }
#endif
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

bool PropertyItemDelegate::editorEvent (QEvent * event, QAbstractItemModel* model,
                                        const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event && event->type() == QEvent::MouseButtonPress)
        this->pressed = true;
    else
        this->pressed = false;
    return QItemDelegate::editorEvent(event, model, option, index);
}

bool PropertyItemDelegate::eventFilter(QObject *o, QEvent *ev)
{
    if (ev->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(ev);
        if (ke->key() == Qt::Key_Return) {
            auto label = qobject_cast<QLabel*>(o);
            if (label && qobject_cast<LinkLabel*>(label->parentWidget())) {
                LinkLabel *ll = static_cast<LinkLabel*>(label->parentWidget());
                QMetaObject::invokeMethod(ll, "returnPressed", Qt::QueuedConnection);
                return true;
            }
        }
    }
    else if (ev->type() == QEvent::FocusOut) {
        PropertyEditor *parentEditor = qobject_cast<PropertyEditor*>(this->parent());
        auto widget = qobject_cast<QWidget*>(o);
        if (widget && parentEditor && parentEditor->activeEditor
                   && widget != parentEditor->activeEditor)
        {
            // We event filter child QAbstractButton and QLabel of an editor,
            // which requires special focus change in order to not mess up with
            // QItemDelegate's logic.
            QWidget *w = QApplication::focusWidget();
            while (w) { // don't worry about focus changes internally in the editor
                if (w == widget || w == parentEditor->activeEditor)
                    return false;

                // ignore focus change to 3D view or tree view, because, for
                // example DlgPropertyLink is implemented as modal-less dialog
                // to allow selection in 3D and tree view.
                if (qobject_cast<View3DInventorViewer*>(w))
                    return false;
                if (qobject_cast<TreeWidget*>(w))
                    return false;
                w = w->parentWidget();
            }
        }
    }
    return QItemDelegate::eventFilter(o, ev);
}

QWidget * PropertyItemDelegate::createEditor (QWidget * parent, const QStyleOptionViewItem & /*option*/, 
                                              const QModelIndex & index ) const
{
    if (!index.isValid())
        return 0;

    PropertyItem *childItem = static_cast<PropertyItem*>(index.internalPointer());
    if (!childItem)
        return 0;

    PropertyEditor *parentEditor = qobject_cast<PropertyEditor*>(this->parent());
    if(parentEditor)
        parentEditor->closeEditor();

    if (childItem->isSeparator())
        return 0;

    FC_LOG("create editor " << index.row() << "," << index.column());

    QWidget* editor;
    expressionEditor = 0;
    if(parentEditor && parentEditor->isBinding())
        expressionEditor = editor = childItem->createExpressionEditor(parent, this, SLOT(valueChanged()));
    else
        editor = childItem->createEditor(parent, this, SLOT(valueChanged()));
    if (editor) // Make sure the editor background is painted so the cell content doesn't show through
        editor->setAutoFillBackground(true);
    if (editor && childItem->isReadOnly())
        editor->setDisabled(true);
    else if (editor /*&& this->pressed*/) {
        // We changed the way editor is activated in PropertyEditor (in response
        // of signal activated and clicked), so now we should grab focus
        // regardless of "pressed" or not (e.g. when activated by keyboard
        // enter)
        editor->setFocus();
    }
    this->pressed = false;

    if (editor) {
        for (auto w : editor->findChildren<QWidget*>()) {
            if (qobject_cast<QAbstractButton*>(w)
                    || qobject_cast<QLabel*>(w))
            {
                w->installEventFilter(const_cast<PropertyItemDelegate*>(this));
            }
        }
        parentEditor->activeEditor = editor;
        parentEditor->editingIndex = index;
    }

    return editor;
}

void PropertyItemDelegate::valueChanged()
{
    QWidget* editor = qobject_cast<QWidget*>(sender());
    if (editor) {
        Base::FlagToggler<> flag(changed);
        commitData(editor);
    }
}

void PropertyItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (!index.isValid())
        return;
    QVariant data = index.data(Qt::EditRole);
    PropertyItem *childItem = static_cast<PropertyItem*>(index.internalPointer());
    editor->blockSignals(true);
    if(expressionEditor == editor)
        childItem->setExpressionEditorData(editor, data);
    else
        childItem->setEditorData(editor, data);
    editor->blockSignals(false);
    return;
}

void PropertyItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (!index.isValid() || !changed)
        return;
    PropertyItem *childItem = static_cast<PropertyItem*>(index.internalPointer());
    QVariant data;
    if(expressionEditor == editor)
        data = childItem->expressionEditorData(editor);
    else
        data = childItem->editorData(editor);
    model->setData(index, data, Qt::EditRole);
}

#include "moc_PropertyItemDelegate.cpp"

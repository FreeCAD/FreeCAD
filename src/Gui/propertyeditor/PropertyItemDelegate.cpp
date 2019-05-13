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

FC_LOG_LEVEL_INIT("PropertyView",true,true);

using namespace Gui::PropertyEditor;


PropertyItemDelegate::PropertyItemDelegate(QObject* parent)
    : QItemDelegate(parent), expressionEditor(0)
    , pressed(false), activeTransactionID(0),changed(false)
{
    connect(this, SIGNAL(closeEditor(QWidget*, QAbstractItemDelegate::EndEditHint)),
            this, SLOT(editorClosed(QWidget*, QAbstractItemDelegate::EndEditHint)));
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
        option.state &= ~QStyle::State_Selected;
    }

    if (index.column() == 1) {
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

bool PropertyItemDelegate::editorEvent (QEvent * event, QAbstractItemModel* model,
                                        const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event && event->type() == QEvent::MouseButtonPress)
        this->pressed = true;
    else
        this->pressed = false;
    return QItemDelegate::editorEvent(event, model, option, index);
}

void PropertyItemDelegate::editorClosed(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    int id = 0;
    auto &app = App::GetApplication();
    const char *name = app.getActiveTransaction(&id);
    if(id && id==activeTransactionID) {
        FC_LOG("editor close transaction " << name);
        app.closeActiveTransaction(false,id);
        activeTransactionID = 0;
    }
    FC_LOG("editor close " << editor);

    // don't close the editor when pressing Tab or Shift+Tab
    // https://forum.freecadweb.org/viewtopic.php?f=3&t=34627#p290957
    if (editor && hint != EditNextItem && hint != EditPreviousItem)
        editor->close();
}

QWidget * PropertyItemDelegate::createEditor (QWidget * parent, const QStyleOptionViewItem & /*option*/, 
                                              const QModelIndex & index ) const
{
    if (!index.isValid())
        return 0;

    PropertyItem *childItem = static_cast<PropertyItem*>(index.internalPointer());
    if (!childItem)
        return 0;

    FC_LOG("create editor " << index.row() << "," << index.column());

    PropertyEditor *parentEditor = qobject_cast<PropertyEditor*>(this->parent());
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

    auto &app = App::GetApplication();
    if(!app.autoTransaction()) 
        return editor;
    else if(app.getActiveTransaction())
        FC_LOG("editor already transacting " << app.getActiveTransaction());
    else {
        auto items = childItem->getPropertyData();
        for(auto propItem=childItem->parent();items.empty() && propItem;propItem=propItem->parent())
            items = propItem->getPropertyData();
        if(items.empty()) 
            FC_LOG("editor no item");
        else {
            auto prop = items[0];
            auto parent = prop->getContainer();
            auto obj  = dynamic_cast<App::DocumentObject*>(parent);
            if(!obj || !obj->getDocument())
                FC_LOG("invalid object");
            else if(obj->getDocument()->hasPendingTransaction())
                FC_LOG("pending transaction");
            else {
                std::ostringstream str;
                str << "Change ";
                for(auto prop : items) {
                    if(prop->getContainer()!=obj) {
                        obj = 0;
                        break;
                    }
                }
                if(obj && obj->getNameInDocument())
                    str << obj->getNameInDocument() << '.';
                else
                    str << "property ";
                str << prop->getName();
                if(items.size()>1)
                    str << "...";
                activeTransactionID = app.setActiveTransaction(str.str().c_str());
                FC_LOG("editor transaction " << app.getActiveTransaction());
            }
        }
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

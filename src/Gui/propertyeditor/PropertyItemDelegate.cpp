// SPDX-License-Identifier: LGPL-2.1-or-later
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

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QModelIndex>
#include <QPainter>
#include <QSignalBlocker>
#include <QTimer>
#include <QKeyEvent>

#include <Base/Tools.h>

#include "PropertyItemDelegate.h"
#include "MDIView.h"
#include "PropertyEditor.h"
#include "PropertyItem.h"
#include "Tree.h"


FC_LOG_LEVEL_INIT("PropertyView", true, true)

using namespace Gui::PropertyEditor;


PropertyItemDelegate::PropertyItemDelegate(QObject* parent)
    : QItemDelegate(parent)
    , expressionEditor(nullptr)
    , pressed(false)
    , changed(false)
{}

PropertyItemDelegate::~PropertyItemDelegate() = default;

QSize PropertyItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize size = QItemDelegate::sizeHint(option, index);
    size += QSize(0, 5);
    return size;
}

void PropertyItemDelegate::paint(
    QPainter* painter,
    const QStyleOptionViewItem& opt,
    const QModelIndex& index
) const
{
    QStyleOptionViewItem option = opt;

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
        if (property && property->isReadOnly()) {
            option.state &= ~QStyle::State_Enabled;
        }
    }

    option.state &= ~QStyle::State_HasFocus;

    if (property && property->isSeparator()) {
        QBrush brush = option.palette.dark();
        QObject* par = parent();
        if (par) {
            QVariant value = par->property("groupBackground");
            if (value.canConvert<QBrush>()) {
                brush = value.value<QBrush>();
            }
        }
        painter->fillRect(option.rect, brush);
    }

    QPen savedPen = painter->pen();

    if (index.column() == 1 && property && dynamic_cast<PropertyBoolItem*>(property)) {
        bool checked = index.data(Qt::EditRole).toBool();
        bool readonly = property->isReadOnly();

        QStyle* style = option.widget ? option.widget->style() : QApplication::style();
        QPalette palette = option.widget ? option.widget->palette() : QApplication::palette();

        QStyleOptionButton checkboxOption;

        checkboxOption.state |= readonly ? QStyle::State_ReadOnly : QStyle::State_Enabled;
        checkboxOption.state |= checked ? QStyle::State_On : QStyle::State_Off;

        // draw the item (background etc.)
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);

        // Draw the checkbox
        checkboxOption.rect
            = style->subElementRect(QStyle::SE_CheckBoxIndicator, &checkboxOption, option.widget);
        int leftSpacing = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, option.widget);

        QRect checkboxRect = QStyle::alignedRect(
            option.direction,
            Qt::AlignVCenter,
            checkboxOption.rect.size(),
            option.rect.adjusted(leftSpacing, 0, -leftSpacing, 0)
        );
        checkboxOption.rect = checkboxRect;

        style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &checkboxOption, painter, option.widget);

        // Draw the label of the checkbox
        QString labelText = checked ? tr("Yes") : tr("No");
        int spacing = style->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr, option.widget);
        QRect textRect(
            checkboxOption.rect.right() + spacing,
            checkboxOption.rect.top(),
            option.rect.right() - (checkboxOption.rect.right() + spacing),
            checkboxOption.rect.height()
        );
        if (readonly) {
            painter->setPen(palette.color(QPalette::Disabled, QPalette::Text));
        }
        else {
            painter->setPen(palette.color(QPalette::Text));
        }
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, labelText);
    }
    else {
        QItemDelegate::paint(painter, option, index);
    }

    QColor color = static_cast<QRgb>(QApplication::style()->styleHint(
        QStyle::SH_Table_GridLineColor,
        &opt,
        qobject_cast<QWidget*>(parent())
    ));
    painter->setPen(QPen(color));
    if (index.column() == 1 || !(property && property->isSeparator())) {
        int right = (option.direction == Qt::LeftToRight) ? option.rect.right() : option.rect.left();
        painter->drawLine(right, option.rect.y(), right, option.rect.bottom());
    }
    painter->drawLine(option.rect.x(), option.rect.bottom(), option.rect.right(), option.rect.bottom());
    painter->setPen(savedPen);
}

bool PropertyItemDelegate::editorEvent(
    QEvent* event,
    QAbstractItemModel* model,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
)
{
    auto property = static_cast<PropertyItem*>(index.internalPointer());

    if ((property && !property->isSeparator())
        && (!event || event->type() == QEvent::MouseButtonDblClick)) {
        // ignore double click, as it could cause editor lock with checkboxes
        // due to the editor being close immediately after toggling the checkbox
        // which is currently done on first click
        this->pressed = true;
        return true;
    }
    bool mouseButton = event->type() == QEvent::MouseButtonPress;
    if (mouseButton) {
        this->pressed = true;
    }
    return QItemDelegate::editorEvent(event, model, option, index);
}

bool PropertyItemDelegate::eventFilter(QObject* o, QEvent* ev)
{
    if (ev->type() == QEvent::KeyPress) {
        auto* checkBox = qobject_cast<QCheckBox*>(o);
        if (checkBox) {
            auto* keyEvent = static_cast<QKeyEvent*>(ev);
            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter
                || keyEvent->key() == Qt::Key_Space) {

                checkBox->toggle();

                // Manually commit the data WITHOUT closing the editor.
                // This keeps the focus on the checkbox so subsequent 'Enter'
                // presses will toggle it again immediately.
                if (propertyEditor) {
                    // We must set 'changed' to true so setModelData updates the model,
                    // then revert it back (handled by FlagToggler).
                    Base::FlagToggler<> flag(changed);
                    Q_EMIT commitData(propertyEditor);
                }
                return true;
            }
        }
    }
    else if (ev->type() == QEvent::FocusIn) {
        if (auto* comboBox = qobject_cast<QComboBox*>(o); comboBox && propertyEditor == comboBox) {
            comboBox->showPopup();
        }
        if (auto* checkBox = qobject_cast<QCheckBox*>(o); checkBox && propertyEditor == checkBox) {
            if (this->pressed) {
                checkBox->toggle();
                QTimer::singleShot(0, checkBox, [this, checkBox]() {
                    if (propertyEditor == checkBox) {
                        valueChanged();
                    }
                });
            }
        }
        this->pressed = false;
    }
    else if (ev->type() == QEvent::FocusOut) {
        if (auto button = qobject_cast<Gui::ColorButton*>(o)) {
            // Ignore the event if the ColorButton's modal dialog is active.
            if (button->property("modal_dialog_active").toBool()) {
                return true;
            }
        }
    }
    QPointer<QObject> guardedObject(o);
    const bool filtered = QItemDelegate::eventFilter(o, ev);
    // Closing an editor can delete it while this event is being filtered.
    return filtered || !guardedObject;
}

QWidget* PropertyItemDelegate::createEditor(
    QWidget* parent,
    const QStyleOptionViewItem& /*option*/,
    const QModelIndex& index
) const
{
    if (!index.isValid()) {
        return nullptr;
    }

    auto childItem = static_cast<PropertyItem*>(index.internalPointer());
    if (!childItem || childItem->isSeparator() || childItem->isReadOnly()) {
        return nullptr;
    }

    auto parentEditor = qobject_cast<PropertyEditor*>(this->parent());
    auto createEditor = [this, childItem, parent]() {
        // Can't use a terniary here because the lambdas have different types.
        if (qobject_cast<PropertyBoolItem*>(childItem)) {
            // Boolean properties use a checkbox that is basically artificial
            // (it is not rendered).  Therefore, the callback is handled in
            // eventFilter()
            return childItem->createEditor(parent, []() noexcept {});
        }
        return childItem->createEditor(parent, [this]() {
            const_cast<PropertyItemDelegate*>(this)->valueChanged();  // NOLINT
        });
    };

    FC_LOG("create editor " << index.row() << "," << index.column());
    QWidget* editor = nullptr;
    expressionEditor = nullptr;
    userEditor = nullptr;
    if (parentEditor && parentEditor->isBinding()) {
        expressionEditor = editor = childItem->createExpressionEditor(parent, [this]() {
            const_cast<PropertyItemDelegate*>(this)->valueChanged();  // NOLINT
        });
        propertyEditor = editor;
    }
    else {
        const auto& props = childItem->getPropertyData();
        if (!props.empty() && props[0]->testStatus(App::Property::UserEdit)) {
            editor = userEditor = childItem->createPropertyEditorWidget(parent);
            propertyEditor = editor;
        }
        else {
            propertyEditor = editor = createEditor();
        }
    }
    if (editor) {
        // Make sure the editor background is painted so the cell content doesn't show through
        editor->setAutoFillBackground(true);
        Q_EMIT const_cast<PropertyItemDelegate*>(this)->editorCreated(editor, index);
    }
    return editor;
}

void PropertyItemDelegate::valueChanged()
{
    QPointer<QWidget> editor = propertyEditor;
    if (!editor) {
        return;
    }
    Base::FlagToggler<> flag(changed);
    Q_EMIT commitData(editor);
    if (editor && (qobject_cast<QComboBox*>(editor) || qobject_cast<QCheckBox*>(editor))) {
        Q_EMIT closeEditor(editor);
    }
}

void PropertyItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (!index.isValid()) {
        return;
    }
    QVariant data = index.data(Qt::EditRole);
    auto childItem = static_cast<PropertyItem*>(index.internalPointer());
    const QSignalBlocker blocker(editor);
    if (expressionEditor == editor) {
        childItem->setExpressionEditorData(editor, data);
    }
    else if (userEditor == editor) {
        userEditor->setValue(PropertyItemAttorney::toString(childItem, data));
    }
    else {
        childItem->setEditorData(editor, data);
    }
}

void PropertyItemDelegate::setModelData(
    QWidget* editor,
    QAbstractItemModel* model,
    const QModelIndex& index
) const
{
    if (!index.isValid() || userEditor) {
        return;
    }
    auto childItem = static_cast<PropertyItem*>(index.internalPointer());
    const bool commitOnClose = childItem->commitOnEditorClose();
    if (!changed && !commitOnClose) {
        return;
    }
    QVariant data;
    if (expressionEditor == editor) {
        data = childItem->expressionEditorData(editor);
    }
    else {
        data = childItem->editorData(editor);
    }
    if (commitOnClose && !changed && data == index.data(Qt::EditRole)) {
        return;
    }
    model->setData(index, data, Qt::EditRole);
}

#include "moc_PropertyItemDelegate.cpp"

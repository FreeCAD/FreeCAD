// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QApplication>
# include <QGuiApplication>
# include <QKeyEvent>
# include <QPainter>
# include <QTimer>
# include <QVBoxLayout>
# include <QWindow>
#endif

#include "CommandPalette.h"
#include "CommandCompleter.h"
#include "Application.h"
#include "Command.h"
#include "Action.h"
#include "MainWindow.h"

using namespace Gui;

////////////////////// CommandItemDelegate implementation //////////////////////
CommandItemDelegate::CommandItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

void CommandItemDelegate::paint(
    QPainter* painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const
{
    if (!index.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();

    // draw bg and selection
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    option.widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, option.widget);

    QString title = index.data(CommandMenuTextRole).toString();
    QString tooltip = index.data(Qt::ToolTipRole).toString();
    QString groupName = index.data(CommandGroupRole).toString();
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();

    QRect rect = option.rect;
    constexpr int iconSize = 24;
    constexpr int margin = 8;
    constexpr int spacing = 8;

    // check if item is enabled (for graying out)
    bool isEnabled = (index.flags() & Qt::ItemIsEnabled);

    // draw icon
    QRect iconRect = rect;
    iconRect.setLeft(rect.left() + margin);
    iconRect.setTop(rect.top() + ((rect.height() - iconSize) / 2));
    iconRect.setSize(QSize(iconSize, iconSize));

    if (!icon.isNull()) {
        QIcon::Mode mode = isEnabled ? QIcon::Normal : QIcon::Disabled;
        icon.paint(painter, iconRect, Qt::AlignCenter, mode);
    }

    // adjust rect for text (after icon)
    QRect textRect = rect;
    textRect.setLeft(iconRect.right() + spacing);
    textRect.setRight(rect.right() - margin);
    textRect.adjust(0, 4, 0, -4);

    // draw title
    QFont titleFont = option.font;
    painter->setFont(titleFont);

    QColor textColor = option.palette.color(
        isEnabled ? QPalette::Normal : QPalette::Disabled,
        option.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text
    );
    painter->setPen(textColor);

    QRect titleRect = textRect;
    if (!tooltip.isEmpty()) {
        titleRect.setHeight(textRect.height() / 2);
    }

    // draw group name on the right if available
    if (!groupName.isEmpty()) {
        QFont groupFont = titleFont;
        groupFont.setPointSize(qMax(groupFont.pointSize() - 1, 8));
        painter->setFont(groupFont);

        QColor groupColor = textColor;
        // make group name transparent
        groupColor.setAlpha(150);
        painter->setPen(groupColor);

        QFontMetrics groupFm(groupFont);
        int groupWidth = groupFm.horizontalAdvance(groupName);

        QRect groupRect = titleRect;
        groupRect.setLeft(titleRect.right() - groupWidth);

        painter->drawText(groupRect, Qt::AlignRight | Qt::AlignVCenter, groupName);

        // adjust title rect to not overlap with group name
        titleRect.setRight(groupRect.left() - 10);

        // reset font and color for title
        painter->setFont(titleFont);
        painter->setPen(textColor);
    }

    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, title);

    // draw tooltip/description and description if it is avaialble
    if (!tooltip.isEmpty()) {
        QFont tooltipFont = option.font;
        tooltipFont.setPointSize(qMax(tooltipFont.pointSize() - 1, 8));
        painter->setFont(tooltipFont);

        QColor tooltipColor = textColor;
        // make the tooltip a bit transparent
        tooltipColor.setAlpha(180);
        painter->setPen(tooltipColor);

        QRect tooltipRect = textRect;
        tooltipRect.setTop(titleRect.bottom());

        // the model gives plain text, elide it if it is too long
        QFontMetrics fm(tooltipFont);
        QString elidedTooltip = fm.elidedText(tooltip, Qt::ElideRight, tooltipRect.width());

        painter->drawText(tooltipRect, Qt::AlignLeft | Qt::AlignVCenter, elidedTooltip);
    }

    painter->restore();
}

QSize CommandItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index)
    // every row has room for a description, so the view doesn't have to ask each row for its size;
    // the list view stretches the rows to its width
    const double height = option.fontMetrics.height() * 2.8;

    return {0, static_cast<int>(height)};
}

////////////////////// CommandPalette implementation //////////////////////

CommandPalette::CommandPalette(QWidget* parent)
    : QDialog(parent)
{
    setupUi();

    setModal(false);
    // a popup closes by itself on a click outside it or on Escape
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);

    searchLineEdit->installEventFilter(this);
}

void CommandPalette::setupUi()
{
    constexpr int layoutMargin = 10;
    constexpr int layoutSpacing = 5;
    constexpr int searchMinWidth = 500;
    constexpr int searchMinHeight = 30;
    constexpr int listMinHeight = 400;
    constexpr int listMaxHeight = 600;
    constexpr int paletteMinWidth = 520;
    constexpr int paletteMaxWidth = 800;
    constexpr int paletteMinHeight = 450;

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(layoutMargin, layoutMargin, layoutMargin, layoutMargin);
    mainLayout->setSpacing(layoutSpacing);

    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText(tr("Type a command name..."));
    searchLineEdit->setClearButtonEnabled(true);
    searchLineEdit->setMinimumWidth(searchMinWidth);
    searchLineEdit->setMinimumHeight(searchMinHeight);

    commandListView = new QListView(this);
    commandListView->setMinimumHeight(listMinHeight);
    commandListView->setMaximumHeight(listMaxHeight);
    commandListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    commandListView->setSelectionMode(QAbstractItemView::SingleSelection);
    commandListView->setItemDelegate(new CommandItemDelegate(this));
    commandListView->setUniformItemSizes(true);
    commandListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    mainLayout->addWidget(searchLineEdit);
    mainLayout->addWidget(commandListView);

    connect(searchLineEdit, &QLineEdit::textChanged, this, &CommandPalette::onTextChanged);
    connect(commandListView, &QListView::activated, this, &CommandPalette::onListItemActivated);

    setMinimumWidth(paletteMinWidth);
    setMaximumWidth(paletteMaxWidth);
    setMinimumHeight(paletteMinHeight);
}

void CommandPalette::createCompleter()
{
    completer = new CommandCompleter(searchLineEdit, this);

    // Remove widget association to prevent completer's popup; we embed the
    // completion model inside our own QListView instead.
    completer->setWidget(nullptr);
    disconnect(searchLineEdit, nullptr, completer, nullptr);

    commandListView->setModel(completer->completionModel());
    connect(completer, &CommandCompleter::commandActivated, this, &CommandPalette::onCommandActivated);
}

void CommandPalette::refreshCommands()
{
    completer->setFilterInactive(true);
    // keep what was typed while the list was being filled
    completer->setCompletionPrefix(searchLineEdit->text());

    if (commandListView->model()->rowCount() > 0) {
        commandListView->setCurrentIndex(commandListView->model()->index(0, 0));
    }
}

void CommandPalette::showPalette()
{
    searchLineEdit->clear();

    centerOnMainWindow();

    show();
    raise();
    activateWindow();

    searchLineEdit->setFocus();

    // the first time the list is filled after the palette is on screen, see paintEvent()
    if (completer) {
        refreshCommands();
    }
}

void CommandPalette::paintEvent(QPaintEvent* event)
{
    QDialog::paintEvent(event);

    // Building the command list takes a moment the first time, so show the empty palette right
    // away and fill it once it has been drawn.
    if (!completer && !fillPending) {
        fillPending = true;
        QTimer::singleShot(0, this, [this] {
            createCompleter();
            refreshCommands();
        });
    }
}

void CommandPalette::centerOnMainWindow()
{
    QWidget* mainWindow = getMainWindow();
    if (!mainWindow) {
        return;
    }

    QRect mainWindowRect = mainWindow->geometry();

    int x = mainWindowRect.x() + ((mainWindowRect.width() - width()) / 2);
    int y = mainWindowRect.y() + (mainWindowRect.height() / 4);

    move(x, y);
}

bool CommandPalette::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == searchLineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // esc closes the palette
        if (keyEvent->key() == Qt::Key_Escape) {
            if (searchLineEdit->text().isEmpty()) {
                close();
                return true;
            }
        }

        // forward events to list, but basically they are used for going up/down
        if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up
            || keyEvent->key() == Qt::Key_PageDown || keyEvent->key() == Qt::Key_PageUp) {
            QApplication::sendEvent(commandListView, event);
            return true;
        }

        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            QModelIndex currentIndex = commandListView->currentIndex();
            if (currentIndex.isValid()) {
                onListItemActivated(currentIndex);
                return true;
            }
        }
    }

    return QDialog::eventFilter(obj, event);
}

void CommandPalette::onCommandActivated(const QByteArray& commandName)
{
    // Get the command
    Command* cmd = Application::Instance->commandManager().getCommandByName(commandName.constData());
    if (!cmd) {
        // not sure how would it be possible to get here, but just as a sanity check
        Base::Console().warning("CMD Palette:: Command '%s' not found\n", commandName.constData());
        close();
        return;
    }

    close();

    // run cmd
    try {
        Application::Instance->commandManager().runCommandByName(commandName.constData());
    }
    catch (const Base::Exception& e) {
        Base::Console().error(
            "CMD Palette:: Error executing command '%s': %s\n",
            commandName.constData(),
            e.what()
        );
    }
    catch (...) {
        Base::Console().error(
            "CMD Palette:: Unknown error executing command '%s'\n",
            commandName.constData()
        );
    }
}

void CommandPalette::onTextChanged(const QString& text)
{
    // the list isn't filled yet, refreshCommands() takes the text into account
    if (!completer) {
        return;
    }

    // update completer filter to match the text
    completer->setCompletionPrefix(text);

    // select first matched item
    if (commandListView->model()->rowCount() > 0) {
        commandListView->setCurrentIndex(commandListView->model()->index(0, 0));
    }
}

void CommandPalette::onListItemActivated(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    QByteArray commandName = commandListView->model()->data(index, CommandNameRole).toByteArray();

    if (!commandName.isEmpty()) {
        onCommandActivated(commandName);
    }
}

#include "moc_CommandPalette.cpp"

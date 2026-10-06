// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD Project Association AISBL
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>

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
    constexpr int textPadding = 4;
    constexpr int groupSpacing = 10;
    constexpr int smallestPointSize = 8;
    constexpr int groupAlpha = 150;
    constexpr int descriptionAlpha = 180;

    bool isEnabled = (index.flags() & Qt::ItemIsEnabled);

    QRect iconRect = rect;
    iconRect.setLeft(rect.left() + margin);
    iconRect.setTop(rect.top() + ((rect.height() - iconSize) / 2));
    iconRect.setSize(QSize(iconSize, iconSize));

    if (!icon.isNull()) {
        QIcon::Mode mode = isEnabled ? QIcon::Normal : QIcon::Disabled;
        icon.paint(painter, iconRect, Qt::AlignCenter, mode);
    }

    QRect textRect = rect;
    textRect.setLeft(iconRect.right() + spacing);
    textRect.setRight(rect.right() - margin);
    textRect.adjust(0, textPadding, 0, -textPadding);

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

    if (!groupName.isEmpty()) {
        QFont groupFont = titleFont;
        groupFont.setPointSize(qMax(groupFont.pointSize() - 1, smallestPointSize));
        painter->setFont(groupFont);

        QColor groupColor = textColor;
        groupColor.setAlpha(groupAlpha);
        painter->setPen(groupColor);

        QFontMetrics groupFm(groupFont);
        int groupWidth = groupFm.horizontalAdvance(groupName);

        QRect groupRect = titleRect;
        groupRect.setLeft(titleRect.right() - groupWidth);

        painter->drawText(groupRect, Qt::AlignRight | Qt::AlignVCenter, groupName);

        titleRect.setRight(groupRect.left() - groupSpacing);

        painter->setFont(titleFont);
        painter->setPen(textColor);
    }

    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, title);

    if (!tooltip.isEmpty()) {
        QFont tooltipFont = option.font;
        tooltipFont.setPointSize(qMax(tooltipFont.pointSize() - 1, smallestPointSize));
        painter->setFont(tooltipFont);

        QColor tooltipColor = textColor;
        tooltipColor.setAlpha(descriptionAlpha);
        painter->setPen(tooltipColor);

        QRect tooltipRect = textRect;
        tooltipRect.setTop(titleRect.bottom());

        QFontMetrics fm(tooltipFont);
        QString elidedTooltip = fm.elidedText(tooltip, Qt::ElideRight, tooltipRect.width());

        painter->drawText(tooltipRect, Qt::AlignLeft | Qt::AlignVCenter, elidedTooltip);
    }

    painter->restore();
}

QSize CommandItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index)
    // all rows have room for a description, the list view stretches them to its width
    constexpr double linesPerRow = 2.8;
    const double height = option.fontMetrics.height() * linesPerRow;

    return {0, static_cast<int>(height)};
}

////////////////////// CommandPalette implementation //////////////////////

CommandPalette::CommandPalette(QWidget* parent)
    : QDialog(parent)
{
    setupUi();

    // a popup closes by itself on a click outside it or on Escape
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);

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

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(layoutMargin, layoutMargin, layoutMargin, layoutMargin);
    mainLayout->setSpacing(layoutSpacing);

    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText(tr("Type a command name…"));
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
    completer->setSearchText(searchLineEdit->text());
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

    constexpr int heightDivisor = 4;
    QRect mainWindowRect = mainWindow->geometry();

    int x = mainWindowRect.x() + ((mainWindowRect.width() - width()) / 2);
    int y = mainWindowRect.y() + (mainWindowRect.height() / heightDivisor);

    move(x, y);
}

bool CommandPalette::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == searchLineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

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
    Command* cmd = Application::Instance->commandManager().getCommandByName(commandName.constData());
    if (!cmd) {
        // only a command removed while the palette was open can end up here
        Base::Console().warning("Command palette: command '{}' not found\n", commandName.constData());
        close();
        return;
    }

    close();

    try {
        // run it like a click on its button: a toggle command gets its new state and a drop-down
        // runs its current entry, runCommandByName() would always pass 0
        auto action = cmd->getAction();
        if (action && action->action()) {
            action->action()->trigger();
        }
        else {
            Application::Instance->commandManager().runCommandByName(commandName.constData());
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().error(
            "Command palette: error running command '{}': {}\n",
            commandName.constData(),
            e.what()
        );
    }
    catch (...) {
        Base::Console().error(
            "Command palette: unknown error running command '{}'\n",
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

    completer->setSearchText(text);
    completer->setCompletionPrefix(text);

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

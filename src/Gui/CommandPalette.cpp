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
# include <QTextDocument>
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

    // use custom roles
    const int CommandMenuTextRole = Qt::UserRole + 1;
    const int CommandGroupRole = Qt::UserRole + 2;

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

        // strip HTML tags from tooltip
        QTextDocument doc;
        doc.setHtml(tooltip);
        QString plainText = doc.toPlainText();

        // elide if desc is too long
        QFontMetrics fm(tooltipFont);
        QString elidedTooltip = fm.elidedText(plainText, Qt::ElideRight, tooltipRect.width());

        painter->drawText(tooltipRect, Qt::AlignLeft | Qt::AlignVCenter, elidedTooltip);
    }

    painter->restore();
}

QSize CommandItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString tooltip = index.data(Qt::ToolTipRole).toString();

    // if there's a tooltip, make the item taller
    const double height = option.fontMetrics.height() * (tooltip.isEmpty() ? 1.5 : 2.8);

    return {option.rect.width(), static_cast<int>(height)};
}

////////////////////// CommandPalette implementation //////////////////////

CommandPalette::CommandPalette(QWidget* parent)
    : QDialog(parent)
{
    setupUi();

    setModal(false);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);

    // WA_DeleteOnClose because for the love of god i couldn't make it
    // clear the state of completer upon closing and it remembered full
    // previous state
    setAttribute(Qt::WA_DeleteOnClose, true);

    searchLineEdit->installEventFilter(this);
}

void CommandPalette::setupUi()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(5);
    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText(tr("Type a command name..."));
    searchLineEdit->setClearButtonEnabled(true);
    searchLineEdit->setMinimumWidth(500);
    searchLineEdit->setMinimumHeight(30);

    completer = new CommandCompleter(searchLineEdit, this);

    // remove widget association to prevent completer's popup as we just embed the logic inside
    // our own qlistwidget
    completer->setWidget(nullptr);

    // disconnect completer's signals as we don't use them, we need the base model only
    disconnect(searchLineEdit, nullptr, completer, nullptr);

    // Create a list view to show commands directly in the dialog
    commandListView = new QListView(this);

    // use completion model from CommandCompleter
    commandListView->setModel(completer->completionModel());
    commandListView->setMinimumHeight(400);
    commandListView->setMaximumHeight(600);
    commandListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    commandListView->setSelectionMode(QAbstractItemView::SingleSelection);

    // set custom delegate to show command name and description
    commandListView->setItemDelegate(new CommandItemDelegate(this));

    mainLayout->addWidget(searchLineEdit);
    mainLayout->addWidget(commandListView);

    connect(searchLineEdit, &QLineEdit::textChanged, this, &CommandPalette::onTextChanged);
    connect(commandListView, &QListView::activated, this, &CommandPalette::onListItemActivated);
    connect(completer, &CommandCompleter::commandActivated, this, &CommandPalette::onCommandActivated);

    setMinimumWidth(520);
    setMaximumWidth(800);
    setMinimumHeight(450);

    setStyleSheet(QStringLiteral(
        "QDialog {"
        "    background-color: palette(window);"
        "    border: 1px solid palette(dark);"
        "    border-radius: 5px;"
        "}"
        "QLineEdit {"
        "    padding: 5px;"
        "    font-size: 14px;"
        "    border: 1px solid palette(mid);"
        "    border-radius: 3px;"
        "}"
        "QLineEdit:focus {"
        "    border: 2px solid palette(highlight);"
        "}"
        "QListView {"
        "    border: 1px solid palette(mid);"
        "    border-radius: 3px;"
        "}"
    ));
}

void CommandPalette::showPalette()
{
    searchLineEdit->clear();
    completer->setFilterInactive(true);

    // set empty prefix to show all cmds by default
    completer->setCompletionPrefix(QString());

    centerOnMainWindow();

    show();
    raise();
    activateWindow();

    searchLineEdit->setFocus();

    if (commandListView->model()->rowCount() > 0) {
        commandListView->setCurrentIndex(commandListView->model()->index(0, 0));
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
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            QWidget* widget = qobject_cast<QWidget*>(obj);
            if (widget) {
                if (widget == this || widget->isAncestorOf(this) || isAncestorOf(widget)) {
                    return QDialog::eventFilter(obj, event);
                }
            }

            // check if click is outside the dialog and close if yes
            QPoint globalPos = mouseEvent->globalPosition().toPoint();
            if (isClickOutside(globalPos)) {
                close();
                return false;  // propagate further to target widget
            }
        }
    }

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

void CommandPalette::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);
}

void CommandPalette::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    // install application-wide event filter to catch clicks outside
    qApp->installEventFilter(this);
}

void CommandPalette::hideEvent(QHideEvent* event)
{
    QDialog::hideEvent(event);

    // remove application-wide event filter when hidden
    qApp->removeEventFilter(this);
}

bool CommandPalette::isClickOutside(const QPoint& globalPos) const
{
    QRect dialogRect = QRect(mapToGlobal(QPoint(0, 0)), size());
    return !dialogRect.contains(globalPos);
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

    // get the command name from the model
    const int CommandNameRole = Qt::UserRole;
    QByteArray commandName = commandListView->model()->data(index, CommandNameRole).toByteArray();

    if (!commandName.isEmpty()) {
        onCommandActivated(commandName);
    }
}

#include "moc_CommandPalette.cpp"

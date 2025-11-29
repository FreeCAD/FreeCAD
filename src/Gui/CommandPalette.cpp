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
#include <QApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#endif

#include "CommandPalette.h"
#include "CommandCompleter.h"
#include "Application.h"
#include "Command.h"
#include "Action.h"
#include "MainWindow.h"

using namespace Gui;

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
                return false; // propagate further to target widget
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

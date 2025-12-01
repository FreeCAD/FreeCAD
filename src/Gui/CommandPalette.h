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

#ifndef GUI_COMMAND_PALETTE_H
#define GUI_COMMAND_PALETTE_H

#include <FCGlobal.h>
#include <QDialog>
#include <QListView>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

namespace Gui
{

class CommandCompleter;

/**
 * CommandItemDelegate - Custom delegate to display command name and tooltip
 */
class CommandItemDelegate: public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit CommandItemDelegate(QObject* parent = nullptr);

    void paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

/**
 * CommandPalette - A quick command search and execution dialog
 *
 * This dialog provides a VS Code style command palette that allows users to:
 * - Quickly search for commands by typing
 * - Navigate through results using keyboard arrows or mouse
 * - Execute commands by pressing Enter or double-clicking
 * - Close the palette with Escape
 *
 * The palette is triggered by a keyboard shortcut (default: Ctrl+Shift+P)
 */
class GuiExport CommandPalette: public QDialog
{
    Q_OBJECT

public:
    explicit CommandPalette(QWidget* parent = nullptr);
    ~CommandPalette() override = default;
    void showPalette();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private Q_SLOTS:
    void onCommandActivated(const QByteArray& commandName);
    void onTextChanged(const QString& text);
    void onListItemActivated(const QModelIndex& index);

private:
    void setupUi();
    void centerOnMainWindow();
    bool isClickOutside(const QPoint& globalPos) const;

    QLineEdit* searchLineEdit;
    QListView* commandListView;
    CommandCompleter* completer;
    QVBoxLayout* mainLayout;
};

}  // namespace Gui

#endif  // GUI_COMMAND_PALETTE_H

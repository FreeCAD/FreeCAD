// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
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

#pragma once

#include <QWidget>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>

namespace Gui
{

class SplitButton: public QWidget
{
    Q_OBJECT
public:
    explicit SplitButton(QWidget* parent = nullptr);
    explicit SplitButton(const QString& text, QWidget* parent = nullptr);

    QPushButton* mainButton() const
    {
        return m_main;
    }

    QToolButton* menuButton() const
    {
        return m_menuButton;
    }

    QMenu* menu() const
    {
        return m_menu;
    }

Q_SIGNALS:
    void defaultClicked();
    void triggered(QAction*);

private:
    QPushButton* m_main;
    QToolButton* m_menuButton;
    QMenu* m_menu;
};

}  // namespace Gui

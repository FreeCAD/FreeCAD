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

#include "SplitButton.h"

#include <QHBoxLayout>

using namespace Gui;

SplitButton::SplitButton(QWidget* parent)
    : SplitButton(QStringLiteral(""), parent)
{}

SplitButton::SplitButton(const QString& text, QWidget* parent)
    : QWidget(parent)
    , m_main(new QPushButton(text, this))
    , m_menuButton(new QToolButton(this))
    , m_menu(new QMenu(this))
{
    auto* layout = new QHBoxLayout(this);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_main);
    layout->addWidget(m_menuButton);

    // Behavior
    m_main->setAutoDefault(false);
    m_main->setDefault(false);

    m_menuButton->setMenu(m_menu);
    m_menuButton->setPopupMode(QToolButton::InstantPopup);
    m_menuButton->setArrowType(Qt::DownArrow);
    m_menuButton->setToolButtonStyle(Qt::ToolButtonIconOnly);

    connect(m_main, &QPushButton::clicked, this, &SplitButton::defaultClicked);
    connect(m_menu, &QMenu::triggered, this, &SplitButton::triggered);

    // Styling to make it look like a single split button
    m_main->setProperty("splitRole", QLatin1String("main"));
    m_menuButton->setProperty("splitRole", QLatin1String("menu"));
}

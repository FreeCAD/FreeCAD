/* SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
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
# include <QMainWindow>
# include <QMenu>
# include <QHBoxLayout>
# include <QPushButton>
# include <QSpacerItem>
# include <QMouseEvent>
# include <QEvent>
# include <QPoint>
# include <QTimer>
#endif

#include "BitmapFactory.h"
#include "CustomTitleBar.h"

using namespace Gui;
using namespace std;

CustomTitleBar::CustomTitleBar(QMainWindow* mainWindow)
    : QWidget(mainWindow)
    , m_mainWindow(mainWindow)
{

    m_mainMenu = new QMenu(this);
    int barHeight = 35;

    // Set up layout
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Menu button
    m_menuButton = new QPushButton(this);
    m_menuButton->setIcon(Gui::BitmapFactory().iconFromTheme("freecad"));
    m_menuButton->setToolTip(tr("Show Main Menu"));
    m_menuButton->setFlat(true);
    layout->addWidget(m_menuButton);

    m_menuButton->setMenu(m_mainMenu);
    connect(m_menuButton, &QPushButton::clicked, this, &CustomTitleBar::showUnifiedMenu);

    // Spacer to push controls to the right
    layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    // Minimize button
    m_minimizeButton = new QPushButton(this);
    m_minimizeButton->setIcon(Gui::BitmapFactory().iconFromTheme("window-minimize"));
    m_minimizeButton->setToolTip(tr("Minimize"));
    m_minimizeButton->setObjectName(QString::fromLatin1("MinimizeButton"));
    m_minimizeButton->setFlat(true);
    layout->addWidget(m_minimizeButton);
    connect(m_minimizeButton, &QPushButton::clicked, m_mainWindow, &QMainWindow::showMinimized);

    // Maximize/Restore button
    m_maximizeButton = new QPushButton(this);
    updateMaximizeRestoreIcon();
    m_maximizeButton->setToolTip(tr("Maximize/Restore"));
    m_maximizeButton->setObjectName(QString::fromLatin1("MaximizeButton"));
    m_maximizeButton->setFlat(true);
    layout->addWidget(m_maximizeButton);
    connect(m_maximizeButton, &QPushButton::clicked, this, &CustomTitleBar::toggleMaximizeRestore);

    // Connect to the window state change signal
    m_mainWindow->installEventFilter(this);

    // Close button
    m_closeButton = new QPushButton(this);
    m_closeButton->setIcon(Gui::BitmapFactory().iconFromTheme("window-close"));
    m_closeButton->setToolTip(tr("Close"));
    m_closeButton->setFlat(true);
    m_closeButton->setObjectName(QString::fromLatin1("CloseButton"));
    layout->addWidget(m_closeButton);
    connect(m_closeButton, &QPushButton::clicked, m_mainWindow, &QMainWindow::close);

    setHeight(barHeight);

    setLayout(layout);

    setAttribute(Qt::WA_StyledBackground, true);

    show();
}

void CustomTitleBar::setHeight(int height)
{
    // Set fixed button size
    QSize buttonSize(height, height);  // Compact button size
    for (auto* button : {m_menuButton, m_minimizeButton, m_maximizeButton, m_closeButton}) {
        button->setFixedSize(buttonSize);
    }

    setFixedHeight(height);  // Adjust height as needed
}

bool CustomTitleBar::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_mainWindow && event->type() == QEvent::WindowStateChange) {
        updateMaximizeRestoreIcon();  // Update the button icon when the state changes
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void CustomTitleBar::toggleMaximizeRestore()
{
    if (m_mainWindow->isMaximized()) {
        m_mainWindow->showNormal();
    }
    else {
        m_mainWindow->showMaximized();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
# ifdef FC_OS_WIN32
        QTimer::singleShot(10, this, [this]() {
            // Bug with Qt6 the window is kind of restored when showMaximized only once
            m_mainWindow->showMaximized();
        });
# endif
#endif
    }
}

void CustomTitleBar::updateMaximizeRestoreIcon()
{
    bool isDarkTheme = (palette().color(QPalette::Window).value() < 128);  // Detect dark theme

    if (m_mainWindow->isMaximized()) {
        m_maximizeButton->setIcon(
            Gui::BitmapFactory().iconFromTheme(isDarkTheme ? "window-restore-dark" : "window-restore")
        );
    }
    else {
        m_maximizeButton->setIcon(
            Gui::BitmapFactory().iconFromTheme(isDarkTheme ? "window-maximize-dark" : "window-maximize")
        );
    }
}

QMenu* CustomTitleBar::mainMenu()
{
    return m_mainMenu;
}

void CustomTitleBar::showUnifiedMenu()
{
    m_menuButton->showMenu();
}


#ifndef FC_OS_WIN32
void CustomTitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPosition = event->globalPos();
    }
    QWidget::mousePressEvent(event);
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        if (m_mainWindow->isMaximized()) {
            m_mainWindow->showNormal();
        }
        QPoint delta = event->globalPos() - m_dragStartPosition;
        m_mainWindow->move(m_mainWindow->pos() + delta);
        m_dragStartPosition = event->globalPos();
    }
    QWidget::mouseMoveEvent(event);
}

void CustomTitleBar::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}

void CustomTitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        toggleMaximizeRestore();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}
#endif

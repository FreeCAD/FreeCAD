/* SPDX - License - Identifier: LGPL - 2.1 - or -later
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


#ifndef GUI_CUSTOMTITLEBAR_H
#define GUI_CUSTOMTITLEBAR_H

#include <QWidget>

#include "FCGlobal.h"

class QMenu;
class QPoint;
class QPushButton;
class QMainWindow;

namespace Gui
{
class GuiExport CustomTitleBar: public QWidget
{
    Q_OBJECT

public:
    explicit CustomTitleBar(QMainWindow* mainWindow);
    QMenu* mainMenu();

    bool eventFilter(QObject* o, QEvent* e) override;
    void setHeight(int height);


#ifndef FC_OS_WIN32
    // For OS that are not windows, we implement basic title bar functionalities:
    // - Click and drag to move window
    // - Double click to maximize/restore

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QPoint m_dragStartPosition;
    bool m_dragging = false;
#endif

private Q_SLOTS:
    void toggleMaximizeRestore();
    void updateMaximizeRestoreIcon();
    void showUnifiedMenu();

private:
    QMenu* m_mainMenu;
    QMainWindow* m_mainWindow;
    QPushButton* m_minimizeButton;
    QPushButton* m_maximizeButton;
    QPushButton* m_closeButton;
    QPushButton* m_menuButton;
};

}  // namespace Gui

#endif  // GUI_CUSTOMTITLEBAR_H

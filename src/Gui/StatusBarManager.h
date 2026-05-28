// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Max Wilfinger
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

#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

#include <FCGlobal.h>

#include <Base/Parameter.h>

class QMenu;
class QPoint;
class QStatusBar;
class QWidget;

namespace Gui
{

/** Logical placement zone for a status bar item. */
enum class StatusBarSlot
{
    Left,   //< left-aligned area (maps to QStatusBar non-permanent widgets)
    Right,  //< right-aligned area (maps to QStatusBar permanent widgets)
};

/** Metadata used to register a widget with the status bar. */
struct StatusBarItemSpec
{
    QByteArray id;  //< stable identifier (persistence + removal key)
    QString title;  //< toggle-menu label; empty => not toggleable
    StatusBarSlot slot = StatusBarSlot::Right;
    int order = 0;                     //< ordering within the slot (lower = closer to the left)
    bool persistentVisibility = true;  //< restore/save visibility across sessions
    bool ownVisibility = false;        //< widget controls its own visibility (e.g. progress bar)
    int stretch = 0;
};

/**
 * @brief Owns the FreeCAD status bar layout, ordering, visibility persistence
 * and the widget-toggle context menu.
 *
 * Callers (core C++ widgets and Python workbenches) register a widget together
 * with a StatusBarItemSpec instead of manipulating QStatusBar directly.  The
 * manager decides where the widget is placed (slot + order), restores its saved
 * visibility, and adds it to the right-click toggle menu.  This keeps all status
 * bar layout knowledge in one place.
 */
class GuiExport StatusBarManager: public QObject
{
    Q_OBJECT

public:
    static StatusBarManager* instance();
    static void destruct();

    /** Register \a widget with the given \a spec and place it on the status bar. */
    void addItem(QWidget* widget, const StatusBarItemSpec& spec);
    /** Remove the registered item identified by \a id (the widget is not deleted). */
    void removeItem(const QByteArray& id);

    /** Append a checkable toggle action for every registered titled item to \a menu. */
    void populateToggleMenu(QMenu& menu);

private:
    StatusBarManager();
    ~StatusBarManager() override = default;

    struct Entry
    {
        StatusBarItemSpec spec;
        QPointer<QWidget> widget;
        int sequence = 0;  //< registration order, used as a stable tie-breaker
    };

    QStatusBar* statusBar() const;
    static bool sortsBefore(const Entry& a, const Entry& b);
    void place(QWidget* widget, const StatusBarItemSpec& spec);

    QList<Entry> items;
    int sequenceCounter = 0;
    ParameterGrp::handle hGrp;

    static StatusBarManager* _instance;
};

}  // namespace Gui

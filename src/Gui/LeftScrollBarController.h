// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <functional>

#include <QObject>

#include <FCGlobal.h>

class QAbstractScrollArea;
class QDockWidget;
class QScrollBar;

namespace Gui
{

/**
 * Shows the vertical scroll bars of all scroll areas inside a dock window on their left edge.
 *
 * For every scroll area the native vertical bar is switched off and a synchronized bar is placed
 * in front of it. Which side is wanted is decided by the predicate given at construction, so the
 * bar moves back to the native position when the dock is moved, floated or the option is turned
 * off. Scroll areas whose native bar is hidden by a style sheet (see the overlay "Hide scroll bar"
 * options) and scroll areas that cannot be re-laid out are left alone.
 *
 * The controller is owned by the dock widget and updates itself when the dock is shown or moved.
 * Call update() after anything that changes the predicate's result.
 */
class GuiExport LeftScrollBarController: public QObject
{
    Q_OBJECT

public:
    using Predicate = std::function<bool()>;

    LeftScrollBarController(QDockWidget* dock, Predicate wantLeft);

    void update();

    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    void schedule();
    QScrollBar* wrap(QAbstractScrollArea* area);

    QDockWidget* dock;
    Predicate wantLeft;
};

}  // namespace Gui

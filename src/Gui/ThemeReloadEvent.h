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

#include <FCGlobal.h>
#include <QEvent>

namespace Gui
{

/**
 * @brief Event sent to qApp to trigger a full theme reload.
 *
 * Since qApp->style() may return QStyleSheetStyle (which proxies FreeCADStyle) rather
 * than FreeCADStyle itself, a direct qobject_cast is unreliable. Instead, post or send
 * a ThemeReloadEvent to qApp; registered event filters intercept it and perform their
 * respective reload actions:
 *
 * - ThemeReloadHandler (Application.cpp): reloads style parameters and re-applies the
 *   stylesheet.
 * - FreeCADStyle (event filter on qApp): clears the token cache and schedules a repaint
 *   of all widgets.
 *
 * Usage:
 * @code
 * ThemeReloadEvent event;
 * QApplication::sendEvent(qApp, &event);
 * @endcode
 */
class GuiExport ThemeReloadEvent: public QEvent
{
public:
    ThemeReloadEvent()
        : QEvent(registeredType())
    {}

    /** @brief Returns the unique QEvent::Type registered for ThemeReloadEvent. */
    static QEvent::Type registeredType();
};

}  // namespace Gui

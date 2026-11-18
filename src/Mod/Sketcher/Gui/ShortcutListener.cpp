// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2018 Abdullah Tahiri
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

#include <QKeyEvent>
#include <QKeySequence>


#include "ShortcutListener.h"
#include "ViewProviderSketch.h"


using namespace SketcherGui;

// ******************** ShortcutListener *********************************************//
ShortcutListener::ShortcutListener(ViewProviderSketch* vp)
    : pViewProvider {vp}
{}

ShortcutListener::~ShortcutListener() = default;

bool ShortcutListener::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent* kevent = static_cast<QKeyEvent*>(event);  // NOLINT
        if (kevent->matches(QKeySequence::Delete)) {
            kevent->accept();
            pViewProvider->deleteSelected();
            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}

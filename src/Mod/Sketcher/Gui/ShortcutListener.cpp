// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2018 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

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

/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
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

#ifndef GUI_TOOLS_H
#define GUI_TOOLS_H

#include <QFontMetrics>
#include <QKeyEvent>
#include <QKeySequence>
#include <FCGlobal.h>

namespace Gui {

/*!
 * \brief The QtTools class
 * Helper class to reduce adding a lot of extra QT_VERSION checks to client code.
 */
class GuiExport QtTools {
public:
    static int horizontalAdvance(const QFontMetrics& fm, QChar ch) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        return fm.horizontalAdvance(ch);
#else
        return fm.width(ch);
#endif
    }
    static int horizontalAdvance(const QFontMetrics& fm, const QString& text, int len = -1) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        return fm.horizontalAdvance(text, len);
#else
        return fm.width(text, len);
#endif
    }
    static bool matches(QKeyEvent* ke, const QKeySequence& ks) {
        uint searchkey = (ke->modifiers() | ke->key()) & ~(Qt::KeypadModifier | Qt::GroupSwitchModifier);
        return ks == QKeySequence(searchkey);
    }
};

} // namespace Gui

#endif // GUI_TOOLS_H

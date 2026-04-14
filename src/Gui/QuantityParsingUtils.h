// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
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

#include <QLocale>
#include <QString>

namespace Gui::detail
{
inline bool isAsciiExponentMarker(const QChar ch)
{
    return ch == QLatin1Char('e') || ch == QLatin1Char('E');
}

inline bool looksLikeLocalizedGroupedNumber(const QString& text, const QLocale& locale)
{
    const QString groupSeparatorText = locale.groupSeparator();
    if (groupSeparatorText.isEmpty()) {
        return false;
    }

    const QString decimalPointText = locale.decimalPoint();
    const QChar decimalPoint = decimalPointText.isEmpty() ? QChar() : decimalPointText.front();
    const QChar groupSeparator = groupSeparatorText.front();

    int pos = text.indexOf(groupSeparator);
    while (pos != -1) {
        const int groupEnd = pos + groupSeparatorText.size();
        if (pos > 0 && groupEnd < text.size() && text.at(pos - 1).isDigit()) {
            int digitsAfter = 0;
            int cursor = groupEnd;
            while (cursor < text.size() && text.at(cursor).isDigit() && digitsAfter < 4) {
                ++digitsAfter;
                ++cursor;
            }

            if (digitsAfter == 3) {
                const QChar following = cursor < text.size() ? text.at(cursor) : QChar();
                if (following == decimalPoint || following == groupSeparator || following.isNull()
                    || (!following.isDigit() && !isAsciiExponentMarker(following))) {
                    return true;
                }
            }
        }

        pos = text.indexOf(groupSeparator, groupEnd);
    }

    return false;
}

template<typename ParseFn, typename FixupFn>
bool parseWithFixupFallback(const QString& text, const QLocale& locale, ParseFn tryParse, FixupFn fixup)
{
    auto tryNormalized = [&] {
        QString normalized = text;
        fixup(normalized);
        return normalized != text && tryParse(normalized);
    };

    if (looksLikeLocalizedGroupedNumber(text, locale) && tryNormalized()) {
        return true;
    }

    if (tryParse(text)) {
        return true;
    }

    return tryNormalized();
}

}  // namespace Gui::detail

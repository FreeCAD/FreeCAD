/***************************************************************************
 *   Copyright (c) 2026 The FreeCAD Project Association AISBL              *
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

#include "TreeSearchUtil.h"

namespace Gui::TreeSearchUtil
{

QRegularExpression wildcardToRegex(const QString& pattern)
{
    const bool hasWildcard = pattern.contains(QLatin1Char('*')) || pattern.contains(QLatin1Char('?'));
    const QString effectivePattern = hasWildcard ? pattern : QStringLiteral("*%1*").arg(pattern);

    QString rx;
    rx.reserve(effectivePattern.size() * 2 + 2);
    rx += QLatin1Char('^');

    for (const QChar ch : effectivePattern) {
        if (ch == QLatin1Char('*')) {
            rx += QStringLiteral(".*");
        }
        else if (ch == QLatin1Char('?')) {
            rx += QLatin1Char('.');
        }
        else {
            rx += QRegularExpression::escape(QString(ch));
        }
    }

    rx += QLatin1Char('$');
    return QRegularExpression(rx, QRegularExpression::CaseInsensitiveOption);
}

bool regexMatches(const QRegularExpression& re, const QString& haystack)
{
    return re.isValid() && re.match(haystack).hasMatch();
}

}  // namespace Gui::TreeSearchUtil

// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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

#pragma once

#include <QString>
#include <QStringList>
#include <FCGlobal.h>

namespace App
{

class AppExport ExpressionTokenizer
{
public:
    /** Extract the completion prefix from the expression text at cursor position pos,
     * e.g. "1 + Sketch.Constr" yields "Sketch.Constr". Records the character range the
     * accepted completion will replace, retrievable via getPrefixRange(); returns an
     * empty string when there is nothing to complete at the position.
     */
    QString extractCompletionPrefix(const QString& text, int pos);

    /** Split a completion prefix into completer path components, e.g. "Sketch." into
     * ("Sketch", ""). Lexical only, so incomplete prefixes that ObjectIdentifier::parse()
     * rejects still split; returns an empty list if the prefix is not a path.
     */
    QStringList splitCompletionPath(const QString& path) const;

    void getPrefixRange(int& start, int& end) const
    {
        start = prefixStart;
        end = prefixEnd;
    }

    void updatePrefixEnd(int end)
    {
        prefixEnd = end;
    }

private:
    int prefixStart = 0;
    int prefixEnd = 0;
};

}  // namespace App

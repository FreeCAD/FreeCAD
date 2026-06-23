// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD project association AISBL
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

#include <string>

#include <Base/Translation.h>

// Declares a Qt-free `tr()` compatible helper for App classes.
// When no translation handler is installed, this returns the source text.
#define FC_APP_DECLARE_TR_FUNCTIONS(contextLiteral) \
public: \
    static std::string tr(const char* sourceText, const char* disambiguation = nullptr, int n = -1) \
    { \
        return ::Base::Translation::translate( \
            contextLiteral, \
            sourceText ? sourceText : "", \
            disambiguation ? disambiguation : "", \
            n \
        ); \
    }

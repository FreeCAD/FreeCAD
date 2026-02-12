// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "debug.h"

#include <string>

#include <clang/Lex/Lexer.h>


namespace Debug
{

std::string toSourceText(const cl::Stmt* stmt, const cl::ASTContext& ctx)
{
    if (!stmt) {
        return "";
    }
    const auto& sourceMgr = ctx.getSourceManager();
    const auto& langOpts = ctx.getLangOpts();
    cl::SourceRange range = stmt->getSourceRange();
    return cl::Lexer::getSourceText(cl::CharSourceRange::getTokenRange(range), sourceMgr, langOpts).str();
}

}  // namespace Debug

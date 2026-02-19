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

#ifndef DEBUG_H
#define DEBUG_H

#include <string>

#include <clang/AST/ASTContext.h>
#include <clang/AST/Stmt.h>

#define DEBUG_TYPE "property-accessor-rewriter"
#define DEBUG false

#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

namespace Debug
{
namespace cl = clang;

inline llvm::raw_ostream& out()
{
    if (!DEBUG) {
        return llvm::nulls();
    }

    return llvm::dbgs();
}

std::string toSourceText(const cl::Stmt* stmt, const cl::ASTContext& ctx);

}  // namespace Debug


#endif

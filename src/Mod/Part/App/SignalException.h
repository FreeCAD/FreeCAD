// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *                                                                         *
 *   Copyright (c) 2026 The FreeCAD project association AISBL              *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#pragma once

#include <functional>
#include <Mod/Part/PartGlobal.h>

namespace Part
{

/// Executes a callable with POSIX signal handlers that convert crashes
/// (SIGSEGV, SIGBUS, SIGFPE, etc.) into C++ exceptions (Standard_Failure).
/// On Linux/macOS the signal handler does longjmp back into guard(),
/// which then throws, so no macro (OCC_CATCH_SIGNALS) is needed at the call site.
/// On other platforms this is a no-op passthrough.
class PartExport SignalException
{
public:
    static void guard(std::function<void()> fn);
};

}  // namespace Part

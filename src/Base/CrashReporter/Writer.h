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

#include <FCConfig.h>
#include <FCGlobal.h>
#include <string>

#ifdef _MSC_VER
// Declared by <windows.h>, forward-declared here so this header stays self-contained
// without pulling the Windows headers into every consumer.
struct _EXCEPTION_POINTERS;
#endif

namespace Base::CrashReporter
{
class BaseExport Writer
{
public:
    /**
     * Perform cpptrace init that isn't safe to do in the signal handler. If cpptrace is not
     * installed this is a no-op.
     */
    static void prewarm();

    /**
     * Install the signal handlers to capture crashes.
     */
    static void install(const std::string& crashReportDirectory);

    /**
     * Get the generated name of the fcrash file.
     *
     * @return The UTF-8-encoded file path
     */
    static std::string crashReportFilePath();

#ifdef _MSC_VER
    static void handleException(_EXCEPTION_POINTERS* exceptionInfo);

    /**
     * Set the path to a minidump file, if one was created.
     *
     * @param path The path to the minidump file
     */
    static void setMinidumpPath(const std::string& path);
#endif  // _MSC_VER
};
}  // namespace Base::CrashReporter

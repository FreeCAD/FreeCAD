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

#include <FCGlobal.h>
#include <string>

namespace Base::CrashReporter
{
class BaseExport Writer
{
public:
    /**
     * Perform cpptrace init that isn't safe to do in the signal handler. If cpptrace is not install
     * this is a no-op.
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

#ifdef FC_OS_WIN32
    static void handleException(_EXCEPTION_POINTERS* exceptionInfo);

    /**
     * Set the path to a minidump file, if one was created.
     *
     * @param path The path to the minidump file
     */
    static void setMinidumpPath(const std::string& path);
#endif
};
}  // namespace Base::CrashReporter

// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008 Jürgen Riegel <juergen.riegel@web.de>
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

// This entire file is purely Windows-only
#if defined(_MSC_VER)

# include "WindowsMiniDump.h"

# include <FCConfig.h>

# include <windows.h>
# include <DbgHelp.h>

# include <atomic>
# include <ctime>
# include <string>

# include "Writer.h"
# include "Base/Console.h"
# include "Base/FileInfo.h"

namespace
{

MINIDUMP_TYPE s_dumpTyp = MiniDumpNormal;
std::wstring s_dumpPathW;  // Set by install()
std::atomic_flag writing;

LONG __stdcall MyCrashHandlerExceptionFilter(EXCEPTION_POINTERS* pEx)
{
    if (writing.test_and_set()) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    // Write the fcrash file **first**, since those operations should be the safest and least
    // likely to themselves crash or corrupt anything.
    Base::CrashReporter::Writer::handleException(pEx);

    HANDLE hFile = CreateFileW(
        s_dumpPathW.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION stMDEI {};
        stMDEI.ThreadId = GetCurrentThreadId();
        stMDEI.ExceptionPointers = pEx;
        stMDEI.ClientPointers = false;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, s_dumpTyp, &stMDEI, NULL, NULL);
        CloseHandle(hFile);
    }
    return EXCEPTION_CONTINUE_SEARCH;  // this will trigger the "normal" OS error-dialog
}
}  // Anonymous namespace

namespace Base::CrashReporter
{
void WindowsCrashReporter::install(const std::string& crashReportDirectory)
{
    FileInfo fcrash(Writer::crashReportFilePath());
    std::string minidumpFilename = FileInfo::pathToString(
        FileInfo::stringToPath(fcrash.dirPath()) / (fcrash.fileNamePure() + ".dmp")
    );
    if (minidumpFilename.length() > MAX_PATH - 1) {
        Console().warning("CrashReporter: Crash file path too long: %s\n", minidumpFilename);
        return;
    }
    Writer::setMinidumpPath(minidumpFilename);
    s_dumpPathW = FileInfo(minidumpFilename).toStdWString();

    // Make sure to do this **last**, in case some of the previous work actually triggers an
    // exception -- we don't want a partially-initialized filter to get installed.
    SetUnhandledExceptionFilter(MyCrashHandlerExceptionFilter);
}
}  // namespace Base::CrashReporter
#endif

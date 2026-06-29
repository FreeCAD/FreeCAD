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
#include "Format.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
#include <optional>

#include <Base/Exception.h>

namespace Base::CrashReporter
{

struct ParsedFrame
{
    // Raw Data
    std::uint64_t rawAddress = 0;
    std::uint64_t moduleOffset = 0;
    std::string modulePath;  // NOTE: Avoid PII here! Strip to only filename.

    // Symbolicated frame
    std::string symbol;
    std::string file;
    std::optional<std::uint32_t> line;
    bool isInline {false};
};

struct ParsedCrashReport
{
    std::string pathToRawReportFile;

    std::uint64_t faultAddress = 0;
    std::uint64_t threadID = 0;
    std::chrono::system_clock::time_point timestamp;

    std::uint32_t processID = 0;
    std::uint32_t code = 0;  // Signal number or SEH ExceptionCode

    bool partialWrite {false};
    bool captureWasSignalSafe {false};

    std::string buildID;
    std::string minidumpPath;
    std::string exceptionMessage;

    OS osID = OS::None;
    std::string osVersion;
    Architecture architectureID = Architecture::None;

    std::uint8_t freecadVersionMajor = 0;
    std::uint8_t freecadVersionMinor = 0;
    std::uint8_t freecadVersionPatch = 0;
    std::string freecadVersionSuffix;

    bool symbolicated = false;
    std::vector<ParsedFrame> stackFrames;
};

/**
 * Parse and symbolicate a FreeCAD Crash Report file
 *
 * This should be run with the exact version of FreeCAD that created the file in the first place.
 * If it's run on a different version (maybe because of a software upgrade), the stack is *not*
 * symbolicated, even if cpptrace is available.
 *
 * It is expected to be run on the next successful startup of FreeCAD after the crash occurred.
 * Throws a `Base::BadFormatError` if the file format is broken, or some other derivative of
 * `Base::Exception` for various file-reading errors, etc. A *truncated* file is treated as a
 * non-fatal failure. The parser will do its best to read the file, and will set the `partialWrite`
 * flag.
 *
 * \returns a ParsedCrashReport with symbol information (depending on the availability of debug
 * symbols that symbolication may be more or less complete).
 */
[[nodiscard]] ParsedCrashReport BaseExport parse(const std::string& pathToRawReportFile);

/**
 * Clean up a set of stack frames from a crash report by eliminating the top few frames that
 * represent the crash handling code itself.
 *
 * @param frames The original list of stack frames from a parsed crash report
 * @return An updated list, stripped of the report-generation plumbing
 */
[[nodiscard]] std::vector<ParsedFrame> BaseExport trimLeadingPlumbingFrames(
    const std::vector<ParsedFrame>& frames
);

}  // namespace Base::CrashReporter

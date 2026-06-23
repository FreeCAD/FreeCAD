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

#include "Reader.h"
#include "Format.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

#include "Base/Stream.h"

#ifdef FC_HAVE_CPPTRACE
#include <cpptrace/cpptrace.hpp>
#include <unordered_map>  // Only needed for the cpptrace branch for moduleName lookup table
#endif


using namespace Base::CrashReporter;

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic, hicpp-exception-baseclass)


namespace
{
std::string_view extractStringFromTable(std::span<const char> stringTable, std::size_t offset)
{
    if (offset == NoString) {
        return {};
    }
    if (offset + sizeof(std::uint16_t) > stringTable.size()) {
        throw ::Base::BadFormatError("String buffer ran out of data");
    }

    std::uint16_t length {0};
    std::memcpy(&length, stringTable.data() + offset, sizeof(std::uint16_t));
    if (length > MaxStringLength) {
        throw ::Base::BadFormatError("String length exceeds the maximum string length");
    }
    if (offset + sizeof(std::uint16_t) + length > stringTable.size()) {
        throw ::Base::BadFormatError("String length exceeds storage");
    }

    return {stringTable.data() + offset + sizeof(std::uint16_t), length};
}
}

ParsedCrashReport Base::CrashReporter::parse(const std::string &pathToRawReportFile)
{
    Base::FileInfo fileInfo (pathToRawReportFile);
    if (!fileInfo.exists()) {
        throw Base::FileException(std::string("Cannot read file ") + pathToRawReportFile, fileInfo);
    }

    auto onDiskFileSize = fileInfo.size();
    if (onDiskFileSize < sizeof(Header) + sizeof(Footer) || onDiskFileSize > MaxFileSize) {
        throw Base::BadFormatError("Corrupted crash report file " + pathToRawReportFile);
    }
    std::vector<char> buffer (onDiskFileSize);
    Base::ifstream ifs (fileInfo, std::ios::binary);
    ifs.read(buffer.data(), onDiskFileSize);

    if (!ifs || std::cmp_not_equal(ifs.gcount(), onDiskFileSize)) {
        throw Base::BadFormatError("File read truncated for " + pathToRawReportFile);
    }

    Header header;
    std::memcpy(&header, buffer.data(), sizeof(Header));
    if (header.magic != MagicNumber) {
        throw Base::BadFormatError("Unexpected magic number in crash report file " + pathToRawReportFile);
    }
    if (header.version != 1) {
        throw Base::BadFormatError("Using the fccrash reader for v1, but found v" + std::to_string(header.version));
    }
    if (header.fileSize != onDiskFileSize) {
        throw Base::BadFormatError("Filesize mismatch in " + pathToRawReportFile);
    }

    // Header data matches expectations, start parsing:
    ParsedCrashReport parsedReport;

    Footer footer;
    std::memcpy(&footer, buffer.data() + header.fileSize - sizeof(Footer), sizeof(Footer));
    auto calculatedChecksum = crc32(std::span<const char>(buffer.data(), header.fileSize - sizeof(Footer)));
    parsedReport.partialWrite = (calculatedChecksum != footer.checksum) || hasFlag(header.flags, Flags::PartialWrite);

    parsedReport.pathToRawReportFile = pathToRawReportFile;
    parsedReport.faultAddress = header.faultAddress;
    parsedReport.threadID = header.threadID;
    parsedReport.timestamp = std::chrono::system_clock::time_point{
        std::chrono::seconds{header.timestamp}
    };
    parsedReport.processID = header.processID;
    parsedReport.code = header.code;

    parsedReport.captureWasSignalSafe = hasFlag (header.flags, Flags::CaptureWasSignalSafe);

    if (header.stringTableOffset + sizeof(Footer) > header.fileSize) {
        throw Base::BadFormatError("String table offset exceeds file size");
    }
    std::size_t stringTableSize = header.fileSize - header.stringTableOffset - sizeof(Footer);
    auto stringTable = std::span<const char>(buffer.data() + header.stringTableOffset, stringTableSize);

    parsedReport.buildID = extractStringFromTable(stringTable, header.buildIDStringOffset);
    parsedReport.minidumpPath = extractStringFromTable(stringTable, header.minidumpPathStringOffset);
    parsedReport.exceptionMessage = extractStringFromTable(stringTable, header.exceptionMessageStringOffset);

    //parsedReport.osVersion = Set by App-level consumer at report-submission, Base has no easy access to OS information
    parsedReport.osID = header.osID;
    parsedReport.architectureID = header.architectureID;

    parsedReport.freecadVersionMajor = header.freecadVersionMajor;
    parsedReport.freecadVersionMinor = header.freecadVersionMinor;
    parsedReport.freecadVersionPatch = header.freecadVersionPatch;
    parsedReport.freecadVersionSuffix = extractStringFromTable(stringTable, header.freecadVersionSuffixStringOffset);

    // Read the stack frames (with some error checking):
    if (header.frameCount > MaxFrames) {
        throw Base::BadFormatError("Frame count exceeds the maximum number of frames");
    }
    if (header.frameTableOffset + header.frameCount*sizeof(Frame) > header.stringTableOffset) {
        throw Base::BadFormatError("Frame count doesn't fit in available storage");
    }

#ifdef FC_HAVE_CPPTRACE
    // Symbolicate:
    cpptrace::object_trace objectTrace;
    std::unordered_map<cpptrace::frame_ptr, std::string> modulePathMap;
    for (std::uint32_t i = 0; i < header.frameCount; i++) {
        Frame rawFrame;
        std::memcpy(&rawFrame, buffer.data() + header.frameTableOffset + i * sizeof(Frame), sizeof(Frame));
        cpptrace::object_frame objectFrame;
        objectFrame.raw_address = rawFrame.rawAddress;
        objectFrame.object_address = rawFrame.moduleOffset;
        objectFrame.object_path = extractStringFromTable(stringTable, rawFrame.moduleStringOffset);
        modulePathMap[objectFrame.raw_address] = objectFrame.object_path;  // For later lookup
        objectTrace.frames.push_back(std::move(objectFrame));
    }

    auto stackTrace = objectTrace.resolve();  // This call does the actual resolution
    parsedReport.stackFrames.reserve(stackTrace.frames.size());
    for (const auto &frame : stackTrace.frames) {
        ParsedFrame parsedFrame;
        parsedFrame.rawAddress = frame.raw_address;
        parsedFrame.moduleOffset = frame.object_address;

        // To avoid any PII in the backtrace, only include the filename, not the full path:
        parsedFrame.modulePath = Base::FileInfo(modulePathMap[frame.raw_address]).fileName();

        parsedFrame.symbol = frame.symbol;
        parsedFrame.file = frame.filename;
        parsedFrame.line = frame.line.has_value() ? std::optional(frame.line.value()) : std::nullopt;
        parsedFrame.isInline = frame.is_inline;
        parsedReport.stackFrames.push_back(std::move(parsedFrame));
    }
#else

    parsedReport.stackFrames.reserve(header.frameCount);
    for (std::uint32_t i = 0; i < header.frameCount; i++) {
        Frame rawFrame;
        std::memcpy(&rawFrame, buffer.data() + header.frameTableOffset + i * sizeof(Frame), sizeof(Frame));

        ParsedFrame parsedFrame;
        parsedFrame.rawAddress = rawFrame.rawAddress;
        parsedFrame.moduleOffset = rawFrame.moduleOffset;
        std::string modulePath {extractStringFromTable(stringTable, rawFrame.moduleStringOffset)};
        parsedFrame.modulePath = Base::FileInfo(modulePath).fileName(); // Avoid PII!

        parsedReport.stackFrames.push_back(std::move(parsedFrame));
    }
#endif

    return parsedReport;
}

std::vector<ParsedFrame> Base::CrashReporter::trimLeadingPlumbingFrames(const std::vector<ParsedFrame>& frames)
{
    // To detect the first real frame of the crash stack, we detect the OS-specific "trampoline"
    // function:
    static constexpr std::array dispatchAnchors {
        std::string_view {"KiUserExceptionDispatcher"},  // Windows
        std::string_view {"__restore_rt"},               // glibc rt_sigreturn
        std::string_view {"__kernel_rt_sigreturn"},      // vDSO (ARM and others)
        std::string_view {"__sigtramp"},                 // macOS / BSD
    };

    auto isAnchor = [&](const ParsedFrame& frame) {
        return std::ranges::any_of(dispatchAnchors, [&](std::string_view anchor) {
            return frame.symbol.find(anchor) != std::string::npos;
        });
    };

    auto anchor = std::ranges::find_if(frames, isAnchor);
    if (anchor == frames.end()) {
        return frames;  // no recognizable trampoline, do nothing
    }
    auto firstReal = std::next(anchor);
    if (firstReal == frames.end()) {
        return frames;  // trampoline was the deepest frame?! Seems sketchy, return everything
    }
    return {firstReal, frames.end()};
}

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic, hicpp-exception-baseclass)

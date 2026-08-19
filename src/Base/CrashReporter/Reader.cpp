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
#include "FaultCodes.h"

#include <Build/Version.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>  // IWYU pragma: keep
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "Base/Stream.h"

#ifdef FC_HAVE_CPPTRACE
# include <cpptrace/cpptrace.hpp>
# include <filesystem>     // Only needed for the cpptrace branch to strip the source root
# include <unordered_map>  // Only needed for the cpptrace branch for moduleName lookup table
#endif


using namespace Base::CrashReporter;

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic, hicpp-exception-baseclass)


namespace
{
std::optional<std::string> extractStringFromTable(std::span<const char> stringTable, std::size_t offset)
{
    if (offset == NoString) {
        return std::nullopt;
    }
    if (offset + sizeof(std::uint16_t) > stringTable.size()) {
        throw Base::BadFormatError("String buffer ran out of data");
    }

    std::uint16_t length {0};
    std::memcpy(&length, stringTable.data() + offset, sizeof(std::uint16_t));
    if (length > MaxStringLength) {
        throw Base::BadFormatError("String length exceeds the maximum string length");
    }
    if (offset + sizeof(std::uint16_t) + length > stringTable.size()) {
        throw Base::BadFormatError("String length exceeds storage");
    }

    return std::string {stringTable.data() + offset + sizeof(std::uint16_t), length};
}

#ifdef FC_HAVE_CPPTRACE
/**
 * Reduce a source path recorded in the debug info to something safe to put in a crash report.
 *
 * Symbolication only runs when the running build matches the build that crashed, which in
 * practice means a build compiled from source on this machine, so the DWARF compilation
 * directory is the developer's home directory. Strip the source root to leave a path relative
 * to it (for example "src/Base/CrashReporter/Writer.cpp"), which is what triage actually wants,
 * and fall back to the bare filename for anything outside the source tree such as third-party
 * or system libraries.
 */
std::string stripSourceRoot(const std::string& path)
{
    if (path.empty()) {
        return {};
    }

    // cpptrace hands back the path exactly as the compiler recorded it, which routinely
    // contains ".." segments, so normalize before comparing against the source root.
    const std::string normalized = std::filesystem::path(path).lexically_normal().generic_string();

    static const std::string sourceRoot
        = std::filesystem::path(FC_SOURCE_DIR).lexically_normal().generic_string();

    if (!sourceRoot.empty() && normalized.size() > sourceRoot.size()
        && normalized.starts_with(sourceRoot) && normalized.at(sourceRoot.size()) == '/') {
        return normalized.substr(sourceRoot.size() + 1);
    }

    return Base::FileInfo(normalized).fileName();
}

/**
 * Trim the "+ offset" that a symbol-table lookup appends to a function name.
 *
 * This is purely heuristic: the "<name> + <decimal>" form is an implementation detail of cpptrace's
 * symbol-table resolution, and was observed in its Mach-O backend and is not part of its documented
 * API. If cpptrace ever changes the format, this stops matching and the offsets reappear in the
 * symbols; the `realCrashCapturesUsableFrames` test is designed to catch that. Note that requiring
 * all digits after the " + " keeps demangled template arguments such as "thinger<1 + 2>" intact.
 */
std::string stripSymbolOffset(const std::string& symbol)
{
    const auto plus = symbol.rfind(" + ");
    if (plus == std::string::npos || plus == 0) {
        return symbol;
    }
    const auto offsetDigits = std::string_view {symbol}.substr(plus + 3);
    if (offsetDigits.empty()
        || offsetDigits.find_first_not_of("0123456789") != std::string_view::npos) {
        return symbol;
    }
    return symbol.substr(0, plus);
}
#endif
}  // namespace

ParsedCrashReport Base::CrashReporter::parse(const std::string& pathToRawReportFile)
{
    FileInfo fileInfo(pathToRawReportFile);
    if (!fileInfo.exists()) {
        throw FileException(std::string("Cannot read file ") + pathToRawReportFile, fileInfo);
    }

    auto onDiskFileSize = fileInfo.size();
    if (onDiskFileSize < sizeof(Header) + sizeof(Footer) || onDiskFileSize > MaxFileSize) {
        throw BadFormatError("Corrupted crash report file " + pathToRawReportFile);
    }
    std::vector<char> buffer(onDiskFileSize);
    Base::ifstream ifs(fileInfo, std::ios::binary);  // NOLINT
    ifs.read(buffer.data(), onDiskFileSize);

    if (!ifs || std::cmp_not_equal(ifs.gcount(), onDiskFileSize)) {
        throw BadFormatError("File read truncated for " + pathToRawReportFile);
    }

    Header header;
    std::memcpy(&header, buffer.data(), sizeof(Header));
    if (header.magic != MagicNumber) {
        throw BadFormatError("Unexpected magic number in crash report file " + pathToRawReportFile);
    }
    if (header.version != 1) {
        throw BadFormatError(
            "Using the fcrash reader for v1, but found v" + std::to_string(header.version)
        );
    }
    if (header.fileSize != onDiskFileSize) {
        throw BadFormatError("Filesize mismatch in " + pathToRawReportFile);
    }

    // Header data matches expectations, start parsing:
    ParsedCrashReport parsedReport;

    Footer footer;
    std::memcpy(&footer, buffer.data() + header.fileSize - sizeof(Footer), sizeof(Footer));
    auto calculatedChecksum = crc32(
        std::span<const char>(buffer.data(), header.fileSize - sizeof(Footer))
    );
    parsedReport.partialWrite = (calculatedChecksum != footer.checksum)
        || hasFlag(header.flags, Flags::PartialWrite);

    parsedReport.pathToRawReportFile = pathToRawReportFile;
    parsedReport.code = header.code;
    parsedReport.faultName = faultCodeName(header.code);
    if (faultAddressIsMeaningful(header.code)) {
        parsedReport.faultAddress = header.faultAddress;
    }

    parsedReport.threadID = header.threadID;
    parsedReport.timestamp = std::chrono::system_clock::time_point {
        std::chrono::seconds {header.timestamp}
    };
    parsedReport.processID = header.processID;

    parsedReport.captureWasSignalSafe = hasFlag(header.flags, Flags::CaptureWasSignalSafe);

    if (header.stringTableOffset + sizeof(Footer) > header.fileSize) {
        throw BadFormatError("String table offset exceeds file size");
    }
    std::size_t stringTableSize = header.fileSize - header.stringTableOffset - sizeof(Footer);
    auto stringTable = std::span<const char>(buffer.data() + header.stringTableOffset, stringTableSize);

    parsedReport.buildID = extractStringFromTable(stringTable, header.buildIDStringOffset);
    parsedReport.minidumpPath = extractStringFromTable(stringTable, header.minidumpPathStringOffset);

    // parsedReport.osVersion = Set by App-level consumer at report-submission, Base has no easy
    // access to OS information
    parsedReport.osID = header.osID;
    parsedReport.architectureID = header.architectureID;

    parsedReport.freecadVersionMajor = header.freecadVersionMajor;
    parsedReport.freecadVersionMinor = header.freecadVersionMinor;
    parsedReport.freecadVersionPatch = header.freecadVersionPatch;
    parsedReport.freecadVersionSuffix
        = extractStringFromTable(stringTable, header.freecadVersionSuffixStringOffset).value_or("");

    // Read the stack frames (with some error checking):
    if (header.frameCount > MaxFrames) {
        throw BadFormatError("Frame count exceeds the maximum number of frames");
    }
    if (header.frameTableOffset + (header.frameCount * sizeof(Frame)) > header.stringTableOffset) {
        throw BadFormatError("Frame count doesn't fit in available storage");
    }

#if defined(FC_HAVE_CPPTRACE) && defined(FCRepositoryHash)
    // Check to see if the current running version is the same as the one in the fcrash file:
    const bool doSymbolication = parsedReport.buildID == FCRepositoryHash;
#else
    constexpr bool doSymbolication = false;
#endif

    if (doSymbolication) {
#ifdef FC_HAVE_CPPTRACE
        // Symbolicate:
        cpptrace::object_trace objectTrace;
        std::unordered_map<cpptrace::frame_ptr, std::string> modulePathMap;
        for (std::uint32_t i = 0; i < header.frameCount; i++) {
            Frame rawFrame;
            std::memcpy(
                &rawFrame,
                buffer.data() + header.frameTableOffset + (i * sizeof(Frame)),
                sizeof(Frame)
            );
            cpptrace::object_frame objectFrame;
            objectFrame.raw_address = rawFrame.rawAddress;
            objectFrame.object_address = rawFrame.moduleOffset;
            objectFrame.object_path
                = extractStringFromTable(stringTable, rawFrame.moduleStringOffset).value_or("");
            modulePathMap[objectFrame.raw_address] = objectFrame.object_path;  // For later lookup
            objectTrace.frames.push_back(std::move(objectFrame));
        }

        const auto [frames] = objectTrace.resolve();  // This call does the actual resolution
        parsedReport.stackFrames.reserve(frames.size());
        for (const auto& frame : frames) {
            ParsedFrame parsedFrame;
            parsedFrame.rawAddress = frame.raw_address;
            parsedFrame.moduleOffset = frame.object_address;

            const std::string& objectPath = modulePathMap[frame.raw_address];

            // To avoid any PII in the backtrace, only include the filename, not the full path:
            parsedFrame.modulePath = FileInfo(objectPath).fileName();

            parsedFrame.symbol = frame.symbol.empty()
                ? std::nullopt
                : std::make_optional(stripSymbolOffset(frame.symbol));

            // `file` means a source file, if we've got it
            const bool haveRealSourceFile = !frame.filename.empty() && frame.filename != objectPath;
            parsedFrame.file = haveRealSourceFile
                ? std::make_optional(stripSourceRoot(frame.filename))
                : std::nullopt;
            parsedFrame.line = frame.line.has_value() ? std::make_optional(frame.line.value())
                                                      : std::nullopt;
            parsedFrame.isInline = frame.is_inline;
            parsedReport.stackFrames.push_back(std::move(parsedFrame));
        }
        parsedReport.symbolicated = true;
#endif
    }
    else {
        parsedReport.stackFrames.reserve(header.frameCount);
        for (std::uint32_t i = 0; i < header.frameCount; i++) {
            Frame rawFrame;
            std::memcpy(
                &rawFrame,
                buffer.data() + header.frameTableOffset + (i * sizeof(Frame)),
                sizeof(Frame)
            );

            ParsedFrame parsedFrame;
            parsedFrame.rawAddress = rawFrame.rawAddress;
            parsedFrame.moduleOffset = rawFrame.moduleOffset;
            std::optional<std::string> modulePath {
                extractStringFromTable(stringTable, rawFrame.moduleStringOffset)
            };

            // Strip any potential PII from the crashing filename:
            parsedFrame.modulePath = FileInfo(modulePath.value_or("")).fileName();

            parsedReport.stackFrames.push_back(std::move(parsedFrame));
        }
    }

    return parsedReport;
}

std::vector<ParsedFrame> Base::CrashReporter::trimLeadingPlumbingFrames(
    const std::vector<ParsedFrame>& frames
)
{
    // To detect the first real frame of the crash stack, we detect the OS-specific "trampoline"
    // function:
    static constexpr std::array dispatchAnchors {
        std::string_view {"KiUserExceptionDispatcher"},  // Windows
        std::string_view {"__restore_rt"},               // glibc and musl rt_sigreturn
        std::string_view {"__kernel_rt_sigreturn"},      // vDSO (ARM and others)
        std::string_view {"__sigtramp"},                 // macOS / BSD
    };

    auto isAnchor = [&](const ParsedFrame& frame) {
        return std::ranges::any_of(dispatchAnchors, [&](std::string_view anchor) {
            return frame.symbol.has_value() && frame.symbol.value().find(anchor) != std::string::npos;
        });
    };

    const auto anchor = std::ranges::find_if(frames, isAnchor);
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

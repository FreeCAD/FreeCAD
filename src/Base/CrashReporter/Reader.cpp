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
#include <cstring>
#include <utility>

#include "Base/Stream.h"
//#include <QSysInfo>
//#include <QString>


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

    //parsedReport.osVersion = QSysInfo::prettyProductName().toStdString();  // Maybe stale on macOS
    parsedReport.osID = header.osID;
    parsedReport.architectureID = header.architectureID;

    parsedReport.freecadVersionMajor = header.freecadVersionMajor;
    parsedReport.freecadVersionMinor = header.freecadVersionMinor;
    parsedReport.freecadVersionPatch = header.freecadVersionPatch;
    parsedReport.freecadVersionRevision = header.freecadVersionRevision;
    parsedReport.freecadVersionSuffix = extractStringFromTable(stringTable, header.freecadVersionSuffixStringOffset);

    // Read the stack frames (with some error checking):
    if (header.frameCount > MaxFrames) {
        throw Base::BadFormatError("Frame count exceeds the maximum number of frames");
    }
    if (header.frameTableOffset + header.frameCount*sizeof(Frame) > header.stringTableOffset) {
        throw Base::BadFormatError("Frame count doesn't fit in available storage");
    }

    parsedReport.stackFrames.reserve(header.frameCount);
    for (std::uint32_t i = 0; i < header.frameCount; i++) {
        Frame rawFrame;
        std::memcpy(&rawFrame, buffer.data() + header.frameTableOffset + i * sizeof(Frame), sizeof(Frame));

        ParsedFrame parsedFrame;
        parsedFrame.rawAddress = rawFrame.rawAddress;
        parsedFrame.moduleOffset = rawFrame.moduleOffset;
        parsedFrame.modulePath = extractStringFromTable(stringTable, rawFrame.moduleStringOffset);

        parsedReport.stackFrames.push_back(std::move(parsedFrame));
    }

    // Symbolicate here...

    // And later after we've put the data into it...
    return parsedReport;
}

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic, hicpp-exception-baseclass)

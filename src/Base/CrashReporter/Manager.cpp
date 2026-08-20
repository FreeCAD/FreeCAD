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

#include "Manager.h"
#include "Reader.h"
#include "Base/Console.h"

#include <Base/FileInfo.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace Base::CrashReporter;

// File-static storage: not members, invisible to any outside code
static std::string s_crashReportDirectory;
static std::vector<ParsedCrashReport> s_reports;

void Manager::scan(const std::string& crashReportDirectory, RetentionPolicy policy)
{
    FileInfo fileInfo {crashReportDirectory};
    if (!fileInfo.exists()) {
        return;
    }
    if (!fileInfo.isDir()) {
        Console().error("Expected parameter to be a directory: %s\n", crashReportDirectory.c_str());
        return;
    }
    s_crashReportDirectory = crashReportDirectory;

    s_reports.clear();
    for (const auto& contentItem : fileInfo.getDirectoryContent()) {
        if (!contentItem.isFile()) {
            continue;  // Do not recurse
        }
        if (contentItem.extension() == "fcrash") {
            // This is what we are looking for: read it
            try {
                auto report = parse(contentItem.filePath());
                archive(report);
                report.stackFrames = trimLeadingPlumbingFrames(report.stackFrames);
                s_reports.push_back(report);
            }
            catch (const Base::Exception& e) {
                // Some sort of corrupt report: log it
                Console().warning(
                    "Corrupted crash report file found: %s\n%s\n",
                    contentItem.filePath().c_str(),
                    e.what()
                );
                FileInfo bad {contentItem.filePath()};
                bad.renameFile((contentItem.filePath() + ".corrupt").c_str());
            }
        }
    }
    enforceRetention(policy);
}

const std::vector<ParsedCrashReport>& Manager::reports()
{
    return s_reports;
}

void Manager::clear()
{
    if (s_crashReportDirectory.empty()) {
        return;  // never scanned: nothing to clear
    }

    FileInfo fileInfo {s_crashReportDirectory};
    if (fileInfo.isDir()) {
        bool success = fileInfo.deleteDirectoryRecursive();
        if (!success) {
            throw FileSystemError("Failed to delete directory " + fileInfo.filePath());
        }
    }
    s_reports.clear();
    if (!fileInfo.createDirectories()) {
        throw FileSystemError("Failed to create directory " + fileInfo.filePath());
    }
}

void Manager::archive(ParsedCrashReport& report)
{
    auto newPaths = archiveFile(report.pathToRawReportFile, report.minidumpPath);
    report.pathToRawReportFile = newPaths.first;
    report.minidumpPath = newPaths.second;
}

namespace
{
Base::FileInfo getArchive()
{
    return Base::FileInfo {s_crashReportDirectory + "/archive"};
}

/// Our fcrash files (and optional minidump files) have a timestamp embedded in their filenames, so
/// to figure out the crash timestamp we can extract it directly from the filename, without having
/// to read or parse anything else.
std::optional<std::int64_t> crashTimestampFromName(const Base::FileInfo& f)
{
    std::string stem = f.fileNamePure();  // "crash-<timestamp>-<pid>"
    auto first = stem.find('-');
    auto last = stem.rfind('-');
    if (first == std::string::npos || last == first) {
        return std::nullopt;
    }
    std::int64_t timestamp = 0;
    std::string_view mid {stem.data() + first + 1, last - first - 1};
    if (std::from_chars(mid.data(), mid.data() + mid.size(), timestamp).ec != std::errc {}) {
        return std::nullopt;
    }
    return timestamp;
}
}  // namespace

void Manager::enforceRetention(RetentionPolicy policy)
{
    auto fileInfo = getArchive();
    auto archiveContents = fileInfo.getDirectoryContent();
    if (archiveContents.size() < policy.maxReports) {
        return;
    }

    // Base our retention count on the number of fcstd files: create a list of just those
    std::vector<FileInfo> fcrashFiles;
    std::ranges::copy_if(archiveContents, std::back_inserter(fcrashFiles), [](const auto& fi) {
        return fi.isFile() && fi.extension() == "fcrash";
    });

    if (fcrashFiles.size() <= policy.maxReports) {
        return;
    }
    const std::size_t maxNumToDelete = fcrashFiles.size() - policy.maxReports;

    // Sort oldest-to-newest (filenames are timestamp-based)
    std::ranges::sort(fcrashFiles, [](const auto& a, const auto& b) {
        return a.fileName() < b.fileName();
    });

    // Now figure out if any of the reports are older than our policy specifies:
    std::size_t deleteCounter = 0;
    for (const auto& contentItem : fcrashFiles) {
        auto timestamp = crashTimestampFromName(contentItem);
        if (!timestamp.has_value()) {
            continue;
        }

        // Check how many days old it is: if it's older than our retention policy, and we still
        // need to delete more files, get rid of this one, and possibly its companion minidump file,
        // if one exists
        namespace ch = std::chrono;
        auto fileTimepoint = ch::system_clock::time_point(ch::seconds(timestamp.value()));
        auto nowTimepoint = ch::system_clock::now();
        auto timeDiff = nowTimepoint - fileTimepoint;
        if (timeDiff <= policy.maxAge) {
            break;  // No need to continue since we sorted by age to begin with
        }

        // Delete the old fcrash file and its optional minidump companion
        std::string dmp = contentItem.dirPath() + "/" + contentItem.fileNamePure() + ".dmp";
        if (!contentItem.deleteFile()) {
            Console().warning("Failed to delete file %s\n", contentItem.filePath().c_str());
        }
        FileInfo dmpInfo {dmp};
        if (dmpInfo.exists()) {
            if (!dmpInfo.deleteFile()) {
                Console().warning("Failed to delete file %s\n", dmpInfo.filePath().c_str());
            }
        }

        ++deleteCounter;
        if (deleteCounter >= maxNumToDelete) {
            break;  // Done!
        }
    }
}

std::pair<std::string, std::string> Manager::archiveFile(
    const std::string& fcrashPath,
    const std::string& dumpPath
)
{
    std::pair<std::string, std::string> newPaths {fcrashPath, dumpPath};
    auto archivePath = getArchive();
    if (!archivePath.exists()) {
        if (!archivePath.createDirectories()) {
            throw FileSystemError("Could not create " + archivePath.filePath());
        }
    }
    auto archiveBase = archivePath.filePath();
    if (FileInfo fcrashInfo {fcrashPath}; fcrashInfo.exists()) {
        fcrashInfo.renameFile((archiveBase + "/" + fcrashInfo.fileName()).c_str());
        newPaths.first = fcrashInfo.filePath();
    }
    if (!dumpPath.empty()) {
        if (FileInfo dmpInfo {dumpPath}; dmpInfo.exists()) {
            dmpInfo.renameFile((archiveBase + "/" + dmpInfo.fileName()).c_str());
            newPaths.second = dmpInfo.filePath();
        }
    }
    return newPaths;
}

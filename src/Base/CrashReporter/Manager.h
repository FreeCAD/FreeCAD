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

#include "Reader.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Base::CrashReporter
{

/**
 * How long crash reports are retained. We enforce the union here: we only delete reports that
 * exceed *both* of these values. So if there are fewer than maxReports reports, nothing is ever
 * deleted. The same is true if no report is older than maxAge.
 */
struct RetentionPolicy
{
    std::size_t maxReports = 10;
    std::chrono::days maxAge {90};
};

class BaseExport Manager
{
public:
    /**
     * Scan a directory for new crash reports
     *
     * This method populates a statically-stored vector of crash reports that is then accessible
     * using `reports()`. The on-disk reports are automatically moved into an archived directory so
     * they don't appear as new reports on the next launch. It also checks the old archives and
     * enforces the report retention policy. Note that the scan is *not* recursive (in particular,
     * the archives live in an "archive" subdirectory of the passed-in path).
     *
     * @param crashReportDirectory The path to the new crash reports
     * @param policy A report retention policy that overrides the defaults (optional)
     */
    static void scan(const std::string& crashReportDirectory, RetentionPolicy policy = {});

    /**
     * Get the reports that were discovered by the last run of `scan()`.
     *
     * @return A vector of new (non-archived) crash reports
     */
    static const std::vector<ParsedCrashReport>& reports();

    /**
     * Full reset of all archived reports. Empties the entire crash directory (as configured by the
     * run of `scan()`).
     */
    static void clear();

private:
    /**
     * Move the crash report file(s) to the archive subdirectory.
     *
     * By definition the archive subdirectory is simply the directory of the original file with an
     * "archive" folder appended at the end.
     *
     * @param report The report to archive. Modified by the process to update its internal path
     * elements.
     */
    static void archive(ParsedCrashReport& report);

    /**
     * Enforce the given retention policy, deleting old reports as needed.
     *
     * @param policy The retention policy to enforce
     */
    static void enforceRetention(RetentionPolicy policy);

    /**
     * Move the crash reports to the archive subfolder.
     *
     * @param fcrashPath path to the *.fcrash file
     * @param dumpPath path to the *.dmp file
     *
     * @returns a pair of new path strings pointing to the archived files (the minidump string will
     * be empty if there was no minidump file).
     */
    static std::pair<std::string, std::string> archiveFile(
        const std::string& fcrashPath,
        const std::string& dumpPath = {}
    );
};

}  // namespace Base::CrashReporter

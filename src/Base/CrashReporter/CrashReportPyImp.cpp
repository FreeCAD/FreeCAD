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

#include <CXX/Python3/Objects.hxx>

#include <chrono>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <string>

#include "FaultCodes.h"

#include "CrashReporter/CrashFramePy.h"

#include "CrashReporter/CrashReportPy.h"
#include "CrashReporter/CrashReportPy.cpp"  // NOLINT

using namespace Base;

std::string CrashReportPy::representation() const
{
    const PointerType report = getParsedCrashReportPtr();
    const std::string timestamp = fmt::format( /*ISO-8601 UTC*/
        "{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(report->timestamp));
    return fmt::format(
        "<CrashReport {} at {}, {} frames, {}{}>",
        report->faultName,
        timestamp,
        report->stackFrames.size(),
        report->symbolicated ? "symbolicated" : "unsymbolicated",
        report->partialWrite ? ", partial" : ""  // Partial should be rare
    );
}

Py::String CrashReportPy::getpath_to_raw_report_file() const
{
    return getParsedCrashReportPtr()->pathToRawReportFile;
}

// These getters return "value or None", so slicing the typed PyCXX wrapper down to Py::Object
// is intentional: the discarded accepts() override is the one we do not want to enforce.
Py::Object CrashReportPy::getfault_address() const
{
    const auto& faultAddress = getParsedCrashReportPtr()->faultAddress;
    if (!faultAddress.has_value()) {
        return Py::None();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    return Py::Long(faultAddress.value());
}

Py::Long CrashReportPy::getthread_id() const
{
    return Py::Long(getParsedCrashReportPtr()->threadID);
}

Py::Float CrashReportPy::gettimestamp() const
{
    // Not going to try to use a Python-native DateTime object here, callers can convert easily
    const auto& ts = getParsedCrashReportPtr()->timestamp;
    return Py::Float(std::chrono::duration<double>(ts.time_since_epoch()).count());
}

Py::Long CrashReportPy::getprocess_id() const
{
    return Py::Long(static_cast<unsigned long>(getParsedCrashReportPtr()->processID));
}

Py::Long CrashReportPy::getfault_code() const
{
    return Py::Long(static_cast<unsigned long>(getParsedCrashReportPtr()->code));
}

Py::String CrashReportPy::getfault_name() const
{
    return getParsedCrashReportPtr()->faultName;
}

Py::Boolean CrashReportPy::getpartial_write() const
{
    return getParsedCrashReportPtr()->partialWrite;
}

Py::Boolean CrashReportPy::getcapture_was_signal_safe() const
{
    return getParsedCrashReportPtr()->captureWasSignalSafe;
}

Py::Object CrashReportPy::getbuild_id() const
{
    const auto& buildID = getParsedCrashReportPtr()->buildID;
    if (!buildID.has_value()) {
        return Py::None();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    return Py::String(buildID.value());
}

Py::Object CrashReportPy::getminidump_path() const
{
    const auto& minidumpPath = getParsedCrashReportPtr()->minidumpPath;
    if (!minidumpPath.has_value()) {
        return Py::None();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    return Py::String(minidumpPath.value());
}

Py::String CrashReportPy::getos() const
{
    return toPyString(osName(getParsedCrashReportPtr()->osID));
}

Py::Object CrashReportPy::getos_version() const
{
    const auto& osVersion = getParsedCrashReportPtr()->osVersion;
    if (!osVersion.has_value()) {
        return Py::None();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    return Py::String(osVersion.value());
}

Py::String CrashReportPy::getarchitecture() const
{
    return toPyString(architectureName(getParsedCrashReportPtr()->architectureID));
}

Py::Tuple CrashReportPy::getfreecad_version() const
{
    const PointerType report = getParsedCrashReportPtr();
    Py::Tuple version(4);
    version.setItem(0, Py::Long(report->freecadVersionMajor));
    version.setItem(1, Py::Long(report->freecadVersionMinor));
    version.setItem(2, Py::Long(report->freecadVersionPatch));
    version.setItem(3, Py::String(report->freecadVersionSuffix));
    return version;
}

Py::Boolean CrashReportPy::getsymbolicated() const
{
    return getParsedCrashReportPtr()->symbolicated;
}

Py::List CrashReportPy::getstack_frames() const
{
    Py::List frames;
    for (const auto& frame : getParsedCrashReportPtr()->stackFrames) {
        frames.append(Py::asObject(new CrashFramePy(new ParsedFrame(frame))));
    }
    return frames;
}

PyObject* CrashReportPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int CrashReportPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

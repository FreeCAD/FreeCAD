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

// Fault code mapping: translates from the signal code number to the name of the signal and an
// indicator about whether that signal has an "address" that is relevant.

#pragma once

#include <FCGlobal.h>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <Base/CrashReporter/Format.h>

namespace Base::CrashReporter
{

struct FaultDescription
{
    std::uint32_t code;
    std::string_view name;
    bool addressIsRelevant;
};

/**
 * Determine whether the fault address is expected to hold meaning for a given code.
 *
 * @param[in] code The signal/fault code
 * @return True if the address is expected to contain useful information, otherwise false
 */
[[nodiscard]] bool BaseExport faultAddressIsMeaningful(std::uint32_t code);

/**
 * Map a signal/exception number to a description.
 *
 * @param[in] code The signal/fault code
 * @return A description of the signal, or std::nullopt
 */
std::optional<FaultDescription> BaseExport describeFaultCode(std::uint32_t code);

/**
 * Get a text representation of the fault that a given code represents.
 *
 * @param code The signal/fault code
 * @return The user-visible label for a signal code. Still not quite "friendly", but better than a
 * random-looking number.
 */
[[nodiscard]] std::string BaseExport faultCodeName(std::uint32_t code);

}  // namespace Base::CrashReporter

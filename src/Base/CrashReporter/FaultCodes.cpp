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

#include "FaultCodes.h"

#include <FCConfig.h>
#include <fmt/format.h>

#if defined(FC_OS_LINUX) || defined(FC_OS_BSD) || defined(FC_OS_MACOSX)
# include <csignal>
#elif defined(FC_OS_WIN32)
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# include <windows.h>
#endif

#include <array>
#include <algorithm>

namespace Base::CrashReporter
{

// clang-format off
namespace
{
#if defined(FC_OS_LINUX) || defined(FC_OS_BSD) || defined(FC_OS_MACOSX)

constexpr std::array faultCodeToDescription {
    FaultDescription {.code = SIGILL,  .name = "SIGILL",  .addressIsRelevant = true},
    FaultDescription {.code = SIGABRT, .name = "SIGABRT", .addressIsRelevant = false},
    FaultDescription {.code = SIGFPE,  .name = "SIGFPE",  .addressIsRelevant = true},
    FaultDescription {.code = SIGBUS,  .name = "SIGBUS",  .addressIsRelevant = true},
    FaultDescription {.code = SIGSEGV, .name = "SIGSEGV", .addressIsRelevant = true}
};

#elif defined(FC_OS_WIN32)

// The six codes at the end have no winnt.h macro: three live in ntstatus.h, which cannot be
// included alongside windows.h without WIN32_NO_STATUS, and the last is an MSVC exception-handling
// implementation detail that Microsoft does not document as a constant (but it comes up in real
// life often enough that we should handle it).
// For documentation of values see MS-ERREF section 2.3.1 --
// https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-erref
constexpr std::array faultCodeToDescription {
    FaultDescription {.code = EXCEPTION_GUARD_PAGE,               .name = "EXCEPTION_GUARD_PAGE",               .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_DATATYPE_MISALIGNMENT,    .name = "EXCEPTION_DATATYPE_MISALIGNMENT",    .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_BREAKPOINT,               .name = "EXCEPTION_BREAKPOINT",               .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_SINGLE_STEP,              .name = "EXCEPTION_SINGLE_STEP",              .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_INVALID_HANDLE,           .name = "EXCEPTION_INVALID_HANDLE",           .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_ILLEGAL_INSTRUCTION,      .name = "EXCEPTION_ILLEGAL_INSTRUCTION",      .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_ACCESS_VIOLATION,         .name = "EXCEPTION_ACCESS_VIOLATION",         .addressIsRelevant = true},
    FaultDescription {.code = EXCEPTION_IN_PAGE_ERROR,            .name = "EXCEPTION_IN_PAGE_ERROR",            .addressIsRelevant = true},
    FaultDescription {.code = EXCEPTION_NONCONTINUABLE_EXCEPTION, .name = "EXCEPTION_NONCONTINUABLE_EXCEPTION", .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_INVALID_DISPOSITION,      .name = "EXCEPTION_INVALID_DISPOSITION",      .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_ARRAY_BOUNDS_EXCEEDED,    .name = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED",    .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_FLT_DENORMAL_OPERAND,     .name = "EXCEPTION_FLT_DENORMAL_OPERAND",     .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_FLT_DIVIDE_BY_ZERO,       .name = "EXCEPTION_FLT_DIVIDE_BY_ZERO",       .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_FLT_INEXACT_RESULT,       .name = "EXCEPTION_FLT_INEXACT_RESULT",       .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_FLT_INVALID_OPERATION,    .name = "EXCEPTION_FLT_INVALID_OPERATION",    .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_FLT_OVERFLOW,             .name = "EXCEPTION_FLT_OVERFLOW",             .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_FLT_STACK_CHECK,          .name = "EXCEPTION_FLT_STACK_CHECK",          .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_FLT_UNDERFLOW,            .name = "EXCEPTION_FLT_UNDERFLOW",            .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_INT_DIVIDE_BY_ZERO,       .name = "EXCEPTION_INT_DIVIDE_BY_ZERO",       .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_INT_OVERFLOW,             .name = "EXCEPTION_INT_OVERFLOW",             .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_PRIV_INSTRUCTION,         .name = "EXCEPTION_PRIV_INSTRUCTION",         .addressIsRelevant = false},
    FaultDescription {.code = EXCEPTION_STACK_OVERFLOW,           .name = "EXCEPTION_STACK_OVERFLOW",           .addressIsRelevant = false},
    FaultDescription {.code = 0xC0000374U,                        .name = "STATUS_HEAP_CORRUPTION",             .addressIsRelevant = false},
    FaultDescription {.code = 0xC0000409U,                        .name = "STATUS_STACK_BUFFER_OVERRUN",        .addressIsRelevant = false},
    FaultDescription {.code = 0xC0000420U,                        .name = "STATUS_ASSERTION_FAILURE",           .addressIsRelevant = false},
    FaultDescription {.code = 0xC0000417U,                        .name = "STATUS_INVALID_CRUNTIME_PARAMETER",  .addressIsRelevant = false},
    FaultDescription {.code = 0xC0000602U,                        .name = "STATUS_FAIL_FAST_EXCEPTION",         .addressIsRelevant = false},
    FaultDescription {.code = 0xE06D7363U,                        .name = "MSVC_CPP_EXCEPTION",                 .addressIsRelevant = false}
};

#else

// Where are we?! Actually, probably Cygwin. Don't panic, we're just a bit lost.
constexpr std::array<FaultDescription, 0> faultCodeToDescription {};

#endif
}

// clang-format on

[[nodiscard]] bool faultAddressIsMeaningful(std::uint32_t code)
{
    const auto description = std::ranges::find(faultCodeToDescription, code, &FaultDescription::code);
    if (description != faultCodeToDescription.end()) {
        return description->addressIsRelevant;
    }
    return false;
}

std::optional<FaultDescription> describeFaultCode(std::uint32_t code)
{
    const auto signal = std::ranges::find(faultCodeToDescription, code, &FaultDescription::code);
    return signal != faultCodeToDescription.end() ? std::make_optional(*signal) : std::nullopt;
}

std::string faultCodeName(std::uint32_t code)
{
    if (const auto description = describeFaultCode(code)) {
        return std::string(description->name);
    }
    return fmt::format("UNKNOWN(0x{:08X})", code);
}

}  // namespace Base::CrashReporter

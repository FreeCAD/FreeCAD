// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Céleste Wouters <foss@elementw.net>
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

#include "PyException.h"

#include <typeinfo>

#if defined(__GLIBCXX__) || defined(_LIBCPP_ABI_VERSION)
# include <cxxabi.h>
# include <memory>
#elif defined(_MSC_VER)
# include <windows.h>  // SEH structures
#endif

#include <fmt/format.h>

#include <Base/Console.h>


#if defined(__GLIBCXX__) || defined(_LIBCPP_ABI_VERSION)
static const std::type_info* typeOfCurrentException()
{
    return abi::__cxa_current_exception_type();
}
#elif defined(_MSC_VER)
// Constants and structured lifted from Wine
// See dlls/msvcrt/cppexcept.h and dlls/msvcrt/cpp.c:_is_exception_typeof
static constexpr ULONG_PTR CxxExceptionCode = 0xe06d7363;
// Do not let the names fool you, VS2022/VC143 actually emits the VC6 version.
static constexpr ULONG_PTR CxxExceptionFrameMagicVC6 = 0x19930520;
static constexpr ULONG_PTR CxxExceptionFrameMagicVC7 = 0x19930521;
static constexpr ULONG_PTR CxxExceptionFrameMagicVC8 = 0x19930522;

struct MsCxxTypeInfo
{
    uint32_t flags;
    uint32_t typeInfo;
};
struct MsCxxTypeInfoTable
{
    uint32_t count;
    uint32_t info[3];
};
struct MsCxxExceptionType
{
    uint32_t flags;
    uint32_t destructor;
    uint32_t customHandler;
    uint32_t typeInfoTable;
};

static_assert(
    sizeof(std::exception_ptr) == sizeof(EXCEPTION_POINTERS),
    "Expected std::exception_ptr to be an SEH EXCEPTION_POINTERS in disguise"
);

// MSVC refuses to compile anything related to C++ exceptions in a function that uses SEH,
// and std::current_exception() does not cross the frame of an SEH-using function either,
// hence this code being separate.
static const std::type_info* typeOfSEHCxxException(const EXCEPTION_RECORD* rec)
{
    if (rec->ExceptionCode != CxxExceptionCode || rec->NumberParameters < 3
        || !(
            rec->ExceptionInformation[0] == CxxExceptionFrameMagicVC6
            || rec->ExceptionInformation[0] == CxxExceptionFrameMagicVC7
            || rec->ExceptionInformation[0] == CxxExceptionFrameMagicVC8
        )) {
        return nullptr;
    }
# if defined(_WIN64)
    if (rec->NumberParameters < 4) {
        return nullptr;
    }
    const uintptr_t base = rec->ExceptionInformation[3];
# else
    constexpr uintptr_t base = 0;
# endif
    __try {
        const auto excType = reinterpret_cast<const MsCxxExceptionType*>(rec->ExceptionInformation[2]);
        const auto tiTable = reinterpret_cast<const MsCxxTypeInfoTable*>(base + excType->typeInfoTable);
        if (tiTable->count == 0) {
            return nullptr;
        }
        // Take the first type in the type info table
        const auto cxxTi = reinterpret_cast<const MsCxxTypeInfo*>(base + tiTable->info[0]);
        return reinterpret_cast<const type_info*>(base + cxxTi->typeInfo);
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
    }
    return nullptr;
}

static const std::type_info* typeOfCurrentException()
{
    const auto currentException = std::current_exception();
    return typeOfSEHCxxException(
        reinterpret_cast<const EXCEPTION_POINTERS*>(&currentException)->ExceptionRecord
    );
}
#else
static const std::type_info* typeOfCurrentException()
{
    return nullptr;
}
#endif

namespace Base
{

void pyThrowWrappedBaseException(const Base::Exception& e, bool report)
{
    if (report) {
        e.reportException();
    }
    throw Py::Exception(e.getPyExceptionType(), fmt::format("FreeCAD exception thrown ({})", e.what()));
}

void pyThrowWrappedStdException(const std::exception& e, bool report)
{
    const auto what = fmt::format("FreeCAD exception thrown ({})", e.what());
    if (report) {
        Base::Console().error("%s\n", what);
    }
    if (dynamic_cast<const std::logic_error*>(&e)) {
        if (dynamic_cast<const std::length_error*>(&e) || dynamic_cast<const std::out_of_range*>(&e)) {
            throw Py::IndexError(what);
        }
        throw Py::ValueError(what);
    }
    if (dynamic_cast<const std::bad_alloc*>(&e)) {
        throw Py::MemoryError(what);
    }
    // TODO: throw Py::OSError and forward errno upon std::system_error
    throw Py::RuntimeError(what);
}

void pyThrowWrappedUnknownException(bool report)
{
    const auto type = typeOfCurrentException();
    if (type == nullptr) {
        if (report) {
            Base::Console().error("Unknown C++ exception\n");
        }
        throw Py::RuntimeError("Unknown C++ exception");
    }
#if defined(__GLIBCXX__) || defined(_LIBCPP_ABI_VERSION)
    // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    std::unique_ptr<char[], void (*)(void*)> demangledStorage {
        abi::__cxa_demangle(type->name(), nullptr, nullptr, nullptr),
        std::free
    };
    // NOLINTEND(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    const auto demangled = demangledStorage ? demangledStorage.get() : type->name();
#else
    const auto demangled = type->name();
#endif
    const auto what = fmt::format("Unknown C++ exception ({})", demangled);
    if (report) {
        Base::Console().error("%s\n", what);
    }
    throw Py::RuntimeError(what);
}

}  // namespace Base

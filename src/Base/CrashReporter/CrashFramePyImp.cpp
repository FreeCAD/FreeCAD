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

#include <fmt/format.h>

#include <string>
#include <string_view>

#include "CrashReporter/CrashFramePy.h"
#include "CrashReporter/CrashFramePy.cpp"  // NOLINT

using namespace Base;
using namespace std::literals::string_view_literals;

std::string CrashFramePy::representation() const
{
    const PointerType frame = getParsedFramePtr();
    const std::string& module = frame->modulePath;
    return fmt::format(
        "<CrashFrame 0x{:016X} in {}: {}>",
        frame->rawAddress,
        module.empty() ? "<unknown module>"sv : module,
        frame->symbol.has_value() ? *frame->symbol : "<unresolved>"sv
    );
}

Py::Long CrashFramePy::getaddress() const
{
    return Py::Long(getParsedFramePtr()->rawAddress);
}

Py::String CrashFramePy::getmodule() const
{
    return getParsedFramePtr()->modulePath;
}

Py::Boolean CrashFramePy::getis_inline() const
{
    return getParsedFramePtr()->isInline;
}

// These getters return "value or None", so slicing the typed PyCXX wrapper down to Py::Object
// is intentional: the discarded accepts() override is the one we do not want to enforce.
Py::Object CrashFramePy::getmodule_offset() const
{
    const auto& moduleOffset = getParsedFramePtr()->moduleOffset;
    if (!moduleOffset.has_value()) {
        return Py::None();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    return Py::Long(moduleOffset.value());
}

Py::Object CrashFramePy::getsymbol() const
{
    const auto& symbol = getParsedFramePtr()->symbol;
    if (!symbol.has_value()) {
        return Py::None();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    return Py::String(symbol.value());
}

Py::Object CrashFramePy::getfile() const
{
    const auto& file = getParsedFramePtr()->file;
    if (!file.has_value()) {
        return Py::None();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    return Py::String(file.value());
}

Py::Object CrashFramePy::getline() const
{
    const auto& line = getParsedFramePtr()->line;
    if (!line.has_value()) {
        return Py::None();
    }
    // We need the inner static_cast to ensure that Py::Long uses the right constructor
    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    return Py::Long(static_cast<unsigned long>(line.value()));
}

PyObject* CrashFramePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int CrashFramePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

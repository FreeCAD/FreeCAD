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

#pragma once

#include <FCGlobal.h>

#include <exception>

#include <PyCXX/CXX/Exception.hxx>

#include "Exception.h"


namespace Base
{

[[noreturn]]
BaseExport void pyThrowWrappedBaseException(const Base::Exception& e, bool report);
[[noreturn]]
BaseExport void pyThrowWrappedStdException(const std::exception& e, bool report);
[[noreturn]]
BaseExport void pyThrowWrappedUnknownException(bool report);

/**
 * @brief Wrap the exceptions emitted by some C++ code to PyCXX exceptions so
 *        they can bubble up through a Python call stack.
 * @param callable Callable object, typically a lambda, to wrap the exceptions of.
 * @param report Whether to report the error to the console.
 * @note Does not specifically handle OpenCascade exceptions; use Part's pyWrapCppExceptions
 *       variant to handle calls that can throw those instead.
 */
// NOLINTBEGIN(cppcoreguidelines-missing-std-forward)
template<typename Callable>
inline decltype(auto) pyWrapCppExceptions(Callable&& callable, bool report = false)
{
    try {
        return callable();
    }
    catch (const Py::BaseException&) {
        throw;
    }
    catch (const Base::Exception& e) {
        pyThrowWrappedBaseException(e, report);
    }
    catch (const std::exception& e) {
        pyThrowWrappedStdException(e, report);
    }
    catch (...) {
        pyThrowWrappedUnknownException(report);
    }
}
// NOLINTEND(cppcoreguidelines-missing-std-forward)

}  // namespace Base

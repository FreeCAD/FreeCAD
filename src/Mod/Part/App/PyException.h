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

#include <exception>

#include <Standard_Failure.hxx>

#include <Base/Exception.h>
#include <Base/PyException.h>

#include <Mod/Part/PartGlobal.h>

namespace Part
{

[[noreturn]]
PartExport void pyThrowWrappedOccException(const Standard_Failure& e, PyObject* occErrorType, bool report);

/**
 * @brief Wrap the exceptions emitted by some C++ code to PyCXX exceptions so
 *        they can bubble up through a Python call stack.
 * @param callable Callable object, typically a lambda, to wrap the exceptions of.
 * @param report Whether to report the error to the console.
 * @param occErrorType Python error class to use for OpenCascade exceptions; if nullptr then
 *        an appropriate type for the C++ exception will be used.
 */
// NOLINTBEGIN(cppcoreguidelines-missing-std-forward)
template<typename Callable>
inline decltype(auto) pyWrapCppExceptions(
    Callable&& callable,
    PyObject* occErrorType = nullptr,
    bool report = false
)
{
    try {
        return callable();
    }
    catch (const Py::BaseException&) {
        throw;
    }
    catch (const Standard_Failure& e) {
        pyThrowWrappedOccException(e, occErrorType, report);
    }
    catch (const Base::Exception& e) {
        Base::pyThrowWrappedBaseException(e, report);
    }
    catch (const std::exception& e) {
        Base::pyThrowWrappedStdException(e, report);
    }
    catch (...) {
        Base::pyThrowWrappedUnknownException(report);
    }
}
// NOLINTEND(cppcoreguidelines-missing-std-forward)

}  // namespace Part

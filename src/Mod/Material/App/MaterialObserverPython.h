// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
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

#include <fastsignals/signal.h>
#include <CXX/Objects.hxx>

#include <memory>
#include <vector>

namespace Materials
{

class Material;

class MaterialObserverPython
{
public:
    explicit MaterialObserverPython(const Py::Object& obj);
    virtual ~MaterialObserverPython();

    static void addObserver(const Py::Object& obj);
    static void removeObserver(const Py::Object& obj);
    static void cleanup();

private:
    void slotCreatedMaterial(const Material& material);
    void slotChangedMaterial(const Material& material);
    void slotDeletedMaterial(const Material& material);

private:
    // Keep a strong reference to the Python observer while it is registered.
    Py::Object inst;
    static std::vector<std::unique_ptr<MaterialObserverPython>> _instances;

    using Connection = struct PythonObject
    {
        fastsignals::scoped_connection slot;
        Py::Object py;
        PyObject* ptr()
        {
            return py.ptr();
        }
    };

    Connection pyCreatedMaterial;
    Connection pyChangedMaterial;
    Connection pyDeletedMaterial;
};

}  // namespace Materials

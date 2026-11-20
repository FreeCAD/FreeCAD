// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#include <Base/Interpreter.h>
#include <Base/Tools.h>

#include "SolverRegistry.h"


namespace Assembly
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("AssemblyApp")
    {
        add_varargs_method(
            "getAvailableSolvers",
            &Module::getAvailableSolvers,
            "getAvailableSolvers() -- Returns a list of registered solver backend names."
        );
        initialize("This module is the Assembly module.");  // register with Python
    }

private:
    Py::Object getAvailableSolvers(const Py::Tuple& /*args*/)
    {
        auto solvers = Solver::SolverRegistry::instance().getAvailableSolvers();
        Py::List result;
        for (const auto& name : solvers) {
            result.append(Py::String(name));
        }
        return result;
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace Assembly

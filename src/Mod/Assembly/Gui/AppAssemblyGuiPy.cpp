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

#include "LightweightWorkspaceProxyViewOptimizer.h"

namespace AssemblyGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("AssemblyGui")
    {
        add_noargs_method(
            "optimizeLightweightWorkspaceActiveView",
            &Module::optimizeLightweightWorkspaceActiveView,
            "optimizeLightweightWorkspaceActiveView() -- Run the lightweight workspace "
            "view-aware optimizer immediately for the current active 3D view."
        );
        initialize("This module is the Assembly module.");  // register with Python
    }

    Py::Object optimizeLightweightWorkspaceActiveView()
    {
        LightweightWorkspaceProxyViewOptimizer::optimizeActiveView();
        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace AssemblyGui

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

#include "PreCompiled.h"

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>

#include "ViewProviderAssembly.h"
#include "ViewProviderAssemblyLink.h"
#include "ViewProviderBom.h"
#include "ViewProviderBomGroup.h"
#include "ViewProviderJointGroup.h"
#include "ViewProviderViewGroup.h"
#include "ViewProviderSimulationGroup.h"


namespace AssemblyGui
{
extern PyObject* initModule();
}

/* Python entry */
PyMOD_INIT_FUNC(AssemblyGui)
{
    // load dependent module
    try {
        Base::Interpreter().runString("import SpreadsheetGui");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* mod = AssemblyGui::initModule();
    Base::Console().log("Loading AssemblyGui module... done\n");


    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.

    AssemblyGui::ViewProviderAssembly::init();
    AssemblyGui::ViewProviderAssemblyLink::init();
    AssemblyGui::ViewProviderBom::init();
    AssemblyGui::ViewProviderBomGroup::init();
    AssemblyGui::ViewProviderJointGroup::init();
    AssemblyGui::ViewProviderViewGroup::init();
    AssemblyGui::ViewProviderSimulationGroup::init();

    PyMOD_Return(mod);
}

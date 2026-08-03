// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include "WorkerExtension.h"


using namespace Fem;

EXTENSION_PROPERTY_SOURCE(Fem::WorkerExtension, App::DocumentObjectExtension);

WorkerExtension::WorkerExtension()
{
    initExtensionType(Fem::WorkerExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY_TYPE(
        WorkingDirectory,
        (""),
        "Worker",
        App::PropertyType(App::Prop_Transient | App::Prop_Hidden | App::Prop_Output),
        "Solver working directory"
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        Tool,
        (Py::Object()),
        "Worker",
        App::PropertyType(
            App::Prop_Transient | App::Prop_Hidden | App::Prop_ReadOnly | App::Prop_Output
        ),
        "Tool for run the worker"
    );
}

WorkerExtension::~WorkerExtension() = default;


// Python feature ---------------------------------------------------------

namespace App
{

EXTENSION_PROPERTY_SOURCE_TEMPLATE(Fem::WorkerExtensionPython, Fem::WorkerExtension)

// explicit template instantiation
template class FemExport ExtensionPythonT<Fem::WorkerExtension>;

}  // namespace App

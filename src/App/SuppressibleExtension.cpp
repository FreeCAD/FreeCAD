// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 Florian Foinant-Willig
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


#include <Base/Tools.h>

#include "Extension.h"
#include "SuppressibleExtension.h"
#include "SuppressibleExtensionPy.h"


namespace App
{

EXTENSION_PROPERTY_SOURCE(App::SuppressibleExtension, App::DocumentObjectExtension)


EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::SuppressibleExtensionPython, App::SuppressibleExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<SuppressibleExtensionPythonT<SuppressibleExtension>>;


SuppressibleExtension::SuppressibleExtension()
{
    initExtensionType(SuppressibleExtension::getExtensionClassTypeId());
    EXTENSION_ADD_PROPERTY_TYPE(Suppressed,
                                (false),
                                "Base",
                                PropertyType(Prop_None),
                                "Is object suppressed");
}

SuppressibleExtension::~SuppressibleExtension() = default;

PyObject* SuppressibleExtension::getExtensionPyObject()
{

    if (ExtensionPythonObject.is(Py::_None())) {
        // ref counter is set to 1
        auto ext = new SuppressibleExtensionPy(this);
        ExtensionPythonObject = Py::Object(ext, true);
    }
    return Py::new_reference_to(ExtensionPythonObject);
}

}  // namespace App

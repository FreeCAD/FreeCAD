// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2007 Werner Mayer
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

#include "InventorObject.h"
#include "DocumentObjectPy.h"


using namespace App;

PROPERTY_SOURCE(App::InventorObject, App::GeoFeature)


InventorObject::InventorObject()
{
    ADD_PROPERTY_TYPE(Buffer, (""), "", Prop_None, "String buffer with a scene graph");
    ADD_PROPERTY_TYPE(FileName, (""), "", Prop_None, "Path to an Inventor file");
}

InventorObject::~InventorObject() = default;

short InventorObject::mustExecute() const
{
    return 0;
}

PyObject* InventorObject::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

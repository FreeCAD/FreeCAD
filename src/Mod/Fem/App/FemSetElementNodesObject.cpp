/***************************************************************************
 *   Copyright (c) 2023 Peter McB                                          *
 *                                                                         *
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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
 ***************************************************************************/

#include "PreCompiled.h"

#include <App/DocumentObjectPy.h>

#include "FemSetElementNodesObject.h"


using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemSetElementNodesObject, Fem::FemSetObject)

FemSetElementNodesObject::FemSetElementNodesObject()
{
    ADD_PROPERTY_TYPE(Elements,
                      (),
                      "Element indexes",
                      Prop_None,
                      "Elements belonging to the ElementSet");
}

FemSetElementNodesObject::~FemSetElementNodesObject() = default;

short FemSetElementNodesObject::mustExecute() const
{
    return 0;
}

PyObject* FemSetElementNodesObject::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

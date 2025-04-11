// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

#ifndef FEM_POSTDATAOBJECT_H
#define FEM_POSTDATAOBJECT_H

#include <CXX/Extensions.hxx>
#include <vtkDataObject.h>
#include <vtkSmartPointer.h>


namespace Fem
{

class PostDataObjectPy: public Py::PythonClass<PostDataObjectPy>
{
public:
    static void init_type(PyObject* mod);

    PostDataObjectPy(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kdws);
    ~PostDataObjectPy() override;

    static Py::Object create(vtkSmartPointer<vtkDataObject> data);
    Py::Object getDescription();

    vtkSmartPointer<vtkDataObject> data;

protected:
    Py::Object repr() override;
    Py::Object getattro(const Py::String& attr) override;
};

}  // namespace Fem

#endif  // FEM_POSTDATAOBJECT_H

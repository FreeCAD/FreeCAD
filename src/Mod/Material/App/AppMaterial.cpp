/***************************************************************************
*   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
*                                                                         *
*   This file is part of the FreeCAD CAx development system.              *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU Lesser General Public License (LGPL)    *
*   as published by the Free Software Foundation; either version 2 of     *
*   the License, or (at your option) any later version.                   *
*   for detail see the LICENCE text file.                                 *
*                                                                         *
*   FreeCAD is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU Lesser General Public License for more details.                   *
*                                                                         *
*   You should have received a copy of the GNU Library General Public     *
*   License along with FreeCAD; if not, write to the Free Software        *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
*   USA                                                                   *
*                                                                         *
***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>

// #include "Model.h"
#include "ModelManagerPy.h"
#include "ModelPropertyPy.h"
#include "ModelPy.h"
#include "MaterialPy.h"
#include "MaterialManagerPy.h"

namespace Materials {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Material")
    {
        initialize("This module is the Material module."); // register with Python
    }

    ~Module() override {}

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace Materials

PyMOD_INIT_FUNC(Material)
{
    PyObject* module = Materials::initModule();

    Base::Console().Log("Loading Material module... done\n");

    Base::Interpreter().addType(&Materials::MaterialManagerPy ::Type, module, "MaterialManager");
    Base::Interpreter().addType(&Materials::MaterialPy        ::Type, module, "Material");
    Base::Interpreter().addType(&Materials::ModelManagerPy    ::Type, module, "ModelManager");
    Base::Interpreter().addType(&Materials::ModelPropertyPy   ::Type, module, "ModelProperty");
    Base::Interpreter().addType(&Materials::ModelPy           ::Type, module, "Model");

    PyMOD_Return(module);
}

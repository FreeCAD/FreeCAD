/***************************************************************************
 *   Copyright (c) 2017 Shai Seger <shaise at gmail>                       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "CAMSim.h"
#include "CAMSimPy.h"


namespace CAMSimulator
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("CAMSimulator")
    {
        initialize("This module is the CAMSimulator module.");  // register with Python
    }

    ~Module() override
    {}

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}


}  // namespace CAMSimulator


/* Python entry */
PyMOD_INIT_FUNC(CAMSimulator)
{
    // load dependent module
    try {
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import Path");
        Base::Interpreter().runString("import Mesh");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    //
    PyObject* mod = CAMSimulator::initModule();
    Base::Console().log("Loading CAMSimulator module.... done\n");

    // Add Types to module
    Base::Interpreter().addType(&CAMSimulator::CAMSimPy::Type, mod, "PathSim");

    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
    CAMSimulator::CAMSim::init();

    PyMOD_Return(mod);
}

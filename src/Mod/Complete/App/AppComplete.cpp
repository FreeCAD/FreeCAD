/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
#ifndef _PreComp_
# include <Python.h>
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "CompleteConfiguration.h"

namespace Complete {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Complete")
    {
        initialize("This module is the Complete module."); // register with Python
    }

    virtual ~Module() {}

private:
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace Complete


/* Python entry */
PyMODINIT_FUNC initComplete()
{
    // load dependent module
    try {
        Base::Interpreter().loadModule("Part");
        Base::Interpreter().loadModule("Mesh");
        Base::Interpreter().loadModule("Points");
        //Base::Interpreter().loadModule("MeshPart");
        //Base::Interpreter().loadModule("Assembly");
        Base::Interpreter().loadModule("Drawing");
        Base::Interpreter().loadModule("Raytracing");
#       ifdef COMPLETE_SHOW_SKETCHER
        Base::Interpreter().loadModule("Sketcher");
#       endif
        Base::Interpreter().loadModule("PartDesign");
        Base::Interpreter().loadModule("Image");
        //Base::Interpreter().loadModule("Cam");
#       ifdef COMPLETE_USE_DRAFTING
        try {
            Base::Interpreter().loadModule("Draft");
        }
        catch (const Base::Exception& e) {
            // If called from console then issue a message but don't stop with an error
            PySys_WriteStdout("Import error: %s\n", e.what());
        }
#       endif
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }
    (void)Complete::initModule();
    Base::Console().Log("Loading Complete module... done\n");
}

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
//# include <ode/ode.h>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "Item.h"
#include "Product.h"
#include "ProductRef.h"

#include "Constraint.h"
#include "ConstraintGroup.h"


extern struct PyMethodDef Assembly_methods[];

PyDoc_STRVAR(module_Assembly_doc,
"This module is the Assembly module.");


/* Python entry */
extern "C" {
void AssemblyExport initAssembly()
{
    // load dependent module
    try {
        Base::Interpreter().runString("import Part");
        //Base::Interpreter().runString("import PartDesign");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }
#if PY_MAJOR_VERSION >= 3
    static struct PyModuleDef AssemblyAPIDef = {
        PyModuleDef_HEAD_INIT,
        "Assembly", module_Assembly_doc, -1, Assembly_methods,
        NULL, NULL, NULL, NULL
    };
    PyModule_Create(&AssemblyAPIDef);
#else
    Py_InitModule3("Assembly", Assembly_methods, module_Assembly_doc);   /* mod name, table ptr */
#endif
    Base::Console().Log("Loading Assembly module... done\n");


	//dWorldID id = dWorldCreate();
	//dWorldDestroy(id);

    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
 
    // Item hirachy
    Assembly::Item          ::init();
    Assembly::Product       ::init();
    Assembly::ProductRef    ::init();

    // constraint hirachy
    Assembly::Constraint        ::init();
    Assembly::ConstraintGroup   ::init();
}

} // extern "C"

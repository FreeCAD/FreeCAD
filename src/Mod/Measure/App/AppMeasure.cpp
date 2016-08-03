/***************************************************************************
 *   Copyright (c) 2013      Luke Parry <l.parry@warwick.ac.uk>            *
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

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "Measurement.h"
#include "MeasurementPy.h"

struct PyMethodDef Measure_methods[] = {
//    {"read"   , read,  1},
    {NULL, NULL}        /* end of table marker */
};


PyDoc_STRVAR(module_Measure_doc,
"This module is the Measure module.");


/* Python entry */
extern "C" {
void MeasureExport initMeasure()
{
    // load dependent module
    try {
        Base::Interpreter().runString("import Part");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }
    PyObject* measureModule = Py_InitModule3("Measure", Measure_methods, module_Measure_doc);   /* mod name, table ptr */
 
    // Add Types to module
    Base::Interpreter().addType(&Measure::MeasurementPy      ::Type,measureModule,"Measurement");

    Base::Console().Log("Loading Measure module... done\n");
    
    Measure::Measurement         ::init();
}



} // extern "C"

// debug print for sketchsolv 
void debugprint(std::string s)
{
    Base::Console().Log(s.c_str());
}
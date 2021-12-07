/***************************************************************************
 *   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
 *   Copyright (c) 2007 Jürgen Riegel <Juergen.Riegel@web.de>              *
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


extern struct PyMethodDef Cam_methods[];

PyDoc_STRVAR(module_part_doc,
             "This module is the playground for the CAM stuff.");

extern "C"
{
    void CamExport initCam()
    {
        // load dependent module
        try {
            Base::Interpreter().loadModule("Part");
            Base::Interpreter().loadModule("Mesh");
        }
        catch(const Base::Exception& e) {
            PyErr_SetString(PyExc_ImportError, e.what());
            return;
        }

        Py_InitModule3("Cam", Cam_methods, module_part_doc);   /* mod name, table ptr */
        Base::Console().Log("Loading Cam module... done\n");

        // NOTE: To finish the initialization of our own type objects we must
        // call PyType_Ready, otherwise we run into a segmentation fault, later on.
        // This function is responsible for adding inherited slots from a type's base class.

        return;
    }

} // extern "C"

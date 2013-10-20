/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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

#include "RayFeature.h"
#include "RayProject.h"
#include "RaySegment.h"
#include "LuxFeature.h"
#include "LuxProject.h"

extern struct PyMethodDef Raytracing_methods[];


extern "C" {
void AppRaytracingExport initRaytracing()
{
    // load dependent module
    try {
        Base::Interpreter().loadModule("Part");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }

	Raytracing::RaySegment       ::init();
	Raytracing::RayFeature       ::init();
	Raytracing::RayProject       ::init();
    Raytracing::LuxFeature       ::init();
    Raytracing::LuxProject       ::init();


    (void) Py_InitModule("Raytracing", Raytracing_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading Raytracing module... done\n");

}

} // extern "C" {

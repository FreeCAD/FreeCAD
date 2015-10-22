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

#include "Points.h"
#include "PointsPy.h"
#include "Properties.h"
#include "PropertyPointKernel.h"
#include "FeaturePointsImportAscii.h"


/* registration table  */

extern struct PyMethodDef Points_Import_methods[];



/* Python entry */
extern "C" {
void PointsExport initPoints()
{
    PyObject* pointsModule =  Py_InitModule("Points", Points_Import_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading Points module... done\n");

    // add python types
    Base::Interpreter().addType(&Points::PointsPy  ::Type,pointsModule,"Points");

    // add properties
    Points::PropertyGreyValue     ::init();
    Points::PropertyGreyValueList ::init();
    Points::PropertyNormalList    ::init();
    Points::PropertyCurvatureList ::init();
    Points::PropertyPointKernel   ::init();

    // add data types
    Points::Feature               ::init();
    Points::FeaturePython         ::init();
    Points::Export                ::init();
    Points::ImportAscii           ::init();
}

} // extern "C"

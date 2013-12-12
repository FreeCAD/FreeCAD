/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   Jürgen Riegel 2007                                                    *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <Python.h>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
 
#include "FeaturePage.h"
#include "FeatureView.h"
#include "FeatureViewPart.h"
#include "FeatureViewAnnotation.h"
#include "FeatureViewSymbol.h"
#include "FeatureProjection.h"
#include "FeatureClip.h"
#include "PageGroup.h"

extern struct PyMethodDef Drawing_methods[];

PyDoc_STRVAR(module_drawing_doc,
"This module is the drawing module.");


/* Python entry */
extern "C" {
void DrawingExport initDrawing()
{
    // load dependent module
    try {
        Base::Interpreter().loadModule("Part");
        //Base::Interpreter().loadModule("Mesh");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }
    Py_InitModule3("Drawing", Drawing_methods, module_drawing_doc);   /* mod name, table ptr */
    Base::Console().Log("Loading Drawing module... done\n");


    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
 
    Drawing::FeaturePage            ::init();
    Drawing::FeatureView            ::init();
    Drawing::FeatureViewPart        ::init();
    Drawing::PageGroup              ::init();
    Drawing::FeatureProjection      ::init();
    Drawing::FeatureViewPartPython  ::init();
    Drawing::FeatureViewPython      ::init();
    Drawing::FeatureViewAnnotation  ::init();
    Drawing::FeatureViewSymbol      ::init();
    Drawing::FeatureClip            ::init();
}

} // extern "C"

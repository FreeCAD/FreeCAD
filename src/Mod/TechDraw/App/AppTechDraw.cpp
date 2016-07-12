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

#include "DrawPage.h"
#include "DrawSVGTemplate.h"
#include "DrawParametricTemplate.h"
#include "DrawView.h"
#include "DrawViewCollection.h"
#include "DrawViewPart.h"
#include "DrawViewSection.h"
#include "DrawViewAnnotation.h"
#include "DrawViewDimension.h"
#include "DrawProjGroupItem.h"
#include "DrawProjGroup.h"
#include "DrawViewSymbol.h"
#include "DrawViewClip.h"
#include "DrawHatch.h"
#include "DrawViewDraft.h"
#include "DrawViewSpreadsheet.h"

extern struct PyMethodDef TechDraw_methods[];

PyDoc_STRVAR(module_techdraw_doc,
"This module is the TechDraw module.");


/* Python entry */
extern "C" {
void TechDrawExport initTechDraw()
{
    // load dependent module
    try {
        Base::Interpreter().loadModule("Part");
        Base::Interpreter().loadModule("Measure");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }
    Py_InitModule3("TechDraw", TechDraw_methods, module_techdraw_doc);   /* mod name, table ptr */
    Base::Console().Log("Loading TechDraw module... done\n");


    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.

    TechDraw::DrawPage            ::init();
    TechDraw::DrawView            ::init();
    TechDraw::DrawViewCollection  ::init();
    TechDraw::DrawViewPart        ::init();
    TechDraw::DrawViewAnnotation  ::init();
    TechDraw::DrawViewSymbol      ::init();
    TechDraw::DrawViewSpreadsheet ::init();

    TechDraw::DrawViewSection     ::init();
    TechDraw::DrawViewDimension   ::init();
    TechDraw::DrawProjGroup       ::init();
    TechDraw::DrawProjGroupItem   ::init();
    TechDraw::DrawTemplate        ::init();
    TechDraw::DrawParametricTemplate::init();
    TechDraw::DrawSVGTemplate     ::init();

    TechDraw::DrawViewClip        ::init();
    TechDraw::DrawHatch           ::init();
    TechDraw::DrawViewDraft       ::init();

   // Python Types
    TechDraw::DrawViewPython      ::init();
    TechDraw::DrawViewPartPython  ::init();
    TechDraw::DrawTemplatePython  ::init();
    TechDraw::DrawViewSymbolPython::init();
}

} // extern "C"

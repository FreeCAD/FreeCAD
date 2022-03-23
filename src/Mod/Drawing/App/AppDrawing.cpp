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

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Interpreter.h>
 
#include "FeaturePage.h"
#include "FeatureView.h"
#include "FeatureViewPart.h"
#include "FeatureViewAnnotation.h"
#include "FeatureViewSymbol.h"
#include "FeatureProjection.h"
#include "FeatureViewSpreadsheet.h"
#include "FeatureClip.h"
#include "PageGroup.h"


namespace Drawing {
extern PyObject* initModule();
}

/* Python entry */
PyMOD_INIT_FUNC(Drawing)
{
    // load dependent module
    try {
        Base::Interpreter().loadModule("Part");
        //Base::Interpreter().loadModule("Mesh");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }
    PyObject* mod = Drawing::initModule();
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
    Drawing::FeatureViewSpreadsheet ::init();

    PyMOD_Return(mod);
}

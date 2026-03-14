/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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

#include "FontPy.h"
#include "Measure.h"
#include "ShapeTextPy.h"


namespace Text
{
extern PyObject* initModule();
}

/* Python entry */
PyMOD_INIT_FUNC(Text)
{
    // load dependent module
    try {
       Base::Interpreter().runString("import Part");
    }
    catch (const Base::Exception& e) {
       PyErr_SetString(PyExc_ImportError, e.what());
       PyMOD_Return(nullptr);
    }

    PyObject* textModule = Text::initModule();

    // Add Types to module
    Base::Interpreter().addType(&Text::FontPy::Type, textModule, "Font");
    Base::Interpreter().addType(&Text::ShapeTextPy::Type, textModule, "ShapeText");


    Text::Font::init();
    Text::FontPython::init();
    Text::ShapeText::init();
    Text::ShapeTextPython::init();

    // connect to unified measurement facility
    Text::Measure::initialize();


    Base::Console().log("Loading Text module... done");

    PyMOD_Return(textModule);
}

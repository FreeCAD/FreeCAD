// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2023 Pierre-Louis Boyer <development@Ondsel.com>                       *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#include <App/Application.h>
#include <App/Document.h>
#include <Base/GeometryPyCXX.h>

// inclusion of the generated files (generated out of QuickMeasurePy.xml)
#include "QuickMeasurePy.h"
#include "QuickMeasurePy.cpp"


using namespace MeasureGui;

// returns a string which represents the object e.g. when printed in python
std::string QuickMeasurePy::representation() const
{
    return "<MeasureGui::QuickMeasure>";
}

PyObject* QuickMeasurePy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of BoundBoxPy and the Twin object
    return new QuickMeasurePy(new QuickMeasure);
}

// constructor method
int QuickMeasurePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* QuickMeasurePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int QuickMeasurePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

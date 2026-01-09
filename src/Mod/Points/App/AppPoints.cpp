// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2011 Jürgen Riegel <juergen.riegel@web.de>                             *
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


#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "Points.h"
#include "PointsPy.h"
#include "Properties.h"
#include "PropertyPointKernel.h"
#include "Structured.h"


namespace Points
{
extern PyObject* initModule();
}

/* Python entry */
PyMOD_INIT_FUNC(Points)
{
    // clang-format off
    PyObject* pointsModule = Points::initModule();
    Base::Console().log("Loading Points module… done\n");

    // add python types
    Base::Interpreter().addType(&Points::PointsPy::Type, pointsModule, "Points");

    // add properties
    Points::PropertyGreyValue       ::init();
    Points::PropertyGreyValueList   ::init();
    Points::PropertyNormalList      ::init();
    Points::PropertyCurvatureList   ::init();
    Points::PropertyPointKernel     ::init();

    // add data types
    Points::Feature                 ::init();
    Points::Structured              ::init();
    Points::FeatureCustom           ::init();
    Points::StructuredCustom        ::init();
    Points::FeaturePython           ::init();
    PyMOD_Return(pointsModule);
    // clang-format on
}

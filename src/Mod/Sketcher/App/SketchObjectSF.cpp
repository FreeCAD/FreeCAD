// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008 Jürgen Riegel
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
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

#include "SketchObjectSF.h"


using namespace Sketcher;

PROPERTY_SOURCE(Sketcher::SketchObjectSF, Part::Part2DObject)

SketchObjectSF::SketchObjectSF()
{
    ADD_PROPERTY_TYPE(
        SketchFlatFile,
        (nullptr),
        "",
        (App::PropertyType)(App::Prop_None),
        "SketchFlat file (*.skf) which defines this sketch"
    );
}

short SketchObjectSF::mustExecute() const
{
    if (SketchFlatFile.isTouched()) {
        return 1;
    }
    return 0;
}

App::DocumentObjectExecReturn* SketchObjectSF::execute()
{
    Base::Console().warning(
        "{}: This feature is deprecated and won't be longer supported in future FreeCAD versions\n",
        this->getNameInDocument()
    );
    // do nothing
    return App::DocumentObject::StdReturn;
}

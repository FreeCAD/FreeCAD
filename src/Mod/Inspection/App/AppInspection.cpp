// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2004 Werner Mayer <wmayer@users.sourceforge.net>                       *
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
#include <Base/PyObjectBase.h>

#include "InspectionFeature.h"


namespace Inspection
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Inspection")
    {
        initialize("This module is the Inspection module.");  // register with Python
    }

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace Inspection


/* Python entry */
PyMOD_INIT_FUNC(Inspection)
{
    // ADD YOUR CODE HERE
    //
    //
    PyObject* mod = Inspection::initModule();
    Base::Console().log("Loading Inspection module… done\n");
    // clang-format off
    Inspection::PropertyDistanceList    ::init();
    Inspection::Feature                 ::init();
    Inspection::Group                   ::init();
    // clang-format on
    PyMOD_Return(mod);
}

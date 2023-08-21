/***************************************************************************
 *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
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


namespace DraftUtils {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("DraftUtils")
    {
        add_varargs_method("readDXF",&Module::readDXF,
            "readDXF(filename,[document,ignore_errors]): "
            "Imports a DXF file into the given document. "
            "ignore_errors is True by default. "
            "NOTE: DraftUtils.readDXF is removed. "
            "Use Import.readDxf instead."
        );
        initialize("The DraftUtils module contains utility functions for the Draft module."); // register with Python
    }

private:
    Py::Object readDXF(const Py::Tuple& /*args*/)
    {
        Base::Console().Warning("DraftUtils.readDXF is removed. "
                                "Use Import.readDxf instead.\n");
        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace DraftUtils

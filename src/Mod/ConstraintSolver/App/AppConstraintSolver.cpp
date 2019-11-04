/***************************************************************************
 *   Copyright (c) YEAR YOUR NAME         <Your e-mail address>            *
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
#include <Base/PyObjectBase.h>

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include "ParameterStore.h"
#include "ParameterStorePy.h"

namespace ConstraintSolver {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("ConstraintSolver")
    {
        initialize("This module is the ConstraintSolver module."); // register with Python
    }

    virtual ~Module() {}

private:
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}


} // namespace ConstraintSolver


/* Python entry */
PyMOD_INIT_FUNC(ConstraintSolver)
{
    PyObject* mod = ConstraintSolver::initModule();

    //rename python types
    GCS::ParameterStorePy::Type.tp_name = "ConstraintSolver.ParameterStore";

    //add python types as module members
    Base::Interpreter().addType(&GCS::ParameterStorePy::Type, mod, "ParameterStore");

    //fill type system
    GCS::ParameterStore::init();

    Base::Console().Log("Loading ConstraintSolver module... done\n");
    PyMOD_Return(mod);
}

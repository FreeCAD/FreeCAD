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
#include "ParameterRef.h"
#include "ParameterRefPy.h"
#include "ParameterSubset.h"
#include "ParameterSubsetPy.h"
#include "ValueSet.h"
#include "ValueSetPy.h"
#include "ParaObject.h"
#include "ParaObjectPy.h"
#include "Constraint.h"
#include "ConstraintPy.h"
#include "SimpleConstraint.h"
#include "SimpleConstraintPy.h"
#include "ParaGeometry.h"
#include "ParaGeometryPy.h"

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
    FCS::ParameterStorePy  ::Type.tp_name = "ConstraintSolver.ParameterStore"  ;
    FCS::ParameterRefPy    ::Type.tp_name = "ConstraintSolver.ParameterRef"    ;
    FCS::ParameterSubsetPy ::Type.tp_name = "ConstraintSolver.ParameterSubset" ;
    FCS::ValueSetPy        ::Type.tp_name = "ConstraintSolver.ValueSet"        ;
    FCS::ParaObjectPy      ::Type.tp_name = "ConstraintSolver.ParaObject"      ;
    FCS::ConstraintPy      ::Type.tp_name = "ConstraintSolver.Constraint"      ;
    FCS::SimpleConstraintPy::Type.tp_name = "ConstraintSolver.SimpleConstraint";
    FCS::ParaGeometryPy    ::Type.tp_name = "ConstraintSolver.ParaGeometry"    ;

    //add python types as module members
    Base::Interpreter().addType(&FCS::ParameterStorePy  ::Type, mod, "ParameterStore"  );
    Base::Interpreter().addType(&FCS::ParameterRefPy    ::Type, mod, "ParameterRef"    );
    Base::Interpreter().addType(&FCS::ParameterSubsetPy ::Type, mod, "ParameterSubset" );
    Base::Interpreter().addType(&FCS::ValueSetPy        ::Type, mod, "ValueSet"        );
    Base::Interpreter().addType(&FCS::ParaObjectPy      ::Type, mod, "ParaObject"      );
    Base::Interpreter().addType(&FCS::ConstraintPy      ::Type, mod, "Constraint"      );
    Base::Interpreter().addType(&FCS::SimpleConstraintPy::Type, mod, "SimpleConstraint");
    Base::Interpreter().addType(&FCS::ParaGeometryPy    ::Type, mod, "ParaGeometry"    );

    //fill type system
    FCS::ParameterStore   ::init();
    FCS::ParaObject       ::init();
    FCS::Constraint       ::init();
    FCS::SimpleConstraint ::init();

    Base::Console().Log("Loading ConstraintSolver module... done\n");
    PyMOD_Return(mod);
}

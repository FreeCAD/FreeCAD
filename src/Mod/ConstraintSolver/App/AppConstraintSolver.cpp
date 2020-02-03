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

#include "PyUtils.h"

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
#include "SubSystem.h"
#include "SubSystemPy.h"
#include "SolverBackend.h"
#include "SolverBackendPy.h"
#include "DogLeg.h"
#include "LM.h"
#include "BFGS.h"
#include "SQP.h"

#include "G2D/VectorPy.h"
#include "G2D/ParaGeometry2D.h"
#include "G2D/ParaGeometry2DPy.h"
#include "G2D/ParaPoint.h"
#include "G2D/ParaPointPy.h"
#include "G2D/ParaVector.h"
#include "G2D/ParaVectorPy.h"
#include "G2D/ParaPlacement.h"
#include "G2D/ParaPlacementPy.h"
#include "G2D/ParaTransform.h"
#include "G2D/ParaTransformPy.h"
#include "G2D/ParaShape.h"
#include "G2D/ParaShapePy.h"
#include "G2D/ParaCurve.h"
#include "G2D/ParaCurvePy.h"
#include "G2D/ParaLine.h"
#include "G2D/ParaLinePy.h"
#include "G2D/ParaCircle.h"
#include "G2D/ParaCirclePy.h"
#include "G2D/ParaConic.h"
#include "G2D/ParaConicPy.h"
#include "G2D/ParaParabola.h"
#include "G2D/ParaParabolaPy.h"
#include "G2D/ParaBSplineBase.h"
#include "G2D/ParaBSplineBasePy.h"
#include "G2D/ConstraintCurvePos.h"
#include "G2D/ConstraintCurvePosPy.h"
#include "G2D/ConstraintDistance.h"
#include "G2D/ConstraintDistancePy.h"
#include "G2D/ConstraintPlacementRules.h"
#include "G2D/ConstraintPlacementRulesPy.h"

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


//submodule "G2D"
PyDoc_STRVAR(G2D_doc,
    "2D geometry and constraints"
);
// This is called via the PyImport_AppendInittab mechanism called
// during initialization, to make the built-in __FreeCADBase__
// module known to Python.
//#FIXME: Py2 :
PyMODINIT_FUNC
init_freecad_FCS_G2D_module(void)
{
    static struct PyModuleDef moduleDef = {
        PyModuleDef_HEAD_INIT,
        "__FreeCAD_FCS_G2D__", G2D_doc, -1,
        NULL, NULL, NULL, NULL, NULL
    };
    return PyModule_Create(&moduleDef);
}

/**
 * @brief makeSubmodule: creates a Py module, and assigns it as an attribute of parent module
 * @param parentMod: parent module
 * @param internal_module_name: well, I don't know, FreeCAD.Base has "__FreeCADBase__" as its internal name
 * @param attrname: the name of the attribute of parent module to refer to the new submodule
 * @param initfunc: submodule init func, init_freecad_FCS_G2D_module for example.
 * @return the submodule.
 */
inline Py::Module makeSubmodule(PyObject* parentMod, const char* internal_module_name, const char* attrname, PyObject* (*initfunc)())
{
    //#FIXME: Py2
    PyObject* modules = PyImport_GetModuleDict();
    PyObject* mod = PyImport_ImportModule (internal_module_name);
    if (!mod) {
        PyErr_Clear();
        mod = initfunc();
        PyDict_SetItemString(modules, internal_module_name, mod);
    }
    PyModule_AddObject(parentMod, attrname, Py::new_reference_to(mod));
    return Py::Module(mod, true);
}



/* Python entry */
PyMOD_INIT_FUNC(ConstraintSolver)
{
    PY_TRY{
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
    FCS::SubSystemPy       ::Type.tp_name = "ConstraintSolver.SubSystem"       ;
    FCS::SolverBackendPy   ::Type.tp_name = "ConstraintSolver.SolverBackend"   ;

    //add python types as module members
  //Base::Interpreter().addType(&FCS::                  ::Type, mod, ""                );
    Base::Interpreter().addType(&FCS::ParameterStorePy  ::Type, mod, "ParameterStore"  );
    Base::Interpreter().addType(&FCS::ParameterRefPy    ::Type, mod, "ParameterRef"    );
    Base::Interpreter().addType(&FCS::ParameterSubsetPy ::Type, mod, "ParameterSubset" );
    Base::Interpreter().addType(&FCS::ValueSetPy        ::Type, mod, "ValueSet"        );
    Base::Interpreter().addType(&FCS::ParaObjectPy      ::Type, mod, "ParaObject"      );
    Base::Interpreter().addType(&FCS::ConstraintPy      ::Type, mod, "Constraint"      );
    Base::Interpreter().addType(&FCS::SimpleConstraintPy::Type, mod, "SimpleConstraint");
    Base::Interpreter().addType(&FCS::ParaGeometryPy    ::Type, mod, "ParaGeometry"    );
    Base::Interpreter().addType(&FCS::SubSystemPy       ::Type, mod, "SubSystem"       );
    Base::Interpreter().addType(&FCS::SolverBackendPy   ::Type, mod, "SolverBackend"   );


    Py::Module submodG2D =  makeSubmodule(mod, "__FreeCAD_FCS_G2D__", "G2D", init_freecad_FCS_G2D_module);


    FCS::G2D::VectorPy                   ::Type.tp_name = "ConstraintSolver.G2D.Vector"                   ;
    FCS::G2D::ParaGeometry2DPy           ::Type.tp_name = "ConstraintSolver.G2D.ParaGeometry2D"           ;
    FCS::G2D::ParaPointPy                ::Type.tp_name = "ConstraintSolver.G2D.ParaPoint"                ;
    FCS::G2D::ParaVectorPy               ::Type.tp_name = "ConstraintSolver.G2D.ParaVector"               ;
    FCS::G2D::ParaPlacementPy            ::Type.tp_name = "ConstraintSolver.G2D.ParaPlacement"            ;
    FCS::G2D::ParaTransformPy            ::Type.tp_name = "ConstraintSolver.G2D.ParaTransform"            ;
    FCS::G2D::ParaShapePy                ::Type.tp_name = "ConstraintSolver.G2D.ParaShape"                ;
    FCS::G2D::ParaCurvePy                ::Type.tp_name = "ConstraintSolver.G2D.ParaCurve"                ;
    FCS::G2D::ParaLinePy                 ::Type.tp_name = "ConstraintSolver.G2D.ParaLine"                 ;
    FCS::G2D::ParaCirclePy               ::Type.tp_name = "ConstraintSolver.G2D.ParaCircle"               ;
    FCS::G2D::ParaConicPy                ::Type.tp_name = "ConstraintSolver.G2D.ParaConic"                ;
    FCS::G2D::ParaParabolaPy             ::Type.tp_name = "ConstraintSolver.G2D.ParaParabola"             ;
    FCS::G2D::ParaBSplineBasePy          ::Type.tp_name = "ConstraintSolver.G2D.ParaBSplineBase"          ;
    FCS::G2D::ConstraintCurvePosPy       ::Type.tp_name = "ConstraintSolver.G2D.ConstraintCurvePos"       ;
    FCS::G2D::ConstraintDistancePy       ::Type.tp_name = "ConstraintSolver.G2D.ConstraintDistance"       ;
    FCS::G2D::ConstraintPlacementRulesPy ::Type.tp_name = "ConstraintSolver.G2D.ConstraintPlacementRules" ;

    Base::Interpreter().addType(&FCS::G2D::VectorPy                   ::Type, submodG2D.ptr(), "Vector"                   );
    Base::Interpreter().addType(&FCS::G2D::ParaGeometry2DPy           ::Type, submodG2D.ptr(), "ParaGeometry2D"           );
    Base::Interpreter().addType(&FCS::G2D::ParaPointPy                ::Type, submodG2D.ptr(), "ParaPoint"                );
    Base::Interpreter().addType(&FCS::G2D::ParaVectorPy               ::Type, submodG2D.ptr(), "ParaVector"               );
    Base::Interpreter().addType(&FCS::G2D::ParaPlacementPy            ::Type, submodG2D.ptr(), "ParaPlacement"            );
    Base::Interpreter().addType(&FCS::G2D::ParaTransformPy            ::Type, submodG2D.ptr(), "ParaTransform"            );
    Base::Interpreter().addType(&FCS::G2D::ParaShapePy                ::Type, submodG2D.ptr(), "ParaShape"                );
    Base::Interpreter().addType(&FCS::G2D::ParaCurvePy                ::Type, submodG2D.ptr(), "ParaCurve"                );
    Base::Interpreter().addType(&FCS::G2D::ParaLinePy                 ::Type, submodG2D.ptr(), "ParaLine"                 );
    Base::Interpreter().addType(&FCS::G2D::ParaCirclePy               ::Type, submodG2D.ptr(), "ParaCircle"               );
    Base::Interpreter().addType(&FCS::G2D::ParaConicPy                ::Type, submodG2D.ptr(), "ParaConic"                );
    Base::Interpreter().addType(&FCS::G2D::ParaParabolaPy             ::Type, submodG2D.ptr(), "ParaParabola"             );
    Base::Interpreter().addType(&FCS::G2D::ParaBSplineBasePy          ::Type, submodG2D.ptr(), "ParaBSplineBase"          );
    Base::Interpreter().addType(&FCS::G2D::ConstraintCurvePosPy       ::Type, submodG2D.ptr(), "ConstraintCurvePos"       );
    Base::Interpreter().addType(&FCS::G2D::ConstraintDistancePy       ::Type, submodG2D.ptr(), "ConstraintDistance"       );
    Base::Interpreter().addType(&FCS::G2D::ConstraintPlacementRulesPy ::Type, submodG2D.ptr(), "ConstraintPlacementRules" );
  //Base::Interpreter().addType(&FCS::G2D::                           ::Type, submodG2D.ptr(), ""                         );

    //fill type system
    FCS::ParameterStore     ::init();
    FCS::ParaObject         ::init();
    FCS::Constraint         ::init();
    FCS::SimpleConstraint   ::init();
    FCS::ParaGeometry       ::init();
    FCS::SubSystem          ::init();
    FCS::SolverBackend      ::init();
    FCS::DogLeg             ::init();
    FCS::LM                 ::init();
    FCS::BFGS               ::init();
    FCS::SQP                ::init();


    FCS::G2D::ParaGeometry2D           ::init();
    FCS::G2D::ParaPoint                ::init();
    FCS::G2D::ParaVector               ::init();
    FCS::G2D::ParaPlacement            ::init();
    FCS::G2D::ParaTransform            ::init();
    FCS::G2D::ParaShapeBase            ::init();
    FCS::G2D::ParaCurve                ::init();
    FCS::G2D::ParaLine                 ::init();
    FCS::G2D::ParaCircle               ::init();
    FCS::G2D::ParaConic                ::init();
    FCS::G2D::ParaParabola             ::init();
    FCS::G2D::ParaBSplineBase          ::init();
    FCS::G2D::ConstraintCurvePos       ::init();
    FCS::G2D::ConstraintDistance       ::init();
    FCS::G2D::ConstraintPlacementRules ::init();


    { //import methods from ConstraintSolverPartGlue.py
        Py::Module modPartGlue = FCS::import("ConstraintSolverPartGlue");

        Py::List exports = modPartGlue.getAttr("exports");
        for (Py::Object it : exports){
            std::string attrname = Py::String(it);
            Py::Module(mod).setAttr(attrname, modPartGlue.getAttr(attrname));
        }
    }

    Base::Console().Log("Loading ConstraintSolver module... done\n");
    PyMOD_Return(mod);
    } PY_CATCH;
}

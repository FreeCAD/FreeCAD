/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
#include <CXX/Extensions.hxx>

#include "FemMeshPy.h"
#include "FemMesh.h"
#include "FemMeshProperty.h"
#include "FemAnalysis.h"
#include "FemMeshObject.h"
#include "FemMeshShapeObject.h"
#include "FemMeshShapeNetgenObject.h"

#include "FemSetElementsObject.h"
#include "FemSetFacesObject.h"
#include "FemSetGeometryObject.h"
#include "FemSetNodesObject.h"

#include "HypothesisPy.h"
#include "FemConstraintBearing.h"
#include "FemConstraintFixed.h"
#include "FemConstraintForce.h"
#include "FemConstraintGear.h"
#include "FemConstraintPulley.h"

#include "FemResultObject.h"
#include "FemResultValue.h"
#include "FemResultVector.h"

extern struct PyMethodDef Fem_methods[];

PyDoc_STRVAR(module_Fem_doc,
"This module is the FEM module.");

/* Python entry */
extern "C" {
void AppFemExport initFem()
{
    // load dependend module
    try {
        Base::Interpreter().loadModule("Part");
        //Base::Interpreter().loadModule("Mesh");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }
    PyObject* femModule = Py_InitModule3("Fem", Fem_methods, module_Fem_doc);   /* mod name, table ptr */
    Base::Console().Log("Loading Fem module... done\n");

    Fem::StdMeshers_Arithmetic1DPy              ::init_type(femModule);
    Fem::StdMeshers_AutomaticLengthPy           ::init_type(femModule);
    Fem::StdMeshers_NotConformAllowedPy         ::init_type(femModule);
    Fem::StdMeshers_MaxLengthPy                 ::init_type(femModule);
    Fem::StdMeshers_LocalLengthPy               ::init_type(femModule);
    Fem::StdMeshers_QuadranglePreferencePy      ::init_type(femModule);
    Fem::StdMeshers_Quadrangle_2DPy             ::init_type(femModule);
    Fem::StdMeshers_MaxElementAreaPy            ::init_type(femModule);
    Fem::StdMeshers_Regular_1DPy                ::init_type(femModule);
    Fem::StdMeshers_UseExisting_1DPy            ::init_type(femModule);
    Fem::StdMeshers_UseExisting_2DPy            ::init_type(femModule);
    Fem::StdMeshers_CompositeSegment_1DPy       ::init_type(femModule);
    Fem::StdMeshers_Deflection1DPy              ::init_type(femModule);
    Fem::StdMeshers_LayerDistributionPy         ::init_type(femModule);
    Fem::StdMeshers_LengthFromEdgesPy           ::init_type(femModule);
    Fem::StdMeshers_MaxElementVolumePy          ::init_type(femModule);
    Fem::StdMeshers_MEFISTO_2DPy                ::init_type(femModule);
    Fem::StdMeshers_NumberOfLayersPy            ::init_type(femModule);
    Fem::StdMeshers_NumberOfSegmentsPy          ::init_type(femModule);
    Fem::StdMeshers_Prism_3DPy                  ::init_type(femModule);
    Fem::StdMeshers_Projection_1DPy             ::init_type(femModule);
    Fem::StdMeshers_Projection_2DPy             ::init_type(femModule);
    Fem::StdMeshers_Projection_3DPy             ::init_type(femModule);
    Fem::StdMeshers_ProjectionSource1DPy        ::init_type(femModule);
    Fem::StdMeshers_ProjectionSource2DPy        ::init_type(femModule);
    Fem::StdMeshers_ProjectionSource3DPy        ::init_type(femModule);
    Fem::StdMeshers_QuadraticMeshPy             ::init_type(femModule);
    Fem::StdMeshers_RadialPrism_3DPy            ::init_type(femModule);
    Fem::StdMeshers_SegmentAroundVertex_0DPy    ::init_type(femModule);
    Fem::StdMeshers_SegmentLengthAroundVertexPy ::init_type(femModule);
    Fem::StdMeshers_StartEndLengthPy            ::init_type(femModule);
    Fem::StdMeshers_TrianglePreferencePy        ::init_type(femModule);
    Fem::StdMeshers_Hexa_3DPy                   ::init_type(femModule);

    // Add Types to module
    Base::Interpreter().addType(&Fem::FemMeshPy::Type,femModule,"FemMesh");


    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
 
    Fem::FemAnalysis                ::init();
    Fem::FemAnalysisPython          ::init();
    Fem::FemMesh                    ::init();
    Fem::FemMeshObject              ::init();
    Fem::FemMeshShapeObject         ::init();
    Fem::FemMeshShapeNetgenObject   ::init();
    Fem::PropertyFemMesh            ::init();

    Fem::FemSetObject               ::init();
    Fem::FemSetElementsObject       ::init();
    Fem::FemSetFacesObject          ::init();
    Fem::FemSetGeometryObject       ::init();
    Fem::FemSetNodesObject          ::init();

    Fem::Constraint                 ::init();
    Fem::ConstraintBearing          ::init();
    Fem::ConstraintFixed            ::init();
    Fem::ConstraintForce            ::init();
    Fem::ConstraintGear             ::init();
    Fem::ConstraintPulley           ::init();

    Fem::FemResultObject            ::init();
    Fem::FemResultPython            ::init();
    Fem::FemResultValue             ::init();
    Fem::FemResultValuePython       ::init();
    Fem::FemResultVector            ::init();
    Fem::FemResultVectorPython      ::init();
}

} // extern "C"

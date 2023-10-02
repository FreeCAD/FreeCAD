/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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
#include <SMESH_Version.h>

#ifndef _PreComp_
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepTools.hxx>
#include <Python.h>
#include <SMESH_Gen.hxx>
#include <SMESH_Mesh.hxx>
#include <StdMeshers_Deflection1D.hxx>
#include <StdMeshers_Hexa_3D.hxx>
#include <StdMeshers_LocalLength.hxx>
#include <StdMeshers_MaxElementArea.hxx>
#include <StdMeshers_MaxLength.hxx>
#include <StdMeshers_NumberOfSegments.hxx>
#include <StdMeshers_ProjectionSource1D.hxx>
#include <StdMeshers_ProjectionSource2D.hxx>
#include <StdMeshers_ProjectionSource3D.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
#include <StdMeshers_Quadrangle_2D.hxx>
#include <StdMeshers_RadialPrism_3D.hxx>
#include <StdMeshers_Regular_1D.hxx>
#include <StdMeshers_SegmentAroundVertex_0D.hxx>
#include <StdMeshers_StartEndLength.hxx>
#endif

#include <App/DocumentObjectPy.h>
#include <Mod/Part/App/PartFeature.h>

#include "FemMesh.h"
#include "FemMeshShapeObject.h"


using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemMeshShapeObject, Fem::FemMeshObject)


FemMeshShapeObject::FemMeshShapeObject()
{
    ADD_PROPERTY_TYPE(
        Shape,
        (nullptr),
        "FEM Mesh",
        Prop_None,
        "Geometry object, the mesh is made from. The geometry object has to have a Shape.");
}

FemMeshShapeObject::~FemMeshShapeObject() = default;

App::DocumentObjectExecReturn* FemMeshShapeObject::execute()
{
    Fem::FemMesh newMesh;

    Part::Feature* feat = Shape.getValue<Part::Feature*>();

#if 0
    TopoDS_Shape oshape = feat->Shape.getValue();
    BRepBuilderAPI_Copy copy(oshape);
    const TopoDS_Shape& shape = copy.Shape();
    BRepTools::Clean(shape); // remove triangulation
#else
    TopoDS_Shape shape = feat->Shape.getValue();
#endif

    newMesh.getSMesh()->ShapeToMesh(shape);
    SMESH_Gen* myGen = newMesh.getGenerator();

    int hyp = 0;
#if 0
    SMESH_HypothesisPtr len(new StdMeshers_MaxLength(hyp++, 1, myGen));
    static_cast<StdMeshers_MaxLength*>(len.get())->SetLength(1.0);
    newMesh.addHypothesis(shape, len);

    SMESH_HypothesisPtr loc(new StdMeshers_LocalLength(hyp++, 1, myGen));
    static_cast<StdMeshers_LocalLength*>(loc.get())->SetLength(1.0);
    newMesh.addHypothesis(shape, loc);

    SMESH_HypothesisPtr area(new StdMeshers_MaxElementArea(hyp++, 1, myGen));
    static_cast<StdMeshers_MaxElementArea*>(area.get())->SetMaxArea(1.0);
    newMesh.addHypothesis(shape, area);

    SMESH_HypothesisPtr segm(new StdMeshers_NumberOfSegments(hyp++, 1, myGen));
    static_cast<StdMeshers_NumberOfSegments*>(segm.get())->SetNumberOfSegments(1);
    newMesh.addHypothesis(shape, segm);

    SMESH_HypothesisPtr defl(new StdMeshers_Deflection1D(hyp++, 1, myGen));
    static_cast<StdMeshers_Deflection1D*>(defl.get())->SetDeflection(0.01);
    newMesh.addHypothesis(shape, defl);

    SMESH_HypothesisPtr reg(new StdMeshers_Regular_1D(hyp++, 1, myGen));
    newMesh.addHypothesis(shape, reg);

    //SMESH_HypothesisPtr sel(new StdMeshers_StartEndLength(hyp++, 1, myGen));
    //static_cast<StdMeshers_StartEndLength*>(sel.get())->SetLength(1.0, true);
    //newMesh.addHypothesis(shape, sel;

    SMESH_HypothesisPtr qdp(new StdMeshers_QuadranglePreference(hyp++,1,myGen));
    newMesh.addHypothesis(shape, qdp);

    //SMESH_HypothesisPtr q2d(new StdMeshers_Quadrangle_2D(hyp++,1,myGen));
    //newMesh.addHypothesis(shape, q2d);

    SMESH_HypothesisPtr h3d(new StdMeshers_Hexa_3D(hyp++,1,myGen));
    newMesh.addHypothesis(shape, h3d);

    // create mesh
    newMesh.compute();
#endif
#if 1  // Surface quad mesh
#if SMESH_VERSION_MAJOR >= 9
    SMESH_HypothesisPtr len(new StdMeshers_MaxLength(hyp++, myGen));
    static_cast<StdMeshers_MaxLength*>(len.get())->SetLength(1.0);
    newMesh.addHypothesis(shape, len);

    SMESH_HypothesisPtr loc(new StdMeshers_LocalLength(hyp++, myGen));
    static_cast<StdMeshers_LocalLength*>(loc.get())->SetLength(1.0);
    newMesh.addHypothesis(shape, loc);

    SMESH_HypothesisPtr area(new StdMeshers_MaxElementArea(hyp++, myGen));
    static_cast<StdMeshers_MaxElementArea*>(area.get())->SetMaxArea(1.0);
    newMesh.addHypothesis(shape, area);

    SMESH_HypothesisPtr segm(new StdMeshers_NumberOfSegments(hyp++, myGen));
    static_cast<StdMeshers_NumberOfSegments*>(segm.get())->SetNumberOfSegments(1);
    newMesh.addHypothesis(shape, segm);

    SMESH_HypothesisPtr defl(new StdMeshers_Deflection1D(hyp++, myGen));
    static_cast<StdMeshers_Deflection1D*>(defl.get())->SetDeflection(0.01);
    newMesh.addHypothesis(shape, defl);

    SMESH_HypothesisPtr reg(new StdMeshers_Regular_1D(hyp++, myGen));
    newMesh.addHypothesis(shape, reg);

    SMESH_HypothesisPtr qdp(new StdMeshers_QuadranglePreference(hyp++, myGen));
    newMesh.addHypothesis(shape, qdp);

    SMESH_HypothesisPtr q2d(new StdMeshers_Quadrangle_2D(hyp++, myGen));
    newMesh.addHypothesis(shape, q2d);
#else
    SMESH_HypothesisPtr len(new StdMeshers_MaxLength(hyp++, 1, myGen));
    static_cast<StdMeshers_MaxLength*>(len.get())->SetLength(1.0);
    newMesh.addHypothesis(shape, len);

    SMESH_HypothesisPtr loc(new StdMeshers_LocalLength(hyp++, 1, myGen));
    static_cast<StdMeshers_LocalLength*>(loc.get())->SetLength(1.0);
    newMesh.addHypothesis(shape, loc);

    SMESH_HypothesisPtr area(new StdMeshers_MaxElementArea(hyp++, 1, myGen));
    static_cast<StdMeshers_MaxElementArea*>(area.get())->SetMaxArea(1.0);
    newMesh.addHypothesis(shape, area);

    SMESH_HypothesisPtr segm(new StdMeshers_NumberOfSegments(hyp++, 1, myGen));
    static_cast<StdMeshers_NumberOfSegments*>(segm.get())->SetNumberOfSegments(1);
    newMesh.addHypothesis(shape, segm);

    SMESH_HypothesisPtr defl(new StdMeshers_Deflection1D(hyp++, 1, myGen));
    static_cast<StdMeshers_Deflection1D*>(defl.get())->SetDeflection(0.01);
    newMesh.addHypothesis(shape, defl);

    SMESH_HypothesisPtr reg(new StdMeshers_Regular_1D(hyp++, 1, myGen));
    newMesh.addHypothesis(shape, reg);

    // SMESH_HypothesisPtr sel(new StdMeshers_StartEndLength(hyp++, 1, myGen));
    // static_cast<StdMeshers_StartEndLength*>(sel.get())->SetLength(1.0, true);
    // newMesh.addHypothesis(shape, sel;

    SMESH_HypothesisPtr qdp(new StdMeshers_QuadranglePreference(hyp++, 1, myGen));
    newMesh.addHypothesis(shape, qdp);

    SMESH_HypothesisPtr q2d(new StdMeshers_Quadrangle_2D(hyp++, 1, myGen));
    newMesh.addHypothesis(shape, q2d);
#endif

    // create mesh
    newMesh.compute();
#endif
#if 0  // NETGEN test
    NETGENPlugin_Mesher myNetGenMesher(newMesh.getSMesh(),shape,true);

    //NETGENPlugin_SimpleHypothesis_2D * tet2 = new NETGENPlugin_SimpleHypothesis_2D(hyp++,1,myGen);
    //static_cast<NETGENPlugin_SimpleHypothesis_2D*>(tet2.get())->SetNumberOfSegments(5);
    //static_cast<NETGENPlugin_SimpleHypothesis_2D*>(tet2.get())->SetLocalLength(0.1);
    //static_cast<NETGENPlugin_SimpleHypothesis_2D*>(tet2.get())->LengthFromEdges();
    //myNetGenMesher.SetParameters(tet2);

    //NETGENPlugin_SimpleHypothesis_3D* tet= new NETGENPlugin_SimpleHypothesis_3D(hyp++,1,myGen);
    //static_cast<NETGENPlugin_SimpleHypothesis_3D*>(tet.get())->LengthFromFaces();
    //static_cast<NETGENPlugin_SimpleHypothesis_3D*>(tet.get())->SetMaxElementVolume(0.1);
    //myNetGenMesher.SetParameters( tet);

    myNetGenMesher.Compute();
#endif

    // set the value to the object
    FemMesh.setValue(newMesh);


    return App::DocumentObject::StdReturn;
}

// short FemMeshShapeObject::mustExecute(void) const
//{
//     return 0;
// }

// PyObject *FemMeshShapeObject::getPyObject()
//{
//     if (PythonObject.is(Py::_None())){
//         // ref counter is set to 1
//         PythonObject = Py::Object(new DocumentObjectPy(this),true);
//     }
//     return Py::new_reference_to(PythonObject);
// }

// void FemMeshShapeObject::onChanged(const Property* prop)
//{
//     App::GeoFeature::onChanged(prop);
// }

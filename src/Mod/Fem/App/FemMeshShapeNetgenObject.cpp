/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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
#endif

#include "FemMeshShapeNetgenObject.h"
#include "FemMesh.h"
#include <App/DocumentObjectPy.h>
#include <Base/Placement.h>
#include <Mod/Part/App/PartFeature.h>
#include <Base/Console.h>

#include <SMESH_Gen.hxx>
#include <SMESH_Mesh.hxx>
#include <SMDS_PolyhedralVolumeOfNodes.hxx>
#include <SMDS_VolumeTool.hxx>

#ifdef FCWithNetgen
    #include <NETGENPlugin_SimpleHypothesis_3D.hxx>
    #include <NETGENPlugin_Hypothesis.hxx>
    #include <NETGENPlugin_Mesher.hxx>
#endif 

#include <BRepBuilderAPI_Copy.hxx>
#include <BRepTools.hxx>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemMeshShapeNetgenObject, Fem::FemMeshShapeObject)

const char* FininessEnums[]= {"VeryCoarse","Coarse","Moderate","Fine","VeryFine","UserDefined",NULL};

FemMeshShapeNetgenObject::FemMeshShapeNetgenObject()
{
    ADD_PROPERTY_TYPE(MaxSize,(1000),       "MeshParams",Prop_None,"Maximum element size");
    ADD_PROPERTY_TYPE(SecondOrder,(false),  "MeshParams",Prop_None,"Create quadric elements");
    ADD_PROPERTY_TYPE(Fininess,(2),         "MeshParams",Prop_None,"Fininess level of the mesh");
    Fininess.setEnums(FininessEnums);
    ADD_PROPERTY_TYPE(GrowthRate,(0.3),     "MeshParams",Prop_None," allows to define how much the linear dimensions of two adjacent cells can differ");
    ADD_PROPERTY_TYPE(NbSegsPerEdge,(1),    "MeshParams",Prop_None,"allows to define the minimum number of mesh segments in which edges will be split");
    ADD_PROPERTY_TYPE(NbSegsPerRadius,(2),  "MeshParams",Prop_None,"allows to define the minimum number of mesh segments in which radiuses will be split");
    ADD_PROPERTY_TYPE(Optimize,(true),      "MeshParams",Prop_None,"Shape for the analysis");

}

FemMeshShapeNetgenObject::~FemMeshShapeNetgenObject()
{
}

App::DocumentObjectExecReturn *FemMeshShapeNetgenObject::execute(void) 
{
#ifdef FCWithNetgen
    
    Fem::FemMesh newMesh;

    Part::Feature *feat = Shape.getValue<Part::Feature*>();


    TopoDS_Shape shape = feat->Shape.getValue();

    
    newMesh.getSMesh()->ShapeToMesh(shape);
    SMESH_Gen *myGen = newMesh.getGenerator();

    int hyp=0;

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

    NETGENPlugin_Hypothesis* tet= new NETGENPlugin_Hypothesis(hyp++,1,myGen);
    tet->SetMaxSize(MaxSize.getValue());    
    tet->SetSecondOrder(SecondOrder.getValue());    
    tet->SetOptimize(Optimize.getValue());   
    int iFininess = Fininess.getValue();
    tet->SetFineness((NETGENPlugin_Hypothesis::Fineness)iFininess); 
    if(iFininess == 5){
        tet->SetGrowthRate(GrowthRate.getValue());    
        tet->SetNbSegPerEdge(NbSegsPerEdge.getValue());    
        tet->SetNbSegPerRadius(NbSegsPerRadius.getValue());    
    }
    myNetGenMesher.SetParameters( tet);

    myNetGenMesher.Compute();


    SMESHDS_Mesh* data = const_cast<SMESH_Mesh*>(newMesh.getSMesh())->GetMeshDS();
	const SMDS_MeshInfo& info = data->GetMeshInfo();
	int numFaces = data->NbFaces();
    int numNode = info.NbNodes();
    int numTria = info.NbTriangles();
    int numQuad = info.NbQuadrangles();
    int numPoly = info.NbPolygons();
    int numVolu = info.NbVolumes();
    int numTetr = info.NbTetras();
    int numHexa = info.NbHexas();
    int numPyrd = info.NbPyramids();
    int numPris = info.NbPrisms();
    int numHedr = info.NbPolyhedrons();

    Base::Console().Log("NetgenMesh: %i Nodes, %i Volumes, %i Faces\n",numNode,numVolu,numFaces);

    // set the value to the object
    FemMesh.setValue(newMesh);
#endif 
    
    return App::DocumentObject::StdReturn;
}

//short FemMeshShapeNetgenObject::mustExecute(void) const
//{
//    return 0;
//}

//PyObject *FemMeshShapeNetgenObject::getPyObject()
//{
//    if (PythonObject.is(Py::_None())){
//        // ref counter is set to 1
//        PythonObject = Py::Object(new DocumentObjectPy(this),true);
//    }
//    return Py::new_reference_to(PythonObject); 
//}

//void FemMeshShapeNetgenObject::onChanged(const Property* prop)
//{
//    Fem::FemMeshShapeObject::onChanged(prop);
//}

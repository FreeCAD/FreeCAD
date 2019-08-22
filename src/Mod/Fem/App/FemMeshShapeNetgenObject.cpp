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
# include <SMESH_Gen.hxx>
# include <SMESHDS_Mesh.hxx>
# include <SMESH_Mesh.hxx>
# include <SMDS_PolyhedralVolumeOfNodes.hxx>
# include <SMDS_VolumeTool.hxx>

# include <BRepBuilderAPI_Copy.hxx>
# include <BRepTools.hxx>

# ifdef FCWithNetgen
#  include <NETGENPlugin_SimpleHypothesis_3D.hxx>
#  include <NETGENPlugin_Hypothesis.hxx>
#  include <NETGENPlugin_Mesher.hxx>
# endif

#endif

#include "FemMeshShapeNetgenObject.h"
#include "FemMesh.h"
#include <App/DocumentObjectPy.h>
#include <Base/Placement.h>
#include <Mod/Part/App/PartFeature.h>
#include <Base/Console.h>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemMeshShapeNetgenObject, Fem::FemMeshShapeObject)

const char* FinenessEnums[]= {"VeryCoarse","Coarse","Moderate","Fine","VeryFine","UserDefined",NULL};

FemMeshShapeNetgenObject::FemMeshShapeNetgenObject()
{
    ADD_PROPERTY_TYPE(MaxSize,(1000),       "MeshParams",Prop_None,"Maximum element size");
    ADD_PROPERTY_TYPE(SecondOrder,(true),   "MeshParams",Prop_None,"Create quadric elements");
    ADD_PROPERTY_TYPE(Fineness,(2),         "MeshParams",Prop_None,"Fineness level of the mesh");
    Fineness.setEnums(FinenessEnums);
    ADD_PROPERTY_TYPE(GrowthRate,(0.3),     "MeshParams",Prop_None," allows to define how much the linear dimensions of two adjacent cells can differ");
    ADD_PROPERTY_TYPE(NbSegsPerEdge,(1),    "MeshParams",Prop_None,"allows to define the minimum number of mesh segments in which edges will be split");
    ADD_PROPERTY_TYPE(NbSegsPerRadius,(2),  "MeshParams",Prop_None,"allows to define the minimum number of mesh segments in which radiuses will be split");
    ADD_PROPERTY_TYPE(Optimize,(true),      "MeshParams",Prop_None,"Optimize the resulting mesh");

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

    NETGENPlugin_Mesher myNetGenMesher(newMesh.getSMesh(),shape,true);
    NETGENPlugin_Hypothesis* tet= new NETGENPlugin_Hypothesis(0,1,newMesh.getGenerator());
    tet->SetMaxSize(MaxSize.getValue());
    tet->SetSecondOrder(SecondOrder.getValue());
    tet->SetOptimize(Optimize.getValue());
    int iFineness = Fineness.getValue();
    tet->SetFineness((NETGENPlugin_Hypothesis::Fineness)iFineness);
    if(iFineness == 5){
        tet->SetGrowthRate(GrowthRate.getValue());
        tet->SetNbSegPerEdge(NbSegsPerEdge.getValue());
        tet->SetNbSegPerRadius(NbSegsPerRadius.getValue());
    }
    myNetGenMesher.SetParameters( tet);
    newMesh.getSMesh()->ShapeToMesh(shape);

    myNetGenMesher.Compute();

    // throw Base::RuntimeError("Compute Done\n");

    SMESHDS_Mesh* data = const_cast<SMESH_Mesh*>(newMesh.getSMesh())->GetMeshDS();
    const SMDS_MeshInfo& info = data->GetMeshInfo();
    int numFaces = data->NbFaces();
    int numNode = info.NbNodes();
  //int numTria = info.NbTriangles();
  //int numQuad = info.NbQuadrangles();
  //int numPoly = info.NbPolygons();
    int numVolu = info.NbVolumes();
  //int numTetr = info.NbTetras();
  //int numHexa = info.NbHexas();
  //int numPyrd = info.NbPyramids();
  //int numPris = info.NbPrisms();
  //int numHedr = info.NbPolyhedrons();

    Base::Console().Log("NetgenMesh: %i Nodes, %i Volumes, %i Faces\n",numNode,numVolu,numFaces);

      FemMesh.setValue(newMesh);
    return App::DocumentObject::StdReturn;
#else
    return new App::DocumentObjectExecReturn("The FEM module is built without NETGEN support. Meshing will not work!!!", this);
#endif
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

// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

//  SMESH SMDS : implementation of Salome mesh data structure
//
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif

#include "SMDS_FaceOfEdges.hxx"
#include "SMDS_FaceOfNodes.hxx"
#include "SMDS_Mesh.hxx"
#include "SMDS_PolygonalFaceOfNodes.hxx"
#include "SMDS_PolyhedralVolumeOfNodes.hxx"
#include "SMDS_QuadraticEdge.hxx"
#include "SMDS_QuadraticFaceOfNodes.hxx"
#include "SMDS_QuadraticVolumeOfNodes.hxx"
#include "SMDS_SetIterator.hxx"
#include "SMDS_SpacePosition.hxx"
#include "SMDS_UnstructuredGrid.hxx"
#include "SMDS_VolumeOfFaces.hxx"
#include "SMDS_VolumeOfNodes.hxx"

#include "utilities.h"

#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridWriter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkCell.h>
#include <vtkCellLinks.h>
#include <vtkIdList.h>

#include <algorithm>
#include <map>
#include <iostream>
#include <fstream>
#include <iterator>
using namespace std;

// number of added entities to check memory after
#define CHECKMEMORY_INTERVAL 100000

vector<SMDS_Mesh*> SMDS_Mesh::_meshList = vector<SMDS_Mesh*>();
int SMDS_Mesh::chunkSize = 1024;


//================================================================================
/*!
 * \brief Raise an exception if free memory (ram+swap) too low
 * \param doNotRaise - if true, suppress exception, just return free memory size
 * \retval int - amount of available memory in MB or negative number in failure case
 */
//================================================================================

int SMDS_Mesh::CheckMemory(const bool doNotRaise)
{
    return 1000;
}

///////////////////////////////////////////////////////////////////////////////
/// Create a new mesh object
///////////////////////////////////////////////////////////////////////////////
SMDS_Mesh::SMDS_Mesh()
  :myParent(NULL),
   myNodeIDFactory(new SMDS_MeshNodeIDFactory()),
   myElementIDFactory(new SMDS_MeshElementIDFactory()),
   myHasConstructionEdges(false), myHasConstructionFaces(false),
   myHasInverseElements(true),
   myNodeMin(0), myNodeMax(0),
   myNodePool(0), myEdgePool(0), myFacePool(0), myVolumePool(0),myBallPool(0),
   myModified(false), myModifTime(0), myCompactTime(0),
   xmin(0), xmax(0), ymin(0), ymax(0), zmin(0), zmax(0)
{
  myMeshId = _meshList.size();         // --- index of the mesh to push back in the vector
  myNodeIDFactory->SetMesh(this);
  myElementIDFactory->SetMesh(this);
  _meshList.push_back(this);
  myNodePool = new ObjectPool<SMDS_MeshNode>(SMDS_Mesh::chunkSize);
  myEdgePool = new ObjectPool<SMDS_VtkEdge>(SMDS_Mesh::chunkSize);
  myFacePool = new ObjectPool<SMDS_VtkFace>(SMDS_Mesh::chunkSize);
  myVolumePool = new ObjectPool<SMDS_VtkVolume>(SMDS_Mesh::chunkSize);
  myBallPool = new ObjectPool<SMDS_BallElement>(SMDS_Mesh::chunkSize);

  myNodes.clear();
  myCells.clear();
  //myCellIdSmdsToVtk.clear();
  myCellIdVtkToSmds.clear();
  myGrid = SMDS_UnstructuredGrid::New();
  myGrid->setSMDS_mesh(this);
  myGrid->Initialize();
  myGrid->Allocate();
  vtkPoints* points = vtkPoints::New();
  // bug "21125: EDF 1233 SMESH: Degrardation of precision in a test case for quadratic conversion"
  // Use double type for storing coordinates of nodes instead of float.
  points->SetDataType(VTK_DOUBLE);
  points->SetNumberOfPoints(0 /*SMDS_Mesh::chunkSize*/);
  myGrid->SetPoints( points );
  points->Delete();
  myGrid->BuildLinks();
  this->Modified();
}

///////////////////////////////////////////////////////////////////////////////
/// Create a new child mesh
/// Note that the tree structure of SMDS_Mesh seems to be unused in this version
/// (2003-09-08) of SMESH
///////////////////////////////////////////////////////////////////////////////
SMDS_Mesh::SMDS_Mesh(SMDS_Mesh * parent)
        :myParent(parent), myNodeIDFactory(parent->myNodeIDFactory),
         myElementIDFactory(parent->myElementIDFactory),
         myHasConstructionEdges(false), myHasConstructionFaces(false),
         myHasInverseElements(true),
         myNodePool(parent->myNodePool),
         myEdgePool(parent->myEdgePool),
         myFacePool(parent->myFacePool),
         myVolumePool(parent->myVolumePool),
         myBallPool(parent->myBallPool)
{
}

///////////////////////////////////////////////////////////////////////////////
///Create a submesh and add it to the current mesh
///////////////////////////////////////////////////////////////////////////////

SMDS_Mesh *SMDS_Mesh::AddSubMesh()
{
        SMDS_Mesh *submesh = new SMDS_Mesh(this);
        myChildren.insert(myChildren.end(), submesh);
        return submesh;
}

///////////////////////////////////////////////////////////////////////////////
///create a MeshNode and add it to the current Mesh
///An ID is automatically assigned to the node.
///@return : The created node
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshNode * SMDS_Mesh::AddNode(double x, double y, double z)
{
  return SMDS_Mesh::AddNodeWithID(x,y,z,myNodeIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
///create a MeshNode and add it to the current Mesh
///@param ID : The ID of the MeshNode to create
///@return : The created node or NULL if a node with this ID already exists
///////////////////////////////////////////////////////////////////////////////
SMDS_MeshNode * SMDS_Mesh::AddNodeWithID(double x, double y, double z, int ID)
{
  // find the MeshNode corresponding to ID
  const SMDS_MeshElement *node = myNodeIDFactory->MeshElement(ID);
  if(!node){
    if (ID < 1)
      {
        MESSAGE("=============>  Bad Node Id: " << ID);
        ID = myNodeIDFactory->GetFreeID();
      }
    myNodeIDFactory->adjustMaxId(ID);
    SMDS_MeshNode * node = myNodePool->getNew();
    node->init(ID, myMeshId, 0, x, y, z);

    if (ID >= myNodes.size())
    {
        myNodes.resize(ID+SMDS_Mesh::chunkSize, 0);
//         MESSAGE(" ------------------ myNodes resize " << ID << " --> " << ID+SMDS_Mesh::chunkSize);
    }
    myNodes[ID] = node;
    myNodeIDFactory->BindID(ID,node);
    myInfo.myNbNodes++;
    myModified = true;
    this->adjustBoundingBox(x, y, z);
    return node;
  }else
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/// create a Mesh0DElement and add it to the current Mesh
/// @return : The created Mesh0DElement
///////////////////////////////////////////////////////////////////////////////
SMDS_Mesh0DElement* SMDS_Mesh::Add0DElementWithID(int idnode, int ID)
{
  SMDS_MeshNode * node = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode);
  if (!node) return NULL;
  return SMDS_Mesh::Add0DElementWithID(node, ID);
}

///////////////////////////////////////////////////////////////////////////////
/// create a Mesh0DElement and add it to the current Mesh
/// @return : The created Mesh0DElement
///////////////////////////////////////////////////////////////////////////////
SMDS_Mesh0DElement* SMDS_Mesh::Add0DElement(const SMDS_MeshNode * node)
{
  return SMDS_Mesh::Add0DElementWithID(node, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
/// Create a new Mesh0DElement and at it to the mesh
/// @param idnode ID of the node
/// @param ID ID of the 0D element to create
/// @return The created 0D element or NULL if an element with this
///         ID already exists or if input node is not found.
///////////////////////////////////////////////////////////////////////////////
SMDS_Mesh0DElement* SMDS_Mesh::Add0DElementWithID(const SMDS_MeshNode * n, int ID)
{
  if (!n) return 0;

  if (Nb0DElements() % CHECKMEMORY_INTERVAL == 0) CheckMemory();
  //MESSAGE("Add0DElementWithID" << ID)
  SMDS_Mesh0DElement * el0d = new SMDS_Mesh0DElement(n);
  if (myElementIDFactory->BindID(ID, el0d)) {
    //SMDS_MeshNode *node = const_cast<SMDS_MeshNode*>(n);
    //node->AddInverseElement(el0d);// --- fait avec BindID
    adjustmyCellsCapacity(ID);
    myCells[ID] = el0d;
    myInfo.myNb0DElements++;
    return el0d;
  }

  delete el0d;
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/// create a Ball and add it to the current Mesh
/// @return : The created Ball
///////////////////////////////////////////////////////////////////////////////
SMDS_BallElement* SMDS_Mesh::AddBallWithID(int idnode, double diameter, int ID)
{
  SMDS_MeshNode * node = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode);
  if (!node) return NULL;
  return SMDS_Mesh::AddBallWithID(node, diameter, ID);
}

///////////////////////////////////////////////////////////////////////////////
/// create a Ball and add it to the current Mesh
/// @return : The created Ball
///////////////////////////////////////////////////////////////////////////////
SMDS_BallElement* SMDS_Mesh::AddBall(const SMDS_MeshNode * node, double diameter)
{
  return SMDS_Mesh::AddBallWithID(node, diameter, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
/// Create a new Ball and at it to the mesh
/// @param idnode ID of the node
//  @param diameter ball diameter
/// @param ID ID of the 0D element to create
/// @return The created 0D element or NULL if an element with this
///         ID already exists or if input node is not found.
///////////////////////////////////////////////////////////////////////////////
SMDS_BallElement* SMDS_Mesh::AddBallWithID(const SMDS_MeshNode * n, double diameter, int ID)
{
  if (!n) return 0;

  if (NbBalls() % CHECKMEMORY_INTERVAL == 0) CheckMemory();

  SMDS_BallElement *ball = myBallPool->getNew();
  ball->init(n->getVtkId(), diameter, this);
  if (!this->registerElement(ID,ball))
  {
    this->myGrid->GetCellTypesArray()->SetValue(ball->getVtkId(), VTK_EMPTY_CELL);
    myBallPool->destroy(ball);
    return 0;
  }
  adjustmyCellsCapacity(ID);
  myCells[ID] = ball;
  myInfo.myNbBalls++;
  return ball;
}

///////////////////////////////////////////////////////////////////////////////
/// create a MeshEdge and add it to the current Mesh
/// @return : The created MeshEdge
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshEdge* SMDS_Mesh::AddEdgeWithID(int idnode1, int idnode2, int ID)
{
  SMDS_MeshNode * node1 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode1);
  SMDS_MeshNode * node2 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode2);
  if(!node1 || !node2) return NULL;
  return SMDS_Mesh::AddEdgeWithID(node1, node2, ID);
}

///////////////////////////////////////////////////////////////////////////////
/// create a MeshEdge and add it to the current Mesh
/// @return : The created MeshEdge
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshEdge* SMDS_Mesh::AddEdge(const SMDS_MeshNode * node1,
                                  const SMDS_MeshNode * node2)
{
  return SMDS_Mesh::AddEdgeWithID(node1, node2, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
/// Create a new edge and at it to the mesh
/// @param idnode1 ID of the first node
/// @param idnode2 ID of the second node
/// @param ID ID of the edge to create
/// @return The created edge or NULL if an element with this ID already exists or
/// if input nodes are not found.
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshEdge* SMDS_Mesh::AddEdgeWithID(const SMDS_MeshNode * n1,
                                        const SMDS_MeshNode * n2,
                                        int ID)
{
  if ( !n1 || !n2 ) return 0;
  SMDS_MeshEdge * edge = 0;

  // --- retrieve nodes ID
  vector<vtkIdType> nodeIds;
  nodeIds.clear();
  nodeIds.push_back(n1->getVtkId());
  nodeIds.push_back(n2->getVtkId());

  SMDS_VtkEdge *edgevtk = myEdgePool->getNew();
  edgevtk->init(nodeIds, this);
  if (!this->registerElement(ID,edgevtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(edgevtk->getVtkId(), VTK_EMPTY_CELL);
      myEdgePool->destroy(edgevtk);
      return 0;
    }
  edge = edgevtk;
  adjustmyCellsCapacity(ID);
  myCells[ID] = edge;
  myInfo.myNbEdges++;

//  if (edge && !registerElement(ID, edge))
//  {
//    RemoveElement(edge, false);
//    edge = NULL;
//  }
  return edge;
}

///////////////////////////////////////////////////////////////////////////////
/// Add a triangle defined by its nodes. An ID is automatically affected to the
/// Created face
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddFace(const SMDS_MeshNode * n1,
                                  const SMDS_MeshNode * n2,
                                  const SMDS_MeshNode * n3)
{
  return SMDS_Mesh::AddFaceWithID(n1,n2,n3, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
/// Add a triangle defined by its nodes IDs
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(int idnode1, int idnode2, int idnode3, int ID)
{
  SMDS_MeshNode * node1 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode1);
  SMDS_MeshNode * node2 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode2);
  SMDS_MeshNode * node3 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode3);
  if(!node1 || !node2 || !node3) return NULL;
  return SMDS_Mesh::AddFaceWithID(node1, node2, node3, ID);
}

///////////////////////////////////////////////////////////////////////////////
/// Add a triangle defined by its nodes
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(const SMDS_MeshNode * n1,
                                        const SMDS_MeshNode * n2,
                                        const SMDS_MeshNode * n3,
                                        int ID)
{
  //MESSAGE("AddFaceWithID " << ID)
  SMDS_MeshFace * face=createTriangle(n1, n2, n3, ID);

//  if (face && !registerElement(ID, face)) {
//    RemoveElement(face, false);
//    face = NULL;
//  }
  return face;
}

///////////////////////////////////////////////////////////////////////////////
/// Add a quadrangle defined by its nodes. An ID is automatically affected to the
/// created face
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddFace(const SMDS_MeshNode * n1,
                                  const SMDS_MeshNode * n2,
                                  const SMDS_MeshNode * n3,
                                  const SMDS_MeshNode * n4)
{
  return SMDS_Mesh::AddFaceWithID(n1,n2,n3, n4, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
/// Add a quadrangle defined by its nodes IDs
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(int idnode1,
                                        int idnode2,
                                        int idnode3,
                                        int idnode4,
                                        int ID)
{
  SMDS_MeshNode *node1, *node2, *node3, *node4;
  node1 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode1);
  node2 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode2);
  node3 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode3);
  node4 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode4);
  if(!node1 || !node2 || !node3 || !node4) return NULL;
  return SMDS_Mesh::AddFaceWithID(node1, node2, node3, node4, ID);
}

///////////////////////////////////////////////////////////////////////////////
/// Add a quadrangle defined by its nodes
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(const SMDS_MeshNode * n1,
                                        const SMDS_MeshNode * n2,
                                        const SMDS_MeshNode * n3,
                                        const SMDS_MeshNode * n4,
                                        int ID)
{
  //MESSAGE("AddFaceWithID " << ID);
  SMDS_MeshFace * face=createQuadrangle(n1, n2, n3, n4, ID);

//  if (face && !registerElement(ID, face)) {
//    RemoveElement(face, false);
//    face = NULL;
//  }
  return face;
}

///////////////////////////////////////////////////////////////////////////////
/// Add a triangle defined by its edges. An ID is automatically assigned to the
/// Created face
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddFace(const SMDS_MeshEdge * e1,
                                  const SMDS_MeshEdge * e2,
                                  const SMDS_MeshEdge * e3)
{
  if (!hasConstructionEdges())
    return NULL;
     //MESSAGE("AddFaceWithID");
 return AddFaceWithID(e1,e2,e3, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
/// Add a triangle defined by its edges
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(const SMDS_MeshEdge * e1,
                                        const SMDS_MeshEdge * e2,
                                        const SMDS_MeshEdge * e3,
                                        int ID)
{
  if (!hasConstructionEdges())
    return NULL;
  if ( !e1 || !e2 || !e3 ) return 0;

  if ( NbFaces() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  MESSAGE("AddFaceWithID" << ID);

  SMDS_MeshFace * face = new SMDS_FaceOfEdges(e1,e2,e3);
  adjustmyCellsCapacity(ID);
  myCells[ID] = face;
  myInfo.myNbTriangles++;

  if (!registerElement(ID, face)) {
    registerElement(myElementIDFactory->GetFreeID(), face);
    //RemoveElement(face, false);
    //face = NULL;
  }
  return face;
}

///////////////////////////////////////////////////////////////////////////////
/// Add a quadrangle defined by its edges. An ID is automatically assigned to the
/// Created face
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddFace(const SMDS_MeshEdge * e1,
                                  const SMDS_MeshEdge * e2,
                                  const SMDS_MeshEdge * e3,
                                  const SMDS_MeshEdge * e4)
{
  if (!hasConstructionEdges())
    return NULL;
     //MESSAGE("AddFaceWithID" );
 return AddFaceWithID(e1,e2,e3,e4, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
/// Add a quadrangle defined by its edges
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(const SMDS_MeshEdge * e1,
                                        const SMDS_MeshEdge * e2,
                                        const SMDS_MeshEdge * e3,
                                        const SMDS_MeshEdge * e4,
                                        int ID)
{
  if (!hasConstructionEdges())
    return NULL;
  MESSAGE("AddFaceWithID" << ID);
  if ( !e1 || !e2 || !e3 || !e4 ) return 0;
  if ( NbFaces() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  SMDS_MeshFace * face = new SMDS_FaceOfEdges(e1,e2,e3,e4);
  adjustmyCellsCapacity(ID);
  myCells[ID] = face;
  myInfo.myNbQuadrangles++;

  if (!registerElement(ID, face))
  {
    registerElement(myElementIDFactory->GetFreeID(), face);
    //RemoveElement(face, false);
    //face = NULL;
  }
  return face;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new tetrahedron and add it to the mesh.
///@return The created tetrahedron
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                      const SMDS_MeshNode * n2,
                                      const SMDS_MeshNode * n3,
                                      const SMDS_MeshNode * n4)
{
  int ID = myElementIDFactory->GetFreeID();
    //MESSAGE("AddVolumeWithID " << ID);
  SMDS_MeshVolume * v = SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, ID);
  if(v==NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new tetrahedron and add it to the mesh.
///@param ID The ID of the new volume
///@return The created tetrahedron or NULL if an element with this ID already exists
///or if input nodes are not found.
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume * SMDS_Mesh::AddVolumeWithID(int idnode1,
                                             int idnode2,
                                             int idnode3,
                                             int idnode4,
                                             int ID)
{
    //MESSAGE("AddVolumeWithID" << ID);
  SMDS_MeshNode *node1, *node2, *node3, *node4;
  node1 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode1);
  node2 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode2);
  node3 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode3);
  node4 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode4);
  if(!node1 || !node2 || !node3 || !node4) return NULL;
  return SMDS_Mesh::AddVolumeWithID(node1, node2, node3, node4, ID);
}

///////////////////////////////////////////////////////////////////////////////
///Create a new tetrahedron and add it to the mesh.
///@param ID The ID of the new volume
///@return The created tetrahedron
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                            const SMDS_MeshNode * n2,
                                            const SMDS_MeshNode * n3,
                                            const SMDS_MeshNode * n4,
                                            int ID)
{
  //MESSAGE("AddVolumeWithID " << ID);
  SMDS_MeshVolume* volume = 0;
  if ( !n1 || !n2 || !n3 || !n4) return volume;
  if ( NbVolumes() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionFaces()) {
    SMDS_MeshFace * f1=FindFaceOrCreate(n1,n2,n3);
    SMDS_MeshFace * f2=FindFaceOrCreate(n1,n2,n4);
    SMDS_MeshFace * f3=FindFaceOrCreate(n1,n3,n4);
    SMDS_MeshFace * f4=FindFaceOrCreate(n2,n3,n4);
    volume=new SMDS_VolumeOfFaces(f1,f2,f3,f4);
    adjustmyCellsCapacity(ID);
    myCells[ID] = volume;
    myInfo.myNbTetras++;
  }
  else if(hasConstructionEdges()) {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else {
    // --- retrieve nodes ID
    myNodeIds.resize(4);
    myNodeIds[0] = n1->getVtkId();
    myNodeIds[1] = n3->getVtkId(); // order SMDS-->VTK
    myNodeIds[2] = n2->getVtkId();
    myNodeIds[3] = n4->getVtkId();

    SMDS_VtkVolume *volvtk = myVolumePool->getNew();
    volvtk->init(myNodeIds, this);
    if (!this->registerElement(ID,volvtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(volvtk->getVtkId(), VTK_EMPTY_CELL);
      myVolumePool->destroy(volvtk);
      return 0;
    }
    volume = volvtk;
    adjustmyCellsCapacity(ID);
    myCells[ID] = volume;
    myInfo.myNbTetras++;
  }

  //  if (!registerElement(ID, volume)) {
  //    RemoveElement(volume, false);
  //    volume = NULL;
  //  }
  return volume;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new pyramid and add it to the mesh.
///Nodes 1,2,3 and 4 define the base of the pyramid
///@return The created pyramid
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                      const SMDS_MeshNode * n2,
                                      const SMDS_MeshNode * n3,
                                      const SMDS_MeshNode * n4,
                                      const SMDS_MeshNode * n5)
{
  int ID = myElementIDFactory->GetFreeID();
    //MESSAGE("AddVolumeWithID " << ID);
  SMDS_MeshVolume * v = SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, n5, ID);
  if(v==NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new pyramid and add it to the mesh.
///Nodes 1,2,3 and 4 define the base of the pyramid
///@param ID The ID of the new volume
///@return The created pyramid or NULL if an element with this ID already exists
///or if input nodes are not found.
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume * SMDS_Mesh::AddVolumeWithID(int idnode1,
                                             int idnode2,
                                             int idnode3,
                                             int idnode4,
                                             int idnode5,
                                             int ID)
{
    //MESSAGE("AddVolumeWithID " << ID);
  SMDS_MeshNode *node1, *node2, *node3, *node4, *node5;
  node1 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode1);
  node2 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode2);
  node3 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode3);
  node4 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode4);
  node5 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode5);
  if(!node1 || !node2 || !node3 || !node4 || !node5) return NULL;
  return SMDS_Mesh::AddVolumeWithID(node1, node2, node3, node4, node5, ID);
}

///////////////////////////////////////////////////////////////////////////////
///Create a new pyramid and add it to the mesh.
///Nodes 1,2,3 and 4 define the base of the pyramid
///@param ID The ID of the new volume
///@return The created pyramid
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                            const SMDS_MeshNode * n2,
                                            const SMDS_MeshNode * n3,
                                            const SMDS_MeshNode * n4,
                                            const SMDS_MeshNode * n5,
                                            int ID)
{
  //MESSAGE("AddVolumeWithID " << ID);
  SMDS_MeshVolume* volume = 0;
  if ( !n1 || !n2 || !n3 || !n4 || !n5) return volume;
  if ( NbVolumes() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionFaces()) {
    SMDS_MeshFace * f1=FindFaceOrCreate(n1,n2,n3,n4);
    SMDS_MeshFace * f2=FindFaceOrCreate(n1,n2,n5);
    SMDS_MeshFace * f3=FindFaceOrCreate(n2,n3,n5);
    SMDS_MeshFace * f4=FindFaceOrCreate(n3,n4,n5);
    volume=new SMDS_VolumeOfFaces(f1,f2,f3,f4);
    adjustmyCellsCapacity(ID);
    myCells[ID] = volume;
    myInfo.myNbPyramids++;
  }
  else if(hasConstructionEdges()) {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else {
    // --- retrieve nodes ID
    myNodeIds.resize(5);
    myNodeIds[0] = n1->getVtkId();
    myNodeIds[1] = n4->getVtkId();
    myNodeIds[2] = n3->getVtkId();
    myNodeIds[3] = n2->getVtkId();
    myNodeIds[4] = n5->getVtkId();

    SMDS_VtkVolume *volvtk = myVolumePool->getNew();
    volvtk->init(myNodeIds, this);
    if (!this->registerElement(ID,volvtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(volvtk->getVtkId(), VTK_EMPTY_CELL);
      myVolumePool->destroy(volvtk);
      return 0;
    }
    volume = volvtk;
    adjustmyCellsCapacity(ID);
    myCells[ID] = volume;
    myInfo.myNbPyramids++;
  }

  //  if (!registerElement(ID, volume)) {
  //    RemoveElement(volume, false);
  //    volume = NULL;
  //  }
  return volume;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new prism and add it to the mesh.
///Nodes 1,2,3 is a triangle and 1,2,5,4 a quadrangle.
///@return The created prism
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                      const SMDS_MeshNode * n2,
                                      const SMDS_MeshNode * n3,
                                      const SMDS_MeshNode * n4,
                                      const SMDS_MeshNode * n5,
                                      const SMDS_MeshNode * n6)
{
  int ID = myElementIDFactory->GetFreeID();
    //MESSAGE("AddVolumeWithID " << ID);
  SMDS_MeshVolume * v = SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, n5, n6, ID);
  if(v==NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new prism and add it to the mesh.
///Nodes 1,2,3 is a triangle and 1,2,5,4 a quadrangle.
///@param ID The ID of the new volume
///@return The created prism or NULL if an element with this ID already exists
///or if input nodes are not found.
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume * SMDS_Mesh::AddVolumeWithID(int idnode1,
                                             int idnode2,
                                             int idnode3,
                                             int idnode4,
                                             int idnode5,
                                             int idnode6,
                                             int ID)
{
    //MESSAGE("AddVolumeWithID " << ID);
  SMDS_MeshNode *node1, *node2, *node3, *node4, *node5, *node6;
  node1 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode1);
  node2 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode2);
  node3 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode3);
  node4 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode4);
  node5 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode5);
  node6 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode6);
  if(!node1 || !node2 || !node3 || !node4 || !node5 || !node6) return NULL;
  return SMDS_Mesh::AddVolumeWithID(node1, node2, node3, node4, node5, node6, ID);
}

///////////////////////////////////////////////////////////////////////////////
///Create a new prism and add it to the mesh.
///Nodes 1,2,3 is a triangle and 1,2,5,4 a quadrangle.
///@param ID The ID of the new volume
///@return The created prism
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                            const SMDS_MeshNode * n2,
                                            const SMDS_MeshNode * n3,
                                            const SMDS_MeshNode * n4,
                                            const SMDS_MeshNode * n5,
                                            const SMDS_MeshNode * n6,
                                            int ID)
{
  //MESSAGE("AddVolumeWithID " << ID);
  SMDS_MeshVolume* volume = 0;
  if ( !n1 || !n2 || !n3 || !n4 || !n5 || !n6) return volume;
  if ( NbVolumes() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionFaces()) {
    SMDS_MeshFace * f1=FindFaceOrCreate(n1,n2,n3);
    SMDS_MeshFace * f2=FindFaceOrCreate(n4,n5,n6);
    SMDS_MeshFace * f3=FindFaceOrCreate(n1,n4,n5,n2);
    SMDS_MeshFace * f4=FindFaceOrCreate(n2,n5,n6,n3);
    SMDS_MeshFace * f5=FindFaceOrCreate(n3,n6,n4,n1);
    volume=new SMDS_VolumeOfFaces(f1,f2,f3,f4,f5);
    adjustmyCellsCapacity(ID);
    myCells[ID] = volume;
    myInfo.myNbPrisms++;
  }
  else if(hasConstructionEdges()) {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else {
    // --- retrieve nodes ID
    myNodeIds.resize(6);
    myNodeIds[0] = n1->getVtkId();
    myNodeIds[1] = n2->getVtkId();
    myNodeIds[2] = n3->getVtkId();
    myNodeIds[3] = n4->getVtkId();
    myNodeIds[4] = n5->getVtkId();
    myNodeIds[5] = n6->getVtkId();

    SMDS_VtkVolume *volvtk = myVolumePool->getNew();
    volvtk->init(myNodeIds, this);
    if (!this->registerElement(ID,volvtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(volvtk->getVtkId(), VTK_EMPTY_CELL);
      myVolumePool->destroy(volvtk);
      return 0;
    }
    volume = volvtk;
    adjustmyCellsCapacity(ID);
    myCells[ID] = volume;
    myInfo.myNbPrisms++;
  }

  //  if (!registerElement(ID, volume)) {
  //    RemoveElement(volume, false);
  //    volume = NULL;
  //  }
  return volume;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new hexagonal prism and add it to the mesh.
///@return The created prism
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                      const SMDS_MeshNode * n2,
                                      const SMDS_MeshNode * n3,
                                      const SMDS_MeshNode * n4,
                                      const SMDS_MeshNode * n5,
                                      const SMDS_MeshNode * n6,
                                      const SMDS_MeshNode * n7,
                                      const SMDS_MeshNode * n8,
                                      const SMDS_MeshNode * n9,
                                      const SMDS_MeshNode * n10,
                                      const SMDS_MeshNode * n11,
                                      const SMDS_MeshNode * n12)
{
  int ID = myElementIDFactory->GetFreeID();
  SMDS_MeshVolume * v = SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, n5, n6,
                                                   n7, n8, n9, n10, n11, n12,
                                                   ID);
  if(v==NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new hexagonal prism and add it to the mesh.
///@param ID The ID of the new volume
///@return The created prism or NULL if an element with this ID already exists
///or if input nodes are not found.
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume * SMDS_Mesh::AddVolumeWithID(int idnode1,
                                             int idnode2,
                                             int idnode3,
                                             int idnode4,
                                             int idnode5,
                                             int idnode6,
                                             int idnode7,
                                             int idnode8,
                                             int idnode9,
                                             int idnode10,
                                             int idnode11,
                                             int idnode12,
                                             int ID)
{
  SMDS_MeshNode *node1 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode1);
  SMDS_MeshNode *node2 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode2);
  SMDS_MeshNode *node3 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode3);
  SMDS_MeshNode *node4 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode4);
  SMDS_MeshNode *node5 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode5);
  SMDS_MeshNode *node6 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode6);
  SMDS_MeshNode *node7 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode7);
  SMDS_MeshNode *node8 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode8);
  SMDS_MeshNode *node9 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode9);
  SMDS_MeshNode *node10 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode10);
  SMDS_MeshNode *node11 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode11);
  SMDS_MeshNode *node12 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode12);
  return SMDS_Mesh::AddVolumeWithID(node1, node2, node3, node4, node5, node6,
                                    node7, node8, node9, node10, node11, node12,
                                    ID);
}

///////////////////////////////////////////////////////////////////////////////
///Create a new hexagonal prism and add it to the mesh.
///@param ID The ID of the new volume
///@return The created prism
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                            const SMDS_MeshNode * n2,
                                            const SMDS_MeshNode * n3,
                                            const SMDS_MeshNode * n4,
                                            const SMDS_MeshNode * n5,
                                            const SMDS_MeshNode * n6,
                                            const SMDS_MeshNode * n7,
                                            const SMDS_MeshNode * n8,
                                            const SMDS_MeshNode * n9,
                                            const SMDS_MeshNode * n10,
                                            const SMDS_MeshNode * n11,
                                            const SMDS_MeshNode * n12,
                                            int ID)
{
  SMDS_MeshVolume* volume = 0;
  if(!n1 || !n2 || !n3 || !n4 || !n5 || !n6 ||
     !n7 || !n8 || !n9 || !n10 || !n11 || !n12 )
    return volume;
  if ( NbVolumes() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionFaces()) {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else if(hasConstructionEdges()) {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else {
    // --- retrieve nodes ID
    myNodeIds.resize(12);
    myNodeIds[0] = n1->getVtkId();
    myNodeIds[1] = n6->getVtkId();
    myNodeIds[2] = n5->getVtkId();
    myNodeIds[3] = n4->getVtkId();
    myNodeIds[4] = n3->getVtkId();
    myNodeIds[5] = n2->getVtkId();

    myNodeIds[6] = n7->getVtkId();
    myNodeIds[7] = n12->getVtkId();
    myNodeIds[8] = n11->getVtkId();
    myNodeIds[9] = n10->getVtkId();
    myNodeIds[10] = n9->getVtkId();
    myNodeIds[11] = n8->getVtkId();

    SMDS_VtkVolume *volvtk = myVolumePool->getNew();
    volvtk->init(myNodeIds, this);
    if (!this->registerElement(ID,volvtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(volvtk->getVtkId(), VTK_EMPTY_CELL);
      myVolumePool->destroy(volvtk);
      return 0;
    }
    volume = volvtk;
    adjustmyCellsCapacity(ID);
    myCells[ID] = volume;
    myInfo.myNbHexPrism++;
  }

  return volume;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new hexahedron and add it to the mesh.
///Nodes 1,2,3,4 and 5,6,7,8 are quadrangle and 5,1 and 7,3 are an edges.
///@return The created hexahedron
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                      const SMDS_MeshNode * n2,
                                      const SMDS_MeshNode * n3,
                                      const SMDS_MeshNode * n4,
                                      const SMDS_MeshNode * n5,
                                      const SMDS_MeshNode * n6,
                                      const SMDS_MeshNode * n7,
                                      const SMDS_MeshNode * n8)
{
  int ID = myElementIDFactory->GetFreeID();
 SMDS_MeshVolume * v = SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, n5, n6, n7, n8, ID);
  if(v==NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new hexahedron and add it to the mesh.
///Nodes 1,2,3,4 and 5,6,7,8 are quadrangle and 5,1 and 7,3 are an edges.
///@param ID The ID of the new volume
///@return The created hexahedron or NULL if an element with this ID already
///exists or if input nodes are not found.
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume * SMDS_Mesh::AddVolumeWithID(int idnode1,
                                             int idnode2,
                                             int idnode3,
                                             int idnode4,
                                             int idnode5,
                                             int idnode6,
                                             int idnode7,
                                             int idnode8,
                                             int ID)
{
    //MESSAGE("AddVolumeWithID " << ID);
  SMDS_MeshNode *node1, *node2, *node3, *node4, *node5, *node6, *node7, *node8;
  node1 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode1);
  node2 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode2);
  node3 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode3);
  node4 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode4);
  node5 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode5);
  node6 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode6);
  node7 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode7);
  node8 = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(idnode8);
  if(!node1 || !node2 || !node3 || !node4 || !node5 || !node6 || !node7 || !node8)
    return NULL;
  return SMDS_Mesh::AddVolumeWithID(node1, node2, node3, node4, node5, node6,
                                    node7, node8, ID);
}

///////////////////////////////////////////////////////////////////////////////
///Create a new hexahedron and add it to the mesh.
///Nodes 1,2,3,4 and 5,6,7,8 are quadrangle and 5,1 and 7,3 are an edges.
///@param ID The ID of the new volume
///@return The created prism or NULL if an element with this ID already exists
///or if input nodes are not found.
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                            const SMDS_MeshNode * n2,
                                            const SMDS_MeshNode * n3,
                                            const SMDS_MeshNode * n4,
                                            const SMDS_MeshNode * n5,
                                            const SMDS_MeshNode * n6,
                                            const SMDS_MeshNode * n7,
                                            const SMDS_MeshNode * n8,
                                            int ID)
{
    //MESSAGE("AddVolumeWithID " << ID);
  SMDS_MeshVolume* volume = 0;
  if ( !n1 || !n2 || !n3 || !n4 || !n5 || !n6 || !n7 || !n8) return volume;
  if ( NbVolumes() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionFaces()) {
    SMDS_MeshFace * f1=FindFaceOrCreate(n1,n2,n3,n4);
    SMDS_MeshFace * f2=FindFaceOrCreate(n5,n6,n7,n8);
    SMDS_MeshFace * f3=FindFaceOrCreate(n1,n4,n8,n5);
    SMDS_MeshFace * f4=FindFaceOrCreate(n1,n2,n6,n5);
    SMDS_MeshFace * f5=FindFaceOrCreate(n2,n3,n7,n6);
    SMDS_MeshFace * f6=FindFaceOrCreate(n3,n4,n8,n7);
    volume=new SMDS_VolumeOfFaces(f1,f2,f3,f4,f5,f6);
    adjustmyCellsCapacity(ID);
    myCells[ID] = volume;
    myInfo.myNbHexas++;
  }
  else if(hasConstructionEdges()) {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else {
    // --- retrieve nodes ID
    myNodeIds.resize(8);
    myNodeIds[0] = n1->getVtkId();
    myNodeIds[1] = n4->getVtkId();
    myNodeIds[2] = n3->getVtkId();
    myNodeIds[3] = n2->getVtkId();
    myNodeIds[4] = n5->getVtkId();
    myNodeIds[5] = n8->getVtkId();
    myNodeIds[6] = n7->getVtkId();
    myNodeIds[7] = n6->getVtkId();

    SMDS_VtkVolume *volvtk = myVolumePool->getNew();
    volvtk->init(myNodeIds, this);
    if (!this->registerElement(ID,volvtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(volvtk->getVtkId(), VTK_EMPTY_CELL);
      myVolumePool->destroy(volvtk);
      return 0;
    }
    volume = volvtk;
    adjustmyCellsCapacity(ID);
    myCells[ID] = volume;
    myInfo.myNbHexas++;
  }

  //  if (!registerElement(ID, volume)) {
  //    RemoveElement(volume, false);
  //    volume = NULL;
  //  }
  return volume;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new tetrahedron defined by its faces and add it to the mesh.
///@return The created tetrahedron
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshFace * f1,
                                      const SMDS_MeshFace * f2,
                                      const SMDS_MeshFace * f3,
                                      const SMDS_MeshFace * f4)
{
    //MESSAGE("AddVolumeWithID");
  if (!hasConstructionFaces())
    return NULL;
  return AddVolumeWithID(f1,f2,f3,f4, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
///Create a new tetrahedron defined by its faces and add it to the mesh.
///@param ID The ID of the new volume
///@return The created tetrahedron
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshFace * f1,
                                            const SMDS_MeshFace * f2,
                                            const SMDS_MeshFace * f3,
                                            const SMDS_MeshFace * f4,
                                            int ID)
{
  MESSAGE("AddVolumeWithID" << ID);
  if (!hasConstructionFaces())
    return NULL;
  if ( !f1 || !f2 || !f3 || !f4) return 0;
  if ( NbVolumes() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  SMDS_MeshVolume * volume = new SMDS_VolumeOfFaces(f1,f2,f3,f4);
  adjustmyCellsCapacity(ID);
  myCells[ID] = volume;
  myInfo.myNbTetras++;

  if (!registerElement(ID, volume)) {
    registerElement(myElementIDFactory->GetFreeID(), volume);
    //RemoveElement(volume, false);
    //volume = NULL;
  }
  return volume;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new pyramid defined by its faces and add it to the mesh.
///@return The created pyramid
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshFace * f1,
                                      const SMDS_MeshFace * f2,
                                      const SMDS_MeshFace * f3,
                                      const SMDS_MeshFace * f4,
                                      const SMDS_MeshFace * f5)
{
     //MESSAGE("AddVolumeWithID");
 if (!hasConstructionFaces())
    return NULL;
  return AddVolumeWithID(f1,f2,f3,f4,f5, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
///Create a new pyramid defined by its faces and add it to the mesh.
///@param ID The ID of the new volume
///@return The created pyramid
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshFace * f1,
                                            const SMDS_MeshFace * f2,
                                            const SMDS_MeshFace * f3,
                                            const SMDS_MeshFace * f4,
                                            const SMDS_MeshFace * f5,
                                            int ID)
{
  MESSAGE("AddVolumeWithID" << ID);
  if (!hasConstructionFaces())
    return NULL;
  if ( !f1 || !f2 || !f3 || !f4 || !f5) return 0;
  if ( NbVolumes() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  SMDS_MeshVolume * volume = new SMDS_VolumeOfFaces(f1,f2,f3,f4,f5);
  adjustmyCellsCapacity(ID);
  myCells[ID] = volume;
  myInfo.myNbPyramids++;

  if (!registerElement(ID, volume)) {
    registerElement(myElementIDFactory->GetFreeID(), volume);
    //RemoveElement(volume, false);
    //volume = NULL;
  }
  return volume;
}

///////////////////////////////////////////////////////////////////////////////
///Create a new prism defined by its faces and add it to the mesh.
///@return The created prism
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshFace * f1,
                                      const SMDS_MeshFace * f2,
                                      const SMDS_MeshFace * f3,
                                      const SMDS_MeshFace * f4,
                                      const SMDS_MeshFace * f5,
                                      const SMDS_MeshFace * f6)
{
     //MESSAGE("AddVolumeWithID" );
 if (!hasConstructionFaces())
    return NULL;
  return AddVolumeWithID(f1,f2,f3,f4,f5,f6, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
///Create a new prism defined by its faces and add it to the mesh.
///@param ID The ID of the new volume
///@return The created prism
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshFace * f1,
                                            const SMDS_MeshFace * f2,
                                            const SMDS_MeshFace * f3,
                                            const SMDS_MeshFace * f4,
                                            const SMDS_MeshFace * f5,
                                            const SMDS_MeshFace * f6,
                                            int ID)
{
  MESSAGE("AddVolumeWithID" << ID);
  if (!hasConstructionFaces())
    return NULL;
  if ( !f1 || !f2 || !f3 || !f4 || !f5 || !f6) return 0;
  if ( NbVolumes() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  SMDS_MeshVolume * volume = new SMDS_VolumeOfFaces(f1,f2,f3,f4,f5,f6);
  adjustmyCellsCapacity(ID);
  myCells[ID] = volume;
  myInfo.myNbPrisms++;

  if (!registerElement(ID, volume)) {
    registerElement(myElementIDFactory->GetFreeID(), volume);
    //RemoveElement(volume, false);
    //volume = NULL;
  }
  return volume;
}

///////////////////////////////////////////////////////////////////////////////
/// Add a polygon defined by its nodes IDs
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddPolygonalFaceWithID (const vector<int> & nodes_ids,
                                                  const int           ID)
{
  int nbNodes = nodes_ids.size();
  vector<const SMDS_MeshNode*> nodes (nbNodes);
  for (int i = 0; i < nbNodes; i++) {
    nodes[i] = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(nodes_ids[i]);
    if (!nodes[i]) return NULL;
  }
  return SMDS_Mesh::AddPolygonalFaceWithID(nodes, ID);
}

///////////////////////////////////////////////////////////////////////////////
/// Add a polygon defined by its nodes
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace*
SMDS_Mesh::AddPolygonalFaceWithID (const vector<const SMDS_MeshNode*> & nodes,
                                   const int                            ID)
{
  SMDS_MeshFace * face;

  if ( NbFaces() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if (hasConstructionEdges())
  {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else
  {
    myNodeIds.resize( nodes.size() );
    for ( size_t i = 0; i < nodes.size(); ++i )
      myNodeIds[i] = nodes[i]->getVtkId();

    SMDS_VtkFace *facevtk = myFacePool->getNew();
    facevtk->initPoly(myNodeIds, this);
    if (!this->registerElement(ID,facevtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(facevtk->getVtkId(), VTK_EMPTY_CELL);
      myFacePool->destroy(facevtk);
      return 0;
    }
    face = facevtk;

    adjustmyCellsCapacity(ID);
    myCells[ID] = face;
    myInfo.myNbPolygons++;
  }

  return face;
}

///////////////////////////////////////////////////////////////////////////////
/// Add a polygon defined by its nodes.
/// An ID is automatically affected to the created face.
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddPolygonalFace (const vector<const SMDS_MeshNode*> & nodes)
{
  return SMDS_Mesh::AddPolygonalFaceWithID(nodes, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
/// Add a quadratic polygon defined by its nodes IDs
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddQuadPolygonalFaceWithID (const vector<int> & nodes_ids,
                                                      const int           ID)
{
  vector<const SMDS_MeshNode*> nodes( nodes_ids.size() );
  for ( size_t i = 0; i < nodes.size(); i++) {
    nodes[i] = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(nodes_ids[i]);
    if (!nodes[i]) return NULL;
  }
  return SMDS_Mesh::AddQuadPolygonalFaceWithID(nodes, ID);
}

///////////////////////////////////////////////////////////////////////////////
/// Add a quadratic polygon defined by its nodes
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace*
SMDS_Mesh::AddQuadPolygonalFaceWithID (const vector<const SMDS_MeshNode*> & nodes,
                                       const int                            ID)
{
  SMDS_MeshFace * face;

  if ( NbFaces() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if (hasConstructionEdges())
  {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else
  {
    myNodeIds.resize( nodes.size() );
    for ( size_t i = 0; i < nodes.size(); ++i )
      myNodeIds[i] = nodes[i]->getVtkId();

    SMDS_VtkFace *facevtk = myFacePool->getNew();
    facevtk->initQuadPoly(myNodeIds, this);
    if (!this->registerElement(ID,facevtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(facevtk->getVtkId(), VTK_EMPTY_CELL);
      myFacePool->destroy(facevtk);
      return 0;
    }
    face = facevtk;
    adjustmyCellsCapacity(ID);
    myCells[ID] = face;
    myInfo.myNbQuadPolygons++;
  }
  return face;
}

///////////////////////////////////////////////////////////////////////////////
/// Add a quadratic polygon defined by its nodes.
/// An ID is automatically affected to the created face.
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddQuadPolygonalFace (const vector<const SMDS_MeshNode*> & nodes)
{
  return SMDS_Mesh::AddQuadPolygonalFaceWithID(nodes, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
/// Create a new polyhedral volume and add it to the mesh.
/// @param ID The ID of the new volume
/// @return The created volume or NULL if an element with this ID already exists
/// or if input nodes are not found.
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume * SMDS_Mesh::AddPolyhedralVolumeWithID
                             (const vector<int> & nodes_ids,
                              const vector<int> & quantities,
                              const int           ID)
{
  int nbNodes = nodes_ids.size();
  vector<const SMDS_MeshNode*> nodes (nbNodes);
  for (int i = 0; i < nbNodes; i++) {
    nodes[i] = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(nodes_ids[i]);
    if (!nodes[i]) return NULL;
  }
  return SMDS_Mesh::AddPolyhedralVolumeWithID(nodes, quantities, ID);
}

///////////////////////////////////////////////////////////////////////////////
/// Create a new polyhedral volume and add it to the mesh.
/// @param ID The ID of the new volume
/// @return The created  volume
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume*
SMDS_Mesh::AddPolyhedralVolumeWithID (const vector<const SMDS_MeshNode*>& nodes,
                                      const vector<int>                 & quantities,
                                      const int                           ID)
{
  SMDS_MeshVolume* volume = 0;
  if ( nodes.empty() || quantities.empty() )
    return NULL;
  if ( NbVolumes() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if (hasConstructionFaces())
  {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else if (hasConstructionEdges())
  {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else
  {
    //#ifdef VTK_HAVE_POLYHEDRON
    //MESSAGE("AddPolyhedralVolumeWithID vtk " << ID);
    myNodeIds.resize( nodes.size() );
    for ( size_t i = 0; i < nodes.size(); ++i )
      myNodeIds[i] = nodes[i]->getVtkId();

    SMDS_VtkVolume *volvtk = myVolumePool->getNew();
    volvtk->initPoly(myNodeIds, quantities, this);
    if (!this->registerElement(ID, volvtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(volvtk->getVtkId(), VTK_EMPTY_CELL);
      myVolumePool->destroy(volvtk);
      return 0;
    }
    volume = volvtk;
    //#else
    //      MESSAGE("AddPolyhedralVolumeWithID smds " << ID);
    //      for ( int i = 0; i < nodes.size(); ++i )
    //      if ( !nodes[ i ] ) return 0;
    //      volume = new SMDS_PolyhedralVolumeOfNodes(nodes, quantities);
    //#endif
    adjustmyCellsCapacity(ID);
    myCells[ID] = volume;
    myInfo.myNbPolyhedrons++;
  }

  //#ifndef VTK_HAVE_POLYHEDRON
  //  if (!registerElement(ID, volume))
  //    {
  //      registerElement(myElementIDFactory->GetFreeID(), volume);
  //      //RemoveElement(volume, false);
  //      //volume = NULL;
  //    }
  //#endif
  return volume;
}

///////////////////////////////////////////////////////////////////////////////
/// Create a new polyhedral volume and add it to the mesh.
/// @return The created  volume
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddPolyhedralVolume
                            (const vector<const SMDS_MeshNode*> & nodes,
                             const vector<int>                  & quantities)
{
  int ID = myElementIDFactory->GetFreeID();
  SMDS_MeshVolume * v = SMDS_Mesh::AddPolyhedralVolumeWithID(nodes, quantities, ID);
  if (v == NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

SMDS_MeshVolume* SMDS_Mesh::AddVolumeFromVtkIds(const std::vector<vtkIdType>& vtkNodeIds)
{
  int ID = myElementIDFactory->GetFreeID();
  SMDS_MeshVolume * v = SMDS_Mesh::AddVolumeFromVtkIdsWithID(vtkNodeIds, ID);
  if (v == NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

SMDS_MeshVolume* SMDS_Mesh::AddVolumeFromVtkIdsWithID(const std::vector<vtkIdType>& vtkNodeIds, const int ID)
{
  SMDS_VtkVolume *volvtk = myVolumePool->getNew();
  volvtk->init(vtkNodeIds, this);
  if (!this->registerElement(ID,volvtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(volvtk->getVtkId(), VTK_EMPTY_CELL);
      myVolumePool->destroy(volvtk);
      return 0;
    }
  adjustmyCellsCapacity(ID);
  myCells[ID] = volvtk;
  vtkIdType aVtkType = volvtk->GetVtkType();
  switch (aVtkType)
  {
    case VTK_TETRA:
      myInfo.myNbTetras++;
      break;
    case VTK_PYRAMID:
      myInfo.myNbPyramids++;
      break;
    case VTK_WEDGE:
      myInfo.myNbPrisms++;
      break;
    case VTK_HEXAHEDRON:
      myInfo.myNbHexas++;
      break;
    case VTK_QUADRATIC_TETRA:
      myInfo.myNbQuadTetras++;
      break;
    case VTK_QUADRATIC_PYRAMID:
      myInfo.myNbQuadPyramids++;
      break;
    case VTK_QUADRATIC_WEDGE:
      myInfo.myNbQuadPrisms++;
      break;
    case VTK_QUADRATIC_HEXAHEDRON:
      myInfo.myNbQuadHexas++;
      break;
//#ifdef VTK_HAVE_POLYHEDRON
    case VTK_POLYHEDRON:
      myInfo.myNbPolyhedrons++;
      break;
//#endif
    default:
      myInfo.myNbPolyhedrons++;
      break;
  }
  return volvtk;
}

SMDS_MeshFace* SMDS_Mesh::AddFaceFromVtkIds(const std::vector<vtkIdType>& vtkNodeIds)
{
  int ID = myElementIDFactory->GetFreeID();
  SMDS_MeshFace * f = SMDS_Mesh::AddFaceFromVtkIdsWithID(vtkNodeIds, ID);
  if (f == NULL) myElementIDFactory->ReleaseID(ID);
  return f;
}

SMDS_MeshFace* SMDS_Mesh::AddFaceFromVtkIdsWithID(const std::vector<vtkIdType>& vtkNodeIds, const int ID)
{
  SMDS_VtkFace *facevtk = myFacePool->getNew();
  facevtk->init(vtkNodeIds, this);
  if (!this->registerElement(ID,facevtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(facevtk->getVtkId(), VTK_EMPTY_CELL);
      myFacePool->destroy(facevtk);
      return 0;
    }
  adjustmyCellsCapacity(ID);
  myCells[ID] = facevtk;
  vtkIdType aVtkType = facevtk->GetVtkType();
  switch (aVtkType)
  {
    case VTK_TRIANGLE:
      myInfo.myNbTriangles++;
      break;
    case VTK_QUAD:
      myInfo.myNbQuadrangles++;
      break;
    case VTK_QUADRATIC_TRIANGLE:
      myInfo.myNbQuadTriangles++;
      break;
    case VTK_QUADRATIC_QUAD:
      myInfo.myNbQuadQuadrangles++;
      break;
    case VTK_BIQUADRATIC_QUAD:
      myInfo.myNbBiQuadQuadrangles++;
      break;
    case VTK_BIQUADRATIC_TRIANGLE:
      myInfo.myNbBiQuadTriangles++;
      break;
    case VTK_POLYGON:
      myInfo.myNbPolygons++;
      break;
     default:
      myInfo.myNbPolygons++;
  }
  return facevtk;
}

///////////////////////////////////////////////////////////////////////////////
/// Registers element with the given ID, maintains inverse connections
///////////////////////////////////////////////////////////////////////////////
bool SMDS_Mesh::registerElement(int ID, SMDS_MeshElement* element)
{
  //MESSAGE("registerElement " << ID);
  if ((ID >=0) && (ID < myCells.size()) && myCells[ID]) // --- already bound
  {
    MESSAGE(" ------------------ already bound "<< ID << " " << myCells[ID]->getVtkId());
    return false;
  }

  element->myID = ID;
  element->myMeshId = myMeshId;

  SMDS_MeshCell *cell = dynamic_cast<SMDS_MeshCell*>(element);
  MYASSERT(cell);
  int vtkId = cell->getVtkId();  
  if (vtkId == -1)
    vtkId = myElementIDFactory->SetInVtkGrid(element);

  if (vtkId >= myCellIdVtkToSmds.size()) // --- resize local vector
  {
//     MESSAGE(" --------------------- resize myCellIdVtkToSmds " << vtkId << " --> " << vtkId + SMDS_Mesh::chunkSize);
    myCellIdVtkToSmds.resize(vtkId + SMDS_Mesh::chunkSize, -1);
  }
  myCellIdVtkToSmds[vtkId] = ID;

  myElementIDFactory->updateMinMax(ID);
  return true;
}

//=======================================================================
//function : MoveNode
//purpose  : 
//=======================================================================

void SMDS_Mesh::MoveNode(const SMDS_MeshNode *n, double x, double y, double z)
{
  SMDS_MeshNode * node=const_cast<SMDS_MeshNode*>(n);
  node->setXYZ(x,y,z);
}

///////////////////////////////////////////////////////////////////////////////
/// Return the node whose SMDS ID is 'ID'.
///////////////////////////////////////////////////////////////////////////////
const SMDS_MeshNode * SMDS_Mesh::FindNode(int ID) const
{
  if (ID < 1 || ID >= myNodes.size())
  {
//     MESSAGE("------------------------------------------------------------------------- ");
//     MESSAGE("----------------------------------- bad ID " << ID << " " << myNodes.size());
//     MESSAGE("------------------------------------------------------------------------- ");
    return 0;
  }
  return (const SMDS_MeshNode *)myNodes[ID];
}

///////////////////////////////////////////////////////////////////////////////
/// Return the node whose VTK ID is 'vtkId'.
///////////////////////////////////////////////////////////////////////////////
const SMDS_MeshNode * SMDS_Mesh::FindNodeVtk(int vtkId) const
{
  // TODO if needed use mesh->nodeIdFromVtkToSmds
  if (vtkId < 0 || vtkId >= (myNodes.size() -1))
  {
    MESSAGE("------------------------------------------------------------------------- ");
    MESSAGE("---------------------------- bad VTK ID " << vtkId << " " << myNodes.size());
    MESSAGE("------------------------------------------------------------------------- ");
    return 0;
  }
  return (const SMDS_MeshNode *)myNodes[vtkId+1];
}

///////////////////////////////////////////////////////////////////////////////
///Create a triangle and add it to the current mesh. This method do not bind an
///ID to the create triangle.
///////////////////////////////////////////////////////////////////////////////
SMDS_MeshFace * SMDS_Mesh::createTriangle(const SMDS_MeshNode * node1,
                                          const SMDS_MeshNode * node2,
                                          const SMDS_MeshNode * node3,
                                          int ID)
{
  if ( !node1 || !node2 || !node3) return 0;
  if ( NbFaces() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionEdges())
  {
    SMDS_MeshEdge *edge1, *edge2, *edge3;
    edge1=FindEdgeOrCreate(node1,node2);
    edge2=FindEdgeOrCreate(node2,node3);
    edge3=FindEdgeOrCreate(node3,node1);

    //int ID = myElementIDFactory->GetFreeID(); // -PR- voir si on range cet element
    SMDS_MeshFace * face = new SMDS_FaceOfEdges(edge1,edge2,edge3);
    adjustmyCellsCapacity(ID);
    myCells[ID] = face;
    myInfo.myNbTriangles++;
    return face;
  }
  else
  {
    // --- retrieve nodes ID
    myNodeIds.resize(3);
    myNodeIds[0] = node1->getVtkId();
    myNodeIds[1] = node2->getVtkId();
    myNodeIds[2] = node3->getVtkId();

    SMDS_MeshFace * face = 0;
    SMDS_VtkFace *facevtk = myFacePool->getNew();
    facevtk->init(myNodeIds, this); // put in vtkUnstructuredGrid
    if (!this->registerElement(ID,facevtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(facevtk->getVtkId(), VTK_EMPTY_CELL);
      myFacePool->destroy(facevtk);
      return 0;
    }
    face = facevtk;
    adjustmyCellsCapacity(ID);
    myCells[ID] = face;
    //MESSAGE("createTriangle " << ID << " " << face);
    myInfo.myNbTriangles++;
    return face;
  }
}

///////////////////////////////////////////////////////////////////////////////
///Create a quadrangle and add it to the current mesh. This methode do not bind
///a ID to the create triangle.
///////////////////////////////////////////////////////////////////////////////
SMDS_MeshFace * SMDS_Mesh::createQuadrangle(const SMDS_MeshNode * node1,
                                            const SMDS_MeshNode * node2,
                                            const SMDS_MeshNode * node3,
                                            const SMDS_MeshNode * node4,
                                            int ID)
{
  if ( !node1 || !node2 || !node3 || !node4 ) return 0;
  if ( NbFaces() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionEdges())
  {
    //MESSAGE("createQuadrangle hasConstructionEdges "<< ID);
    SMDS_MeshEdge *edge1, *edge2, *edge3, *edge4;
    edge1=FindEdgeOrCreate(node1,node2);
    edge2=FindEdgeOrCreate(node2,node3);
    edge3=FindEdgeOrCreate(node3,node4);
    edge4=FindEdgeOrCreate(node4,node1);

    SMDS_MeshFace * face = new SMDS_FaceOfEdges(edge1,edge2,edge3,edge4);
    adjustmyCellsCapacity(ID);
    myCells[ID] = face;
    myInfo.myNbQuadrangles++;
    return face;
  }
  else
  {
    // --- retrieve nodes ID
    myNodeIds.resize(4);
    myNodeIds[0] = node1->getVtkId();
    myNodeIds[1] = node2->getVtkId();
    myNodeIds[2] = node3->getVtkId();
    myNodeIds[3] = node4->getVtkId();

    SMDS_MeshFace * face = 0;
    SMDS_VtkFace *facevtk = myFacePool->getNew();
    facevtk->init(myNodeIds, this);
    if (!this->registerElement(ID,facevtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(facevtk->getVtkId(), VTK_EMPTY_CELL);
      myFacePool->destroy(facevtk);
      return 0;
    }
    face = facevtk;
    adjustmyCellsCapacity(ID);
    myCells[ID] = face;
    myInfo.myNbQuadrangles++;
    return face;
  }
}

///////////////////////////////////////////////////////////////////////////////
/// Remove a node and all the elements which own this node
///////////////////////////////////////////////////////////////////////////////

void SMDS_Mesh::RemoveNode(const SMDS_MeshNode * node)
{
  MESSAGE("RemoveNode");
  RemoveElement(node, true);
}

///////////////////////////////////////////////////////////////////////////////
/// Remove an edge and all the elements which own this edge
///////////////////////////////////////////////////////////////////////////////

void SMDS_Mesh::Remove0DElement(const SMDS_Mesh0DElement * elem0d)
{
  MESSAGE("Remove0DElement");
  RemoveElement(elem0d,true);
}

///////////////////////////////////////////////////////////////////////////////
/// Remove an edge and all the elements which own this edge
///////////////////////////////////////////////////////////////////////////////

void SMDS_Mesh::RemoveEdge(const SMDS_MeshEdge * edge)
{
  MESSAGE("RemoveEdge");
  RemoveElement(edge,true);
}

///////////////////////////////////////////////////////////////////////////////
/// Remove an face and all the elements which own this face
///////////////////////////////////////////////////////////////////////////////

void SMDS_Mesh::RemoveFace(const SMDS_MeshFace * face)
{
  MESSAGE("RemoveFace");
  RemoveElement(face, true);
}

///////////////////////////////////////////////////////////////////////////////
/// Remove a volume
///////////////////////////////////////////////////////////////////////////////

void SMDS_Mesh::RemoveVolume(const SMDS_MeshVolume * volume)
{
  MESSAGE("RemoveVolume");
  RemoveElement(volume, true);
}

//=======================================================================
//function : RemoveFromParent
//purpose  :
//=======================================================================

bool SMDS_Mesh::RemoveFromParent()
{
  if (myParent==NULL) return false;
  else return (myParent->RemoveSubMesh(this));
}

//=======================================================================
//function : RemoveSubMesh
//purpose  :
//=======================================================================

bool SMDS_Mesh::RemoveSubMesh(const SMDS_Mesh * aMesh)
{
  bool found = false;

  list<SMDS_Mesh *>::iterator itmsh=myChildren.begin();
  for (; itmsh!=myChildren.end() && !found; itmsh++)
  {
    SMDS_Mesh * submesh = *itmsh;
    if (submesh == aMesh)
    {
      found = true;
      myChildren.erase(itmsh);
    }
  }

  return found;
}

//=======================================================================
//function : ChangeElementNodes
//purpose  :
//=======================================================================

bool SMDS_Mesh::ChangeElementNodes(const SMDS_MeshElement * element,
                                   const SMDS_MeshNode    * nodes[],
                                   const int                nbnodes)
{
  MESSAGE("SMDS_Mesh::ChangeElementNodes");
  // keep current nodes of elem
  set<const SMDS_MeshNode*> oldNodes( element->begin_nodes(), element->end_nodes() );

  // change nodes
  bool Ok = false;
  SMDS_MeshCell* cell = dynamic_cast<SMDS_MeshCell*>((SMDS_MeshElement*) element);
  if (cell)
  {
    Ok = cell->vtkOrder(nodes, nbnodes);
    Ok = cell->ChangeNodes(nodes, nbnodes);
  }

  if ( Ok ) { // update InverseElements

    set<const SMDS_MeshNode*>::iterator it;

    // AddInverseElement to new nodes
    for ( int i = 0; i < nbnodes; i++ ) {
      it = oldNodes.find( nodes[i] );
      if ( it == oldNodes.end() )
        // new node
        const_cast<SMDS_MeshNode*>( nodes[i] )->AddInverseElement( cell );
      else
        // remove from oldNodes a node that remains in elem
        oldNodes.erase( it );
    }
    // RemoveInverseElement from the nodes removed from elem
    for ( it = oldNodes.begin(); it != oldNodes.end(); it++ )
    {
      SMDS_MeshNode * n = const_cast<SMDS_MeshNode *>( *it );
      n->RemoveInverseElement( cell );
    }
  }

  return Ok;
}

//=======================================================================
//function : ChangePolyhedronNodes
//purpose  : to change nodes of polyhedral volume
//=======================================================================
bool SMDS_Mesh::ChangePolyhedronNodes (const SMDS_MeshElement *            elem,
                                       const vector<const SMDS_MeshNode*>& nodes,
                                       const vector<int>                 & quantities)
{
  if (elem->GetType() != SMDSAbs_Volume) {
    MESSAGE("WRONG ELEM TYPE");
    return false;
  }

  const SMDS_VtkVolume* vol = dynamic_cast<const SMDS_VtkVolume*>(elem);
  if (!vol) {
    return false;
  }

  // keep current nodes of elem
  set<const SMDS_MeshElement*> oldNodes;
  SMDS_ElemIteratorPtr itn = elem->nodesIterator();
  while (itn->more()) {
    oldNodes.insert(itn->next());
  }

  // change nodes
  // TODO remove this function
  //bool Ok = const_cast<SMDS_VtkVolume*>(vol)->ChangeNodes(nodes, quantities);
  bool Ok = false;
  if (!Ok) {
    return false;
  }

  // update InverseElements

  // AddInverseElement to new nodes
  int nbnodes = nodes.size();
  set<const SMDS_MeshElement*>::iterator it;
  for (int i = 0; i < nbnodes; i++) {
    it = oldNodes.find(nodes[i]);
    if (it == oldNodes.end()) {
      // new node
      const_cast<SMDS_MeshNode*>(nodes[i])->AddInverseElement(elem);
    } else {
      // remove from oldNodes a node that remains in elem
      oldNodes.erase(it);
    }
  }

  // RemoveInverseElement from the nodes removed from elem
  for (it = oldNodes.begin(); it != oldNodes.end(); it++) {
    SMDS_MeshNode * n = static_cast<SMDS_MeshNode *>
      (const_cast<SMDS_MeshElement *>( *it ));
    n->RemoveInverseElement(elem);
  }

  return Ok;
}


//=======================================================================
//function : Find0DElement
//purpose  :
//=======================================================================
const SMDS_Mesh0DElement* SMDS_Mesh::Find0DElement(int idnode) const
{
  const SMDS_MeshNode * node = FindNode(idnode);
  if(node == NULL) return NULL;
  return Find0DElement(node);
}

const SMDS_Mesh0DElement* SMDS_Mesh::Find0DElement(const SMDS_MeshNode * node)
{
  if (!node) return 0;
  const SMDS_Mesh0DElement* toReturn = NULL;
  SMDS_ElemIteratorPtr it1 = node->GetInverseElementIterator(SMDSAbs_0DElement);
  while (it1->more() && (toReturn == NULL)) {
    const SMDS_MeshElement* e = it1->next();
    if (e->NbNodes() == 1) {
      toReturn = static_cast<const SMDS_Mesh0DElement*>(e);
    }
  }
  return toReturn;
}

//=======================================================================
//function : FindBall
//purpose  :
//=======================================================================

const SMDS_BallElement* SMDS_Mesh::FindBall(int idnode) const
{
  const SMDS_MeshNode * node = FindNode(idnode);
  if(node == NULL) return NULL;
  return FindBall(node);
}

const SMDS_BallElement* SMDS_Mesh::FindBall(const SMDS_MeshNode * node)
{
  if (!node) return 0;
  const SMDS_BallElement* toReturn = NULL;
  SMDS_ElemIteratorPtr it1 = node->GetInverseElementIterator(SMDSAbs_Ball);
  while (it1->more() && (toReturn == NULL)) {
    const SMDS_MeshElement* e = it1->next();
    if (e->GetGeomType() == SMDSGeom_BALL)
      toReturn = static_cast<const SMDS_BallElement*>(e);
  }
  return toReturn;
}

//=======================================================================
//function : Find0DElementOrCreate
//purpose  :
//=======================================================================
//SMDS_Mesh0DElement* SMDS_Mesh::Find0DElementOrCreate(const SMDS_MeshNode * node)
//{
//  if (!node) return 0;
//  SMDS_Mesh0DElement * toReturn = NULL;
//  toReturn = const_cast<SMDS_Mesh0DElement*>(Find0DElement(node));
//  if (toReturn == NULL) {
//    //if (my0DElements.Extent() % CHECKMEMORY_INTERVAL == 0) CheckMemory();
//    toReturn = new SMDS_Mesh0DElement(node);
//    my0DElements.Add(toReturn);
//    myInfo.myNb0DElements++;
//  }
//  return toReturn;
//}


//=======================================================================
//function : FindEdge
//purpose  :
//=======================================================================

const SMDS_MeshEdge* SMDS_Mesh::FindEdge(int idnode1, int idnode2) const
{
  const SMDS_MeshNode * node1=FindNode(idnode1);
  const SMDS_MeshNode * node2=FindNode(idnode2);
  if((node1==NULL)||(node2==NULL)) return NULL;
  return FindEdge(node1,node2);
}

//#include "Profiler.h"
const SMDS_MeshEdge* SMDS_Mesh::FindEdge(const SMDS_MeshNode * node1,
                                         const SMDS_MeshNode * node2)
{
  if ( !node1 ) return 0;
  const SMDS_MeshEdge * toReturn=NULL;
  //PROFILER_Init();
  //PROFILER_Set();
  SMDS_ElemIteratorPtr it1=node1->GetInverseElementIterator(SMDSAbs_Edge);
  //PROFILER_Get(0);
  //PROFILER_Set();
  while(it1->more()) {
    const SMDS_MeshElement * e = it1->next();
    if ( e->NbNodes() == 2 && e->GetNodeIndex( node2 ) >= 0 ) {
      toReturn = static_cast<const SMDS_MeshEdge*>( e );
      break;
    }
  }
  //PROFILER_Get(1);
  return toReturn;
}


//=======================================================================
//function : FindEdgeOrCreate
//purpose  :
//=======================================================================

SMDS_MeshEdge* SMDS_Mesh::FindEdgeOrCreate(const SMDS_MeshNode * node1,
                                           const SMDS_MeshNode * node2)
{
  if ( !node1 || !node2) return 0;
  SMDS_MeshEdge * toReturn=NULL;
  toReturn=const_cast<SMDS_MeshEdge*>(FindEdge(node1,node2));
  if(toReturn==NULL) {
    if ( NbEdges() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
    int ID = myElementIDFactory->GetFreeID(); // -PR- voir si on range cet element
    adjustmyCellsCapacity(ID);
    myNodeIds.resize(2);
    myNodeIds[0] = node1->getVtkId();
    myNodeIds[1] = node2->getVtkId();

    SMDS_VtkEdge *edgevtk = myEdgePool->getNew();
    edgevtk->init(myNodeIds, this);
    if (!this->registerElement(ID,edgevtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(edgevtk->getVtkId(), VTK_EMPTY_CELL);
      myEdgePool->destroy(edgevtk);
      return 0;
    }
    toReturn = edgevtk;
    myCells[ID] = toReturn;
    myInfo.myNbEdges++;
  }
  return toReturn;
}


//=======================================================================
//function : FindEdge
//purpose  :
//=======================================================================

const SMDS_MeshEdge* SMDS_Mesh::FindEdge(int idnode1, int idnode2,
                                         int idnode3) const
{
  const SMDS_MeshNode * node1=FindNode(idnode1);
  const SMDS_MeshNode * node2=FindNode(idnode2);
  const SMDS_MeshNode * node3=FindNode(idnode3);
  return FindEdge(node1,node2,node3);
}

const SMDS_MeshEdge* SMDS_Mesh::FindEdge(const SMDS_MeshNode * node1,
                                         const SMDS_MeshNode * node2,
                                         const SMDS_MeshNode * node3)
{
  if ( !node1 ) return 0;
  SMDS_ElemIteratorPtr it1 = node1->GetInverseElementIterator(SMDSAbs_Edge);
  while(it1->more()) {
    const SMDS_MeshElement * e = it1->next();
    if ( e->NbNodes() == 3 ) {
      SMDS_ElemIteratorPtr it2 = e->nodesIterator();
      while(it2->more()) {
        const SMDS_MeshElement* n = it2->next();
        if( n!=node1 &&
            n!=node2 &&
            n!=node3 )
        {
          e = 0;
          break;
        }
      }
      if ( e )
        return static_cast<const SMDS_MeshEdge *> (e);
    }
  }
  return 0;
}


//=======================================================================
//function : FindFace
//purpose  :
//=======================================================================

const SMDS_MeshFace* SMDS_Mesh::FindFace(int idnode1, int idnode2,
        int idnode3) const
{
  const SMDS_MeshNode * node1=FindNode(idnode1);
  const SMDS_MeshNode * node2=FindNode(idnode2);
  const SMDS_MeshNode * node3=FindNode(idnode3);
  return FindFace(node1, node2, node3);
}

const SMDS_MeshFace* SMDS_Mesh::FindFace(const SMDS_MeshNode *node1,
                                         const SMDS_MeshNode *node2,
                                         const SMDS_MeshNode *node3)
{
  if ( !node1 ) return 0;
  SMDS_ElemIteratorPtr it1 = node1->GetInverseElementIterator(SMDSAbs_Face);
  while(it1->more()) {
    const SMDS_MeshElement * e = it1->next();
    if ( e->NbNodes() == 3 ) {
      SMDS_ElemIteratorPtr it2 = e->nodesIterator();
      while(it2->more()) {
        const SMDS_MeshElement* n = it2->next();
        if( n!=node1 &&
            n!=node2 &&
            n!=node3 )
        {
          e = 0;
          break;
        }
      }
      if ( e )
        return static_cast<const SMDS_MeshFace *> (e);
    }
  }
  return 0;
}

SMDS_MeshFace* SMDS_Mesh::FindFaceOrCreate(const SMDS_MeshNode *node1,
                                           const SMDS_MeshNode *node2,
                                           const SMDS_MeshNode *node3)
{
  SMDS_MeshFace * toReturn=NULL;
  toReturn = const_cast<SMDS_MeshFace*>(FindFace(node1,node2,node3));
  if(toReturn==NULL) {
    int ID = myElementIDFactory->GetFreeID();
    toReturn = createTriangle(node1,node2,node3, ID);
  }
  return toReturn;
}


//=======================================================================
//function : FindFace
//purpose  :
//=======================================================================

const SMDS_MeshFace* SMDS_Mesh::FindFace(int idnode1, int idnode2,
                                         int idnode3, int idnode4) const
{
  const SMDS_MeshNode * node1=FindNode(idnode1);
  const SMDS_MeshNode * node2=FindNode(idnode2);
  const SMDS_MeshNode * node3=FindNode(idnode3);
  const SMDS_MeshNode * node4=FindNode(idnode4);
  return FindFace(node1, node2, node3, node4);
}

const SMDS_MeshFace* SMDS_Mesh::FindFace(const SMDS_MeshNode *node1,
                                         const SMDS_MeshNode *node2,
                                         const SMDS_MeshNode *node3,
                                         const SMDS_MeshNode *node4)
{
  if ( !node1 ) return 0;
  SMDS_ElemIteratorPtr it1 = node1->GetInverseElementIterator(SMDSAbs_Face);
  while(it1->more()) {
    const SMDS_MeshElement * e = it1->next();
    if ( e->NbNodes() == 4 ) {
      SMDS_ElemIteratorPtr it2 = e->nodesIterator();
      while(it2->more()) {
        const SMDS_MeshElement* n = it2->next();
        if( n!=node1 &&
            n!=node2 &&
            n!=node3 &&
            n!=node4 )
        {
          e = 0;
          break;
        }
      }
      if ( e )
        return static_cast<const SMDS_MeshFace *> (e);
    }
  }
  return 0;
}

SMDS_MeshFace* SMDS_Mesh::FindFaceOrCreate(const SMDS_MeshNode *node1,
                                           const SMDS_MeshNode *node2,
                                           const SMDS_MeshNode *node3,
                                           const SMDS_MeshNode *node4)
{
  SMDS_MeshFace * toReturn=NULL;
  toReturn=const_cast<SMDS_MeshFace*>(FindFace(node1,node2,node3,node4));
  if(toReturn==NULL) {
    int ID = myElementIDFactory->GetFreeID();
    toReturn=createQuadrangle(node1,node2,node3,node4,ID);
  }
  return toReturn;
}


//=======================================================================
//function : FindFace
//purpose  :quadratic triangle
//=======================================================================

const SMDS_MeshFace* SMDS_Mesh::FindFace(int idnode1, int idnode2,
                                         int idnode3, int idnode4,
                                         int idnode5, int idnode6) const
{
  const SMDS_MeshNode * node1 = FindNode(idnode1);
  const SMDS_MeshNode * node2 = FindNode(idnode2);
  const SMDS_MeshNode * node3 = FindNode(idnode3);
  const SMDS_MeshNode * node4 = FindNode(idnode4);
  const SMDS_MeshNode * node5 = FindNode(idnode5);
  const SMDS_MeshNode * node6 = FindNode(idnode6);
  return FindFace(node1, node2, node3, node4, node5, node6);
}

const SMDS_MeshFace* SMDS_Mesh::FindFace(const SMDS_MeshNode *node1,
                                         const SMDS_MeshNode *node2,
                                         const SMDS_MeshNode *node3,
                                         const SMDS_MeshNode *node4,
                                         const SMDS_MeshNode *node5,
                                         const SMDS_MeshNode *node6)
{
  if ( !node1 ) return 0;
  SMDS_ElemIteratorPtr it1 = node1->GetInverseElementIterator(SMDSAbs_Face);
  while(it1->more()) {
    const SMDS_MeshElement * e = it1->next();
    if ( e->NbNodes() == 6 ) {
      SMDS_ElemIteratorPtr it2 = e->nodesIterator();
      while(it2->more()) {
        const SMDS_MeshElement* n = it2->next();
        if( n!=node1 &&
            n!=node2 &&
            n!=node3 &&
            n!=node4 &&
            n!=node5 &&
            n!=node6 )
        {
          e = 0;
          break;
        }
      }
      if ( e )
        return static_cast<const SMDS_MeshFace *> (e);
    }
  }
  return 0;
}


//=======================================================================
//function : FindFace
//purpose  : quadratic quadrangle
//=======================================================================

const SMDS_MeshFace* SMDS_Mesh::FindFace(int idnode1, int idnode2,
                                         int idnode3, int idnode4,
                                         int idnode5, int idnode6,
                                         int idnode7, int idnode8) const
{
  const SMDS_MeshNode * node1 = FindNode(idnode1);
  const SMDS_MeshNode * node2 = FindNode(idnode2);
  const SMDS_MeshNode * node3 = FindNode(idnode3);
  const SMDS_MeshNode * node4 = FindNode(idnode4);
  const SMDS_MeshNode * node5 = FindNode(idnode5);
  const SMDS_MeshNode * node6 = FindNode(idnode6);
  const SMDS_MeshNode * node7 = FindNode(idnode7);
  const SMDS_MeshNode * node8 = FindNode(idnode8);
  return FindFace(node1, node2, node3, node4, node5, node6, node7, node8);
}

const SMDS_MeshFace* SMDS_Mesh::FindFace(const SMDS_MeshNode *node1,
                                         const SMDS_MeshNode *node2,
                                         const SMDS_MeshNode *node3,
                                         const SMDS_MeshNode *node4,
                                         const SMDS_MeshNode *node5,
                                         const SMDS_MeshNode *node6,
                                         const SMDS_MeshNode *node7,
                                         const SMDS_MeshNode *node8)
{
  if ( !node1 ) return 0;
  SMDS_ElemIteratorPtr it1 = node1->GetInverseElementIterator(SMDSAbs_Face);
  while(it1->more()) {
    const SMDS_MeshElement * e = it1->next();
    if ( e->NbNodes() == 8 ) {
      SMDS_ElemIteratorPtr it2 = e->nodesIterator();
      while(it2->more()) {
        const SMDS_MeshElement* n = it2->next();
        if( n!=node1 &&
            n!=node2 &&
            n!=node3 &&
            n!=node4 &&
            n!=node5 &&
            n!=node6 &&
            n!=node7 &&
            n!=node8 )
        {
          e = 0;
          break;
        }
      }
      if ( e )
        return static_cast<const SMDS_MeshFace *> (e);
    }
  }
  return 0;
}


//=======================================================================
//function : FindElement
//purpose  :
//=======================================================================

const SMDS_MeshElement* SMDS_Mesh::FindElement(int IDelem) const
{
  if ((IDelem <= 0) || IDelem >= myCells.size())
  {
    MESSAGE("--------------------------------------------------------------------------------- ");
    MESSAGE("----------------------------------- bad IDelem " << IDelem << " " << myCells.size());
    MESSAGE("--------------------------------------------------------------------------------- ");
    // TODO raise an exception
    //assert(0);
    return 0;
  }
  return myCells[IDelem];
}

//=======================================================================
//function : FindFace
//purpose  : find polygon
//=======================================================================

const SMDS_MeshFace* SMDS_Mesh::FindFace (const vector<int>& nodes_ids) const
{
  int nbnodes = nodes_ids.size();
  vector<const SMDS_MeshNode *> poly_nodes (nbnodes);
  for (int inode = 0; inode < nbnodes; inode++) {
    const SMDS_MeshNode * node = FindNode(nodes_ids[inode]);
    if (node == NULL) return NULL;
    poly_nodes[inode] = node;
  }
  return FindFace(poly_nodes);
}

const SMDS_MeshFace* SMDS_Mesh::FindFace (const vector<const SMDS_MeshNode *>& nodes)
{
  return (const SMDS_MeshFace*) FindElement( nodes, SMDSAbs_Face );
}


//================================================================================
/*!
 * \brief Return element based on all given nodes
 *  \param nodes - node of element
 *  \param type - type of element
 *  \param noMedium - true if medium nodes of quadratic element are not included in <nodes>
 *  \retval const SMDS_MeshElement* - found element or NULL
 */
//================================================================================

const SMDS_MeshElement* SMDS_Mesh::FindElement (const vector<const SMDS_MeshNode *>& nodes,
                                                const SMDSAbs_ElementType            type,
                                                const bool                           noMedium)
{
  if ( nodes.size() > 0 && nodes[0] )
  {
    SMDS_ElemIteratorPtr itF = nodes[0]->GetInverseElementIterator(type);
    while (itF->more())
    {
      const SMDS_MeshElement* e = itF->next();
      int nbNodesToCheck = noMedium ? e->NbCornerNodes() : e->NbNodes();
      if ( nbNodesToCheck == nodes.size() )
      {
        for ( size_t i = 1; e && i < nodes.size(); ++i )
        {
          int nodeIndex = e->GetNodeIndex( nodes[ i ]);
          if ( nodeIndex < 0 || nodeIndex >= nbNodesToCheck )
            e = 0;
        }
        if ( e )
          return e;
      }
    }
  }
  return NULL;
}

//=======================================================================
//function : DumpNodes
//purpose  :
//=======================================================================

void SMDS_Mesh::DumpNodes() const
{
        MESSAGE("dump nodes of mesh : ");
        SMDS_NodeIteratorPtr itnode=nodesIterator();
        while(itnode->more()) ; //MESSAGE(itnode->next());
}

//=======================================================================
//function : Dump0DElements
//purpose  :
//=======================================================================
void SMDS_Mesh::Dump0DElements() const
{
  MESSAGE("dump 0D elements of mesh : ");
  SMDS_ElemIteratorPtr it0d = elementsIterator(SMDSAbs_0DElement);
  while(it0d->more()) ; //MESSAGE(it0d->next());
}

//=======================================================================
//function : DumpEdges
//purpose  :
//=======================================================================

void SMDS_Mesh::DumpEdges() const
{
        MESSAGE("dump edges of mesh : ");
        SMDS_EdgeIteratorPtr itedge=edgesIterator();
        while(itedge->more()) ; //MESSAGE(itedge->next());
}

//=======================================================================
//function : DumpFaces
//purpose  :
//=======================================================================

void SMDS_Mesh::DumpFaces() const
{
        MESSAGE("dump faces of mesh : ");
        SMDS_FaceIteratorPtr itface=facesIterator();
        while(itface->more()) ; //MESSAGE(itface->next());
}

//=======================================================================
//function : DumpVolumes
//purpose  :
//=======================================================================

void SMDS_Mesh::DumpVolumes() const
{
        MESSAGE("dump volumes of mesh : ");
        SMDS_VolumeIteratorPtr itvol=volumesIterator();
        while(itvol->more()) ; //MESSAGE(itvol->next());
}

//=======================================================================
//function : DebugStats
//purpose  :
//=======================================================================

void SMDS_Mesh::DebugStats() const
{
  MESSAGE("Debug stats of mesh : ");

  MESSAGE("===== NODES ====="<<NbNodes());
  MESSAGE("===== 0DELEMS ====="<<Nb0DElements());
  MESSAGE("===== EDGES ====="<<NbEdges());
  MESSAGE("===== FACES ====="<<NbFaces());
  MESSAGE("===== VOLUMES ====="<<NbVolumes());

  MESSAGE("End Debug stats of mesh ");

  //#ifdef DEB

  SMDS_NodeIteratorPtr itnode=nodesIterator();
  int sizeofnodes = 0;
  int sizeoffaces = 0;

  while(itnode->more())
  {
    const SMDS_MeshNode *node = itnode->next();

    sizeofnodes += sizeof(*node);

    SMDS_ElemIteratorPtr it = node->GetInverseElementIterator();
    while(it->more())
    {
      const SMDS_MeshElement *me = it->next();
      sizeofnodes += sizeof(me);
    }
  }

  SMDS_FaceIteratorPtr itface=facesIterator();
  while(itface->more())
  {
    const SMDS_MeshElement *face = itface->next();
    sizeoffaces += sizeof(*face);
  }

  MESSAGE("total size of node elements = " << sizeofnodes);;
  MESSAGE("total size of face elements = " << sizeoffaces);;

  //#endif
}

///////////////////////////////////////////////////////////////////////////////
/// Return the number of nodes
///////////////////////////////////////////////////////////////////////////////
int SMDS_Mesh::NbNodes() const
{
        //MESSAGE(myGrid->GetNumberOfPoints());
        //MESSAGE(myInfo.NbNodes());
        //MESSAGE(myNodeMax);
    return myInfo.NbNodes();
}

///////////////////////////////////////////////////////////////////////////////
/// Return the number of 0D elements
///////////////////////////////////////////////////////////////////////////////
int SMDS_Mesh::Nb0DElements() const
{
  return myInfo.Nb0DElements();
}

///////////////////////////////////////////////////////////////////////////////
/// Return the number of 0D elements
///////////////////////////////////////////////////////////////////////////////
int SMDS_Mesh::NbBalls() const
{
  return myInfo.NbBalls();
}

///////////////////////////////////////////////////////////////////////////////
/// Return the number of edges (including construction edges)
///////////////////////////////////////////////////////////////////////////////
int SMDS_Mesh::NbEdges() const
{
  return myInfo.NbEdges();
}

///////////////////////////////////////////////////////////////////////////////
/// Return the number of faces (including construction faces)
///////////////////////////////////////////////////////////////////////////////
int SMDS_Mesh::NbFaces() const
{
  return myInfo.NbFaces();
}

///////////////////////////////////////////////////////////////////////////////
/// Return the number of volumes
///////////////////////////////////////////////////////////////////////////////
int SMDS_Mesh::NbVolumes() const
{
  return myInfo.NbVolumes();
}

///////////////////////////////////////////////////////////////////////////////
/// Return the number of child mesh of this mesh.
/// Note that the tree structure of SMDS_Mesh is unused in SMESH
///////////////////////////////////////////////////////////////////////////////
int SMDS_Mesh::NbSubMesh() const
{
  return myChildren.size();
}

///////////////////////////////////////////////////////////////////////////////
/// Destroy the mesh and all its elements
/// All pointer on elements owned by this mesh become illegals.
///////////////////////////////////////////////////////////////////////////////
SMDS_Mesh::~SMDS_Mesh()
{
  list<SMDS_Mesh*>::iterator itc=myChildren.begin();
  while(itc!=myChildren.end())
  {
    delete *itc;
    itc++;
  }

  if(myParent==NULL)
  {
    delete myNodeIDFactory;
    delete myElementIDFactory;
  }
  else
  {
    SMDS_ElemIteratorPtr eIt = elementsIterator();
    while ( eIt->more() )
      {
        const SMDS_MeshElement *elem = eIt->next();
        myElementIDFactory->ReleaseID(elem->GetID(), elem->getVtkId());
      }
    SMDS_NodeIteratorPtr itn = nodesIterator();
    while (itn->more())
      {
        const SMDS_MeshNode *node = itn->next();
        ((SMDS_MeshNode*)node)->SetPosition(SMDS_SpacePosition::originSpacePosition());
        myNodeIDFactory->ReleaseID(node->GetID(), node->getVtkId());
      }
  }
  myGrid->Delete();

  delete myNodePool;
  delete myVolumePool;
  delete myFacePool;
  delete myEdgePool;
  delete myBallPool;
}

//================================================================================
/*!
 * \brief Clear all data
 */
//================================================================================

void SMDS_Mesh::Clear()
{
  MESSAGE("SMDS_Mesh::Clear");
  if (myParent!=NULL)
    {
    SMDS_ElemIteratorPtr eIt = elementsIterator();
    while ( eIt->more() )
      {
        const SMDS_MeshElement *elem = eIt->next();
        myElementIDFactory->ReleaseID(elem->GetID(), elem->getVtkId());
      }
    SMDS_NodeIteratorPtr itn = nodesIterator();
    while (itn->more())
      {
        const SMDS_MeshNode *node = itn->next();
        myNodeIDFactory->ReleaseID(node->GetID(), node->getVtkId());
      }
    }
  else
    {
    myNodeIDFactory->Clear();
    myElementIDFactory->Clear();
    }

  // SMDS_ElemIteratorPtr itv = elementsIterator();
  // while (itv->more())
  //   {
  //     SMDS_MeshElement* elem = (SMDS_MeshElement*)(itv->next());
  //     SMDSAbs_ElementType aType = elem->GetType();
  //     switch (aType)
  //     {
  //       case SMDSAbs_0DElement:
  //         delete elem;
  //         break;
  //       case SMDSAbs_Edge:
  //          myEdgePool->destroy(static_cast<SMDS_VtkEdge*>(elem));
  //         break;
  //       case SMDSAbs_Face:
  //         myFacePool->destroy(static_cast<SMDS_VtkFace*>(elem));
  //         break;
  //       case SMDSAbs_Volume:
  //         myVolumePool->destroy(static_cast<SMDS_VtkVolume*>(elem));
  //         break;
  //       case SMDSAbs_Ball:
  //         myBallPool->destroy(static_cast<SMDS_BallElement*>(elem));
  //         break;
  //       default:
  //         break;
  //     }
  //   }
  myVolumePool->clear();
  myFacePool->clear();
  myEdgePool->clear();
  myBallPool->clear();

  clearVector( myCells );
  clearVector( myCellIdVtkToSmds );

  SMDS_NodeIteratorPtr itn = nodesIterator();
  while (itn->more())
    {
      SMDS_MeshNode *node = (SMDS_MeshNode*)(itn->next());
      node->SetPosition(SMDS_SpacePosition::originSpacePosition());
      //myNodePool->destroy(node);
    }
  myNodePool->clear();
  clearVector( myNodes );

  list<SMDS_Mesh*>::iterator itc=myChildren.begin();
  while(itc!=myChildren.end())
    (*itc)->Clear();

  myModified = false;
  myModifTime++;
  xmin = 0; xmax = 0;
  ymin = 0; ymax = 0;
  zmin = 0; zmax = 0;

  myInfo.Clear();

  myGrid->Initialize();
  myGrid->Allocate();
  vtkPoints* points = vtkPoints::New();
  // rnv: to fix bug "21125: EDF 1233 SMESH: Degrardation of precision in a test case for quadratic conversion"
  // using double type for storing coordinates of nodes instead float.
  points->SetDataType(VTK_DOUBLE);
  points->SetNumberOfPoints(0 /*SMDS_Mesh::chunkSize*/);
  myGrid->SetPoints( points );
  points->Delete();
  myGrid->BuildLinks();
}

///////////////////////////////////////////////////////////////////////////////
/// Return true if this mesh create faces with edges.
/// A false returned value mean that faces are created with nodes. A concequence
/// is, iteration on edges (SMDS_Element::edgesIterator) will be unavailable.
///////////////////////////////////////////////////////////////////////////////
bool SMDS_Mesh::hasConstructionEdges()
{
        return myHasConstructionEdges;
}

///////////////////////////////////////////////////////////////////////////////
/// Return true if this mesh create volumes with faces
/// A false returned value mean that volumes are created with nodes or edges.
/// (see hasConstructionEdges)
/// A concequence is, iteration on faces (SMDS_Element::facesIterator) will be
/// unavailable.
///////////////////////////////////////////////////////////////////////////////
bool SMDS_Mesh::hasConstructionFaces()
{
        return myHasConstructionFaces;
}

///////////////////////////////////////////////////////////////////////////////
/// Return true if nodes are linked to the finit elements, they are belonging to.
/// Currently, It always return true.
///////////////////////////////////////////////////////////////////////////////
bool SMDS_Mesh::hasInverseElements()
{
        return myHasInverseElements;
}

///////////////////////////////////////////////////////////////////////////////
/// Make this mesh creating construction edges (see hasConstructionEdges)
/// @param b true to have construction edges, else false.
///////////////////////////////////////////////////////////////////////////////
void SMDS_Mesh::setConstructionEdges(bool b)
{
        myHasConstructionEdges=b;
}

///////////////////////////////////////////////////////////////////////////////
/// Make this mesh creating construction faces (see hasConstructionFaces)
/// @param b true to have construction faces, else false.
///////////////////////////////////////////////////////////////////////////////
void SMDS_Mesh::setConstructionFaces(bool b)
{
         myHasConstructionFaces=b;
}

///////////////////////////////////////////////////////////////////////////////
/// Make this mesh creating link from nodes to elements (see hasInverseElements)
/// @param b true to link nodes to elements, else false.
///////////////////////////////////////////////////////////////////////////////
void SMDS_Mesh::setInverseElements(bool b)
{
  if(!b) MESSAGE("Error : inverseElement=false not implemented");
  myHasInverseElements=b;
}

namespace {

  //================================================================================
  /*!
   * \brief Iterator on elements in id increasing order
   */
  //================================================================================

  template <typename ELEM=const SMDS_MeshElement*>
  class IdSortedIterator : public SMDS_Iterator<ELEM>
  {
    SMDS_MeshElementIDFactory&       myIDFact;
    int                              myID, myMaxID, myNbFound, myTotalNb;
    SMDSAbs_ElementType              myType;
    ELEM                             myElem;

  public:
    IdSortedIterator(SMDS_MeshElementIDFactory& fact,
                     const SMDSAbs_ElementType        type, // SMDSAbs_All NOT allowed!!! 
                     const int                        totalNb)
      :myIDFact( fact ),
       myID(1), myMaxID( myIDFact.GetMaxID() ),myNbFound(0), myTotalNb( totalNb ),
       myType( type ),
       myElem(0)
    {
      next();
    }
    bool more()
    {
      return myElem;
    }
    ELEM next()
    {
      ELEM current = myElem;

      for ( myElem = 0;  !myElem && myNbFound < myTotalNb && myID <= myMaxID; ++myID )
        if ((myElem = (ELEM) myIDFact.MeshElement( myID ))
            && myElem->GetType() != myType )
          myElem = 0;

      myNbFound += bool(myElem);

      return current;
    }
  };

  //================================================================================
  /*!
   * \brief Iterator on vector of elements, possibly being resized while iteration
   */
  //================================================================================

  template<typename RETURN_VALUE,
           typename VECTOR_VALUE=SMDS_MeshCell*,
           typename VALUE_FILTER=SMDS::NonNullFilter<VECTOR_VALUE> >
  class ElemVecIterator: public SMDS_Iterator<RETURN_VALUE>
  {
    const std::vector<VECTOR_VALUE>& _vector;
    size_t                           _index;
    bool                             _more;
    VALUE_FILTER                     _filter;
  public:
    ElemVecIterator(const std::vector<VECTOR_VALUE>& vec,
                    const VALUE_FILTER&              filter=VALUE_FILTER() )
      :_vector( vec ), _index(0), _more( !vec.empty() ), _filter( filter )
    {
      if ( _more && !_filter( _vector[ _index ]))
        next();
    }
    virtual bool more()
    {
      return _more;
    }
    virtual RETURN_VALUE next()
    {
      if ( !_more ) return NULL;
      VECTOR_VALUE current = _vector[ _index ];
      _more = 0;
      while ( !_more && ++_index < _vector.size() )
        _more = _filter( _vector[ _index ]);
      return (RETURN_VALUE) current;
    }
  };
}

///////////////////////////////////////////////////////////////////////////////
/// Return an iterator on nodes of the current mesh factory
///////////////////////////////////////////////////////////////////////////////

SMDS_NodeIteratorPtr SMDS_Mesh::nodesIterator(bool idInceasingOrder) const
{
  // naturally always sorted by ID
  typedef ElemVecIterator<const SMDS_MeshNode*, SMDS_MeshNode*> TIterator;
  return SMDS_NodeIteratorPtr( new TIterator(myNodes));
}

SMDS_ElemIteratorPtr SMDS_Mesh::elementGeomIterator(SMDSAbs_GeometryType type) const
{
  // naturally always sorted by ID
  typedef ElemVecIterator
    < const SMDS_MeshElement*, SMDS_MeshCell*, SMDS_MeshElement::GeomFilter > TIterator;
  return SMDS_ElemIteratorPtr
    (new TIterator(myCells, SMDS_MeshElement::GeomFilter( type )));
}

SMDS_ElemIteratorPtr SMDS_Mesh::elementEntityIterator(SMDSAbs_EntityType type) const
{
  if ( type == SMDSEntity_Node )
  {
    typedef ElemVecIterator<const SMDS_MeshElement*, SMDS_MeshNode*> TIterator;
    return SMDS_ElemIteratorPtr( new TIterator(myNodes));
  }
  // naturally always sorted by ID
  typedef ElemVecIterator
    < const SMDS_MeshElement*, SMDS_MeshCell*, SMDS_MeshElement::EntityFilter > TIterator;
  return SMDS_ElemIteratorPtr
    (new TIterator(myCells, SMDS_MeshElement::EntityFilter( type )));
}

///////////////////////////////////////////////////////////////////////////////
/// Return an iterator on elements of the current mesh factory
///////////////////////////////////////////////////////////////////////////////
SMDS_ElemIteratorPtr SMDS_Mesh::elementsIterator(SMDSAbs_ElementType type) const
{
  // naturally always sorted by ID
  switch ( type ) {

  case SMDSAbs_All:
    return SMDS_ElemIteratorPtr (new ElemVecIterator<const SMDS_MeshElement*>(myCells));

  case SMDSAbs_Node:
    return SMDS_ElemIteratorPtr
      ( new ElemVecIterator<const SMDS_MeshElement*, SMDS_MeshNode*>( myNodes ));

  default:
    typedef ElemVecIterator
      < const SMDS_MeshElement*, SMDS_MeshCell*, SMDS_MeshElement::TypeFilter > TIterator;
    return SMDS_ElemIteratorPtr (new TIterator(myCells, SMDS_MeshElement::TypeFilter( type )));
  }
  return SMDS_ElemIteratorPtr();
}

///////////////////////////////////////////////////////////////////////////////
///Return an iterator on edges of the current mesh.
///////////////////////////////////////////////////////////////////////////////

SMDS_EdgeIteratorPtr SMDS_Mesh::edgesIterator(bool idInceasingOrder) const
{
  // naturally always sorted by ID
  typedef ElemVecIterator
    < const SMDS_MeshEdge*, SMDS_MeshCell*, SMDS_MeshElement::TypeFilter > TIterator;
  return SMDS_EdgeIteratorPtr
    (new TIterator(myCells, SMDS_MeshElement::TypeFilter( SMDSAbs_Edge )));
}

///////////////////////////////////////////////////////////////////////////////
///Return an iterator on faces of the current mesh.
///////////////////////////////////////////////////////////////////////////////

SMDS_FaceIteratorPtr SMDS_Mesh::facesIterator(bool idInceasingOrder) const
{
  // naturally always sorted by ID
  typedef ElemVecIterator
    < const SMDS_MeshFace*, SMDS_MeshCell*, SMDS_MeshElement::TypeFilter > TIterator;
  return SMDS_FaceIteratorPtr
    (new TIterator(myCells, SMDS_MeshElement::TypeFilter( SMDSAbs_Face )));
}

///////////////////////////////////////////////////////////////////////////////
///Return an iterator on volumes of the current mesh.
///////////////////////////////////////////////////////////////////////////////

SMDS_VolumeIteratorPtr SMDS_Mesh::volumesIterator(bool idInceasingOrder) const
{
  // naturally always sorted by ID
  typedef ElemVecIterator
    < const SMDS_MeshVolume*, SMDS_MeshCell*, SMDS_MeshElement::TypeFilter > TIterator;
  return SMDS_VolumeIteratorPtr
    (new TIterator(myCells, SMDS_MeshElement::TypeFilter( SMDSAbs_Volume )));
}

///////////////////////////////////////////////////////////////////////////////
/// Do intersection of sets (more than 2)
///////////////////////////////////////////////////////////////////////////////
static set<const SMDS_MeshElement*> * intersectionOfSets(
        set<const SMDS_MeshElement*> vs[], int numberOfSets)
{
        set<const SMDS_MeshElement*>* rsetA=new set<const SMDS_MeshElement*>(vs[0]);
        set<const SMDS_MeshElement*>* rsetB;

        for(int i=0; i<numberOfSets-1; i++)
        {
                rsetB=new set<const SMDS_MeshElement*>();
                set_intersection(
                        rsetA->begin(), rsetA->end(),
                        vs[i+1].begin(), vs[i+1].end(),
                        inserter(*rsetB, rsetB->begin()));
                delete rsetA;
                rsetA=rsetB;
        }
        return rsetA;
}

///////////////////////////////////////////////////////////////////////////////
/// Return the list of finite elements owning the given element: elements
/// containing all the nodes of the given element, for instance faces and
/// volumes containing a given edge.
///////////////////////////////////////////////////////////////////////////////
static set<const SMDS_MeshElement*> * getFinitElements(const SMDS_MeshElement * element)
{
        int numberOfSets=element->NbNodes();
        set<const SMDS_MeshElement*> *initSet = new set<const SMDS_MeshElement*>[numberOfSets];

        SMDS_ElemIteratorPtr itNodes=element->nodesIterator();

        int i=0;
        while(itNodes->more())
        {
          const SMDS_MeshElement* node = itNodes->next();
          MYASSERT(node);
                const SMDS_MeshNode * n=static_cast<const SMDS_MeshNode*>(node);
                SMDS_ElemIteratorPtr itFe = n->GetInverseElementIterator();

                //initSet[i]=set<const SMDS_MeshElement*>();
                while(itFe->more())
                {
                  const SMDS_MeshElement* elem = itFe->next();
                  MYASSERT(elem);
                  initSet[i].insert(elem);

                }

                i++;
        }
        set<const SMDS_MeshElement*> *retSet=intersectionOfSets(initSet, numberOfSets);
//         MESSAGE("nb elems " << i << " intersection " << retSet->size());
        delete [] initSet;
        return retSet;
}

///////////////////////////////////////////////////////////////////////////////
/// Return the list of nodes used only by the given elements
///////////////////////////////////////////////////////////////////////////////
static set<const SMDS_MeshElement*> * getExclusiveNodes(
        set<const SMDS_MeshElement*>& elements)
{
        set<const SMDS_MeshElement*> * toReturn=new set<const SMDS_MeshElement*>();
        set<const SMDS_MeshElement*>::iterator itElements=elements.begin();

        while(itElements!=elements.end())
        {
                SMDS_ElemIteratorPtr itNodes = (*itElements)->nodesIterator();
                itElements++;

                while(itNodes->more())
                {
                        const SMDS_MeshNode * n=static_cast<const SMDS_MeshNode*>(itNodes->next());
                        SMDS_ElemIteratorPtr itFe = n->GetInverseElementIterator();
                        set<const SMDS_MeshElement*> s;
                        while(itFe->more())
                          s.insert(itFe->next());
                        if(s==elements) toReturn->insert(n);
                }
        }
        return toReturn;
}

///////////////////////////////////////////////////////////////////////////////
///Find the children of an element that are made of given nodes
///@param setOfChildren The set in which matching children will be inserted
///@param element The element were to search matching children
///@param nodes The nodes that the children must have to be selected
///////////////////////////////////////////////////////////////////////////////
void SMDS_Mesh::addChildrenWithNodes(set<const SMDS_MeshElement*>& setOfChildren,
                                     const SMDS_MeshElement *      element,
                                     set<const SMDS_MeshElement*>& nodes)
{
  switch(element->GetType())
    {
    case SMDSAbs_Node:
      MESSAGE("Internal Error: This should not happen");
      break;
    case SMDSAbs_0DElement:
      {
      }
      break;
    case SMDSAbs_Edge:
        {
                SMDS_ElemIteratorPtr itn=element->nodesIterator();
                while(itn->more())
                {
                        const SMDS_MeshElement * e=itn->next();
                        if(nodes.find(e)!=nodes.end())
                        {
                          setOfChildren.insert(element);
                          break;
                        }
                }
        } break;
    case SMDSAbs_Face:
        {
                SMDS_ElemIteratorPtr itn=element->nodesIterator();
                while(itn->more())
                {
                        const SMDS_MeshElement * e=itn->next();
                        if(nodes.find(e)!=nodes.end())
                        {
                          setOfChildren.insert(element);
                          break;
                        }
                }
                if(hasConstructionEdges())
                {
                        SMDS_ElemIteratorPtr ite=element->edgesIterator();
                        while(ite->more())
                                addChildrenWithNodes(setOfChildren, ite->next(), nodes);
                }
        } break;
    case SMDSAbs_Volume:
        {
                if(hasConstructionFaces())
                {
                        SMDS_ElemIteratorPtr ite=element->facesIterator();
                        while(ite->more())
                                addChildrenWithNodes(setOfChildren, ite->next(), nodes);
                }
                else if(hasConstructionEdges())
                {
                        SMDS_ElemIteratorPtr ite=element->edgesIterator();
                        while(ite->more())
                                addChildrenWithNodes(setOfChildren, ite->next(), nodes);
                }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///@param elem The element to delete
///@param removenodes if true remaining nodes will be removed
///////////////////////////////////////////////////////////////////////////////
void SMDS_Mesh::RemoveElement(const SMDS_MeshElement * elem,
                              const bool removenodes)
{
  list<const SMDS_MeshElement *> removedElems;
  list<const SMDS_MeshElement *> removedNodes;
  RemoveElement( elem, removedElems, removedNodes, removenodes );
}

///////////////////////////////////////////////////////////////////////////////
///@param elem The element to delete
///@param removedElems to be filled with all removed elements
///@param removedNodes to be filled with all removed nodes
///@param removenodes if true remaining nodes will be removed
///////////////////////////////////////////////////////////////////////////////
void SMDS_Mesh::RemoveElement(const SMDS_MeshElement *        elem,
                              list<const SMDS_MeshElement *>& removedElems,
                              list<const SMDS_MeshElement *>& removedNodes,
                              bool                            removenodes)
{
  //MESSAGE("SMDS_Mesh::RemoveElement " << elem->getVtkId() << " " << removenodes);
  // get finite elements built on elem
  set<const SMDS_MeshElement*> * s1;
  if (    (elem->GetType() == SMDSAbs_0DElement)
      || ((elem->GetType() == SMDSAbs_Edge) && !hasConstructionEdges())
      || ((elem->GetType() == SMDSAbs_Face) && !hasConstructionFaces())
      ||  (elem->GetType() == SMDSAbs_Volume) )
    {
      s1 = new set<const SMDS_MeshElement*> ();
      s1->insert(elem);
    }
  else
    s1 = getFinitElements(elem);

  // get exclusive nodes (which would become free afterwards)
  set<const SMDS_MeshElement*> * s2;
  if (elem->GetType() == SMDSAbs_Node) // a node is removed
    {
      // do not remove nodes except elem
      s2 = new set<const SMDS_MeshElement*> ();
      s2->insert(elem);
      removenodes = true;
    }
  else
    s2 = getExclusiveNodes(*s1);

  // form the set of finite and construction elements to remove
  set<const SMDS_MeshElement*> s3;
  set<const SMDS_MeshElement*>::iterator it = s1->begin();
  while (it != s1->end())
    {
      addChildrenWithNodes(s3, *it, *s2);
      s3.insert(*it);
      it++;
    }
  if (elem->GetType() != SMDSAbs_Node)
    s3.insert(elem);

  // remove finite and construction elements
  it = s3.begin();
  while (it != s3.end())
    {
      // Remove element from <InverseElements> of its nodes
      SMDS_ElemIteratorPtr itn = (*it)->nodesIterator();
      while (itn->more())
        {
          SMDS_MeshNode * n = static_cast<SMDS_MeshNode *> (const_cast<SMDS_MeshElement *> (itn->next()));
          n->RemoveInverseElement((*it));
        }
      int IdToRemove = (*it)->GetID();
      int vtkid = (*it)->getVtkId();
      //MESSAGE("elem Id to remove " << IdToRemove << " vtkid " << vtkid <<
      //        " vtktype " << (*it)->GetVtkType() << " type " << (*it)->GetType());
      switch ((*it)->GetType())
      {
        case SMDSAbs_Node:
          MYASSERT("Internal Error: This should not happen")
          ;
          break;
        case SMDSAbs_0DElement:
          if (IdToRemove >= 0)
            {
              myCells[IdToRemove] = 0; // -PR- ici ou dans myElementIDFactory->ReleaseID ?
              myInfo.remove(*it);
            }
          removedElems.push_back((*it));
          myElementIDFactory->ReleaseID(IdToRemove, vtkid);
          delete (*it);
          break;
        case SMDSAbs_Edge:
          if (IdToRemove >= 0)
            {
              myCells[IdToRemove] = 0;
              myInfo.RemoveEdge(*it);
            }
          removedElems.push_back((*it));
          myElementIDFactory->ReleaseID(IdToRemove, vtkid);
          if (const SMDS_VtkEdge* vtkElem = dynamic_cast<const SMDS_VtkEdge*>(*it))
            myEdgePool->destroy((SMDS_VtkEdge*) vtkElem);
          else
            delete (*it);
          break;
        case SMDSAbs_Face:
          if (IdToRemove >= 0)
            {
              myCells[IdToRemove] = 0;
              myInfo.RemoveFace(*it);
            }
          removedElems.push_back((*it));
          myElementIDFactory->ReleaseID(IdToRemove, vtkid);
          if (const SMDS_VtkFace* vtkElem = dynamic_cast<const SMDS_VtkFace*>(*it))
            myFacePool->destroy((SMDS_VtkFace*) vtkElem);
          else
            delete (*it);
          break;
        case SMDSAbs_Volume:
          if (IdToRemove >= 0)
            {
              myCells[IdToRemove] = 0;
              myInfo.RemoveVolume(*it);
            }
          removedElems.push_back((*it));
          myElementIDFactory->ReleaseID(IdToRemove, vtkid);
          if (const SMDS_VtkVolume* vtkElem = dynamic_cast<const SMDS_VtkVolume*>(*it))
            myVolumePool->destroy((SMDS_VtkVolume*) vtkElem);
          else
            delete (*it);
          break;
        case SMDSAbs_Ball:
          if (IdToRemove >= 0)
            {
              myCells[IdToRemove] = 0;
              myInfo.remove(*it);
            }
          removedElems.push_back((*it));
          myElementIDFactory->ReleaseID(IdToRemove, vtkid);
          if (const SMDS_BallElement* vtkElem = dynamic_cast<const SMDS_BallElement*>(*it))
            myBallPool->destroy(const_cast<SMDS_BallElement*>( vtkElem ));
          else
            delete (*it);
          break;
      }
      if (vtkid >= 0)
        {
          //MESSAGE("VTK_EMPTY_CELL in " << vtkid);
          this->myGrid->GetCellTypesArray()->SetValue(vtkid, VTK_EMPTY_CELL);
        }
      it++;
    }

  // remove exclusive (free) nodes
  if (removenodes)
    {
      it = s2->begin();
      while (it != s2->end())
        {
          int IdToRemove = (*it)->GetID();
          //MESSAGE( "SMDS: RM node " << IdToRemove);
          if (IdToRemove >= 0)
            {
              myNodes[IdToRemove] = 0;
              myInfo.myNbNodes--;
            }
          myNodeIDFactory->ReleaseID((*it)->GetID(), (*it)->getVtkId());
          removedNodes.push_back((*it));
          if (const SMDS_MeshNode* vtkElem = dynamic_cast<const SMDS_MeshNode*>(*it))
          {
            ((SMDS_MeshNode*)vtkElem)->SetPosition(SMDS_SpacePosition::originSpacePosition());
            myNodePool->destroy((SMDS_MeshNode*) vtkElem);
          }
          else
            delete (*it);
          it++;
        }
    }

  delete s2;
  delete s1;
}


///////////////////////////////////////////////////////////////////////////////
///@param elem The element to delete
///////////////////////////////////////////////////////////////////////////////
void SMDS_Mesh::RemoveFreeElement(const SMDS_MeshElement * elem)
{
  int elemId = elem->GetID();
  int vtkId = elem->getVtkId();
  //MESSAGE("RemoveFreeElement " << elemId);
  SMDSAbs_ElementType aType = elem->GetType();
  SMDS_MeshElement* todest = (SMDS_MeshElement*)(elem);
  if (aType == SMDSAbs_Node) {
    //MESSAGE("Remove free node " << elemId);
    // only free node can be removed by this method
    const SMDS_MeshNode* n = static_cast<SMDS_MeshNode*>(todest);
    SMDS_ElemIteratorPtr itFe = n->GetInverseElementIterator();
    if (!itFe->more()) { // free node
      myNodes[elemId] = 0;
      myInfo.myNbNodes--;
      ((SMDS_MeshNode*) n)->SetPosition(SMDS_SpacePosition::originSpacePosition());
      ((SMDS_MeshNode*) n)->SMDS_MeshElement::init( -1, -1, -1 ); // avoid reuse
      myNodePool->destroy(static_cast<SMDS_MeshNode*>(todest));
      myNodeIDFactory->ReleaseID(elemId, vtkId);
    }
  } else {
    if (hasConstructionEdges() || hasConstructionFaces())
      // this methods is only for meshes without descendants
      return;

    //MESSAGE("Remove free element " << elemId);
    // Remove element from <InverseElements> of its nodes
    SMDS_ElemIteratorPtr itn = elem->nodesIterator();
    while (itn->more()) {
      SMDS_MeshNode * n = static_cast<SMDS_MeshNode *>
        (const_cast<SMDS_MeshElement *>(itn->next()));
      n->RemoveInverseElement(elem);
    }

    // in meshes without descendants elements are always free
     switch (aType) {
    case SMDSAbs_0DElement:
      myCells[elemId] = 0;
      myInfo.remove(elem);
      delete elem;
      break;
    case SMDSAbs_Edge:
      myCells[elemId] = 0;
      myInfo.RemoveEdge(elem);
      myEdgePool->destroy(static_cast<SMDS_VtkEdge*>(todest));
      break;
    case SMDSAbs_Face:
      myCells[elemId] = 0;
      myInfo.RemoveFace(elem);
      myFacePool->destroy(static_cast<SMDS_VtkFace*>(todest));
      break;
    case SMDSAbs_Volume:
      myCells[elemId] = 0;
      myInfo.RemoveVolume(elem);
      myVolumePool->destroy(static_cast<SMDS_VtkVolume*>(todest));
      break;
    case SMDSAbs_Ball:
      myCells[elemId] = 0;
      myInfo.remove(elem);
      myBallPool->destroy(static_cast<SMDS_BallElement*>(todest));
      break;
    default:
      break;
    }
    myElementIDFactory->ReleaseID(elemId, vtkId);

    this->myGrid->GetCellTypesArray()->SetValue(vtkId, VTK_EMPTY_CELL);
    // --- to do: keep vtkid in a list of reusable cells
  }
}

/*!
 * Checks if the element is present in mesh.
 * Useful to determine dead pointers.
 */
bool SMDS_Mesh::Contains (const SMDS_MeshElement* elem) const
{
  // we should not imply on validity of *elem, so iterate on containers
  // of all types in the hope of finding <elem> somewhere there
  SMDS_NodeIteratorPtr itn = nodesIterator();
  while (itn->more())
    if (elem == itn->next())
      return true;
  SMDS_ElemIteratorPtr ite = elementsIterator();
  while (ite->more())
    if (elem == ite->next())
      return true;
  return false;
}

//=======================================================================
//function : MaxNodeID
//purpose  :
//=======================================================================

int SMDS_Mesh::MaxNodeID() const
{
  return myNodeMax;
}

//=======================================================================
//function : MinNodeID
//purpose  :
//=======================================================================

int SMDS_Mesh::MinNodeID() const
{
  return myNodeMin;
}

//=======================================================================
//function : MaxElementID
//purpose  :
//=======================================================================

int SMDS_Mesh::MaxElementID() const
{
  return myElementIDFactory->GetMaxID();
}

//=======================================================================
//function : MinElementID
//purpose  :
//=======================================================================

int SMDS_Mesh::MinElementID() const
{
  return myElementIDFactory->GetMinID();
}

//=======================================================================
//function : Renumber
//purpose  : Renumber all nodes or elements.
//=======================================================================

void SMDS_Mesh::Renumber (const bool isNodes, const int  startID, const int  deltaID)
{
    MESSAGE("Renumber");
  if ( deltaID == 0 )
    return;

  SMDS_MeshNodeIDFactory * idFactory =
    isNodes ? myNodeIDFactory : myElementIDFactory;

  // get existing elements in the order of ID increasing
  map<int,SMDS_MeshElement*> elemMap;
  SMDS_ElemIteratorPtr idElemIt = idFactory->elementsIterator();
  while ( idElemIt->more() ) {
    SMDS_MeshElement* elem = const_cast<SMDS_MeshElement*>(idElemIt->next());
    int id = elem->GetID();
    elemMap.insert(map<int,SMDS_MeshElement*>::value_type(id, elem));
  }
  // release their ids
  map<int,SMDS_MeshElement*>::iterator elemIt = elemMap.begin();
  idFactory->Clear();
//   for ( ; elemIt != elemMap.end(); elemIt++ )
//   {
//     int id = (*elemIt).first;
//     idFactory->ReleaseID( id );
//   }
  // set new IDs
  int ID = startID;
  elemIt = elemMap.begin();
  for ( ; elemIt != elemMap.end(); elemIt++ )
  {
    idFactory->BindID( ID, (*elemIt).second );
    ID += deltaID;
  }
}

//=======================================================================
//function : GetElementType
//purpose  : Return type of element or node with id
//=======================================================================

SMDSAbs_ElementType SMDS_Mesh::GetElementType( const int id, const bool iselem ) const
{
  SMDS_MeshElement* elem = 0;
  if( iselem )
    elem = myElementIDFactory->MeshElement( id );
  else
    elem = myNodeIDFactory->MeshElement( id );

  if( !elem )
  {
    //throw SALOME_Exception(LOCALIZED ("this element isn't exist"));
    return SMDSAbs_All;
  }
  else
    return elem->GetType();
}



//********************************************************************
//********************************************************************
//********                                                   *********
//*****       Methods for addition of quadratic elements        ******
//********                                                   *********
//********************************************************************
//********************************************************************

//=======================================================================
//function : AddEdgeWithID
//purpose  :
//=======================================================================
SMDS_MeshEdge* SMDS_Mesh::AddEdgeWithID(int n1, int n2, int n12, int ID)
{
  return SMDS_Mesh::AddEdgeWithID
    ((SMDS_MeshNode*) myNodeIDFactory->MeshElement(n1),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n2),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n12),
     ID);
}

//=======================================================================
//function : AddEdge
//purpose  :
//=======================================================================
SMDS_MeshEdge* SMDS_Mesh::AddEdge(const SMDS_MeshNode* n1,
                                  const SMDS_MeshNode* n2,
                                  const SMDS_MeshNode* n12)
{
  return SMDS_Mesh::AddEdgeWithID(n1, n2, n12, myElementIDFactory->GetFreeID());
}

//=======================================================================
//function : AddEdgeWithID
//purpose  :
//=======================================================================
SMDS_MeshEdge* SMDS_Mesh::AddEdgeWithID(const SMDS_MeshNode * n1,
                                        const SMDS_MeshNode * n2,
                                        const SMDS_MeshNode * n12,
                                        int ID)
{
  if ( !n1 || !n2 || !n12 ) return 0;

  // --- retrieve nodes ID
  myNodeIds.resize(3);
  myNodeIds[0] = n1->getVtkId();
  myNodeIds[1] = n2->getVtkId();
  myNodeIds[2] = n12->getVtkId();

  SMDS_MeshEdge * edge = 0;
  SMDS_VtkEdge *edgevtk = myEdgePool->getNew();
  edgevtk->init(myNodeIds, this);
  if (!this->registerElement(ID,edgevtk))
  {
    this->myGrid->GetCellTypesArray()->SetValue(edgevtk->getVtkId(), VTK_EMPTY_CELL);
    myEdgePool->destroy(edgevtk);
    return 0;
  }
  edge = edgevtk;
  adjustmyCellsCapacity(ID);
  myCells[ID] = edge;
  myInfo.myNbQuadEdges++;

  //  if (!registerElement(ID, edge)) {
  //        RemoveElement(edge, false);
  //        edge = NULL;
  //  }
  return edge;

}


//=======================================================================
//function : AddFace
//purpose  :
//=======================================================================
SMDS_MeshFace* SMDS_Mesh::AddFace(const SMDS_MeshNode * n1,
                                  const SMDS_MeshNode * n2,
                                  const SMDS_MeshNode * n3,
                                  const SMDS_MeshNode * n12,
                                  const SMDS_MeshNode * n23,
                                  const SMDS_MeshNode * n31)
{
  return SMDS_Mesh::AddFaceWithID(n1,n2,n3,n12,n23,n31,
                                  myElementIDFactory->GetFreeID());
}

//=======================================================================
//function : AddFaceWithID
//purpose  :
//=======================================================================
SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(int n1, int n2, int n3,
                                        int n12,int n23,int n31, int ID)
{
  return SMDS_Mesh::AddFaceWithID
    ((SMDS_MeshNode *)myNodeIDFactory->MeshElement(n1) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n2) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n3) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n12),
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n23),
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n31),
     ID);
}

//=======================================================================
//function : AddFaceWithID
//purpose  :
//=======================================================================
SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(const SMDS_MeshNode * n1,
                                        const SMDS_MeshNode * n2,
                                        const SMDS_MeshNode * n3,
                                        const SMDS_MeshNode * n12,
                                        const SMDS_MeshNode * n23,
                                        const SMDS_MeshNode * n31,
                                        int ID)
{
  if ( !n1 || !n2 || !n3 || !n12 || !n23 || !n31) return 0;
  if(hasConstructionEdges()) {
    // creation quadratic edges - not implemented
    return 0;
  }
  else
  {
    // --- retrieve nodes ID
    myNodeIds.resize(6);
    myNodeIds[0] = n1->getVtkId();
    myNodeIds[1] = n2->getVtkId();
    myNodeIds[2] = n3->getVtkId();
    myNodeIds[3] = n12->getVtkId();
    myNodeIds[4] = n23->getVtkId();
    myNodeIds[5] = n31->getVtkId();

    SMDS_MeshFace * face = 0;
    SMDS_VtkFace *facevtk = myFacePool->getNew();
    facevtk->init(myNodeIds, this);
    if (!this->registerElement(ID,facevtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(facevtk->getVtkId(), VTK_EMPTY_CELL);
      myFacePool->destroy(facevtk);
      return 0;
    }
    face = facevtk;
    adjustmyCellsCapacity(ID);
    myCells[ID] = face;
    myInfo.myNbQuadTriangles++;

    //    if (!registerElement(ID, face)) {
    //      RemoveElement(face, false);
    //      face = NULL;
    //    }
    return face;
  }
}


//=======================================================================
//function : AddFace
//purpose  :
//=======================================================================
SMDS_MeshFace* SMDS_Mesh::AddFace(const SMDS_MeshNode * n1,
                                  const SMDS_MeshNode * n2,
                                  const SMDS_MeshNode * n3,
                                  const SMDS_MeshNode * n12,
                                  const SMDS_MeshNode * n23,
                                  const SMDS_MeshNode * n31,
                                  const SMDS_MeshNode * nCenter)
{
  return SMDS_Mesh::AddFaceWithID(n1,n2,n3,n12,n23,n31,nCenter,
                                  myElementIDFactory->GetFreeID());
}

//=======================================================================
//function : AddFaceWithID
//purpose  :
//=======================================================================
SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(int n1, int n2, int n3,
                                        int n12,int n23,int n31, int nCenter, int ID)
{
  return SMDS_Mesh::AddFaceWithID
    ((SMDS_MeshNode *)myNodeIDFactory->MeshElement(n1) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n2) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n3) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n12),
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n23),
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n31),
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(nCenter),
     ID);
}

//=======================================================================
//function : AddFaceWithID
//purpose  :
//=======================================================================
SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(const SMDS_MeshNode * n1,
                                        const SMDS_MeshNode * n2,
                                        const SMDS_MeshNode * n3,
                                        const SMDS_MeshNode * n12,
                                        const SMDS_MeshNode * n23,
                                        const SMDS_MeshNode * n31,
                                        const SMDS_MeshNode * nCenter,
                                        int ID)
{
  if ( !n1 || !n2 || !n3 || !n12 || !n23 || !n31 || !nCenter) return 0;
  if(hasConstructionEdges()) {
    // creation quadratic edges - not implemented
    return 0;
  }
  else
  {
    // --- retrieve nodes ID
    myNodeIds.resize(7);
    myNodeIds[0] = n1->getVtkId();
    myNodeIds[1] = n2->getVtkId();
    myNodeIds[2] = n3->getVtkId();
    myNodeIds[3] = n12->getVtkId();
    myNodeIds[4] = n23->getVtkId();
    myNodeIds[5] = n31->getVtkId();
    myNodeIds[6] = nCenter->getVtkId();

    SMDS_MeshFace * face = 0;
    SMDS_VtkFace *facevtk = myFacePool->getNew();
    facevtk->init(myNodeIds, this);
    if (!this->registerElement(ID,facevtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(facevtk->getVtkId(), VTK_EMPTY_CELL);
      myFacePool->destroy(facevtk);
      return 0;
    }
    face = facevtk;
    adjustmyCellsCapacity(ID);
    myCells[ID] = face;
    myInfo.myNbBiQuadTriangles++;

    //    if (!registerElement(ID, face)) {
    //      RemoveElement(face, false);
    //      face = NULL;
    //    }
    return face;
  }
}


//=======================================================================
//function : AddFace
//purpose  :
//=======================================================================
SMDS_MeshFace* SMDS_Mesh::AddFace(const SMDS_MeshNode * n1,
                                  const SMDS_MeshNode * n2,
                                  const SMDS_MeshNode * n3,
                                  const SMDS_MeshNode * n4,
                                  const SMDS_MeshNode * n12,
                                  const SMDS_MeshNode * n23,
                                  const SMDS_MeshNode * n34,
                                  const SMDS_MeshNode * n41)
{
  return SMDS_Mesh::AddFaceWithID(n1,n2,n3,n4,n12,n23,n34,n41,
                                  myElementIDFactory->GetFreeID());
}

//=======================================================================
//function : AddFaceWithID
//purpose  :
//=======================================================================
SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(int n1, int n2, int n3, int n4,
                                        int n12,int n23,int n34,int n41, int ID)
{
  return SMDS_Mesh::AddFaceWithID
    ((SMDS_MeshNode *)myNodeIDFactory->MeshElement(n1) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n2) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n3) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n4) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n12),
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n23),
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n34),
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n41),
     ID);
}

//=======================================================================
//function : AddFaceWithID
//purpose  :
//=======================================================================
SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(const SMDS_MeshNode * n1,
                                        const SMDS_MeshNode * n2,
                                        const SMDS_MeshNode * n3,
                                        const SMDS_MeshNode * n4,
                                        const SMDS_MeshNode * n12,
                                        const SMDS_MeshNode * n23,
                                        const SMDS_MeshNode * n34,
                                        const SMDS_MeshNode * n41,
                                        int ID)
{
  if ( !n1 || !n2 || !n3 || !n4 || !n12 || !n23 || !n34 || !n41) return 0;
  if(hasConstructionEdges()) {
    // creation quadratic edges - not implemented
    return 0;
  }
  else
  {
    // --- retrieve nodes ID
    myNodeIds.resize(8);
    myNodeIds[0] = n1->getVtkId();
    myNodeIds[1] = n2->getVtkId();
    myNodeIds[2] = n3->getVtkId();
    myNodeIds[3] = n4->getVtkId();
    myNodeIds[4] = n12->getVtkId();
    myNodeIds[5] = n23->getVtkId();
    myNodeIds[6] = n34->getVtkId();
    myNodeIds[7] = n41->getVtkId();

    SMDS_MeshFace * face = 0;
    SMDS_VtkFace *facevtk = myFacePool->getNew();
    facevtk->init(myNodeIds, this);
    if (!this->registerElement(ID,facevtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(facevtk->getVtkId(), VTK_EMPTY_CELL);
      myFacePool->destroy(facevtk);
      return 0;
    }
    face = facevtk;
    adjustmyCellsCapacity(ID);
    myCells[ID] = face;
    myInfo.myNbQuadQuadrangles++;

    //    if (!registerElement(ID, face)) {
    //      RemoveElement(face, false);
    //      face = NULL;
    //    }
    return face;
  }
}

//=======================================================================
//function : AddFace
//purpose  :
//=======================================================================
SMDS_MeshFace* SMDS_Mesh::AddFace(const SMDS_MeshNode * n1,
                                  const SMDS_MeshNode * n2,
                                  const SMDS_MeshNode * n3,
                                  const SMDS_MeshNode * n4,
                                  const SMDS_MeshNode * n12,
                                  const SMDS_MeshNode * n23,
                                  const SMDS_MeshNode * n34,
                                  const SMDS_MeshNode * n41,
                                  const SMDS_MeshNode * nCenter)
{
  return SMDS_Mesh::AddFaceWithID(n1,n2,n3,n4,n12,n23,n34,n41,nCenter,
                                  myElementIDFactory->GetFreeID());
}

//=======================================================================
//function : AddFaceWithID
//purpose  :
//=======================================================================
SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(int n1, int n2, int n3, int n4,
                                        int n12,int n23,int n34,int n41, int nCenter, int ID)
{
  return SMDS_Mesh::AddFaceWithID
    ((SMDS_MeshNode *)myNodeIDFactory->MeshElement(n1) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n2) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n3) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n4) ,
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n12),
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n23),
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n34),
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(n41),
     (SMDS_MeshNode *)myNodeIDFactory->MeshElement(nCenter),
     ID);
}

//=======================================================================
//function : AddFaceWithID
//purpose  :
//=======================================================================
SMDS_MeshFace* SMDS_Mesh::AddFaceWithID(const SMDS_MeshNode * n1,
                                        const SMDS_MeshNode * n2,
                                        const SMDS_MeshNode * n3,
                                        const SMDS_MeshNode * n4,
                                        const SMDS_MeshNode * n12,
                                        const SMDS_MeshNode * n23,
                                        const SMDS_MeshNode * n34,
                                        const SMDS_MeshNode * n41,
                                        const SMDS_MeshNode * nCenter,
                                        int ID)
{
  if ( !n1 || !n2 || !n3 || !n4 || !n12 || !n23 || !n34 || !n41 || !nCenter) return 0;
  if(hasConstructionEdges()) {
    // creation quadratic edges - not implemented
    return 0;
  }
  else
  {
    // --- retrieve nodes ID
    myNodeIds.resize(9);
    myNodeIds[0] = n1->getVtkId();
    myNodeIds[1] = n2->getVtkId();
    myNodeIds[2] = n3->getVtkId();
    myNodeIds[3] = n4->getVtkId();
    myNodeIds[4] = n12->getVtkId();
    myNodeIds[5] = n23->getVtkId();
    myNodeIds[6] = n34->getVtkId();
    myNodeIds[7] = n41->getVtkId();
    myNodeIds[8] = nCenter->getVtkId();

    SMDS_MeshFace * face = 0;
    SMDS_VtkFace *facevtk = myFacePool->getNew();
    facevtk->init(myNodeIds, this);
    if (!this->registerElement(ID,facevtk))
    {
      this->myGrid->GetCellTypesArray()->SetValue(facevtk->getVtkId(), VTK_EMPTY_CELL);
      myFacePool->destroy(facevtk);
      return 0;
    }
    face = facevtk;
    adjustmyCellsCapacity(ID);
    myCells[ID] = face;
    myInfo.myNbBiQuadQuadrangles++;

    //    if (!registerElement(ID, face)) {
    //      RemoveElement(face, false);
    //      face = NULL;
    //    }
    return face;
  }
}


//=======================================================================
//function : AddVolume
//purpose  :
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                      const SMDS_MeshNode * n2,
                                      const SMDS_MeshNode * n3,
                                      const SMDS_MeshNode * n4,
                                      const SMDS_MeshNode * n12,
                                      const SMDS_MeshNode * n23,
                                      const SMDS_MeshNode * n31,
                                      const SMDS_MeshNode * n14,
                                      const SMDS_MeshNode * n24,
                                      const SMDS_MeshNode * n34)
{
  int ID = myElementIDFactory->GetFreeID();
  SMDS_MeshVolume * v = SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, n12, n23,
                                                   n31, n14, n24, n34, ID);
  if(v==NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

//=======================================================================
//function : AddVolumeWithID
//purpose  :
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(int n1, int n2, int n3, int n4,
                                            int n12,int n23,int n31,
                                            int n14,int n24,int n34, int ID)
{
  return SMDS_Mesh::AddVolumeWithID
    ((SMDS_MeshNode*) myNodeIDFactory->MeshElement(n1) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n2) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n3) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n4) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n12),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n23),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n31),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n14),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n24),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n34),
     ID);
}

//=======================================================================
//function : AddVolumeWithID
//purpose  : 2d order tetrahedron of 10 nodes
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                            const SMDS_MeshNode * n2,
                                            const SMDS_MeshNode * n3,
                                            const SMDS_MeshNode * n4,
                                            const SMDS_MeshNode * n12,
                                            const SMDS_MeshNode * n23,
                                            const SMDS_MeshNode * n31,
                                            const SMDS_MeshNode * n14,
                                            const SMDS_MeshNode * n24,
                                            const SMDS_MeshNode * n34,
                                            int ID)
{
  if ( !n1 || !n2 || !n3 || !n4 || !n12 || !n23 || !n31 || !n14 || !n24 || !n34)
    return 0;
  if(hasConstructionFaces()) {
    // creation quadratic faces - not implemented
    return 0;
  }
  // --- retrieve nodes ID
  myNodeIds.resize(10);
  myNodeIds[0] = n1->getVtkId();
  myNodeIds[1] = n3->getVtkId();
  myNodeIds[2] = n2->getVtkId();
  myNodeIds[3] = n4->getVtkId();

  myNodeIds[4] = n31->getVtkId();
  myNodeIds[5] = n23->getVtkId();
  myNodeIds[6] = n12->getVtkId();

  myNodeIds[7] = n14->getVtkId();
  myNodeIds[8] = n34->getVtkId();
  myNodeIds[9] = n24->getVtkId();

  SMDS_VtkVolume *volvtk = myVolumePool->getNew();
  volvtk->init(myNodeIds, this);
  if (!this->registerElement(ID,volvtk))
  {
    this->myGrid->GetCellTypesArray()->SetValue(volvtk->getVtkId(), VTK_EMPTY_CELL);
    myVolumePool->destroy(volvtk);
    return 0;
  }
  adjustmyCellsCapacity(ID);
  myCells[ID] = volvtk;
  myInfo.myNbQuadTetras++;

  //  if (!registerElement(ID, volvtk)) {
  //    RemoveElement(volvtk, false);
  //    volvtk = NULL;
  //  }
  return volvtk;
}


//=======================================================================
//function : AddVolume
//purpose  :
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                      const SMDS_MeshNode * n2,
                                      const SMDS_MeshNode * n3,
                                      const SMDS_MeshNode * n4,
                                      const SMDS_MeshNode * n5,
                                      const SMDS_MeshNode * n12,
                                      const SMDS_MeshNode * n23,
                                      const SMDS_MeshNode * n34,
                                      const SMDS_MeshNode * n41,
                                      const SMDS_MeshNode * n15,
                                      const SMDS_MeshNode * n25,
                                      const SMDS_MeshNode * n35,
                                      const SMDS_MeshNode * n45)
{
  int ID = myElementIDFactory->GetFreeID();
  SMDS_MeshVolume * v =
    SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, n5, n12, n23, n34, n41,
                               n15, n25, n35, n45, ID);
  if(v==NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

//=======================================================================
//function : AddVolumeWithID
//purpose  :
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(int n1, int n2, int n3, int n4, int n5,
                                            int n12,int n23,int n34,int n41,
                                            int n15,int n25,int n35,int n45, int ID)
{
  return SMDS_Mesh::AddVolumeWithID
    ((SMDS_MeshNode*) myNodeIDFactory->MeshElement(n1) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n2) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n3) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n4) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n5) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n12),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n23),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n34),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n41),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n15),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n25),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n35),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n45),
     ID);
}

//=======================================================================
//function : AddVolumeWithID
//purpose  : 2d order pyramid of 13 nodes
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                            const SMDS_MeshNode * n2,
                                            const SMDS_MeshNode * n3,
                                            const SMDS_MeshNode * n4,
                                            const SMDS_MeshNode * n5,
                                            const SMDS_MeshNode * n12,
                                            const SMDS_MeshNode * n23,
                                            const SMDS_MeshNode * n34,
                                            const SMDS_MeshNode * n41,
                                            const SMDS_MeshNode * n15,
                                            const SMDS_MeshNode * n25,
                                            const SMDS_MeshNode * n35,
                                            const SMDS_MeshNode * n45,
                                            int ID)
{
  if (!n1 || !n2 || !n3 || !n4 || !n5 || !n12 || !n23 ||
      !n34 || !n41 || !n15 || !n25 || !n35 || !n45)
    return 0;
  if(hasConstructionFaces()) {
    // creation quadratic faces - not implemented
    return 0;
  }
  // --- retrieve nodes ID
  myNodeIds.resize(13);
  myNodeIds[0] = n1->getVtkId();
  myNodeIds[1] = n4->getVtkId();
  myNodeIds[2] = n3->getVtkId();
  myNodeIds[3] = n2->getVtkId();
  myNodeIds[4] = n5->getVtkId();

  myNodeIds[5] = n41->getVtkId();
  myNodeIds[6] = n34->getVtkId();
  myNodeIds[7] = n23->getVtkId();
  myNodeIds[8] = n12->getVtkId();

  myNodeIds[9] = n15->getVtkId();
  myNodeIds[10] = n45->getVtkId();
  myNodeIds[11] = n35->getVtkId();
  myNodeIds[12] = n25->getVtkId();

  SMDS_VtkVolume *volvtk = myVolumePool->getNew();
  volvtk->init(myNodeIds, this);
  if (!this->registerElement(ID,volvtk))
  {
    this->myGrid->GetCellTypesArray()->SetValue(volvtk->getVtkId(), VTK_EMPTY_CELL);
    myVolumePool->destroy(volvtk);
    return 0;
  }
  adjustmyCellsCapacity(ID);
  myCells[ID] = volvtk;
  myInfo.myNbQuadPyramids++;

  //  if (!registerElement(ID, volvtk)) {
  //    RemoveElement(volvtk, false);
  //    volvtk = NULL;
  //  }
  return volvtk;
}


//=======================================================================
//function : AddVolume
//purpose  :
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                      const SMDS_MeshNode * n2,
                                      const SMDS_MeshNode * n3,
                                      const SMDS_MeshNode * n4,
                                      const SMDS_MeshNode * n5,
                                      const SMDS_MeshNode * n6,
                                      const SMDS_MeshNode * n12,
                                      const SMDS_MeshNode * n23,
                                      const SMDS_MeshNode * n31,
                                      const SMDS_MeshNode * n45,
                                      const SMDS_MeshNode * n56,
                                      const SMDS_MeshNode * n64,
                                      const SMDS_MeshNode * n14,
                                      const SMDS_MeshNode * n25,
                                      const SMDS_MeshNode * n36)
{
  int ID = myElementIDFactory->GetFreeID();
  SMDS_MeshVolume * v =
    SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, n5, n6, n12, n23, n31,
                               n45, n56, n64, n14, n25, n36, ID);
  if(v==NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

//=======================================================================
//function : AddVolumeWithID
//purpose  :
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(int n1, int n2, int n3,
                                            int n4, int n5, int n6,
                                            int n12,int n23,int n31,
                                            int n45,int n56,int n64,
                                            int n14,int n25,int n36, int ID)
{
  return SMDS_Mesh::AddVolumeWithID
    ((SMDS_MeshNode*) myNodeIDFactory->MeshElement(n1) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n2) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n3) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n4) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n5) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n6) ,
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n12),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n23),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n31),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n45),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n56),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n64),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n14),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n25),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n36),
     ID);
}

//=======================================================================
//function : AddVolumeWithID
//purpose  : 2d order Pentahedron with 15 nodes
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                            const SMDS_MeshNode * n2,
                                            const SMDS_MeshNode * n3,
                                            const SMDS_MeshNode * n4,
                                            const SMDS_MeshNode * n5,
                                            const SMDS_MeshNode * n6,
                                            const SMDS_MeshNode * n12,
                                            const SMDS_MeshNode * n23,
                                            const SMDS_MeshNode * n31,
                                            const SMDS_MeshNode * n45,
                                            const SMDS_MeshNode * n56,
                                            const SMDS_MeshNode * n64,
                                            const SMDS_MeshNode * n14,
                                            const SMDS_MeshNode * n25,
                                            const SMDS_MeshNode * n36,
                                            int ID)
{
  if (!n1 || !n2 || !n3 || !n4 || !n5 || !n6 || !n12 || !n23 ||
      !n31 || !n45 || !n56 || !n64 || !n14 || !n25 || !n36)
    return 0;
  if(hasConstructionFaces()) {
    // creation quadratic faces - not implemented
    return 0;
  }
  // --- retrieve nodes ID
  myNodeIds.resize(15);
  myNodeIds[0] = n1->getVtkId();
  myNodeIds[1] = n2->getVtkId();
  myNodeIds[2] = n3->getVtkId();

  myNodeIds[3] = n4->getVtkId();
  myNodeIds[4] = n5->getVtkId();
  myNodeIds[5] = n6->getVtkId();

  myNodeIds[6] = n12->getVtkId();
  myNodeIds[7] = n23->getVtkId();
  myNodeIds[8] = n31->getVtkId();

  myNodeIds[9] = n45->getVtkId();
  myNodeIds[10] = n56->getVtkId();
  myNodeIds[11] = n64->getVtkId();

  myNodeIds[12] = n14->getVtkId();
  myNodeIds[13] = n25->getVtkId();
  myNodeIds[14] = n36->getVtkId();

  SMDS_VtkVolume *volvtk = myVolumePool->getNew();
  volvtk->init(myNodeIds, this);
  if (!this->registerElement(ID,volvtk))
  {
    this->myGrid->GetCellTypesArray()->SetValue(volvtk->getVtkId(), VTK_EMPTY_CELL);
    myVolumePool->destroy(volvtk);
    return 0;
  }
  adjustmyCellsCapacity(ID);
  myCells[ID] = volvtk;
  myInfo.myNbQuadPrisms++;

  //  if (!registerElement(ID, volvtk)) {
  //    RemoveElement(volvtk, false);
  //    volvtk = NULL;
  //  }
  return volvtk;
}


//=======================================================================
//function : AddVolume
//purpose  :
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                      const SMDS_MeshNode * n2,
                                      const SMDS_MeshNode * n3,
                                      const SMDS_MeshNode * n4,
                                      const SMDS_MeshNode * n5,
                                      const SMDS_MeshNode * n6,
                                      const SMDS_MeshNode * n7,
                                      const SMDS_MeshNode * n8,
                                      const SMDS_MeshNode * n12,
                                      const SMDS_MeshNode * n23,
                                      const SMDS_MeshNode * n34,
                                      const SMDS_MeshNode * n41,
                                      const SMDS_MeshNode * n56,
                                      const SMDS_MeshNode * n67,
                                      const SMDS_MeshNode * n78,
                                      const SMDS_MeshNode * n85,
                                      const SMDS_MeshNode * n15,
                                      const SMDS_MeshNode * n26,
                                      const SMDS_MeshNode * n37,
                                      const SMDS_MeshNode * n48)
{
  int ID = myElementIDFactory->GetFreeID();
  SMDS_MeshVolume * v =
    SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, n5, n6, n7, n8, n12, n23, n34, n41,
                               n56, n67, n78, n85, n15, n26, n37, n48, ID);
  if(v==NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

//=======================================================================
//function : AddVolumeWithID
//purpose  :
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(int n1, int n2, int n3, int n4,
                                            int n5, int n6, int n7, int n8,
                                            int n12,int n23,int n34,int n41,
                                            int n56,int n67,int n78,int n85,
                                            int n15,int n26,int n37,int n48, int ID)
{
  return SMDS_Mesh::AddVolumeWithID
    ((SMDS_MeshNode*) myNodeIDFactory->MeshElement(n1),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n2),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n3),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n4),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n5),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n6),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n7),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n8),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n12),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n23),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n34),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n41),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n56),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n67),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n78),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n85),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n15),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n26),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n37),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n48),
     ID);
}

//=======================================================================
//function : AddVolumeWithID
//purpose  : 2d order Hexahedrons with 20 nodes
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                            const SMDS_MeshNode * n2,
                                            const SMDS_MeshNode * n3,
                                            const SMDS_MeshNode * n4,
                                            const SMDS_MeshNode * n5,
                                            const SMDS_MeshNode * n6,
                                            const SMDS_MeshNode * n7,
                                            const SMDS_MeshNode * n8,
                                            const SMDS_MeshNode * n12,
                                            const SMDS_MeshNode * n23,
                                            const SMDS_MeshNode * n34,
                                            const SMDS_MeshNode * n41,
                                            const SMDS_MeshNode * n56,
                                            const SMDS_MeshNode * n67,
                                            const SMDS_MeshNode * n78,
                                            const SMDS_MeshNode * n85,
                                            const SMDS_MeshNode * n15,
                                            const SMDS_MeshNode * n26,
                                            const SMDS_MeshNode * n37,
                                            const SMDS_MeshNode * n48,
                                            int ID)
{
  if (!n1 || !n2 || !n3 || !n4 || !n5 || !n6 || !n7 || !n8 || !n12 || !n23 ||
      !n34 || !n41 || !n56 || !n67 || !n78 || !n85 || !n15 || !n26 || !n37 || !n48)
    return 0;
  if(hasConstructionFaces()) {
    return 0;
    // creation quadratic faces - not implemented
  }
  // --- retrieve nodes ID
  myNodeIds.resize(20);
  myNodeIds[0] = n1->getVtkId();
  myNodeIds[1] = n4->getVtkId();
  myNodeIds[2] = n3->getVtkId();
  myNodeIds[3] = n2->getVtkId();

  myNodeIds[4] = n5->getVtkId();
  myNodeIds[5] = n8->getVtkId();
  myNodeIds[6] = n7->getVtkId();
  myNodeIds[7] = n6->getVtkId();

  myNodeIds[8] = n41->getVtkId();
  myNodeIds[9] = n34->getVtkId();
  myNodeIds[10] = n23->getVtkId();
  myNodeIds[11] = n12->getVtkId();

  myNodeIds[12] = n85->getVtkId();
  myNodeIds[13] = n78->getVtkId();
  myNodeIds[14] = n67->getVtkId();
  myNodeIds[15] = n56->getVtkId();

  myNodeIds[16] = n15->getVtkId();
  myNodeIds[17] = n48->getVtkId();
  myNodeIds[18] = n37->getVtkId();
  myNodeIds[19] = n26->getVtkId();

  SMDS_VtkVolume *volvtk = myVolumePool->getNew();
  volvtk->init(myNodeIds, this);
  if (!this->registerElement(ID,volvtk))
  {
    this->myGrid->GetCellTypesArray()->SetValue(volvtk->getVtkId(), VTK_EMPTY_CELL);
    myVolumePool->destroy(volvtk);
    return 0;
  }
  adjustmyCellsCapacity(ID);
  myCells[ID] = volvtk;
  myInfo.myNbQuadHexas++;

  //  if (!registerElement(ID, volvtk)) {
  //    RemoveElement(volvtk, false);
  //    volvtk = NULL;
  //  }
  return volvtk;
}

//=======================================================================
//function : AddVolume
//purpose  :
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                      const SMDS_MeshNode * n2,
                                      const SMDS_MeshNode * n3,
                                      const SMDS_MeshNode * n4,
                                      const SMDS_MeshNode * n5,
                                      const SMDS_MeshNode * n6,
                                      const SMDS_MeshNode * n7,
                                      const SMDS_MeshNode * n8,
                                      const SMDS_MeshNode * n12,
                                      const SMDS_MeshNode * n23,
                                      const SMDS_MeshNode * n34,
                                      const SMDS_MeshNode * n41,
                                      const SMDS_MeshNode * n56,
                                      const SMDS_MeshNode * n67,
                                      const SMDS_MeshNode * n78,
                                      const SMDS_MeshNode * n85,
                                      const SMDS_MeshNode * n15,
                                      const SMDS_MeshNode * n26,
                                      const SMDS_MeshNode * n37,
                                      const SMDS_MeshNode * n48,
                                      const SMDS_MeshNode * n1234,
                                      const SMDS_MeshNode * n1256,
                                      const SMDS_MeshNode * n2367,
                                      const SMDS_MeshNode * n3478,
                                      const SMDS_MeshNode * n1458,
                                      const SMDS_MeshNode * n5678,
                                      const SMDS_MeshNode * nCenter)
{
  int ID = myElementIDFactory->GetFreeID();
  SMDS_MeshVolume * v =
    SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, n5, n6, n7, n8, n12, n23, n34, n41,
                               n56, n67, n78, n85, n15, n26, n37, n48,
                               n1234, n1256, n2367, n3478, n1458, n5678, nCenter,
                               ID);
  if(v==NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

//=======================================================================
//function : AddVolumeWithID
//purpose  :
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(int n1, int n2, int n3, int n4,
                                            int n5, int n6, int n7, int n8,
                                            int n12,int n23,int n34,int n41,
                                            int n56,int n67,int n78,int n85,
                                            int n15,int n26,int n37,int n48,
                                            int n1234,int n1256,int n2367,int n3478,
                                            int n1458,int n5678,int nCenter, int ID)
{
  return SMDS_Mesh::AddVolumeWithID
    ((SMDS_MeshNode*) myNodeIDFactory->MeshElement(n1),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n2),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n3),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n4),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n5),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n6),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n7),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n8),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n12),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n23),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n34),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n41),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n56),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n67),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n78),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n85),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n15),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n26),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n37),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n48),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n1234),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n1256),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n2367),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n3478),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n1458),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(n5678),
     (SMDS_MeshNode*) myNodeIDFactory->MeshElement(nCenter),
     ID);
}

//=======================================================================
//function : AddVolumeWithID
//purpose  : 2d order Hexahedrons with 20 nodes
//=======================================================================
SMDS_MeshVolume* SMDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                            const SMDS_MeshNode * n2,
                                            const SMDS_MeshNode * n3,
                                            const SMDS_MeshNode * n4,
                                            const SMDS_MeshNode * n5,
                                            const SMDS_MeshNode * n6,
                                            const SMDS_MeshNode * n7,
                                            const SMDS_MeshNode * n8,
                                            const SMDS_MeshNode * n12,
                                            const SMDS_MeshNode * n23,
                                            const SMDS_MeshNode * n34,
                                            const SMDS_MeshNode * n41,
                                            const SMDS_MeshNode * n56,
                                            const SMDS_MeshNode * n67,
                                            const SMDS_MeshNode * n78,
                                            const SMDS_MeshNode * n85,
                                            const SMDS_MeshNode * n15,
                                            const SMDS_MeshNode * n26,
                                            const SMDS_MeshNode * n37,
                                            const SMDS_MeshNode * n48,
                                            const SMDS_MeshNode * n1234,
                                            const SMDS_MeshNode * n1256,
                                            const SMDS_MeshNode * n2367,
                                            const SMDS_MeshNode * n3478,
                                            const SMDS_MeshNode * n1458,
                                            const SMDS_MeshNode * n5678,
                                            const SMDS_MeshNode * nCenter,
                                            int ID)
{
  if (!n1 || !n2 || !n3 || !n4 || !n5 || !n6 || !n7 || !n8 || !n12 || !n23 ||
      !n34 || !n41 || !n56 || !n67 || !n78 || !n85 || !n15 || !n26 || !n37 || !n48 ||
      !n1234 || !n1256 || !n2367 || !n3478 || !n1458 || !n5678 || !nCenter )
    return 0;
  if(hasConstructionFaces()) {
    return 0;
    // creation quadratic faces - not implemented
  }
  // --- retrieve nodes ID
  myNodeIds.resize(27);
  myNodeIds[0] = n1->getVtkId();
  myNodeIds[1] = n4->getVtkId();
  myNodeIds[2] = n3->getVtkId();
  myNodeIds[3] = n2->getVtkId();

  myNodeIds[4] = n5->getVtkId();
  myNodeIds[5] = n8->getVtkId();
  myNodeIds[6] = n7->getVtkId();
  myNodeIds[7] = n6->getVtkId();

  myNodeIds[8] = n41->getVtkId();
  myNodeIds[9] = n34->getVtkId();
  myNodeIds[10] = n23->getVtkId();
  myNodeIds[11] = n12->getVtkId();

  myNodeIds[12] = n85->getVtkId();
  myNodeIds[13] = n78->getVtkId();
  myNodeIds[14] = n67->getVtkId();
  myNodeIds[15] = n56->getVtkId();

  myNodeIds[16] = n15->getVtkId();
  myNodeIds[17] = n48->getVtkId();
  myNodeIds[18] = n37->getVtkId();
  myNodeIds[19] = n26->getVtkId();

  myNodeIds[20] = n1256->getVtkId();
  myNodeIds[21] = n3478->getVtkId();
  myNodeIds[22] = n1458->getVtkId();
  myNodeIds[23] = n2367->getVtkId();
  myNodeIds[24] = n1234->getVtkId();
  myNodeIds[25] = n5678->getVtkId();
  myNodeIds[26] = nCenter->getVtkId();

  SMDS_VtkVolume *volvtk = myVolumePool->getNew();
  volvtk->init(myNodeIds, this);
  if (!this->registerElement(ID,volvtk))
  {
    this->myGrid->GetCellTypesArray()->SetValue(volvtk->getVtkId(), VTK_EMPTY_CELL);
    myVolumePool->destroy(volvtk);
    return 0;
  }
  adjustmyCellsCapacity(ID);
  myCells[ID] = volvtk;
  myInfo.myNbTriQuadHexas++;

  return volvtk;
}


void SMDS_Mesh::updateNodeMinMax()
{
  myNodeMin = 0;
  if (myNodes.size() == 0)
  {
    myNodeMax=0;
    return;
  }
  while (!myNodes[myNodeMin] && (myNodeMin<myNodes.size()))
    myNodeMin++;
  myNodeMax=myNodes.size()-1;
  while (!myNodes[myNodeMax] && (myNodeMin>=0))
    myNodeMin--;
}

void SMDS_Mesh::incrementNodesCapacity(int nbNodes)
{
//  int val = myCellIdSmdsToVtk.size();
//  MESSAGE(" ------------------- resize myCellIdSmdsToVtk " << val << " --> " << val + nbNodes);
//  myCellIdSmdsToVtk.resize(val + nbNodes, -1); // fill new elements with -1
  int val = myNodes.size();
  MESSAGE(" ------------------- resize myNodes " << val << " --> " << val + nbNodes);
  myNodes.resize(val +nbNodes, 0);
}

void SMDS_Mesh::incrementCellsCapacity(int nbCells)
{
  int val = myCellIdVtkToSmds.size();
  MESSAGE(" ------------------- resize myCellIdVtkToSmds " << val << " --> " << val + nbCells);
  myCellIdVtkToSmds.resize(val + nbCells, -1); // fill new elements with -1
  val = myCells.size();
  MESSAGE(" ------------------- resize myCells " << val << " --> " << val + nbCells);
  myNodes.resize(val +nbCells, 0);
}

void SMDS_Mesh::adjustStructure()
{
  myGrid->GetPoints()->GetData()->SetNumberOfTuples(myNodeIDFactory->GetMaxID());
}

void SMDS_Mesh::dumpGrid(string ficdump)
{
        MESSAGE("SMDS_Mesh::dumpGrid " << ficdump);
//  vtkUnstructuredGridWriter* aWriter = vtkUnstructuredGridWriter::New();
//  aWriter->SetFileName(ficdump.c_str());
//  aWriter->SetInput(myGrid);
//  if(myGrid->GetNumberOfCells())
//  {
//    aWriter->Write();
//  }
//  aWriter->Delete();
  ficdump = ficdump + "_connectivity";
  ofstream ficcon(ficdump.c_str(), ios::out);
  int nbPoints = myGrid->GetNumberOfPoints();
  ficcon << "-------------------------------- points " <<  nbPoints << endl;
  for (int i=0; i<nbPoints; i++)
  {
        ficcon << i << " " << *(myGrid->GetPoint(i)) << " " << *(myGrid->GetPoint(i)+1) << " " << " " << *(myGrid->GetPoint(i)+2) << endl;
  }
  int nbCells = myGrid->GetNumberOfCells();
  ficcon << "-------------------------------- cells " <<  nbCells << endl;
  for (int i=0; i<nbCells; i++)
  {
//      MESSAGE(i << " " << myGrid->GetCell(i));
//      MESSAGE("  " << myGrid->GetCell(i)->GetCellType());
        ficcon << i << " - " << myGrid->GetCell(i)->GetCellType() << " -";
        int nbptcell = myGrid->GetCell(i)->GetNumberOfPoints();
        vtkIdList *listid = myGrid->GetCell(i)->GetPointIds();
        for (int j=0; j<nbptcell; j++)
        {
                ficcon << " " <<  listid->GetId(j);
        }
        ficcon << endl;
  }
  ficcon << "-------------------------------- connectivity " <<  nbPoints << endl;
#ifdef VTK_CELL_ARRAY_V2
  vtkCellLinks *links = static_cast<vtkCellLinks*>(myGrid->GetCellLinks());
#else
  vtkCellLinks *links = myGrid->GetCellLinks();
#endif
  for (int i=0; i<nbPoints; i++)
  {
        int ncells = links->GetNcells(i);
        vtkIdType *cells = links->GetCells(i);
        ficcon << i << " - " << ncells << " -";
        for (int j=0; j<ncells; j++)
        {
                ficcon << " " << cells[j];
        }
        ficcon << endl;
  }
  ficcon.close();

}

void SMDS_Mesh::compactMesh()
{
  MESSAGE("SMDS_Mesh::compactMesh do nothing!");
}

int SMDS_Mesh::fromVtkToSmds(int vtkid)
{
  if (vtkid >= 0 && vtkid < myCellIdVtkToSmds.size())
    return myCellIdVtkToSmds[vtkid];
  throw SALOME_Exception(LOCALIZED ("vtk id out of bounds"));
}

void SMDS_Mesh::updateBoundingBox()
{
  xmin = 0; xmax = 0;
  ymin = 0; ymax = 0;
  zmin = 0; zmax = 0;
  vtkPoints *points = myGrid->GetPoints();
  int myNodesSize = this->myNodes.size();
  for (int i = 0; i < myNodesSize; i++)
    {
      if (SMDS_MeshNode *n = myNodes[i])
        {
          double coords[3];
          points->GetPoint(n->myVtkID, coords);
          if (coords[0] < xmin) xmin = coords[0];
          else if (coords[0] > xmax) xmax = coords[0];
          if (coords[1] < ymin) ymin = coords[1];
          else if (coords[1] > ymax) ymax = coords[1];
          if (coords[2] < zmin) zmin = coords[2];
          else if (coords[2] > zmax) zmax = coords[2];
        }
    }
}

double SMDS_Mesh::getMaxDim()
{
  double dmax = 1.e-3;
  if ((xmax - xmin) > dmax) dmax = xmax -xmin;
  if ((ymax - ymin) > dmax) dmax = ymax -ymin;
  if ((zmax - zmin) > dmax) dmax = zmax -zmin;
  MESSAGE("getMaxDim " << dmax);
  return dmax;
}

//! modification that needs compact structure and redraw
void SMDS_Mesh::Modified()
{
  if (this->myModified)
    {
      this->myModifTime++;
      MESSAGE("modified");
      myModified = false;
    }
}

//! get last modification timeStamp
VTK_MTIME_TYPE SMDS_Mesh::GetMTime() const
{
  return this->myModifTime;
}

bool SMDS_Mesh::isCompacted()
{
  if (this->myModifTime > this->myCompactTime)
    {
      MESSAGE(" *** isCompacted " << myCompactTime << " < " << myModifTime);
      this->myCompactTime = this->myModifTime;
      return false;
    }
  return true;
}

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

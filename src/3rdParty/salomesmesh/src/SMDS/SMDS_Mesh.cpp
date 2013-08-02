//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  SMESH SMDS : implementaion of Salome mesh data structure
//
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "utilities.h"
#include "SMDS_Mesh.hxx"
#include "SMDS_VolumeOfNodes.hxx"
#include "SMDS_VolumeOfFaces.hxx"
#include "SMDS_FaceOfNodes.hxx"
#include "SMDS_FaceOfEdges.hxx"
#include "SMDS_PolyhedralVolumeOfNodes.hxx"
#include "SMDS_PolygonalFaceOfNodes.hxx"
#include "SMDS_QuadraticEdge.hxx"
#include "SMDS_QuadraticFaceOfNodes.hxx"
#include "SMDS_QuadraticVolumeOfNodes.hxx"

#include <algorithm>
#include <map>
#include <iterator>
using namespace std;

#ifndef WIN32
#if !(defined(__MACH__) && defined(__APPLE__))
#include <sys/sysinfo.h>
#endif
#endif

// number of added entitis to check memory after
#define CHECKMEMORY_INTERVAL 1000

//================================================================================
/*!
 * \brief Raise an exception if free memory (ram+swap) too low
 * \param doNotRaise - if true, suppres exception, just return free memory size
 * \retval int - amount of available memory in MB or negative number in failure case
 */
//================================================================================

int SMDS_Mesh::CheckMemory(const bool doNotRaise) throw (std::bad_alloc)
{
#if (defined(__MACH__) && defined(__APPLE__))
	return 1000;
#else	
#ifndef WIN32
  struct sysinfo si;
  int err = sysinfo( &si );
  if ( err )
    return -1;

  static int limit = -1;
  if ( limit < 0 ) {
    int status = system("SMDS_MemoryLimit"); // it returns lower limit of free RAM
    if (status >= 0 ) {
      limit = WEXITSTATUS(status);
    }
    if ( limit < 20 )
      limit = 20;
    else
      limit = int( limit * 1.5 );
#ifdef _DEBUG_
    MESSAGE ( "SMDS_Mesh::CheckMemory() memory limit = " << limit << " MB" );
#endif
  }

  const unsigned long Mbyte = 1024 * 1024;
  // compute separately to avoid overflow
  int freeMb =
    ( si.freeram  * si.mem_unit ) / Mbyte +
    ( si.freeswap * si.mem_unit ) / Mbyte;

  if ( freeMb > limit )
    return freeMb - limit;

  if ( doNotRaise )
    return 0;
#ifdef _DEBUG_
  MESSAGE ("SMDS_Mesh::CheckMemory() throws as free memory too low: " << freeMb <<" MB" );
#endif
  throw std::bad_alloc();
#else
  return -1;
#endif
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// Create a new mesh object
///////////////////////////////////////////////////////////////////////////////
SMDS_Mesh::SMDS_Mesh()
	:myParent(NULL),
        myNodeIDFactory(new SMDS_MeshElementIDFactory()),
	myElementIDFactory(new SMDS_MeshElementIDFactory()),
	myHasConstructionEdges(false), myHasConstructionFaces(false),
	myHasInverseElements(true)
{
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
	myHasInverseElements(true)
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
    if ( myNodes.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
    SMDS_MeshNode * node=new SMDS_MeshNode(x, y, z);
    myNodes.Add(node);
    myNodeIDFactory->BindID(ID,node);
    myInfo.myNbNodes++;
    return node;
  }else
    return NULL;
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

  if ( myEdges.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();

  SMDS_MeshEdge * edge=new SMDS_MeshEdge(n1,n2);
  if(myElementIDFactory->BindID(ID, edge)) {
    SMDS_MeshNode *node1,*node2;
    node1=const_cast<SMDS_MeshNode*>(n1);
    node2=const_cast<SMDS_MeshNode*>(n2);
    node1->AddInverseElement(edge);
    node2->AddInverseElement(edge);		
    myEdges.Add(edge);
    myInfo.myNbEdges++;
    return edge;
  } 
  else {
    delete edge;
    return NULL;
  }
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
  SMDS_MeshFace * face=createTriangle(n1, n2, n3);

  if (face && !registerElement(ID, face)) {
    RemoveElement(face, false);
    face = NULL;
  }
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
  SMDS_MeshFace * face=createQuadrangle(n1, n2, n3, n4);

  if (face && !registerElement(ID, face)) {
    RemoveElement(face, false);
    face = NULL;
  }
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

  if ( myFaces.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();

  SMDS_MeshFace * face = new SMDS_FaceOfEdges(e1,e2,e3);
  myFaces.Add(face);
  myInfo.myNbTriangles++;

  if (!registerElement(ID, face)) {
    RemoveElement(face, false);
    face = NULL;
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
  if ( !e1 || !e2 || !e3 || !e4 ) return 0;
  if ( myFaces.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  SMDS_MeshFace * face = new SMDS_FaceOfEdges(e1,e2,e3,e4);
  myFaces.Add(face);
  myInfo.myNbQuadrangles++;

  if (!registerElement(ID, face))
  {
    RemoveElement(face, false);
    face = NULL;
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
  SMDS_MeshVolume* volume = 0;
  if ( !n1 || !n2 || !n3 || !n4) return volume;
  if ( myVolumes.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionFaces()) {
    SMDS_MeshFace * f1=FindFaceOrCreate(n1,n2,n3);
    SMDS_MeshFace * f2=FindFaceOrCreate(n1,n2,n4);
    SMDS_MeshFace * f3=FindFaceOrCreate(n1,n3,n4);
    SMDS_MeshFace * f4=FindFaceOrCreate(n2,n3,n4);
    volume=new SMDS_VolumeOfFaces(f1,f2,f3,f4);
    myVolumes.Add(volume);
    myInfo.myNbTetras++;
  }
  else if(hasConstructionEdges()) {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else {
    volume=new SMDS_VolumeOfNodes(n1,n2,n3,n4);
    myVolumes.Add(volume);
    myInfo.myNbTetras++;
  }

  if (!registerElement(ID, volume)) {
    RemoveElement(volume, false);
    volume = NULL;
  }
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
  SMDS_MeshVolume* volume = 0;
  if ( !n1 || !n2 || !n3 || !n4 || !n5) return volume;
  if ( myVolumes.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionFaces()) {
    SMDS_MeshFace * f1=FindFaceOrCreate(n1,n2,n3,n4);
    SMDS_MeshFace * f2=FindFaceOrCreate(n1,n2,n5);
    SMDS_MeshFace * f3=FindFaceOrCreate(n2,n3,n5);
    SMDS_MeshFace * f4=FindFaceOrCreate(n3,n4,n5);
    volume=new SMDS_VolumeOfFaces(f1,f2,f3,f4);
    myVolumes.Add(volume);
    myInfo.myNbPyramids++;
  }
  else if(hasConstructionEdges()) {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else {
    volume=new SMDS_VolumeOfNodes(n1,n2,n3,n4,n5);
    myVolumes.Add(volume);
    myInfo.myNbPyramids++;
  }

  if (!registerElement(ID, volume)) {
    RemoveElement(volume, false);
    volume = NULL;
  }
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
  SMDS_MeshVolume* volume = 0;
  if ( !n1 || !n2 || !n3 || !n4 || !n5 || !n6) return volume;
  if ( myVolumes.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionFaces()) {
    SMDS_MeshFace * f1=FindFaceOrCreate(n1,n2,n3);
    SMDS_MeshFace * f2=FindFaceOrCreate(n4,n5,n6);
    SMDS_MeshFace * f3=FindFaceOrCreate(n1,n4,n5,n2);
    SMDS_MeshFace * f4=FindFaceOrCreate(n2,n5,n6,n3);
    SMDS_MeshFace * f5=FindFaceOrCreate(n3,n6,n4,n1);
    volume=new SMDS_VolumeOfFaces(f1,f2,f3,f4,f5);
    myVolumes.Add(volume);
    myInfo.myNbPrisms++;
  }
  else if(hasConstructionEdges()) {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else {
    volume=new SMDS_VolumeOfNodes(n1,n2,n3,n4,n5,n6);
    myVolumes.Add(volume);
    myInfo.myNbPrisms++;
  }

  if (!registerElement(ID, volume)) {
    RemoveElement(volume, false);
    volume = NULL;
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
  SMDS_MeshVolume* volume = 0;
  if ( !n1 || !n2 || !n3 || !n4 || !n5 || !n6 || !n7 || !n8) return volume;
  if ( myVolumes.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionFaces()) {
    SMDS_MeshFace * f1=FindFaceOrCreate(n1,n2,n3,n4);
    SMDS_MeshFace * f2=FindFaceOrCreate(n5,n6,n7,n8);
    SMDS_MeshFace * f3=FindFaceOrCreate(n1,n4,n8,n5);
    SMDS_MeshFace * f4=FindFaceOrCreate(n1,n2,n6,n5);
    SMDS_MeshFace * f5=FindFaceOrCreate(n2,n3,n7,n6);
    SMDS_MeshFace * f6=FindFaceOrCreate(n3,n4,n8,n7);
    volume=new SMDS_VolumeOfFaces(f1,f2,f3,f4,f5,f6);
    myVolumes.Add(volume);
    myInfo.myNbHexas++;
  }
  else if(hasConstructionEdges()) {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else {
//    volume=new SMDS_HexahedronOfNodes(n1,n2,n3,n4,n5,n6,n7,n8);
    volume=new SMDS_VolumeOfNodes(n1,n2,n3,n4,n5,n6,n7,n8);
    myVolumes.Add(volume);
    myInfo.myNbHexas++;
  }

  if (!registerElement(ID, volume)) {
    RemoveElement(volume, false);
    volume = NULL;
  }
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
  if (!hasConstructionFaces())
    return NULL;
  if ( !f1 || !f2 || !f3 || !f4) return 0;
  if ( myVolumes.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  SMDS_MeshVolume * volume = new SMDS_VolumeOfFaces(f1,f2,f3,f4);
  myVolumes.Add(volume);
  myInfo.myNbTetras++;

  if (!registerElement(ID, volume)) {
    RemoveElement(volume, false);
    volume = NULL;
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
  if (!hasConstructionFaces())
    return NULL;
  if ( !f1 || !f2 || !f3 || !f4 || !f5) return 0;
  if ( myVolumes.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  SMDS_MeshVolume * volume = new SMDS_VolumeOfFaces(f1,f2,f3,f4,f5);
  myVolumes.Add(volume);
  myInfo.myNbPyramids++;

  if (!registerElement(ID, volume)) {
    RemoveElement(volume, false);
    volume = NULL;
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
  if (!hasConstructionFaces())
    return NULL;
  if ( !f1 || !f2 || !f3 || !f4 || !f5 || !f6) return 0;
  if ( myVolumes.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  SMDS_MeshVolume * volume = new SMDS_VolumeOfFaces(f1,f2,f3,f4,f5,f6);
  myVolumes.Add(volume);
  myInfo.myNbPrisms++;

  if (!registerElement(ID, volume)) {
    RemoveElement(volume, false);
    volume = NULL;
  }
  return volume;
}

///////////////////////////////////////////////////////////////////////////////
/// Add a polygon defined by its nodes IDs
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddPolygonalFaceWithID (std::vector<int> nodes_ids,
                                                  const int        ID)
{
  int nbNodes = nodes_ids.size();
  std::vector<const SMDS_MeshNode*> nodes (nbNodes);
  for (int i = 0; i < nbNodes; i++) {
    nodes[i] = (SMDS_MeshNode *)myNodeIDFactory->MeshElement(nodes_ids[i]);
    if (!nodes[i]) return NULL;
  }
  return SMDS_Mesh::AddPolygonalFaceWithID(nodes, ID);	
}

///////////////////////////////////////////////////////////////////////////////
/// Add a polygon defined by its nodes
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddPolygonalFaceWithID
                          (std::vector<const SMDS_MeshNode*> nodes,
                           const int                         ID)
{
  SMDS_MeshFace * face;

  if ( myFaces.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if (hasConstructionEdges())
  {
    MESSAGE("Error : Not implemented");
    return NULL;
  }
  else
  {
    for ( int i = 0; i < nodes.size(); ++i )
      if ( !nodes[ i ] ) return 0;
    face = new SMDS_PolygonalFaceOfNodes(nodes);
    myFaces.Add(face);
    myInfo.myNbPolygons++;
  }

  if (!registerElement(ID, face)) {
    RemoveElement(face, false);
    face = NULL;
  }
  return face;
}

///////////////////////////////////////////////////////////////////////////////
/// Add a polygon defined by its nodes.
/// An ID is automatically affected to the created face.
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshFace* SMDS_Mesh::AddPolygonalFace (std::vector<const SMDS_MeshNode*> nodes)
{
  return SMDS_Mesh::AddPolygonalFaceWithID(nodes, myElementIDFactory->GetFreeID());
}

///////////////////////////////////////////////////////////////////////////////
/// Create a new polyhedral volume and add it to the mesh. 
/// @param ID The ID of the new volume
/// @return The created volume or NULL if an element with this ID already exists
/// or if input nodes are not found.
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume * SMDS_Mesh::AddPolyhedralVolumeWithID
                             (std::vector<int> nodes_ids,
                              std::vector<int> quantities,
                              const int        ID)
{
  int nbNodes = nodes_ids.size();
  std::vector<const SMDS_MeshNode*> nodes (nbNodes);
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

SMDS_MeshVolume* SMDS_Mesh::AddPolyhedralVolumeWithID
                            (std::vector<const SMDS_MeshNode*> nodes,
                             std::vector<int>                  quantities,
                             const int                         ID)
{
  SMDS_MeshVolume* volume;
  if ( myVolumes.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if (hasConstructionFaces()) {
    MESSAGE("Error : Not implemented");
    return NULL;
  } else if (hasConstructionEdges()) {
    MESSAGE("Error : Not implemented");
    return NULL;
  } else {
    for ( int i = 0; i < nodes.size(); ++i )
      if ( !nodes[ i ] ) return 0;
    volume = new SMDS_PolyhedralVolumeOfNodes(nodes, quantities);
    myVolumes.Add(volume);
    myInfo.myNbPolyhedrons++;
  }

  if (!registerElement(ID, volume)) {
    RemoveElement(volume, false);
    volume = NULL;
  }
  return volume;
}

///////////////////////////////////////////////////////////////////////////////
/// Create a new polyhedral volume and add it to the mesh. 
/// @return The created  volume
///////////////////////////////////////////////////////////////////////////////

SMDS_MeshVolume* SMDS_Mesh::AddPolyhedralVolume
                            (std::vector<const SMDS_MeshNode*> nodes,
                             std::vector<int>                  quantities)
{
  int ID = myElementIDFactory->GetFreeID();
  SMDS_MeshVolume * v = SMDS_Mesh::AddPolyhedralVolumeWithID(nodes, quantities, ID);
  if (v == NULL) myElementIDFactory->ReleaseID(ID);
  return v;
}

///////////////////////////////////////////////////////////////////////////////
/// Registers element with the given ID, maintains inverse connections
///////////////////////////////////////////////////////////////////////////////
bool SMDS_Mesh::registerElement(int ID, SMDS_MeshElement * element)
{
  if (myElementIDFactory->BindID(ID, element)) {
    SMDS_ElemIteratorPtr it = element->nodesIterator();
    while (it->more()) {
      SMDS_MeshNode *node = static_cast<SMDS_MeshNode*>
        (const_cast<SMDS_MeshElement*>(it->next()));
      node->AddInverseElement(element);
    }
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
/// Return the node whose ID is 'ID'.
///////////////////////////////////////////////////////////////////////////////
const SMDS_MeshNode * SMDS_Mesh::FindNode(int ID) const
{
  return (const SMDS_MeshNode *)myNodeIDFactory->MeshElement(ID);
}

///////////////////////////////////////////////////////////////////////////////
///Create a triangle and add it to the current mesh. This methode do not bind a
///ID to the create triangle.
///////////////////////////////////////////////////////////////////////////////
SMDS_MeshFace * SMDS_Mesh::createTriangle(const SMDS_MeshNode * node1,
                                          const SMDS_MeshNode * node2,
                                          const SMDS_MeshNode * node3)
{
  if ( !node1 || !node2 || !node3) return 0;
  if ( myFaces.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionEdges())
  {
    SMDS_MeshEdge *edge1, *edge2, *edge3;
    edge1=FindEdgeOrCreate(node1,node2);
    edge2=FindEdgeOrCreate(node2,node3);
    edge3=FindEdgeOrCreate(node3,node1);

    SMDS_MeshFace * face = new SMDS_FaceOfEdges(edge1,edge2,edge3);
    myFaces.Add(face);
    myInfo.myNbTriangles++;
    return face;
  }
  else
  {
    SMDS_MeshFace * face = new SMDS_FaceOfNodes(node1,node2,node3);
    myFaces.Add(face);
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
					    const SMDS_MeshNode * node4)
{
  if ( !node1 || !node2 || !node3 || !node4 ) return 0;
  if ( myFaces.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
  if(hasConstructionEdges())
  {
    SMDS_MeshEdge *edge1, *edge2, *edge3, *edge4;
    edge1=FindEdgeOrCreate(node1,node2);
    edge2=FindEdgeOrCreate(node2,node3);
    edge3=FindEdgeOrCreate(node3,node4);
    edge4=FindEdgeOrCreate(node4,node1);

    SMDS_MeshFace * face = new SMDS_FaceOfEdges(edge1,edge2,edge3,edge4);
    myFaces.Add(face);
    myInfo.myNbQuadrangles++;
    return face;
  }
  else
  {
    SMDS_MeshFace * face = new SMDS_FaceOfNodes(node1,node2,node3,node4);
    myFaces.Add(face);
    myInfo.myNbQuadrangles++;
    return face;
  }
}

///////////////////////////////////////////////////////////////////////////////
/// Remove a node and all the elements which own this node
///////////////////////////////////////////////////////////////////////////////

void SMDS_Mesh::RemoveNode(const SMDS_MeshNode * node)
{
	RemoveElement(node, true);
}

///////////////////////////////////////////////////////////////////////////////
/// Remove an edge and all the elements which own this edge
///////////////////////////////////////////////////////////////////////////////

void SMDS_Mesh::RemoveEdge(const SMDS_MeshEdge * edge)
{
	RemoveElement(edge,true);
}

///////////////////////////////////////////////////////////////////////////////
/// Remove an face and all the elements which own this face
///////////////////////////////////////////////////////////////////////////////

void SMDS_Mesh::RemoveFace(const SMDS_MeshFace * face)
{
	RemoveElement(face, true);
}

///////////////////////////////////////////////////////////////////////////////
/// Remove a volume
///////////////////////////////////////////////////////////////////////////////

void SMDS_Mesh::RemoveVolume(const SMDS_MeshVolume * volume)
{
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
  // keep current nodes of elem
  set<const SMDS_MeshElement*> oldNodes;
  SMDS_ElemIteratorPtr itn = element->nodesIterator();
  while(itn->more())
    oldNodes.insert(  itn->next() );

  if ( !element->IsPoly() )
    myInfo.remove( element ); // element may change type

  // change nodes
  bool Ok = false;
  SMDS_MeshElement* elem = const_cast<SMDS_MeshElement*>(element);
  switch ( elem->GetType() )
  {
  case SMDSAbs_Edge: {
    if ( nbnodes == 2 ) {
      if ( SMDS_MeshEdge* edge = dynamic_cast<SMDS_MeshEdge*>( elem ))
        Ok = edge->ChangeNodes( nodes[0], nodes[1] );
    }
    else if ( nbnodes == 3 ) {
      if ( SMDS_QuadraticEdge* edge = dynamic_cast<SMDS_QuadraticEdge*>( elem ))
        Ok = edge->ChangeNodes( nodes[0], nodes[1], nodes[2] );
    }
    break;
  }
  case SMDSAbs_Face: {
    if ( SMDS_FaceOfNodes* face = dynamic_cast<SMDS_FaceOfNodes*>( elem ))
      Ok = face->ChangeNodes( nodes, nbnodes );
    else
      if ( SMDS_QuadraticFaceOfNodes* QF = dynamic_cast<SMDS_QuadraticFaceOfNodes*>( elem ))
        Ok = QF->ChangeNodes( nodes, nbnodes );
      else
        if (SMDS_PolygonalFaceOfNodes* face = dynamic_cast<SMDS_PolygonalFaceOfNodes*>(elem))
          Ok = face->ChangeNodes(nodes, nbnodes);
    break;
  }
  case SMDSAbs_Volume: {
    if ( SMDS_VolumeOfNodes* vol = dynamic_cast<SMDS_VolumeOfNodes*>( elem ))
      Ok = vol->ChangeNodes( nodes, nbnodes );
    else 
      if ( SMDS_QuadraticVolumeOfNodes* QV = dynamic_cast<SMDS_QuadraticVolumeOfNodes*>( elem ))
        Ok = QV->ChangeNodes( nodes, nbnodes );
    break;
  }
  default:
    MESSAGE ( "WRONG ELEM TYPE");
  }

  if ( Ok ) { // update InverseElements

    set<const SMDS_MeshElement*>::iterator it;

    // AddInverseElement to new nodes
    for ( int i = 0; i < nbnodes; i++ ) {
      it = oldNodes.find( nodes[i] );
      if ( it == oldNodes.end() )
        // new node
        const_cast<SMDS_MeshNode*>( nodes[i] )->AddInverseElement( elem );
      else
        // remove from oldNodes a node that remains in elem
        oldNodes.erase( it );
    }
    // RemoveInverseElement from the nodes removed from elem
    for ( it = oldNodes.begin(); it != oldNodes.end(); it++ )
    {
      SMDS_MeshNode * n = static_cast<SMDS_MeshNode *>
        (const_cast<SMDS_MeshElement *>( *it ));
      n->RemoveInverseElement( elem );
    }
  }

  if ( !element->IsPoly() )
    myInfo.add( element ); // element may change type

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

  const SMDS_PolyhedralVolumeOfNodes* vol = dynamic_cast<const SMDS_PolyhedralVolumeOfNodes*>(elem);
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
  bool Ok = const_cast<SMDS_PolyhedralVolumeOfNodes*>(vol)->ChangeNodes(nodes, quantities);
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
    if ( myEdges.Extent() % CHECKMEMORY_INTERVAL == 0 ) CheckMemory();
    toReturn=new SMDS_MeshEdge(node1,node2);
    myEdges.Add(toReturn);
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
    toReturn = createTriangle(node1,node2,node3);
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
    toReturn=createQuadrangle(node1,node2,node3,node4);
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
  return myElementIDFactory->MeshElement(IDelem);
}

//=======================================================================
//function : FindFace
//purpose  : find polygon
//=======================================================================

const SMDS_MeshFace* SMDS_Mesh::FindFace (std::vector<int> nodes_ids) const
{
  int nbnodes = nodes_ids.size();
  std::vector<const SMDS_MeshNode *> poly_nodes (nbnodes);
  for (int inode = 0; inode < nbnodes; inode++) {
    const SMDS_MeshNode * node = FindNode(nodes_ids[inode]);
    if (node == NULL) return NULL;
  }
  return FindFace(poly_nodes);
}

const SMDS_MeshFace* SMDS_Mesh::FindFace (std::vector<const SMDS_MeshNode *> nodes)
{
  if ( nodes.size() > 2 && nodes[0] ) {
    SMDS_ElemIteratorPtr itF = nodes[0]->GetInverseElementIterator(SMDSAbs_Face);
    while (itF->more()) {
      const SMDS_MeshElement* f = itF->next();
      if ( f->NbNodes() == nodes.size() ) {
        SMDS_ElemIteratorPtr it2 = f->nodesIterator();
        while(it2->more()) {
          if ( find( nodes.begin(), nodes.end(), it2->next() ) == nodes.end() ) {
            f = 0;
            break;
          }
        }
        if ( f )
          return static_cast<const SMDS_MeshFace *> (f);
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
	while(itnode->more()) MESSAGE(itnode->next());
}

//=======================================================================
//function : DumpEdges
//purpose  : 
//=======================================================================

void SMDS_Mesh::DumpEdges() const
{
	MESSAGE("dump edges of mesh : ");
	SMDS_EdgeIteratorPtr itedge=edgesIterator();
	while(itedge->more()) MESSAGE(itedge->next());
}

//=======================================================================
//function : DumpFaces
//purpose  : 
//=======================================================================

void SMDS_Mesh::DumpFaces() const
{
	MESSAGE("dump faces of mesh : ");
	SMDS_FaceIteratorPtr itface=facesIterator();
	while(itface->more()) MESSAGE(itface->next());
}

//=======================================================================
//function : DumpVolumes
//purpose  : 
//=======================================================================

void SMDS_Mesh::DumpVolumes() const
{
	MESSAGE("dump volumes of mesh : ");
	SMDS_VolumeIteratorPtr itvol=volumesIterator();
	while(itvol->more()) MESSAGE(itvol->next());
}

//=======================================================================
//function : DebugStats
//purpose  : 
//=======================================================================

void SMDS_Mesh::DebugStats() const
{
	MESSAGE("Debug stats of mesh : ");

	MESSAGE("===== NODES ====="<<NbNodes());
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
	return myNodes.Size();
}

///////////////////////////////////////////////////////////////////////////////
/// Return the number of edges (including construction edges)
///////////////////////////////////////////////////////////////////////////////
int SMDS_Mesh::NbEdges() const
{
	return myEdges.Size();
}

///////////////////////////////////////////////////////////////////////////////
/// Return the number of faces (including construction faces)
///////////////////////////////////////////////////////////////////////////////
int SMDS_Mesh::NbFaces() const
{
	return myFaces.Size();
}

///////////////////////////////////////////////////////////////////////////////
/// Return the number of volumes
///////////////////////////////////////////////////////////////////////////////
int SMDS_Mesh::NbVolumes() const
{
	return myVolumes.Size();
}

///////////////////////////////////////////////////////////////////////////////
/// Return the number of child mesh of this mesh.
/// Note that the tree structure of SMDS_Mesh seems to be unused in this version
/// (2003-09-08) of SMESH
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
      myElementIDFactory->ReleaseID(eIt->next()->GetID());
    SMDS_NodeIteratorPtr itn = nodesIterator();
    while (itn->more())
      myNodeIDFactory->ReleaseID(itn->next()->GetID());
  }
  SetOfNodes::Iterator itn(myNodes);
  for (; itn.More(); itn.Next())
    delete itn.Value();

  SetOfEdges::Iterator ite(myEdges);
  for (; ite.More(); ite.Next())
  {
    SMDS_MeshElement* elem = ite.Value();
    delete elem;
  }

  SetOfFaces::Iterator itf(myFaces);
  for (; itf.More(); itf.Next())
  {
    SMDS_MeshElement* elem = itf.Value();
    delete elem;
  }

  SetOfVolumes::Iterator itv(myVolumes);
  for (; itv.More(); itv.Next())
  {
    SMDS_MeshElement* elem = itv.Value();
    delete elem;
  }

}

//================================================================================
/*!
 * \brief Clear all data
 */
//================================================================================

void SMDS_Mesh::Clear()
{
  if (myParent!=NULL) {
    SMDS_ElemIteratorPtr eIt = elementsIterator();
    while ( eIt->more() )
      myElementIDFactory->ReleaseID(eIt->next()->GetID());
    SMDS_NodeIteratorPtr itn = nodesIterator();
    while (itn->more())
      myNodeIDFactory->ReleaseID(itn->next()->GetID());
  }
  else {
    myNodeIDFactory->Clear();
    myElementIDFactory->Clear();
  }
  SMDS_VolumeIteratorPtr itv = volumesIterator();
  while (itv->more())
    delete itv->next();
  myVolumes.Clear();

  SMDS_FaceIteratorPtr itf = facesIterator();
  while (itf->more())
    delete itf->next();
  myFaces.Clear();
      
  SMDS_EdgeIteratorPtr ite = edgesIterator();
  while (ite->more())
    delete ite->next();
  myEdges.Clear();

  SMDS_NodeIteratorPtr itn = nodesIterator();
  while (itn->more())
    delete itn->next();
  myNodes.Clear();

  list<SMDS_Mesh*>::iterator itc=myChildren.begin();
  while(itc!=myChildren.end())
    (*itc)->Clear();

  myInfo.Clear();
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

///////////////////////////////////////////////////////////////////////////////
/// Return an iterator on nodes of the current mesh factory
///////////////////////////////////////////////////////////////////////////////
class SMDS_Mesh_MyNodeIterator:public SMDS_NodeIterator
{
  SMDS_ElemIteratorPtr myIterator;
 public:
  SMDS_Mesh_MyNodeIterator(const SMDS_ElemIteratorPtr& it):myIterator(it)
  {}

  bool more()
  {
    return myIterator->more();
  }

  const SMDS_MeshNode* next()
  {
    return static_cast<const SMDS_MeshNode*>(myIterator->next());
  }
};

SMDS_NodeIteratorPtr SMDS_Mesh::nodesIterator() const
{
  return SMDS_NodeIteratorPtr
    (new SMDS_Mesh_MyNodeIterator(myNodeIDFactory->elementsIterator()));
}

///////////////////////////////////////////////////////////////////////////////
/// Return an iterator on elements of the current mesh factory
///////////////////////////////////////////////////////////////////////////////
SMDS_ElemIteratorPtr SMDS_Mesh::elementsIterator() const
{
  return myElementIDFactory->elementsIterator();
}

///////////////////////////////////////////////////////////////////////////////
///Return an iterator on edges of the current mesh.
///////////////////////////////////////////////////////////////////////////////
class SMDS_Mesh_MyEdgeIterator:public SMDS_EdgeIterator
{
  typedef SMDS_Mesh::SetOfEdges SetOfEdges;
  SetOfEdges::Iterator myIterator;
 public:
  SMDS_Mesh_MyEdgeIterator(const SetOfEdges& s):myIterator(s)
  {}

  bool more()
  {
    while(myIterator.More())
    {
      if(myIterator.Value()->GetID()!=-1)
        return true;
      myIterator.Next();
    }
    return false;
  }

  const SMDS_MeshEdge* next()
  {
    const SMDS_MeshEdge* current = myIterator.Value();
    myIterator.Next();
    return current;
  }
};

SMDS_EdgeIteratorPtr SMDS_Mesh::edgesIterator() const
{
  return SMDS_EdgeIteratorPtr(new SMDS_Mesh_MyEdgeIterator(myEdges));
}

///////////////////////////////////////////////////////////////////////////////
///Return an iterator on faces of the current mesh.
///////////////////////////////////////////////////////////////////////////////
class SMDS_Mesh_MyFaceIterator:public SMDS_FaceIterator
{
  typedef SMDS_Mesh::SetOfFaces SetOfFaces;
  SetOfFaces::Iterator myIterator;
 public:
  SMDS_Mesh_MyFaceIterator(const SetOfFaces& s):myIterator(s)
  {}

  bool more()
  {
    while(myIterator.More())
    {
      if(myIterator.Value()->GetID()!=-1)
        return true;
      myIterator.Next();
    }
    return false;
  }

  const SMDS_MeshFace* next()
  {
    const SMDS_MeshFace* current = myIterator.Value();
    myIterator.Next();
    return current;
  }
};

SMDS_FaceIteratorPtr SMDS_Mesh::facesIterator() const
{
  return SMDS_FaceIteratorPtr(new SMDS_Mesh_MyFaceIterator(myFaces));
}

///////////////////////////////////////////////////////////////////////////////
///Return an iterator on volumes of the current mesh.
///////////////////////////////////////////////////////////////////////////////
class SMDS_Mesh_MyVolumeIterator:public SMDS_VolumeIterator
{
  typedef SMDS_Mesh::SetOfVolumes SetOfVolumes;
  SetOfVolumes::Iterator myIterator;
 public:
  SMDS_Mesh_MyVolumeIterator(const SetOfVolumes& s):myIterator(s)
  {}

  bool more()
  {
    return myIterator.More() != Standard_False;
  }

  const SMDS_MeshVolume* next()
  {
    const SMDS_MeshVolume* current = myIterator.Value();
    myIterator.Next();
    return current;
  }
};

SMDS_VolumeIteratorPtr SMDS_Mesh::volumesIterator() const
{
  return SMDS_VolumeIteratorPtr(new SMDS_Mesh_MyVolumeIterator(myVolumes));
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
			std::inserter(*rsetB, rsetB->begin()));
		delete rsetA;
		rsetA=rsetB;
	}
	return rsetA;
}

///////////////////////////////////////////////////////////////////////////////
/// Return the list of finit elements owning the given element
///////////////////////////////////////////////////////////////////////////////
static set<const SMDS_MeshElement*> * getFinitElements(const SMDS_MeshElement * element)
{
	int numberOfSets=element->NbNodes();
 	set<const SMDS_MeshElement*> *initSet = new set<const SMDS_MeshElement*>[numberOfSets];

	SMDS_ElemIteratorPtr itNodes=element->nodesIterator();

	int i=0;
	while(itNodes->more())
	{
		const SMDS_MeshNode * n=static_cast<const SMDS_MeshNode*>(itNodes->next());
		SMDS_ElemIteratorPtr itFe = n->GetInverseElementIterator();

		//initSet[i]=set<const SMDS_MeshElement*>();
		while(itFe->more())
                  initSet[i].insert(itFe->next());

		i++;
	}
	set<const SMDS_MeshElement*> *retSet=intersectionOfSets(initSet, numberOfSets);
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
void SMDS_Mesh::addChildrenWithNodes(set<const SMDS_MeshElement*>&	setOfChildren, 
	const SMDS_MeshElement * element, set<const SMDS_MeshElement*>& nodes)
{
	
	switch(element->GetType())
	{
	case SMDSAbs_Node:
		MESSAGE("Internal Error: This should not append");
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
///@param removedElems contains all removed elements
///@param removedNodes contains all removed nodes
///@param removenodes if true remaining nodes will be removed
///////////////////////////////////////////////////////////////////////////////
void SMDS_Mesh::RemoveElement(const SMDS_MeshElement *        elem,
                              list<const SMDS_MeshElement *>& removedElems,
                              list<const SMDS_MeshElement *>& removedNodes,
                              bool                            removenodes)
{
  // get finite elements built on elem
  set<const SMDS_MeshElement*> * s1;
  if (!hasConstructionEdges() && elem->GetType() == SMDSAbs_Edge ||
      !hasConstructionFaces() && elem->GetType() == SMDSAbs_Face ||
      elem->GetType() == SMDSAbs_Volume)
  {
    s1 = new set<const SMDS_MeshElement*>();
    s1->insert(elem);
  }
  else
    s1 = getFinitElements(elem);

  // get exclusive nodes (which would become free afterwards)
  set<const SMDS_MeshElement*> * s2;
  if (elem->GetType() == SMDSAbs_Node) // a node is removed
  {
    // do not remove nodes except elem
    s2 = new set<const SMDS_MeshElement*>();
    s2->insert(elem);
	removenodes = true;
  }
  else
    s2 = getExclusiveNodes(*s1);

  // form the set of finite and construction elements to remove
  set<const SMDS_MeshElement*> s3;
  set<const SMDS_MeshElement*>::iterator it=s1->begin();
  while(it!=s1->end())
  {
    addChildrenWithNodes(s3, *it ,*s2);
    s3.insert(*it);
    it++;
  }
  if(elem->GetType()!=SMDSAbs_Node) s3.insert(elem);

  // remove finite and construction elements
  it=s3.begin();
  while(it!=s3.end())
  {
    // Remove element from <InverseElements> of its nodes
    SMDS_ElemIteratorPtr itn=(*it)->nodesIterator();
    while(itn->more())
    {
      SMDS_MeshNode * n = static_cast<SMDS_MeshNode *>
        (const_cast<SMDS_MeshElement *>(itn->next()));
      n->RemoveInverseElement( (*it) );
    }

    switch((*it)->GetType())
    {
    case SMDSAbs_Node:
      MESSAGE("Internal Error: This should not happen");
      break;
    case SMDSAbs_Edge:
      myEdges.Remove(static_cast<SMDS_MeshEdge*>
                    (const_cast<SMDS_MeshElement*>(*it)));
      myInfo.RemoveEdge(*it);
      break;
    case SMDSAbs_Face:
      myFaces.Remove(static_cast<SMDS_MeshFace*>
                    (const_cast<SMDS_MeshElement*>(*it)));
      myInfo.RemoveFace(*it);
      break;
    case SMDSAbs_Volume:
      myVolumes.Remove(static_cast<SMDS_MeshVolume*>
                      (const_cast<SMDS_MeshElement*>(*it)));
      myInfo.RemoveVolume(*it);
      break;
    }
    //MESSAGE( "SMDS: RM elem " << (*it)->GetID() );
    removedElems.push_back( (*it) );
    myElementIDFactory->ReleaseID((*it)->GetID());
    delete (*it);
    it++;
  }

  // remove exclusive (free) nodes
  if(removenodes)
  {
    it=s2->begin();
    while(it!=s2->end())
    {
      //MESSAGE( "SMDS: RM node " << (*it)->GetID() );
      myNodes.Remove(static_cast<SMDS_MeshNode*>
                    (const_cast<SMDS_MeshElement*>(*it)));
      myInfo.myNbNodes--;
      myNodeIDFactory->ReleaseID((*it)->GetID());
      removedNodes.push_back( (*it) );
      delete *it;
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
  SMDSAbs_ElementType aType = elem->GetType();
  if (aType == SMDSAbs_Node) {
    // only free node can be removed by this method
    const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>(elem);
    SMDS_ElemIteratorPtr itFe = n->GetInverseElementIterator();
    if (!itFe->more()) { // free node
      myNodes.Remove(const_cast<SMDS_MeshNode*>(n));
      myInfo.myNbNodes--;
      myNodeIDFactory->ReleaseID(elem->GetID());
      delete elem;
    }
  } else {
    if (hasConstructionEdges() || hasConstructionFaces())
      // this methods is only for meshes without descendants
      return;

    // Remove element from <InverseElements> of its nodes
    SMDS_ElemIteratorPtr itn = elem->nodesIterator();
    while (itn->more()) {
      SMDS_MeshNode * n = static_cast<SMDS_MeshNode *>
        (const_cast<SMDS_MeshElement *>(itn->next()));
      n->RemoveInverseElement(elem);
    }

    // in meshes without descendants elements are always free
    switch (aType) {
    case SMDSAbs_Edge:
      myEdges.Remove(static_cast<SMDS_MeshEdge*>
                     (const_cast<SMDS_MeshElement*>(elem)));
      myInfo.RemoveEdge(elem);
      break;
    case SMDSAbs_Face:
      myFaces.Remove(static_cast<SMDS_MeshFace*>
                     (const_cast<SMDS_MeshElement*>(elem)));
      myInfo.RemoveFace(elem);
      break;
    case SMDSAbs_Volume:
      myVolumes.Remove(static_cast<SMDS_MeshVolume*>
                       (const_cast<SMDS_MeshElement*>(elem)));
      myInfo.RemoveVolume(elem);
      break;
    default:
      break;
    }
    myElementIDFactory->ReleaseID(elem->GetID());
    delete elem;
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
  SMDS_EdgeIteratorPtr ite = edgesIterator();
  while (ite->more())
    if (elem == ite->next())
      return true;
  SMDS_FaceIteratorPtr itf = facesIterator();
  while (itf->more())
    if (elem == itf->next())
      return true;
  SMDS_VolumeIteratorPtr itv = volumesIterator();
  while (itv->more())
    if (elem == itv->next())
      return true;
  return false;
}

//=======================================================================
//function : MaxNodeID
//purpose  : 
//=======================================================================

int SMDS_Mesh::MaxNodeID() const
{
  return myNodeIDFactory->GetMaxID();
}

//=======================================================================
//function : MinNodeID
//purpose  : 
//=======================================================================

int SMDS_Mesh::MinNodeID() const
{
  return myNodeIDFactory->GetMinID();
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
  if ( deltaID == 0 )
    return;

  SMDS_MeshElementIDFactory * idFactory =
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
    //throw SMESH_Exception(LOCALIZED ("this element isn't exist"));
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
  SMDS_QuadraticEdge* edge = new SMDS_QuadraticEdge(n1,n2,n12);
  if(myElementIDFactory->BindID(ID, edge)) {
    SMDS_MeshNode *node1,*node2, *node12;
    node1 = const_cast<SMDS_MeshNode*>(n1);
    node2 = const_cast<SMDS_MeshNode*>(n2);
    node12 = const_cast<SMDS_MeshNode*>(n12);
    node1->AddInverseElement(edge);
    node2->AddInverseElement(edge);
    node12->AddInverseElement(edge);
    myEdges.Add(edge);
    myInfo.myNbQuadEdges++;
    return edge;
  } 
  else {
    delete edge;
    return NULL;
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
  SMDS_QuadraticFaceOfNodes* face =
    new SMDS_QuadraticFaceOfNodes(n1,n2,n3,n12,n23,n31);
  myFaces.Add(face);
  myInfo.myNbQuadTriangles++;

  if (!registerElement(ID, face)) {
    RemoveElement(face, false);
    face = NULL;
  }
  return face;
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
  }
  SMDS_QuadraticFaceOfNodes* face =
    new SMDS_QuadraticFaceOfNodes(n1,n2,n3,n4,n12,n23,n34,n41);
  myFaces.Add(face);
  myInfo.myNbQuadQuadrangles++;

  if (!registerElement(ID, face)) {
    RemoveElement(face, false);
    face = NULL;
  }
  return face;
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
  SMDS_QuadraticVolumeOfNodes * volume =
    new SMDS_QuadraticVolumeOfNodes(n1,n2,n3,n4,n12,n23,n31,n14,n24,n34);
  myVolumes.Add(volume);
  myInfo.myNbQuadTetras++;

  if (!registerElement(ID, volume)) {
    RemoveElement(volume, false);
    volume = NULL;
  }
  return volume;
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
  SMDS_QuadraticVolumeOfNodes * volume =
    new SMDS_QuadraticVolumeOfNodes(n1,n2,n3,n4,n5,n12,n23,
                                    n34,n41,n15,n25,n35,n45);
  myVolumes.Add(volume);
  myInfo.myNbQuadPyramids++;

  if (!registerElement(ID, volume)) {
    RemoveElement(volume, false);
    volume = NULL;
  }
  return volume;
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
  SMDS_QuadraticVolumeOfNodes * volume =
    new SMDS_QuadraticVolumeOfNodes(n1,n2,n3,n4,n5,n6,n12,n23,n31,
                                    n45,n56,n64,n14,n25,n36);
  myVolumes.Add(volume);
  myInfo.myNbQuadPrisms++;

  if (!registerElement(ID, volume)) {
    RemoveElement(volume, false);
    volume = NULL;
  }
  return volume;
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
  SMDS_QuadraticVolumeOfNodes * volume =
    new SMDS_QuadraticVolumeOfNodes(n1,n2,n3,n4,n5,n6,n7,n8,n12,n23,n34,n41,
                                    n56,n67,n78,n85,n15,n26,n37,n48);
  myVolumes.Add(volume);
  myInfo.myNbQuadHexas++;

  if (!registerElement(ID, volume)) {
    RemoveElement(volume, false);
    volume = NULL;
  }
  return volume;
}

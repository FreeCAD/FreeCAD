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
//=============================================================================
// File      : NETGENPlugin_NETGEN_3D.cxx
//             Moved here from SMESH_NETGEN_3D.cxx
// Created   : lundi 27 Janvier 2003
// Author    : Nadir BOUHAMOU (CEA)
// Project   : SALOME
//=============================================================================
//
#include "NETGENPlugin_NETGEN_3D.hxx"

#include "NETGENPlugin_Mesher.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_ControlsDef.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_MeshEditor.hxx"
#include "StdMeshers_QuadToTriaAdaptor.hxx"

#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#include <Standard_Failure.hxx>
#include <Standard_ErrorHandler.hxx>

#include "utilities.h"

#include <list>
#include <vector>
#include <map>

/*
  Netgen include files
*/

namespace nglib {
#include <nglib.h>
}
using namespace nglib;
using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

NETGENPlugin_NETGEN_3D::NETGENPlugin_NETGEN_3D(int hypId, int studyId,
			     SMESH_Gen* gen)
  : SMESH_3D_Algo(hypId, studyId, gen)
{
  MESSAGE("NETGENPlugin_NETGEN_3D::NETGENPlugin_NETGEN_3D");
  _name = "NETGEN_3D";
  _shapeType = (1 << TopAbs_SHELL) | (1 << TopAbs_SOLID);// 1 bit /shape type
  _compatibleHypothesis.push_back("MaxElementVolume");

  _maxElementVolume = 0.;

  _hypMaxElementVolume = NULL;

  _requireShape = false; // can work without shape
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

NETGENPlugin_NETGEN_3D::~NETGENPlugin_NETGEN_3D()
{
  MESSAGE("NETGENPlugin_NETGEN_3D::~NETGENPlugin_NETGEN_3D");
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool NETGENPlugin_NETGEN_3D::CheckHypothesis
                         (SMESH_Mesh& aMesh,
                          const TopoDS_Shape& aShape,
                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  MESSAGE("NETGENPlugin_NETGEN_3D::CheckHypothesis");

  _hypMaxElementVolume = NULL;
  _maxElementVolume = DBL_MAX;

  list<const SMESHDS_Hypothesis*>::const_iterator itl;
  const SMESHDS_Hypothesis* theHyp;

  const list<const SMESHDS_Hypothesis*>& hyps = GetUsedHypothesis(aMesh, aShape);
  int nbHyp = hyps.size();
  if (!nbHyp)
  {
    aStatus = SMESH_Hypothesis::HYP_OK;
    //aStatus = SMESH_Hypothesis::HYP_MISSING;
    return true;  // can work with no hypothesis
  }

  itl = hyps.begin();
  theHyp = (*itl); // use only the first hypothesis

  string hypName = theHyp->GetName();

  bool isOk = false;

  if (hypName == "MaxElementVolume")
  {
    _hypMaxElementVolume = static_cast<const StdMeshers_MaxElementVolume*> (theHyp);
    ASSERT(_hypMaxElementVolume);
    _maxElementVolume = _hypMaxElementVolume->GetMaxVolume();
    isOk =true;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }
  else
    aStatus = SMESH_Hypothesis::HYP_INCOMPATIBLE;

  return isOk;
}

//=============================================================================
/*!
 *Here we are going to use the NETGEN mesher
 */
//=============================================================================

bool NETGENPlugin_NETGEN_3D::Compute(SMESH_Mesh&         aMesh,
                                     const TopoDS_Shape& aShape)
{
  MESSAGE("NETGENPlugin_NETGEN_3D::Compute with maxElmentsize = " << _maxElementVolume);

  SMESHDS_Mesh* meshDS = aMesh.GetMeshDS();

  const int invalid_ID = -1;

  SMESH::Controls::Area areaControl;
  SMESH::Controls::TSequenceOfXYZ nodesCoords;

  // -------------------------------------------------------------------
  // get triangles on aShell and make a map of nodes to Netgen node IDs
  // -------------------------------------------------------------------

  SMESH_MesherHelper helper(aMesh);
  SMESH_MesherHelper* myTool = &helper;
  bool _quadraticMesh = myTool->IsQuadraticSubMesh(aShape);

  typedef map< const SMDS_MeshNode*, int, TIDCompare > TNodeToIDMap;
  TNodeToIDMap nodeToNetgenID;
  list< const SMDS_MeshElement* > triangles;
  list< bool >                    isReversed; // orientation of triangles

  TopAbs_ShapeEnum mainType = aMesh.GetShapeToMesh().ShapeType();
  bool checkReverse = ( mainType == TopAbs_COMPOUND || mainType == TopAbs_COMPSOLID );

  // for the degeneraged edge: ignore all but one node on it;
  // map storing ids of degen edges and vertices and their netgen id:
  map< int, int* > degenShapeIdToPtrNgId;
  map< int, int* >::iterator shId_ngId;
  list< int > degenNgIds;

  StdMeshers_QuadToTriaAdaptor Adaptor;
  Adaptor.Compute(aMesh,aShape);

  for (TopExp_Explorer exp(aShape,TopAbs_FACE);exp.More();exp.Next())
  {
    const TopoDS_Shape& aShapeFace = exp.Current();
    const SMESHDS_SubMesh * aSubMeshDSFace = meshDS->MeshElements( aShapeFace );
    if ( aSubMeshDSFace )
    {
      bool isRev = false;
      if ( checkReverse && helper.NbAncestors(aShapeFace, aMesh, aShape.ShapeType()) > 1 )
        // IsReversedSubMesh() can work wrong on strongly curved faces,
        // so we use it as less as possible
        isRev = SMESH_Algo::IsReversedSubMesh( TopoDS::Face(aShapeFace), meshDS );

      SMDS_ElemIteratorPtr iteratorElem = aSubMeshDSFace->GetElements();
      while ( iteratorElem->more() ) // loop on elements on a face
      {
        // check element
        const SMDS_MeshElement* elem = iteratorElem->next();
        if ( !elem )
          return error( COMPERR_BAD_INPUT_MESH, "Null element encounters");
        bool isTraingle = ( elem->NbNodes()==3 || (_quadraticMesh && elem->NbNodes()==6 ));
        if ( !isTraingle ) {
          //return error( COMPERR_BAD_INPUT_MESH,
          //              SMESH_Comment("Not triangle element ")<<elem->GetID());
          // using adaptor
          const list<const SMDS_FaceOfNodes*>* faces = Adaptor.GetTriangles(elem);
          if(faces==0) {
            return error( COMPERR_BAD_INPUT_MESH,
                          SMESH_Comment("Not triangles in adaptor for element ")<<elem->GetID());
          }
          list<const SMDS_FaceOfNodes*>::const_iterator itf = faces->begin();
          for(; itf!=faces->end(); itf++ ) {
            triangles.push_back( (*itf) );
            isReversed.push_back( isRev );
            // put triange's nodes to nodeToNetgenID map
            SMDS_ElemIteratorPtr triangleNodesIt = (*itf)->nodesIterator();
            while ( triangleNodesIt->more() ) {
              const SMDS_MeshNode * node =
                static_cast<const SMDS_MeshNode *>(triangleNodesIt->next());
              if(myTool->IsMedium(node))
                continue;
              nodeToNetgenID.insert( make_pair( node, invalid_ID ));
            }
          }
        }
        else {
          // keep a triangle
          triangles.push_back( elem );
          isReversed.push_back( isRev );
          // put elem nodes to nodeToNetgenID map
          SMDS_ElemIteratorPtr triangleNodesIt = elem->nodesIterator();
          while ( triangleNodesIt->more() ) {
            const SMDS_MeshNode * node =
              static_cast<const SMDS_MeshNode *>(triangleNodesIt->next());
            if(myTool->IsMedium(node))
              continue;
            nodeToNetgenID.insert( make_pair( node, invalid_ID ));
          }
        }
#ifdef _DEBUG_
        // check if a trainge is degenerated
        areaControl.GetPoints( elem, nodesCoords );
        double area = areaControl.GetValue( nodesCoords );
        if ( area <= DBL_MIN ) {
          MESSAGE( "Warning: Degenerated " << elem );
        }
#endif
      }
      // look for degeneraged edges and vetices
      for (TopExp_Explorer expE(aShapeFace,TopAbs_EDGE);expE.More();expE.Next())
      {
        TopoDS_Edge aShapeEdge = TopoDS::Edge( expE.Current() );
        if ( BRep_Tool::Degenerated( aShapeEdge ))
        {
          degenNgIds.push_back( invalid_ID );
          int* ptrIdOnEdge = & degenNgIds.back();
          // remember edge id
          int edgeID = meshDS->ShapeToIndex( aShapeEdge );
          degenShapeIdToPtrNgId.insert( make_pair( edgeID, ptrIdOnEdge ));
          // remember vertex id
          int vertexID = meshDS->ShapeToIndex( TopExp::FirstVertex( aShapeEdge ));
          degenShapeIdToPtrNgId.insert( make_pair( vertexID, ptrIdOnEdge ));
        }
      }
    }
  }
  // ---------------------------------
  // Feed the Netgen with surface mesh
  // ---------------------------------

  int Netgen_NbOfNodes = 0;
  int Netgen_param2ndOrder = 0;
  double Netgen_paramFine = 1.;
  double Netgen_paramSize = pow( 72, 1/6. ) * pow( _maxElementVolume, 1/3. );

  double Netgen_point[3];
  int Netgen_triangle[3];
  int Netgen_tetrahedron[4];

  Ng_Init();

  Ng_Mesh * Netgen_mesh = Ng_NewMesh();

  // set nodes and remember thier netgen IDs
  bool isDegen = false, hasDegen = !degenShapeIdToPtrNgId.empty();
  TNodeToIDMap::iterator n_id = nodeToNetgenID.begin();
  for ( ; n_id != nodeToNetgenID.end(); ++n_id )
  {
    const SMDS_MeshNode* node = n_id->first;

    // ignore nodes on degenerated edge
    if ( hasDegen ) {
      int shapeId = node->GetPosition()->GetShapeId();
      shId_ngId = degenShapeIdToPtrNgId.find( shapeId );
      isDegen = ( shId_ngId != degenShapeIdToPtrNgId.end() );
      if ( isDegen && *(shId_ngId->second) != invalid_ID ) {
        n_id->second = *(shId_ngId->second);
        continue;
      }
    }
    Netgen_point [ 0 ] = node->X();
    Netgen_point [ 1 ] = node->Y();
    Netgen_point [ 2 ] = node->Z();
    Ng_AddPoint(Netgen_mesh, Netgen_point);
    n_id->second = ++Netgen_NbOfNodes; // set netgen ID

    if ( isDegen ) // all nodes on a degen edge get one netgen ID
      *(shId_ngId->second) = n_id->second;
  }

  // set triangles
  list< const SMDS_MeshElement* >::iterator tria = triangles.begin();
  list< bool >::iterator                 reverse = isReversed.begin();
  for ( ; tria != triangles.end(); ++tria, ++reverse )
  {
    int i = 0;
    SMDS_ElemIteratorPtr triangleNodesIt = (*tria)->nodesIterator();
    while ( triangleNodesIt->more() ) {
      const SMDS_MeshNode * node =
        static_cast<const SMDS_MeshNode *>(triangleNodesIt->next());
      if(myTool->IsMedium(node))
        continue;
      Netgen_triangle[ *reverse ? 2 - i : i ] = nodeToNetgenID[ node ];
      ++i;
    }
    if ( !hasDegen ||
         // ignore degenerated triangles, they have 2 or 3 same ids
         (Netgen_triangle[0] != Netgen_triangle[1] &&
          Netgen_triangle[0] != Netgen_triangle[2] &&
          Netgen_triangle[2] != Netgen_triangle[1] ))
    {
      Ng_AddSurfaceElement(Netgen_mesh, NG_TRIG, Netgen_triangle);
    }
  }

  // -------------------------
  // Generate the volume mesh
  // -------------------------

  Ng_Meshing_Parameters Netgen_param;

#ifdef NETGEN_V5
  Netgen_param.second_order = Netgen_param2ndOrder;
#endif
  Netgen_param.fineness = Netgen_paramFine;
  Netgen_param.maxh = Netgen_paramSize;

  Ng_Result status;

  try {
#if (OCC_VERSION_MAJOR << 16 | OCC_VERSION_MINOR << 8 | OCC_VERSION_MAINTENANCE) > 0x060100
    OCC_CATCH_SIGNALS;
#endif
    status = Ng_GenerateVolumeMesh(Netgen_mesh, &Netgen_param);
  }
  catch (Standard_Failure& exc) {
    error(COMPERR_OCC_EXCEPTION, exc.GetMessageString());
    status = NG_VOLUME_FAILURE;
  }
  catch (...) {
    error("Exception in Ng_GenerateVolumeMesh()");
    status = NG_VOLUME_FAILURE;
  }
  if ( GetComputeError()->IsOK() ) {
    switch ( status ) {
    case NG_SURFACE_INPUT_ERROR:error( status, "NG_SURFACE_INPUT_ERROR");
    case NG_VOLUME_FAILURE:     error( status, "NG_VOLUME_FAILURE");
    case NG_STL_INPUT_ERROR:    error( status, "NG_STL_INPUT_ERROR");
    case NG_SURFACE_FAILURE:    error( status, "NG_SURFACE_FAILURE");
    case NG_FILE_NOT_FOUND:     error( status, "NG_FILE_NOT_FOUND");
    };
  }

  int Netgen_NbOfNodesNew = Ng_GetNP(Netgen_mesh);

  int Netgen_NbOfTetra = Ng_GetNE(Netgen_mesh);

  MESSAGE("End of Volume Mesh Generation. status=" << status <<
          ", nb new nodes: " << Netgen_NbOfNodesNew - Netgen_NbOfNodes <<
          ", nb tetra: " << Netgen_NbOfTetra);

  // -------------------------------------------------------------------
  // Feed back the SMESHDS with the generated Nodes and Volume Elements
  // -------------------------------------------------------------------

  bool isOK = ( /*status == NG_OK &&*/ Netgen_NbOfTetra > 0 );// get whatever built
  if ( isOK )
  {
    // vector of nodes in which node index == netgen ID
    vector< const SMDS_MeshNode* > nodeVec ( Netgen_NbOfNodesNew + 1 );
    // insert old nodes into nodeVec
    for ( n_id = nodeToNetgenID.begin(); n_id != nodeToNetgenID.end(); ++n_id ) {
      nodeVec.at( n_id->second ) = n_id->first;
    }
    // create and insert new nodes into nodeVec
    int nodeIndex = Netgen_NbOfNodes + 1;
    int shapeID = meshDS->ShapeToIndex( aShape );
    for ( ; nodeIndex <= Netgen_NbOfNodesNew; ++nodeIndex )
    {
      Ng_GetPoint( Netgen_mesh, nodeIndex, Netgen_point );
      SMDS_MeshNode * node = meshDS->AddNode(Netgen_point[0],
                                             Netgen_point[1],
                                             Netgen_point[2]);
      meshDS->SetNodeInVolume(node, shapeID);
      nodeVec.at(nodeIndex) = node;
    }

    // create tetrahedrons
    for ( int elemIndex = 1; elemIndex <= Netgen_NbOfTetra; ++elemIndex )
    {
      Ng_GetVolumeElement(Netgen_mesh, elemIndex, Netgen_tetrahedron);
      SMDS_MeshVolume * elt = myTool->AddVolume (nodeVec.at( Netgen_tetrahedron[0] ),
                                                 nodeVec.at( Netgen_tetrahedron[1] ),
                                                 nodeVec.at( Netgen_tetrahedron[2] ),
                                                 nodeVec.at( Netgen_tetrahedron[3] ));
      meshDS->SetMeshElementOnShape(elt, shapeID );
    }
  }

  Ng_DeleteMesh(Netgen_mesh);
  Ng_Exit();

  NETGENPlugin_Mesher::RemoveTmpFiles();

  return (status == NG_OK);
}

bool NETGENPlugin_NETGEN_3D::Compute(SMESH_Mesh& aMesh,
                                     SMESH_MesherHelper* aHelper)
{
  MESSAGE("NETGENPlugin_NETGEN_3D::Compute with maxElmentsize = " << _maxElementVolume);  
  const int invalid_ID = -1;
  bool _quadraticMesh = false;
  typedef map< const SMDS_MeshNode*, int, TIDCompare > TNodeToIDMap;
  TNodeToIDMap nodeToNetgenID;
  list< const SMDS_MeshElement* > triangles;
  SMESHDS_Mesh* MeshDS = aHelper->GetMeshDS();

  SMESH_MesherHelper::MType MeshType = aHelper->IsQuadraticMesh();
  
  if(MeshType == SMESH_MesherHelper::COMP)
    return error( COMPERR_BAD_INPUT_MESH,
                  SMESH_Comment("Mesh with linear and quadratic elements given."));
  else if (MeshType == SMESH_MesherHelper::QUADRATIC)
    _quadraticMesh = true;
    
  StdMeshers_QuadToTriaAdaptor Adaptor;
  Adaptor.Compute(aMesh);

  SMDS_FaceIteratorPtr fIt = MeshDS->facesIterator();
  TIDSortedElemSet sortedFaces; //  0020279: control the "random" use when using mesh algorithms
  while( fIt->more()) sortedFaces.insert( fIt->next() );

  TIDSortedElemSet::iterator itFace = sortedFaces.begin(), fEnd = sortedFaces.end();
  for ( ; itFace != fEnd; ++itFace ) {
    // check element
    const SMDS_MeshElement* elem = *itFace;
    if ( !elem )
      return error( COMPERR_BAD_INPUT_MESH, "Null element encounters");
    bool isTraingle = ( elem->NbNodes()==3 || (_quadraticMesh && elem->NbNodes()==6 ));
    if ( !isTraingle ) {
      //return error( COMPERR_BAD_INPUT_MESH,
      //              SMESH_Comment("Not triangle element ")<<elem->GetID());
      // using adaptor
      const list<const SMDS_FaceOfNodes*>* faces = Adaptor.GetTriangles(elem);
      if(faces==0) {
        return error( COMPERR_BAD_INPUT_MESH,
                      SMESH_Comment("Not triangles in adaptor for element ")<<elem->GetID());
      }
      list<const SMDS_FaceOfNodes*>::const_iterator itf = faces->begin();
      for(; itf!=faces->end(); itf++ ) {
        triangles.push_back( (*itf) );
        // put triange's nodes to nodeToNetgenID map
        SMDS_ElemIteratorPtr triangleNodesIt = (*itf)->nodesIterator();
        while ( triangleNodesIt->more() ) {
          const SMDS_MeshNode * node =
            static_cast<const SMDS_MeshNode *>(triangleNodesIt->next());
          if(aHelper->IsMedium(node))
            continue;
          nodeToNetgenID.insert( make_pair( node, invalid_ID ));
        }
      }
    }
    else {
      // keep a triangle
      triangles.push_back( elem );
      // put elem nodes to nodeToNetgenID map
      SMDS_ElemIteratorPtr triangleNodesIt = elem->nodesIterator();
      while ( triangleNodesIt->more() ) {
        const SMDS_MeshNode * node =
          static_cast<const SMDS_MeshNode *>(triangleNodesIt->next());
        if(aHelper->IsMedium(node))
          continue;
        nodeToNetgenID.insert( make_pair( node, invalid_ID ));
      }
    }
  }

  // ---------------------------------
  // Feed the Netgen with surface mesh
  // ---------------------------------

  int Netgen_NbOfNodes = 0;
  int Netgen_param2ndOrder = 0;
  double Netgen_paramFine = 1.;
  double Netgen_paramSize = pow( 72, 1/6. ) * pow( _maxElementVolume, 1/3. );
  
  double Netgen_point[3];
  int Netgen_triangle[3];
  int Netgen_tetrahedron[4];

  Ng_Init();

  Ng_Mesh * Netgen_mesh = Ng_NewMesh();

    // set nodes and remember thier netgen IDs
  
  TNodeToIDMap::iterator n_id = nodeToNetgenID.begin();
  for ( ; n_id != nodeToNetgenID.end(); ++n_id )
  {
    const SMDS_MeshNode* node = n_id->first;

    Netgen_point [ 0 ] = node->X();
    Netgen_point [ 1 ] = node->Y();
    Netgen_point [ 2 ] = node->Z();
    Ng_AddPoint(Netgen_mesh, Netgen_point);
    n_id->second = ++Netgen_NbOfNodes; // set netgen ID

  }

  // set triangles
  list< const SMDS_MeshElement* >::iterator tria = triangles.begin();
  for ( ; tria != triangles.end(); ++tria)
  {
    int i = 0;
    SMDS_ElemIteratorPtr triangleNodesIt = (*tria)->nodesIterator();
    while ( triangleNodesIt->more() ) {
      const SMDS_MeshNode * node =
        static_cast<const SMDS_MeshNode *>(triangleNodesIt->next());
      if(aHelper->IsMedium(node))
        continue;
      Netgen_triangle[ i ] = nodeToNetgenID[ node ];
      ++i;
    }
    
    Ng_AddSurfaceElement(Netgen_mesh, NG_TRIG, Netgen_triangle);
  }

  // -------------------------
  // Generate the volume mesh
  // -------------------------

  Ng_Meshing_Parameters Netgen_param;

#ifdef NETGEN_V5
  Netgen_param.second_order = Netgen_param2ndOrder;
#endif
  Netgen_param.fineness = Netgen_paramFine;
  Netgen_param.maxh = Netgen_paramSize;

  Ng_Result status;

  try {
#if (OCC_VERSION_MAJOR << 16 | OCC_VERSION_MINOR << 8 | OCC_VERSION_MAINTENANCE) > 0x060100
    OCC_CATCH_SIGNALS;
#endif
    status = Ng_GenerateVolumeMesh(Netgen_mesh, &Netgen_param);
  }
  catch (Standard_Failure& exc) {
    error(COMPERR_OCC_EXCEPTION, exc.GetMessageString());
    status = NG_VOLUME_FAILURE;
  }
  catch (...) {
    error("Bad mesh input!!!");
    status = NG_VOLUME_FAILURE;
  }
  if ( GetComputeError()->IsOK() ) {
    error( status, "Bad mesh input!!!");
  }

  int Netgen_NbOfNodesNew = Ng_GetNP(Netgen_mesh);

  int Netgen_NbOfTetra = Ng_GetNE(Netgen_mesh);

  MESSAGE("End of Volume Mesh Generation. status=" << status <<
          ", nb new nodes: " << Netgen_NbOfNodesNew - Netgen_NbOfNodes <<
          ", nb tetra: " << Netgen_NbOfTetra);

  // -------------------------------------------------------------------
  // Feed back the SMESHDS with the generated Nodes and Volume Elements
  // -------------------------------------------------------------------

  bool isOK = ( Netgen_NbOfTetra > 0 );// get whatever built
  if ( isOK )
  {
    // vector of nodes in which node index == netgen ID
    vector< const SMDS_MeshNode* > nodeVec ( Netgen_NbOfNodesNew + 1 );
    // insert old nodes into nodeVec
    for ( n_id = nodeToNetgenID.begin(); n_id != nodeToNetgenID.end(); ++n_id ) {
      nodeVec.at( n_id->second ) = n_id->first;
    }
    // create and insert new nodes into nodeVec
    int nodeIndex = Netgen_NbOfNodes + 1;
    
    for ( ; nodeIndex <= Netgen_NbOfNodesNew; ++nodeIndex )
    {
      Ng_GetPoint( Netgen_mesh, nodeIndex, Netgen_point );
      SMDS_MeshNode * node = aHelper->AddNode(Netgen_point[0],
					      Netgen_point[1],
					      Netgen_point[2]);
      nodeVec.at(nodeIndex) = node;
    }

    // create tetrahedrons
    for ( int elemIndex = 1; elemIndex <= Netgen_NbOfTetra; ++elemIndex )
    {
      Ng_GetVolumeElement(Netgen_mesh, elemIndex, Netgen_tetrahedron);
      aHelper->AddVolume (nodeVec.at( Netgen_tetrahedron[0] ),
			  nodeVec.at( Netgen_tetrahedron[1] ),
			  nodeVec.at( Netgen_tetrahedron[2] ),
			  nodeVec.at( Netgen_tetrahedron[3] ));
    }
  }

  Ng_DeleteMesh(Netgen_mesh);
  Ng_Exit();
  
  NETGENPlugin_Mesher::RemoveTmpFiles();
  
  return (status == NG_OK);
}

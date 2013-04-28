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
//  SMESH SMESHDS : management of mesh data and SMESH document
//  File   : SMESHDS_Mesh.hxx
//  Module : SMESH
//
#ifndef _SMESHDS_Mesh_HeaderFile
#define _SMESHDS_Mesh_HeaderFile

#include "SMESH_SMESHDS.hxx"

#include "SMDS_Mesh.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_MeshEdge.hxx"
#include "SMDS_MeshFace.hxx"
#include "SMDS_MeshVolume.hxx"
#include "SMESHDS_Hypothesis.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESHDS_Script.hxx"

#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>

#include <NCollection_DataMap.hxx>
#include <map>
/*
 * Using of native haah_map isn't portable and don't work on WIN32 platform.
 * So this functionality implement on new NCollection_DataMap technology
 */
#include "SMESHDS_DataMapOfShape.hxx"

class SMESHDS_GroupBase;

class SMESHDS_EXPORT SMESHDS_Mesh:public SMDS_Mesh{
public:
  SMESHDS_Mesh(int theMeshID, bool theIsEmbeddedMode);
  bool IsEmbeddedMode();

  void ShapeToMesh(const TopoDS_Shape & S);
  TopoDS_Shape ShapeToMesh() const;
  bool AddHypothesis(const TopoDS_Shape & SS, const SMESHDS_Hypothesis * H);
  bool RemoveHypothesis(const TopoDS_Shape & S, const SMESHDS_Hypothesis * H);
  
  virtual SMDS_MeshNode* AddNodeWithID(double x, double y, double z, int ID);
  virtual SMDS_MeshNode * AddNode(double x, double y, double z);
  
  virtual SMDS_MeshEdge* AddEdgeWithID(int n1, int n2, int ID);
  virtual SMDS_MeshEdge* AddEdgeWithID(const SMDS_MeshNode * n1,
				       const SMDS_MeshNode * n2, 
				       int ID);
  virtual SMDS_MeshEdge* AddEdge(const SMDS_MeshNode * n1,
				 const SMDS_MeshNode * n2);
  
  // 2d order edge with 3 nodes: n12 - node between n1 and n2
  virtual SMDS_MeshEdge* AddEdgeWithID(int n1, int n2, int n12, int ID);
  virtual SMDS_MeshEdge* AddEdgeWithID(const SMDS_MeshNode * n1,
				       const SMDS_MeshNode * n2, 
				       const SMDS_MeshNode * n12, 
				       int ID);
  virtual SMDS_MeshEdge* AddEdge(const SMDS_MeshNode * n1,
                                 const SMDS_MeshNode * n2,
                                 const SMDS_MeshNode * n12);

  virtual SMDS_MeshFace* AddFaceWithID(int n1, int n2, int n3, int ID);
  virtual SMDS_MeshFace* AddFaceWithID(const SMDS_MeshNode * n1,
				       const SMDS_MeshNode * n2,
				       const SMDS_MeshNode * n3, 
				       int ID);
  virtual SMDS_MeshFace* AddFace(const SMDS_MeshNode * n1,
				 const SMDS_MeshNode * n2,
				 const SMDS_MeshNode * n3);

  virtual SMDS_MeshFace* AddFaceWithID(int n1, int n2, int n3, int n4, int ID);
  virtual SMDS_MeshFace* AddFaceWithID(const SMDS_MeshNode * n1,
				       const SMDS_MeshNode * n2,
				       const SMDS_MeshNode * n3,
				       const SMDS_MeshNode * n4, 
				       int ID);
  virtual SMDS_MeshFace* AddFace(const SMDS_MeshNode * n1,
				 const SMDS_MeshNode * n2,
				 const SMDS_MeshNode * n3,
				 const SMDS_MeshNode * n4);

  // 2d order triangle of 6 nodes
  virtual SMDS_MeshFace* AddFaceWithID(int n1, int n2, int n3,
                                       int n12,int n23,int n31, int ID);
  virtual SMDS_MeshFace* AddFaceWithID(const SMDS_MeshNode * n1,
				       const SMDS_MeshNode * n2,
				       const SMDS_MeshNode * n3, 
				       const SMDS_MeshNode * n12,
				       const SMDS_MeshNode * n23,
				       const SMDS_MeshNode * n31, 
				       int ID);
  virtual SMDS_MeshFace* AddFace(const SMDS_MeshNode * n1,
				 const SMDS_MeshNode * n2,
				 const SMDS_MeshNode * n3,
                                 const SMDS_MeshNode * n12,
				 const SMDS_MeshNode * n23,
				 const SMDS_MeshNode * n31);

  // 2d order quadrangle
  virtual SMDS_MeshFace* AddFaceWithID(int n1, int n2, int n3, int n4,
                                       int n12,int n23,int n34,int n41, int ID);
  virtual SMDS_MeshFace* AddFaceWithID(const SMDS_MeshNode * n1,
				       const SMDS_MeshNode * n2,
				       const SMDS_MeshNode * n3,
				       const SMDS_MeshNode * n4, 
				       const SMDS_MeshNode * n12,
				       const SMDS_MeshNode * n23,
				       const SMDS_MeshNode * n34,
				       const SMDS_MeshNode * n41, 
				       int ID);
  virtual SMDS_MeshFace* AddFace(const SMDS_MeshNode * n1,
				 const SMDS_MeshNode * n2,
				 const SMDS_MeshNode * n3,
				 const SMDS_MeshNode * n4,
                                 const SMDS_MeshNode * n12,
				 const SMDS_MeshNode * n23,
				 const SMDS_MeshNode * n34,
				 const SMDS_MeshNode * n41);

  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4, int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
					   const SMDS_MeshNode * n2,
					   const SMDS_MeshNode * n3,
					   const SMDS_MeshNode * n4, 
					   int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
				     const SMDS_MeshNode * n2,
				     const SMDS_MeshNode * n3,
				     const SMDS_MeshNode * n4);

  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4, int n5, int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
					   const SMDS_MeshNode * n2,
					   const SMDS_MeshNode * n3,
					   const SMDS_MeshNode * n4,
					   const SMDS_MeshNode * n5, 
					   int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
				     const SMDS_MeshNode * n2,
				     const SMDS_MeshNode * n3,
				     const SMDS_MeshNode * n4,
				     const SMDS_MeshNode * n5);

  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4, int n5, int n6, int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
					   const SMDS_MeshNode * n2,
					   const SMDS_MeshNode * n3,
					   const SMDS_MeshNode * n4,
					   const SMDS_MeshNode * n5,
					   const SMDS_MeshNode * n6, 
					   int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
				     const SMDS_MeshNode * n2,
				     const SMDS_MeshNode * n3,
				     const SMDS_MeshNode * n4,
				     const SMDS_MeshNode * n5,
				     const SMDS_MeshNode * n6);

  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
					   const SMDS_MeshNode * n2,
					   const SMDS_MeshNode * n3,
					   const SMDS_MeshNode * n4,
					   const SMDS_MeshNode * n5,
					   const SMDS_MeshNode * n6,
					   const SMDS_MeshNode * n7,
					   const SMDS_MeshNode * n8, 
					   int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
				     const SMDS_MeshNode * n2,
				     const SMDS_MeshNode * n3,
				     const SMDS_MeshNode * n4,
				     const SMDS_MeshNode * n5,
				     const SMDS_MeshNode * n6,
				     const SMDS_MeshNode * n7,
				     const SMDS_MeshNode * n8);
  
  // 2d order tetrahedron of 10 nodes
  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4,
                                           int n12,int n23,int n31,
                                           int n14,int n24,int n34, int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
					   const SMDS_MeshNode * n2,
					   const SMDS_MeshNode * n3,
					   const SMDS_MeshNode * n4, 
					   const SMDS_MeshNode * n12,
					   const SMDS_MeshNode * n23,
					   const SMDS_MeshNode * n31,
					   const SMDS_MeshNode * n14, 
					   const SMDS_MeshNode * n24,
					   const SMDS_MeshNode * n34, 
					   int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
				     const SMDS_MeshNode * n2,
				     const SMDS_MeshNode * n3,
				     const SMDS_MeshNode * n4,
                                     const SMDS_MeshNode * n12,
                                     const SMDS_MeshNode * n23,
                                     const SMDS_MeshNode * n31,
                                     const SMDS_MeshNode * n14, 
                                     const SMDS_MeshNode * n24,
                                     const SMDS_MeshNode * n34);

  // 2d order pyramid of 13 nodes
  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4, int n5,
                                           int n12,int n23,int n34,int n41,
                                           int n15,int n25,int n35,int n45,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
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
					   int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
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
                                     const SMDS_MeshNode * n45);

  // 2d order Pentahedron with 15 nodes
  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3,
                                           int n4, int n5, int n6,
                                           int n12,int n23,int n31,
                                           int n45,int n56,int n64,
                                           int n14,int n25,int n36,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
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
					   int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
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
                                     const SMDS_MeshNode * n36);

  // 2d order Hexahedrons with 20 nodes
  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4,
                                           int n5, int n6, int n7, int n8,
                                           int n12,int n23,int n34,int n41,
                                           int n56,int n67,int n78,int n85,
                                           int n15,int n26,int n37,int n48,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
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
					   int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
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
                                     const SMDS_MeshNode * n48);

  virtual SMDS_MeshFace* AddPolygonalFaceWithID (std::vector<int> nodes_ids,
                                                 const int        ID);

  virtual SMDS_MeshFace* AddPolygonalFaceWithID (std::vector<const SMDS_MeshNode*> nodes,
                                                 const int                         ID);

  virtual SMDS_MeshFace* AddPolygonalFace (std::vector<const SMDS_MeshNode*> nodes);

  virtual SMDS_MeshVolume* AddPolyhedralVolumeWithID
                           (std::vector<int> nodes_ids,
                            std::vector<int> quantities,
                            const int        ID);

  virtual SMDS_MeshVolume* AddPolyhedralVolumeWithID
                           (std::vector<const SMDS_MeshNode*> nodes,
                            std::vector<int>                  quantities,
                            const int                         ID);

  virtual SMDS_MeshVolume* AddPolyhedralVolume
                           (std::vector<const SMDS_MeshNode*> nodes,
                            std::vector<int>                  quantities);

  void MoveNode(const SMDS_MeshNode *, double x, double y, double z);
  virtual void RemoveNode(const SMDS_MeshNode *);
  void RemoveElement(const SMDS_MeshElement *);

  /*! Remove only the given element/node and only if it is free.
   *  Methods do not work for meshes with descendants.
   *  Implemented for fast cleaning of meshes.
   */
  void RemoveFreeNode   (const SMDS_MeshNode *,    SMESHDS_SubMesh *, bool fromGroups=true);
  void RemoveFreeElement(const SMDS_MeshElement *, SMESHDS_SubMesh *, bool fromGroups=true);

  void ClearMesh();

  bool ChangeElementNodes(const SMDS_MeshElement * elem,
                          const SMDS_MeshNode    * nodes[],
                          const int                nbnodes);
  bool ChangePolygonNodes(const SMDS_MeshElement * elem,
                          std::vector<const SMDS_MeshNode*> nodes);
  bool ChangePolyhedronNodes(const SMDS_MeshElement * elem,
                             std::vector<const SMDS_MeshNode*> nodes,
                             std::vector<int>                  quantities);
  void Renumber (const bool isNodes, const int startID=1, const int deltaID=1);

  void SetNodeInVolume(SMDS_MeshNode * aNode, const TopoDS_Shell & S);
  void SetNodeInVolume(SMDS_MeshNode * aNode, const TopoDS_Solid & S);
  void SetNodeOnFace(SMDS_MeshNode * aNode, const TopoDS_Face & S, double u=0., double v=0.);
  void SetNodeOnEdge(SMDS_MeshNode * aNode, const TopoDS_Edge & S, double u=0.);
  void SetNodeOnVertex(SMDS_MeshNode * aNode, const TopoDS_Vertex & S);
  void UnSetNodeOnShape(const SMDS_MeshNode * aNode);
  void SetMeshElementOnShape(const SMDS_MeshElement * anElt,
			     const TopoDS_Shape & S);
  void UnSetMeshElementOnShape(const SMDS_MeshElement * anElt,
			       const TopoDS_Shape & S);
  bool HasMeshElements(const TopoDS_Shape & S);
  SMESHDS_SubMesh * MeshElements(const TopoDS_Shape & S) const;
  SMESHDS_SubMesh * MeshElements(const int Index);
  std::list<int> SubMeshIndices();
  const std::map<int,SMESHDS_SubMesh*>& SubMeshes() const
  { return myShapeIndexToSubMesh; }

  bool HasHypothesis(const TopoDS_Shape & S);
  const std::list<const SMESHDS_Hypothesis*>& GetHypothesis(const TopoDS_Shape & S) const;
  SMESHDS_Script * GetScript();
  void ClearScript();
  int ShapeToIndex(const TopoDS_Shape & aShape) const;
  const TopoDS_Shape& IndexToShape(int ShapeIndex) const;
  int MaxShapeIndex() const { return myIndexToShape.Extent(); }

  SMESHDS_SubMesh * NewSubMesh(int Index);
  int AddCompoundSubmesh(const TopoDS_Shape& S, TopAbs_ShapeEnum type = TopAbs_SHAPE);
  void SetNodeInVolume(const SMDS_MeshNode * aNode, int Index);
  void SetNodeOnFace(SMDS_MeshNode * aNode, int Index , double u=0., double v=0.);
  void SetNodeOnEdge(SMDS_MeshNode * aNode, int Index , double u=0.);
  void SetNodeOnVertex(SMDS_MeshNode * aNode, int Index);
  void SetMeshElementOnShape(const SMDS_MeshElement * anElt, int Index);

  void AddGroup (SMESHDS_GroupBase* theGroup)      { myGroups.insert(theGroup); }
  void RemoveGroup (SMESHDS_GroupBase* theGroup)   { myGroups.erase(theGroup); }
  int GetNbGroups() const                      { return myGroups.size(); }
  const std::set<SMESHDS_GroupBase*>& GetGroups() const { return myGroups; }

  bool IsGroupOfSubShapes (const TopoDS_Shape& aSubShape) const;

  ~SMESHDS_Mesh();
  
private:
  void addNodeToSubmesh( const SMDS_MeshNode* aNode, int Index )
  {
    //Update or build submesh
    std::map<int,SMESHDS_SubMesh*>::iterator it = myShapeIndexToSubMesh.find( Index );
    if ( it == myShapeIndexToSubMesh.end() )
      it = myShapeIndexToSubMesh.insert( std::make_pair(Index, new SMESHDS_SubMesh() )).first;
    it->second->AddNode( aNode ); // add aNode to submesh
    }
  
  /*int HashCode( const TopoDS_Shape& S, const Standard_Integer theUpper ) const
  {
      return S.HashCode(2147483647);
  }*/ 

  typedef std::list<const SMESHDS_Hypothesis*> THypList;

  typedef NCollection_DataMap< TopoDS_Shape, THypList > ShapeToHypothesis;

  ShapeToHypothesis          myShapeToHypothesis;

  int                        myMeshID;
  TopoDS_Shape               myShape;

  typedef std::map<int,SMESHDS_SubMesh*> TShapeIndexToSubMesh;
  TShapeIndexToSubMesh myShapeIndexToSubMesh;

  TopTools_IndexedMapOfShape myIndexToShape;

  typedef std::set<SMESHDS_GroupBase*> TGroups;
  TGroups myGroups;

  SMESHDS_Script*            myScript;
  bool                       myIsEmbeddedMode;

  // optimize addition of nodes/elements to submeshes by, SetNodeInVolume() etc:
  // avoid search of submeshes in maps
  bool add( const SMDS_MeshElement* elem, SMESHDS_SubMesh* subMesh );
  SMESHDS_SubMesh* getSubmesh( const TopoDS_Shape & shape);
  SMESHDS_SubMesh* getSubmesh( const int            Index );
  int                        myCurSubID;
  TopoDS_Shape               myCurSubShape;
  SMESHDS_SubMesh*           myCurSubMesh;
};


#endif

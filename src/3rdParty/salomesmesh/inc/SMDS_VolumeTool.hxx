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
// File      : SMDS_VolumeTool.hxx
// Module    : SMESH
// Created   : Tue Jul 13 11:27:17 2004
// Author    : Edward AGAPOV (eap)
//
#ifndef SMDS_VolumeTool_HeaderFile
#define SMDS_VolumeTool_HeaderFile

#include "SMESH_SMDS.hxx"

class SMDS_MeshElement;
class SMDS_MeshNode;
class SMDS_VtkVolume;
class SMDS_MeshVolume;

#include <vector>
#include <set>
#include <map>

// =========================================================================
//
// Class providing topological and other information about SMDS_MeshVolume:
// allows iteration on faces or, to be precise, on nodes of volume sides;
// provides info on nodes connection etc.
//
// =========================================================================

class SMDS_EXPORT SMDS_VolumeTool
{
 public:

  enum VolumeType { UNKNOWN = -1, TETRA = 0, PYRAM, PENTA, HEXA,
                    HEX_PRISM, QUAD_TETRA, QUAD_PYRAM, QUAD_PENTA, QUAD_HEXA,
                    POLYHEDA, NB_VOLUME_TYPES }; // to keep synchronised with GetSize()!

  SMDS_VolumeTool ();
  ~SMDS_VolumeTool ();
  SMDS_VolumeTool (const SMDS_MeshElement* theVolume,
                   const bool              ignoreCentralNodes=true);

  bool Set (const SMDS_MeshElement* theVolume,
            const bool              ignoreCentralNodes=true);
  // Set volume.
  // Return false if theVolume is not of type SMDSAbs_Volume.
  // ignoreCentralNodes makes skip nodes at face centers when returning
  // nodes of faces of SMDSEntity_TriQuad_Hexa

  const SMDS_MeshVolume* Element() const;
  // return element

  int ID() const;
  // return element ID

  bool IsPoly() const { return myPolyedre != 0; }

  // -----------------------
  // general info
  // -----------------------

  VolumeType GetVolumeType() const;

  bool IsForward() const { return myVolForward; }
  // Check volume orientation. can be changed by Inverse().
  // See node order of forward volumes at the file bottom

  void Inverse();
  // Change nodes order as if the volume changes its orientation:
  // top and bottom faces are reversed.
  // Result of IsForward() and methods returning nodes change

  const SMDS_MeshNode** GetNodes() { return &myVolumeNodes[0]; }
  // Return array of volume nodes

  int NbNodes() { return myVolumeNodes.size(); }
  // Return array of volume nodes

  double GetSize() const;
  // Return element volume

  bool GetBaryCenter (double & X, double & Y, double & Z) const;

  bool IsOut(double X, double Y, double Z, double tol) const;
  // Classify a point

  // -----------------------
  // info on node connection
  // -----------------------

  bool IsLinked (const SMDS_MeshNode* theNode1,
                 const SMDS_MeshNode* theNode2,
                 const bool           theIgnoreMediumNodes=false) const;
  // Return true if theNode1 is linked with theNode2.
  // If theIgnoreMediumNodes then corner nodes of quadratic cell are considered linked as well

  bool IsLinked (const int theNode1Index,
                 const int theNode2Index,
                 bool      theIgnoreMediumNodes=false) const;
  // Return true if the node with theNode1Index is linked
  // with the node with theNode2Index
  // If theIgnoreMediumNodes then corner nodes of quadratic cell are considered linked as well

  int GetNodeIndex(const SMDS_MeshNode* theNode) const;
  // Return an index of theNode

  int GetAllExistingEdges(std::vector<const SMDS_MeshElement*> & edges) const;
  // Fill vector with boundary edges existing in the mesh

  double MinLinearSize2() const;
  // Return minimal square distance between connected corner nodes

  double MaxLinearSize2() const;
  // Return maximal square distance between connected corner nodes

  // -------------
  // info on faces
  // -------------
  // For all elements, 0-th face is bottom based on the first nodes.
  // For prismatic elements (tetra,hexa,prisms), 1-th face is a top one.
  // For all elements, side faces follow order of bottom nodes

  void SetExternalNormal ();
  // Node order in faces  will be so that faces normals are external.

  int NbFaces() const { return myNbFaces; }
  // Return number of faces of the volume. In the following
  // methods 0 <= faceIndex < NbFaces()

  int NbFaceNodes( int faceIndex ) const;
  // Return number of nodes in the array of face nodes

  const int* GetFaceNodesIndices( int faceIndex ) const;
  // Return the array of face nodes indices
  // To comfort link iteration, the array
  // length == NbFaceNodes( faceIndex ) + 1 and
  // the last node index == the first one, except for
  // SMDSEntity_TriQuad_Hexa at ignoreCentralNodes==false.
  // NOTE: for the quadratic volume, node indices are in the order the nodes encounter
  // in face boundary and not the order they are in the mesh face

  const SMDS_MeshNode** GetFaceNodes( int faceIndex ) const;
  // Return the array of face nodes.
  // To comfort link iteration, the array
  // length == NbFaceNodes( faceIndex ) + 1 and
  // the last node == the first one, except for
  // SMDSEntity_TriQuad_Hexa at ignoreCentralNodes==false.
  // NOTE: for the quadratic volume, nodes are in the order they encounter in face boundary
  // and not the order they are in the mesh face
  // WARNING: do not modify the array, some methods
  //          work basing on its contents

  bool GetFaceNodes (int faceIndex,
                     std::set<const SMDS_MeshNode*>& theFaceNodes ) const;
  // Return a set of face nodes.

  bool IsFaceExternal( int faceIndex ) const;
  // Check normal orientation of a face.
  // SetExternalNormal() is taken into account.

  bool IsFreeFace(  int faceIndex, const SMDS_MeshElement** otherVol=0 ) const;
  // Fast check that only one volume is built on nodes of a given face
  // otherVol returns another volume sharing the given facet

  bool IsFreeFaceAdv(  int faceIndex, const SMDS_MeshElement** otherVol=0 ) const;
  // Thorough check that all volumes built on the face nodes lays on one side
  // otherVol returns another volume sharing the given facet

  bool GetFaceNormal (int faceIndex, double & X, double & Y, double & Z) const;
  // Return a normal to a face

  bool GetFaceBaryCenter (int faceIndex, double & X, double & Y, double & Z) const;
  // Return barycenter of a face

  double GetFaceArea( int faceIndex ) const;
  // Return face area

  int GetOppFaceIndex( int faceIndex ) const;
  // Return index of the opposite face if it exists, else -1.

  int GetCenterNodeIndex( int faceIndex ) const;
  // Return index of the node located at face center of a quadratic element like HEX27

  int GetFaceIndex( const std::set<const SMDS_MeshNode*>& theFaceNodes,
                    const int                             theFaceIndexHint=-1) const;
  // Return index of a face formed by theFaceNodes.
  // Return -1 if a face not found

  //int GetFaceIndex( const std::set<int>& theFaceNodesIndices );
  // Return index of a face formed by theFaceNodesIndices
  // Return -1 if a face not found

  int GetAllExistingFaces(std::vector<const SMDS_MeshElement*> & faces) const;
  // Fill vector with boundary faces existing in the mesh

  // ------------------------
  // static methods for faces
  // ------------------------

  static VolumeType GetType(int nbNodes);
  // return VolumeType by nb of nodes in a volume

  static int NbFaces( VolumeType type );
  // return nb of faces by volume type

  static const int* GetFaceNodesIndices(VolumeType type,
                                        int        faceIndex,
                                        bool       external);
  // Return the array of face nodes indices
  // To comfort link iteration, the array
  // length == NbFaceNodes( faceIndex ) + 1 and
  // the last node index == the first one.
  // Nodes at face centers of SMDSEntity_TriQuad_Hexa are ignored

  static int NbFaceNodes(VolumeType type, int faceIndex );
  // Return number of nodes in the array of face nodes
  // Nodes at face centers of SMDSEntity_TriQuad_Hexa are ignored

  static int NbCornerNodes(VolumeType type);
  // Useful to know nb of corner nodes of a quadratic volume

  static int GetOppFaceIndexOfHex( int faceIndex );
  // Return index of the opposite face of the hexahedron

 private:

  bool setFace( int faceIndex ) const;

  bool projectNodesToNormal( int faceIndex, double& minProj, double& maxProj ) const;

  const SMDS_MeshElement* myVolume;
  const SMDS_VtkVolume*   myPolyedre;
  bool                    myIgnoreCentralNodes;

  bool                    myVolForward;
  int                     myNbFaces;
  std::vector<const SMDS_MeshNode*> myVolumeNodes;
  std::vector< int >      myPolyIndices; // of a myCurFace
  std::vector< int >      myPolyQuantities;
  std::vector< int >      myPolyFacetOri; // -1-in, +1-out, 0-undef

  typedef std::pair<int,int> Link;
  std::map<Link, int>     myFwdLinks; // used in IsFaceExternal() to find out myPolyFacetOri

  mutable bool            myExternalFaces;

  mutable const int*      myAllFacesNodeIndices_F;
  mutable const int*      myAllFacesNodeIndices_RE;
  mutable const int*      myAllFacesNbNodes;
  mutable int             myMaxFaceNbNodes;

  struct SaveFacet;
  struct Facet
  {
    int                   myIndex;
    int                   myNbNodes;
    int*                  myNodeIndices;
    std::vector<const SMDS_MeshNode*> myNodes;
  };
  mutable Facet           myCurFace;

};
#endif


///////////////////////////////////////////////////////////////////////////
//
//                   ORDER OF NODES OF FORWARD ELEMENT
//
///////////////////////////////////////////////////////////////////////////
/*
//           N3
//           +
//          /|\
//         / | \
//        /  |  \
//    N0 +---|---+ N1                TETRAHEDRON
//       \   |   /
//        \  |  /
//         \ | /
//          \|/
//           +
//           N2

//            + N4
//           /|\
//          / | \
//         /  |  \
//        /   |   \
//    N3 +---------+ N5
//       |    |    |
//       |    + N1 |
//       |   / \   |                PENTAHEDRON
//       |  /   \  |
//       | /     \ |
//       |/       \|
//    N0 +---------+ N2

//         N5+----------+N6
//          /|         /|
//         / |        / |
//        /  |       /  |
//     N4+----------+N7 |
//       |   |      |   |           HEXAHEDRON
//       |   |      |   |
//       |   |      |   |
//       | N1+------|---+N2
//       |  /       |  /
//       | /        | /
//       |/         |/
//     N0+----------+N3
//
*/

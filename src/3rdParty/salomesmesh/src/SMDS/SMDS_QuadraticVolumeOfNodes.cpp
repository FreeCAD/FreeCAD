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
// File:      SMDS_QuadraticVolumeOfNodes.cxx
// Created:   17.01.06 09:46:11
// Author:    Sergey KUUL
//
#include "SMDS_QuadraticVolumeOfNodes.hxx"

#include "SMDS_IteratorOfElements.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_SetIterator.hxx"
#include "SMDS_VolumeTool.hxx"

#include "utilities.h"

using namespace std;


//=======================================================================
//function : SMDS_QuadraticVolumeOfNodes()
//purpose  : Constructor tetrahedron of 10 nodes
//=======================================================================

SMDS_QuadraticVolumeOfNodes::SMDS_QuadraticVolumeOfNodes
                                               (const SMDS_MeshNode * n1,
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
  //MESSAGE("*********************************************** SMDS_QuadraticVolumeOfNodes");
  myNodes.resize( 10 );
  myNodes[ 0 ] = n1;
  myNodes[ 1 ] = n2;
  myNodes[ 2 ] = n3;
  myNodes[ 3 ] = n4;
  myNodes[ 4 ] = n12;
  myNodes[ 5 ] = n23;
  myNodes[ 6 ] = n31;
  myNodes[ 7 ] = n14;
  myNodes[ 8 ] = n24;
  myNodes[ 9 ] = n34;
}


//=======================================================================
//function : SMDS_QuadraticVolumeOfNodes()
//purpose  : Constructor pyramid of 13 nodes
//=======================================================================

SMDS_QuadraticVolumeOfNodes::SMDS_QuadraticVolumeOfNodes
                                               (const SMDS_MeshNode * n1,
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
  //MESSAGE("*********************************************** SMDS_QuadraticVolumeOfNodes");
  myNodes.resize( 13 );
  myNodes[ 0 ] = n1;
  myNodes[ 1 ] = n2;
  myNodes[ 2 ] = n3;
  myNodes[ 3 ] = n4;
  myNodes[ 4 ] = n5;
  myNodes[ 5 ] = n12;
  myNodes[ 6 ] = n23;
  myNodes[ 7 ] = n34;
  myNodes[ 8 ] = n41;
  myNodes[ 9 ] = n15;
  myNodes[ 10 ] = n25;
  myNodes[ 11 ] = n35;
  myNodes[ 12 ] = n45;
}


//=======================================================================
//function : SMDS_QuadraticVolumeOfNodes()
//purpose  : Constructor Pentahedron with 15 nodes
//=======================================================================

SMDS_QuadraticVolumeOfNodes::SMDS_QuadraticVolumeOfNodes
                                               (const SMDS_MeshNode * n1,
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
  //MESSAGE("*********************************************** SMDS_QuadraticVolumeOfNodes");
  myNodes.resize( 15 );
  myNodes[ 0 ] = n1;
  myNodes[ 1 ] = n2;
  myNodes[ 2 ] = n3;
  myNodes[ 3 ] = n4;
  myNodes[ 4 ] = n5;
  myNodes[ 5 ] = n6;
  myNodes[ 6 ] = n12;
  myNodes[ 7 ] = n23;
  myNodes[ 8 ] = n31;
  myNodes[ 9 ] = n45;
  myNodes[ 10 ] = n56;
  myNodes[ 11 ] = n64;
  myNodes[ 12 ] = n14;
  myNodes[ 13 ] = n25;
  myNodes[ 14 ] = n36;
}


//=======================================================================
//function : SMDS_QuadraticVolumeOfNodes()
//purpose  : Constructor Hexahedrons with 20 nodes
//=======================================================================

SMDS_QuadraticVolumeOfNodes::SMDS_QuadraticVolumeOfNodes
                                               (const SMDS_MeshNode * n1,
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
  //MESSAGE("*********************************************** SMDS_QuadraticVolumeOfNodes");
  myNodes.resize( 20 );
  myNodes[ 0 ] = n1;
  myNodes[ 1 ] = n2;
  myNodes[ 2 ] = n3;
  myNodes[ 3 ] = n4;
  myNodes[ 4 ] = n5;
  myNodes[ 5 ] = n6;
  myNodes[ 6 ] = n7;
  myNodes[ 7 ] = n8;
  myNodes[ 8 ] = n12;
  myNodes[ 9 ] = n23;
  myNodes[ 10 ] = n34;
  myNodes[ 11 ] = n41;
  myNodes[ 12 ] = n56;
  myNodes[ 13 ] = n67;
  myNodes[ 14 ] = n78;
  myNodes[ 15 ] = n85;
  myNodes[ 16 ] = n15;
  myNodes[ 17 ] = n26;
  myNodes[ 18 ] = n37;
  myNodes[ 19 ] = n48;
}


//=======================================================================
//function : IsMediumNode
//purpose  : 
//=======================================================================

bool SMDS_QuadraticVolumeOfNodes::IsMediumNode(const SMDS_MeshNode* node) const
{
  int nbCorners = 0;
  switch (myNodes.size()) {
  case 10: nbCorners = 4; break;
  case 13: nbCorners = 5; break;
  case 15: nbCorners = 6; break;
  default: nbCorners = 8;
  }
  for ( int i = nbCorners; i<myNodes.size(); i++) {
    if(myNodes[i]==node) return true;
  }
  return false;
}


//=======================================================================
//function : ChangeNodes
//purpose  : 
//=======================================================================

bool SMDS_QuadraticVolumeOfNodes::ChangeNodes(const SMDS_MeshNode* nodes[],
                                              const int            nbNodes)
{
  if( nbNodes==10 || nbNodes==13 || nbNodes==15 || nbNodes==20 ) {
    myNodes.resize(nbNodes);
    int i=0;
    for(; i<nbNodes; i++) {
      myNodes[i] = nodes[i];
    }
    return true;
  }
  return false;
}


//=======================================================================
//function : NbNodes
//purpose  : 
//=======================================================================
int SMDS_QuadraticVolumeOfNodes::NbNodes() const
{
  return myNodes.size();
}


//=======================================================================
//function : NbEdges
//purpose  : 
//=======================================================================
int SMDS_QuadraticVolumeOfNodes::NbEdges() const
{
  if(myNodes.size()==10)
    return 6;
  else if(myNodes.size()==13)
    return 8;
  else if(myNodes.size()==15)
    return 9;
  else
    return 12;
}


//=======================================================================
//function : NbFaces
//purpose  : 
//=======================================================================
int SMDS_QuadraticVolumeOfNodes::NbFaces() const
{
  if(myNodes.size()==10)
    return 4;
  else if(myNodes.size()==20)
    return 6;
  else
    return 5;
}

//=======================================================================
//function : Print
//purpose  : 
//=======================================================================
void SMDS_QuadraticVolumeOfNodes::Print(ostream & OS) const
{
  OS << "quadratic volume <" << GetID() << " > : ";
  int i, nbNodes = myNodes.size();
  for (i = 0; i < nbNodes - 1; i++)
    OS << myNodes[i] << ",";
  OS << myNodes[i] << ") " << endl;
}


//=======================================================================
//private class : SMDS_QuadraticVolumeOfNodes_MyIterator
//purpose  : 
//=======================================================================

class SMDS_QuadraticVolumeOfNodes_MyIterator : public SMDS_NodeVectorElemIterator
{
public:
  SMDS_QuadraticVolumeOfNodes_MyIterator(const vector<const SMDS_MeshNode *>& s):
    SMDS_NodeVectorElemIterator( s.begin(), s.end() ) {}
};

/// ===================================================================
/*!
 * \brief Iterator on faces or edges of volume
 */
/// ===================================================================

class _MySubIterator : public SMDS_ElemIterator
{
  vector< const SMDS_MeshElement* > myElems;
  int myIndex;
public:
  _MySubIterator(const SMDS_MeshVolume* vol, SMDSAbs_ElementType type):myIndex(0) {
    SMDS_VolumeTool vTool(vol);
    if (type == SMDSAbs_Face)
      vTool.GetAllExistingFaces( myElems );
    else
      vTool.GetAllExistingFaces( myElems );
  }
  /// Return true if and only if there are other object in this iterator
  virtual bool more() { return myIndex < myElems.size(); }

  /// Return the current object and step to the next one
  virtual const SMDS_MeshElement* next() { return myElems[ myIndex++ ]; }
};

//=======================================================================
//function : elementsIterator
//purpose  : 
//=======================================================================

SMDS_ElemIteratorPtr SMDS_QuadraticVolumeOfNodes::elementsIterator
                                         (SMDSAbs_ElementType type) const
{
  switch(type)
  {
  case SMDSAbs_Volume:
    return SMDS_MeshElement::elementsIterator(SMDSAbs_Volume);
  case SMDSAbs_Node:
    return SMDS_ElemIteratorPtr(new SMDS_QuadraticVolumeOfNodes_MyIterator(myNodes));
  case SMDSAbs_Edge:
    return SMDS_ElemIteratorPtr(new _MySubIterator(this,SMDSAbs_Edge));
    break;
  case SMDSAbs_Face:
    return SMDS_ElemIteratorPtr(new _MySubIterator(this,SMDSAbs_Face));
    break;
  default:
    return SMDS_ElemIteratorPtr
      (new SMDS_IteratorOfElements
       (this,type,SMDS_ElemIteratorPtr
        (new SMDS_QuadraticVolumeOfNodes_MyIterator(myNodes))));
  }
  return SMDS_ElemIteratorPtr();
}

/*!
 * \brief Return node by its index
 * \param ind - node index
 * \retval const SMDS_MeshNode* - the node
 */
const SMDS_MeshNode* SMDS_QuadraticVolumeOfNodes::GetNode(const int ind) const
{
  return myNodes[ ind ];
}

SMDSAbs_EntityType SMDS_QuadraticVolumeOfNodes::GetEntityType() const
{
  SMDSAbs_EntityType aType = SMDSEntity_Quad_Tetra;
  switch(NbNodes())
  {
  case 10: aType = SMDSEntity_Quad_Tetra;   break;
  case 13: aType = SMDSEntity_Quad_Pyramid; break;
  case 15: aType = SMDSEntity_Quad_Penta;   break;
  case 20:
  default: aType = SMDSEntity_Quad_Hexa;    break;
  }
  return aType;
}

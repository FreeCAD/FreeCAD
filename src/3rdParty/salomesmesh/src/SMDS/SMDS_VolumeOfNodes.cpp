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

#include "SMDS_VolumeOfNodes.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_SetIterator.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMDS_Mesh.hxx"
#include "utilities.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
/// Create an hexahedron. node 1,2,3,4 and 5,6,7,8 are quadrangle and
/// 5,1 and 7,3 are an edges.
///////////////////////////////////////////////////////////////////////////////
SMDS_VolumeOfNodes::SMDS_VolumeOfNodes(
                const SMDS_MeshNode * node1,
                const SMDS_MeshNode * node2,
                const SMDS_MeshNode * node3,
                const SMDS_MeshNode * node4,
                const SMDS_MeshNode * node5,
                const SMDS_MeshNode * node6,
                const SMDS_MeshNode * node7,
                const SMDS_MeshNode * node8)
{
  //MESSAGE("***************************************************** SMDS_VolumeOfNodes");
        myNbNodes = 8;
        myNodes = new const SMDS_MeshNode* [myNbNodes];
        myNodes[0]=node1;
        myNodes[1]=node2;
        myNodes[2]=node3;
        myNodes[3]=node4;
        myNodes[4]=node5;
        myNodes[5]=node6;
        myNodes[6]=node7;
        myNodes[7]=node8;
}

SMDS_VolumeOfNodes::SMDS_VolumeOfNodes(
                const SMDS_MeshNode * node1,
                const SMDS_MeshNode * node2,
                const SMDS_MeshNode * node3,
                const SMDS_MeshNode * node4)
{
  //MESSAGE("***************************************************** SMDS_VolumeOfNodes");
        myNbNodes = 4;
        myNodes = new const SMDS_MeshNode* [myNbNodes];
        myNodes[0]=node1;
        myNodes[1]=node2;
        myNodes[2]=node3;
        myNodes[3]=node4;
}

SMDS_VolumeOfNodes::SMDS_VolumeOfNodes(
                const SMDS_MeshNode * node1,
                const SMDS_MeshNode * node2,
                const SMDS_MeshNode * node3,
                const SMDS_MeshNode * node4,
                const SMDS_MeshNode * node5)
{
  //MESSAGE("***************************************************** SMDS_VolumeOfNodes");
        myNbNodes = 5;
        myNodes = new const SMDS_MeshNode* [myNbNodes];
        myNodes[0]=node1;
        myNodes[1]=node2;
        myNodes[2]=node3;
        myNodes[3]=node4;
        myNodes[4]=node5;
}

SMDS_VolumeOfNodes::SMDS_VolumeOfNodes(
                const SMDS_MeshNode * node1,
                const SMDS_MeshNode * node2,
                const SMDS_MeshNode * node3,
                const SMDS_MeshNode * node4,
                const SMDS_MeshNode * node5,
                const SMDS_MeshNode * node6)
{
  //MESSAGE("***************************************************** SMDS_VolumeOfNodes");
        myNbNodes = 6;
        myNodes = new const SMDS_MeshNode* [myNbNodes];
        myNodes[0]=node1;
        myNodes[1]=node2;
        myNodes[2]=node3;
        myNodes[3]=node4;
        myNodes[4]=node5;
        myNodes[5]=node6;
}

bool SMDS_VolumeOfNodes::ChangeNodes(const SMDS_MeshNode* nodes[],
                                     const int            nbNodes)
{
  if (nbNodes < 4 || nbNodes > 8 || nbNodes == 7)
    return false;

  delete [] myNodes;
  myNbNodes = nbNodes;
  myNodes = new const SMDS_MeshNode* [myNbNodes];
  for ( int i = 0; i < nbNodes; i++ )
    myNodes[ i ] = nodes [ i ];

  return true;
}

SMDS_VolumeOfNodes::~SMDS_VolumeOfNodes()
{
  if (myNodes != NULL) {
    delete [] myNodes;
    myNodes = NULL;
  }
}

void SMDS_VolumeOfNodes::Print(ostream & OS) const
{
        OS << "volume <" << GetID() << "> : ";
        int i;
        for (i = 0; i < NbNodes()-1; ++i) OS << myNodes[i] << ",";
        OS << myNodes[NbNodes()-1]<< ") " << endl;
}

int SMDS_VolumeOfNodes::NbFaces() const
{
        switch(NbNodes())
        {
        case 4: return 4;
        case 5: return 5;
        case 6: return 5;
        case 8: return 6;
        default: MESSAGE("invalid number of nodes");
        }
        return 0;
}

int SMDS_VolumeOfNodes::NbNodes() const
{
        return myNbNodes;
}

int SMDS_VolumeOfNodes::NbEdges() const
{
        switch(NbNodes())
        {
        case 4: return 6;
        case 5: return 8;
        case 6: return 9;
        case 8: return 12;
        default: MESSAGE("invalid number of nodes");
        }
        return 0;
}

/*!
 * \brief Iterator on node of volume
 */
class SMDS_VolumeOfNodes_MyIterator:public SMDS_NodeArrayElemIterator
{
 public:
  SMDS_VolumeOfNodes_MyIterator(const SMDS_MeshNode* const* s, int l):
    SMDS_NodeArrayElemIterator( s, & s[ l ]) {}
};

/*!
 * \brief Iterator on faces or edges of volume
 */
class _MySubIterator : public SMDS_ElemIterator
{
  vector< const SMDS_MeshElement* > myElems;
  int myIndex;
public:
  _MySubIterator(const SMDS_VolumeOfNodes* vol, SMDSAbs_ElementType type):myIndex(0) {
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

SMDS_ElemIteratorPtr SMDS_VolumeOfNodes::elementsIterator(SMDSAbs_ElementType type) const
{
  switch(type)
  {
  case SMDSAbs_Volume:
    return SMDS_MeshElement::elementsIterator(SMDSAbs_Volume);
  case SMDSAbs_Node:
    return SMDS_ElemIteratorPtr(new SMDS_VolumeOfNodes_MyIterator(myNodes,myNbNodes));
  case SMDSAbs_Face:
    return SMDS_ElemIteratorPtr(new _MySubIterator(this,SMDSAbs_Face));
  case SMDSAbs_Edge:
    return SMDS_ElemIteratorPtr(new _MySubIterator(this,SMDSAbs_Edge));
  default:
    MESSAGE("ERROR : Iterator not implemented");
    return SMDS_ElemIteratorPtr((SMDS_ElemIterator*)NULL);
  }
}

SMDSAbs_ElementType SMDS_VolumeOfNodes::GetType() const
{
        return SMDSAbs_Volume;
}

/*!
 * \brief Return node by its index
 * \param ind - node index
 * \retval const SMDS_MeshNode* - the node
 */
const SMDS_MeshNode* SMDS_VolumeOfNodes::GetNode(const int ind) const
{
  return myNodes[ ind ];
}

SMDSAbs_EntityType SMDS_VolumeOfNodes::GetEntityType() const
{
  SMDSAbs_EntityType aType = SMDSEntity_Tetra;
  switch(myNbNodes)
  {
  case 4: aType = SMDSEntity_Tetra;   break;
  case 5: aType = SMDSEntity_Pyramid; break;
  case 6: aType = SMDSEntity_Penta;   break;
  case 8:
  default: aType = SMDSEntity_Hexa;    break;
  }
  return aType;
}

SMDSAbs_GeometryType SMDS_VolumeOfNodes::GetGeomType() const
{
  SMDSAbs_GeometryType aType = SMDSGeom_NONE;
  switch(myNbNodes)
  {
  case 4: aType = SMDSGeom_TETRA;   break;
  case 5: aType = SMDSGeom_PYRAMID; break;
  case 6: aType = SMDSGeom_PENTA;   break;
  case 12: aType = SMDSGeom_HEXAGONAL_PRISM; break;
  case 8:
  default: aType = SMDSGeom_HEXA;    break;
  }
  return aType;
}


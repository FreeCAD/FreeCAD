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
// File      : SMDS_MeshInfo.hxx
// Created   : Mon Sep 24 18:32:41 2007
// Author    : Edward AGAPOV (eap)
//
#ifndef SMDS_MeshInfo_HeaderFile
#define SMDS_MeshInfo_HeaderFile

using namespace std;

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshElement.hxx"

class SMDS_EXPORT SMDS_MeshInfo
{
public:

  inline SMDS_MeshInfo();
  inline void Clear();

  int NbNodes() const { return myNbNodes; }

  inline int NbEdges      (SMDSAbs_ElementOrder order = ORDER_ANY) const;
  inline int NbFaces      (SMDSAbs_ElementOrder order = ORDER_ANY) const;
  inline int NbTriangles  (SMDSAbs_ElementOrder order = ORDER_ANY) const;
  inline int NbQuadrangles(SMDSAbs_ElementOrder order = ORDER_ANY) const;
  int NbPolygons() const { return myNbPolygons; }

  inline int NbVolumes (SMDSAbs_ElementOrder order = ORDER_ANY) const;
  inline int NbTetras  (SMDSAbs_ElementOrder order = ORDER_ANY) const;
  inline int NbHexas   (SMDSAbs_ElementOrder order = ORDER_ANY) const;
  inline int NbPyramids(SMDSAbs_ElementOrder order = ORDER_ANY) const;
  inline int NbPrisms  (SMDSAbs_ElementOrder order = ORDER_ANY) const;
  int NbPolyhedrons() const { return myNbPolyhedrons; }

private:
  friend class SMDS_Mesh;

  // methods to count NOT POLY elements
  inline void remove(const SMDS_MeshElement* el);
  inline void add   (const SMDS_MeshElement* el);
  inline int  index(SMDSAbs_ElementType type, int nbNodes);
  // methods to remove elements of ANY kind
  inline void RemoveEdge(const SMDS_MeshElement* el);
  inline void RemoveFace(const SMDS_MeshElement* el);
  inline void RemoveVolume(const SMDS_MeshElement* el);

  int myNbNodes;

  int myNbEdges      , myNbQuadEdges      ;
  int myNbTriangles  , myNbQuadTriangles  ;
  int myNbQuadrangles, myNbQuadQuadrangles;
  int myNbPolygons;

  int myNbTetras  , myNbQuadTetras  ;
  int myNbHexas   , myNbQuadHexas   ;
  int myNbPyramids, myNbQuadPyramids;
  int myNbPrisms  , myNbQuadPrisms  ;
  int myNbPolyhedrons;

  std::vector<int*> myNb; // pointers to myNb... fields
  std::vector<int>  myShift; // shift to get an index in myNb by elem->NbNodes()
};

inline SMDS_MeshInfo::SMDS_MeshInfo():
  myNbNodes(0),
  myNbEdges      (0), myNbQuadEdges      (0),
  myNbTriangles  (0), myNbQuadTriangles  (0),
  myNbQuadrangles(0), myNbQuadQuadrangles(0),
  myNbPolygons(0),
  myNbTetras  (0), myNbQuadTetras  (0),
  myNbHexas   (0), myNbQuadHexas   (0),
  myNbPyramids(0), myNbQuadPyramids(0),
  myNbPrisms  (0), myNbQuadPrisms  (0),
  myNbPolyhedrons(0)
{
  // Number of nodes in standard element types
  // n   v  f  e
  // o   o  a  d
  // d   l  c  g
  // e      e  e
  // -----------
  // 1      
  // 2         *
  // 3      *
  // 4   *  *  *
  // 5   *  
  // 6   *  *
  // 7      
  // 8   *  *
  // 9      
  // 10  *  
  // 11     
  // 12     
  // 13  *  
  // 14     
  // 15  *  
  // 16     
  // 17     
  // 18     
  // 19     
  // 20  *
  //
  // So to have a unique index for each type basing on nb of nodes, we use a shift:
  myShift.resize(SMDSAbs_Volume + 1, 0);
  myShift[ SMDSAbs_Face ] = +8; // 3->11, 4->12, 6->14, 8->16
  myShift[ SMDSAbs_Edge ] = -2; // 2->0, 4->2

  myNb.resize( index( SMDSAbs_Volume,20 ) + 1, NULL);
  myNb[ index( SMDSAbs_Node,1 )] = & myNbNodes;

  myNb[ index( SMDSAbs_Edge,2 )] = & myNbEdges;
  myNb[ index( SMDSAbs_Edge,4 )] = & myNbQuadEdges;

  myNb[ index( SMDSAbs_Face,3 )] = & myNbTriangles;
  myNb[ index( SMDSAbs_Face,4 )] = & myNbQuadrangles;
  myNb[ index( SMDSAbs_Face,6 )] = & myNbQuadTriangles;
  myNb[ index( SMDSAbs_Face,8 )] = & myNbQuadQuadrangles;

  myNb[ index( SMDSAbs_Volume, 4)]  = & myNbTetras;
  myNb[ index( SMDSAbs_Volume, 5)]  = & myNbPyramids;
  myNb[ index( SMDSAbs_Volume, 6)]  = & myNbPrisms;
  myNb[ index( SMDSAbs_Volume, 8)]  = & myNbHexas;
  myNb[ index( SMDSAbs_Volume, 10)] = & myNbQuadTetras;  
  myNb[ index( SMDSAbs_Volume, 13)] = & myNbQuadPyramids;
  myNb[ index( SMDSAbs_Volume, 15)] = & myNbQuadPrisms;  
  myNb[ index( SMDSAbs_Volume, 20)] = & myNbQuadHexas;   
}
inline void // Clear
SMDS_MeshInfo::Clear()
{ for ( int i=0; i<myNb.size(); ++i ) if ( myNb[i] ) (*myNb[i])=0;
  myNbPolygons=myNbPolyhedrons=0;
}
inline int // index
SMDS_MeshInfo::index(SMDSAbs_ElementType type, int nbNodes)
{ return nbNodes + myShift[ type ]; }

inline void // remove
SMDS_MeshInfo::remove(const SMDS_MeshElement* el)
{ --(*myNb[ index(el->GetType(), el->NbNodes()) ]); }

inline void // add
SMDS_MeshInfo::add(const SMDS_MeshElement* el)
{ ++(*myNb[ index(el->GetType(), el->NbNodes()) ]); }

inline void // RemoveEdge
SMDS_MeshInfo::RemoveEdge(const SMDS_MeshElement* el)
{ if ( el->IsQuadratic() ) --myNbQuadEdges; else --myNbEdges; }

inline void // RemoveFace
SMDS_MeshInfo::RemoveFace(const SMDS_MeshElement* el)
{ if ( el->IsPoly() ) --myNbPolygons; else remove( el ); }

inline void // RemoveVolume
SMDS_MeshInfo::RemoveVolume(const SMDS_MeshElement* el)
{ if ( el->IsPoly() ) --myNbPolyhedrons; else remove( el ); }

inline int // NbEdges
SMDS_MeshInfo::NbEdges      (SMDSAbs_ElementOrder order) const
{ return order == ORDER_ANY ? myNbEdges+myNbQuadEdges : order == ORDER_LINEAR ? myNbEdges : myNbQuadEdges; }

inline int // NbFaces
SMDS_MeshInfo::NbFaces      (SMDSAbs_ElementOrder order) const
{ return NbTriangles(order)+NbQuadrangles(order)+(order == ORDER_QUADRATIC ? 0 : myNbPolygons); }

inline int // NbTriangles
SMDS_MeshInfo::NbTriangles  (SMDSAbs_ElementOrder order) const
{ return order == ORDER_ANY ? myNbTriangles+myNbQuadTriangles : order == ORDER_LINEAR ? myNbTriangles : myNbQuadTriangles; }

inline int // NbQuadrangles
SMDS_MeshInfo::NbQuadrangles(SMDSAbs_ElementOrder order) const
{ return order == ORDER_ANY ? myNbQuadrangles+myNbQuadQuadrangles : order == ORDER_LINEAR ? myNbQuadrangles : myNbQuadQuadrangles; }

inline int // NbVolumes
SMDS_MeshInfo::NbVolumes (SMDSAbs_ElementOrder order) const
{ return NbTetras(order) + NbHexas(order) + NbPyramids(order) + NbPrisms(order) + (order == ORDER_QUADRATIC ? 0 : myNbPolyhedrons); }

inline int // NbTetras
SMDS_MeshInfo::NbTetras  (SMDSAbs_ElementOrder order) const
{ return order == ORDER_ANY ? myNbTetras+myNbQuadTetras : order == ORDER_LINEAR ? myNbTetras : myNbQuadTetras; }

inline int // NbHexas
SMDS_MeshInfo::NbHexas   (SMDSAbs_ElementOrder order) const
{ return order == ORDER_ANY ? myNbHexas+myNbQuadHexas : order == ORDER_LINEAR ? myNbHexas : myNbQuadHexas; }

inline int // NbPyramids
SMDS_MeshInfo::NbPyramids(SMDSAbs_ElementOrder order) const
{ return order == ORDER_ANY ? myNbPyramids+myNbQuadPyramids : order == ORDER_LINEAR ? myNbPyramids : myNbQuadPyramids; }

inline int // NbPrisms
SMDS_MeshInfo::NbPrisms  (SMDSAbs_ElementOrder order) const
{ return order == ORDER_ANY ? myNbPrisms+myNbQuadPrisms : order == ORDER_LINEAR ? myNbPrisms : myNbQuadPrisms; }

#endif

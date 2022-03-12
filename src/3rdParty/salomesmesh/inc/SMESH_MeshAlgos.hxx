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
// File      : SMESH_MeshAlgos.hxx
// Created   : Tue Apr 30 18:00:36 2013
// Author    : Edward AGAPOV (eap)

// This file holds some low level algorithms extracted from SMESH_MeshEditor
// to make them accessible from Controls package


#ifndef __SMESH_MeshAlgos_HXX__
#define __SMESH_MeshAlgos_HXX__

#include "SMESH_Utils.hxx"

#include "SMDSAbs_ElementType.hxx"
#include "SMDS_ElemIterator.hxx"
#include "SMESH_TypeDefs.hxx"

#include <TopAbs_State.hxx>
#include <vector>

class gp_Pnt;
class gp_Ax1;
class SMDS_MeshNode;
class SMDS_MeshElement;
class SMDS_Mesh;

//=======================================================================
/*!
 * \brief Searcher for the node closest to a point
 */
//=======================================================================

struct SMESHUtils_EXPORT SMESH_NodeSearcher
{
  virtual ~SMESH_NodeSearcher() {}
  virtual const SMDS_MeshNode* FindClosestTo( const gp_Pnt& pnt ) = 0;
  virtual void MoveNode( const SMDS_MeshNode* node, const gp_Pnt& toPnt ) = 0;
  virtual int  FindNearPoint(const gp_Pnt&                        point,
                             const double                         tolerance,
                             std::vector< const SMDS_MeshNode* >& foundNodes) = 0;
};

//=======================================================================
/*!
 * \brief Searcher for elements
 */
//=======================================================================

struct SMESHUtils_EXPORT SMESH_ElementSearcher
{
  /*!
   * \brief Find elements of given type where the given point is IN or ON.
   *        Returns nb of found elements and elements them-selves.
   *
   * 'ALL' type means elements of any type excluding nodes and 0D elements
   */
  virtual int FindElementsByPoint(const gp_Pnt&                           point,
                                  SMDSAbs_ElementType                     type,
                                  std::vector< const SMDS_MeshElement* >& foundElems) = 0;
  /*!
   * \brief Return an element most close to the given point
   */
  virtual const SMDS_MeshElement* FindClosestTo( const gp_Pnt&       point,
                                                 SMDSAbs_ElementType type) = 0;
  /*!
   * \brief Return elements possibly intersecting the line
   */
  virtual void GetElementsNearLine( const gp_Ax1&                           line,
                                    SMDSAbs_ElementType                     type,
                                    std::vector< const SMDS_MeshElement* >& foundElems) = 0;
  /*!
   * \brief Find out if the given point is out of closed 2D mesh.
   */
  virtual TopAbs_State GetPointState(const gp_Pnt& point) = 0;
  virtual ~SMESH_ElementSearcher();
};

namespace SMESH_MeshAlgos
{
  /*!
   * \brief Return true if the point is IN or ON of the element
   */
  SMESHUtils_EXPORT
  bool IsOut( const SMDS_MeshElement* element, const gp_Pnt& point, double tol );

  SMESHUtils_EXPORT
  double GetDistance( const SMDS_MeshElement* elem, const gp_Pnt& point );

  SMESHUtils_EXPORT
  double GetDistance( const SMDS_MeshEdge* edge, const gp_Pnt& point );

  SMESHUtils_EXPORT
  double GetDistance( const SMDS_MeshFace* face, const gp_Pnt& point );

  SMESHUtils_EXPORT
  double GetDistance( const SMDS_MeshVolume* volume, const gp_Pnt& point );

  SMESHUtils_EXPORT
  void GetBarycentricCoords( const gp_XY& point,
                             const gp_XY& t0, const gp_XY& t1, const gp_XY& t2,
                             double &    bc0, double &    bc1);

  /*!
   * Return a face having linked nodes n1 and n2 and which is
   * - not in avoidSet,
   * - in elemSet provided that !elemSet.empty()
   * i1 and i2 optionally returns indices of n1 and n2
   */
  SMESHUtils_EXPORT
  const SMDS_MeshElement* FindFaceInSet(const SMDS_MeshNode*    n1,
                                        const SMDS_MeshNode*    n2,
                                        const TIDSortedElemSet& elemSet,
                                        const TIDSortedElemSet& avoidSet,
                                        int*                    i1=0,
                                        int*                    i2=0);
  /*!
   * \brief Calculate normal of a mesh face
   */
  SMESHUtils_EXPORT
  bool FaceNormal(const SMDS_MeshElement* F, gp_XYZ& normal, bool normalized=true);

  /*!
   * \brief Return nodes common to two elements
   */
  SMESHUtils_EXPORT
  std::vector< const SMDS_MeshNode*> GetCommonNodes(const SMDS_MeshElement* e1,
                                                    const SMDS_MeshElement* e2);

  /*!
   * \brief Return SMESH_NodeSearcher. The caller is responsible for deleting it
   */
  SMESHUtils_EXPORT
  SMESH_NodeSearcher* GetNodeSearcher( SMDS_Mesh& mesh );

  /*!
   * \brief Return SMESH_ElementSearcher. The caller is responsible for deleting it
   */
  SMESHUtils_EXPORT
  SMESH_ElementSearcher* GetElementSearcher( SMDS_Mesh& mesh,
                                             double     tolerance=-1.);
  SMESHUtils_EXPORT
  SMESH_ElementSearcher* GetElementSearcher( SMDS_Mesh& mesh,
                                             SMDS_ElemIteratorPtr elemIt,
                                             double     tolerance=-1. );



  typedef std::vector<const SMDS_MeshNode*> TFreeBorder;
  typedef std::vector<TFreeBorder>          TFreeBorderVec;
  struct TFreeBorderPart
  {
    int _border; // border index within a TFreeBorderVec
    int _node1;  // node index within the border-th TFreeBorder
    int _node2;
    int _nodeLast;
  };
  typedef std::vector<TFreeBorderPart>  TCoincidentGroup;
  typedef std::vector<TCoincidentGroup> TCoincidentGroupVec;
  struct CoincidentFreeBorders
  {
    TFreeBorderVec      _borders;          // nodes of all free borders
    TCoincidentGroupVec _coincidentGroups; // groups of coincident parts of borders
  };

  /*!
   * Returns TFreeBorder's coincident within the given tolerance.
   * If the tolerance <= 0.0 then one tenth of an average size of elements adjacent
   * to free borders being compared is used.
   *
   * (Implemented in ./SMESH_FreeBorders.cxx)
   */
  SMESHUtils_EXPORT
  void FindCoincidentFreeBorders(SMDS_Mesh&              mesh,
                                 double                  tolerance,
                                 CoincidentFreeBorders & foundFreeBordes);
  

} // SMESH_MeshAlgos

#endif

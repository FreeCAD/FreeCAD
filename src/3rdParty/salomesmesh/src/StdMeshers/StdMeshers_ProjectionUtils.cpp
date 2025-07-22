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

//  SMESH SMESH : idl implementation based on 'SMESH' unit's classes
// File      : StdMeshers_ProjectionUtils.cxx
// Created   : Fri Oct 27 10:24:28 2006
// Author    : Edward AGAPOV (eap)
//
#include "StdMeshers_ProjectionUtils.hxx"

#include "StdMeshers_ProjectionSource1D.hxx"
#include "StdMeshers_ProjectionSource2D.hxx"
#include "StdMeshers_ProjectionSource3D.hxx"

#include "SMDS_EdgePosition.hxx"
#include "SMESH_Algo.hxx"
#include "SMESH_Block.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_Hypothesis.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "SMESH_MeshAlgos.hxx"

#include "utilities.h"

#include <BRepAdaptor_Surface.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom_Curve.hxx>
#include <TopAbs.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_Array1OfShape.hxx>
#include <TopTools_DataMapIteratorOfDataMapOfShapeListOfShape.hxx>
#include <TopTools_DataMapIteratorOfDataMapOfShapeShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <math_Gauss.hxx>

#include <numeric>
#include <limits>

using namespace std;


#define RETURN_BAD_RESULT(msg) { MESSAGE(")-: Error: " << msg); return false; }
#define CONT_BAD_RESULT(msg) { MESSAGE(")-: Error: " << msg); continue; }
#define SHOW_SHAPE(v,msg) \
// { \
//  if ( (v).IsNull() ) cout << msg << " NULL SHAPE" << endl; \
// else if ((v).ShapeType() == TopAbs_VERTEX) {\
//   gp_Pnt p = BRep_Tool::Pnt( TopoDS::Vertex( (v) ));\
//   cout<<msg<<" "<<shapeIndex((v))<<" ( "<<p.X()<<", "<<p.Y()<<", "<<p.Z()<<" )"<<endl;} \
// else {\
//   cout << msg << " "; TopAbs::Print((v).ShapeType(),cout) <<" "<<shapeIndex((v))<<endl;}\
// }
#define SHOW_LIST(msg,l) \
// { \
//     cout << msg << " ";\
//     list< TopoDS_Edge >::const_iterator e = l.begin();\
//     for ( int i = 0; e != l.end(); ++e, ++i ) {\
//       cout << i << "V (" << TopExp::FirstVertex( *e, true ).TShape().operator->() << ") "\
//            << i << "E (" << e->TShape().operator->() << "); "; }\
//     cout << endl;\
//   }

namespace HERE = StdMeshers_ProjectionUtils;

namespace {

  static SMESHDS_Mesh* theMeshDS[2] = { 0, 0 }; // used for debug only
  inline intptr_t shapeIndex(const TopoDS_Shape& S)
  {
    if ( theMeshDS[0] && theMeshDS[1] )
      return max(theMeshDS[0]->ShapeToIndex(S), theMeshDS[1]->ShapeToIndex(S) );
    return intptr_t(S.TShape().operator->());
  }

  //================================================================================
  /*!
   * \brief Write shape for debug purposes
   */
  //================================================================================

  bool storeShapeForDebug(const TopoDS_Shape& shape)
  {
#ifdef _DEBUG_
    const char* type[] ={"COMPOUND","COMPSOLID","SOLID","SHELL","FACE","WIRE","EDGE","VERTEX"};
    BRepTools::Write( shape, SMESH_Comment("/tmp/") << type[shape.ShapeType()] << "_"
                      << shape.TShape().operator->() << ".brep");
#endif
    return false;
  }
  
  //================================================================================
  /*!
   * \brief Reverse order of edges in a list and their orientation
    * \param edges - list of edges to reverse
    * \param nbEdges - number of edges to reverse
   */
  //================================================================================

  void reverseEdges( list< TopoDS_Edge > & edges, const int nbEdges, const int firstEdge=0)
  {
    SHOW_LIST("BEFORE REVERSE", edges);

    list< TopoDS_Edge >::iterator eIt = edges.begin();
    std::advance( eIt, firstEdge );
    list< TopoDS_Edge >::iterator eBackIt = eIt;
    for ( int i = 0; i < nbEdges; ++i, ++eBackIt )
      eBackIt->Reverse(); // reverse edge
    // reverse list
    --eBackIt;
    while ( eIt != eBackIt )
    {
      std::swap( *eIt, *eBackIt );
      SHOW_LIST("# AFTER SWAP", edges)
        if ( (++eIt) != eBackIt )
          --eBackIt;
    }
    SHOW_LIST("ATFER REVERSE", edges)
  }

  //================================================================================
  /*!
   * \brief Check if propagation is possible
    * \param theMesh1 - source mesh
    * \param theMesh2 - target mesh
    * \retval bool - true if possible
   */
  //================================================================================

  bool isPropagationPossible( SMESH_Mesh* theMesh1, SMESH_Mesh* theMesh2 )
  {
    if ( theMesh1 != theMesh2 ) {
      TopoDS_Shape mainShape1 = theMesh1->GetMeshDS()->ShapeToMesh();
      TopoDS_Shape mainShape2 = theMesh2->GetMeshDS()->ShapeToMesh();
      return mainShape1.IsSame( mainShape2 );
    }
    return true;
  }

  //================================================================================
  /*!
   * \brief Fix up association of edges in faces by possible propagation
    * \param nbEdges - nb of edges in an outer wire
    * \param edges1 - edges of one face
    * \param edges2 - matching edges of another face
    * \param theMesh1 - mesh 1
    * \param theMesh2 - mesh 2
    * \retval bool - true if association was fixed
   */
  //================================================================================

  bool fixAssocByPropagation( const int             nbEdges,
                              list< TopoDS_Edge > & edges1,
                              list< TopoDS_Edge > & edges2,
                              SMESH_Mesh*           theMesh1,
                              SMESH_Mesh*           theMesh2)
  {
    if ( nbEdges == 2 && isPropagationPossible( theMesh1, theMesh2 ) )
    {
      list< TopoDS_Edge >::iterator eIt2 = ++edges2.begin(); // 2nd edge of the 2nd face
      TopoDS_Edge edge2 = HERE::GetPropagationEdge( theMesh1, *eIt2, edges1.front() ).second;
      if ( !edge2.IsNull() ) { // propagation found for the second edge
        reverseEdges( edges2, nbEdges );
        return true;
      }
    }
    return false;
  }

  //================================================================================
  /*!
   * \brief Associate faces having one edge in the outer wire.
   *       No check is done if there is really only one outer edge
   */
  //================================================================================

  bool assocFewEdgesFaces( const TopoDS_Face&     face1,
                           SMESH_Mesh*            mesh1,
                           const TopoDS_Face&     face2,
                           SMESH_Mesh*            mesh2,
                           HERE::TShapeShapeMap & theMap)
  {
    TopoDS_Vertex v1 = TopoDS::Vertex( HERE::OuterShape( face1, TopAbs_VERTEX ));
    TopoDS_Vertex v2 = TopoDS::Vertex( HERE::OuterShape( face2, TopAbs_VERTEX ));
    TopoDS_Vertex VV1[2] = { v1, v1 };
    TopoDS_Vertex VV2[2] = { v2, v2 };
    list< TopoDS_Edge > edges1, edges2;
    if ( int nbE = HERE::FindFaceAssociation( face1, VV1, face2, VV2, edges1, edges2 ))
    {
      HERE::InsertAssociation( face1, face2, theMap );
      fixAssocByPropagation( nbE, edges1, edges2, mesh1, mesh2 );
      list< TopoDS_Edge >::iterator eIt1 = edges1.begin();
      list< TopoDS_Edge >::iterator eIt2 = edges2.begin();
      for ( ; eIt1 != edges1.end(); ++eIt1, ++eIt2 )
      {
        HERE::InsertAssociation( *eIt1, *eIt2, theMap );
        v1 = SMESH_MesherHelper::IthVertex( 0, *eIt1 );
        v2 = SMESH_MesherHelper::IthVertex( 0, *eIt2 );
        HERE::InsertAssociation( v1, v2, theMap );
      }
      theMap.SetAssocType( HERE::TShapeShapeMap::FEW_EF );
      return true;
    }
    return false;
  }

  //================================================================================
  /*!
   * \brief Look for a group containing a target shape and similar to a source group
    * \param tgtShape - target edge or face
    * \param tgtMesh1 - target mesh
    * \param srcGroup - source group
    * \retval TopoDS_Shape - found target group
   */
  //================================================================================

  TopoDS_Shape findGroupContaining(const TopoDS_Shape& tgtShape,
                                   const SMESH_Mesh*   tgtMesh1,
                                   const TopoDS_Shape& srcGroup)
  {
    list<SMESH_subMesh*> subMeshes = tgtMesh1->GetGroupSubMeshesContaining(tgtShape);
    list<SMESH_subMesh*>::iterator sm = subMeshes.begin();
    int type, last = TopAbs_SHAPE;
    for ( ; sm != subMeshes.end(); ++sm ) {
      const TopoDS_Shape & group = (*sm)->GetSubShape();
      // check if group is similar to srcGroup
      for ( type = srcGroup.ShapeType(); type < last; ++type)
        if ( SMESH_MesherHelper::Count( srcGroup, (TopAbs_ShapeEnum)type, 0) !=
             SMESH_MesherHelper::Count( group,    (TopAbs_ShapeEnum)type, 0))
          break;
      if ( type == last )
        return group;
    }
    return TopoDS_Shape();
  }

  //================================================================================
  /*!
   * \brief Find association of groups at top and bottom of prism
   */
  //================================================================================

  bool assocGroupsByPropagation(const TopoDS_Shape&   theGroup1,
                                const TopoDS_Shape&   theGroup2,
                                SMESH_Mesh&           theMesh,
                                HERE::TShapeShapeMap& theMap)
  {
    // If groups are on top and bottom of prism then we can associate
    // them using "vertical" (or "side") edges and faces of prism since
    // they connect corresponding vertices and edges of groups.

    TopTools_IndexedMapOfShape subshapes1, subshapes2;
    TopExp::MapShapes( theGroup1, subshapes1 );
    TopExp::MapShapes( theGroup2, subshapes2 );
    TopTools_ListIteratorOfListOfShape ancestIt;

    // Iterate on vertices of group1 to find corresponding vertices in group2
    // and associate adjacent edges and faces

    TopTools_MapOfShape verticShapes;
    TopExp_Explorer vExp1( theGroup1, TopAbs_VERTEX );
    for ( ; vExp1.More(); vExp1.Next() )
    {
      const TopoDS_Vertex& v1 = TopoDS::Vertex( vExp1.Current() );
      if ( theMap.IsBound( v1 )) continue; // already processed

      // Find "vertical" edge ending in v1 and whose other vertex belongs to group2
      TopoDS_Shape verticEdge, v2;
      ancestIt.Initialize( theMesh.GetAncestors( v1 ));
      for ( ; verticEdge.IsNull() && ancestIt.More(); ancestIt.Next() )
      {
        if ( ancestIt.Value().ShapeType() != TopAbs_EDGE ) continue;
        v2 = HERE::GetNextVertex( TopoDS::Edge( ancestIt.Value() ), v1 );
        if ( subshapes2.Contains( v2 ))
          verticEdge = ancestIt.Value();
      }
      if ( verticEdge.IsNull() )
        return false;

      HERE::InsertAssociation( v1, v2, theMap);

      // Associate edges by vertical faces sharing the found vertical edge
      ancestIt.Initialize( theMesh.GetAncestors( verticEdge ) );
      for ( ; ancestIt.More(); ancestIt.Next() )
      {
        if ( ancestIt.Value().ShapeType() != TopAbs_FACE ) continue;
        if ( !verticShapes.Add( ancestIt.Value() )) continue;
        const TopoDS_Face& face = TopoDS::Face( ancestIt.Value() );

        // get edges of the face
        TopoDS_Edge edgeGr1, edgeGr2, verticEdge2;
        list< TopoDS_Edge > edges;    list< int > nbEdgesInWire;
        SMESH_Block::GetOrderedEdges( face, edges, nbEdgesInWire, v1);
        if ( nbEdgesInWire.front() != 4 )
          return storeShapeForDebug( face );
        list< TopoDS_Edge >::iterator edge = edges.begin();
        if ( verticEdge.IsSame( *edge )) {
          edgeGr2     = *(++edge);
          verticEdge2 = *(++edge);
          edgeGr1     = *(++edge);
        } else {
          edgeGr1     = *(edge++);
          verticEdge2 = *(edge++);
          edgeGr2     = *(edge++);
        }

        HERE::InsertAssociation( edgeGr1, edgeGr2.Reversed(), theMap);
      }
    }

    // Associate faces
    TopoDS_Iterator gr1It( theGroup1 );
    if ( gr1It.Value().ShapeType() == TopAbs_FACE )
    {
      // find a boundary edge of group1 to start from
      TopoDS_Shape bndEdge = HERE::GetBoundaryEdge( theGroup1, theMesh );
      if ( bndEdge.IsNull() )
        return false;

      list< TopoDS_Shape > edges(1, bndEdge);
      list< TopoDS_Shape >::iterator edge1 = edges.begin();
      for ( ; edge1 != edges.end(); ++edge1 )
      {
        // there must be one or zero not associated faces between ancestors of edge
        // belonging to theGroup1
        TopoDS_Shape face1;
        ancestIt.Initialize( theMesh.GetAncestors( *edge1 ) );
        for ( ; ancestIt.More() && face1.IsNull(); ancestIt.Next() ) {
          if ( ancestIt.Value().ShapeType() == TopAbs_FACE &&
               !theMap.IsBound( ancestIt.Value() ) &&
               subshapes1.Contains( ancestIt.Value() ))
            face1 = ancestIt.Value();

          // add edges of face1 to start searching for adjacent faces from
          for ( TopExp_Explorer e(face1, TopAbs_EDGE); e.More(); e.Next())
            if ( !edge1->IsSame( e.Current() ))
              edges.push_back( e.Current() );
        }
        if ( !face1.IsNull() ) {
          // find the corresponding face of theGroup2
          TopoDS_Shape edge2 = theMap( *edge1 );
          TopoDS_Shape face2;
          ancestIt.Initialize( theMesh.GetAncestors( edge2 ) );
          for ( ; ancestIt.More() && face2.IsNull(); ancestIt.Next() ) {
            if ( ancestIt.Value().ShapeType() == TopAbs_FACE &&
                 !theMap.IsBound( ancestIt.Value(), /*is2nd=*/true ) &&
                 subshapes2.Contains( ancestIt.Value() ))
              face2 = ancestIt.Value();
          }
          if ( face2.IsNull() )
            return false;

          HERE::InsertAssociation( face1, face2, theMap);
        }
      }
    }
    theMap.SetAssocType( HERE::TShapeShapeMap::PROPAGATION );
    return true;
  }

  //================================================================================
  /*!
   * \brief Return true if uv position of the vIndex-th vertex of edge on face is close
   * enough to given uv 
   */
  //================================================================================

  bool sameVertexUV( const TopoDS_Edge& edge,
                     const TopoDS_Face& face,
                     const int&         vIndex,
                     const gp_Pnt2d&    uv,
                     const double&      tol2d )
  {
    TopoDS_Vertex VV[2];
    TopExp::Vertices( edge, VV[0], VV[1], true);
    gp_Pnt2d v1UV = BRep_Tool::Parameters( VV[vIndex], face);
    double dist2d = v1UV.Distance( uv );
    return dist2d < tol2d;
  }

  //================================================================================
  /*!
   * \brief Returns an EDGE suitable for search of initial vertex association
   */
  //================================================================================

  bool getOuterEdges( const TopoDS_Shape        shape,
                      SMESH_Mesh&               mesh,
                      std::list< TopoDS_Edge >& allBndEdges )
  {
    if ( shape.ShapeType() == TopAbs_COMPOUND )
    {
      TopoDS_Iterator it( shape );
      if ( it.More() && it.Value().ShapeType() == TopAbs_FACE ) // group of FACEs
      {
        // look for a boundary EDGE of a group
        StdMeshers_ProjectionUtils::GetBoundaryEdge( shape, mesh, &allBndEdges );
        if ( !allBndEdges.empty() )
          return true;
      }
    }
    SMESH_MesherHelper helper( mesh );
    helper.SetSubShape( shape );

    TopExp_Explorer expF( shape, TopAbs_FACE ), expE;
    if ( expF.More() ) {
      for ( ; expF.More(); expF.Next() ) {
        TopoDS_Shape wire =
          StdMeshers_ProjectionUtils::OuterShape( TopoDS::Face( expF.Current() ), TopAbs_WIRE );
        for ( expE.Init( wire, TopAbs_EDGE ); expE.More(); expE.Next() )
          if ( ! helper.IsClosedEdge( TopoDS::Edge( expE.Current() )))
          {
            if ( helper.IsSeamShape( expE.Current() ))
              allBndEdges.push_back( TopoDS::Edge( expE.Current() ));
            else
              allBndEdges.push_front( TopoDS::Edge( expE.Current() ));
          }
      }
    }
    else if ( shape.ShapeType() != TopAbs_EDGE) { // no faces
      for ( expE.Init( shape, TopAbs_EDGE ); expE.More(); expE.Next() )
        if ( ! helper.IsClosedEdge( TopoDS::Edge( expE.Current() )))
        {
          if ( helper.IsSeamShape( expE.Current() ))
            allBndEdges.push_back( TopoDS::Edge( expE.Current() ));
          else
            allBndEdges.push_front( TopoDS::Edge( expE.Current() ));
        }
    }
    else if ( shape.ShapeType() == TopAbs_EDGE ) {
      if ( ! helper.IsClosedEdge( TopoDS::Edge( shape )))
        allBndEdges.push_back( TopoDS::Edge( shape ));
    }
    return !allBndEdges.empty();
  }

} // namespace

//=======================================================================
/*
 * Looks for association of all sub-shapes of two shapes
 *  \param theShape1 - target shape
 *  \param theMesh1 - mesh built on shape 1
 *  \param theShape2 - source shape
 *  \param theMesh2 - mesh built on shape 2
 *  \param theAssociation - association map to be filled that may
 *                          contain association of one or two pairs of vertices
 *  \retval bool - true if association found
 */
//=======================================================================

bool StdMeshers_ProjectionUtils::FindSubShapeAssociation(const TopoDS_Shape& theShape1,
                                                         SMESH_Mesh*         theMesh1,
                                                         const TopoDS_Shape& theShape2,
                                                         SMESH_Mesh*         theMesh2,
                                                         TShapeShapeMap &    theMap)
{
  // Structure of this long function is following
  // 1) Group -> Group projection: theShape1 is a group member,
  //    theShape2 is another group. We find the group theShape1 is in and recall self.
  // 2) Accosiate same shapes with different location (partners).
  // 3) If vertex association is given, perform association according to shape type:
  //       switch ( ShapeType ) {
  //         case TopAbs_EDGE:
  //         case ...:
  //       }
  // 4) else try to accosiate in different ways:
  //       a) accosiate shapes by propagation and other simple cases
  //            switch ( ShapeType ) {
  //            case TopAbs_EDGE:
  //            case ...:
  //            }
  //       b) find association of a couple of vertices and recall self.
  //

  theMeshDS[0] = theMesh1->GetMeshDS(); // debug
  theMeshDS[1] = theMesh2->GetMeshDS();

  // =================================================================================
  // 1) Is it the case of associating a group member -> another group? (PAL16202, 16203)
  // =================================================================================
  if ( theShape1.ShapeType() != theShape2.ShapeType() )
  {
    TopoDS_Shape group1, group2;
    if ( theShape1.ShapeType() == TopAbs_COMPOUND ) {
      group1 = theShape1;
      group2 = findGroupContaining( theShape2, theMesh2, group1 );
    }
    else if ( theShape2.ShapeType() == TopAbs_COMPOUND ) {
      group2 = theShape2;
      group1 = findGroupContaining( theShape1, theMesh1, group2 );
    }
    if ( group1.IsNull() || group2.IsNull() )
      RETURN_BAD_RESULT("Different shape types");
    // Associate compounds
    return FindSubShapeAssociation(group1, theMesh1, group2, theMesh2, theMap );
  }

  // ============
  // 2) Is partner?
  // ============
  bool partner = theShape1.IsPartner( theShape2 );
  TopTools_DataMapIteratorOfDataMapOfShapeShape vvIt( theMap._map1to2 );
  for ( ; partner && vvIt.More(); vvIt.Next() )
    partner = vvIt.Key().IsPartner( vvIt.Value() );

  if ( partner ) // Same shape with different location
  {
    // recursively associate all sub-shapes of theShape1 and theShape2
    typedef list< pair< TopoDS_Shape, TopoDS_Shape > > TShapePairsList;
    TShapePairsList shapesQueue( 1, make_pair( theShape1, theShape2 ));
    TShapePairsList::iterator s1_s2 = shapesQueue.begin();
    for ( ; s1_s2 != shapesQueue.end(); ++s1_s2 )
    {
      if ( theMap.IsBound( s1_s2->first )) // avoid re-binding for a seam edge
        continue; // to avoid this:           Forward seam -> Reversed seam
      InsertAssociation( s1_s2->first, s1_s2->second, theMap );
      TopoDS_Iterator s1It( s1_s2->first), s2It( s1_s2->second );
      for ( ; s1It.More(); s1It.Next(), s2It.Next() )
        shapesQueue.push_back( make_pair( s1It.Value(), s2It.Value() ));
    }
    theMap.SetAssocType( TShapeShapeMap::PARTNER );
    return true;
  }

  if ( !theMap.IsEmpty() )
  {
    //======================================================================
    // 3) HAS initial vertex association
    //======================================================================
    bool isVCloseness = ( theMap._assocType == TShapeShapeMap::CLOSE_VERTEX );
    theMap.SetAssocType( TShapeShapeMap::INIT_VERTEX );
    switch ( theShape1.ShapeType() ) {
      // ----------------------------------------------------------------------
    case TopAbs_EDGE: { // TopAbs_EDGE
      // ----------------------------------------------------------------------
      if ( theMap.Extent() != 1 )
        RETURN_BAD_RESULT("Wrong map extent " << theMap.Extent() );
      TopoDS_Edge edge1 = TopoDS::Edge( theShape1 );
      TopoDS_Edge edge2 = TopoDS::Edge( theShape2 );
      if ( edge1.Orientation() >= TopAbs_INTERNAL ) edge1.Orientation( TopAbs_FORWARD );
      if ( edge2.Orientation() >= TopAbs_INTERNAL ) edge2.Orientation( TopAbs_FORWARD );
      TopoDS_Vertex VV1[2], VV2[2];
      TopExp::Vertices( edge1, VV1[0], VV1[1] );
      TopExp::Vertices( edge2, VV2[0], VV2[1] );
      int i1 = 0, i2 = 0;
      if ( theMap.IsBound( VV1[ i1 ] )) i1 = 1;
      if ( theMap.IsBound( VV2[ i2 ] )) i2 = 1;
      InsertAssociation( VV1[ i1 ], VV2[ i2 ], theMap );
      InsertAssociation( theShape1, theShape2, theMap );
      return true;
    }
      // ----------------------------------------------------------------------
    case TopAbs_FACE: { // TopAbs_FACE
      // ----------------------------------------------------------------------
      TopoDS_Face face1 = TopoDS::Face( theShape1 );
      TopoDS_Face face2 = TopoDS::Face( theShape2 );
      if ( face1.Orientation() >= TopAbs_INTERNAL ) face1.Orientation( TopAbs_FORWARD );
      if ( face2.Orientation() >= TopAbs_INTERNAL ) face2.Orientation( TopAbs_FORWARD );

      TopoDS_Vertex VV1[2], VV2[2];
      // find a not closed edge of face1 both vertices of which are associated
      int nbEdges = 0;
      TopExp_Explorer exp ( face1, TopAbs_EDGE );
      for ( ; VV2[ 1 ].IsNull() && exp.More(); exp.Next(), ++nbEdges ) {
        TopExp::Vertices( TopoDS::Edge( exp.Current() ), VV1[0], VV1[1] );
        if ( theMap.IsBound( VV1[0] ) ) {
          VV2[ 0 ] = TopoDS::Vertex( theMap( VV1[0] ));
          if ( theMap.IsBound( VV1[1] ) && !VV1[0].IsSame( VV1[1] ))
            VV2[ 1 ] = TopoDS::Vertex( theMap( VV1[1] ));
        }
      }
      if ( VV2[ 1 ].IsNull() ) { // 2 bound vertices not found
        if ( nbEdges > 1 ) {
          RETURN_BAD_RESULT("2 bound vertices not found" );
        } else {
          VV2[ 1 ] = VV2[ 0 ];
        }
      }
      list< TopoDS_Edge > edges1, edges2;
      int nbE = FindFaceAssociation( face1, VV1, face2, VV2, edges1, edges2, isVCloseness );
      if ( !nbE ) RETURN_BAD_RESULT("FindFaceAssociation() failed");
      fixAssocByPropagation( nbE, edges1, edges2, theMesh1, theMesh2 );

      list< TopoDS_Edge >::iterator eIt1 = edges1.begin();
      list< TopoDS_Edge >::iterator eIt2 = edges2.begin();
      for ( ; eIt1 != edges1.end(); ++eIt1, ++eIt2 )
      {
        InsertAssociation( *eIt1, *eIt2, theMap );
        VV1[0] = TopExp::FirstVertex( *eIt1, true );
        VV2[0] = TopExp::FirstVertex( *eIt2, true );
        InsertAssociation( VV1[0], VV2[0], theMap );
      }
      InsertAssociation( theShape1, theShape2, theMap );
      return true;
    }
      // ----------------------------------------------------------------------
    case TopAbs_SHELL: // TopAbs_SHELL, TopAbs_SOLID
    case TopAbs_SOLID: {
      // ----------------------------------------------------------------------
      TopoDS_Vertex VV1[2], VV2[2];
      // try to find a not closed edge of shape1 both vertices of which are associated
      TopoDS_Edge edge1;
      TopExp_Explorer exp ( theShape1, TopAbs_EDGE );
      for ( ; VV2[ 1 ].IsNull() && exp.More(); exp.Next() ) {
        edge1 = TopoDS::Edge( exp.Current() );
        if ( edge1.Orientation() >= TopAbs_INTERNAL ) edge1.Orientation( TopAbs_FORWARD );
        TopExp::Vertices( edge1 , VV1[0], VV1[1] );
        if ( theMap.IsBound( VV1[0] )) {
          VV2[ 0 ] = TopoDS::Vertex( theMap( VV1[0] ));
          if ( theMap.IsBound( VV1[1] ) && !VV1[0].IsSame( VV1[1] ))
            VV2[ 1 ] = TopoDS::Vertex( theMap( VV1[1] ));
        }
      }
      if ( VV2[ 1 ].IsNull() ) // 2 bound vertices not found
        RETURN_BAD_RESULT("2 bound vertices not found" );
      // get an edge2 of theShape2 corresponding to edge1
      TopoDS_Edge edge2 = GetEdgeByVertices( theMesh2, VV2[ 0 ], VV2[ 1 ]);
      if ( edge2.IsNull() )
        RETURN_BAD_RESULT("GetEdgeByVertices() failed");

      // build map of edge to faces if shapes are not sub-shapes of main ones
      bool isSubOfMain = false;
      if ( SMESHDS_SubMesh * sm = theMesh1->GetMeshDS()->MeshElements( theShape1 ))
        isSubOfMain = !sm->IsComplexSubmesh();
      else
        isSubOfMain = theMesh1->GetMeshDS()->ShapeToIndex( theShape1 );
      TAncestorMap e2f1, e2f2;
      const TAncestorMap& edgeToFace1 = isSubOfMain ? theMesh1->GetAncestorMap() : e2f1;
      const TAncestorMap& edgeToFace2 = isSubOfMain ? theMesh2->GetAncestorMap() : e2f2;
      if (!isSubOfMain) {
        TopExp::MapShapesAndAncestors( theShape1, TopAbs_EDGE, TopAbs_FACE, e2f1 );
        TopExp::MapShapesAndAncestors( theShape2, TopAbs_EDGE, TopAbs_FACE, e2f2 );
        if ( !edgeToFace1.Contains( edge1 ))
          RETURN_BAD_RESULT("edge1 does not belong to theShape1");
        if ( !edgeToFace2.Contains( edge2 ))
          RETURN_BAD_RESULT("edge2 does not belong to theShape2");
      }
      //
      // Look for 2 corresponing faces:
      //
      TopoDS_Shape F1, F2;

      // get a face sharing edge1 (F1)
      TopTools_ListIteratorOfListOfShape ancestIt1( edgeToFace1.FindFromKey( edge1 ));
      for ( ; F1.IsNull() && ancestIt1.More(); ancestIt1.Next() )
        if ( ancestIt1.Value().ShapeType() == TopAbs_FACE )
          F1 = ancestIt1.Value().Oriented //( TopAbs_FORWARD );
            ( SMESH_MesherHelper::GetSubShapeOri( theShape1, ancestIt1.Value() ));
      if ( F1.IsNull() )
        RETURN_BAD_RESULT(" Face1 not found");

      // get 2 faces sharing edge2 (one of them is F2)
      TopoDS_Shape FF2[2];
      TopTools_ListIteratorOfListOfShape ancestIt2( edgeToFace2.FindFromKey( edge2 ));
      for ( int i = 0; FF2[1].IsNull() && ancestIt2.More(); ancestIt2.Next() )
        if ( ancestIt2.Value().ShapeType() == TopAbs_FACE )
          FF2[ i++ ] = ancestIt2.Value().Oriented // ( TopAbs_FORWARD );
            ( SMESH_MesherHelper::GetSubShapeOri( theShape2, ancestIt2.Value() ));

      // get oriented edge1 and edge2 from F1 and FF2[0]
      for ( exp.Init( F1, TopAbs_EDGE ); exp.More(); exp.Next() )
        if ( edge1.IsSame( exp.Current() )) {
          edge1 = TopoDS::Edge( exp.Current() );
          break;
        }
      for ( exp.Init( FF2[ 0 ], TopAbs_EDGE ); exp.More(); exp.Next() )
        if ( edge2.IsSame( exp.Current() )) {
          edge2 = TopoDS::Edge( exp.Current() );
          break;
        }

      // compare first vertices of edge1 and edge2
      TopExp::Vertices( edge1, VV1[0], VV1[1], true );
      TopExp::Vertices( edge2, VV2[0], VV2[1], true );
      F2 = FF2[ 0 ]; // (F2 !)
      if ( !VV1[ 0 ].IsSame( theMap( VV2[ 0 ], /*is2=*/true))) {
        edge2.Reverse();
        if ( FF2[ 1 ].IsNull() )
          F2.Reverse();
        else
          F2 = FF2[ 1 ];
      }

      // association of face sub-shapes and neighbour faces
      list< pair < TopoDS_Face, TopoDS_Edge > > FE1, FE2;
      list< pair < TopoDS_Face, TopoDS_Edge > >::iterator fe1, fe2;
      FE1.push_back( make_pair( TopoDS::Face( F1 ), edge1 ));
      FE2.push_back( make_pair( TopoDS::Face( F2 ), edge2 ));
      for ( fe1 = FE1.begin(), fe2 = FE2.begin(); fe1 != FE1.end(); ++fe1, ++fe2 )
      {
        const TopoDS_Face& face1 = fe1->first;
        if ( theMap.IsBound( face1 ) ) continue;
        const TopoDS_Face& face2 = fe2->first;
        edge1 = fe1->second;
        edge2 = fe2->second;
        TopExp::Vertices( edge1, VV1[0], VV1[1], true );
        TopExp::Vertices( edge2, VV2[0], VV2[1], true );
        list< TopoDS_Edge > edges1, edges2;
        int nbE = FindFaceAssociation( face1, VV1, face2, VV2, edges1, edges2, isVCloseness );
        if ( !nbE ) RETURN_BAD_RESULT("FindFaceAssociation() failed");
        InsertAssociation( face1, face2, theMap ); // assoc faces
        MESSAGE("Assoc FACE " << theMesh1->GetMeshDS()->ShapeToIndex( face1 )<<
                " to "        << theMesh2->GetMeshDS()->ShapeToIndex( face2 ));
        if ( nbE == 2 && (edge1.IsSame( edges1.front())) != (edge2.IsSame( edges2.front())))
        {
          reverseEdges( edges2, nbE );
        }
        list< TopoDS_Edge >::iterator eIt1 = edges1.begin();
        list< TopoDS_Edge >::iterator eIt2 = edges2.begin();
        for ( ; eIt1 != edges1.end(); ++eIt1, ++eIt2 )
        {
          if ( !InsertAssociation( *eIt1, *eIt2, theMap ))  // assoc edges
            continue; // already associated
          VV1[0] = TopExp::FirstVertex( *eIt1, true );
          VV2[0] = TopExp::FirstVertex( *eIt2, true );
          InsertAssociation( VV1[0], VV2[0], theMap ); // assoc vertices

          // add adjacent faces to process
          TopoDS_Face nextFace1 = GetNextFace( edgeToFace1, *eIt1, face1 );
          TopoDS_Face nextFace2 = GetNextFace( edgeToFace2, *eIt2, face2 );
          if ( !nextFace1.IsNull() && !nextFace2.IsNull() ) {
            if ( SMESH_MesherHelper::GetSubShapeOri( nextFace1, *eIt1 ) == eIt1->Orientation() )
              nextFace1.Reverse();
            if ( SMESH_MesherHelper::GetSubShapeOri( nextFace2, *eIt2 ) == eIt2->Orientation() )
              nextFace2.Reverse();
            FE1.push_back( make_pair( nextFace1, *eIt1 ));
            FE2.push_back( make_pair( nextFace2, *eIt2 ));
          }
        }
      }
      InsertAssociation( theShape1, theShape2, theMap );
      return true;
    }
      // ----------------------------------------------------------------------
    case TopAbs_COMPOUND: { // GROUP
      // ----------------------------------------------------------------------
      // Maybe groups contain only one member
      TopoDS_Iterator it1( theShape1 ), it2( theShape2 );
      TopAbs_ShapeEnum memberType = it1.Value().ShapeType();
      int nbMembers = SMESH_MesherHelper::Count( theShape1, memberType, true );
      if ( nbMembers == 0 ) return true;
      if ( nbMembers == 1 ) {
        return FindSubShapeAssociation( it1.Value(), theMesh1, it2.Value(), theMesh2, theMap );
      }
      // Try to make shells of faces
      //
      BRep_Builder builder;
      TopoDS_Shell shell1, shell2;
      builder.MakeShell(shell1); builder.MakeShell(shell2);
      if ( memberType == TopAbs_FACE ) {
        // just add faces of groups to shells
        for (; it1.More(); it1.Next(), it2.Next() )
          builder.Add( shell1, it1.Value() ), builder.Add( shell2, it2.Value() );
      }
      else if ( memberType == TopAbs_EDGE ) {
        // Try to add faces sharing more than one edge of a group or
        // sharing all its vertices with the group
        TopTools_IndexedMapOfShape groupVertices[2];
        TopExp::MapShapes( theShape1, TopAbs_VERTEX, groupVertices[0]);
        TopExp::MapShapes( theShape2, TopAbs_VERTEX, groupVertices[1]);
        //
        TopTools_MapOfShape groupEdges[2], addedFaces[2];
        bool hasInitAssoc = (!theMap.IsEmpty()), initAssocOK = !hasInitAssoc;
        for (; it1.More(); it1.Next(), it2.Next() ) {
          groupEdges[0].Add( it1.Value() );
          groupEdges[1].Add( it2.Value() );
          if ( !initAssocOK ) {
            // for shell association there must be an edge with both vertices bound
            TopoDS_Vertex v1, v2;
            TopExp::Vertices( TopoDS::Edge( it1.Value().Oriented(TopAbs_FORWARD)), v1, v2 );
            initAssocOK = ( theMap.IsBound( v1 ) && theMap.IsBound( v2 ));
          }
        }
        for (int is2ndGroup = 0; initAssocOK && is2ndGroup < 2; ++is2ndGroup) {
          const TopoDS_Shape& group = is2ndGroup ? theShape2: theShape1;
          SMESH_Mesh*         mesh  = is2ndGroup ? theMesh2 : theMesh1;
          TopoDS_Shell&       shell = is2ndGroup ? shell2   : shell1;
          for ( TopoDS_Iterator it( group ); it.More(); it.Next() ) {
            const TopoDS_Edge& edge = TopoDS::Edge( it.Value() );
            TopoDS_Face face;
            for ( int iF = 0; iF < 2; ++iF ) { // loop on 2 faces sharing edge
              face = GetNextFace(mesh->GetAncestorMap(), edge, face);
              if ( !face.IsNull() ) {
                int nbGroupEdges = 0;
                for ( TopExp_Explorer f( face, TopAbs_EDGE ); f.More(); f.Next())
                  if ( groupEdges[ is2ndGroup ].Contains( f.Current() ))
                    if ( ++nbGroupEdges > 1 )
                      break;
                bool add = (nbGroupEdges > 1 ||
                            SMESH_MesherHelper::Count( face, TopAbs_EDGE, true ) == 1 );
                if ( !add ) {
                  add = true;
                  for ( TopExp_Explorer v( face, TopAbs_VERTEX ); add && v.More(); v.Next())
                    add = groupVertices[ is2ndGroup ].Contains( v.Current() );
                }
                if ( add && addedFaces[ is2ndGroup ].Add( face ))
                  builder.Add( shell, face );
              }
            }
          }
        }
      } else {
        RETURN_BAD_RESULT("Unexpected group type");
      }
      // Associate shells
      //
      int nbFaces1 = SMESH_MesherHelper:: Count( shell1, TopAbs_FACE, 0 );
      int nbFaces2 = SMESH_MesherHelper:: Count( shell2, TopAbs_FACE, 0 );
      if ( nbFaces1 != nbFaces2 )
        RETURN_BAD_RESULT("Different nb of faces found for shells");
      if ( nbFaces1 > 0 ) {
        bool ok = false;
        if ( nbFaces1 == 1 ) {
          TopoDS_Shape F1 = TopoDS_Iterator( shell1 ).Value();
          TopoDS_Shape F2 = TopoDS_Iterator( shell2 ).Value();
          ok = FindSubShapeAssociation( F1, theMesh1, F2, theMesh2, theMap );
        }
        else {
          ok = FindSubShapeAssociation(shell1, theMesh1, shell2, theMesh2, theMap );
        }
        // Check if all members are mapped 
        if ( ok ) {
          TopTools_MapOfShape boundMembers[2];
          TopoDS_Iterator mIt;
          for ( mIt.Initialize( theShape1 ); mIt.More(); mIt.Next())
            if ( theMap.IsBound( mIt.Value() )) {
              boundMembers[0].Add( mIt.Value() );
              boundMembers[1].Add( theMap( mIt.Value() ));
            }
          if ( boundMembers[0].Extent() != nbMembers ) {
            // make compounds of not bound members
            TopoDS_Compound comp[2];
            for ( int is2ndGroup = 0; is2ndGroup < 2; ++is2ndGroup ) {
              builder.MakeCompound( comp[is2ndGroup] );
              for ( mIt.Initialize( is2ndGroup ? theShape2:theShape1 ); mIt.More(); mIt.Next())
                if ( ! boundMembers[ is2ndGroup ].Contains( mIt.Value() ))
                  builder.Add( comp[ is2ndGroup ], mIt.Value() );
            }
            // check if theMap contains initial association for the comp's
            bool hasInitialAssoc = false;
            if ( memberType == TopAbs_EDGE ) {
              for ( TopExp_Explorer v( comp[0], TopAbs_VERTEX ); v.More(); v.Next())
                if ( theMap.IsBound( v.Current() )) {
                  hasInitialAssoc = true;
                  break;
                }
            }
            if ( hasInitialAssoc == bool( !theMap.IsEmpty() ))
              ok = FindSubShapeAssociation( comp[0], theMesh1, comp[1], theMesh2, theMap );
            else {
              TShapeShapeMap tmpMap;
              ok = FindSubShapeAssociation( comp[0], theMesh1, comp[1], theMesh2, tmpMap );
              if ( ok ) {
                TopTools_DataMapIteratorOfDataMapOfShapeShape mapIt( tmpMap._map1to2 );
                for ( ; mapIt.More(); mapIt.Next() )
                  theMap.Bind( mapIt.Key(), mapIt.Value());
              }
            }
          }
        }
        return ok;
      }
      // Each edge of an edge group is shared by own faces
      // ------------------------------------------------------------------
      //
      // map vertices to edges sharing them, avoid doubling edges in lists
      TopTools_DataMapOfShapeListOfShape v2e[2];
      for (int isFirst = 0; isFirst < 2; ++isFirst ) {
        const TopoDS_Shape& group = isFirst ? theShape1 : theShape2;
        TopTools_DataMapOfShapeListOfShape& veMap = v2e[ isFirst ? 0 : 1 ];
        TopTools_MapOfShape addedEdges;
        for ( TopExp_Explorer e( group, TopAbs_EDGE ); e.More(); e.Next() ) {
          const TopoDS_Shape& edge = e.Current();
          if ( addedEdges.Add( edge )) {
            for ( TopExp_Explorer v( edge, TopAbs_VERTEX ); v.More(); v.Next()) {
              const TopoDS_Shape& vertex = v.Current();
              if ( !veMap.IsBound( vertex )) {
                TopTools_ListOfShape l;
                veMap.Bind( vertex, l );
              }
              veMap( vertex ).Append( edge );
            }
          }
        }   
      }
      while ( !v2e[0].IsEmpty() )
      {
        // find a bound vertex
        TopoDS_Vertex V[2];
        TopTools_DataMapIteratorOfDataMapOfShapeListOfShape v2eIt( v2e[0] );
        for ( ; v2eIt.More(); v2eIt.Next())
          if ( theMap.IsBound( v2eIt.Key() )) {
            V[0] = TopoDS::Vertex( v2eIt.Key() );
            V[1] = TopoDS::Vertex( theMap( V[0] ));
            break;
          }
        if ( V[0].IsNull() )
          RETURN_BAD_RESULT("No more bound vertices");

        while ( !V[0].IsNull() && v2e[0].IsBound( V[0] )) {
          TopTools_ListOfShape& edges0 = v2e[0]( V[0] );
          TopTools_ListOfShape& edges1 = v2e[1]( V[1] );
          int nbE0 = edges0.Extent(), nbE1 = edges1.Extent();
          if ( nbE0 != nbE1 )
            RETURN_BAD_RESULT("Different nb of edges: "<< nbE0 << " != " << nbE1);

          if ( nbE0 == 1 )
          {
            TopoDS_Edge e0 = TopoDS::Edge( edges0.First() );
            TopoDS_Edge e1 = TopoDS::Edge( edges1.First() );
            v2e[0].UnBind( V[0] );
            v2e[1].UnBind( V[1] );
            InsertAssociation( e0, e1, theMap );
            MESSAGE("Assoc edge " << theMesh1->GetMeshDS()->ShapeToIndex( e0 )<<
                    " to "        << theMesh2->GetMeshDS()->ShapeToIndex( e1 ));
            V[0] = GetNextVertex( e0, V[0] );
            V[1] = GetNextVertex( e1, V[1] );
            if ( !V[0].IsNull() ) {
              InsertAssociation( V[0], V[1], theMap );
              MESSAGE("Assoc vertex " << theMesh1->GetMeshDS()->ShapeToIndex( V[0] )<<
                      " to "          << theMesh2->GetMeshDS()->ShapeToIndex( V[1] ));
            }
          }
          else if ( nbE0 == 2 )
          {
            // one of edges must have both ends bound
            TopoDS_Vertex v0e0 = GetNextVertex( TopoDS::Edge( edges0.First() ), V[0] );
            TopoDS_Vertex v1e0 = GetNextVertex( TopoDS::Edge( edges0.Last() ),  V[0] );
            TopoDS_Vertex v0e1 = GetNextVertex( TopoDS::Edge( edges1.First() ), V[1] );
            TopoDS_Vertex v1e1 = GetNextVertex( TopoDS::Edge( edges1.Last() ),  V[1] );
            TopoDS_Shape e0b, e1b, e0n, e1n, v1b; // bound and not-bound
            TopoDS_Vertex v0n, v1n;
            if ( theMap.IsBound( v0e0 )) {
              v0n = v1e0; e0b = edges0.First(); e0n = edges0.Last(); v1b = theMap( v0e0 );
            } else if ( theMap.IsBound( v1e0 )) {
              v0n = v0e0; e0n = edges0.First(); e0b = edges0.Last(); v1b = theMap( v1e0 );
            } else {
              RETURN_BAD_RESULT("None of vertices bound");
            }
            if ( v1b.IsSame( v1e1 )) {
              v1n = v0e1; e1n = edges1.First(); e1b = edges1.Last();
            } else {
              v1n = v1e1; e1b = edges1.First(); e1n = edges1.Last();
            }
            InsertAssociation( e0b, e1b, theMap );
            InsertAssociation( e0n, e1n, theMap );
            InsertAssociation( v0n, v1n, theMap );
            MESSAGE("Assoc edge " << theMesh1->GetMeshDS()->ShapeToIndex( e0b )<<
                    " to "        << theMesh2->GetMeshDS()->ShapeToIndex( e1b ));
            MESSAGE("Assoc edge " << theMesh1->GetMeshDS()->ShapeToIndex( e0n )<<
                    " to "        << theMesh2->GetMeshDS()->ShapeToIndex( e1n ));
            MESSAGE("Assoc vertex " << theMesh1->GetMeshDS()->ShapeToIndex( v0n )<<
                    " to "          << theMesh2->GetMeshDS()->ShapeToIndex( v1n ));
            v2e[0].UnBind( V[0] );
            v2e[1].UnBind( V[1] );
            V[0] = v0n;
            V[1] = v1n;
          }
          else {
            RETURN_BAD_RESULT("Not implemented");
          }
        }
      } //while ( !v2e[0].IsEmpty() )
      return true;
    }

    default:
      RETURN_BAD_RESULT("Unexpected shape type");

    } // end switch by shape type
  } // end case of available initial vertex association

  //======================================================================
  // 4) NO INITIAL VERTEX ASSOCIATION
  //======================================================================

  switch ( theShape1.ShapeType() ) {

  case TopAbs_EDGE: {
    // ----------------------------------------------------------------------
    TopoDS_Edge edge1 = TopoDS::Edge( theShape1 );
    TopoDS_Edge edge2 = TopoDS::Edge( theShape2 );
    if ( isPropagationPossible( theMesh1, theMesh2 ))
    {
      TopoDS_Edge prpEdge = GetPropagationEdge( theMesh1, edge2, edge1 ).second;
      if ( !prpEdge.IsNull() )
      {
        TopoDS_Vertex VV1[2], VV2[2];
        TopExp::Vertices( edge1,   VV1[0], VV1[1], true );
        TopExp::Vertices( prpEdge, VV2[0], VV2[1], true );
        InsertAssociation( VV1[ 0 ], VV2[ 0 ], theMap );
        InsertAssociation( VV1[ 1 ], VV2[ 1 ], theMap );
        if ( VV1[0].IsSame( VV1[1] ) || // one of edges is closed
             VV2[0].IsSame( VV2[1] ) )
        {
          InsertAssociation( edge1, prpEdge, theMap ); // insert with a proper orientation
        }
        InsertAssociation( theShape1, theShape2, theMap );
        theMap.SetAssocType( TShapeShapeMap::PROPAGATION );
        return true; // done
      }
    }
    if ( SMESH_MesherHelper::IsClosedEdge( edge1 ) &&
         SMESH_MesherHelper::IsClosedEdge( edge2 ))
    {
      // TODO: find out a proper orientation (is it possible?)
      InsertAssociation( edge1, edge2, theMap ); // insert with a proper orientation
      InsertAssociation( TopExp::FirstVertex(edge1), TopExp::FirstVertex(edge2),
                         theMap );
      InsertAssociation( theShape1, theShape2, theMap );
      return true; // done
    }
    break; // try by vertex closeness
  }

  case TopAbs_FACE: {
    // ----------------------------------------------------------------------
    if ( isPropagationPossible( theMesh1, theMesh2 )) // try by propagation in one mesh
    {
      TopoDS_Face face1 = TopoDS::Face(theShape1);
      TopoDS_Face face2 = TopoDS::Face(theShape2);
      if ( face1.Orientation() >= TopAbs_INTERNAL ) face1.Orientation( TopAbs_FORWARD );
      if ( face2.Orientation() >= TopAbs_INTERNAL ) face2.Orientation( TopAbs_FORWARD );
      TopoDS_Edge edge1, edge2;
      // get outer edge of theShape1
      TopoDS_Shape wire = OuterShape( face1, TopAbs_WIRE );
      //edge1 = TopoDS::Edge( OuterShape( face1, TopAbs_EDGE ));
      // use map to find the closest propagation edge
      map<int, pair< TopoDS_Edge, TopoDS_Edge > > propag_edges;
      for ( TopoDS_Iterator edgeIt( wire ); edgeIt.More(); edgeIt.Next() )
      {
        edge1 = TopoDS::Edge( edgeIt.Value() );
        // find out if any edge of face2 is a propagation edge of outer edge1
        for ( TopExp_Explorer exp( face2, TopAbs_EDGE ); exp.More(); exp.Next() ) {
          edge2 = TopoDS::Edge( exp.Current() );
          pair<int,TopoDS_Edge> step_edge = GetPropagationEdge( theMesh1, edge2, edge1 );
          if ( !step_edge.second.IsNull() ) { // propagation found
            propag_edges.insert( make_pair( step_edge.first,
                                            ( make_pair( edge1, step_edge.second ))));
            if ( step_edge.first == 1 ) break; // most close found
          }
        }
        if ( !propag_edges.empty() && propag_edges.begin()->first == 1 ) break;
      }
      if ( !propag_edges.empty() ) // propagation found
      {
        edge1 = propag_edges.begin()->second.first;
        edge2 = propag_edges.begin()->second.second;
        TopoDS_Vertex VV1[2], VV2[2];
        TopExp::Vertices( edge1, VV1[0], VV1[1], true );
        TopExp::Vertices( edge2, VV2[0], VV2[1], true );
        list< TopoDS_Edge > edges1, edges2;
        int nbE = FindFaceAssociation( face1, VV1, face2, VV2, edges1, edges2 );
        if ( !nbE ) RETURN_BAD_RESULT("FindFaceAssociation() failed");
        // take care of proper association of propagated edges
        bool same1 = edge1.IsSame( edges1.front() );
        bool same2 = edge2.IsSame( edges2.front() );
        if ( same1 != same2 )
        {
          reverseEdges(edges2, nbE);
          if ( nbE != 2 ) // 2 degen edges of 4 (issue 0021144)
            edges2.splice( edges2.end(), edges2, edges2.begin());
        }
        // store association
        list< TopoDS_Edge >::iterator eIt1 = edges1.begin();
        list< TopoDS_Edge >::iterator eIt2 = edges2.begin();
        for ( ; eIt1 != edges1.end(); ++eIt1, ++eIt2 )
        {
          InsertAssociation( *eIt1, *eIt2, theMap );
          VV1[0] = TopExp::FirstVertex( *eIt1, true );
          VV2[0] = TopExp::FirstVertex( *eIt2, true );
          InsertAssociation( VV1[0], VV2[0], theMap );
        }
        InsertAssociation( theShape1, theShape2, theMap );
        theMap.SetAssocType( TShapeShapeMap::PROPAGATION );
        return true;
      }
    }
    break; // try by vertex closeness
  }
  case TopAbs_COMPOUND: {
    // ----------------------------------------------------------------------
    if ( isPropagationPossible( theMesh1, theMesh2 )) {

      // try to accosiate all using propagation
      if ( assocGroupsByPropagation( theShape1, theShape2, *theMesh1, theMap ))
        return true;

      // find a boundary edge of theShape1
      TopoDS_Edge E = GetBoundaryEdge( theShape1, *theMesh1 );
      if ( E.IsNull() )
        break; // try by vertex closeness

      // find association for vertices of edge E
      TopoDS_Vertex VV1[2], VV2[2];
      for(TopExp_Explorer eexp(E, TopAbs_VERTEX); eexp.More(); eexp.Next()) {
        TopoDS_Vertex V1 = TopoDS::Vertex( eexp.Current() );
        // look for an edge ending in E whose one vertex is in theShape1
        // and the other, in theShape2
        const TopTools_ListOfShape& Ancestors = theMesh1->GetAncestors(V1);
        TopTools_ListIteratorOfListOfShape ita(Ancestors);
        for(; ita.More(); ita.Next()) {
          if( ita.Value().ShapeType() != TopAbs_EDGE ) continue;
          TopoDS_Edge edge = TopoDS::Edge(ita.Value());
          bool FromShape1 = false;
          for(TopExp_Explorer expe(theShape1, TopAbs_EDGE); expe.More(); expe.Next() ) {
            if(edge.IsSame(expe.Current())) {
              FromShape1 = true;
              break;
            }
          }
          if(!FromShape1) {
            // is it an edge between theShape1 and theShape2?
            TopExp_Explorer expv(edge, TopAbs_VERTEX);
            TopoDS_Vertex V2 = TopoDS::Vertex( expv.Current() );
            if(V2.IsSame(V1)) {
              expv.Next();
              V2 = TopoDS::Vertex( expv.Current() );
            }
            bool FromShape2 = false;
            for ( expv.Init( theShape2, TopAbs_VERTEX ); expv.More(); expv.Next()) {
              if ( V2.IsSame( expv.Current() )) {
                FromShape2 = true;
                break;
              }
            }
            if ( FromShape2 ) {
              if ( VV1[0].IsNull() )
                VV1[0] = V1, VV2[0] = V2;
              else
                VV1[1] = V1, VV2[1] = V2;
              break; // from loop on ancestors of V1
            }
          }
        }
      }
      if ( !VV1[1].IsNull() ) {
        InsertAssociation( VV1[0], VV2[0], theMap );
        InsertAssociation( VV1[1], VV2[1], theMap );
        TShapeShapeMap::EAssocType asType = theMap._assocType;
        theMap.SetAssocType( TShapeShapeMap::PROPAGATION );
        if ( FindSubShapeAssociation( theShape1, theMesh1, theShape2, theMesh2, theMap ))
          return true;
        theMap._assocType = asType;
      }
    }
    break; // try by vertex closeness
  }
  default:;
  }

  // 4.b) Find association by closeness of vertices
  // ----------------------------------------------

  TopTools_IndexedMapOfShape vMap1, vMap2;
  TopExp::MapShapes( theShape1, TopAbs_VERTEX, vMap1 );
  TopExp::MapShapes( theShape2, TopAbs_VERTEX, vMap2 );
  TopoDS_Vertex VV1[2], VV2[2];

  if ( vMap1.Extent() != vMap2.Extent() )
  {
    if ( SMESH_MesherHelper:: Count( theShape1, TopAbs_EDGE, /*ignoreSame=*/false ) !=
         SMESH_MesherHelper:: Count( theShape2, TopAbs_EDGE, /*ignoreSame=*/false ))
      RETURN_BAD_RESULT("Different nb of vertices");
  }

  if ( vMap1.Extent() == 1 || vMap2.Extent() == 1 ) {
    InsertAssociation( vMap1(1), vMap2(1), theMap );
    if ( theShape1.ShapeType() == TopAbs_EDGE ) {
      if ( vMap1.Extent() == 2 )
        InsertAssociation( vMap1(2), vMap2(1), theMap );
      else if ( vMap2.Extent() == 2 )
        InsertAssociation( vMap2(2), vMap1(1), theMap );
      InsertAssociation( theShape1, theShape2, theMap );
      return true;
    }
    return FindSubShapeAssociation( theShape1, theMesh1, theShape2, theMesh2, theMap);
  }

  // Try to associate by common vertices of an edge
  for ( int i = 1; i <= vMap1.Extent(); ++i )
  {
    const TopoDS_Shape& v1 = vMap1(i);
    if ( vMap2.Contains( v1 ))
    {
      // find an egde sharing v1 and sharing at the same time another common vertex
      PShapeIteratorPtr edgeIt = SMESH_MesherHelper::GetAncestors( v1, *theMesh1, TopAbs_EDGE);
      bool edgeFound = false;
      while ( edgeIt->more() && !edgeFound )
      {
        TopoDS_Edge edge = TopoDS::Edge( edgeIt->next()->Oriented(TopAbs_FORWARD));
        TopExp::Vertices(edge, VV1[0], VV1[1]);
        if ( !VV1[0].IsSame( VV1[1] ))
          edgeFound = ( vMap2.Contains( VV1[ v1.IsSame(VV1[0]) ? 1:0]));
      }
      if ( edgeFound )
      {
        InsertAssociation( VV1[0], VV1[0], theMap );
        InsertAssociation( VV1[1], VV1[1], theMap );
        TShapeShapeMap::EAssocType asType = theMap._assocType;
        theMap.SetAssocType( TShapeShapeMap::COMMON_VERTEX );
        if ( FindSubShapeAssociation( theShape1, theMesh1, theShape2, theMesh2, theMap ))
          return true;
        theMap._assocType = asType;
      }
    }
  }

  // Find transformation to make the shapes be of similar size at same location

  Bnd_Box box[2];
  for ( int i = 1; i <= vMap1.Extent(); ++i )
    box[ 0 ].Add( BRep_Tool::Pnt ( TopoDS::Vertex( vMap1( i ))));
  for ( int i = 1; i <= vMap2.Extent(); ++i )
    box[ 1 ].Add( BRep_Tool::Pnt ( TopoDS::Vertex( vMap2( i ))));

  gp_Pnt gc[2]; // box center
  double x0,y0,z0, x1,y1,z1;
  box[0].Get( x0,y0,z0, x1,y1,z1 );
  gc[0] = 0.5 * ( gp_XYZ( x0,y0,z0 ) + gp_XYZ( x1,y1,z1 ));
  box[1].Get( x0,y0,z0, x1,y1,z1 );
  gc[1] = 0.5 * ( gp_XYZ( x0,y0,z0 ) + gp_XYZ( x1,y1,z1 ));

  // 1 -> 2
  gp_Vec vec01( gc[0], gc[1] );
  double scale = sqrt( box[1].SquareExtent() / box[0].SquareExtent() );

  // Find 2 closest vertices

  // get 2 linked vertices of shape 1 not belonging to an inner wire of a face
  std::list< TopoDS_Edge > allBndEdges1;
  if ( !getOuterEdges( theShape1, *theMesh1, allBndEdges1 ))
  {
    if ( theShape1.ShapeType() != TopAbs_FACE )
      RETURN_BAD_RESULT("Edge not found");
    return assocFewEdgesFaces( TopoDS::Face( theShape1 ), theMesh1, 
                               TopoDS::Face( theShape2 ), theMesh2, theMap );
  }
  std::list< TopoDS_Edge >::iterator edge1 = allBndEdges1.begin();
  double minDist = std::numeric_limits<double>::max();
  for ( int nbChecked=0; edge1 != allBndEdges1.end() && nbChecked++ < 10; ++edge1 )
  {
    TopoDS_Vertex edge1VV[2];
    TopExp::Vertices( TopoDS::Edge( edge1->Oriented(TopAbs_FORWARD)), edge1VV[0], edge1VV[1]);
    if ( edge1VV[0].IsSame( edge1VV[1] ))
      continue;//RETURN_BAD_RESULT("Only closed edges");

    // find vertices closest to 2 linked vertices of shape 1
    double dist2[2] = { 1e+100, 1e+100 };
    TopoDS_Vertex edge2VV[2];
    for ( int i1 = 0; i1 < 2; ++i1 )
    {
      gp_Pnt p1 = BRep_Tool::Pnt( edge1VV[ i1 ]);
      p1.Scale( gc[0], scale );
      p1.Translate( vec01 );
      if ( !i1 ) {
        // select a closest vertex among all ones in vMap2
        for ( int i2 = 1; i2 <= vMap2.Extent(); ++i2 )
        {
          TopoDS_Vertex V2 = TopoDS::Vertex( vMap2( i2 ));
          gp_Pnt        p2 = BRep_Tool::Pnt ( V2 );
          double        d2 = p1.SquareDistance( p2 );
          if ( d2 < dist2[ 0 ] && d2 < minDist ) {
            edge2VV[ 0 ] = V2;
            dist2  [ 0 ] = d2;
          }
        }
      }
      else if ( !edge2VV[0].IsNull() ) {
        // select a closest vertex among ends of edges meeting at edge2VV[0]
        PShapeIteratorPtr edgeIt = SMESH_MesherHelper::GetAncestors( edge2VV[0],
                                                                     *theMesh2, TopAbs_EDGE);
        while ( const TopoDS_Shape* edge2 = edgeIt->next() )
          for ( TopoDS_Iterator itV2( *edge2 ); itV2.More(); itV2.Next() )
          {
            if ( itV2.Value().IsSame( edge2VV[ 0 ])) continue;
            if ( !vMap2.Contains( itV2.Value()    )) continue;
            TopoDS_Vertex V2 = TopoDS::Vertex( itV2.Value() );
            gp_Pnt        p2 = BRep_Tool::Pnt ( V2 );
            double        d2 = p1.SquareDistance( p2 );
            if ( d2 < dist2[1] && d2 < minDist ) {
              edge2VV[ 1 ] = V2;
              dist2  [ 1 ] = d2;
            }
          }
      }
    }
    if ( dist2[0] + dist2[1] < minDist ) {
      VV1[0] = edge1VV[0];
      VV1[1] = edge1VV[1];
      VV2[0] = edge2VV[0];
      VV2[1] = edge2VV[1];
      minDist = dist2[0] + dist2[1];
      if ( minDist < 1e-10 )
        break;
    }
  }
  theMap.SetAssocType( TShapeShapeMap::CLOSE_VERTEX );

  InsertAssociation( VV1[ 0 ], VV2[ 0 ], theMap );
  InsertAssociation( VV1[ 1 ], VV2[ 1 ], theMap );
  MESSAGE("Initial assoc VERT " << theMesh1->GetMeshDS()->ShapeToIndex( VV1[ 0 ] )<<
          " to "                << theMesh2->GetMeshDS()->ShapeToIndex( VV2[ 0 ] )<<
          "\nand         VERT " << theMesh1->GetMeshDS()->ShapeToIndex( VV1[ 1 ] )<<
          " to "                << theMesh2->GetMeshDS()->ShapeToIndex( VV2[ 1 ] ));
  if ( theShape1.ShapeType() == TopAbs_EDGE ) {
    InsertAssociation( theShape1, theShape2, theMap );
    return true;
  }

  return FindSubShapeAssociation( theShape1, theMesh1, theShape2, theMesh2, theMap );
}

//================================================================================
/*
 * Find association of edges of faces
 *  \param face1 - face 1
 *  \param VV1 - vertices of face 1
 *  \param face2 - face 2
 *  \param VV2 - vertices of face 2 associated with ones of face 1
 *  \param edges1 - out list of edges of face 1
 *  \param edges2 - out list of edges of face 2
 *  \param isClosenessAssoc - is association starting by VERTEX closeness
 *  \retval int - nb of edges in an outer wire in a success case, else zero
 */
//================================================================================

int StdMeshers_ProjectionUtils::FindFaceAssociation(const TopoDS_Face&    face1,
                                                    TopoDS_Vertex         VV1[2],
                                                    const TopoDS_Face&    face2,
                                                    TopoDS_Vertex         VV2[2],
                                                    list< TopoDS_Edge > & edges1,
                                                    list< TopoDS_Edge > & edges2,
                                                    const bool            isClosenessAssoc)
{
  bool OK = false;
  list< int > nbEInW1, nbEInW2;
  list< TopoDS_Edge >::iterator edgeIt;
  int i_ok_wire_algo = -1;
  for ( int outer_wire_algo = 0; outer_wire_algo < 2 && !OK; ++outer_wire_algo )
  {
    edges1.clear();
    edges2.clear();

    if ( SMESH_Block::GetOrderedEdges( face1, edges1, nbEInW1, VV1[0], outer_wire_algo) !=
         SMESH_Block::GetOrderedEdges( face2, edges2, nbEInW2, VV2[0], outer_wire_algo) )
      CONT_BAD_RESULT("Different number of wires in faces ");

    if ( nbEInW1 != nbEInW2 && outer_wire_algo == 0 &&
         ( std::accumulate( nbEInW1.begin(), nbEInW1.end(), 0) !=
           std::accumulate( nbEInW2.begin(), nbEInW2.end(), 0)))
      RETURN_BAD_RESULT("Different number of edges in faces");

    if ( nbEInW1.front() != nbEInW2.front() )
      CONT_BAD_RESULT("Different number of edges in the outer wire: " <<
                      nbEInW1.front() << " != " << nbEInW2.front());

    i_ok_wire_algo = outer_wire_algo;

    // Define if we need to reverse one of wires to make edges in lists match each other

    bool reverse = false;
    const bool severalWires = ( nbEInW1.size() > 1 );

    if ( !VV1[1].IsSame( TopExp::LastVertex( edges1.front(), true )))
    {
      reverse = true;
      // check if the second vertex belongs to the first or last edge in the wire
      edgeIt = --edges1.end(); // pointer to the last edge in the outer wire
      if ( severalWires ) {
        edgeIt = edges1.begin();
        std::advance( edgeIt, nbEInW1.front()-1 );
      }
      if ( TopExp::FirstVertex( *edgeIt ).IsSame( TopExp::LastVertex( *edgeIt )) &&
           SMESH_Algo::isDegenerated( *edgeIt )) {
        --edgeIt; // skip a degenerated edge (test 3D_mesh_Projection_00/A3)
      }
      if ( !VV1[1].IsSame( TopExp::FirstVertex( *edgeIt, true ))) {
        CONT_BAD_RESULT("GetOrderedEdges() failed");
      }
    }
    if ( !VV2[1].IsSame( TopExp::LastVertex( edges2.front(), true )))
    {
      reverse = !reverse;
      // check if the second vertex belongs to the first or last edge in the wire
      edgeIt = --edges2.end(); // pointer to the last edge in the outer wire
      if ( severalWires ) {
        edgeIt = edges2.begin();
        std::advance( edgeIt, nbEInW2.front()-1 );
      }
      if ( TopExp::FirstVertex( *edgeIt ).IsSame( TopExp::LastVertex( *edgeIt )) &&
           SMESH_Algo::isDegenerated( *edgeIt )) {
        --edgeIt;  // skip a degenerated edge
      }
      if ( !VV2[1].IsSame( TopExp::FirstVertex( *edgeIt, true ))) {
        CONT_BAD_RESULT("GetOrderedEdges() failed");
      }
    }
    if ( reverse )
    {
      reverseEdges( edges2 , nbEInW2.front());

      if ( SMESH_Algo::isDegenerated( edges2.front() ))
      {
        // move a degenerated edge to the back of the outer wire
        edgeIt = edges2.end();
        if ( severalWires ) {
          edgeIt = edges2.begin();
          std::advance( edgeIt, nbEInW2.front() );
        }
        edges2.splice( edgeIt, edges2, edges2.begin() );
      }
      if (( VV1[1].IsSame( TopExp::LastVertex( edges1.front(), true ))) !=
          ( VV2[1].IsSame( TopExp::LastVertex( edges2.front(), true ))))
        CONT_BAD_RESULT("GetOrderedEdges() failed");
    }
    OK = true;

  } // loop algos getting an outer wire

  if ( OK && nbEInW1.front() > 4 ) // care of a case where faces are closed (23032)
  {
    // check if the first edges are seam ones
    list< TopoDS_Edge >::iterator revSeam1, revSeam2;
    revSeam1 = std::find( ++edges1.begin(), edges1.end(), edges1.front().Reversed());
    revSeam2 = edges2.end();
    if ( revSeam1 != edges1.end() )
      revSeam2 = std::find( ++edges2.begin(), edges2.end(), edges2.front().Reversed());
    if ( revSeam2 != edges2.end() ) // two seams detected
    {
      bool reverse =
        std::distance( edges1.begin(), revSeam1 ) != std::distance( edges2.begin(), revSeam2 );
      if ( !reverse && isClosenessAssoc )
      {
        // compare orientations of a non-seam edges using 3D closeness;
        // look for a non-seam edges
        list< TopoDS_Edge >::iterator edge1 = ++edges1.begin();
        list< TopoDS_Edge >::iterator edge2 = ++edges2.begin();
        for ( ; edge1 != edges1.end(); ++edge1, ++edge2 )
        {
          if (( edge1 == revSeam1 ) ||
              ( SMESH_Algo::isDegenerated( *edge1 )) ||
              ( std::find( ++edges1.begin(), edges1.end(), edge1->Reversed()) != edges1.end() ))
            continue;
          gp_Pnt p1 = BRep_Tool::Pnt( VV1[0] );
          gp_Pnt p2 = BRep_Tool::Pnt( VV2[0] );
          gp_Vec vec2to1( p2, p1 );

          gp_Pnt pp1[2], pp2[2];
          const double r = 0.2345;
          double f,l;
          Handle(Geom_Curve) C = BRep_Tool::Curve( *edge1, f,l );
          pp1[0] = C->Value( f * r + l * ( 1. - r ));
          pp1[1] = C->Value( l * r + f * ( 1. - r ));
          if ( edge1->Orientation() == TopAbs_REVERSED )
            std::swap( pp1[0], pp1[1] );
          C = BRep_Tool::Curve( *edge2, f,l );
          if ( C.IsNull() ) return 0;
          pp2[0] = C->Value( f * r + l * ( 1. - r )).Translated( vec2to1 );
          pp2[1] = C->Value( l * r + f * ( 1. - r )).Translated( vec2to1 );
          if ( edge2->Orientation() == TopAbs_REVERSED )
            std::swap( pp2[0], pp2[1] );

          double dist00 = pp1[0].SquareDistance( pp2[0] );
          double dist01 = pp1[0].SquareDistance( pp2[1] );
          reverse = ( dist00 > dist01 );
          break;
        }
      }
      if ( reverse ) // make a seam counterpart be the first
      {
        list< TopoDS_Edge >::iterator outWireEnd = edges2.begin();
        std::advance( outWireEnd, nbEInW2.front() );
        edges2.splice( outWireEnd, edges2, edges2.begin(), ++revSeam2 );
        reverseEdges( edges2 , nbEInW2.front());
      }
    }
  }
  
  // Try to orient all (if !OK) or only internal wires (issue 0020996) by UV similarity

  if (( !OK || nbEInW1.size() > 1 ) && i_ok_wire_algo > -1 )
  {
    // Check that Vec(VV1[0],VV1[1]) in 2D on face1 is the same
    // as Vec(VV2[0],VV2[1]) on face2
    double vTol = BRep_Tool::Tolerance( VV1[0] );
    BRepAdaptor_Surface surface1( face1, true );
    BRepAdaptor_Surface surface2( face2, true );
    // TODO: use TrsfFinder2D to superpose the faces
    gp_Pnt2d v0f1UV( surface1.FirstUParameter(), surface1.FirstVParameter() );
    gp_Pnt2d v0f2UV( surface2.FirstUParameter(), surface2.FirstVParameter() );
    gp_Pnt2d v1f1UV( surface1.LastUParameter(),  surface1.LastVParameter() );
    gp_Pnt2d v1f2UV( surface2.LastUParameter(),  surface2.LastVParameter() );
    double vTolUV =
      surface1.UResolution( vTol ) + surface1.VResolution( vTol ); // let's be tolerant
    // VV1[0] = TopExp::FirstVertex( edges1.front(), true ); // ori is important if face is closed
    // VV1[1] = TopExp::LastVertex ( edges1.front(), true );
    // VV2[0] = TopExp::FirstVertex( edges2.front(), true );
    // VV2[1] = TopExp::LastVertex ( edges2.front(), true );
    // gp_Pnt2d v0f1UV = BRep_Tool::Parameters( VV1[0], face1 );
    // gp_Pnt2d v0f2UV = BRep_Tool::Parameters( VV2[0], face2 );
    // gp_Pnt2d v1f1UV = BRep_Tool::Parameters( VV1[1], face1 );
    // gp_Pnt2d v1f2UV = BRep_Tool::Parameters( VV2[1], face2 );
    gp_Vec2d v01f1Vec( v0f1UV, v1f1UV );
    gp_Vec2d v01f2Vec( v0f2UV, v1f2UV );
    if ( Abs( v01f1Vec.X()-v01f2Vec.X()) < vTolUV &&
         Abs( v01f1Vec.Y()-v01f2Vec.Y()) < vTolUV )
    {
      if ( !OK /*i_ok_wire_algo != 1*/ )
      {
        edges1.clear();
        edges2.clear();
        SMESH_Block::GetOrderedEdges( face1, edges1, nbEInW1, VV1[0], i_ok_wire_algo);
        SMESH_Block::GetOrderedEdges( face2, edges2, nbEInW2, VV2[0], i_ok_wire_algo);
      }
      gp_XY dUV = v0f2UV.XY() - v0f1UV.XY(); // UV shift between 2 faces
      //
      // skip edges of the outer wire (if the outer wire is OK)
      list< int >::iterator nbE2, nbE1 = nbEInW1.begin();
      list< TopoDS_Edge >::iterator edge2Beg, edge1Beg = edges1.begin();
      if ( OK ) std::advance( edge1Beg, *nbE1++ );
      list< TopoDS_Edge >::iterator edge2End, edge1End;
      //
      // find corresponding wires of face2
      for ( int iW1 = OK; nbE1 != nbEInW1.end(); ++nbE1, ++iW1 ) // loop on wires of face1
      {
        // reach an end of edges of a current wire1
        edge1End = edge1Beg;
        std::advance( edge1End, *nbE1 );
        // UV on face1 to find on face2
        TopoDS_Vertex v01 = SMESH_MesherHelper::IthVertex(0,*edge1Beg);
        TopoDS_Vertex v11 = SMESH_MesherHelper::IthVertex(1,*edge1Beg);
        v0f1UV = BRep_Tool::Parameters( v01, face1 );
        v1f1UV = BRep_Tool::Parameters( v11, face1 );
        v0f1UV.ChangeCoord() += dUV;
        v1f1UV.ChangeCoord() += dUV;
        //
        // look through wires of face2
        edge2Beg = edges2.begin();
        nbE2     = nbEInW2.begin();
        if ( OK ) std::advance( edge2Beg, *nbE2++ );
        for ( int iW2 = OK; nbE2 != nbEInW2.end(); ++nbE2, ++iW2 ) // loop on wires of face2
        {
          // reach an end of edges of a current wire2
          edge2End = edge2Beg;
          std::advance( edge2End, *nbE2 );
          if ( *nbE1 == *nbE2 && iW2 >= iW1 )
          {
            // rotate edge2 untill coincidence with edge1 in 2D
            int i = *nbE2;
            bool sameUV = false;
            while ( !( sameUV = sameVertexUV( *edge2Beg, face2, 0, v0f1UV, vTolUV )) && --i > 0 )
              // move edge2Beg to place before edge2End
              edges2.splice( edge2End, edges2, edge2Beg++ );

            if ( sameUV )
            {
              if ( iW1 == 0 ) OK = true; // OK is for the first wire

              // reverse edges2 if needed
              if ( SMESH_MesherHelper::IsClosedEdge( *edge1Beg ))
              {
                // Commented (so far?) as it's not checked if orientation must be same or reversed
                // double f,l;
                // Handle(Geom2d_Curve) c1 = BRep_Tool::CurveOnSurface( *edge1Beg, face1,f,l );
                // if (  edge1Beg->Orientation() == TopAbs_REVERSED )
                //   std::swap( f,l );
                // gp_Pnt2d uv1 = dUV + c1->Value( f * 0.8 + l * 0.2 ).XY();

                // Handle(Geom2d_Curve) c2 = BRep_Tool::CurveOnSurface( *edge2Beg, face2,f,l );
                // if (  edge2Beg->Orientation() == TopAbs_REVERSED )
                //   std::swap( f,l );
                // gp_Pnt2d uv2 = c2->Value( f * 0.8 + l * 0.2 );
                // gp_Pnt2d uv3 = c2->Value( l * 0.8 + f * 0.2 );

                // if ( uv1.SquareDistance( uv2 ) > uv1.SquareDistance( uv3 ))
                //   edge2Beg->Reverse();
              }
              else
              {
                if ( !sameVertexUV( *edge2Beg, face2, 1, v1f1UV, vTolUV ))
                  reverseEdges( edges2 , *nbE2, std::distance( edges2.begin(),edge2Beg ));
              }

              // put wire2 at a right place within edges2
              if ( iW1 != iW2 ) {
                list< TopoDS_Edge >::iterator place2 = edges2.begin();
                std::advance( place2, std::distance( edges1.begin(), edge1Beg ));
                edges2.splice( place2, edges2, edge2Beg, edge2End );
                // move nbE2 as well
                list< int >::iterator placeNbE2 = nbEInW2.begin();
                std::advance( placeNbE2, iW1 );
                nbEInW2.splice( placeNbE2, nbEInW2, nbE2 );
              }
              break;
            }
          }
          // prepare to the next wire loop
          edge2Beg = edge2End;
        }
        edge1Beg = edge1End;
      }
    }
  }

  const int nbEdges = nbEInW1.front();
  if ( OK && nbEdges == 2 )
  {
    // if wires include 2 edges, it's impossible to associate them using
    // topological information only. Try to use length of edges for association.
    double l1[2], l2[2];
    edgeIt = edges1.begin();
    l1[0] = SMESH_Algo::EdgeLength( *edgeIt++ );
    l1[1] = SMESH_Algo::EdgeLength( *edgeIt++ );
    if ( Abs( l1[0] - l1[1] ) > 0.1 * Max( l1[0], l1[1] ) )
    {
      edgeIt = edges2.begin();
      l2[0] = SMESH_Algo::EdgeLength( *edgeIt++ );
      l2[1] = SMESH_Algo::EdgeLength( *edgeIt++ );
      if (( l1[0] < l1[1] ) != ( l2[0] < l2[1] ))
      {
        reverseEdges( edges2, nbEdges );
      }
    }
  }

  return OK ? nbEInW1.front() : 0;
}

//=======================================================================
//function : InitVertexAssociation
//purpose  : 
//=======================================================================

void StdMeshers_ProjectionUtils::InitVertexAssociation( const SMESH_Hypothesis* theHyp,
                                                        TShapeShapeMap &        theAssociationMap)
{
  string hypName = theHyp->GetName();
  if ( hypName == "ProjectionSource1D" ) {
    const StdMeshers_ProjectionSource1D * hyp =
      static_cast<const StdMeshers_ProjectionSource1D*>( theHyp );
    if ( hyp->HasVertexAssociation() )
      InsertAssociation( hyp->GetTargetVertex(),hyp->GetSourceVertex(),theAssociationMap );
  }
  else if ( hypName == "ProjectionSource2D" ) {
    const StdMeshers_ProjectionSource2D * hyp =
      static_cast<const StdMeshers_ProjectionSource2D*>( theHyp );
    if ( hyp->HasVertexAssociation() ) {
      InsertAssociation( hyp->GetTargetVertex(1),hyp->GetSourceVertex(1),theAssociationMap);
      InsertAssociation( hyp->GetTargetVertex(2),hyp->GetSourceVertex(2),theAssociationMap);
    }
  }
  else if ( hypName == "ProjectionSource3D" ) {
    const StdMeshers_ProjectionSource3D * hyp =
      static_cast<const StdMeshers_ProjectionSource3D*>( theHyp );
    if ( hyp->HasVertexAssociation() ) {
      InsertAssociation( hyp->GetTargetVertex(1),hyp->GetSourceVertex(1),theAssociationMap);
      InsertAssociation( hyp->GetTargetVertex(2),hyp->GetSourceVertex(2),theAssociationMap);
    }
  }
}

//=======================================================================
/*
 * Inserts association theShape1 <-> theShape2 to TShapeShapeMap
 *  \param theShape1 - target shape
 *  \param theShape2 - source shape
 *  \param theAssociationMap - association map 
 *  \retval bool - true if there was no association for these shapes before
 */
//=======================================================================

bool StdMeshers_ProjectionUtils::InsertAssociation( const TopoDS_Shape& theShape1, // tgt
                                                    const TopoDS_Shape& theShape2, // src
                                                    TShapeShapeMap &    theAssociationMap)
{
  if ( !theShape1.IsNull() && !theShape2.IsNull() ) {
    SHOW_SHAPE(theShape1,"Assoc ");
    SHOW_SHAPE(theShape2," to ");
    bool isNew = ( theAssociationMap.Bind( theShape1, theShape2 ));
    return isNew;
  }
  else {
    throw SALOME_Exception("StdMeshers_ProjectionUtils: attempt to associate NULL shape");
  }
  return false;
}

//=======================================================================
/*
 * Finds an edge by its vertices in a main shape of the mesh
 *  \param aMesh - the mesh
 *  \param V1 - vertex 1
 *  \param V2 - vertex 2
 *  \retval TopoDS_Edge - found edge
 */
//=======================================================================

TopoDS_Edge StdMeshers_ProjectionUtils::GetEdgeByVertices( SMESH_Mesh*          theMesh,
                                                           const TopoDS_Vertex& theV1,
                                                           const TopoDS_Vertex& theV2)
{
  if ( theMesh && !theV1.IsNull() && !theV2.IsNull() )
  {
    TopTools_ListIteratorOfListOfShape ancestorIt( theMesh->GetAncestors( theV1 ));
    for ( ; ancestorIt.More(); ancestorIt.Next() )
      if ( ancestorIt.Value().ShapeType() == TopAbs_EDGE )
        for ( TopExp_Explorer expV ( ancestorIt.Value(), TopAbs_VERTEX );
              expV.More();
              expV.Next() )
          if ( theV2.IsSame( expV.Current() ))
            return TopoDS::Edge( ancestorIt.Value() );
  }
  return TopoDS_Edge();
}

//================================================================================
/*
 * Return another face sharing an edge
 *  \param edgeToFaces - data map of descendants to ancestors
 *  \param edge - edge
 *  \param face - face
 *  \retval TopoDS_Face - found face
 */
//================================================================================

TopoDS_Face StdMeshers_ProjectionUtils::GetNextFace( const TAncestorMap& edgeToFaces,
                                                     const TopoDS_Edge&  edge,
                                                     const TopoDS_Face&  face)
{
//   if ( !edge.IsNull() && !face.IsNull() && edgeToFaces.Contains( edge ))
  if ( !edge.IsNull() && edgeToFaces.Contains( edge )) // PAL16202
  {
    TopTools_ListIteratorOfListOfShape ancestorIt( edgeToFaces.FindFromKey( edge ));
    for ( ; ancestorIt.More(); ancestorIt.Next() )
      if ( ancestorIt.Value().ShapeType() == TopAbs_FACE &&
           !face.IsSame( ancestorIt.Value() ))
        return TopoDS::Face( ancestorIt.Value() );
  }
  return TopoDS_Face();
}

//================================================================================
/*
 * Return other vertex of an edge
 */
//================================================================================

TopoDS_Vertex StdMeshers_ProjectionUtils::GetNextVertex(const TopoDS_Edge&   edge,
                                                        const TopoDS_Vertex& vertex)
{
  TopoDS_Vertex vF,vL;
  TopExp::Vertices(edge,vF,vL);
  if ( vF.IsSame( vL ))
    return TopoDS_Vertex();
  return vertex.IsSame( vF ) ? vL : vF; 
}

//================================================================================
/*
 * Return a propagation edge
 *  \param aMesh - mesh
 *  \param anEdge - edge to find by propagation
 *  \param fromEdge - start edge for propagation
 *  \param chain - return, if !NULL, a propagation chain passed till
 *         anEdge; if anEdge.IsNull() then a full propagation chain is returned;
 *         fromEdge is the 1st in the chain
 *  \retval pair<int,TopoDS_Edge> - propagation step and found edge
 */
//================================================================================

pair<int,TopoDS_Edge>
StdMeshers_ProjectionUtils::GetPropagationEdge( SMESH_Mesh*                 aMesh,
                                                const TopoDS_Edge&          anEdge,
                                                const TopoDS_Edge&          fromEdge,
                                                TopTools_IndexedMapOfShape* chain)
{
  TopTools_IndexedMapOfShape locChain;
  TopTools_IndexedMapOfShape& aChain = chain ? *chain : locChain;
  int step = 0;

  //TopTools_IndexedMapOfShape checkedWires;
  BRepTools_WireExplorer aWE;
  TopoDS_Shape fourEdges[4];

  // List of edges, added to chain on the previous cycle pass
  TopTools_ListOfShape listPrevEdges;
  listPrevEdges.Append( fromEdge );
  aChain.Add( fromEdge );

  // Collect all edges pass by pass
  while (listPrevEdges.Extent() > 0)
  {
    step++;
    // List of edges, added to chain on this cycle pass
    TopTools_ListOfShape listCurEdges;

    // Find the next portion of edges
    TopTools_ListIteratorOfListOfShape itE (listPrevEdges);
    for (; itE.More(); itE.Next())
    {
      const TopoDS_Shape& anE = itE.Value();

      // Iterate on faces, having edge <anE>
      TopTools_ListIteratorOfListOfShape itA (aMesh->GetAncestors(anE));
      for (; itA.More(); itA.Next())
      {
        const TopoDS_Shape& aW = itA.Value();

        // There are objects of different type among the ancestors of edge
        if ( aW.ShapeType() == TopAbs_WIRE /*&& checkedWires.Add( aW )*/)
        {
          Standard_Integer nb = 0, found = -1;
          for ( aWE.Init( TopoDS::Wire( aW )); aWE.More(); aWE.Next() ) {
            if (nb+1 > 4) {
              found = -1;
              break;
            }
            fourEdges[ nb ] = aWE.Current();
            if ( aWE.Current().IsSame( anE )) found = nb;
            nb++;
          }
          if (nb == 4 && found >= 0) {
            // Quadrangle face found, get an opposite edge
            TopoDS_Shape& anOppE = fourEdges[( found + 2 ) % 4 ];

            // add anOppE to aChain if ...
            int prevChainSize = aChain.Extent();
            if ( aChain.Add(anOppE) > prevChainSize ) { // ... anOppE is not in aChain
              // Add found edge to the chain oriented so that to
              // have it co-directed with a forward MainEdge
              TopAbs_Orientation ori = anE.Orientation();
              if ( anOppE.Orientation() == fourEdges[found].Orientation() )
                ori = TopAbs::Reverse( ori );
              anOppE.Orientation( ori );
              if ( anOppE.IsSame( anEdge ))
                return make_pair( step, TopoDS::Edge( anOppE ));
              listCurEdges.Append(anOppE);
            }
          } // if (nb == 4 && found >= 0)
        } // if (aF.ShapeType() == TopAbs_WIRE)
      } // loop on ancestors of anE
    } // loop on listPrevEdges

    listPrevEdges = listCurEdges;
  } // while (listPrevEdges.Extent() > 0)

  return make_pair( INT_MAX, TopoDS_Edge());
}

//================================================================================
/*
 * Find corresponding nodes on two faces
 *  \param face1 - the first face
 *  \param mesh1 - mesh containing elements on the first face
 *  \param face2 - the second face
 *  \param mesh2 - mesh containing elements on the second face
 *  \param assocMap - map associating sub-shapes of the faces
 *  \param node1To2Map - map containing found matching nodes
 *  \retval bool - is a success
 */
//================================================================================

bool StdMeshers_ProjectionUtils::
FindMatchingNodesOnFaces( const TopoDS_Face&     face1,
                          SMESH_Mesh*            mesh1,
                          const TopoDS_Face&     face2,
                          SMESH_Mesh*            mesh2,
                          const TShapeShapeMap & assocMap,
                          TNodeNodeMap &         node1To2Map)
{
  SMESHDS_Mesh* meshDS1 = mesh1->GetMeshDS();
  SMESHDS_Mesh* meshDS2 = mesh2->GetMeshDS();

  SMESH_MesherHelper helper1( *mesh1 );
  SMESH_MesherHelper helper2( *mesh2 );

  // Get corresponding submeshes and roughly check match of meshes

  SMESHDS_SubMesh * SM2 = meshDS2->MeshElements( face2 );
  SMESHDS_SubMesh * SM1 = meshDS1->MeshElements( face1 );
  if ( !SM2 || !SM1 )
    RETURN_BAD_RESULT("Empty submeshes");
  if ( SM2->NbNodes()    != SM1->NbNodes() ||
       SM2->NbElements() != SM1->NbElements() )
    RETURN_BAD_RESULT("Different meshes on corresponding faces "
                      << meshDS1->ShapeToIndex( face1 ) << " and "
                      << meshDS2->ShapeToIndex( face2 ));
  if ( SM2->NbElements() == 0 )
    RETURN_BAD_RESULT("Empty submeshes");

  helper1.SetSubShape( face1 );
  helper2.SetSubShape( face2 );
  if ( helper1.HasSeam() != helper2.HasSeam() )
    RETURN_BAD_RESULT("Different faces' geometry");

  // Data to call SMESH_MeshEditor::FindMatchingNodes():

  // 1. Nodes of corresponding links:

  // get 2 matching edges, try to find not seam ones
  TopoDS_Edge edge1, edge2, seam1, seam2, anyEdge1, anyEdge2;
  TopExp_Explorer eE( OuterShape( face2, TopAbs_WIRE ), TopAbs_EDGE );
  do {
    // edge 2
    TopoDS_Edge e2 = TopoDS::Edge( eE.Current() );
    eE.Next();
    // edge 1
    if ( !assocMap.IsBound( e2, /*is2nd=*/true ))
      continue;
      //RETURN_BAD_RESULT("Association not found for edge " << meshDS2->ShapeToIndex( e2 ));
    TopoDS_Edge e1 = TopoDS::Edge( assocMap( e2, /*is2nd=*/true ));
    if ( !helper1.IsSubShape( e1, face1 ))
      RETURN_BAD_RESULT("Wrong association, edge " << meshDS1->ShapeToIndex( e1 ) <<
                        " isn't a sub-shape of face " << meshDS1->ShapeToIndex( face1 ));
    // check that there are nodes on edges
    SMESHDS_SubMesh * eSM1 = meshDS1->MeshElements( e1 );
    SMESHDS_SubMesh * eSM2 = meshDS2->MeshElements( e2 );
    bool nodesOnEdges = ( eSM1 && eSM2 && eSM1->NbNodes() && eSM2->NbNodes() );
    // check that the nodes on edges belong to faces
    // (as NETGEN ignores nodes on the degenerated geom edge)
    bool nodesOfFaces = false;
    if ( nodesOnEdges ) {
      const SMDS_MeshNode* n1 = eSM1->GetNodes()->next();
      const SMDS_MeshNode* n2 = eSM2->GetNodes()->next();
      nodesOfFaces = ( n1->GetInverseElementIterator(SMDSAbs_Face)->more() &&
                       n2->GetInverseElementIterator(SMDSAbs_Face)->more() );
    }
    if ( nodesOfFaces )
    {
      if ( helper2.IsRealSeam( e2 )) {
        seam1 = e1; seam2 = e2;
      }
      else {
        edge1 = e1; edge2 = e2;
      }
    }
    else {
      anyEdge1 = e1; anyEdge2 = e2;
    }
  } while ( edge2.IsNull() && eE.More() );
  //
  if ( edge2.IsNull() ) {
    edge1 = seam1; edge2 = seam2;
  }
  bool hasNodesOnEdge = (! edge2.IsNull() );
  if ( !hasNodesOnEdge ) {
    // 0020338 - nb segments == 1
    edge1 = anyEdge1; edge2 = anyEdge2;
  }

  // get 2 matching vertices
  TopoDS_Vertex V2 = TopExp::FirstVertex( TopoDS::Edge( edge2 ));
  if ( !assocMap.IsBound( V2, /*is2nd=*/true ))
  {
    V2 = TopExp::LastVertex( TopoDS::Edge( edge2 ));
    if ( !assocMap.IsBound( V2, /*is2nd=*/true ))
      RETURN_BAD_RESULT("Association not found for vertex " << meshDS2->ShapeToIndex( V2 ));
  }
  TopoDS_Vertex V1 = TopoDS::Vertex( assocMap( V2, /*is2nd=*/true ));

  // nodes on vertices
  const SMDS_MeshNode* vNode1 = SMESH_Algo::VertexNode( V1, meshDS1 );
  const SMDS_MeshNode* vNode2 = SMESH_Algo::VertexNode( V2, meshDS2 );
  if ( !vNode1 ) RETURN_BAD_RESULT("No node on vertex #" << meshDS1->ShapeToIndex( V1 ));
  if ( !vNode2 ) RETURN_BAD_RESULT("No node on vertex #" << meshDS2->ShapeToIndex( V2 ));

  // nodes on edges linked with nodes on vertices
  const SMDS_MeshNode* nullNode = 0;
  vector< const SMDS_MeshNode*> eNode1( 2, nullNode );
  vector< const SMDS_MeshNode*> eNode2( 2, nullNode );
  if ( hasNodesOnEdge )
  {
    int nbNodeToGet = 1;
    if ( helper1.IsClosedEdge( edge1 ) || helper2.IsClosedEdge( edge2 ) )
      nbNodeToGet = 2;
    for ( int is2 = 0; is2 < 2; ++is2 )
    {
      TopoDS_Edge &     edge  = is2 ? edge2 : edge1;
      SMESHDS_Mesh *    smDS  = is2 ? meshDS2 : meshDS1;
      SMESHDS_SubMesh* edgeSM = smDS->MeshElements( edge );
      // nodes linked with ones on vertices
      const SMDS_MeshNode*           vNode = is2 ? vNode2 : vNode1;
      vector< const SMDS_MeshNode*>& eNode = is2 ? eNode2 : eNode1;
      int nbGotNode = 0;
      SMDS_ElemIteratorPtr vElem = vNode->GetInverseElementIterator(SMDSAbs_Edge);
      while ( vElem->more() && nbGotNode != nbNodeToGet ) {
        const SMDS_MeshElement* elem = vElem->next();
        if ( edgeSM->Contains( elem ))
          eNode[ nbGotNode++ ] = 
            ( elem->GetNode(0) == vNode ) ? elem->GetNode(1) : elem->GetNode(0);
      }
      if ( nbGotNode > 1 ) // sort found nodes by param on edge
      {
        SMESH_MesherHelper* helper = is2 ? &helper2 : &helper1;
        double u0 = helper->GetNodeU( edge, eNode[ 0 ]);
        double u1 = helper->GetNodeU( edge, eNode[ 1 ]);
        if ( u0 > u1 ) std::swap( eNode[ 0 ], eNode[ 1 ]);
      }
      if ( nbGotNode == 0 )
        RETURN_BAD_RESULT("Found no nodes on edge " << smDS->ShapeToIndex( edge ) <<
                          " linked to " << vNode );
    }
  }
  else // 0020338 - nb segments == 1
  {
    // get 2 other matching vertices
    V2 = TopExp::LastVertex( TopoDS::Edge( edge2 ));
    if ( !assocMap.IsBound( V2, /*is2nd=*/true ))
      RETURN_BAD_RESULT("Association not found for vertex " << meshDS2->ShapeToIndex( V2 ));
    V1 = TopoDS::Vertex( assocMap( V2, /*is2nd=*/true ));

    // nodes on vertices
    eNode1[0] = SMESH_Algo::VertexNode( V1, meshDS1 );
    eNode2[0] = SMESH_Algo::VertexNode( V2, meshDS2 );
    if ( !eNode1[0] ) RETURN_BAD_RESULT("No node on vertex #" << meshDS1->ShapeToIndex( V1 ));
    if ( !eNode2[0] ) RETURN_BAD_RESULT("No node on vertex #" << meshDS2->ShapeToIndex( V2 ));
  }

  // 2. face sets

  int assocRes;
  for ( int iAttempt = 0; iAttempt < 2; ++iAttempt )
  {
    set<const SMDS_MeshElement*> Elems1, Elems2;
    for ( int is2 = 0; is2 < 2; ++is2 )
    {
      set<const SMDS_MeshElement*> & elems = is2 ? Elems2 : Elems1;
      SMESHDS_SubMesh*                  sm = is2 ? SM2 : SM1;
      SMESH_MesherHelper*           helper = is2 ? &helper2 : &helper1;
      const TopoDS_Face &             face = is2 ? face2 : face1;
      SMDS_ElemIteratorPtr eIt = sm->GetElements();

      if ( !helper->IsRealSeam( is2 ? edge2 : edge1 ))
      {
        while ( eIt->more() ) elems.insert( elems.end(), eIt->next() );
      }
      else
      {
        // the only suitable edge is seam, i.e. it is a sphere.
        // FindMatchingNodes() will not know which way to go from any edge.
        // So we ignore all faces having nodes on edges or vertices except
        // one of faces sharing current start nodes

        // find a face to keep
        const SMDS_MeshElement* faceToKeep = 0;
        const SMDS_MeshNode* vNode = is2 ? vNode2 : vNode1;
        const SMDS_MeshNode* eNode = is2 ? eNode2[0] : eNode1[0];
        TIDSortedElemSet inSet, notInSet;

        const SMDS_MeshElement* f1 =
          SMESH_MeshAlgos::FindFaceInSet( vNode, eNode, inSet, notInSet );
        if ( !f1 ) RETURN_BAD_RESULT("The first face on seam not found");
        notInSet.insert( f1 );

        const SMDS_MeshElement* f2 =
          SMESH_MeshAlgos::FindFaceInSet( vNode, eNode, inSet, notInSet );
        if ( !f2 ) RETURN_BAD_RESULT("The second face on seam not found");

        // select a face with less UV of vNode
        const SMDS_MeshNode* notSeamNode[2] = {0, 0};
        for ( int iF = 0; iF < 2; ++iF ) {
          const SMDS_MeshElement* f = ( iF ? f2 : f1 );
          for ( int i = 0; !notSeamNode[ iF ] && i < f->NbNodes(); ++i ) {
            const SMDS_MeshNode* node = f->GetNode( i );
            if ( !helper->IsSeamShape( node->getshapeId() ))
              notSeamNode[ iF ] = node;
          }
        }
        gp_Pnt2d uv1 = helper->GetNodeUV( face, vNode, notSeamNode[0] );
        gp_Pnt2d uv2 = helper->GetNodeUV( face, vNode, notSeamNode[1] );
        if ( uv1.X() + uv1.Y() > uv2.X() + uv2.Y() )
          faceToKeep = f2;
        else
          faceToKeep = f1;

        // fill elem set
        elems.insert( faceToKeep );
        while ( eIt->more() ) {
          const SMDS_MeshElement* f = eIt->next();
          int nbNodes = f->NbNodes();
          if ( f->IsQuadratic() )
            nbNodes /= 2;
          bool onBnd = false;
          for ( int i = 0; !onBnd && i < nbNodes; ++i ) {
            const SMDS_MeshNode* node = f->GetNode( i );
            onBnd = ( node->GetPosition()->GetTypeOfPosition() != SMDS_TOP_FACE);
          }
          if ( !onBnd )
            elems.insert( f );
        }
        // add also faces adjacent to faceToKeep
        int nbNodes = faceToKeep->NbNodes();
        if ( faceToKeep->IsQuadratic() ) nbNodes /= 2;
        notInSet.insert( f1 );
        notInSet.insert( f2 );
        for ( int i = 0; i < nbNodes; ++i ) {
          const SMDS_MeshNode* n1 = faceToKeep->GetNode( i );
          const SMDS_MeshNode* n2 = faceToKeep->GetNode(( i+1 ) % nbNodes );
          f1 = SMESH_MeshAlgos::FindFaceInSet( n1, n2, inSet, notInSet );
          if ( f1 )
            elems.insert( f1 );
        }
      } // case on a sphere
    } // loop on 2 faces

    node1To2Map.clear();
    assocRes = SMESH_MeshEditor::FindMatchingNodes( Elems1, Elems2,
                                                    vNode1, vNode2,
                                                    eNode1[0], eNode2[0],
                                                    node1To2Map);
    if (( assocRes != SMESH_MeshEditor::SEW_OK ) &&
        ( eNode1[1] || eNode2[1] )) // there is another node to try (on a closed EDGE)
    {
      node1To2Map.clear();
      if ( eNode1[1] ) std::swap( eNode1[0], eNode1[1] );
      else             std::swap( eNode2[0], eNode2[1] );
      continue; // one more attempt
    }

    break;
  }
  if ( assocRes != SMESH_MeshEditor::SEW_OK )
    RETURN_BAD_RESULT("FindMatchingNodes() result " << assocRes );

  // On a sphere, add matching nodes on the edge

  if ( helper1.IsRealSeam( edge1 ))
  {
    // sort nodes on edges by param on edge
    map< double, const SMDS_MeshNode* > u2nodesMaps[2];
    for ( int is2 = 0; is2 < 2; ++is2 )
    {
      TopoDS_Edge &     edge  = is2 ? edge2 : edge1;
      SMESHDS_Mesh *    smDS  = is2 ? meshDS2 : meshDS1;
      SMESHDS_SubMesh* edgeSM = smDS->MeshElements( edge );
      map< double, const SMDS_MeshNode* > & pos2nodes = u2nodesMaps[ is2 ];

      SMDS_NodeIteratorPtr nIt = edgeSM->GetNodes();
      while ( nIt->more() ) {
        const SMDS_MeshNode* node = nIt->next();
        const SMDS_EdgePosition* pos =
          static_cast<const SMDS_EdgePosition*>(node->GetPosition());
        pos2nodes.insert( make_pair( pos->GetUParameter(), node ));
      }
      if ( pos2nodes.size() != edgeSM->NbNodes() )
        RETURN_BAD_RESULT("Equal params of nodes on edge "
                          << smDS->ShapeToIndex( edge ) << " of face " << is2 );
    }
    if ( u2nodesMaps[0].size() != u2nodesMaps[1].size() )
      RETURN_BAD_RESULT("Different nb of new nodes on edges or wrong params");

    // compare edge orientation
    double u1 = helper1.GetNodeU( edge1, vNode1 );
    double u2 = helper2.GetNodeU( edge2, vNode2 );
    bool isFirst1 = ( u1 < u2nodesMaps[0].begin()->first );
    bool isFirst2 = ( u2 < u2nodesMaps[1].begin()->first );
    bool reverse ( isFirst1 != isFirst2 );

    // associate matching nodes
    map< double, const SMDS_MeshNode* >::iterator u_Node1, u_Node2, end1;
    map< double, const SMDS_MeshNode* >::reverse_iterator uR_Node2;
    u_Node1 = u2nodesMaps[0].begin();
    u_Node2 = u2nodesMaps[1].begin();
    uR_Node2 = u2nodesMaps[1].rbegin();
    end1 = u2nodesMaps[0].end();
    for ( ; u_Node1 != end1; ++u_Node1 ) {
      const SMDS_MeshNode* n1 = u_Node1->second;
      const SMDS_MeshNode* n2 = ( reverse ? (uR_Node2++)->second : (u_Node2++)->second );
      node1To2Map.insert( make_pair( n1, n2 ));
    }

    // associate matching nodes on the last vertices
    V2 = TopExp::LastVertex( TopoDS::Edge( edge2 ));
    if ( !assocMap.IsBound( V2, /*is2nd=*/true ))
      RETURN_BAD_RESULT("Association not found for vertex " << meshDS2->ShapeToIndex( V2 ));
    V1 = TopoDS::Vertex( assocMap( V2, /*is2nd=*/true ));
    vNode1 = SMESH_Algo::VertexNode( V1, meshDS1 );
    vNode2 = SMESH_Algo::VertexNode( V2, meshDS2 );
    if ( !vNode1 ) RETURN_BAD_RESULT("No node on vertex #" << meshDS1->ShapeToIndex( V1 ));
    if ( !vNode2 ) RETURN_BAD_RESULT("No node on vertex #" << meshDS2->ShapeToIndex( V2 ));
    node1To2Map.insert( make_pair( vNode1, vNode2 ));
  }

  // don't know why this condition is usually true :(
  //   if ( node1To2Map.size() * quadFactor < SM1->NbNodes() )
  //     MESSAGE("FindMatchingNodes() found too few node pairs starting from nodes ("
  //             << vNode1->GetID() << " - " << eNode1[0]->GetID() << ") ("
  //             << vNode2->GetID() << " - " << eNode2[0]->GetID() << "):"
  //             << node1To2Map.size() * quadFactor << " < " << SM1->NbNodes());

  return true;
}

//================================================================================
/*
 * Return any sub-shape of a face belonging to the outer wire
 *  \param face - the face
 *  \param type - type of sub-shape to return
 *  \retval TopoDS_Shape - the found sub-shape
 */
//================================================================================

TopoDS_Shape StdMeshers_ProjectionUtils::OuterShape( const TopoDS_Face& face,
                                                     TopAbs_ShapeEnum   type)
{
  TopExp_Explorer exp( BRepTools::OuterWire( face ), type );
  if ( exp.More() )
    return exp.Current();
  return TopoDS_Shape();
}

//================================================================================
/*
 * Check that sub-mesh is computed and try to compute it if is not
 *  \param sm - sub-mesh to compute
 *  \param iterationNb - int used to stop infinite recursive call
 *  \retval bool - true if computed
 */
//================================================================================

bool StdMeshers_ProjectionUtils::MakeComputed(SMESH_subMesh * sm, const int iterationNb)
{
  if ( iterationNb > 10 )
    RETURN_BAD_RESULT("Infinite recursive projection");
  if ( !sm )
    RETURN_BAD_RESULT("NULL submesh");
  if ( sm->IsMeshComputed() )
    return true;

  SMESH_Mesh*   mesh = sm->GetFather();
  SMESH_Gen*     gen = mesh->GetGen();
  SMESH_Algo*   algo = sm->GetAlgo();
  TopoDS_Shape shape = sm->GetSubShape();
  if ( !algo )
  {
    if ( shape.ShapeType() != TopAbs_COMPOUND )
    {
      // No algo assigned to a non-compound sub-mesh.
      // Try to find an all-dimensional algo of an upper dimension
      int dim = gen->GetShapeDim( shape );
      for ( ++dim; ( dim <= 3 && !algo ); ++dim )
      {
        SMESH_HypoFilter hypoFilter( SMESH_HypoFilter::IsAlgo() );
        hypoFilter.And( SMESH_HypoFilter::HasDim( dim ));
        list <const SMESHDS_Hypothesis * > hyps;
        list< TopoDS_Shape >               assignedTo;
        int nbAlgos =
          mesh->GetHypotheses( shape, hypoFilter, hyps, true, &assignedTo );
        if ( nbAlgos > 1 ) // concurrent algos
        {
          vector<SMESH_subMesh*> smList; // where an algo is assigned
          list< TopoDS_Shape >::iterator shapeIt = assignedTo.begin();
          for ( ; shapeIt != assignedTo.end(); ++shapeIt )
            smList.push_back( mesh->GetSubMesh( *shapeIt ));

          mesh->SortByMeshOrder( smList );
          algo  = smList.front()->GetAlgo();
          shape = smList.front()->GetSubShape();
        }
        else if ( nbAlgos == 1 )
        {
          algo = (SMESH_Algo*) hyps.front();
          shape = assignedTo.front();
        }
      }
      if ( !algo )
        return false;
    }
    else
    {
      // group
      bool computed = true;
      for ( TopoDS_Iterator grMember( shape ); grMember.More(); grMember.Next())
        if ( SMESH_subMesh* grSub = mesh->GetSubMesh( grMember.Value() ))
          if ( !MakeComputed( grSub, iterationNb + 1 ))
            computed = false;
      return computed;
    }
  }

  string algoType = algo->GetName();
  if ( algoType.substr(0, 11) != "Projection_")
    return gen->Compute( *mesh, shape, /*shapeOnly=*/true );

  // try to compute source mesh

  const list <const SMESHDS_Hypothesis *> & hyps =
    algo->GetUsedHypothesis( *mesh, shape );

  TopoDS_Shape srcShape;
  SMESH_Mesh* srcMesh = 0;
  list <const SMESHDS_Hypothesis*>::const_iterator hIt = hyps.begin();
  for ( ; srcShape.IsNull() && hIt != hyps.end(); ++hIt ) {
    string hypName = (*hIt)->GetName();
    if ( hypName == "ProjectionSource1D" ) {
      const StdMeshers_ProjectionSource1D * hyp =
        static_cast<const StdMeshers_ProjectionSource1D*>( *hIt );
      srcShape = hyp->GetSourceEdge();
      srcMesh = hyp->GetSourceMesh();
    }
    else if ( hypName == "ProjectionSource2D" ) {
      const StdMeshers_ProjectionSource2D * hyp =
        static_cast<const StdMeshers_ProjectionSource2D*>( *hIt );
      srcShape = hyp->GetSourceFace();
      srcMesh = hyp->GetSourceMesh();
    }
    else if ( hypName == "ProjectionSource3D" ) {
      const StdMeshers_ProjectionSource3D * hyp =
        static_cast<const StdMeshers_ProjectionSource3D*>( *hIt );
      srcShape = hyp->GetSource3DShape();
      srcMesh = hyp->GetSourceMesh();
    }
  }
  if ( srcShape.IsNull() ) // no projection source defined
    return gen->Compute( *mesh, shape, /*shapeOnly=*/true );

  if ( srcShape.IsSame( shape ))
    RETURN_BAD_RESULT("Projection from self");
    
  if ( !srcMesh )
    srcMesh = mesh;

  if ( MakeComputed( srcMesh->GetSubMesh( srcShape ), iterationNb + 1 ) &&
       gen->Compute( *mesh, shape, /*shapeOnly=*/true ))
    return sm->IsMeshComputed();

  return false;
}


//================================================================================
/*
 * Returns an error message to show in case if MakeComputed( sm ) fails.
 */
//================================================================================

std::string StdMeshers_ProjectionUtils::SourceNotComputedError( SMESH_subMesh * sm,
                                                                SMESH_Algo*     projAlgo )
{
  const char usualMessage [] = "Source mesh not computed";
  if ( !projAlgo )
    return usualMessage;
  if ( !sm || sm->GetAlgoState() != SMESH_subMesh::NO_ALGO )
    return usualMessage; // algo is OK, anything else is KO.

  // Try to find a type of all-dimentional algorithm that would compute the
  // given sub-mesh if it could be launched before projection
  const TopoDS_Shape shape = sm->GetSubShape();
  const int       shapeDim = SMESH_Gen::GetShapeDim( shape );

  for ( int dimIncrement = 1; shapeDim + dimIncrement < 4; ++dimIncrement )
  {
    SMESH_HypoFilter filter( SMESH_HypoFilter::IsAlgo() );
    filter.And( filter.HasDim( shapeDim + dimIncrement ));

    SMESH_Algo* algo = (SMESH_Algo*) sm->GetFather()->GetHypothesis( shape, filter, true );
    if ( algo && !algo->NeedDiscreteBoundary() )
      return SMESH_Comment("\"")
        << algo->GetFeatures()._label << "\""
        << " can't be used to compute the source mesh for \""
        << projAlgo->GetFeatures()._label << "\" in this case";
  }
  return usualMessage;
}

//================================================================================
/*
 * Return a boundary EDGE (or all boundary EDGEs) of edgeContainer
 */
//================================================================================

TopoDS_Edge
StdMeshers_ProjectionUtils::GetBoundaryEdge(const TopoDS_Shape&       edgeContainer,
                                            const SMESH_Mesh&         mesh,
                                            std::list< TopoDS_Edge >* allBndEdges)
{
  TopTools_IndexedMapOfShape facesOfEdgeContainer, facesNearEdge;
  TopExp::MapShapes( edgeContainer, TopAbs_FACE, facesOfEdgeContainer );

  if ( !facesOfEdgeContainer.IsEmpty() ) 
    for ( TopExp_Explorer exp(edgeContainer, TopAbs_EDGE); exp.More(); exp.Next() )
    {
      const TopoDS_Edge& edge = TopoDS::Edge( exp.Current() );
      facesNearEdge.Clear();
      PShapeIteratorPtr faceIt = SMESH_MesherHelper::GetAncestors( edge, mesh, TopAbs_FACE );
      while ( const TopoDS_Shape* face = faceIt->next() )
        if ( facesOfEdgeContainer.Contains( *face ))
          if ( facesNearEdge.Add( *face ) && facesNearEdge.Extent() > 1 )
            break;
      if ( facesNearEdge.Extent() == 1 ) {
        if ( allBndEdges )
          allBndEdges->push_back( edge );
        else
          return edge;
      }
    }

  return TopoDS_Edge();
}


namespace { // Definition of event listeners

  SMESH_subMeshEventListener* getSrcSubMeshListener();

  //================================================================================
  /*!
   * \brief Listener that resets an event listener on source submesh when 
   * "ProjectionSource*D" hypothesis is modified
   */
  //================================================================================

  struct HypModifWaiter: SMESH_subMeshEventListener
  {
    HypModifWaiter():SMESH_subMeshEventListener(false,// won't be deleted by submesh
                                                "StdMeshers_ProjectionUtils::HypModifWaiter") {}
    void ProcessEvent(const int event, const int eventType, SMESH_subMesh* subMesh,
                      EventListenerData*, const SMESH_Hypothesis*)
    {
      if ( event     == SMESH_subMesh::MODIF_HYP &&
           eventType == SMESH_subMesh::ALGO_EVENT)
      {
        // delete current source listener
        subMesh->DeleteEventListener( getSrcSubMeshListener() );
        // let algo set a new one
        if ( SMESH_Algo* algo = subMesh->GetAlgo() )
          algo->SetEventListener( subMesh );
      }
    }
  };
  //================================================================================
  /*!
   * \brief return static HypModifWaiter
   */
  //================================================================================

  SMESH_subMeshEventListener* getHypModifWaiter() {
    static HypModifWaiter aHypModifWaiter;
    return &aHypModifWaiter;
  }
  //================================================================================
  /*!
   * \brief return static listener for source shape submeshes
   */
  //================================================================================

  SMESH_subMeshEventListener* getSrcSubMeshListener() {
    static SMESH_subMeshEventListener srcListener(false, // won't be deleted by submesh
                                                  "StdMeshers_ProjectionUtils::SrcSubMeshListener");
    return &srcListener;
  }
}

//================================================================================
/*
 * Set event listeners to submesh with projection algo
 *  \param subMesh - submesh with projection algo
 *  \param srcShape - source shape
 *  \param srcMesh - source mesh
 */
//================================================================================

void StdMeshers_ProjectionUtils::SetEventListener(SMESH_subMesh* subMesh,
                                                  TopoDS_Shape   srcShape,
                                                  SMESH_Mesh*    srcMesh)
{
  // Set the listener that resets an event listener on source submesh when
  // "ProjectionSource*D" hypothesis is modified since source shape can be changed
  subMesh->SetEventListener( getHypModifWaiter(),0,subMesh);

  // Set an event listener to submesh of the source shape
  if ( !srcShape.IsNull() )
  {
    if ( !srcMesh )
      srcMesh = subMesh->GetFather();

    SMESH_subMesh* srcShapeSM = srcMesh->GetSubMesh( srcShape );

    if ( srcShapeSM != subMesh ) {
      if ( srcShapeSM->GetSubMeshDS() &&
           srcShapeSM->GetSubMeshDS()->IsComplexSubmesh() )
      {  // source shape is a group
        TopExp_Explorer it(srcShapeSM->GetSubShape(), // explore the group into sub-shapes...
                           subMesh->GetSubShape().ShapeType()); // ...of target shape type
        for (; it.More(); it.Next())
        {
          SMESH_subMesh* srcSM = srcMesh->GetSubMesh( it.Current() );
          if ( srcSM != subMesh )
          {
            SMESH_subMeshEventListenerData* data =
              srcSM->GetEventListenerData(getSrcSubMeshListener());
            if ( data )
              data->mySubMeshes.push_back( subMesh );
            else
              data = SMESH_subMeshEventListenerData::MakeData( subMesh );
            subMesh->SetEventListener ( getSrcSubMeshListener(), data, srcSM );
          }
        }
      }
      else
      {
        if ( SMESH_subMeshEventListenerData* data =
             srcShapeSM->GetEventListenerData( getSrcSubMeshListener() ))
        {
          bool alreadyIn =
            (std::find( data->mySubMeshes.begin(),
                        data->mySubMeshes.end(), subMesh ) != data->mySubMeshes.end() );
          if ( !alreadyIn )
            data->mySubMeshes.push_back( subMesh );
        }
        else
        {
          subMesh->SetEventListener( getSrcSubMeshListener(),
                                     SMESH_subMeshEventListenerData::MakeData( subMesh ),
                                     srcShapeSM );
        }
      }
    }
  }
}

namespace StdMeshers_ProjectionUtils
{

  //================================================================================
  /*!
   * \brief Computes transformation between two sets of 2D points using
   *        a least square approximation
   *
   * See "Surface Mesh Projection For Hexahedral Mesh Generation By Sweeping"
   * by X.Roca, J.Sarrate, A.Huerta. (2.2)
   */
  //================================================================================

  bool TrsfFinder2D::Solve( const vector< gp_XY >& srcPnts,
                            const vector< gp_XY >& tgtPnts )
  {
    // find gravity centers
    gp_XY srcGC( 0,0 ), tgtGC( 0,0 );
    for ( size_t i = 0; i < srcPnts.size(); ++i )
    {
      srcGC += srcPnts[i];
      tgtGC += tgtPnts[i];
    }
    srcGC /= srcPnts.size();
    tgtGC /= tgtPnts.size();

    // find trsf

    math_Matrix mat (1,4,1,4, 0.);
    math_Vector vec (1,4, 0.);

    // cout << "m1 = smesh.Mesh('src')" << endl
    //      << "m2 = smesh.Mesh('tgt')" << endl;
    double xx = 0, xy = 0, yy = 0;
    for ( size_t i = 0; i < srcPnts.size(); ++i )
    {
      gp_XY srcUV = srcPnts[i] - srcGC;
      gp_XY tgtUV = tgtPnts[i] - tgtGC;
      xx += srcUV.X() * srcUV.X();
      yy += srcUV.Y() * srcUV.Y();
      xy += srcUV.X() * srcUV.Y();
      vec( 1 ) += srcUV.X() * tgtUV.X();
      vec( 2 ) += srcUV.Y() * tgtUV.X();
      vec( 3 ) += srcUV.X() * tgtUV.Y();
      vec( 4 ) += srcUV.Y() * tgtUV.Y();
      // cout << "m1.AddNode( " << srcUV.X() << ", " << srcUV.Y() << ", 0 )" << endl
      //      << "m2.AddNode( " << tgtUV.X() << ", " << tgtUV.Y() << ", 0 )" << endl;
    }
    mat( 1,1 ) = mat( 3,3 ) = xx;
    mat( 2,2 ) = mat( 4,4 ) = yy;
    mat( 1,2 ) = mat( 2,1 ) = mat( 3,4 ) = mat( 4,3 ) = xy;

    math_Gauss solver( mat );
    if ( !solver.IsDone() )
      return false;
    solver.Solve( vec );
    if ( vec.Norm2() < gp::Resolution() )
      return false;
    // cout << vec( 1 ) << "\t " << vec( 2 ) << endl
    //      << vec( 3 ) << "\t " << vec( 4 ) << endl;

    _trsf.SetTranslationPart( tgtGC );
    _srcOrig = srcGC;

    gp_Mat2d& M = const_cast< gp_Mat2d& >( _trsf.VectorialPart());
    M( 1,1 ) = vec( 1 );
    M( 2,1 ) = vec( 2 ); // | 1 3 | -- is it correct ????????
    M( 1,2 ) = vec( 3 ); // | 2 4 |
    M( 2,2 ) = vec( 4 );

    return true;
  }

  //================================================================================
  /*!
   * \brief Transforms a 2D points using a found transformation
   */
  //================================================================================

  gp_XY TrsfFinder2D::Transform( const gp_Pnt2d& srcUV ) const
  {
    gp_XY uv = srcUV.XY() - _srcOrig ;
    _trsf.Transforms( uv );
    return uv;
  }

  //================================================================================
  /*!
   * \brief Computes transformation between two sets of 3D points using
   *        a least square approximation
   *
   * See "Surface Mesh Projection For Hexahedral Mesh Generation By Sweeping"
   * by X.Roca, J.Sarrate, A.Huerta. (2.4)
   */
  //================================================================================

  bool TrsfFinder3D::Solve( const vector< gp_XYZ > & srcPnts,
                            const vector< gp_XYZ > & tgtPnts )
  {
    // find gravity center
    gp_XYZ srcGC( 0,0,0 ), tgtGC( 0,0,0 );
    for ( size_t i = 0; i < srcPnts.size(); ++i )
    {
      srcGC += srcPnts[i];
      tgtGC += tgtPnts[i];
    }
    srcGC /= srcPnts.size();
    tgtGC /= tgtPnts.size();

    gp_XYZ srcOrig = 2 * srcGC - tgtGC;
    gp_XYZ tgtOrig = srcGC;

    // find trsf

    math_Matrix mat (1,9,1,9, 0.);
    math_Vector vec (1,9, 0.);

    double xx = 0, yy = 0, zz = 0;
    double xy = 0, xz = 0, yz = 0;
    for ( size_t i = 0; i < srcPnts.size(); ++i )
    {
      gp_XYZ src = srcPnts[i] - srcOrig;
      gp_XYZ tgt = tgtPnts[i] - tgtOrig;
      xx += src.X() * src.X();
      yy += src.Y() * src.Y();
      zz += src.Z() * src.Z();
      xy += src.X() * src.Y();
      xz += src.X() * src.Z();
      yz += src.Y() * src.Z();
      vec( 1 ) += src.X() * tgt.X();
      vec( 2 ) += src.Y() * tgt.X();
      vec( 3 ) += src.Z() * tgt.X();
      vec( 4 ) += src.X() * tgt.Y();
      vec( 5 ) += src.Y() * tgt.Y();
      vec( 6 ) += src.Z() * tgt.Y();
      vec( 7 ) += src.X() * tgt.Z();
      vec( 8 ) += src.Y() * tgt.Z();
      vec( 9 ) += src.Z() * tgt.Z();
    }
    mat( 1,1 ) = mat( 4,4 ) = mat( 7,7 ) = xx;
    mat( 2,2 ) = mat( 5,5 ) = mat( 8,8 ) = yy;
    mat( 3,3 ) = mat( 6,6 ) = mat( 9,9 ) = zz;
    mat( 1,2 ) = mat( 2,1 ) = mat( 4,5 ) = mat( 5,4 ) = mat( 7,8 ) = mat( 8,7 ) = xy;
    mat( 1,3 ) = mat( 3,1 ) = mat( 4,6 ) = mat( 6,4 ) = mat( 7,9 ) = mat( 9,7 ) = xz;
    mat( 2,3 ) = mat( 3,2 ) = mat( 5,6 ) = mat( 6,5 ) = mat( 8,9 ) = mat( 9,8 ) = yz;

    math_Gauss solver( mat );
    if ( !solver.IsDone() )
      return false;
    solver.Solve( vec );
    if ( vec.Norm2() < gp::Resolution() )
      return false;
    // cout << endl
    //      << vec( 1 ) << "\t " << vec( 2 ) << "\t " << vec( 3 ) << endl
    //      << vec( 4 ) << "\t " << vec( 5 ) << "\t " << vec( 6 ) << endl
    //      << vec( 7 ) << "\t " << vec( 8 ) << "\t " << vec( 9 ) << endl;

    _srcOrig = srcOrig;
    _trsf.SetTranslationPart( tgtOrig );

    gp_Mat& M = const_cast< gp_Mat& >( _trsf.VectorialPart() );
    M.SetRows( gp_XYZ( vec( 1 ), vec( 2 ), vec( 3 )),
               gp_XYZ( vec( 4 ), vec( 5 ), vec( 6 )),
               gp_XYZ( vec( 7 ), vec( 8 ), vec( 9 )));
    return true;
  }

  //================================================================================
  /*!
   * \brief Transforms a 3D point using a found transformation
   */
  //================================================================================

  gp_XYZ TrsfFinder3D::Transform( const gp_Pnt& srcP ) const
  {
    gp_XYZ p = srcP.XYZ() - _srcOrig;
    _trsf.Transforms( p );
    return p;
  }

  //================================================================================
  /*!
   * \brief Transforms a 3D vector using a found transformation
   */
  //================================================================================

  gp_XYZ TrsfFinder3D::TransformVec( const gp_Vec& v ) const
  {
    return v.XYZ().Multiplied( _trsf.VectorialPart() );
  }
  //================================================================================
  /*!
   * \brief Inversion
   */
  //================================================================================

  bool TrsfFinder3D::Invert()
  {
    if (( _trsf.Form() == gp_Translation ) &&
        ( _srcOrig.X() != 0 || _srcOrig.Y() != 0 || _srcOrig.Z() != 0 ))
    {
      // seems to be defined via Solve()
      gp_XYZ newSrcOrig = _trsf.TranslationPart();
      gp_Mat& M = const_cast< gp_Mat& >( _trsf.VectorialPart() );
      const double D = M.Determinant();
      if ( D < 1e-3 * ( newSrcOrig - _srcOrig ).Modulus() )
      {
#ifdef _DEBUG_
        cerr << "TrsfFinder3D::Invert()"
             << "D " << M.Determinant() << " IsSingular " << M.IsSingular() << endl;
#endif
        return false;
      }
      gp_Mat Minv = M.Inverted();
      _trsf.SetTranslationPart( _srcOrig );
      _srcOrig = newSrcOrig;
      M = Minv;
    }
    else
    {
      _trsf.Invert();
    }
    return true;
  }
}

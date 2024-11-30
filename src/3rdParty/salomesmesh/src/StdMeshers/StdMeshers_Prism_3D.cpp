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

// File      : StdMeshers_Prism_3D.cxx
// Module    : SMESH
// Created   : Fri Oct 20 11:37:07 2006
// Author    : Edward AGAPOV (eap)
//
#include "StdMeshers_Prism_3D.hxx"

#include "SMDS_EdgePosition.hxx"
#include "SMDS_VolumeOfNodes.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_MesherHelper.hxx"
#include "StdMeshers_FaceSide.hxx"
#include "StdMeshers_ProjectionSource1D.hxx"
#include "StdMeshers_ProjectionSource2D.hxx"
#include "StdMeshers_ProjectionUtils.hxx"
#include "StdMeshers_Projection_1D.hxx"
#include "StdMeshers_Projection_1D2D.hxx"
#include "StdMeshers_Quadrangle_2D.hxx"

#include "utilities.h"

#include <BRepAdaptor_CompCurve.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_B3d.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom2d_Line.hxx>
#include <GeomLib_IsPlanarSurface.hxx>
#include <Geom_Curve.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopTools_SequenceOfShape.hxx>
#include <TopoDS.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>

#include <limits>

using namespace std;

#define RETURN_BAD_RESULT(msg) { MESSAGE(")-: Error: " << msg); return false; }
#define gpXYZ(n) SMESH_TNodeXYZ(n)

#ifdef _DEBUG_
#define DBGOUT(msg) //cout << msg << endl;
#define SHOWYXZ(msg, xyz)                                               \
  // { gp_Pnt p (xyz);                                                     \
  //   cout << msg << " ("<< p.X() << "; " <<p.Y() << "; " <<p.Z() << ") " <<endl; }
#else
#define DBGOUT(msg)
#define SHOWYXZ(msg, xyz)
#endif

namespace NSProjUtils = StdMeshers_ProjectionUtils;

typedef SMESH_Comment TCom;

enum { ID_BOT_FACE = SMESH_Block::ID_Fxy0,
       ID_TOP_FACE = SMESH_Block::ID_Fxy1,
       BOTTOM_EDGE = 0, TOP_EDGE, V0_EDGE, V1_EDGE, // edge IDs in face
       NB_WALL_FACES = 4 }; //

namespace {

  //=======================================================================
  /*!
   * \brief Quadrangle algorithm
   */
  struct TQuadrangleAlgo : public StdMeshers_Quadrangle_2D
  {
    TQuadrangleAlgo(int studyId, SMESH_Gen* gen)
      : StdMeshers_Quadrangle_2D( gen->GetANewId(), studyId, gen)
    {
    }
    static StdMeshers_Quadrangle_2D* instance( SMESH_Algo*         fatherAlgo,
                                               SMESH_MesherHelper* helper=0)
    {
      static TQuadrangleAlgo* algo = new TQuadrangleAlgo( fatherAlgo->GetStudyId(),
                                                          fatherAlgo->GetGen() );
      if ( helper &&
           algo->myProxyMesh &&
           algo->myProxyMesh->GetMesh() != helper->GetMesh() )
        algo->myProxyMesh.reset( new SMESH_ProxyMesh( *helper->GetMesh() ));

      algo->myQuadList.clear();

      if ( helper )
        algo->_quadraticMesh = helper->GetIsQuadratic();

      return algo;
    }
  };
  //=======================================================================
  /*!
   * \brief Algorithm projecting 1D mesh
   */
  struct TProjction1dAlgo : public StdMeshers_Projection_1D
  {
    StdMeshers_ProjectionSource1D myHyp;

    TProjction1dAlgo(int studyId, SMESH_Gen* gen)
      : StdMeshers_Projection_1D( gen->GetANewId(), studyId, gen),
        myHyp( gen->GetANewId(), studyId, gen)
    {
      StdMeshers_Projection_1D::_sourceHypo = & myHyp;
    }
    static TProjction1dAlgo* instance( SMESH_Algo* fatherAlgo )
    {
      static TProjction1dAlgo* algo = new TProjction1dAlgo( fatherAlgo->GetStudyId(),
                                                            fatherAlgo->GetGen() );
      return algo;
    }
  };
  //=======================================================================
  /*!
   * \brief Algorithm projecting 2D mesh
   */
  struct TProjction2dAlgo : public StdMeshers_Projection_1D2D
  {
    StdMeshers_ProjectionSource2D myHyp;

    TProjction2dAlgo(int studyId, SMESH_Gen* gen)
      : StdMeshers_Projection_1D2D( gen->GetANewId(), studyId, gen),
        myHyp( gen->GetANewId(), studyId, gen)
    {
      StdMeshers_Projection_2D::_sourceHypo = & myHyp;
    }
    static TProjction2dAlgo* instance( SMESH_Algo* fatherAlgo )
    {
      static TProjction2dAlgo* algo = new TProjction2dAlgo( fatherAlgo->GetStudyId(),
                                                            fatherAlgo->GetGen() );
      return algo;
    }
    const NSProjUtils::TNodeNodeMap& GetNodesMap()
    {
      return _src2tgtNodes;
    }
  };
  //=======================================================================
  /*!
   * \brief Returns already computed EDGEs
   */
  void getPrecomputedEdges( SMESH_MesherHelper&    theHelper,
                            const TopoDS_Shape&    theShape,
                            vector< TopoDS_Edge >& theEdges)
  {
    theEdges.clear();

    SMESHDS_Mesh* meshDS = theHelper.GetMeshDS();
    SMESHDS_SubMesh* sm;

    TopTools_IndexedMapOfShape edges;
    TopExp::MapShapes( theShape, TopAbs_EDGE, edges );
    for ( int iE = 1; iE <= edges.Extent(); ++iE )
    {
      const TopoDS_Shape edge = edges( iE );
      if (( ! ( sm = meshDS->MeshElements( edge ))) ||
          ( sm->NbElements() == 0 ))
        continue;

      // there must not be FACEs meshed with triangles and sharing a computed EDGE
      // as the precomputed EDGEs are used for propagation other to 'vertical' EDGEs
      bool faceFound = false;
      PShapeIteratorPtr faceIt =
        theHelper.GetAncestors( edge, *theHelper.GetMesh(), TopAbs_FACE );
      while ( const TopoDS_Shape* face = faceIt->next() )

        if (( sm = meshDS->MeshElements( *face )) &&
            ( sm->NbElements() > 0 ) &&
            ( !theHelper.IsSameElemGeometry( sm, SMDSGeom_QUADRANGLE ) ))
        {
          faceFound = true;
          break;
        }
      if ( !faceFound )
        theEdges.push_back( TopoDS::Edge( edge ));
    }
  }

  //================================================================================
  /*!
   * \brief Make \a botE be the BOTTOM_SIDE of \a quad.
   *        Return false if the BOTTOM_SIDE is composite
   */
  //================================================================================

  bool setBottomEdge( const TopoDS_Edge&   botE,
                      FaceQuadStruct::Ptr& quad,
                      const TopoDS_Shape&  face)
  {
    quad->side[ QUAD_TOP_SIDE  ].grid->Reverse();
    quad->side[ QUAD_LEFT_SIDE ].grid->Reverse();
    int edgeIndex = 0;
    bool isComposite = false;
    for ( size_t i = 0; i < quad->side.size(); ++i )
    {
      StdMeshers_FaceSidePtr quadSide = quad->side[i];
      for ( int iE = 0; iE < quadSide->NbEdges(); ++iE )
        if ( botE.IsSame( quadSide->Edge( iE )))
        {
          if ( quadSide->NbEdges() > 1 )
            isComposite = true; //return false;
          edgeIndex = i;
          i = quad->side.size(); // to quit from the outer loop
          break;
        }
    }
    if ( edgeIndex != QUAD_BOTTOM_SIDE )
      quad->shift( quad->side.size() - edgeIndex, /*keepUnitOri=*/false );

    quad->face = TopoDS::Face( face );

    return !isComposite;
  }

  //================================================================================
  /*!
   * \brief Return iterator pointing to node column for the given parameter
   * \param columnsMap - node column map
   * \param parameter - parameter
   * \retval TParam2ColumnMap::iterator - result
   *
   * it returns closest left column
   */
  //================================================================================

  TParam2ColumnIt getColumn( const TParam2ColumnMap* columnsMap,
                             const double            parameter )
  {
    TParam2ColumnIt u_col = columnsMap->upper_bound( parameter );
    if ( u_col != columnsMap->begin() )
      --u_col;
    return u_col; // return left column
  }

  //================================================================================
  /*!
   * \brief Return nodes around given parameter and a ratio
   * \param column - node column
   * \param param - parameter
   * \param node1 - lower node
   * \param node2 - upper node
   * \retval double - ratio
   */
  //================================================================================

  double getRAndNodes( const TNodeColumn*     column,
                       const double           param,
                       const SMDS_MeshNode* & node1,
                       const SMDS_MeshNode* & node2)
  {
    if ( param >= 1.0 || column->size() == 1) {
      node1 = node2 = column->back();
      return 0;
    }

    int i = int( param * ( column->size() - 1 ));
    double u0 = double( i )/ double( column->size() - 1 );
    double r = ( param - u0 ) * ( column->size() - 1 );

    node1 = (*column)[ i ];
    node2 = (*column)[ i + 1];
    return r;
  }

  //================================================================================
  /*!
   * \brief Compute boundary parameters of face parts
    * \param nbParts - nb of parts to split columns into
    * \param columnsMap - node columns of the face to split
    * \param params - computed parameters
   */
  //================================================================================

  void splitParams( const int               nbParts,
                    const TParam2ColumnMap* columnsMap,
                    vector< double > &      params)
  {
    params.clear();
    params.reserve( nbParts + 1 );
    TParam2ColumnIt last_par_col = --columnsMap->end();
    double par = columnsMap->begin()->first; // 0.
    double parLast = last_par_col->first;
    params.push_back( par );
    for ( int i = 0; i < nbParts - 1; ++ i )
    {
      double partSize = ( parLast - par ) / double ( nbParts - i );
      TParam2ColumnIt par_col = getColumn( columnsMap, par + partSize );
      if ( par_col->first == par ) {
        ++par_col;
        if ( par_col == last_par_col ) {
          while ( i < nbParts - 1 )
            params.push_back( par + partSize * i++ );
          break;
        }
      }
      par = par_col->first;
      params.push_back( par );
    }
    params.push_back( parLast ); // 1.
  }

  //================================================================================
  /*!
   * \brief Return coordinate system for z-th layer of nodes
   */
  //================================================================================

  gp_Ax2 getLayerCoordSys(const int                           z,
                          const vector< const TNodeColumn* >& columns,
                          int&                                xColumn)
  {
    // gravity center of a layer
    gp_XYZ O(0,0,0);
    int vertexCol = -1;
    for ( int i = 0; i < columns.size(); ++i )
    {
      O += gpXYZ( (*columns[ i ])[ z ]);
      if ( vertexCol < 0 &&
           columns[ i ]->front()->GetPosition()->GetTypeOfPosition() == SMDS_TOP_VERTEX )
        vertexCol = i;
    }
    O /= columns.size();

    // Z axis
    gp_Vec Z(0,0,0);
    int iPrev = columns.size()-1;
    for ( int i = 0; i < columns.size(); ++i )
    {
      gp_Vec v1( O, gpXYZ( (*columns[ iPrev ])[ z ]));
      gp_Vec v2( O, gpXYZ( (*columns[ i ]    )[ z ]));
      Z += v1 ^ v2;
      iPrev = i;
    }

    if ( vertexCol >= 0 )
    {
      O = gpXYZ( (*columns[ vertexCol ])[ z ]);
    }
    if ( xColumn < 0 || xColumn >= columns.size() )
    {
      // select a column for X dir
      double maxDist = 0;
      for ( int i = 0; i < columns.size(); ++i )
      {
        double dist = ( O - gpXYZ((*columns[ i ])[ z ])).SquareModulus();
        if ( dist > maxDist )
        {
          xColumn = i;
          maxDist = dist;
        }
      }
    }

    // X axis
    gp_Vec X( O, gpXYZ( (*columns[ xColumn ])[ z ]));

    return gp_Ax2( O, Z, X);
  }

  //================================================================================
  /*!
   * \brief Removes submeshes that are or can be meshed with regular grid from given list
   *  \retval int - nb of removed submeshes
   */
  //================================================================================

  int removeQuasiQuads(list< SMESH_subMesh* >&   notQuadSubMesh,
                       SMESH_MesherHelper*       helper,
                       StdMeshers_Quadrangle_2D* quadAlgo)
  {
    int nbRemoved = 0;
    //SMESHDS_Mesh* mesh = notQuadSubMesh.front()->GetFather()->GetMeshDS();
    list< SMESH_subMesh* >::iterator smIt = notQuadSubMesh.begin();
    while ( smIt != notQuadSubMesh.end() )
    {
      SMESH_subMesh* faceSm = *smIt;
      SMESHDS_SubMesh* faceSmDS = faceSm->GetSubMeshDS();
      int nbQuads = faceSmDS ? faceSmDS->NbElements() : 0;
      bool toRemove;
      if ( nbQuads > 0 )
        toRemove = helper->IsStructured( faceSm );
      else
        toRemove = (bool)quadAlgo->CheckNbEdges( *helper->GetMesh(),
                                           faceSm->GetSubShape() );
      nbRemoved += toRemove;
      if ( toRemove )
        smIt = notQuadSubMesh.erase( smIt );
      else
        ++smIt;
    }

    return nbRemoved;
  }

  //================================================================================
  /*!
   * \brief Return and angle between two EDGEs
   *  \return double - the angle normalized so that
   * >~ 0  -> 2.0
   *  PI/2 -> 1.0
   *  PI   -> 0.0
   * -PI/2 -> -1.0
   * <~ 0  -> -2.0
   */
  //================================================================================

  // double normAngle(const TopoDS_Edge & E1, const TopoDS_Edge & E2, const TopoDS_Face & F)
  // {
  //   return SMESH_MesherHelper::GetAngle( E1, E2, F ) / ( 0.5 * M_PI );
  // }

  //================================================================================
  /*!
   * Consider continuous straight EDGES as one side - mark them to unite
   */
  //================================================================================

  int countNbSides( const Prism_3D::TPrismTopo & thePrism,
                    vector<int> &                nbUnitePerEdge,
                    vector< double > &           edgeLength)
  {
    int nbEdges = thePrism.myNbEdgesInWires.front();  // nb outer edges
    int nbSides = nbEdges;

    
    list< TopoDS_Edge >::const_iterator edgeIt = thePrism.myBottomEdges.begin();
    std::advance( edgeIt, nbEdges-1 );
    TopoDS_Edge   prevE = *edgeIt;
    // bool isPrevStraight = SMESH_Algo::IsStraight( prevE );
    int           iPrev = nbEdges - 1;

    int iUnite = -1; // the first of united EDGEs

    // analyse angles between EDGEs
    int nbCorners = 0;
    vector< bool > isCorner( nbEdges );
    edgeIt = thePrism.myBottomEdges.begin();
    for ( int iE = 0; iE < nbEdges; ++iE, ++edgeIt )
    {
      const TopoDS_Edge&  curE = *edgeIt;
      edgeLength[ iE ] = SMESH_Algo::EdgeLength( curE );

      // double normAngle = normAngle( prevE, curE, thePrism.myBottom );
      // isCorner[ iE ] = false;
      // if ( normAngle < 2.0 )
      // {
      //   if ( normAngle < 0.001 ) // straight or obtuse angle
      //   {
      //     // unite EDGEs in order not to put a corner of the unit quadrangle at this VERTEX
      //     if ( iUnite < 0 )
      //       iUnite = iPrev;
      //     nbUnitePerEdge[ iUnite ]++;
      //     nbUnitePerEdge[ iE ] = -1;
      //     --nbSides;
      //   }
      //   else
      //   {
      //     isCorner[ iE ] = true;
      //     nbCorners++;
      //     iUnite = -1;
      //   }
      // }
      // prevE = curE;
    }

    if ( nbCorners > 4 )
    {
      // define which of corners to put on a side of the unit quadrangle
    }
    // edgeIt = thePrism.myBottomEdges.begin();
    // for ( int iE = 0; iE < nbEdges; ++iE, ++edgeIt )
    // {
    //   const TopoDS_Edge&  curE = *edgeIt;
    //   edgeLength[ iE ] = SMESH_Algo::EdgeLength( curE );

    //   const bool isCurStraight = SMESH_Algo::IsStraight( curE );
    //   if ( isPrevStraight && isCurStraight && SMESH_Algo::IsContinuous( prevE, curE ))
    //   {
    //     if ( iUnite < 0 )
    //       iUnite = iPrev;
    //     nbUnitePerEdge[ iUnite ]++;
    //     nbUnitePerEdge[ iE ] = -1;
    //     --nbSides;
    //   }
    //   else
    //   {
    //     iUnite = -1;
    //   }
    //   prevE          = curE;
    //   isPrevStraight = isCurStraight;
    //   iPrev = iE;
    // }
    
    return nbSides;
  }

  inline void pointsToPython(const std::vector<gp_XYZ>& p)
  {
#ifdef _DEBUG_
    for ( int i = SMESH_Block::ID_V000; i < p.size(); ++i )
    {
      cout << "mesh.AddNode( " << p[i].X() << ", "<< p[i].Y() << ", "<< p[i].Z() << ") # " << i <<" " ;
      SMESH_Block::DumpShapeID( i, cout ) << endl;
    }
#endif
  }
} // namespace

//=======================================================================
//function : StdMeshers_Prism_3D
//purpose  : 
//=======================================================================

StdMeshers_Prism_3D::StdMeshers_Prism_3D(int hypId, int studyId, SMESH_Gen* gen)
  :SMESH_3D_Algo(hypId, studyId, gen)
{
  _name                    = "Prism_3D";
  _shapeType               = (1 << TopAbs_SOLID); // 1 bit per shape type
  _onlyUnaryInput          = false; // mesh all SOLIDs at once
  _requireDiscreteBoundary = false; // mesh FACEs and EDGEs by myself
  _supportSubmeshes        = true;  // "source" FACE must be meshed by other algo
  _neededLowerHyps[ 1 ]    = true;  // suppress warning on hiding a global 1D algo
  _neededLowerHyps[ 2 ]    = true;  // suppress warning on hiding a global 2D algo

  //myProjectTriangles       = false;
  mySetErrorToSM           = true;  // to pass an error to a sub-mesh of a current solid or not
}

//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

StdMeshers_Prism_3D::~StdMeshers_Prism_3D()
{}

//=======================================================================
//function : CheckHypothesis
//purpose  : 
//=======================================================================

bool StdMeshers_Prism_3D::CheckHypothesis(SMESH_Mesh&                          aMesh,
                                          const TopoDS_Shape&                  aShape,
                                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  // Check shape geometry
/*  PAL16229
  aStatus = SMESH_Hypothesis::HYP_BAD_GEOMETRY;

  // find not quadrangle faces
  list< TopoDS_Shape > notQuadFaces;
  int nbEdge, nbWire, nbFace = 0;
  TopExp_Explorer exp( aShape, TopAbs_FACE );
  for ( ; exp.More(); exp.Next() ) {
    ++nbFace;
    const TopoDS_Shape& face = exp.Current();
    nbEdge = NSProjUtils::Count( face, TopAbs_EDGE, 0 );
    nbWire = NSProjUtils::Count( face, TopAbs_WIRE, 0 );
    if (  nbEdge!= 4 || nbWire!= 1 ) {
      if ( !notQuadFaces.empty() ) {
        if ( NSProjUtils::Count( notQuadFaces.back(), TopAbs_EDGE, 0 ) != nbEdge ||
             NSProjUtils::Count( notQuadFaces.back(), TopAbs_WIRE, 0 ) != nbWire )
          RETURN_BAD_RESULT("Different not quad faces");
      }
      notQuadFaces.push_back( face );
    }
  }
  if ( !notQuadFaces.empty() )
  {
    if ( notQuadFaces.size() != 2 )
      RETURN_BAD_RESULT("Bad nb not quad faces: " << notQuadFaces.size());

    // check total nb faces
    nbEdge = NSProjUtils::Count( notQuadFaces.back(), TopAbs_EDGE, 0 );
    if ( nbFace != nbEdge + 2 )
      RETURN_BAD_RESULT("Bad nb of faces: " << nbFace << " but must be " << nbEdge + 2);
  }
*/
  // no hypothesis
  aStatus = SMESH_Hypothesis::HYP_OK;
  return true;
}

//=======================================================================
//function : Compute
//purpose  : Compute mesh on a COMPOUND of SOLIDs
//=======================================================================

bool StdMeshers_Prism_3D::Compute(SMESH_Mesh& theMesh, const TopoDS_Shape& theShape)
{
  SMESH_MesherHelper helper( theMesh );
  myHelper = &helper;

  int nbSolids = helper.Count( theShape, TopAbs_SOLID, /*skipSame=*/false );
  if ( nbSolids < 1 )
    return true;

  TopTools_IndexedDataMapOfShapeListOfShape faceToSolids;
  TopExp::MapShapesAndAncestors( theShape, TopAbs_FACE, TopAbs_SOLID, faceToSolids );

  // look for meshed FACEs ("source" FACEs) that must be prism bottoms
  list< TopoDS_Face > meshedFaces, notQuadMeshedFaces, notQuadFaces;
  const bool meshHasQuads = ( theMesh.NbQuadrangles() > 0 );
  //StdMeshers_Quadrangle_2D* quadAlgo = TQuadrangleAlgo::instance( this );
  for ( int iF = 1; iF <= faceToSolids.Extent(); ++iF )
  {
    const TopoDS_Face& face = TopoDS::Face( faceToSolids.FindKey( iF ));
    SMESH_subMesh*   faceSM = theMesh.GetSubMesh( face );
    if ( !faceSM->IsEmpty() )
    {
      if ( !meshHasQuads ||
           !helper.IsSameElemGeometry( faceSM->GetSubMeshDS(), SMDSGeom_QUADRANGLE ) ||
           !helper.IsStructured( faceSM )
           )
        notQuadMeshedFaces.push_front( face );
      else if ( myHelper->Count( face, TopAbs_EDGE, /*ignoreSame=*/false ) != 4 )
        meshedFaces.push_front( face );
      else
        meshedFaces.push_back( face );
    }
    // not add not quadrilateral FACE as we can't compute it
    // else if ( !quadAlgo->CheckNbEdges( theMesh, face ))
    // // not add not quadrilateral FACE as it can be a prism side
    // // else if ( myHelper->Count( face, TopAbs_EDGE, /*ignoreSame=*/false ) != 4 )
    // {
    //   notQuadFaces.push_back( face );
    // }
  }
  // notQuadFaces are of medium priority, put them before ordinary meshed faces
  meshedFaces.splice( meshedFaces.begin(), notQuadFaces );
  // notQuadMeshedFaces are of highest priority, put them before notQuadFaces
  meshedFaces.splice( meshedFaces.begin(), notQuadMeshedFaces );

  Prism_3D::TPrismTopo prism;
  myPropagChains = 0;
  bool selectBottom = meshedFaces.empty();

  if ( nbSolids == 1 )
  {
    TopoDS_Shape solid = TopExp_Explorer( theShape, TopAbs_SOLID ).Current();
    if ( !meshedFaces.empty() )
      prism.myBottom = meshedFaces.front();
    return ( initPrism( prism, solid, selectBottom ) &&
             compute( prism ));
  }

  // find propagation chains from already computed EDGEs
  vector< TopoDS_Edge > computedEdges;
  getPrecomputedEdges( helper, theShape, computedEdges );
  myPropagChains = new TopTools_IndexedMapOfShape[ computedEdges.size() + 1 ];
  SMESHUtils::ArrayDeleter< TopTools_IndexedMapOfShape > pcDel( myPropagChains );
  for ( size_t i = 0, nb = 0; i < computedEdges.size(); ++i )
  {
    StdMeshers_ProjectionUtils::GetPropagationEdge( &theMesh, TopoDS_Edge(),
                                                    computedEdges[i], myPropagChains + nb );
    if ( myPropagChains[ nb ].Extent() < 2 ) // an empty map is a termination sign
      myPropagChains[ nb ].Clear();
    else
      nb++;
  }

  TopTools_MapOfShape meshedSolids;
  list< Prism_3D::TPrismTopo > meshedPrism;
  list< TopoDS_Face > suspectSourceFaces;
  TopTools_ListIteratorOfListOfShape solidIt;

  while ( meshedSolids.Extent() < nbSolids )
  {
    if ( _computeCanceled )
      return toSM( error( SMESH_ComputeError::New(COMPERR_CANCELED)));

    // compute prisms having avident computed source FACE
    while ( !meshedFaces.empty() )
    {
      TopoDS_Face face = meshedFaces.front();
      meshedFaces.pop_front();
      TopTools_ListOfShape& solidList = faceToSolids.ChangeFromKey( face );
      while ( !solidList.IsEmpty() )
      {
        TopoDS_Shape solid = solidList.First();
        solidList.RemoveFirst();
        if ( meshedSolids.Add( solid ))
        {
          prism.Clear();
          prism.myBottom = face;
          if ( !initPrism( prism, solid, selectBottom ) ||
               !compute( prism ))
            return false;

          SMESHDS_SubMesh* smDS = theMesh.GetMeshDS()->MeshElements( prism.myTop );
          if ( !myHelper->IsSameElemGeometry( smDS, SMDSGeom_QUADRANGLE ))
          {
            meshedFaces.push_front( prism.myTop );
          }
          else
          {
            suspectSourceFaces.push_back( prism.myTop );
          }
          meshedPrism.push_back( prism );
        }
      }
    }
    if ( meshedSolids.Extent() == nbSolids )
      break;

    // below in the loop we try to find source FACEs somehow

    // project mesh from source FACEs of computed prisms to
    // prisms sharing wall FACEs
    list< Prism_3D::TPrismTopo >::iterator prismIt = meshedPrism.begin();
    for ( ; prismIt != meshedPrism.end(); ++prismIt )
    {
      for ( size_t iW = 0; iW < prismIt->myWallQuads.size(); ++iW )
      {
        Prism_3D::TQuadList::iterator wQuad = prismIt->myWallQuads[iW].begin();
        for ( ; wQuad != prismIt->myWallQuads[iW].end(); ++ wQuad )
        {
          const TopoDS_Face& wFace = (*wQuad)->face;
          TopTools_ListOfShape& solidList = faceToSolids.ChangeFromKey( wFace );
          solidIt.Initialize( solidList );
          while ( solidIt.More() )
          {
            const TopoDS_Shape& solid = solidIt.Value();
            if ( meshedSolids.Contains( solid )) {
              solidList.Remove( solidIt );
              continue; // already computed prism
            }
            if ( myHelper->IsBlock( solid )) {
              solidIt.Next();
              continue; // too trivial
            }
            // find a source FACE of the SOLID: it's a FACE sharing a bottom EDGE with wFace
            const TopoDS_Edge& wEdge = (*wQuad)->side[ QUAD_TOP_SIDE ].grid->Edge(0);
            PShapeIteratorPtr faceIt = myHelper->GetAncestors( wEdge, *myHelper->GetMesh(),
                                                               TopAbs_FACE);
            while ( const TopoDS_Shape* f = faceIt->next() )
            {
              const TopoDS_Face& candidateF = TopoDS::Face( *f );
              if ( candidateF.IsSame( wFace )) continue;
              // select a source FACE: prismIt->myBottom or prismIt->myTop
              TopoDS_Face sourceF = prismIt->myBottom;
              for ( TopExp_Explorer v( prismIt->myTop, TopAbs_VERTEX ); v.More(); v.Next() )
                if ( myHelper->IsSubShape( v.Current(), candidateF )) {
                  sourceF = prismIt->myTop;
                  break;
                }
              prism.Clear();
              prism.myBottom = candidateF;
              mySetErrorToSM = false;
              if ( !myHelper->IsSubShape( candidateF, prismIt->myShape3D ) &&
                   myHelper ->IsSubShape( candidateF, solid ) &&
                   !myHelper->GetMesh()->GetSubMesh( candidateF )->IsMeshComputed() &&
                   initPrism( prism, solid, /*selectBottom=*/false ) &&
                   !myHelper->GetMesh()->GetSubMesh( prism.myTop )->IsMeshComputed() &&
                   !myHelper->GetMesh()->GetSubMesh( prism.myBottom )->IsMeshComputed() &&
                   project2dMesh( sourceF, prism.myBottom ))
              {
                mySetErrorToSM = true;
                if ( !compute( prism ))
                  return false;
                SMESHDS_SubMesh* smDS = theMesh.GetMeshDS()->MeshElements( prism.myTop );
                if ( !myHelper->IsSameElemGeometry( smDS, SMDSGeom_QUADRANGLE ))
                {
                  meshedFaces.push_front( prism.myTop );
                  meshedFaces.push_front( prism.myBottom );
                  selectBottom = false;
                }
                meshedPrism.push_back( prism );
                meshedSolids.Add( solid );
              }
              InitComputeError();
            }
            mySetErrorToSM = true;
            InitComputeError();
            if ( meshedSolids.Contains( solid ))
              solidList.Remove( solidIt );
            else
              solidIt.Next();
          }
        }
      }
      if ( !meshedFaces.empty() )
        break; // to compute prisms with avident sources
    }

    if ( meshedFaces.empty() )
    {
      meshedFaces.splice( meshedFaces.end(), suspectSourceFaces );
      selectBottom = true;
    }

    // find FACEs with local 1D hyps, which has to be computed by now,
    // or at least any computed FACEs
    if ( meshedFaces.empty() )
    {
      int prevNbFaces = 0;
      for ( int iF = 1; iF <= faceToSolids.Extent(); ++iF )
      {
        const TopoDS_Face&               face = TopoDS::Face( faceToSolids.FindKey( iF ));
        const TopTools_ListOfShape& solidList = faceToSolids.FindFromKey( face );
        if ( solidList.IsEmpty() ) continue;
        SMESH_subMesh*                 faceSM = theMesh.GetSubMesh( face );
        if ( !faceSM->IsEmpty() )
        {
          int nbFaces = faceSM->GetSubMeshDS()->NbElements();
          if ( prevNbFaces < nbFaces )
          {
            if ( !meshedFaces.empty() ) meshedFaces.pop_back();
            meshedFaces.push_back( face ); // lower priority
            selectBottom = true;
            prevNbFaces = nbFaces;
          }
        }
        else
        {
          bool allSubMeComputed = true;
          SMESH_subMeshIteratorPtr smIt = faceSM->getDependsOnIterator(false,true);
          while ( smIt->more() && allSubMeComputed )
            allSubMeComputed = smIt->next()->IsMeshComputed();
          if ( allSubMeComputed )
          {
            faceSM->ComputeStateEngine( SMESH_subMesh::COMPUTE );
            if ( !faceSM->IsEmpty() ) {
              meshedFaces.push_front( face ); // higher priority
              selectBottom = true;
              break;
            }
            else {
              faceSM->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
            }
          }
        }
      }
    }


    // TODO. there are other ways to find out the source FACE:
    // propagation, topological similarity, ect.

    // simply try to mesh all not meshed SOLIDs
    if ( meshedFaces.empty() )
    {
      for ( TopExp_Explorer solid( theShape, TopAbs_SOLID ); solid.More(); solid.Next() )
      {
        mySetErrorToSM = false;
        prism.Clear();
        if ( !meshedSolids.Contains( solid.Current() ) &&
             initPrism( prism, solid.Current() ))
        {
          mySetErrorToSM = true;
          if ( !compute( prism ))
            return false;
          meshedFaces.push_front( prism.myTop );
          meshedFaces.push_front( prism.myBottom );
          meshedPrism.push_back( prism );
          meshedSolids.Add( solid.Current() );
          selectBottom = true;
        }
        mySetErrorToSM = true;
      }
    }

    if ( meshedFaces.empty() ) // set same error to 10 not-computed solids
    {
      SMESH_ComputeErrorPtr err = SMESH_ComputeError::New
        ( COMPERR_BAD_INPUT_MESH, "No meshed source face found", this );

      const int maxNbErrors = 10; // limit nb errors not to overload the Compute dialog
      TopExp_Explorer solid( theShape, TopAbs_SOLID );
      for ( int i = 0; ( i < maxNbErrors && solid.More() ); ++i, solid.Next() )
        if ( !meshedSolids.Contains( solid.Current() ))
        {
          SMESH_subMesh* sm = theMesh.GetSubMesh( solid.Current() );
          sm->GetComputeError() = err;
        }
      return error( err );
    }
  }
  return error( COMPERR_OK );
}

//================================================================================
/*!
 * \brief Find wall faces by bottom edges
 */
//================================================================================

bool StdMeshers_Prism_3D::getWallFaces( Prism_3D::TPrismTopo & thePrism,
                                        const int              totalNbFaces)
{
  thePrism.myWallQuads.clear();

  SMESH_Mesh* mesh = myHelper->GetMesh();

  StdMeshers_Quadrangle_2D* quadAlgo = TQuadrangleAlgo::instance( this, myHelper );

  TopTools_MapOfShape faceMap;
  TopTools_IndexedDataMapOfShapeListOfShape edgeToFaces;   
  TopExp::MapShapesAndAncestors( thePrism.myShape3D,
                                 TopAbs_EDGE, TopAbs_FACE, edgeToFaces );

  // ------------------------------
  // Get the 1st row of wall FACEs
  // ------------------------------

  list< TopoDS_Edge >::iterator edge = thePrism.myBottomEdges.begin();
  std::list< int >::iterator     nbE = thePrism.myNbEdgesInWires.begin();
  int iE = 0;
  double f,l;
  while ( edge != thePrism.myBottomEdges.end() )
  {
    ++iE;
    if ( BRep_Tool::Curve( *edge, f,l ).IsNull() )
    {
      edge = thePrism.myBottomEdges.erase( edge );
      --iE;
      --(*nbE);
    }
    else
    {
      TopTools_ListIteratorOfListOfShape faceIt( edgeToFaces.FindFromKey( *edge ));
      for ( ; faceIt.More(); faceIt.Next() )
      {
        const TopoDS_Face& face = TopoDS::Face( faceIt.Value() );
        if ( !thePrism.myBottom.IsSame( face ))
        {
          Prism_3D::TQuadList quadList( 1, quadAlgo->CheckNbEdges( *mesh, face ));
          if ( !quadList.back() )
            return toSM( error(TCom("Side face #") << shapeID( face )
                               << " not meshable with quadrangles"));
          bool isCompositeBase = ! setBottomEdge( *edge, quadList.back(), face );
          if ( isCompositeBase )
          {
            // it's OK if all EDGEs of the bottom side belongs to the bottom FACE
            StdMeshers_FaceSidePtr botSide = quadList.back()->side[ QUAD_BOTTOM_SIDE ];
            for ( int iE = 0; iE < botSide->NbEdges(); ++iE )
              if ( !myHelper->IsSubShape( botSide->Edge(iE), thePrism.myBottom ))
                return toSM( error(TCom("Composite 'horizontal' edges are not supported")));
          }
          if ( faceMap.Add( face ))
            thePrism.myWallQuads.push_back( quadList );
          break;
        }
      }
      ++edge;
    }
    if ( iE == *nbE )
    {
      iE = 0;
      ++nbE;
    }
  }

  // -------------------------
  // Find the rest wall FACEs
  // -------------------------

  // Compose a vector of indixes of right neighbour FACE for each wall FACE
  // that is not so evident in case of several WIREs in the bottom FACE
  thePrism.myRightQuadIndex.clear();
  for ( size_t i = 0; i < thePrism.myWallQuads.size(); ++i )
    thePrism.myRightQuadIndex.push_back( i+1 );
  list< int >::iterator nbEinW = thePrism.myNbEdgesInWires.begin();
  for ( int iLeft = 0; nbEinW != thePrism.myNbEdgesInWires.end(); ++nbEinW )
  {
    thePrism.myRightQuadIndex[ iLeft + *nbEinW - 1 ] = iLeft; // 1st EDGE index of a current WIRE
    iLeft += *nbEinW;
  }

  while ( totalNbFaces - faceMap.Extent() > 2 )
  {
    // find wall FACEs adjacent to each of wallQuads by the right side EDGE
    int nbKnownFaces;
    do {
      nbKnownFaces = faceMap.Extent();
      StdMeshers_FaceSidePtr rightSide, topSide; // sides of the quad
      for ( size_t i = 0; i < thePrism.myWallQuads.size(); ++i )
      {
        rightSide = thePrism.myWallQuads[i].back()->side[ QUAD_RIGHT_SIDE ];
        for ( int iE = 0; iE < rightSide->NbEdges(); ++iE ) // rightSide can be composite
        {
          const TopoDS_Edge & rightE = rightSide->Edge( iE );
          TopTools_ListIteratorOfListOfShape face( edgeToFaces.FindFromKey( rightE ));
          for ( ; face.More(); face.Next() )
            if ( faceMap.Add( face.Value() ))
            {
              // a new wall FACE encountered, store it in thePrism.myWallQuads
              const int iRight = thePrism.myRightQuadIndex[i];
              topSide = thePrism.myWallQuads[ iRight ].back()->side[ QUAD_TOP_SIDE ];
              const TopoDS_Edge&   newBotE = topSide->Edge(0);
              const TopoDS_Shape& newWallF = face.Value();
              thePrism.myWallQuads[ iRight ].push_back( quadAlgo->CheckNbEdges( *mesh, newWallF ));
              if ( !thePrism.myWallQuads[ iRight ].back() )
                return toSM( error(TCom("Side face #") << shapeID( newWallF ) <<
                                   " not meshable with quadrangles"));
              if ( ! setBottomEdge( newBotE, thePrism.myWallQuads[ iRight ].back(), newWallF ))
                return toSM( error(TCom("Composite 'horizontal' edges are not supported")));
            }
        }
      }
    } while ( nbKnownFaces != faceMap.Extent() );

    // find wall FACEs adjacent to each of thePrism.myWallQuads by the top side EDGE
    if ( totalNbFaces - faceMap.Extent() > 2 )
    {
      const int nbFoundWalls = faceMap.Extent();
      for ( size_t i = 0; i < thePrism.myWallQuads.size(); ++i )
      {
        StdMeshers_FaceSidePtr topSide = thePrism.myWallQuads[i].back()->side[ QUAD_TOP_SIDE ];
        const TopoDS_Edge &       topE = topSide->Edge( 0 );
        if ( topSide->NbEdges() > 1 )
          return toSM( error(COMPERR_BAD_SHAPE, TCom("Side face #") <<
                             shapeID( thePrism.myWallQuads[i].back()->face )
                             << " has a composite top edge"));
        TopTools_ListIteratorOfListOfShape faceIt( edgeToFaces.FindFromKey( topE ));
        for ( ; faceIt.More(); faceIt.Next() )
          if ( faceMap.Add( faceIt.Value() ))
          {
            // a new wall FACE encountered, store it in wallQuads
            thePrism.myWallQuads[ i ].push_back( quadAlgo->CheckNbEdges( *mesh, faceIt.Value() ));
            if ( !thePrism.myWallQuads[ i ].back() )
              return toSM( error(TCom("Side face #") << shapeID( faceIt.Value() ) <<
                                 " not meshable with quadrangles"));
            if ( ! setBottomEdge( topE, thePrism.myWallQuads[ i ].back(), faceIt.Value() ))
              return toSM( error(TCom("Composite 'horizontal' edges are not supported")));
            if ( totalNbFaces - faceMap.Extent() == 2 )
            {
              i = thePrism.myWallQuads.size(); // to quit from the outer loop
              break;
            }
          }
      }
      if ( nbFoundWalls == faceMap.Extent() )
        return toSM( error("Failed to find wall faces"));

    }
  } // while ( totalNbFaces - faceMap.Extent() > 2 )

  // ------------------
  // Find the top FACE
  // ------------------

  if ( thePrism.myTop.IsNull() )
  {
    // now only top and bottom FACEs are not in the faceMap
    faceMap.Add( thePrism.myBottom );
    for ( TopExp_Explorer f( thePrism.myShape3D, TopAbs_FACE );f.More(); f.Next() )
      if ( !faceMap.Contains( f.Current() )) {
        thePrism.myTop = TopoDS::Face( f.Current() );
        break;
      }
    if ( thePrism.myTop.IsNull() )
      return toSM( error("Top face not found"));
  }

  // Check that the top FACE shares all the top EDGEs
  for ( size_t i = 0; i < thePrism.myWallQuads.size(); ++i )
  {
    StdMeshers_FaceSidePtr topSide = thePrism.myWallQuads[i].back()->side[ QUAD_TOP_SIDE ];
    const TopoDS_Edge &       topE = topSide->Edge( 0 );
    if ( !myHelper->IsSubShape( topE, thePrism.myTop ))
      return toSM( error( TCom("Wrong source face: #") << shapeID( thePrism.myBottom )));
  }

  return true;
}

//=======================================================================
//function : compute
//purpose  : Compute mesh on a SOLID
//=======================================================================

bool StdMeshers_Prism_3D::compute(const Prism_3D::TPrismTopo& thePrism)
{
  myHelper->IsQuadraticSubMesh( thePrism.myShape3D );
  if ( _computeCanceled )
    return toSM( error( SMESH_ComputeError::New(COMPERR_CANCELED)));

  // Assure the bottom is meshed
  SMESH_subMesh * botSM = myHelper->GetMesh()->GetSubMesh( thePrism.myBottom );
  if (( botSM->IsEmpty() ) &&
      ( ! botSM->GetAlgo() ||
        ! _gen->Compute( *botSM->GetFather(), botSM->GetSubShape(), /*shapeOnly=*/true )))
    return error( COMPERR_BAD_INPUT_MESH,
                  TCom( "No mesher defined to compute the face #")
                  << shapeID( thePrism.myBottom ));

  // Make all side FACEs of thePrism meshed with quads
  if ( !computeWalls( thePrism ))
    return false;

  // Analyse mesh and geometry to find all block sub-shapes and submeshes
  // (after fixing IPAL52499 myBlock is used as a holder of boundary nodes
  // and for 2D projection in hard cases where StdMeshers_Projection_2D fails;
  // location of internal nodes is usually computed by StdMeshers_Sweeper)
  if ( !myBlock.Init( myHelper, thePrism ))
    return toSM( error( myBlock.GetError()));

  SMESHDS_Mesh* meshDS = myHelper->GetMeshDS();

  int volumeID = meshDS->ShapeToIndex( thePrism.myShape3D );

  // Try to get gp_Trsf to get all nodes from bottom ones
  vector<gp_Trsf> trsf;
  gp_Trsf bottomToTopTrsf;
  // if ( !myBlock.GetLayersTransformation( trsf, thePrism ))
  //   trsf.clear();
  // else if ( !trsf.empty() )
  //   bottomToTopTrsf = trsf.back();

  // To compute coordinates of a node inside a block, it is necessary to know
  // 1. normalized parameters of the node by which
  // 2. coordinates of node projections on all block sub-shapes are computed

  // So we fill projections on vertices at once as they are same for all nodes
  myShapeXYZ.resize( myBlock.NbSubShapes() );
  for ( int iV = SMESH_Block::ID_FirstV; iV < SMESH_Block::ID_FirstE; ++iV ) {
    myBlock.VertexPoint( iV, myShapeXYZ[ iV ]);
    SHOWYXZ("V point " <<iV << " ", myShapeXYZ[ iV ]);
  }

  // Projections on the top and bottom faces are taken from nodes existing
  // on these faces; find correspondence between bottom and top nodes
  myUseBlock = false;
  myBotToColumnMap.clear();
  if ( !assocOrProjBottom2Top( bottomToTopTrsf, thePrism ) ) // it also fills myBotToColumnMap
    return false;


  // Create nodes inside the block

  // use transformation (issue 0020680, IPAL0052499)
  StdMeshers_Sweeper sweeper;
  double tol;
  bool allowHighBndError;

  if ( !myUseBlock )
  {
    // load boundary nodes into sweeper
    bool dummy;
    list< TopoDS_Edge >::const_iterator edge = thePrism.myBottomEdges.begin();
    for ( ; edge != thePrism.myBottomEdges.end(); ++edge )
    {
      int edgeID = meshDS->ShapeToIndex( *edge );
      TParam2ColumnMap* u2col = const_cast<TParam2ColumnMap*>
        ( myBlock.GetParam2ColumnMap( edgeID, dummy ));
      TParam2ColumnMap::iterator u2colIt = u2col->begin();
      for ( ; u2colIt != u2col->end(); ++u2colIt )
        sweeper.myBndColumns.push_back( & u2colIt->second );
    }
    // load node columns inside the bottom face
    TNode2ColumnMap::iterator bot_column = myBotToColumnMap.begin();
    for ( ; bot_column != myBotToColumnMap.end(); ++bot_column )
      sweeper.myIntColumns.push_back( & bot_column->second );

    tol = getSweepTolerance( thePrism );
    allowHighBndError = !isSimpleBottom( thePrism );
  }

  if ( !myUseBlock && sweeper.ComputeNodes( *myHelper, tol, allowHighBndError ))
  {
  }
  else // use block approach
  {
    // loop on nodes inside the bottom face
    Prism_3D::TNode prevBNode;
    TNode2ColumnMap::iterator bot_column = myBotToColumnMap.begin();
    for ( ; bot_column != myBotToColumnMap.end(); ++bot_column )
    {
      const Prism_3D::TNode& tBotNode = bot_column->first; // bottom TNode
      if ( tBotNode.GetPositionType() != SMDS_TOP_FACE )
        continue; // node is not inside the FACE

      // column nodes; middle part of the column are zero pointers
      TNodeColumn& column = bot_column->second;

      gp_XYZ botParams, topParams;
      if ( !tBotNode.HasParams() )
      {
        // compute bottom node parameters
        gp_XYZ paramHint(-1,-1,-1);
        if ( prevBNode.IsNeighbor( tBotNode ))
          paramHint = prevBNode.GetParams();
        if ( !myBlock.ComputeParameters( tBotNode.GetCoords(), tBotNode.ChangeParams(),
                                         ID_BOT_FACE, paramHint ))
          return toSM( error(TCom("Can't compute normalized parameters for node ")
                             << tBotNode.myNode->GetID() << " on the face #"
                             << myBlock.SubMesh( ID_BOT_FACE )->GetId() ));
        prevBNode = tBotNode;

        botParams = topParams = tBotNode.GetParams();
        topParams.SetZ( 1 );

        // compute top node parameters
        if ( column.size() > 2 ) {
          gp_Pnt topCoords = gpXYZ( column.back() );
          if ( !myBlock.ComputeParameters( topCoords, topParams, ID_TOP_FACE, topParams ))
            return toSM( error(TCom("Can't compute normalized parameters ")
                               << "for node " << column.back()->GetID()
                               << " on the face #"<< column.back()->getshapeId() ));
        }
      }
      else // top nodes are created by projection using parameters
      {
        botParams = topParams = tBotNode.GetParams();
        topParams.SetZ( 1 );
      }

      myShapeXYZ[ ID_BOT_FACE ] = tBotNode.GetCoords();
      myShapeXYZ[ ID_TOP_FACE ] = gpXYZ( column.back() );

      // vertical loop
      TNodeColumn::iterator columnNodes = column.begin();
      for ( int z = 0; columnNodes != column.end(); ++columnNodes, ++z)
      {
        const SMDS_MeshNode* & node = *columnNodes;
        if ( node ) continue; // skip bottom or top node

        // params of a node to create
        double rz = (double) z / (double) ( column.size() - 1 );
        gp_XYZ params = botParams * ( 1 - rz ) + topParams * rz;

        // set coords on all faces and nodes
        const int nbSideFaces = 4;
        int sideFaceIDs[nbSideFaces] = { SMESH_Block::ID_Fx0z,
                                         SMESH_Block::ID_Fx1z,
                                         SMESH_Block::ID_F0yz,
                                         SMESH_Block::ID_F1yz };
        for ( int iF = 0; iF < nbSideFaces; ++iF )
          if ( !setFaceAndEdgesXYZ( sideFaceIDs[ iF ], params, z ))
            return false;

        // compute coords for a new node
        gp_XYZ coords;
        if ( !SMESH_Block::ShellPoint( params, myShapeXYZ, coords ))
          return toSM( error("Can't compute coordinates by normalized parameters"));

        // if ( !meshDS->MeshElements( volumeID ) ||
        //      meshDS->MeshElements( volumeID )->NbNodes() == 0 )
        //   pointsToPython(myShapeXYZ);
        SHOWYXZ("TOPFacePoint ",myShapeXYZ[ ID_TOP_FACE]);
        SHOWYXZ("BOT Node "<< tBotNode.myNode->GetID(),gpXYZ(tBotNode.myNode));
        SHOWYXZ("ShellPoint ",coords);

        // create a node
        node = meshDS->AddNode( coords.X(), coords.Y(), coords.Z() );
        meshDS->SetNodeInVolume( node, volumeID );

        if ( _computeCanceled )
          return false;
      }
    } // loop on bottom nodes
  }

  // Create volumes

  SMESHDS_SubMesh* smDS = myBlock.SubMeshDS( ID_BOT_FACE );
  if ( !smDS ) return toSM( error(COMPERR_BAD_INPUT_MESH, "Null submesh"));

  // loop on bottom mesh faces
  SMDS_ElemIteratorPtr faceIt = smDS->GetElements();
  while ( faceIt->more() )
  {
    const SMDS_MeshElement* face = faceIt->next();
    if ( !face || face->GetType() != SMDSAbs_Face )
      continue;

    // find node columns for each node
    int nbNodes = face->NbCornerNodes();
    vector< const TNodeColumn* > columns( nbNodes );
    for ( int i = 0; i < nbNodes; ++i )
    {
      const SMDS_MeshNode* n = face->GetNode( i );
      if ( n->GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE ) {
        TNode2ColumnMap::iterator bot_column = myBotToColumnMap.find( n );
        if ( bot_column == myBotToColumnMap.end() )
          return toSM( error(TCom("No nodes found above node ") << n->GetID() ));
        columns[ i ] = & bot_column->second;
      }
      else {
        columns[ i ] = myBlock.GetNodeColumn( n );
        if ( !columns[ i ] )
          return toSM( error(TCom("No side nodes found above node ") << n->GetID() ));
      }
    }
    // create prisms
    AddPrisms( columns, myHelper );

  } // loop on bottom mesh faces

  // clear data
  myBotToColumnMap.clear();
  myBlock.Clear();

  // update state of sub-meshes (mostly in order to erase improper errors)
  SMESH_subMesh* sm = myHelper->GetMesh()->GetSubMesh( thePrism.myShape3D );
  SMESH_subMeshIteratorPtr smIt = sm->getDependsOnIterator(/*includeSelf=*/false);
  while ( smIt->more() )
  {
    sm = smIt->next();
    sm->GetComputeError().reset();
    sm->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
  }

  return true;
}

//=======================================================================
//function : computeWalls
//purpose  : Compute 2D mesh on walls FACEs of a prism
//=======================================================================

bool StdMeshers_Prism_3D::computeWalls(const Prism_3D::TPrismTopo& thePrism)
{
  SMESH_Mesh*     mesh = myHelper->GetMesh();
  SMESHDS_Mesh* meshDS = myHelper->GetMeshDS();
  DBGOUT( endl << "COMPUTE Prism " << meshDS->ShapeToIndex( thePrism.myShape3D ));

  TProjction1dAlgo*      projector1D = TProjction1dAlgo::instance( this );
  StdMeshers_Quadrangle_2D* quadAlgo = TQuadrangleAlgo::instance( this, myHelper );

  // SMESH_HypoFilter hyp1dFilter( SMESH_HypoFilter::IsAlgo(),/*not=*/true);
  // hyp1dFilter.And( SMESH_HypoFilter::HasDim( 1 ));
  // hyp1dFilter.And( SMESH_HypoFilter::IsMoreLocalThan( thePrism.myShape3D, *mesh ));

  // Discretize equally 'vertical' EDGEs
  // -----------------------------------
  // find source FACE sides for projection: either already computed ones or
  // the 'most composite' ones
  const size_t nbWalls = thePrism.myWallQuads.size();
  vector< int > wgt( nbWalls, 0 ); // "weight" of a wall
  for ( size_t iW = 0; iW != nbWalls; ++iW )
  {
    Prism_3D::TQuadList::const_iterator quad = thePrism.myWallQuads[iW].begin();
    for ( ; quad != thePrism.myWallQuads[iW].end(); ++quad )
    {
      StdMeshers_FaceSidePtr lftSide = (*quad)->side[ QUAD_LEFT_SIDE ];
      for ( int i = 0; i < lftSide->NbEdges(); ++i )
      {
        ++wgt[ iW ];
        const TopoDS_Edge& E = lftSide->Edge(i);
        if ( mesh->GetSubMesh( E )->IsMeshComputed() )
        {
          wgt[ iW ] += 100;
          wgt[ myHelper->WrapIndex( iW+1, nbWalls)] += 10;
          wgt[ myHelper->WrapIndex( iW-1, nbWalls)] += 10;
        }
        // else if ( mesh->GetHypothesis( E, hyp1dFilter, true )) // local hypothesis!
        //   wgt += 100;
      }
    }
    // in quadratic mesh, pass ignoreMediumNodes to quad sides
    if ( myHelper->GetIsQuadratic() )
    {
      quad = thePrism.myWallQuads[iW].begin();
      for ( ; quad != thePrism.myWallQuads[iW].end(); ++quad )
        for ( int i = 0; i < NB_QUAD_SIDES; ++i )
          (*quad)->side[ i ].grid->SetIgnoreMediumNodes( true );
    }
  }
  multimap< int, int > wgt2quad;
  for ( size_t iW = 0; iW != nbWalls; ++iW )
    wgt2quad.insert( make_pair( wgt[ iW ], iW ));

  // Project 'vertical' EDGEs, from left to right
  multimap< int, int >::reverse_iterator w2q = wgt2quad.rbegin();
  for ( ; w2q != wgt2quad.rend(); ++w2q )
  {
    const int iW = w2q->second;
    const Prism_3D::TQuadList&         quads = thePrism.myWallQuads[ iW ];
    Prism_3D::TQuadList::const_iterator quad = quads.begin();
    for ( ; quad != quads.end(); ++quad )
    {
      StdMeshers_FaceSidePtr rgtSide = (*quad)->side[ QUAD_RIGHT_SIDE ]; // tgt
      StdMeshers_FaceSidePtr lftSide = (*quad)->side[ QUAD_LEFT_SIDE ];  // src
      bool swapLeftRight = ( lftSide->NbSegments( /*update=*/true ) == 0 &&
                             rgtSide->NbSegments( /*update=*/true )  > 0 );
      if ( swapLeftRight )
        std::swap( lftSide, rgtSide );

      // assure that all the source (left) EDGEs are meshed
      int nbSrcSegments = 0;
      for ( int i = 0; i < lftSide->NbEdges(); ++i )
      {
        const TopoDS_Edge& srcE = lftSide->Edge(i);
        SMESH_subMesh*    srcSM = mesh->GetSubMesh( srcE );
        if ( !srcSM->IsMeshComputed() ) {
          DBGOUT( "COMPUTE V edge " << srcSM->GetId() );
          TopoDS_Edge prpgSrcE = findPropagationSource( srcE );
          if ( !prpgSrcE.IsNull() ) {
            srcSM->ComputeSubMeshStateEngine( SMESH_subMesh::COMPUTE );
            projector1D->myHyp.SetSourceEdge( prpgSrcE );
            projector1D->Compute( *mesh, srcE );
            srcSM->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
          }
          else {
            srcSM->ComputeSubMeshStateEngine( SMESH_subMesh::COMPUTE );
            srcSM->ComputeStateEngine       ( SMESH_subMesh::COMPUTE );
          }
          if ( !srcSM->IsMeshComputed() )
            return toSM( error( "Can't compute 1D mesh" ));
        }
        nbSrcSegments += srcSM->GetSubMeshDS()->NbElements();
      }
      // check target EDGEs
      int nbTgtMeshed = 0, nbTgtSegments = 0;
      vector< bool > isTgtEdgeComputed( rgtSide->NbEdges() );
      for ( int i = 0; i < rgtSide->NbEdges(); ++i )
      {
        const TopoDS_Edge& tgtE = rgtSide->Edge(i);
        SMESH_subMesh*    tgtSM = mesh->GetSubMesh( tgtE );
        if ( !( isTgtEdgeComputed[ i ] = tgtSM->IsMeshComputed() )) {
          tgtSM->ComputeSubMeshStateEngine( SMESH_subMesh::COMPUTE );
          tgtSM->ComputeStateEngine       ( SMESH_subMesh::COMPUTE );
        }
        if ( tgtSM->IsMeshComputed() ) {
          ++nbTgtMeshed;
          nbTgtSegments += tgtSM->GetSubMeshDS()->NbElements();
        }
      }
      if ( rgtSide->NbEdges() == nbTgtMeshed ) // all tgt EDGEs meshed
      {
        if ( nbTgtSegments != nbSrcSegments )
        {
          bool badMeshRemoved = false;
          // remove just computed segments
          for ( int i = 0; i < rgtSide->NbEdges(); ++i )
            if ( !isTgtEdgeComputed[ i ])
            {
              const TopoDS_Edge& tgtE = rgtSide->Edge(i);
              SMESH_subMesh*    tgtSM = mesh->GetSubMesh( tgtE );
              tgtSM->ComputeStateEngine( SMESH_subMesh::CLEAN );
              badMeshRemoved = true;
              nbTgtMeshed--;
            }
          if ( !badMeshRemoved )
          {
            for ( int i = 0; i < lftSide->NbEdges(); ++i )
              addBadInputElements( meshDS->MeshElements( lftSide->Edge( i )));
            for ( int i = 0; i < rgtSide->NbEdges(); ++i )
              addBadInputElements( meshDS->MeshElements( rgtSide->Edge( i )));
            return toSM( error( TCom("Different nb of segment on logically vertical edges #")
                                << shapeID( lftSide->Edge(0) ) << " and #"
                                << shapeID( rgtSide->Edge(0) ) << ": "
                                << nbSrcSegments << " != " << nbTgtSegments ));
          }
        }
        else // if ( nbTgtSegments == nbSrcSegments )
        {
          continue;
        }
      }
      // Compute 'vertical projection'
      if ( nbTgtMeshed == 0 )
      {
        // compute nodes on target VERTEXes
        const UVPtStructVec&  srcNodeStr = lftSide->GetUVPtStruct();
        if ( srcNodeStr.size() == 0 )
          return toSM( error( TCom("Invalid node positions on edge #") <<
                              shapeID( lftSide->Edge(0) )));
        vector< SMDS_MeshNode* > newNodes( srcNodeStr.size() );
        for ( int is2ndV = 0; is2ndV < 2; ++is2ndV )
        {
          const TopoDS_Edge& E = rgtSide->Edge( is2ndV ? rgtSide->NbEdges()-1 : 0 );
          TopoDS_Vertex      v = myHelper->IthVertex( is2ndV, E );
          mesh->GetSubMesh( v )->ComputeStateEngine( SMESH_subMesh::COMPUTE );
          const SMDS_MeshNode* n = SMESH_Algo::VertexNode( v, meshDS );
          newNodes[ is2ndV ? 0 : newNodes.size()-1 ] = (SMDS_MeshNode*) n;
        }

        // compute nodes on target EDGEs
        DBGOUT( "COMPUTE V edge (proj) " << shapeID( lftSide->Edge(0)));
        rgtSide->Reverse(); // direct it same as the lftSide
        myHelper->SetElementsOnShape( false ); // myHelper holds the prism shape
        TopoDS_Edge tgtEdge;
        for ( size_t iN = 1; iN < srcNodeStr.size()-1; ++iN ) // add nodes
        {
          gp_Pnt       p = rgtSide->Value3d  ( srcNodeStr[ iN ].normParam );
          double       u = rgtSide->Parameter( srcNodeStr[ iN ].normParam, tgtEdge );
          newNodes[ iN ] = meshDS->AddNode( p.X(), p.Y(), p.Z() );
          meshDS->SetNodeOnEdge( newNodes[ iN ], tgtEdge, u );
        }
        for ( size_t iN = 1; iN < srcNodeStr.size(); ++iN ) // add segments
        {
          // find an EDGE to set a new segment
          std::pair<int, TopAbs_ShapeEnum> id2type = 
            myHelper->GetMediumPos( newNodes[ iN-1 ], newNodes[ iN ] );
          if ( id2type.second != TopAbs_EDGE )
          {
            // new nodes are on different EDGEs; put one of them on VERTEX
            const int      edgeIndex = rgtSide->EdgeIndex( srcNodeStr[ iN-1 ].normParam );
            const double vertexParam = rgtSide->LastParameter( edgeIndex );
            TopoDS_Vertex     vertex = rgtSide->LastVertex( edgeIndex );
            const SMDS_MeshNode*  vn = SMESH_Algo::VertexNode( vertex, meshDS );
            const gp_Pnt           p = BRep_Tool::Pnt( vertex );
            const int         isPrev = ( Abs( srcNodeStr[ iN-1 ].normParam - vertexParam ) <
                                         Abs( srcNodeStr[ iN   ].normParam - vertexParam ));
            meshDS->UnSetNodeOnShape( newNodes[ iN-isPrev ] );
            meshDS->SetNodeOnVertex ( newNodes[ iN-isPrev ], vertex );
            meshDS->MoveNode        ( newNodes[ iN-isPrev ], p.X(), p.Y(), p.Z() );
            id2type.first = newNodes[ iN-(1-isPrev) ]->getshapeId();
            if ( vn )
            {
              SMESH_MeshEditor::TListOfListOfNodes lln( 1, list< const SMDS_MeshNode* >() );
              lln.back().push_back ( vn );
              lln.back().push_front( newNodes[ iN-isPrev ] ); // to keep 
              SMESH_MeshEditor( mesh ).MergeNodes( lln );
            }
          }
          SMDS_MeshElement* newEdge = myHelper->AddEdge( newNodes[ iN-1 ], newNodes[ iN ] );
          meshDS->SetMeshElementOnShape( newEdge, id2type.first );
        }
        myHelper->SetElementsOnShape( true );
        for ( int i = 0; i < rgtSide->NbEdges(); ++i ) // update state of sub-meshes
        {
          const TopoDS_Edge& E = rgtSide->Edge( i );
          SMESH_subMesh* tgtSM = mesh->GetSubMesh( E );
          tgtSM->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
        }

        // to continue projection from the just computed side as a source
        if ( !swapLeftRight && rgtSide->NbEdges() > 1 && w2q->second == iW )
        {
          std::pair<int,int> wgt2quadKeyVal( w2q->first + 1, thePrism.myRightQuadIndex[ iW ]);
          wgt2quad.insert( wgt2quadKeyVal ); // it will be skipped by ++w2q
          wgt2quad.insert( wgt2quadKeyVal );
          w2q = wgt2quad.rbegin();
        }
      }
      else
      {
        // HOPE assigned hypotheses are OK, so that equal nb of segments will be generated
        //return toSM( error("Partial projection not implemented"));
      }
    } // loop on quads of a composite wall side
  } // loop on the ordered wall sides



  for ( size_t iW = 0; iW != thePrism.myWallQuads.size(); ++iW )
  {
    Prism_3D::TQuadList::const_iterator quad = thePrism.myWallQuads[iW].begin();
    for ( ; quad != thePrism.myWallQuads[iW].end(); ++quad )
    {
      const TopoDS_Face& face = (*quad)->face;
      SMESH_subMesh*      fSM = mesh->GetSubMesh( face );
      if ( ! fSM->IsMeshComputed() )
      {
        // Top EDGEs must be projections from the bottom ones
        // to compute stuctured quad mesh on wall FACEs
        // ---------------------------------------------------
        const TopoDS_Edge& botE = (*quad)->side[ QUAD_BOTTOM_SIDE ].grid->Edge(0);
        const TopoDS_Edge& topE = (*quad)->side[ QUAD_TOP_SIDE    ].grid->Edge(0);
        SMESH_subMesh*    botSM = mesh->GetSubMesh( botE );
        SMESH_subMesh*    topSM = mesh->GetSubMesh( topE );
        SMESH_subMesh*    srcSM = botSM;
        SMESH_subMesh*    tgtSM = topSM;
        srcSM->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
        tgtSM->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
        if ( !srcSM->IsMeshComputed() && tgtSM->IsMeshComputed() )
          std::swap( srcSM, tgtSM );

        if ( !srcSM->IsMeshComputed() )
        {
          DBGOUT( "COMPUTE H edge " << srcSM->GetId());
          srcSM->ComputeSubMeshStateEngine( SMESH_subMesh::COMPUTE ); // nodes on VERTEXes
          srcSM->ComputeStateEngine( SMESH_subMesh::COMPUTE );        // segments on the EDGE
        }

        if ( tgtSM->IsMeshComputed() &&
             tgtSM->GetSubMeshDS()->NbNodes() != srcSM->GetSubMeshDS()->NbNodes() )
        {
          // the top EDGE is computed differently than the bottom one,
          // try to clear a wrong mesh
          bool isAdjFaceMeshed = false;
          PShapeIteratorPtr fIt = myHelper->GetAncestors( tgtSM->GetSubShape(),
                                                          *mesh, TopAbs_FACE );
          while ( const TopoDS_Shape* f = fIt->next() )
            if (( isAdjFaceMeshed = mesh->GetSubMesh( *f )->IsMeshComputed() ))
              break;
          if ( isAdjFaceMeshed )
            return toSM( error( TCom("Different nb of segment on logically horizontal edges #")
                                << shapeID( botE ) << " and #"
                                << shapeID( topE ) << ": "
                                << tgtSM->GetSubMeshDS()->NbElements() << " != "
                                << srcSM->GetSubMeshDS()->NbElements() ));
          tgtSM->ComputeStateEngine( SMESH_subMesh::CLEAN );
        }
        if ( !tgtSM->IsMeshComputed() )
        {
          // compute nodes on VERTEXes
          SMESH_subMeshIteratorPtr smIt = tgtSM->getDependsOnIterator(/*includeSelf=*/false);
          while ( smIt->more() )
            smIt->next()->ComputeStateEngine( SMESH_subMesh::COMPUTE );
          // project segments
          DBGOUT( "COMPUTE H edge (proj) " << tgtSM->GetId());
          projector1D->myHyp.SetSourceEdge( TopoDS::Edge( srcSM->GetSubShape() ));
          projector1D->InitComputeError();
          bool ok = projector1D->Compute( *mesh, tgtSM->GetSubShape() );
          if ( !ok )
          {
            SMESH_ComputeErrorPtr err = projector1D->GetComputeError();
            if ( err->IsOK() ) err->myName = COMPERR_ALGO_FAILED;
            tgtSM->GetComputeError() = err;
            return false;
          }
        }
        tgtSM->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );


        // Compute quad mesh on wall FACEs
        // -------------------------------

        // make all EDGES meshed
        fSM->ComputeSubMeshStateEngine( SMESH_subMesh::COMPUTE );
        if ( !fSM->SubMeshesComputed() )
          return toSM( error( COMPERR_BAD_INPUT_MESH,
                              "Not all edges have valid algorithm and hypothesis"));
        // mesh the <face>
        quadAlgo->InitComputeError();
        DBGOUT( "COMPUTE Quad face " << fSM->GetId());
        bool ok = quadAlgo->Compute( *mesh, face );
        fSM->GetComputeError() = quadAlgo->GetComputeError();
        if ( !ok )
          return false;
        fSM->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
      }
      if ( myHelper->GetIsQuadratic() )
      {
        // fill myHelper with medium nodes built by quadAlgo
        SMDS_ElemIteratorPtr fIt = fSM->GetSubMeshDS()->GetElements();
        while ( fIt->more() )
          myHelper->AddTLinks( dynamic_cast<const SMDS_MeshFace*>( fIt->next() ));
      }
    }
  }

  return true;
}

//=======================================================================
/*!
 * \brief Returns a source EDGE of propagation to a given EDGE
 */
//=======================================================================

TopoDS_Edge StdMeshers_Prism_3D::findPropagationSource( const TopoDS_Edge& E )
{
  if ( myPropagChains )
    for ( size_t i = 0; !myPropagChains[i].IsEmpty(); ++i )
      if ( myPropagChains[i].Contains( E ))
        return TopoDS::Edge( myPropagChains[i].FindKey( 1 ));

  return TopoDS_Edge();
}

//=======================================================================
//function : Evaluate
//purpose  : 
//=======================================================================

bool StdMeshers_Prism_3D::Evaluate(SMESH_Mesh&         theMesh,
                                   const TopoDS_Shape& theShape,
                                   MapShapeNbElems&    aResMap)
{
  if ( theShape.ShapeType() == TopAbs_COMPOUND )
  {
    bool ok = true;
    for ( TopoDS_Iterator it( theShape ); it.More(); it.Next() )
      ok &= Evaluate( theMesh, it.Value(), aResMap );
    return ok;
  }
  SMESH_MesherHelper helper( theMesh );
  myHelper = &helper;
  myHelper->SetSubShape( theShape );

  // find face contains only triangles
  vector < SMESH_subMesh * >meshFaces;
  TopTools_SequenceOfShape aFaces;
  int NumBase = 0, i = 0, NbQFs = 0;
  for (TopExp_Explorer exp(theShape, TopAbs_FACE); exp.More(); exp.Next()) {
    i++;
    aFaces.Append(exp.Current());
    SMESH_subMesh *aSubMesh = theMesh.GetSubMesh(exp.Current());
    meshFaces.push_back(aSubMesh);
    MapShapeNbElemsItr anIt = aResMap.find(meshFaces[i-1]);
    if( anIt==aResMap.end() )
      return toSM( error( "Submesh can not be evaluated"));

    std::vector<int> aVec = (*anIt).second;
    int nbtri = Max(aVec[SMDSEntity_Triangle],aVec[SMDSEntity_Quad_Triangle]);
    int nbqua = Max(aVec[SMDSEntity_Quadrangle],aVec[SMDSEntity_Quad_Quadrangle]);
    if( nbtri==0 && nbqua>0 ) {
      NbQFs++;
    }
    if( nbtri>0 ) {
      NumBase = i;
    }
  }

  if(NbQFs<4) {
    std::vector<int> aResVec(SMDSEntity_Last);
    for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
    SMESH_subMesh * sm = theMesh.GetSubMesh(theShape);
    aResMap.insert(std::make_pair(sm,aResVec));
    return toSM( error( "Submesh can not be evaluated" ));
  }

  if(NumBase==0) NumBase = 1; // only quads => set 1 faces as base

  // find number of 1d elems for base face
  int nb1d = 0;
  TopTools_MapOfShape Edges1;
  for (TopExp_Explorer exp(aFaces.Value(NumBase), TopAbs_EDGE); exp.More(); exp.Next()) {
    Edges1.Add(exp.Current());
    SMESH_subMesh *sm = theMesh.GetSubMesh(exp.Current());
    if( sm ) {
      MapShapeNbElemsItr anIt = aResMap.find(sm);
      if( anIt == aResMap.end() ) continue;
      std::vector<int> aVec = (*anIt).second;
      nb1d += Max(aVec[SMDSEntity_Edge],aVec[SMDSEntity_Quad_Edge]);
    }
  }
  // find face opposite to base face
  int OppNum = 0;
  for(i=1; i<=6; i++) {
    if(i==NumBase) continue;
    bool IsOpposite = true;
    for(TopExp_Explorer exp(aFaces.Value(i), TopAbs_EDGE); exp.More(); exp.Next()) {
      if( Edges1.Contains(exp.Current()) ) {
        IsOpposite = false;
        break;
      }
    }
    if(IsOpposite) {
      OppNum = i;
      break;
    }
  }
  // find number of 2d elems on side faces
  int nb2d = 0;
  for(i=1; i<=6; i++) {
    if( i==OppNum || i==NumBase ) continue;
    MapShapeNbElemsItr anIt = aResMap.find( meshFaces[i-1] );
    if( anIt == aResMap.end() ) continue;
    std::vector<int> aVec = (*anIt).second;
    nb2d += Max(aVec[SMDSEntity_Quadrangle],aVec[SMDSEntity_Quad_Quadrangle]);
  }
  
  MapShapeNbElemsItr anIt = aResMap.find( meshFaces[NumBase-1] );
  std::vector<int> aVec = (*anIt).second;
  bool IsQuadratic = (aVec[SMDSEntity_Quad_Triangle]>aVec[SMDSEntity_Triangle]) ||
                     (aVec[SMDSEntity_Quad_Quadrangle]>aVec[SMDSEntity_Quadrangle]);
  int nb2d_face0_3 = Max(aVec[SMDSEntity_Triangle],aVec[SMDSEntity_Quad_Triangle]);
  int nb2d_face0_4 = Max(aVec[SMDSEntity_Quadrangle],aVec[SMDSEntity_Quad_Quadrangle]);
  int nb0d_face0 = aVec[SMDSEntity_Node];
  int nb1d_face0_int = ( nb2d_face0_3*3 + nb2d_face0_4*4 - nb1d ) / 2;

  std::vector<int> aResVec(SMDSEntity_Last);
  for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
  if(IsQuadratic) {
    aResVec[SMDSEntity_Quad_Penta] = nb2d_face0_3 * ( nb2d/nb1d );
    aResVec[SMDSEntity_Quad_Hexa] = nb2d_face0_4 * ( nb2d/nb1d );
    aResVec[SMDSEntity_Node] = nb0d_face0 * ( 2*nb2d/nb1d - 1 ) - nb1d_face0_int * nb2d/nb1d;
  }
  else {
    aResVec[SMDSEntity_Node] = nb0d_face0 * ( nb2d/nb1d - 1 );
    aResVec[SMDSEntity_Penta] = nb2d_face0_3 * ( nb2d/nb1d );
    aResVec[SMDSEntity_Hexa] = nb2d_face0_4 * ( nb2d/nb1d );
  }
  SMESH_subMesh * sm = theMesh.GetSubMesh(theShape);
  aResMap.insert(std::make_pair(sm,aResVec));

  return true;
}

//================================================================================
/*!
 * \brief Create prisms
 * \param columns - columns of nodes generated from nodes of a mesh face
 * \param helper - helper initialized by mesh and shape to add prisms to
 */
//================================================================================

void StdMeshers_Prism_3D::AddPrisms( vector<const TNodeColumn*> & columns,
                                     SMESH_MesherHelper*          helper)
{
  int nbNodes = columns.size();
  int nbZ     = columns[0]->size();
  if ( nbZ < 2 ) return;

  // find out orientation
  bool isForward = true;
  SMDS_VolumeTool vTool;
  int z = 1;
  switch ( nbNodes ) {
  case 3: {
    SMDS_VolumeOfNodes tmpPenta ( (*columns[0])[z-1], // bottom
                                  (*columns[1])[z-1],
                                  (*columns[2])[z-1],
                                  (*columns[0])[z],   // top
                                  (*columns[1])[z],
                                  (*columns[2])[z] );
    vTool.Set( &tmpPenta );
    isForward  = vTool.IsForward();
    break;
  }
  case 4: {
    SMDS_VolumeOfNodes tmpHex( (*columns[0])[z-1], (*columns[1])[z-1], // bottom
                               (*columns[2])[z-1], (*columns[3])[z-1],
                               (*columns[0])[z],   (*columns[1])[z],   // top
                               (*columns[2])[z],   (*columns[3])[z] );
    vTool.Set( &tmpHex );
    isForward  = vTool.IsForward();
    break;
  }
  default:
    const int di = (nbNodes+1) / 3;
    SMDS_VolumeOfNodes tmpVol ( (*columns[0]   )[z-1],
                                (*columns[di]  )[z-1],
                                (*columns[2*di])[z-1],
                                (*columns[0]   )[z],
                                (*columns[di]  )[z],
                                (*columns[2*di])[z] );
    vTool.Set( &tmpVol );
    isForward  = vTool.IsForward();
  }

  // vertical loop on columns

  helper->SetElementsOnShape( true );

  switch ( nbNodes ) {

  case 3: { // ---------- pentahedra
    const int i1 = isForward ? 1 : 2;
    const int i2 = isForward ? 2 : 1;
    for ( z = 1; z < nbZ; ++z )
      helper->AddVolume( (*columns[0 ])[z-1], // bottom
                         (*columns[i1])[z-1],
                         (*columns[i2])[z-1],
                         (*columns[0 ])[z],   // top
                         (*columns[i1])[z],
                         (*columns[i2])[z] );
    break;
  }
  case 4: { // ---------- hexahedra
    const int i1 = isForward ? 1 : 3;
    const int i3 = isForward ? 3 : 1;
    for ( z = 1; z < nbZ; ++z )
      helper->AddVolume( (*columns[0])[z-1], (*columns[i1])[z-1], // bottom
                         (*columns[2])[z-1], (*columns[i3])[z-1],
                         (*columns[0])[z],   (*columns[i1])[z],     // top
                         (*columns[2])[z],   (*columns[i3])[z] );
    break;
  }
  case 6: { // ---------- octahedra
    const int iBase1 = isForward ? -1 : 0;
    const int iBase2 = isForward ?  0 :-1;
    for ( z = 1; z < nbZ; ++z )
      helper->AddVolume( (*columns[0])[z+iBase1], (*columns[1])[z+iBase1], // bottom or top
                         (*columns[2])[z+iBase1], (*columns[3])[z+iBase1],
                         (*columns[4])[z+iBase1], (*columns[5])[z+iBase1],
                         (*columns[0])[z+iBase2], (*columns[1])[z+iBase2], // top or bottom
                         (*columns[2])[z+iBase2], (*columns[3])[z+iBase2],
                         (*columns[4])[z+iBase2], (*columns[5])[z+iBase2] );
    break;
  }
  default: // ---------- polyhedra
    vector<int> quantities( 2 + nbNodes, 4 );
    quantities[0] = quantities[1] = nbNodes;
    columns.resize( nbNodes + 1 );
    columns[ nbNodes ] = columns[ 0 ];
    const int i1 = isForward ? 1 : 3;
    const int i3 = isForward ? 3 : 1;
    const int iBase1 = isForward ? -1 : 0;
    const int iBase2 = isForward ?  0 :-1;
    vector<const SMDS_MeshNode*> nodes( 2*nbNodes + 4*nbNodes);
    for ( z = 1; z < nbZ; ++z )
    {
      for ( int i = 0; i < nbNodes; ++i ) {
        nodes[ i             ] = (*columns[ i ])[z+iBase1]; // bottom or top
        nodes[ 2*nbNodes-i-1 ] = (*columns[ i ])[z+iBase2]; // top or bottom
        // side
        int di = 2*nbNodes + 4*i;
        nodes[ di+0 ] = (*columns[i  ])[z  ];
        nodes[ di+i1] = (*columns[i+1])[z  ];
        nodes[ di+2 ] = (*columns[i+1])[z-1];
        nodes[ di+i3] = (*columns[i  ])[z-1];
      }
      helper->AddPolyhedralVolume( nodes, quantities );
    }

  } // switch ( nbNodes )
}

//================================================================================
/*!
 * \brief Find correspondence between bottom and top nodes
 *  If elements on the bottom and top faces are topologically different,
 *  and projection is possible and allowed, perform the projection
 *  \retval bool - is a success or not
 */
//================================================================================

bool StdMeshers_Prism_3D::assocOrProjBottom2Top( const gp_Trsf & bottomToTopTrsf,
                                                 const Prism_3D::TPrismTopo& thePrism)
{
  SMESH_subMesh * botSM = myHelper->GetMesh()->GetSubMesh( thePrism.myBottom );
  SMESH_subMesh * topSM = myHelper->GetMesh()->GetSubMesh( thePrism.myTop    );

  SMESHDS_SubMesh * botSMDS = botSM->GetSubMeshDS();
  SMESHDS_SubMesh * topSMDS = topSM->GetSubMeshDS();

  if ( !botSMDS || botSMDS->NbElements() == 0 )
  {
    _gen->Compute( *myHelper->GetMesh(), botSM->GetSubShape(), /*aShapeOnly=*/true );
    botSMDS = botSM->GetSubMeshDS();
    if ( !botSMDS || botSMDS->NbElements() == 0 )
      return toSM( error(TCom("No elements on face #") << botSM->GetId() ));
  }

  bool needProject = !topSM->IsMeshComputed();
  if ( !needProject &&
       (botSMDS->NbElements() != topSMDS->NbElements() ||
        botSMDS->NbNodes()    != topSMDS->NbNodes()))
  {
    MESSAGE("nb elem bot " << botSMDS->NbElements() <<
            " top " << ( topSMDS ? topSMDS->NbElements() : 0 ));
    MESSAGE("nb node bot " << botSMDS->NbNodes() <<
            " top " << ( topSMDS ? topSMDS->NbNodes() : 0 ));
    return toSM( error(TCom("Mesh on faces #") << botSM->GetId()
                       <<" and #"<< topSM->GetId() << " seems different" ));
  }

  if ( 0/*needProject && !myProjectTriangles*/ )
    return toSM( error(TCom("Mesh on faces #") << botSM->GetId()
                       <<" and #"<< topSM->GetId() << " seems different" ));
  ///RETURN_BAD_RESULT("Need to project but not allowed");

  NSProjUtils::TNodeNodeMap n2nMap;
  const NSProjUtils::TNodeNodeMap* n2nMapPtr = & n2nMap;
  if ( needProject )
  {
    if ( !projectBottomToTop( bottomToTopTrsf, thePrism ))
      return false;
    n2nMapPtr = & TProjction2dAlgo::instance( this )->GetNodesMap();
  }

  if ( !n2nMapPtr || n2nMapPtr->size() < botSMDS->NbNodes() )
  {
    // associate top and bottom faces
    NSProjUtils::TShapeShapeMap shape2ShapeMap;
    const bool sameTopo =
      NSProjUtils::FindSubShapeAssociation( thePrism.myBottom, myHelper->GetMesh(),
                                            thePrism.myTop,    myHelper->GetMesh(),
                                            shape2ShapeMap);
    if ( !sameTopo )
      for ( size_t iQ = 0; iQ < thePrism.myWallQuads.size(); ++iQ )
      {
        const Prism_3D::TQuadList& quadList = thePrism.myWallQuads[iQ];
        StdMeshers_FaceSidePtr      botSide = quadList.front()->side[ QUAD_BOTTOM_SIDE ];
        StdMeshers_FaceSidePtr      topSide = quadList.back ()->side[ QUAD_TOP_SIDE ];
        if ( botSide->NbEdges() == topSide->NbEdges() )
        {
          for ( int iE = 0; iE < botSide->NbEdges(); ++iE )
          {
            NSProjUtils::InsertAssociation( botSide->Edge( iE ),
                                            topSide->Edge( iE ), shape2ShapeMap );
            NSProjUtils::InsertAssociation( myHelper->IthVertex( 0, botSide->Edge( iE )),
                                            myHelper->IthVertex( 0, topSide->Edge( iE )),
                                            shape2ShapeMap );
          }
        }
        else
        {
          TopoDS_Vertex vb, vt;
          StdMeshers_FaceSidePtr sideB, sideT;
          vb = myHelper->IthVertex( 0, botSide->Edge( 0 ));
          vt = myHelper->IthVertex( 0, topSide->Edge( 0 ));
          sideB = quadList.front()->side[ QUAD_LEFT_SIDE ];
          sideT = quadList.back ()->side[ QUAD_LEFT_SIDE ];
          if ( vb.IsSame( sideB->FirstVertex() ) &&
               vt.IsSame( sideT->LastVertex() ))
          {
            NSProjUtils::InsertAssociation( botSide->Edge( 0 ),
                                            topSide->Edge( 0 ), shape2ShapeMap );
            NSProjUtils::InsertAssociation( vb, vt, shape2ShapeMap );
          }
          vb = myHelper->IthVertex( 1, botSide->Edge( botSide->NbEdges()-1 ));
          vt = myHelper->IthVertex( 1, topSide->Edge( topSide->NbEdges()-1 ));
          sideB = quadList.front()->side[ QUAD_RIGHT_SIDE ];
          sideT = quadList.back ()->side[ QUAD_RIGHT_SIDE ];
          if ( vb.IsSame( sideB->FirstVertex() ) &&
               vt.IsSame( sideT->LastVertex() ))
          {
            NSProjUtils::InsertAssociation( botSide->Edge( botSide->NbEdges()-1 ),
                                            topSide->Edge( topSide->NbEdges()-1 ),
                                            shape2ShapeMap );
            NSProjUtils::InsertAssociation( vb, vt, shape2ShapeMap );
          }
        }
      }

    // Find matching nodes of top and bottom faces
    n2nMapPtr = & n2nMap;
    if ( ! NSProjUtils::FindMatchingNodesOnFaces( thePrism.myBottom, myHelper->GetMesh(),
                                                  thePrism.myTop,    myHelper->GetMesh(),
                                                  shape2ShapeMap, n2nMap ))
    {
      if ( sameTopo )
        return toSM( error(TCom("Mesh on faces #") << botSM->GetId()
                           <<" and #"<< topSM->GetId() << " seems different" ));
      else
        return toSM( error(TCom("Topology of faces #") << botSM->GetId()
                           <<" and #"<< topSM->GetId() << " seems different" ));
    }
  }

  // Fill myBotToColumnMap

  int zSize = myBlock.VerticalSize();
  TNodeNodeMap::const_iterator bN_tN = n2nMapPtr->begin();
  for ( ; bN_tN != n2nMapPtr->end(); ++bN_tN )
  {
    const SMDS_MeshNode* botNode = bN_tN->first;
    const SMDS_MeshNode* topNode = bN_tN->second;
    if ( botNode->GetPosition()->GetTypeOfPosition() != SMDS_TOP_FACE )
      continue; // wall columns are contained in myBlock
    // create node column
    Prism_3D::TNode bN( botNode );
    TNode2ColumnMap::iterator bN_col =
      myBotToColumnMap.insert( make_pair ( bN, TNodeColumn() )).first;
    TNodeColumn & column = bN_col->second;
    column.resize( zSize );
    column.front() = botNode;
    column.back()  = topNode;
  }
  return true;
}

//================================================================================
/*!
 * \brief Remove faces from the top face and re-create them by projection from the bottom
 * \retval bool - a success or not
 */
//================================================================================

bool StdMeshers_Prism_3D::projectBottomToTop( const gp_Trsf &             bottomToTopTrsf,
                                              const Prism_3D::TPrismTopo& thePrism )
{
  if ( project2dMesh( thePrism.myBottom, thePrism.myTop ))
  {
    return true;
  }
  NSProjUtils::TNodeNodeMap& n2nMap = 
    (NSProjUtils::TNodeNodeMap&) TProjction2dAlgo::instance( this )->GetNodesMap();
  n2nMap.clear();

  myUseBlock = true;

  SMESHDS_Mesh*  meshDS = myHelper->GetMeshDS();
  SMESH_subMesh * botSM = myHelper->GetMesh()->GetSubMesh( thePrism.myBottom );
  SMESH_subMesh * topSM = myHelper->GetMesh()->GetSubMesh( thePrism.myTop );

  SMESHDS_SubMesh * botSMDS = botSM->GetSubMeshDS();
  SMESHDS_SubMesh * topSMDS = topSM->GetSubMeshDS();

  if ( topSMDS && topSMDS->NbElements() > 0 )
  {
    //topSM->ComputeStateEngine( SMESH_subMesh::CLEAN ); -- avoid propagation of events
    for ( SMDS_ElemIteratorPtr eIt = topSMDS->GetElements(); eIt->more(); )
      meshDS->RemoveFreeElement( eIt->next(), topSMDS, /*fromGroups=*/false );
    for ( SMDS_NodeIteratorPtr nIt = topSMDS->GetNodes(); nIt->more(); )
      meshDS->RemoveFreeNode( nIt->next(), topSMDS, /*fromGroups=*/false );
  }

  const TopoDS_Face& botFace = thePrism.myBottom; // oriented within
  const TopoDS_Face& topFace = thePrism.myTop;    //    the 3D SHAPE
  int topFaceID = meshDS->ShapeToIndex( thePrism.myTop );

  SMESH_MesherHelper botHelper( *myHelper->GetMesh() );
  botHelper.SetSubShape( botFace );
  botHelper.ToFixNodeParameters( true );
  bool checkUV;
  SMESH_MesherHelper topHelper( *myHelper->GetMesh() );
  topHelper.SetSubShape( topFace );
  topHelper.ToFixNodeParameters( true );
  double distXYZ[4], fixTol = 10 * topHelper.MaxTolerance( topFace );

  // Fill myBotToColumnMap

  int zSize = myBlock.VerticalSize();
  Prism_3D::TNode prevTNode;
  SMDS_NodeIteratorPtr nIt = botSMDS->GetNodes();
  while ( nIt->more() )
  {
    const SMDS_MeshNode* botNode = nIt->next();
    const SMDS_MeshNode* topNode = 0;
    if ( botNode->GetPosition()->GetTypeOfPosition() != SMDS_TOP_FACE )
      continue; // strange

    Prism_3D::TNode bN( botNode );
    if ( bottomToTopTrsf.Form() == gp_Identity )
    {
      // compute bottom node params
      gp_XYZ paramHint(-1,-1,-1);
      if ( prevTNode.IsNeighbor( bN ))
      {
        paramHint = prevTNode.GetParams();
        // double tol = 1e-2 * ( prevTNode.GetCoords() - bN.GetCoords() ).Modulus();
        // myBlock.SetTolerance( Min( myBlock.GetTolerance(), tol ));
      }
      if ( !myBlock.ComputeParameters( bN.GetCoords(), bN.ChangeParams(),
                                       ID_BOT_FACE, paramHint ))
        return toSM( error(TCom("Can't compute normalized parameters for node ")
                           << botNode->GetID() << " on the face #"<< botSM->GetId() ));
      prevTNode = bN;
      // compute top node coords
      gp_XYZ topXYZ; gp_XY topUV;
      if ( !myBlock.FacePoint( ID_TOP_FACE, bN.GetParams(), topXYZ ) ||
           !myBlock.FaceUV   ( ID_TOP_FACE, bN.GetParams(), topUV ))
        return toSM( error(TCom("Can't compute coordinates "
                                "by normalized parameters on the face #")<< topSM->GetId() ));
      topNode = meshDS->AddNode( topXYZ.X(),topXYZ.Y(),topXYZ.Z() );
      meshDS->SetNodeOnFace( topNode, topFaceID, topUV.X(), topUV.Y() );
    }
    else // use bottomToTopTrsf
    {
      gp_XYZ coords = bN.GetCoords();
      bottomToTopTrsf.Transforms( coords );
      topNode = meshDS->AddNode( coords.X(), coords.Y(), coords.Z() );
      gp_XY topUV = botHelper.GetNodeUV( botFace, botNode, 0, &checkUV );
      meshDS->SetNodeOnFace( topNode, topFaceID, topUV.X(), topUV.Y() );
      distXYZ[0] = -1;
      if ( topHelper.CheckNodeUV( topFace, topNode, topUV, fixTol, /*force=*/false, distXYZ ) &&
           distXYZ[0] > fixTol && distXYZ[0] < fixTol * 1e+3 )
        meshDS->MoveNode( topNode, distXYZ[1], distXYZ[2], distXYZ[3] ); // transform can be inaccurate
    }
    // create node column
    TNode2ColumnMap::iterator bN_col = 
      myBotToColumnMap.insert( make_pair ( bN, TNodeColumn() )).first;
    TNodeColumn & column = bN_col->second;
    column.resize( zSize );
    column.front() = botNode;
    column.back()  = topNode;

    n2nMap.insert( n2nMap.end(), make_pair( botNode, topNode ));

    if ( _computeCanceled )
      return toSM( error( SMESH_ComputeError::New(COMPERR_CANCELED)));
  }

  // Create top faces

  const bool oldSetElemsOnShape = myHelper->SetElementsOnShape( false );

  // care of orientation;
  // if the bottom faces is orienetd OK then top faces must be reversed
  bool reverseTop = true;
  if ( myHelper->NbAncestors( botFace, *myBlock.Mesh(), TopAbs_SOLID ) > 1 )
    reverseTop = ! myHelper->IsReversedSubMesh( botFace );
  int iFrw, iRev, *iPtr = &( reverseTop ? iRev : iFrw );

  // loop on bottom mesh faces
  SMDS_ElemIteratorPtr faceIt = botSMDS->GetElements();
  vector< const SMDS_MeshNode* > nodes;
  while ( faceIt->more() )
  {
    const SMDS_MeshElement* face = faceIt->next();
    if ( !face || face->GetType() != SMDSAbs_Face )
      continue;

    // find top node in columns for each bottom node
    int nbNodes = face->NbCornerNodes();
    nodes.resize( nbNodes );
    for ( iFrw = 0, iRev = nbNodes-1; iFrw < nbNodes; ++iFrw, --iRev )
    {
      const SMDS_MeshNode* n = face->GetNode( *iPtr );
      if ( n->GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE ) {
        TNode2ColumnMap::iterator bot_column = myBotToColumnMap.find( n );
        if ( bot_column == myBotToColumnMap.end() )
          return toSM( error(TCom("No nodes found above node ") << n->GetID() ));
        nodes[ iFrw ] = bot_column->second.back();
      }
      else {
        const TNodeColumn* column = myBlock.GetNodeColumn( n );
        if ( !column )
          return toSM( error(TCom("No side nodes found above node ") << n->GetID() ));
        nodes[ iFrw ] = column->back();
      }
    }
    SMDS_MeshElement* newFace = 0;
    switch ( nbNodes ) {

    case 3: {
      newFace = myHelper->AddFace(nodes[0], nodes[1], nodes[2]);
      break;
    }
    case 4: {
      newFace = myHelper->AddFace( nodes[0], nodes[1], nodes[2], nodes[3] );
      break;
    }
    default:
      newFace = meshDS->AddPolygonalFace( nodes );
    }
    if ( newFace )
      meshDS->SetMeshElementOnShape( newFace, topFaceID );
  }

  myHelper->SetElementsOnShape( oldSetElemsOnShape );

  // Check the projected mesh

  if ( thePrism.myNbEdgesInWires.size() > 1 && // there are holes
       topHelper.IsDistorted2D( topSM, /*checkUV=*/false ))
  {
    SMESH_MeshEditor editor( topHelper.GetMesh() );

    // smooth in 2D or 3D?
    TopLoc_Location loc;
    Handle(Geom_Surface) surface = BRep_Tool::Surface( topFace, loc );
    bool isPlanar = GeomLib_IsPlanarSurface( surface ).IsPlanar();

    bool isFixed = false;
    set<const SMDS_MeshNode*> fixedNodes;
    for ( int iAttemp = 0; !isFixed && iAttemp < 10; ++iAttemp )
    {
      TIDSortedElemSet faces;
      for ( faceIt = topSMDS->GetElements(); faceIt->more(); )
        faces.insert( faces.end(), faceIt->next() );

      SMESH_MeshEditor::SmoothMethod algo =
        iAttemp ? SMESH_MeshEditor::CENTROIDAL : SMESH_MeshEditor::LAPLACIAN;

      // smoothing
      editor.Smooth( faces, fixedNodes, algo, /*nbIterations=*/ 10,
                     /*theTgtAspectRatio=*/1.0, /*the2D=*/!isPlanar);

      isFixed = !topHelper.IsDistorted2D( topSM, /*checkUV=*/true );
    }
    if ( !isFixed )
      return toSM( error( TCom("Projection from face #") << botSM->GetId()
                          << " to face #" << topSM->GetId()
                          << " failed: inverted elements created"));
  }

  return true;
}

//=======================================================================
//function : getSweepTolerance
//purpose  : Compute tolerance to pass to StdMeshers_Sweeper
//=======================================================================

double StdMeshers_Prism_3D::getSweepTolerance( const Prism_3D::TPrismTopo& thePrism )
{
  SMESHDS_Mesh*    meshDS = myHelper->GetMeshDS();
  SMESHDS_SubMesh * sm[2] = { meshDS->MeshElements( thePrism.myBottom ),
                              meshDS->MeshElements( thePrism.myTop )    };
  double minDist = 1e100;

  vector< SMESH_TNodeXYZ > nodes;
  for ( int iSM = 0; iSM < 2; ++iSM )
  {
    if ( !sm[ iSM ]) continue;

    SMDS_ElemIteratorPtr fIt = sm[ iSM ]->GetElements();
    while ( fIt->more() )
    {
      const SMDS_MeshElement* face = fIt->next();
      const int            nbNodes = face->NbCornerNodes();
      SMDS_ElemIteratorPtr     nIt = face->nodesIterator();

      nodes.resize( nbNodes + 1 );
      for ( int iN = 0; iN < nbNodes; ++iN )
        nodes[ iN ] = nIt->next();
      nodes.back() = nodes[0];
      
      // loop on links
      double dist2;
      for ( int iN = 0; iN < nbNodes; ++iN )
      {
        if ( nodes[ iN   ]._node->GetPosition()->GetDim() < 2 &&
             nodes[ iN+1 ]._node->GetPosition()->GetDim() < 2 )
        {
          // it's a boundary link; measure distance of other
          // nodes to this link
          gp_XYZ linkDir = nodes[ iN ] - nodes[ iN+1 ];
          double linkLen = linkDir.Modulus();
          bool   isDegen = ( linkLen < numeric_limits<double>::min() );
          if ( !isDegen ) linkDir /= linkLen;
          for ( int iN2 = 0; iN2 < nbNodes; ++iN2 ) // loop on other nodes
          {
            if ( nodes[ iN2 ] == nodes[ iN ] ||
                 nodes[ iN2 ] == nodes[ iN+1 ]) continue;
            if ( isDegen )
            {
              dist2 = ( nodes[ iN ] - nodes[ iN2 ]).SquareModulus();
            }
            else
            {
              dist2 = linkDir.CrossSquareMagnitude( nodes[ iN ] - nodes[ iN2 ]);
            }
            if ( dist2 > numeric_limits<double>::min() )
              minDist = Min ( minDist, dist2 );
          }
        }
        // measure length link
        else if ( nodes[ iN ]._node < nodes[ iN+1 ]._node ) // not to measure same link twice
        {
          dist2 = ( nodes[ iN ] - nodes[ iN+1 ]).SquareModulus();
          if ( dist2 > numeric_limits<double>::min() )
            minDist = Min ( minDist, dist2 );
        }
      }
    }
  }
  return 0.1 * Sqrt ( minDist );
}

//=======================================================================
//function : isSimpleQuad
//purpose  : check if the bottom FACE is meshable with nice qudrangles,
//           if so the block aproach can work rather fast.
//           This is a temporary mean caused by problems in StdMeshers_Sweeper
//=======================================================================

bool StdMeshers_Prism_3D::isSimpleBottom( const Prism_3D::TPrismTopo& thePrism )
{
  // analyse angles between edges
  double nbConcaveAng = 0, nbConvexAng = 0;
  TopoDS_Face reverseBottom = TopoDS::Face( thePrism.myBottom.Reversed() ); // see initPrism()
  TopoDS_Vertex commonV;
  const list< TopoDS_Edge >& botEdges = thePrism.myBottomEdges;
  list< TopoDS_Edge >::const_iterator edge = botEdges.begin();
  while ( edge != botEdges.end() )
  {
    if ( SMESH_Algo::isDegenerated( *edge ))
      return false;
    TopoDS_Edge e1 = *edge++;
    TopoDS_Edge e2 = ( edge == botEdges.end() ? botEdges.front() : *edge );
    if ( ! TopExp::CommonVertex( e1, e2,  commonV ))
    {
      e2 = botEdges.front();
      if ( ! TopExp::CommonVertex( e1, e2,  commonV ))
        break;
    }
    double angle = myHelper->GetAngle( e1, e2, reverseBottom, commonV );
    if ( angle < -5 * M_PI/180 )
      if ( ++nbConcaveAng > 1 )
        return false;
    if ( angle > 85 * M_PI/180 )
      if ( ++nbConvexAng > 4 )
        return false;
  }
  return true;
}

//=======================================================================
//function : project2dMesh
//purpose  : Project mesh faces from a source FACE of one prism (theSrcFace)
//           to a source FACE of another prism (theTgtFace)
//=======================================================================

bool StdMeshers_Prism_3D::project2dMesh(const TopoDS_Face& theSrcFace,
                                        const TopoDS_Face& theTgtFace)
{
  TProjction2dAlgo* projector2D = TProjction2dAlgo::instance( this );
  projector2D->myHyp.SetSourceFace( theSrcFace );
  bool ok = projector2D->Compute( *myHelper->GetMesh(), theTgtFace );

  SMESH_subMesh* tgtSM = myHelper->GetMesh()->GetSubMesh( theTgtFace );
  if ( !ok && tgtSM->GetSubMeshDS() ) {
    //tgtSM->ComputeStateEngine( SMESH_subMesh::CLEAN ); -- avoid propagation of events
    SMESHDS_Mesh*     meshDS = myHelper->GetMeshDS();
    SMESHDS_SubMesh* tgtSMDS = tgtSM->GetSubMeshDS();
    for ( SMDS_ElemIteratorPtr eIt = tgtSMDS->GetElements(); eIt->more(); )
      meshDS->RemoveFreeElement( eIt->next(), tgtSMDS, /*fromGroups=*/false );
    for ( SMDS_NodeIteratorPtr nIt = tgtSMDS->GetNodes(); nIt->more(); )
      meshDS->RemoveFreeNode( nIt->next(), tgtSMDS, /*fromGroups=*/false );
  }
  tgtSM->ComputeStateEngine       ( SMESH_subMesh::CHECK_COMPUTE_STATE );
  tgtSM->ComputeSubMeshStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );

  return ok;
}

//================================================================================
/*!
 * \brief Set projection coordinates of a node to a face and it's sub-shapes
 * \param faceID - the face given by in-block ID
 * \param params - node normalized parameters
 * \retval bool - is a success
 */
//================================================================================

bool StdMeshers_Prism_3D::setFaceAndEdgesXYZ( const int faceID, const gp_XYZ& params, int z )
{
  // find base and top edges of the face
  enum { BASE = 0, TOP, LEFT, RIGHT };
  vector< int > edgeVec; // 0-base, 1-top
  SMESH_Block::GetFaceEdgesIDs( faceID, edgeVec );

  myBlock.EdgePoint( edgeVec[ BASE ], params, myShapeXYZ[ edgeVec[ BASE ]]);
  myBlock.EdgePoint( edgeVec[ TOP  ], params, myShapeXYZ[ edgeVec[ TOP ]]);

  SHOWYXZ("\nparams ", params);
  SHOWYXZ("TOP is " <<edgeVec[ TOP ], myShapeXYZ[ edgeVec[ TOP]]);
  SHOWYXZ("BASE is "<<edgeVec[ BASE], myShapeXYZ[ edgeVec[ BASE]]);

  if ( faceID == SMESH_Block::ID_Fx0z || faceID == SMESH_Block::ID_Fx1z )
  {
    myBlock.EdgePoint( edgeVec[ LEFT ], params, myShapeXYZ[ edgeVec[ LEFT ]]);
    myBlock.EdgePoint( edgeVec[ RIGHT ], params, myShapeXYZ[ edgeVec[ RIGHT ]]);

    SHOWYXZ("VER "<<edgeVec[ LEFT], myShapeXYZ[ edgeVec[ LEFT]]);
    SHOWYXZ("VER "<<edgeVec[ RIGHT], myShapeXYZ[ edgeVec[ RIGHT]]);
  }
  myBlock.FacePoint( faceID, params, myShapeXYZ[ faceID ]);
  SHOWYXZ("FacePoint "<<faceID, myShapeXYZ[ faceID]);

  return true;
}

//=======================================================================
//function : toSM
//purpose  : If (!isOK), sets the error to a sub-mesh of a current SOLID
//=======================================================================

bool StdMeshers_Prism_3D::toSM( bool isOK )
{
  if ( mySetErrorToSM &&
       !isOK &&
       myHelper &&
       !myHelper->GetSubShape().IsNull() &&
       myHelper->GetSubShape().ShapeType() == TopAbs_SOLID)
  {
    SMESH_subMesh* sm = myHelper->GetMesh()->GetSubMesh( myHelper->GetSubShape() );
    sm->GetComputeError() = this->GetComputeError();
    // clear error in order not to return it twice
    _error = COMPERR_OK;
    _comment.clear();
  }
  return isOK;
}

//=======================================================================
//function : shapeID
//purpose  : Return index of a shape
//=======================================================================

int StdMeshers_Prism_3D::shapeID( const TopoDS_Shape& S )
{
  if ( S.IsNull() ) return 0;
  if ( !myHelper  ) return -3;
  return myHelper->GetMeshDS()->ShapeToIndex( S );
}

namespace // utils used by StdMeshers_Prism_3D::IsApplicable()
{
  struct EdgeWithNeighbors
  {
    TopoDS_Edge _edge;
    int         _iL, _iR;
    EdgeWithNeighbors(const TopoDS_Edge& E, int iE, int nbE, int shift = 0 ):
      _edge( E ),
      _iL( SMESH_MesherHelper::WrapIndex( iE-1, nbE ) + shift ),
      _iR( SMESH_MesherHelper::WrapIndex( iE+1, nbE ) + shift )
    {
    }
    EdgeWithNeighbors() {}
  };
  struct PrismSide
  {
    TopoDS_Face                 _face;
    TopTools_IndexedMapOfShape *_faces; // pointer because its copy constructor is private
    TopoDS_Edge                 _topEdge;
    vector< EdgeWithNeighbors >*_edges;
    int                         _iBotEdge;
    vector< bool >              _isCheckedEdge;
    int                         _nbCheckedEdges; // nb of EDGEs whose location is defined
    PrismSide                  *_leftSide;
    PrismSide                  *_rightSide;
    const TopoDS_Edge& Edge( int i ) const
    {
      return (*_edges)[ i ]._edge;
    }
    int FindEdge( const TopoDS_Edge& E ) const
    {
      for ( size_t i = 0; i < _edges->size(); ++i )
        if ( E.IsSame( Edge( i ))) return i;
      return -1;
    }
    bool IsSideFace( const TopoDS_Shape& face ) const
    {
      if ( _faces->Contains( face )) // avoid returning true for a prism top FACE
        return ( !_face.IsNull() || !( face.IsSame( _faces->FindKey( _faces->Extent() ))));
      return false;
    }
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Return ordered edges of a face
   */
  bool getEdges( const TopoDS_Face&            face,
                 vector< EdgeWithNeighbors > & edges,
                 const bool                    noHolesAllowed)
  {
    list< TopoDS_Edge > ee;
    list< int >         nbEdgesInWires;
    int nbW = SMESH_Block::GetOrderedEdges( face, ee, nbEdgesInWires );
    if ( nbW > 1 && noHolesAllowed )
      return false;

    int iE, nbTot = 0;
    list< TopoDS_Edge >::iterator e = ee.begin();
    list< int >::iterator       nbE = nbEdgesInWires.begin();
    for ( ; nbE != nbEdgesInWires.end(); ++nbE )
      for ( iE = 0; iE < *nbE; ++e, ++iE )
        if ( SMESH_Algo::isDegenerated( *e ))
        {
          e = --ee.erase( e );
          --(*nbE);
          --iE;
        }
        else
        {
          e->Orientation( TopAbs_FORWARD ); // for operator==() to work
        }

    edges.clear();
    e = ee.begin();
    for ( nbE = nbEdgesInWires.begin(); nbE != nbEdgesInWires.end(); ++nbE )
    {
      for ( iE = 0; iE < *nbE; ++e, ++iE )
        edges.push_back( EdgeWithNeighbors( *e, iE, *nbE, nbTot ));
      nbTot += *nbE;
    }
    return edges.size();
  }
  //--------------------------------------------------------------------------------
  /*!
   * \brief Return another faces sharing an edge
   */
  const TopoDS_Shape & getAnotherFace( const TopoDS_Face& face,
                                       const TopoDS_Edge& edge,
                                       TopTools_IndexedDataMapOfShapeListOfShape& facesOfEdge)
  {
    TopTools_ListIteratorOfListOfShape faceIt( facesOfEdge.FindFromKey( edge ));
    for ( ; faceIt.More(); faceIt.Next() )
      if ( !face.IsSame( faceIt.Value() ))
        return faceIt.Value();
    return face;
  }
}

//================================================================================
/*!
 * \brief Return true if the algorithm can mesh this shape
 *  \param [in] aShape - shape to check
 *  \param [in] toCheckAll - if true, this check returns OK if all shapes are OK,
 *              else, returns OK if at least one shape is OK
 */
//================================================================================

bool StdMeshers_Prism_3D::IsApplicable(const TopoDS_Shape & shape, bool toCheckAll)
{
  TopExp_Explorer sExp( shape, TopAbs_SOLID );
  if ( !sExp.More() )
    return false;

  for ( ; sExp.More(); sExp.Next() )
  {
    // check nb shells
    TopoDS_Shape shell;
    TopExp_Explorer shExp( sExp.Current(), TopAbs_SHELL );
    if ( shExp.More() ) {
      shell = shExp.Current();
      shExp.Next();
      if ( shExp.More() )
        shell.Nullify();
    }
    if ( shell.IsNull() ) {
      if ( toCheckAll ) return false;
      continue;
    }
    // get all faces
    TopTools_IndexedMapOfShape allFaces;
    TopExp::MapShapes( shell, TopAbs_FACE, allFaces );
    if ( allFaces.Extent() < 3 ) {
      if ( toCheckAll ) return false;
      continue;
    }
    // is a box?
    if ( allFaces.Extent() == 6 )
    {
      TopTools_IndexedMapOfOrientedShape map;
      bool isBox = SMESH_Block::FindBlockShapes( TopoDS::Shell( shell ),
                                                 TopoDS_Vertex(), TopoDS_Vertex(), map );
      if ( isBox ) {
        if ( !toCheckAll ) return true;
        continue;
      }
    }
#ifdef _DEBUG_
    TopTools_IndexedMapOfShape allShapes;
    TopExp::MapShapes( shape, allShapes );
#endif

    TopTools_IndexedDataMapOfShapeListOfShape facesOfEdge;
    TopTools_ListIteratorOfListOfShape faceIt;
    TopExp::MapShapesAndAncestors( sExp.Current(), TopAbs_EDGE, TopAbs_FACE , facesOfEdge );
    if ( facesOfEdge.IsEmpty() ) {
      if ( toCheckAll ) return false;
      continue;
    }

    typedef vector< EdgeWithNeighbors > TEdgeWithNeighborsVec;
    vector< TEdgeWithNeighborsVec > faceEdgesVec( allFaces.Extent() + 1 );
    TopTools_IndexedMapOfShape* facesOfSide = new TopTools_IndexedMapOfShape[ faceEdgesVec.size() ];
    SMESHUtils::ArrayDeleter<TopTools_IndexedMapOfShape> delFacesOfSide( facesOfSide );

    // try to use each face as a bottom one
    bool prismDetected = false;
    for ( int iF = 1; iF < allFaces.Extent() && !prismDetected; ++iF )
    {
      const TopoDS_Face& botF = TopoDS::Face( allFaces( iF ));

      TEdgeWithNeighborsVec& botEdges = faceEdgesVec[ iF ];
      if ( botEdges.empty() )
        if ( !getEdges( botF, botEdges, /*noHoles=*/false ))
          break;
      if ( allFaces.Extent()-1 <= (int) botEdges.size() )
        continue; // all faces are adjacent to botF - no top FACE

      // init data of side FACEs
      vector< PrismSide > sides( botEdges.size() );
      for ( int iS = 0; iS < botEdges.size(); ++iS )
      {
        sides[ iS ]._topEdge = botEdges[ iS ]._edge;
        sides[ iS ]._face    = botF;
        sides[ iS ]._leftSide  = & sides[ botEdges[ iS ]._iR ];
        sides[ iS ]._rightSide = & sides[ botEdges[ iS ]._iL ];
        sides[ iS ]._faces = & facesOfSide[ iS ];
        sides[ iS ]._faces->Clear();
      }

      bool isOK = true; // ok for a current botF
      bool isAdvanced = true; // is new data found in a current loop
      int  nbFoundSideFaces = 0;
      for ( int iLoop = 0; isOK && isAdvanced; ++iLoop )
      {
        isAdvanced = false;
        for ( size_t iS = 0; iS < sides.size() && isOK; ++iS )
        {
          PrismSide& side = sides[ iS ];
          if ( side._face.IsNull() )
            continue; // probably the prism top face is the last of side._faces

          if ( side._topEdge.IsNull() )
          {
            // find vertical EDGEs --- EGDEs shared with neighbor side FACEs
            for ( int is2nd = 0; is2nd < 2 && isOK; ++is2nd ) // 2 adjacent neighbors
            {
              int di = is2nd ? 1 : -1;
              const PrismSide* adjSide = is2nd ? side._rightSide : side._leftSide;
              for ( size_t i = 1; i < side._edges->size(); ++i )
              {
                int iE = SMESH_MesherHelper::WrapIndex( i*di + side._iBotEdge, side._edges->size());
                if ( side._isCheckedEdge[ iE ] ) continue;
                const TopoDS_Edge&      vertE = side.Edge( iE );
                const TopoDS_Shape& neighborF = getAnotherFace( side._face, vertE, facesOfEdge );
                bool isEdgeShared = adjSide->IsSideFace( neighborF );
                if ( isEdgeShared )
                {
                  isAdvanced = true;
                  side._isCheckedEdge[ iE ] = true;
                  side._nbCheckedEdges++;
                  int nbNotCheckedE = side._edges->size() - side._nbCheckedEdges;
                  if ( nbNotCheckedE == 1 )
                    break;
                }
                else
                {
                  if ( i == 1 && iLoop == 0 ) isOK = false;
                  break;
                }
              }
            }
            // find a top EDGE
            int nbNotCheckedE = side._edges->size() - side._nbCheckedEdges;
            if ( nbNotCheckedE == 1 )
            {
              vector<bool>::iterator ii = std::find( side._isCheckedEdge.begin(),
                                                     side._isCheckedEdge.end(), false );
              if ( ii != side._isCheckedEdge.end() )
              {
                size_t iE = std::distance( side._isCheckedEdge.begin(), ii );
                side._topEdge = side.Edge( iE );
              }
            }
            isOK = ( nbNotCheckedE >= 1 );
          }
          else //if ( !side._topEdge.IsNull() )
          {
            // get a next face of a side
            const TopoDS_Shape& f = getAnotherFace( side._face, side._topEdge, facesOfEdge );
            side._faces->Add( f );
            bool stop = false;
            if ( f.IsSame( side._face ) || // _topEdge is a seam
                 SMESH_MesherHelper::Count( f, TopAbs_WIRE, false ) != 1 )
            {
              stop = true;
            }
            else if ( side._leftSide != & side ) // not closed side face
            {
              if ( side._leftSide->_faces->Contains( f ))
              {
                stop = true; // probably f is the prism top face
                side._leftSide->_face.Nullify();
                side._leftSide->_topEdge.Nullify();
              }
              if ( side._rightSide->_faces->Contains( f ))
              {
                stop = true; // probably f is the prism top face
                side._rightSide->_face.Nullify();
                side._rightSide->_topEdge.Nullify();
              }
            }
            if ( stop )
            {
              side._face.Nullify();
              side._topEdge.Nullify();
              continue;
            }
            side._face  = TopoDS::Face( f );
            int faceID  = allFaces.FindIndex( side._face );
            side._edges = & faceEdgesVec[ faceID ];
            if ( side._edges->empty() )
              if ( !getEdges( side._face, * side._edges, /*noHoles=*/true ))
                break;
            const int nbE = side._edges->size();
            if ( nbE >= 4 )
            {
              isAdvanced = true;
              ++nbFoundSideFaces;
              side._iBotEdge = side.FindEdge( side._topEdge );
              side._isCheckedEdge.clear();
              side._isCheckedEdge.resize( nbE, false );
              side._isCheckedEdge[ side._iBotEdge ] = true;
              side._nbCheckedEdges = 1; // bottom EDGE is known
            }
            side._topEdge.Nullify();
            isOK = ( !side._edges->empty() || side._faces->Extent() > 1 );

          } //if ( !side._topEdge.IsNull() )

        } // loop on prism sides

        if ( nbFoundSideFaces > allFaces.Extent() )
        {
          isOK = false;
        }
        if ( iLoop > allFaces.Extent() * 10 )
        {
          isOK = false;
#ifdef _DEBUG_
          cerr << "BUG: infinite loop in StdMeshers_Prism_3D::IsApplicable()" << endl;
#endif
        }
      } // while isAdvanced

      if ( isOK && sides[0]._faces->Extent() > 1 )
      {
        const int nbFaces = sides[0]._faces->Extent();
        if ( botEdges.size() == 1 ) // cylinder
        {
          prismDetected = ( nbFaces == allFaces.Extent()-1 );
        }
        else
        {
          const TopoDS_Shape& topFace = sides[0]._faces->FindKey( nbFaces );
          size_t iS;
          for ( iS = 1; iS < sides.size(); ++iS )
            if ( !sides[ iS ]._faces->Contains( topFace ))
              break;
          prismDetected = ( iS == sides.size() );
        }
      }
    } // loop on allFaces

    if ( !prismDetected && toCheckAll ) return false;
    if ( prismDetected && !toCheckAll ) return true;

  } // loop on solids

  return toCheckAll;
}

namespace Prism_3D
{
  //================================================================================
  /*!
   * \brief Return true if this node and other one belong to one face
   */
  //================================================================================

  bool Prism_3D::TNode::IsNeighbor( const Prism_3D::TNode& other ) const
  {
    if ( !other.myNode || !myNode ) return false;

    SMDS_ElemIteratorPtr fIt = other.myNode->GetInverseElementIterator(SMDSAbs_Face);
    while ( fIt->more() )
      if ( fIt->next()->GetNodeIndex( myNode ) >= 0 )
        return true;
    return false;
  }

  //================================================================================
  /*!
   * \brief Prism initialization
   */
  //================================================================================

  void TPrismTopo::Clear()
  {
    myShape3D.Nullify();
    myTop.Nullify();
    myBottom.Nullify();
    myWallQuads.clear();
    myBottomEdges.clear();
    myNbEdgesInWires.clear();
    myWallQuads.clear();
  }

  //================================================================================
  /*!
   * \brief Set upside-down
   */
  //================================================================================

  void TPrismTopo::SetUpsideDown()
  {
    std::swap( myBottom, myTop );
    myBottomEdges.clear();
    std::reverse( myBottomEdges.begin(), myBottomEdges.end() );
    for ( size_t i = 0; i < myWallQuads.size(); ++i )
    {
      myWallQuads[i].reverse();
      TQuadList::iterator q = myWallQuads[i].begin();
      for ( ; q != myWallQuads[i].end(); ++q )
      {
        (*q)->shift( 2, /*keepUnitOri=*/true );
      }
      myBottomEdges.push_back( myWallQuads[i].front()->side[ QUAD_BOTTOM_SIDE ].grid->Edge(0) );
    }
  }

} // namespace Prism_3D

//================================================================================
/*!
 * \brief Constructor. Initialization is needed
 */
//================================================================================

StdMeshers_PrismAsBlock::StdMeshers_PrismAsBlock()
{
  mySide = 0;
}

StdMeshers_PrismAsBlock::~StdMeshers_PrismAsBlock()
{
  Clear();
}
void StdMeshers_PrismAsBlock::Clear()
{
  myHelper = 0;
  myShapeIDMap.Clear();
  myError.reset();

  if ( mySide ) {
    delete mySide; mySide = 0;
  }
  myParam2ColumnMaps.clear();
  myShapeIndex2ColumnMap.clear();
}

//=======================================================================
//function : initPrism
//purpose  : Analyse shape geometry and mesh.
//           If there are triangles on one of faces, it becomes 'bottom'.
//           thePrism.myBottom can be already set up.
//=======================================================================

bool StdMeshers_Prism_3D::initPrism(Prism_3D::TPrismTopo& thePrism,
                                    const TopoDS_Shape&   theShape3D,
                                    const bool            selectBottom)
{
  myHelper->SetSubShape( theShape3D );

  SMESH_subMesh* mainSubMesh = myHelper->GetMesh()->GetSubMeshContaining( theShape3D );
  if ( !mainSubMesh ) return toSM( error(COMPERR_BAD_INPUT_MESH,"Null submesh of shape3D"));

  // detect not-quad FACE sub-meshes of the 3D SHAPE
  list< SMESH_subMesh* > notQuadGeomSubMesh;
  list< SMESH_subMesh* > notQuadElemSubMesh;
  list< SMESH_subMesh* > meshedSubMesh;
  int nbFaces = 0;
  //
  SMESH_subMesh* anyFaceSM = 0;
  SMESH_subMeshIteratorPtr smIt = mainSubMesh->getDependsOnIterator(false,true);
  while ( smIt->more() )
  {
    SMESH_subMesh* sm = smIt->next();
    const TopoDS_Shape& face = sm->GetSubShape();
    if      ( face.ShapeType() > TopAbs_FACE ) break;
    else if ( face.ShapeType() < TopAbs_FACE ) continue;
    nbFaces++;
    anyFaceSM = sm;

    // is quadrangle FACE?
    list< TopoDS_Edge > orderedEdges;
    list< int >         nbEdgesInWires;
    int nbWires = SMESH_Block::GetOrderedEdges( TopoDS::Face( face ), orderedEdges,
                                                nbEdgesInWires );
    if ( nbWires != 1 || nbEdgesInWires.front() != 4 )
      notQuadGeomSubMesh.push_back( sm );

    // look for a not structured sub-mesh
    if ( !sm->IsEmpty() )
    {
      meshedSubMesh.push_back( sm );
      if ( !myHelper->IsSameElemGeometry( sm->GetSubMeshDS(), SMDSGeom_QUADRANGLE ) ||
           !myHelper->IsStructured      ( sm ))
        notQuadElemSubMesh.push_back( sm );
    }
  }

  int nbNotQuadMeshed = notQuadElemSubMesh.size();
  int       nbNotQuad = notQuadGeomSubMesh.size();
  bool     hasNotQuad = ( nbNotQuad || nbNotQuadMeshed );

  // detect bad cases
  if ( nbNotQuadMeshed > 2 )
  {
    return toSM( error(COMPERR_BAD_INPUT_MESH,
                       TCom("More than 2 faces with not quadrangle elements: ")
                       <<nbNotQuadMeshed));
  }
  if ( nbNotQuad > 2 || !thePrism.myBottom.IsNull() )
  {
    // Issue 0020843 - one of side FACEs is quasi-quadrilateral (not 4 EDGEs).
    // Remove from notQuadGeomSubMesh faces meshed with regular grid
    int nbQuasiQuads = removeQuasiQuads( notQuadGeomSubMesh, myHelper,
                                         TQuadrangleAlgo::instance(this,myHelper) );
    nbNotQuad -= nbQuasiQuads;
    if ( nbNotQuad > 2 )
      return toSM( error(COMPERR_BAD_SHAPE,
                         TCom("More than 2 not quadrilateral faces: ") <<nbNotQuad));
    hasNotQuad = ( nbNotQuad || nbNotQuadMeshed );
  }

  // Analyse mesh and topology of FACEs: choose the bottom sub-mesh.
  // If there are not quadrangle FACEs, they are top and bottom ones.
  // Not quadrangle FACEs must be only on top and bottom.

  SMESH_subMesh * botSM = 0;
  SMESH_subMesh * topSM = 0;

  if ( hasNotQuad ) // can choose a bottom FACE
  {
    if ( nbNotQuadMeshed > 0 ) botSM = notQuadElemSubMesh.front();
    else                       botSM = notQuadGeomSubMesh.front();
    if ( nbNotQuadMeshed > 1 ) topSM = notQuadElemSubMesh.back();
    else if ( nbNotQuad  > 1 ) topSM = notQuadGeomSubMesh.back();

    if ( topSM == botSM ) {
      if ( nbNotQuadMeshed > 1 ) topSM = notQuadElemSubMesh.front();
      else                       topSM = notQuadGeomSubMesh.front();
    }

    // detect mesh triangles on wall FACEs
    if ( nbNotQuad == 2 && nbNotQuadMeshed > 0 ) {
      bool ok = false;
      if ( nbNotQuadMeshed == 1 )
        ok = ( find( notQuadGeomSubMesh.begin(),
                     notQuadGeomSubMesh.end(), botSM ) != notQuadGeomSubMesh.end() );
      else
        ok = ( notQuadGeomSubMesh == notQuadElemSubMesh );
      if ( !ok )
        return toSM( error(COMPERR_BAD_INPUT_MESH,
                           "Side face meshed with not quadrangle elements"));
    }
  }

  thePrism.myNotQuadOnTop = ( nbNotQuadMeshed > 1 );

  // use thePrism.myBottom
  if ( !thePrism.myBottom.IsNull() )
  {
    if ( botSM ) { // <-- not quad geom or mesh on botSM
      if ( ! botSM->GetSubShape().IsSame( thePrism.myBottom )) {
        std::swap( botSM, topSM );
        if ( !botSM || ! botSM->GetSubShape().IsSame( thePrism.myBottom )) {
          if ( !selectBottom )
            return toSM( error( COMPERR_BAD_INPUT_MESH,
                                "Incompatible non-structured sub-meshes"));
          std::swap( botSM, topSM );
          thePrism.myBottom = TopoDS::Face( botSM->GetSubShape() );
        }
      }
    }
    else if ( !selectBottom ) {
      botSM = myHelper->GetMesh()->GetSubMesh( thePrism.myBottom );
    }
  }
  if ( !botSM ) // find a proper bottom
  {
    bool savedSetErrorToSM = mySetErrorToSM;
    mySetErrorToSM = false; // ingore errors in initPrism()

    // search among meshed FACEs
    list< SMESH_subMesh* >::iterator sm = meshedSubMesh.begin();
    for ( ; !botSM && sm != meshedSubMesh.end(); ++sm )
    {
      thePrism.Clear();
      botSM             = *sm;
      thePrism.myBottom = TopoDS::Face( botSM->GetSubShape() );
      if ( !initPrism( thePrism, theShape3D, /*selectBottom=*/false ))
        botSM = NULL;
    }
    // search among all FACEs
    for ( TopExp_Explorer f( theShape3D, TopAbs_FACE ); !botSM && f.More(); f.Next() )
    {
      int minNbFaces = 2 + myHelper->Count( f.Current(), TopAbs_EDGE, false);
      if ( nbFaces < minNbFaces) continue;
      thePrism.Clear();
      thePrism.myBottom = TopoDS::Face( f.Current() );
      botSM             = myHelper->GetMesh()->GetSubMesh( thePrism.myBottom );
      if ( !initPrism( thePrism, theShape3D, /*selectBottom=*/false ))
        botSM = NULL;
    }
    mySetErrorToSM = savedSetErrorToSM;
    return botSM ? true : toSM( error( COMPERR_BAD_SHAPE ));
  }

  // find vertex 000 - the one with smallest coordinates (for easy DEBUG :-)
  TopoDS_Vertex V000;
  double minVal = DBL_MAX, minX, val;
  for ( TopExp_Explorer exp( botSM->GetSubShape(), TopAbs_VERTEX );
        exp.More(); exp.Next() )
  {
    const TopoDS_Vertex& v = TopoDS::Vertex( exp.Current() );
    gp_Pnt P = BRep_Tool::Pnt( v );
    val = P.X() + P.Y() + P.Z();
    if ( val < minVal || ( val == minVal && P.X() < minX )) {
      V000 = v;
      minVal = val;
      minX = P.X();
    }
  }

  thePrism.myShape3D = theShape3D;
  if ( thePrism.myBottom.IsNull() )
    thePrism.myBottom  = TopoDS::Face( botSM->GetSubShape() );
  thePrism.myBottom.Orientation( myHelper->GetSubShapeOri( theShape3D, thePrism.myBottom ));
  thePrism.myTop.   Orientation( myHelper->GetSubShapeOri( theShape3D, thePrism.myTop ));

  // Get ordered bottom edges
  TopoDS_Face reverseBottom = // to have order of top EDGEs as in the top FACE
    TopoDS::Face( thePrism.myBottom.Reversed() );
  SMESH_Block::GetOrderedEdges( reverseBottom,
                                thePrism.myBottomEdges,
                                thePrism.myNbEdgesInWires, V000 );

  // Get Wall faces corresponding to the ordered bottom edges and the top FACE
  if ( !getWallFaces( thePrism, nbFaces )) // it also sets thePrism.myTop
    return false; //toSM( error(COMPERR_BAD_SHAPE, "Can't find side faces"));

  if ( topSM )
  {
    if ( !thePrism.myTop.IsSame( topSM->GetSubShape() ))
      return toSM( error
                   (notQuadGeomSubMesh.empty() ? COMPERR_BAD_INPUT_MESH : COMPERR_BAD_SHAPE,
                    "Non-quadrilateral faces are not opposite"));

    // check that the found top and bottom FACEs are opposite
    list< TopoDS_Edge >::iterator edge = thePrism.myBottomEdges.begin();
    for ( ; edge != thePrism.myBottomEdges.end(); ++edge )
      if ( myHelper->IsSubShape( *edge, thePrism.myTop ))
        return toSM( error
                     (notQuadGeomSubMesh.empty() ? COMPERR_BAD_INPUT_MESH : COMPERR_BAD_SHAPE,
                      "Non-quadrilateral faces are not opposite"));
  }

  if ( thePrism.myBottomEdges.size() > thePrism.myWallQuads.size() )
  {
    // composite bottom sides => set thePrism upside-down
    thePrism.SetUpsideDown();
  }

  return true;
}

//================================================================================
/*!
 * \brief Initialization.
 * \param helper - helper loaded with mesh and 3D shape
 * \param thePrism - a prism data
 * \retval bool - false if a mesh or a shape are KO
 */
//================================================================================

bool StdMeshers_PrismAsBlock::Init(SMESH_MesherHelper*         helper,
                                   const Prism_3D::TPrismTopo& thePrism)
{
  myHelper = helper;
  SMESHDS_Mesh* meshDS = myHelper->GetMeshDS();
  SMESH_Mesh*     mesh = myHelper->GetMesh();

  if ( mySide ) {
    delete mySide; mySide = 0;
  }
  vector< TSideFace* >         sideFaces( NB_WALL_FACES, 0 );
  vector< pair< double, double> > params( NB_WALL_FACES );
  mySide = new TSideFace( *mesh, sideFaces, params );


  SMESH_Block::init();
  myShapeIDMap.Clear();
  myShapeIndex2ColumnMap.clear();
  
  int wallFaceIds[ NB_WALL_FACES ] = { // to walk around a block
    SMESH_Block::ID_Fx0z, SMESH_Block::ID_F1yz,
    SMESH_Block::ID_Fx1z, SMESH_Block::ID_F0yz
  };

  myError = SMESH_ComputeError::New();

  myNotQuadOnTop = thePrism.myNotQuadOnTop;

  // Find columns of wall nodes and calculate edges' lengths
  // --------------------------------------------------------

  myParam2ColumnMaps.clear();
  myParam2ColumnMaps.resize( thePrism.myBottomEdges.size() ); // total nb edges

  size_t iE, nbEdges = thePrism.myNbEdgesInWires.front(); // nb outer edges
  vector< double > edgeLength( nbEdges );
  multimap< double, int > len2edgeMap;

  // for each EDGE: either split into several parts, or join with several next EDGEs
  vector<int> nbSplitPerEdge( nbEdges, 0 );
  vector<int> nbUnitePerEdge( nbEdges, 0 ); // -1 means "joined to a previous"

  // consider continuous straight EDGEs as one side
  const int nbSides = countNbSides( thePrism, nbUnitePerEdge, edgeLength );

  list< TopoDS_Edge >::const_iterator edgeIt = thePrism.myBottomEdges.begin();
  for ( iE = 0; iE < nbEdges; ++iE, ++edgeIt )
  {
    TParam2ColumnMap & faceColumns = myParam2ColumnMaps[ iE ];

    Prism_3D::TQuadList::const_iterator quad = thePrism.myWallQuads[ iE ].begin();
    for ( ; quad != thePrism.myWallQuads[ iE ].end(); ++quad )
    {
      const TopoDS_Edge& quadBot = (*quad)->side[ QUAD_BOTTOM_SIDE ].grid->Edge( 0 );
      if ( !myHelper->LoadNodeColumns( faceColumns, (*quad)->face, quadBot, meshDS ))
        return error(COMPERR_BAD_INPUT_MESH, TCom("Can't find regular quadrangle mesh ")
                     << "on a side face #" << MeshDS()->ShapeToIndex( (*quad)->face ));
    }
    SHOWYXZ("\np1 F " <<iE, gpXYZ(faceColumns.begin()->second.front() ));
    SHOWYXZ("p2 F "   <<iE, gpXYZ(faceColumns.rbegin()->second.front() ));
    SHOWYXZ("V First "<<iE, BRep_Tool::Pnt( TopExp::FirstVertex(*edgeIt,true )));

    if ( nbSides < NB_WALL_FACES ) // fill map used to split faces
      len2edgeMap.insert( make_pair( edgeLength[ iE ], iE )); // sort edges by length
  }
  // Load columns of internal edges (forming holes)
  // and fill map ShapeIndex to TParam2ColumnMap for them
  for ( ; edgeIt != thePrism.myBottomEdges.end() ; ++edgeIt, ++iE )
  {
    TParam2ColumnMap & faceColumns = myParam2ColumnMaps[ iE ];

    Prism_3D::TQuadList::const_iterator quad = thePrism.myWallQuads[ iE ].begin();
    for ( ; quad != thePrism.myWallQuads[ iE ].end(); ++quad )
    {
      const TopoDS_Edge& quadBot = (*quad)->side[ QUAD_BOTTOM_SIDE ].grid->Edge( 0 );
      if ( !myHelper->LoadNodeColumns( faceColumns, (*quad)->face, quadBot, meshDS ))
        return error(COMPERR_BAD_INPUT_MESH, TCom("Can't find regular quadrangle mesh ")
                     << "on a side face #" << MeshDS()->ShapeToIndex( (*quad)->face ));
    }
    // edge columns
    int id = MeshDS()->ShapeToIndex( *edgeIt );
    bool isForward = true; // meaningless for intenal wires
    myShapeIndex2ColumnMap[ id ] = make_pair( & faceColumns, isForward );
    // columns for vertices
    // 1
    const SMDS_MeshNode* n0 = faceColumns.begin()->second.front();
    id = n0->getshapeId();
    myShapeIndex2ColumnMap[ id ] = make_pair( & faceColumns, isForward );
    // 2
    const SMDS_MeshNode* n1 = faceColumns.rbegin()->second.front();
    id = n1->getshapeId();
    myShapeIndex2ColumnMap[ id ] = make_pair( & faceColumns, isForward );

    // SHOWYXZ("\np1 F " <<iE, gpXYZ(faceColumns.begin()->second.front() ));
    // SHOWYXZ("p2 F "   <<iE, gpXYZ(faceColumns.rbegin()->second.front() ));
    // SHOWYXZ("V First "<<iE, BRep_Tool::Pnt( TopExp::FirstVertex(*edgeIt,true )));
  }

  // Create 4 wall faces of a block
  // -------------------------------

  if ( nbSides <= NB_WALL_FACES ) // ************* Split faces if necessary
  {
    if ( nbSides != NB_WALL_FACES ) // define how to split
    {
      if ( len2edgeMap.size() != nbEdges )
        RETURN_BAD_RESULT("Uniqueness of edge lengths not assured");

      multimap< double, int >::reverse_iterator maxLen_i = len2edgeMap.rbegin();
      multimap< double, int >::reverse_iterator midLen_i = ++len2edgeMap.rbegin();

      double maxLen = maxLen_i->first;
      double midLen = ( len2edgeMap.size() == 1 ) ? 0 : midLen_i->first;
      switch ( nbEdges ) {
      case 1: // 0-th edge is split into 4 parts
        nbSplitPerEdge[ 0 ] = 4;
        break;
      case 2: // either the longest edge is split into 3 parts, or both edges into halves
        if ( maxLen / 3 > midLen / 2 ) {
          nbSplitPerEdge[ maxLen_i->second ] = 3;
        }
        else {
          nbSplitPerEdge[ maxLen_i->second ] = 2;
          nbSplitPerEdge[ midLen_i->second ] = 2;
        }
        break;
      case 3:
        if ( nbSides == 2 )
          // split longest into 3 parts
          nbSplitPerEdge[ maxLen_i->second ] = 3;
        else
          // split longest into halves
          nbSplitPerEdge[ maxLen_i->second ] = 2;
      }
    }
  }
  else // **************************** Unite faces
  {
    int nbExraFaces = nbSides - 4; // nb of faces to fuse
    for ( iE = 0; iE < nbEdges; ++iE )
    {
      if ( nbUnitePerEdge[ iE ] < 0 )
        continue;
      // look for already united faces
      for ( int i = iE; i < iE + nbExraFaces; ++i )
      {
        if ( nbUnitePerEdge[ i ] > 0 ) // a side including nbUnitePerEdge[i]+1 edge
          nbExraFaces += nbUnitePerEdge[ i ];
        nbUnitePerEdge[ i ] = -1;
      }
      nbUnitePerEdge[ iE ] = nbExraFaces;
      break;
    }
  }

  // Create TSideFace's
  int iSide = 0;
  list< TopoDS_Edge >::const_iterator botE = thePrism.myBottomEdges.begin();
  for ( iE = 0; iE < nbEdges; ++iE, ++botE )
  {
    TFaceQuadStructPtr quad = thePrism.myWallQuads[ iE ].front();
    const int       nbSplit = nbSplitPerEdge[ iE ];
    const int   nbExraFaces = nbUnitePerEdge[ iE ] + 1;
    if ( nbSplit > 0 ) // split
    {
      vector< double > params;
      splitParams( nbSplit, &myParam2ColumnMaps[ iE ], params );
      const bool isForward =
        StdMeshers_PrismAsBlock::IsForwardEdge( myHelper->GetMeshDS(),
                                                myParam2ColumnMaps[iE],
                                                *botE, SMESH_Block::ID_Fx0z );
      for ( int i = 0; i < nbSplit; ++i ) {
        double f = ( isForward ? params[ i ]   : params[ nbSplit - i-1 ]);
        double l = ( isForward ? params[ i+1 ] : params[ nbSplit - i ]);
        TSideFace* comp = new TSideFace( *mesh, wallFaceIds[ iSide ],
                                         thePrism.myWallQuads[ iE ], *botE,
                                         &myParam2ColumnMaps[ iE ], f, l );
        mySide->SetComponent( iSide++, comp );
      }
    }
    else if ( nbExraFaces > 1 ) // unite
    {
      double u0 = 0, sumLen = 0;
      for ( int i = iE; i < iE + nbExraFaces; ++i )
        sumLen += edgeLength[ i ];

      vector< TSideFace* >        components( nbExraFaces );
      vector< pair< double, double> > params( nbExraFaces );
      bool endReached = false;
      for ( int i = 0; i < nbExraFaces; ++i, ++botE, ++iE )
      {
        if ( iE == nbEdges )
        {
          endReached = true;
          botE = thePrism.myBottomEdges.begin();
          iE = 0;
        }
        components[ i ] = new TSideFace( *mesh, wallFaceIds[ iSide ],
                                         thePrism.myWallQuads[ iE ], *botE,
                                         &myParam2ColumnMaps[ iE ]);
        double u1 = u0 + edgeLength[ iE ] / sumLen;
        params[ i ] = make_pair( u0 , u1 );
        u0 = u1;
      }
      TSideFace* comp = new TSideFace( *mesh, components, params );
      mySide->SetComponent( iSide++, comp );
      if ( endReached )
        break;
      --iE; // for increment in an external loop on iE
      --botE;
    }
    else if ( nbExraFaces < 0 ) // skip already united face
    {
    }
    else // use as is
    {
      TSideFace* comp = new TSideFace( *mesh, wallFaceIds[ iSide ],
                                       thePrism.myWallQuads[ iE ], *botE,
                                       &myParam2ColumnMaps[ iE ]);
      mySide->SetComponent( iSide++, comp );
    }
  }


  // Fill geometry fields of SMESH_Block
  // ------------------------------------

  vector< int > botEdgeIdVec;
  SMESH_Block::GetFaceEdgesIDs( ID_BOT_FACE, botEdgeIdVec );

  bool isForward[NB_WALL_FACES] = { true, true, true, true };
  Adaptor2d_Curve2d* botPcurves[NB_WALL_FACES];
  Adaptor2d_Curve2d* topPcurves[NB_WALL_FACES];

  for ( int iF = 0; iF < NB_WALL_FACES; ++iF )
  {
    TSideFace * sideFace = mySide->GetComponent( iF );
    if ( !sideFace )
      RETURN_BAD_RESULT("NULL TSideFace");
    int fID = sideFace->FaceID(); // in-block ID

    // fill myShapeIDMap
    if ( sideFace->InsertSubShapes( myShapeIDMap ) != 8 &&
         !sideFace->IsComplex())
      MESSAGE( ": Warning : InsertSubShapes() < 8 on side " << iF );

    // side faces geometry
    Adaptor2d_Curve2d* pcurves[NB_WALL_FACES];
    if ( !sideFace->GetPCurves( pcurves ))
      RETURN_BAD_RESULT("TSideFace::GetPCurves() failed");

    SMESH_Block::TFace& tFace = myFace[ fID - ID_FirstF ];
    tFace.Set( fID, sideFace->Surface(), pcurves, isForward );

    SHOWYXZ( endl<<"F "<< iF << " id " << fID << " FRW " << sideFace->IsForward(), sideFace->Value(0,0));
    // edges 3D geometry
    vector< int > edgeIdVec;
    SMESH_Block::GetFaceEdgesIDs( fID, edgeIdVec );
    for ( int isMax = 0; isMax < 2; ++isMax ) {
      {
        int eID = edgeIdVec[ isMax ];
        SMESH_Block::TEdge& tEdge = myEdge[ eID - ID_FirstE ];
        tEdge.Set( eID, sideFace->HorizCurve(isMax), true);
        SHOWYXZ(eID<<" HOR"<<isMax<<"(0)", sideFace->HorizCurve(isMax)->Value(0));
        SHOWYXZ(eID<<" HOR"<<isMax<<"(1)", sideFace->HorizCurve(isMax)->Value(1));
      }
      {
        int eID = edgeIdVec[ isMax+2 ];
        SMESH_Block::TEdge& tEdge = myEdge[ eID - ID_FirstE  ];
        tEdge.Set( eID, sideFace->VertiCurve(isMax), true);
        SHOWYXZ(eID<<" VER"<<isMax<<"(0)", sideFace->VertiCurve(isMax)->Value(0));
        SHOWYXZ(eID<<" VER"<<isMax<<"(1)", sideFace->VertiCurve(isMax)->Value(1));

        // corner points
        vector< int > vertexIdVec;
        SMESH_Block::GetEdgeVertexIDs( eID, vertexIdVec );
        myPnt[ vertexIdVec[0] - ID_FirstV ] = tEdge.GetCurve()->Value(0).XYZ();
        myPnt[ vertexIdVec[1] - ID_FirstV ] = tEdge.GetCurve()->Value(1).XYZ();
      }
    }
    // pcurves on horizontal faces
    for ( iE = 0; iE < NB_WALL_FACES; ++iE ) {
      if ( edgeIdVec[ BOTTOM_EDGE ] == botEdgeIdVec[ iE ] ) {
        botPcurves[ iE ] = sideFace->HorizPCurve( false, thePrism.myBottom );
        topPcurves[ iE ] = sideFace->HorizPCurve( true,  thePrism.myTop );
        break;
      }
    }
    //sideFace->dumpNodes( 4 ); // debug
  }
  // horizontal faces geometry
  {
    SMESH_Block::TFace& tFace = myFace[ ID_BOT_FACE - ID_FirstF ];
    tFace.Set( ID_BOT_FACE, new BRepAdaptor_Surface( thePrism.myBottom ), botPcurves, isForward );
    SMESH_Block::Insert( thePrism.myBottom, ID_BOT_FACE, myShapeIDMap );
  }
  {
    SMESH_Block::TFace& tFace = myFace[ ID_TOP_FACE - ID_FirstF ];
    tFace.Set( ID_TOP_FACE, new BRepAdaptor_Surface( thePrism.myTop ), topPcurves, isForward );
    SMESH_Block::Insert( thePrism.myTop, ID_TOP_FACE, myShapeIDMap );
  }
  //faceGridToPythonDump( SMESH_Block::ID_Fxy0, 50 );
  //faceGridToPythonDump( SMESH_Block::ID_Fxy1 );

  // Fill map ShapeIndex to TParam2ColumnMap
  // ----------------------------------------

  list< TSideFace* > fList;
  list< TSideFace* >::iterator fListIt;
  fList.push_back( mySide );
  for ( fListIt = fList.begin(); fListIt != fList.end(); ++fListIt)
  {
    int nb = (*fListIt)->NbComponents();
    for ( int i = 0; i < nb; ++i ) {
      if ( TSideFace* comp = (*fListIt)->GetComponent( i ))
        fList.push_back( comp );
    }
    if ( TParam2ColumnMap* cols = (*fListIt)->GetColumns()) {
      // columns for a base edge
      int id = MeshDS()->ShapeToIndex( (*fListIt)->BaseEdge() );
      bool isForward = (*fListIt)->IsForward();
      myShapeIndex2ColumnMap[ id ] = make_pair( cols, isForward );

      // columns for vertices
      const SMDS_MeshNode* n0 = cols->begin()->second.front();
      id = n0->getshapeId();
      myShapeIndex2ColumnMap[ id ] = make_pair( cols, isForward );

      const SMDS_MeshNode* n1 = cols->rbegin()->second.front();
      id = n1->getshapeId();
      myShapeIndex2ColumnMap[ id ] = make_pair( cols, !isForward );
    }
  }

// #define SHOWYXZ(msg, xyz) {                     \
//     gp_Pnt p (xyz);                                                     \
//     cout << msg << " ("<< p.X() << "; " <<p.Y() << "; " <<p.Z() << ") " <<endl; \
//   }
//   double _u[]={ 0.1, 0.1, 0.9, 0.9 };
//   double _v[]={ 0.1, 0.9, 0.1, 0.9 };
//   for ( int z = 0; z < 2; ++z )
//     for ( int i = 0; i < 4; ++i )
//     {
//       //gp_XYZ testPar(0.25, 0.25, 0), testCoord;
//       int iFace = (z ? ID_TOP_FACE : ID_BOT_FACE);
//       gp_XYZ testPar(_u[i], _v[i], z), testCoord;
//       if ( !FacePoint( iFace, testPar, testCoord ))
//         RETURN_BAD_RESULT("TEST FacePoint() FAILED");
//       SHOWYXZ("IN TEST PARAM" , testPar);
//       SHOWYXZ("OUT TEST CORD" , testCoord);
//       if ( !ComputeParameters( testCoord, testPar , iFace))
//         RETURN_BAD_RESULT("TEST ComputeParameters() FAILED");
//       SHOWYXZ("OUT TEST PARAM" , testPar);
//     }
  return true;
}

//================================================================================
/*!
 * \brief Return pointer to column of nodes
 * \param node - bottom node from which the returned column goes up
 * \retval const TNodeColumn* - the found column
 */
//================================================================================

const TNodeColumn* StdMeshers_PrismAsBlock::GetNodeColumn(const SMDS_MeshNode* node) const
{
  int sID = node->getshapeId();

  map<int, pair< TParam2ColumnMap*, bool > >::const_iterator col_frw =
    myShapeIndex2ColumnMap.find( sID );
  if ( col_frw != myShapeIndex2ColumnMap.end() ) {
    const TParam2ColumnMap* cols = col_frw->second.first;
    TParam2ColumnIt u_col = cols->begin();
    for ( ; u_col != cols->end(); ++u_col )
      if ( u_col->second[ 0 ] == node )
        return & u_col->second;
  }
  return 0;
}

//=======================================================================
//function : GetLayersTransformation
//purpose  : Return transformations to get coordinates of nodes of each layer
//           by nodes of the bottom. Layer is a set of nodes at a certain step
//           from bottom to top.
//           Transformation to get top node from bottom ones is computed
//           only if the top FACE is not meshed.
//=======================================================================

bool StdMeshers_PrismAsBlock::GetLayersTransformation(vector<gp_Trsf> &           trsf,
                                                      const Prism_3D::TPrismTopo& prism) const
{
  const bool itTopMeshed = !SubMesh( ID_BOT_FACE )->IsEmpty();
  const int zSize = VerticalSize();
  if ( zSize < 3 && !itTopMeshed ) return true;
  trsf.resize( zSize - 1 );

  // Select some node columns by which we will define coordinate system of layers

  vector< const TNodeColumn* > columns;
  {
    bool isReverse;
    list< TopoDS_Edge >::const_iterator edgeIt = prism.myBottomEdges.begin();
    for ( int iE = 0; iE < prism.myNbEdgesInWires.front(); ++iE, ++edgeIt )
    {
      if ( SMESH_Algo::isDegenerated( *edgeIt )) continue;
      const TParam2ColumnMap* u2colMap =
        GetParam2ColumnMap( MeshDS()->ShapeToIndex( *edgeIt ), isReverse );
      if ( !u2colMap ) return false;
      double f = u2colMap->begin()->first, l = u2colMap->rbegin()->first;
      //isReverse = ( edgeIt->Orientation() == TopAbs_REVERSED );
      //if ( isReverse ) swap ( f, l ); -- u2colMap takes orientation into account
      const int nbCol = 5;
      for ( int i = 0; i < nbCol; ++i )
      {
        double u = f + i/double(nbCol) * ( l - f );
        const TNodeColumn* col = & getColumn( u2colMap, u )->second;
        if ( columns.empty() || col != columns.back() )
          columns.push_back( col );
      }
    }
  }

  // Find tolerance to check transformations

  double tol2;
  {
    Bnd_B3d bndBox;
    for ( int i = 0; i < columns.size(); ++i )
      bndBox.Add( gpXYZ( columns[i]->front() ));
    tol2 = bndBox.SquareExtent() * 1e-5;
  }

  // Compute transformations

  int xCol = -1;
  gp_Trsf fromCsZ, toCs0;
  gp_Ax3 cs0 = getLayerCoordSys(0, columns, xCol );
  //double dist0 = cs0.Location().Distance( gpXYZ( (*columns[0])[0]));
  toCs0.SetTransformation( cs0 );
  for ( int z = 1; z < zSize; ++z )
  {
    gp_Ax3 csZ = getLayerCoordSys(z, columns, xCol );
    //double distZ = csZ.Location().Distance( gpXYZ( (*columns[0])[z]));
    fromCsZ.SetTransformation( csZ );
    fromCsZ.Invert();
    gp_Trsf& t = trsf[ z-1 ];
    t = fromCsZ * toCs0;
    //t.SetScaleFactor( distZ/dist0 ); - it does not work properly, wrong base point

    // check a transformation
    for ( int i = 0; i < columns.size(); ++i )
    {
      gp_Pnt p0 = gpXYZ( (*columns[i])[0] );
      gp_Pnt pz = gpXYZ( (*columns[i])[z] );
      t.Transforms( p0.ChangeCoord() );
      if ( p0.SquareDistance( pz ) > tol2 )
      {
        t = gp_Trsf();
        return ( z == zSize - 1 ); // OK if fails only botton->top trsf
      }
    }
  }
  return true;
}

//================================================================================
/*!
 * \brief Check curve orientation of a bottom edge
  * \param meshDS - mesh DS
  * \param columnsMap - node columns map of side face
  * \param bottomEdge - the bottom edge
  * \param sideFaceID - side face in-block ID
  * \retval bool - true if orientation coinside with in-block forward orientation
 */
//================================================================================

bool StdMeshers_PrismAsBlock::IsForwardEdge(SMESHDS_Mesh*           meshDS,
                                            const TParam2ColumnMap& columnsMap,
                                            const TopoDS_Edge &     bottomEdge,
                                            const int               sideFaceID)
{
  bool isForward = false;
  if ( SMESH_MesherHelper::IsClosedEdge( bottomEdge ))
  {
    isForward = ( bottomEdge.Orientation() == TopAbs_FORWARD );
  }
  else
  {
    const TNodeColumn&     firstCol = columnsMap.begin()->second;
    const SMDS_MeshNode* bottomNode = firstCol[0];
    TopoDS_Shape firstVertex = SMESH_MesherHelper::GetSubShapeByNode( bottomNode, meshDS );
    isForward = ( firstVertex.IsSame( TopExp::FirstVertex( bottomEdge, true )));
  }
  // on 2 of 4 sides first vertex is end
  if ( sideFaceID == ID_Fx1z || sideFaceID == ID_F0yz )
    isForward = !isForward;
  return isForward;
}

//=======================================================================
//function : faceGridToPythonDump
//purpose  : Prints a script creating a normal grid on the prism side
//=======================================================================

void StdMeshers_PrismAsBlock::faceGridToPythonDump(const SMESH_Block::TShapeID face,
                                                   const int                   nb)
{
#ifdef _DEBUG_
  gp_XYZ pOnF[6] = { gp_XYZ(0,0,0), gp_XYZ(0,0,1),
                     gp_XYZ(0,0,0), gp_XYZ(0,1,0),
                     gp_XYZ(0,0,0), gp_XYZ(1,0,0) };
  gp_XYZ p2;
  cout << "mesh = smesh.Mesh( 'Face " << face << "')" << endl;
  SMESH_Block::TFace& f = myFace[ face - ID_FirstF ];
  gp_XYZ params = pOnF[ face - ID_FirstF ];
  //const int nb = 10; // nb face rows
  for ( int j = 0; j <= nb; ++j )
  {
    params.SetCoord( f.GetVInd(), double( j )/ nb );
    for ( int i = 0; i <= nb; ++i )
    {
      params.SetCoord( f.GetUInd(), double( i )/ nb );
      gp_XYZ p = f.Point( params );
      gp_XY uv = f.GetUV( params );
      cout << "mesh.AddNode( " << p.X() << ", " << p.Y() << ", " << p.Z() << " )"
           << " # " << 1 + i + j * ( nb + 1 )
           << " ( " << i << ", " << j << " ) "
           << " UV( " << uv.X() << ", " << uv.Y() << " )" << endl;
      ShellPoint( params, p2 );
      double dist = ( p2 - p ).Modulus();
      if ( dist > 1e-4 )
        cout << "#### dist from ShellPoint " << dist
             << " (" << p2.X() << ", " << p2.Y() << ", " << p2.Z() << " ) " << endl;
    }
  }
  for ( int j = 0; j < nb; ++j )
    for ( int i = 0; i < nb; ++i )
    {
      int n = 1 + i + j * ( nb + 1 );
      cout << "mesh.AddFace([ "
           << n << ", " << n+1 << ", "
           << n+nb+2 << ", " << n+nb+1 << "]) " << endl;
    }
  
#endif
}

//================================================================================
/*!
 * \brief Constructor
  * \param faceID - in-block ID
  * \param face - geom FACE
  * \param baseEdge - EDGE proreply oriented in the bottom EDGE !!!
  * \param columnsMap - map of node columns
  * \param first - first normalized param
  * \param last - last normalized param
 */
//================================================================================

StdMeshers_PrismAsBlock::TSideFace::TSideFace(SMESH_Mesh&                mesh,
                                              const int                  faceID,
                                              const Prism_3D::TQuadList& quadList,
                                              const TopoDS_Edge&         baseEdge,
                                              TParam2ColumnMap*          columnsMap,
                                              const double               first,
                                              const double               last):
  myID( faceID ),
  myParamToColumnMap( columnsMap ),
  myHelper( mesh )
{
  myParams.resize( 1 );
  myParams[ 0 ] = make_pair( first, last );
  mySurface     = PSurface( new BRepAdaptor_Surface( quadList.front()->face ));
  myBaseEdge    = baseEdge;
  myIsForward   = StdMeshers_PrismAsBlock::IsForwardEdge( myHelper.GetMeshDS(),
                                                          *myParamToColumnMap,
                                                          myBaseEdge, myID );
  myHelper.SetSubShape( quadList.front()->face );

  if ( quadList.size() > 1 ) // side is vertically composite
  {
    // fill myShapeID2Surf map to enable finding a right surface by any sub-shape ID

    SMESHDS_Mesh* meshDS = myHelper.GetMeshDS();

    TopTools_IndexedDataMapOfShapeListOfShape subToFaces;
    Prism_3D::TQuadList::const_iterator quad = quadList.begin();
    for ( ; quad != quadList.end(); ++quad )
    {
      const TopoDS_Face& face = (*quad)->face;
      TopExp::MapShapesAndAncestors( face, TopAbs_VERTEX, TopAbs_FACE, subToFaces );
      TopExp::MapShapesAndAncestors( face, TopAbs_EDGE,   TopAbs_FACE, subToFaces );
      myShapeID2Surf.insert( make_pair( meshDS->ShapeToIndex( face ),
                                        PSurface( new BRepAdaptor_Surface( face ))));
    }
    for ( int i = 1; i <= subToFaces.Extent(); ++i )
    {
      const TopoDS_Shape&     sub = subToFaces.FindKey( i );
      TopTools_ListOfShape& faces = subToFaces( i );
      int subID  = meshDS->ShapeToIndex( sub );
      int faceID = meshDS->ShapeToIndex( faces.First() );
      myShapeID2Surf.insert ( make_pair( subID, myShapeID2Surf[ faceID ]));
    }
  }
}

//================================================================================
/*!
 * \brief Constructor of a complex side face
 */
//================================================================================

StdMeshers_PrismAsBlock::TSideFace::
TSideFace(SMESH_Mesh&                             mesh,
          const vector< TSideFace* >&             components,
          const vector< pair< double, double> > & params)
  :myID( components[0] ? components[0]->myID : 0 ),
   myParamToColumnMap( 0 ),
   myParams( params ),
   myIsForward( true ),
   myComponents( components ),
   myHelper( mesh )
{
  if ( myID == ID_Fx1z || myID == ID_F0yz )
  {
    // reverse components
    std::reverse( myComponents.begin(), myComponents.end() );
    std::reverse( myParams.begin(),     myParams.end() );
    for ( size_t i = 0; i < myParams.size(); ++i )
    {
      const double f = myParams[i].first;
      const double l = myParams[i].second;
      myParams[i] = make_pair( 1. - l, 1. - f );
    }
  }
}
//================================================================================
/*!
 * \brief Copy constructor
  * \param other - other side
 */
//================================================================================

StdMeshers_PrismAsBlock::TSideFace::TSideFace( const TSideFace& other ):
  myID               ( other.myID ),
  myParamToColumnMap ( other.myParamToColumnMap ),
  mySurface          ( other.mySurface ),
  myBaseEdge         ( other.myBaseEdge ),
  myShapeID2Surf     ( other.myShapeID2Surf ),
  myParams           ( other.myParams ),
  myIsForward        ( other.myIsForward ),
  myComponents       ( other.myComponents.size() ),
  myHelper           ( *other.myHelper.GetMesh() )
{
  for (int i = 0 ; i < myComponents.size(); ++i )
    myComponents[ i ] = new TSideFace( *other.myComponents[ i ]);
}

//================================================================================
/*!
 * \brief Deletes myComponents
 */
//================================================================================

StdMeshers_PrismAsBlock::TSideFace::~TSideFace()
{
  for (int i = 0 ; i < myComponents.size(); ++i )
    if ( myComponents[ i ] )
      delete myComponents[ i ];
}

//================================================================================
/*!
 * \brief Return geometry of the vertical curve
  * \param isMax - true means curve located closer to (1,1,1) block point
  * \retval Adaptor3d_Curve* - curve adaptor
 */
//================================================================================

Adaptor3d_Curve* StdMeshers_PrismAsBlock::TSideFace::VertiCurve(const bool isMax) const
{
  if ( !myComponents.empty() ) {
    if ( isMax )
      return myComponents.back()->VertiCurve(isMax);
    else
      return myComponents.front()->VertiCurve(isMax);
  }
  double f = myParams[0].first, l = myParams[0].second;
  if ( !myIsForward ) std::swap( f, l );
  return new TVerticalEdgeAdaptor( myParamToColumnMap, isMax ? l : f );
}

//================================================================================
/*!
 * \brief Return geometry of the top or bottom curve
  * \param isTop - 
  * \retval Adaptor3d_Curve* - 
 */
//================================================================================

Adaptor3d_Curve* StdMeshers_PrismAsBlock::TSideFace::HorizCurve(const bool isTop) const
{
  return new THorizontalEdgeAdaptor( this, isTop );
}

//================================================================================
/*!
 * \brief Return pcurves
  * \param pcurv - array of 4 pcurves
  * \retval bool - is a success
 */
//================================================================================

bool StdMeshers_PrismAsBlock::TSideFace::GetPCurves(Adaptor2d_Curve2d* pcurv[4]) const
{
  int iEdge[ 4 ] = { BOTTOM_EDGE, TOP_EDGE, V0_EDGE, V1_EDGE };

  for ( int i = 0 ; i < 4 ; ++i ) {
    Handle(Geom2d_Line) line;
    switch ( iEdge[ i ] ) {
    case TOP_EDGE:
      line = new Geom2d_Line( gp_Pnt2d( 0, 1 ), gp::DX2d() ); break;
    case BOTTOM_EDGE:
      line = new Geom2d_Line( gp::Origin2d(), gp::DX2d() ); break;
    case V0_EDGE:
      line = new Geom2d_Line( gp::Origin2d(), gp::DY2d() ); break;
    case V1_EDGE:
      line = new Geom2d_Line( gp_Pnt2d( 1, 0 ), gp::DY2d() ); break;
    }
    pcurv[ i ] = new Geom2dAdaptor_Curve( line, 0, 1 );
  }
  return true;
}

//================================================================================
/*!
 * \brief Returns geometry of pcurve on a horizontal face
  * \param isTop - is top or bottom face
  * \param horFace - a horizontal face
  * \retval Adaptor2d_Curve2d* - curve adaptor
 */
//================================================================================

Adaptor2d_Curve2d*
StdMeshers_PrismAsBlock::TSideFace::HorizPCurve(const bool         isTop,
                                                const TopoDS_Face& horFace) const
{
  return new TPCurveOnHorFaceAdaptor( this, isTop, horFace );
}

//================================================================================
/*!
 * \brief Return a component corresponding to parameter
  * \param U - parameter along a horizontal size
  * \param localU - parameter along a horizontal size of a component
  * \retval TSideFace* - found component
 */
//================================================================================

StdMeshers_PrismAsBlock::TSideFace*
StdMeshers_PrismAsBlock::TSideFace::GetComponent(const double U,double & localU) const
{
  localU = U;
  if ( myComponents.empty() )
    return const_cast<TSideFace*>( this );

  int i;
  for ( i = 0; i < myComponents.size(); ++i )
    if ( U < myParams[ i ].second )
      break;
  if ( i >= myComponents.size() )
    i = myComponents.size() - 1;

  double f = myParams[ i ].first, l = myParams[ i ].second;
  localU = ( U - f ) / ( l - f );
  return myComponents[ i ];
}

//================================================================================
/*!
 * \brief Find node columns for a parameter
  * \param U - parameter along a horizontal edge
  * \param col1 - the 1st found column
  * \param col2 - the 2nd found column
  * \retval r - normalized position of U between the found columns
 */
//================================================================================

double StdMeshers_PrismAsBlock::TSideFace::GetColumns(const double      U,
                                                      TParam2ColumnIt & col1,
                                                      TParam2ColumnIt & col2) const
{
  double u = U, r = 0;
  if ( !myComponents.empty() ) {
    TSideFace * comp = GetComponent(U,u);
    return comp->GetColumns( u, col1, col2 );
  }

  if ( !myIsForward )
    u = 1 - u;
  double f = myParams[0].first, l = myParams[0].second;
  u = f + u * ( l - f );

  col1 = col2 = getColumn( myParamToColumnMap, u );
  if ( ++col2 == myParamToColumnMap->end() ) {
    --col2;
    r = 0.5;
  }
  else {
    double uf = col1->first;
    double ul = col2->first;
    r = ( u - uf ) / ( ul - uf );
  }
  return r;
}

//================================================================================
/*!
 * \brief Return all nodes at a given height together with their normalized parameters
 *  \param [in] Z - the height of interest
 *  \param [out] nodes - map of parameter to node
 */
//================================================================================

void StdMeshers_PrismAsBlock::
TSideFace::GetNodesAtZ(const int Z,
                       map<double, const SMDS_MeshNode* >& nodes ) const
{
  if ( !myComponents.empty() )
  {
    double u0 = 0.;
    for ( size_t i = 0; i < myComponents.size(); ++i )
    {
      map<double, const SMDS_MeshNode* > nn;
      myComponents[i]->GetNodesAtZ( Z, nn );
      map<double, const SMDS_MeshNode* >::iterator u2n = nn.begin();
      if ( !nodes.empty() && nodes.rbegin()->second == u2n->second )
        ++u2n;
      const double uRange = myParams[i].second - myParams[i].first;
      for ( ; u2n != nn.end(); ++u2n )
        nodes.insert( nodes.end(), make_pair( u0 + uRange * u2n->first, u2n->second ));
      u0 += uRange;
    }
  }
  else
  {
    double f = myParams[0].first, l = myParams[0].second;
    if ( !myIsForward )
      std::swap( f, l );
    const double uRange = l - f;
    if ( Abs( uRange ) < std::numeric_limits<double>::min() )
      return;
    TParam2ColumnIt u2col = getColumn( myParamToColumnMap, myParams[0].first + 1e-3 );
    for ( ; u2col != myParamToColumnMap->end(); ++u2col )
      if ( u2col->first > myParams[0].second + 1e-9 )
        break;
      else
        nodes.insert( nodes.end(),
                      make_pair( ( u2col->first - f ) / uRange, u2col->second[ Z ] ));
  }
}

//================================================================================
/*!
 * \brief Return coordinates by normalized params
  * \param U - horizontal param
  * \param V - vertical param
  * \retval gp_Pnt - result point
 */
//================================================================================

gp_Pnt StdMeshers_PrismAsBlock::TSideFace::Value(const Standard_Real U,
                                                 const Standard_Real V) const
{
  if ( !myComponents.empty() ) {
    double u;
    TSideFace * comp = GetComponent(U,u);
    return comp->Value( u, V );
  }

  TParam2ColumnIt u_col1, u_col2;
  double vR, hR = GetColumns( U, u_col1, u_col2 );

  const SMDS_MeshNode* nn[4];

  // BEGIN issue 0020680: Bad cell created by Radial prism in center of torus
  // Workaround for a wrongly located point returned by mySurface.Value() for
  // UV located near boundary of BSpline surface.
  // To bypass the problem, we take point from 3D curve of EDGE.
  // It solves pb of the bloc_fiss_new.py
  const double tol = 1e-3;
  if ( V < tol || V+tol >= 1. )
  {
    nn[0] = V < tol ? u_col1->second.front() : u_col1->second.back();
    nn[2] = V < tol ? u_col2->second.front() : u_col2->second.back();
    TopoDS_Edge edge;
    if ( V < tol )
    {
      edge = myBaseEdge;
    }
    else
    {
      TopoDS_Shape s = myHelper.GetSubShapeByNode( nn[0], myHelper.GetMeshDS() );
      if ( s.ShapeType() != TopAbs_EDGE )
        s = myHelper.GetSubShapeByNode( nn[2], myHelper.GetMeshDS() );
      if ( !s.IsNull() && s.ShapeType() == TopAbs_EDGE )
        edge = TopoDS::Edge( s );
    }
    if ( !edge.IsNull() )
    {
      double u1 = myHelper.GetNodeU( edge, nn[0], nn[2] );
      double u3 = myHelper.GetNodeU( edge, nn[2], nn[0] );
      double u = u1 * ( 1 - hR ) + u3 * hR;
      TopLoc_Location loc; double f,l;
      Handle(Geom_Curve) curve = BRep_Tool::Curve( edge,loc,f,l );
      return curve->Value( u ).Transformed( loc );
    }
  }
  // END issue 0020680: Bad cell created by Radial prism in center of torus

  vR = getRAndNodes( & u_col1->second, V, nn[0], nn[1] );
  vR = getRAndNodes( & u_col2->second, V, nn[2], nn[3] );

  if ( !myShapeID2Surf.empty() ) // side is vertically composite
  {
    // find a FACE on which the 4 nodes lie
    TSideFace* me = (TSideFace*) this;
    int notFaceID1 = 0, notFaceID2 = 0;
    for ( int i = 0; i < 4; ++i )
      if ( nn[i]->GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE ) // node on FACE
      {
        me->mySurface = me->myShapeID2Surf[ nn[i]->getshapeId() ];
        notFaceID2 = 0;
        break;
      }
      else if ( notFaceID1 == 0 ) // node on EDGE or VERTEX
      {
        me->mySurface  = me->myShapeID2Surf[ nn[i]->getshapeId() ];
        notFaceID1 = nn[i]->getshapeId();
      }
      else if ( notFaceID1 != nn[i]->getshapeId() ) // node on other EDGE or VERTEX
      {
        if ( mySurface != me->myShapeID2Surf[ nn[i]->getshapeId() ])
          notFaceID2 = nn[i]->getshapeId();
      }
    if ( notFaceID2 ) // no nodes of FACE and nodes are on different FACEs
    {
      SMESHDS_Mesh* meshDS = myHelper.GetMeshDS();
      TopoDS_Shape face = myHelper.GetCommonAncestor( meshDS->IndexToShape( notFaceID1 ),
                                                       meshDS->IndexToShape( notFaceID2 ),
                                                       *myHelper.GetMesh(),
                                                       TopAbs_FACE );
      if ( face.IsNull() ) 
        throw SALOME_Exception("StdMeshers_PrismAsBlock::TSideFace::Value() face.IsNull()");
      int faceID = meshDS->ShapeToIndex( face );
      me->mySurface = me->myShapeID2Surf[ faceID ];
      if ( !mySurface )
        throw SALOME_Exception("StdMeshers_PrismAsBlock::TSideFace::Value() !mySurface");
    }
  }
  ((TSideFace*) this)->myHelper.SetSubShape( mySurface->Face() );

  gp_XY uv1 = myHelper.GetNodeUV( mySurface->Face(), nn[0], nn[2]);
  gp_XY uv2 = myHelper.GetNodeUV( mySurface->Face(), nn[1], nn[3]);
  gp_XY uv12 = uv1 * ( 1 - vR ) + uv2 * vR;

  gp_XY uv3 = myHelper.GetNodeUV( mySurface->Face(), nn[2], nn[0]);
  gp_XY uv4 = myHelper.GetNodeUV( mySurface->Face(), nn[3], nn[1]);
  gp_XY uv34 = uv3 * ( 1 - vR ) + uv4 * vR;

  gp_XY uv = uv12 * ( 1 - hR ) + uv34 * hR;

  gp_Pnt p = mySurface->Value( uv.X(), uv.Y() );
  return p;
}


//================================================================================
/*!
 * \brief Return boundary edge
  * \param edge - edge index
  * \retval TopoDS_Edge - found edge
 */
//================================================================================

TopoDS_Edge StdMeshers_PrismAsBlock::TSideFace::GetEdge(const int iEdge) const
{
  if ( !myComponents.empty() ) {
    switch ( iEdge ) {
    case V0_EDGE : return myComponents.front()->GetEdge( iEdge );
    case V1_EDGE : return myComponents.back() ->GetEdge( iEdge );
    default: return TopoDS_Edge();
    }
  }
  TopoDS_Shape edge;
  const SMDS_MeshNode* node = 0;
  SMESHDS_Mesh * meshDS = myHelper.GetMesh()->GetMeshDS();
  TNodeColumn* column;

  switch ( iEdge ) {
  case TOP_EDGE:
  case BOTTOM_EDGE:
    column = & (( ++myParamToColumnMap->begin())->second );
    node = ( iEdge == TOP_EDGE ) ? column->back() : column->front();
    edge = myHelper.GetSubShapeByNode ( node, meshDS );
    if ( edge.ShapeType() == TopAbs_VERTEX ) {
      column = & ( myParamToColumnMap->begin()->second );
      node = ( iEdge == TOP_EDGE ) ? column->back() : column->front();
    }
    break;
  case V0_EDGE:
  case V1_EDGE: {
    bool back = ( iEdge == V1_EDGE );
    if ( !myIsForward ) back = !back;
    if ( back )
      column = & ( myParamToColumnMap->rbegin()->second );
    else
      column = & ( myParamToColumnMap->begin()->second );
    if ( column->size() > 0 )
      edge = myHelper.GetSubShapeByNode( (*column)[ 1 ], meshDS );
    if ( edge.IsNull() || edge.ShapeType() == TopAbs_VERTEX )
      node = column->front();
    break;
  }
  default:;
  }
  if ( !edge.IsNull() && edge.ShapeType() == TopAbs_EDGE )
    return TopoDS::Edge( edge );

  // find edge by 2 vertices
  TopoDS_Shape V1 = edge;
  TopoDS_Shape V2 = myHelper.GetSubShapeByNode( node, meshDS );
  if ( !V2.IsNull() && V2.ShapeType() == TopAbs_VERTEX && !V2.IsSame( V1 ))
  {
    TopoDS_Shape ancestor = myHelper.GetCommonAncestor( V1, V2, *myHelper.GetMesh(), TopAbs_EDGE);
    if ( !ancestor.IsNull() )
      return TopoDS::Edge( ancestor );
  }
  return TopoDS_Edge();
}

//================================================================================
/*!
 * \brief Fill block sub-shapes
  * \param shapeMap - map to fill in
  * \retval int - nb inserted sub-shapes
 */
//================================================================================

int StdMeshers_PrismAsBlock::TSideFace::InsertSubShapes(TBlockShapes& shapeMap) const
{
  int nbInserted = 0;

  // Insert edges
  vector< int > edgeIdVec;
  SMESH_Block::GetFaceEdgesIDs( myID, edgeIdVec );

  for ( int i = BOTTOM_EDGE; i <=V1_EDGE ; ++i ) {
    TopoDS_Edge e = GetEdge( i );
    if ( !e.IsNull() ) {
      nbInserted += SMESH_Block::Insert( e, edgeIdVec[ i ], shapeMap);
    }
  }

  // Insert corner vertices

  TParam2ColumnIt col1, col2 ;
  vector< int > vertIdVec;

  // from V0 column
  SMESH_Block::GetEdgeVertexIDs( edgeIdVec[ V0_EDGE ], vertIdVec);
  GetColumns(0, col1, col2 );
  const SMDS_MeshNode* node0 = col1->second.front();
  const SMDS_MeshNode* node1 = col1->second.back();
  TopoDS_Shape v0 = myHelper.GetSubShapeByNode( node0, myHelper.GetMeshDS());
  TopoDS_Shape v1 = myHelper.GetSubShapeByNode( node1, myHelper.GetMeshDS());
  if ( v0.ShapeType() == TopAbs_VERTEX ) {
    nbInserted += SMESH_Block::Insert( v0, vertIdVec[ 0 ], shapeMap);
  }
  if ( v1.ShapeType() == TopAbs_VERTEX ) {
    nbInserted += SMESH_Block::Insert( v1, vertIdVec[ 1 ], shapeMap);
  }
  
  // from V1 column
  SMESH_Block::GetEdgeVertexIDs( edgeIdVec[ V1_EDGE ], vertIdVec);
  GetColumns(1, col1, col2 );
  node0 = col2->second.front();
  node1 = col2->second.back();
  v0 = myHelper.GetSubShapeByNode( node0, myHelper.GetMeshDS());
  v1 = myHelper.GetSubShapeByNode( node1, myHelper.GetMeshDS());
  if ( v0.ShapeType() == TopAbs_VERTEX ) {
    nbInserted += SMESH_Block::Insert( v0, vertIdVec[ 0 ], shapeMap);
  }
  if ( v1.ShapeType() == TopAbs_VERTEX ) {
    nbInserted += SMESH_Block::Insert( v1, vertIdVec[ 1 ], shapeMap);
  }

//   TopoDS_Vertex V0, V1, Vcom;
//   TopExp::Vertices( myBaseEdge, V0, V1, true );
//   if ( !myIsForward ) std::swap( V0, V1 );

//   // bottom vertex IDs
//   SMESH_Block::GetEdgeVertexIDs( edgeIdVec[ _u0 ], vertIdVec);
//   SMESH_Block::Insert( V0, vertIdVec[ 0 ], shapeMap);
//   SMESH_Block::Insert( V1, vertIdVec[ 1 ], shapeMap);

//   TopoDS_Edge sideEdge = GetEdge( V0_EDGE );
//   if ( sideEdge.IsNull() || !TopExp::CommonVertex( botEdge, sideEdge, Vcom ))
//     return false;

//   // insert one side edge
//   int edgeID;
//   if ( Vcom.IsSame( V0 )) edgeID = edgeIdVec[ _v0 ];
//   else                    edgeID = edgeIdVec[ _v1 ];
//   SMESH_Block::Insert( sideEdge, edgeID, shapeMap);

//   // top vertex of the side edge
//   SMESH_Block::GetEdgeVertexIDs( edgeID, vertIdVec);
//   TopoDS_Vertex Vtop = TopExp::FirstVertex( sideEdge );
//   if ( Vcom.IsSame( Vtop ))
//     Vtop = TopExp::LastVertex( sideEdge );
//   SMESH_Block::Insert( Vtop, vertIdVec[ 1 ], shapeMap);

//   // other side edge
//   sideEdge = GetEdge( V1_EDGE );
//   if ( sideEdge.IsNull() )
//     return false;
//   if ( edgeID = edgeIdVec[ _v1 ]) edgeID = edgeIdVec[ _v0 ];
//   else                            edgeID = edgeIdVec[ _v1 ];
//   SMESH_Block::Insert( sideEdge, edgeID, shapeMap);
  
//   // top edge
//   TopoDS_Edge topEdge = GetEdge( TOP_EDGE );
//   SMESH_Block::Insert( topEdge, edgeIdVec[ _u1 ], shapeMap);

//   // top vertex of the other side edge
//   if ( !TopExp::CommonVertex( topEdge, sideEdge, Vcom ))
//     return false;
//   SMESH_Block::GetEdgeVertexIDs( edgeID, vertIdVec );
//   SMESH_Block::Insert( Vcom, vertIdVec[ 1 ], shapeMap);

  return nbInserted;
}

//================================================================================
/*!
 * \brief Dump ids of nodes of sides
 */
//================================================================================

void StdMeshers_PrismAsBlock::TSideFace::dumpNodes(int nbNodes) const
{
#ifdef _DEBUG_
  cout << endl << "NODES OF FACE "; SMESH_Block::DumpShapeID( myID, cout ) << endl;
  THorizontalEdgeAdaptor* hSize0 = (THorizontalEdgeAdaptor*) HorizCurve(0);
  cout << "Horiz side 0: "; hSize0->dumpNodes(nbNodes); cout << endl;
  THorizontalEdgeAdaptor* hSize1 = (THorizontalEdgeAdaptor*) HorizCurve(1);
  cout << "Horiz side 1: "; hSize1->dumpNodes(nbNodes); cout << endl;
  TVerticalEdgeAdaptor* vSide0 = (TVerticalEdgeAdaptor*) VertiCurve(0);
  cout << "Verti side 0: "; vSide0->dumpNodes(nbNodes); cout << endl;
  TVerticalEdgeAdaptor* vSide1 = (TVerticalEdgeAdaptor*) VertiCurve(1);
  cout << "Verti side 1: "; vSide1->dumpNodes(nbNodes); cout << endl;
  delete hSize0; delete hSize1; delete vSide0; delete vSide1;
#endif
}

//================================================================================
/*!
 * \brief Creates TVerticalEdgeAdaptor 
  * \param columnsMap - node column map
  * \param parameter - normalized parameter
 */
//================================================================================

StdMeshers_PrismAsBlock::TVerticalEdgeAdaptor::
TVerticalEdgeAdaptor( const TParam2ColumnMap* columnsMap, const double parameter)
{
  myNodeColumn = & getColumn( columnsMap, parameter )->second;
}

//================================================================================
/*!
 * \brief Return coordinates for the given normalized parameter
  * \param U - normalized parameter
  * \retval gp_Pnt - coordinates
 */
//================================================================================

gp_Pnt StdMeshers_PrismAsBlock::TVerticalEdgeAdaptor::Value(const Standard_Real U) const
{
  const SMDS_MeshNode* n1;
  const SMDS_MeshNode* n2;
  double r = getRAndNodes( myNodeColumn, U, n1, n2 );
  return gpXYZ(n1) * ( 1 - r ) + gpXYZ(n2) * r;
}

//================================================================================
/*!
 * \brief Dump ids of nodes
 */
//================================================================================

void StdMeshers_PrismAsBlock::TVerticalEdgeAdaptor::dumpNodes(int nbNodes) const
{
#ifdef _DEBUG_
  for ( int i = 0; i < nbNodes && i < myNodeColumn->size(); ++i )
    cout << (*myNodeColumn)[i]->GetID() << " ";
  if ( nbNodes < myNodeColumn->size() )
    cout << myNodeColumn->back()->GetID();
#endif
}

//================================================================================
/*!
 * \brief Return coordinates for the given normalized parameter
  * \param U - normalized parameter
  * \retval gp_Pnt - coordinates
 */
//================================================================================

gp_Pnt StdMeshers_PrismAsBlock::THorizontalEdgeAdaptor::Value(const Standard_Real U) const
{
  return mySide->TSideFace::Value( U, myV );
}

//================================================================================
/*!
 * \brief Dump ids of <nbNodes> first nodes and the last one
 */
//================================================================================

void StdMeshers_PrismAsBlock::THorizontalEdgeAdaptor::dumpNodes(int nbNodes) const
{
#ifdef _DEBUG_
  // Not bedugged code. Last node is sometimes incorrect
  const TSideFace* side = mySide;
  double u = 0;
  if ( mySide->IsComplex() )
    side = mySide->GetComponent(0,u);

  TParam2ColumnIt col, col2;
  TParam2ColumnMap* u2cols = side->GetColumns();
  side->GetColumns( u , col, col2 );
  
  int j, i = myV ? mySide->ColumnHeight()-1 : 0;

  const SMDS_MeshNode* n = 0;
  const SMDS_MeshNode* lastN
    = side->IsForward() ? u2cols->rbegin()->second[ i ] : u2cols->begin()->second[ i ];
  for ( j = 0; j < nbNodes && n != lastN; ++j )
  {
    n = col->second[ i ];
    cout << n->GetID() << " ";
    if ( side->IsForward() )
      ++col;
    else
      --col;
  }

  // last node
  u = 1;
  if ( mySide->IsComplex() )
    side = mySide->GetComponent(1,u);

  side->GetColumns( u , col, col2 );
  if ( n != col->second[ i ] )
    cout << col->second[ i ]->GetID();
#endif
}

//================================================================================
/*!
 * \brief Costructor of TPCurveOnHorFaceAdaptor fills its map of
 * normalized parameter to node UV on a horizontal face
 *  \param [in] sideFace - lateral prism side
 *  \param [in] isTop - is \a horFace top or bottom of the prism
 *  \param [in] horFace - top or bottom face of the prism
 */
//================================================================================

StdMeshers_PrismAsBlock::
TPCurveOnHorFaceAdaptor::TPCurveOnHorFaceAdaptor( const TSideFace*   sideFace,
                                                  const bool         isTop,
                                                  const TopoDS_Face& horFace)
{
  if ( sideFace && !horFace.IsNull() )
  {
    //cout << "\n\t FACE " << sideFace->FaceID() << endl;
    const int Z = isTop ? sideFace->ColumnHeight() - 1 : 0;
    map<double, const SMDS_MeshNode* > u2nodes;
    sideFace->GetNodesAtZ( Z, u2nodes );
    if ( u2nodes.empty() )
      return;

    SMESH_MesherHelper helper( *sideFace->GetMesh() );
    helper.SetSubShape( horFace );

    bool okUV;
    gp_XY uv;
    double f,l;
    Handle(Geom2d_Curve) C2d;
    int edgeID = -1;
    const double tol = 10 * helper.MaxTolerance( horFace );
    const SMDS_MeshNode* prevNode = u2nodes.rbegin()->second;

    map<double, const SMDS_MeshNode* >::iterator u2n = u2nodes.begin();
    for ( ; u2n != u2nodes.end(); ++u2n )
    {
      const SMDS_MeshNode* n = u2n->second;
      okUV = false;
      if ( n->GetPosition()->GetTypeOfPosition() == SMDS_TOP_EDGE )
      {
        if ( n->getshapeId() != edgeID )
        {
          C2d.Nullify();
          edgeID = n->getshapeId();
          TopoDS_Shape S = helper.GetSubShapeByNode( n, helper.GetMeshDS() );
          if ( !S.IsNull() && S.ShapeType() == TopAbs_EDGE )
          {
            C2d = BRep_Tool::CurveOnSurface( TopoDS::Edge( S ), horFace, f,l );
          }
        }
        if ( !C2d.IsNull() )
        {
          double u = static_cast< const SMDS_EdgePosition* >( n->GetPosition() )->GetUParameter();
          if ( f <= u && u <= l )
          {
            uv = C2d->Value( u ).XY();
            okUV = helper.CheckNodeUV( horFace, n, uv, tol );
          }
        }
      }
      if ( !okUV )
        uv = helper.GetNodeUV( horFace, n, prevNode, &okUV );

      myUVmap.insert( myUVmap.end(), make_pair( u2n->first, uv ));
      // cout << n->getshapeId() << " N " << n->GetID()
      //      << " \t" << uv.X() << ", " << uv.Y() << " \t" << u2n->first << endl;

      prevNode = n;
    }
  }
}

//================================================================================
/*!
 * \brief Return UV on pcurve for the given normalized parameter
  * \param U - normalized parameter
  * \retval gp_Pnt - coordinates
 */
//================================================================================

gp_Pnt2d StdMeshers_PrismAsBlock::TPCurveOnHorFaceAdaptor::Value(const Standard_Real U) const
{
  map< double, gp_XY >::const_iterator i1 = myUVmap.upper_bound( U );

  if ( i1 == myUVmap.end() )
    return myUVmap.empty() ? gp_XY(0,0) : myUVmap.rbegin()->second;

  if ( i1 == myUVmap.begin() )
    return (*i1).second;

  map< double, gp_XY >::const_iterator i2 = i1--;

  double r = ( U - i1->first ) / ( i2->first - i1->first );
  return i1->second * ( 1 - r ) + i2->second * r;
}

//================================================================================
/*!
 * \brief Projects internal nodes using transformation found by boundary nodes
 */
//================================================================================

bool StdMeshers_Sweeper::projectIntPoints(const vector< gp_XYZ >&    fromBndPoints,
                                          const vector< gp_XYZ >&    toBndPoints,
                                          const vector< gp_XYZ >&    fromIntPoints,
                                          vector< gp_XYZ >&          toIntPoints,
                                          NSProjUtils::TrsfFinder3D& trsf,
                                          vector< gp_XYZ > *         bndError)
{
  // find transformation
  if ( trsf.IsIdentity() && !trsf.Solve( fromBndPoints, toBndPoints ))
    return false;

  // compute internal points using the found trsf
  for ( size_t iP = 0; iP < fromIntPoints.size(); ++iP )
  {
    toIntPoints[ iP ] = trsf.Transform( fromIntPoints[ iP ]);
  }

  // compute boundary error
  if ( bndError )
  {
    bndError->resize( fromBndPoints.size() );
    gp_XYZ fromTrsf;
    for ( size_t iP = 0; iP < fromBndPoints.size(); ++iP )
    {
      fromTrsf = trsf.Transform( fromBndPoints[ iP ] );
      (*bndError)[ iP ]  = toBndPoints[ iP ] - fromTrsf;
    }
  }
  return true;
}

//================================================================================
/*!
 * \brief Add boundary error to ineternal points
 */
//================================================================================

void StdMeshers_Sweeper::applyBoundaryError(const vector< gp_XYZ >& bndPoints,
                                            const vector< gp_XYZ >& bndError1,
                                            const vector< gp_XYZ >& bndError2,
                                            const double            r,
                                            vector< gp_XYZ >&       intPoints,
                                            vector< double >&       int2BndDist)
{
  // fix each internal point
  const double eps = 1e-100;
  for ( size_t iP = 0; iP < intPoints.size(); ++iP )
  {
    gp_XYZ & intPnt = intPoints[ iP ];

    // compute distance from intPnt to each boundary node
    double int2BndDistSum = 0;
    for ( size_t iBnd = 0; iBnd < bndPoints.size(); ++iBnd )
    {
      int2BndDist[ iBnd ] = 1 / (( intPnt - bndPoints[ iBnd ]).SquareModulus() + eps );
      int2BndDistSum += int2BndDist[ iBnd ];
    }

    // apply bndError
    for ( size_t iBnd = 0; iBnd < bndPoints.size(); ++iBnd )
    {
      intPnt += bndError1[ iBnd ] * ( 1 - r ) * int2BndDist[ iBnd ] / int2BndDistSum;
      intPnt += bndError2[ iBnd ] * r         * int2BndDist[ iBnd ] / int2BndDistSum;
    }
  }
}

//================================================================================
/*!
 * \brief Creates internal nodes of the prism
 */
//================================================================================

bool StdMeshers_Sweeper::ComputeNodes( SMESH_MesherHelper& helper,
                                       const double        tol,
                                       const bool          allowHighBndError)
{
  const size_t zSize = myBndColumns[0]->size();
  const size_t zSrc = 0, zTgt = zSize-1;
  if ( zSize < 3 ) return true;

  vector< vector< gp_XYZ > > intPntsOfLayer( zSize ); // node coodinates to compute
  // set coordinates of src and tgt nodes
  for ( size_t z = 0; z < intPntsOfLayer.size(); ++z )
    intPntsOfLayer[ z ].resize( myIntColumns.size() );
  for ( size_t iP = 0; iP < myIntColumns.size(); ++iP )
  {
    intPntsOfLayer[ zSrc ][ iP ] = intPoint( iP, zSrc );
    intPntsOfLayer[ zTgt ][ iP ] = intPoint( iP, zTgt );
  }

  // compute coordinates of internal nodes by projecting (transfroming) src and tgt
  // nodes towards the central layer

  vector< NSProjUtils::TrsfFinder3D > trsfOfLayer( zSize );
  vector< vector< gp_XYZ > >          bndError( zSize );

  // boundary points used to compute an affine transformation from a layer to a next one
  vector< gp_XYZ > fromSrcBndPnts( myBndColumns.size() ), fromTgtBndPnts( myBndColumns.size() );
  vector< gp_XYZ > toSrcBndPnts  ( myBndColumns.size() ), toTgtBndPnts  ( myBndColumns.size() );
  for ( size_t iP = 0; iP < myBndColumns.size(); ++iP )
  {
    fromSrcBndPnts[ iP ] = bndPoint( iP, zSrc );
    fromTgtBndPnts[ iP ] = bndPoint( iP, zTgt );
  }

  size_t zS = zSrc + 1;
  size_t zT = zTgt - 1;
  for ( ; zS < zT; ++zS, --zT ) // vertical loop on layers
  {
    for ( size_t iP = 0; iP < myBndColumns.size(); ++iP )
    {
      toSrcBndPnts[ iP ] = bndPoint( iP, zS );
      toTgtBndPnts[ iP ] = bndPoint( iP, zT );
    }
    if (! projectIntPoints( fromSrcBndPnts, toSrcBndPnts,
                            intPntsOfLayer[ zS-1 ], intPntsOfLayer[ zS ],
                            trsfOfLayer   [ zS-1 ], & bndError[ zS-1 ]))
      return false;
    if (! projectIntPoints( fromTgtBndPnts, toTgtBndPnts,
                            intPntsOfLayer[ zT+1 ], intPntsOfLayer[ zT ],
                            trsfOfLayer   [ zT+1 ], & bndError[ zT+1 ]))
      return false;

    // if ( zT == zTgt - 1 )
    // {
    //   for ( size_t iP = 0; iP < myBndColumns.size(); ++iP )
    //   {
    //     gp_XYZ fromTrsf = trsfOfLayer   [ zT+1].Transform( fromTgtBndPnts[ iP ] );
    //     cout << "mesh.AddNode( "
    //          << fromTrsf.X() << ", "
    //          << fromTrsf.Y() << ", "
    //          << fromTrsf.Z() << ") " << endl;
    //   }
    //   for ( size_t iP = 0; iP < myIntColumns.size(); ++iP )
    //     cout << "mesh.AddNode( "
    //          << intPntsOfLayer[ zT ][ iP ].X() << ", "
    //          << intPntsOfLayer[ zT ][ iP ].Y() << ", "
    //          << intPntsOfLayer[ zT ][ iP ].Z() << ") " << endl;
    // }

    fromTgtBndPnts.swap( toTgtBndPnts );
    fromSrcBndPnts.swap( toSrcBndPnts );
  }

  // Compute two projections of internal points to the central layer
  // in order to evaluate an error of internal points

  bool centerIntErrorIsSmall;
  vector< gp_XYZ > centerSrcIntPnts( myIntColumns.size() );
  vector< gp_XYZ > centerTgtIntPnts( myIntColumns.size() );

  for ( size_t iP = 0; iP < myBndColumns.size(); ++iP )
  {
    toSrcBndPnts[ iP ] = bndPoint( iP, zS );
    toTgtBndPnts[ iP ] = bndPoint( iP, zT );
  }
  if (! projectIntPoints( fromSrcBndPnts, toSrcBndPnts,
                          intPntsOfLayer[ zS-1 ], centerSrcIntPnts,
                          trsfOfLayer   [ zS-1 ], & bndError[ zS-1 ]))
    return false;
  if (! projectIntPoints( fromTgtBndPnts, toTgtBndPnts,
                          intPntsOfLayer[ zT+1 ], centerTgtIntPnts,
                          trsfOfLayer   [ zT+1 ], & bndError[ zT+1 ]))
    return false;

  // evaluate an error of internal points on the central layer
  centerIntErrorIsSmall = true;
  if ( zS == zT ) // odd zSize
  {
    for ( size_t iP = 0; ( iP < myIntColumns.size() && centerIntErrorIsSmall ); ++iP )
      centerIntErrorIsSmall =
        (centerSrcIntPnts[ iP ] - centerTgtIntPnts[ iP ]).SquareModulus() < tol*tol;
  }
  else // even zSize
  {
    for ( size_t iP = 0; ( iP < myIntColumns.size() && centerIntErrorIsSmall ); ++iP )
      centerIntErrorIsSmall =
        (intPntsOfLayer[ zS-1 ][ iP ] - centerTgtIntPnts[ iP ]).SquareModulus() < tol*tol;
  }

  // Evaluate an error of boundary points

  bool bndErrorIsSmall = true;
  for ( size_t iP = 0; ( iP < myBndColumns.size() && bndErrorIsSmall ); ++iP )
  {
    double sumError = 0;
    for ( size_t z = 1; z < zS; ++z ) // loop on layers
      sumError += ( bndError[ z-1     ][ iP ].Modulus() +
                    bndError[ zSize-z ][ iP ].Modulus() );

    bndErrorIsSmall = ( sumError < tol );
  }

  if ( !bndErrorIsSmall && !allowHighBndError )
    return false;

  // compute final points on the central layer
  std::vector< double > int2BndDist( myBndColumns.size() ); // work array of applyBoundaryError()
  double r = zS / ( zSize - 1.);
  if ( zS == zT )
  {
    for ( size_t iP = 0; iP < myIntColumns.size(); ++iP )
    {
      intPntsOfLayer[ zS ][ iP ] =
        ( 1 - r ) * centerSrcIntPnts[ iP ] + r * centerTgtIntPnts[ iP ];
    }
    if ( !bndErrorIsSmall )
    {
      applyBoundaryError( toSrcBndPnts, bndError[ zS-1 ], bndError[ zS+1 ], r,
                          intPntsOfLayer[ zS ], int2BndDist );
    }
  }
  else
  {
    for ( size_t iP = 0; iP < myIntColumns.size(); ++iP )
    {
      intPntsOfLayer[ zS ][ iP ] =
        r * intPntsOfLayer[ zS ][ iP ] + ( 1 - r ) * centerSrcIntPnts[ iP ];
      intPntsOfLayer[ zT ][ iP ] =
        r * intPntsOfLayer[ zT ][ iP ] + ( 1 - r ) * centerTgtIntPnts[ iP ];
    }
    if ( !bndErrorIsSmall )
    {
      applyBoundaryError( toSrcBndPnts, bndError[ zS-1 ], bndError[ zS+1 ], r,
                          intPntsOfLayer[ zS ], int2BndDist );
      applyBoundaryError( toTgtBndPnts, bndError[ zT+1 ], bndError[ zT-1 ], r,
                          intPntsOfLayer[ zT ], int2BndDist );
    }
  }

  //centerIntErrorIsSmall = true;
  //bndErrorIsSmall = true;
  if ( !centerIntErrorIsSmall )
  {
    // Compensate the central error; continue adding projection
    // by going from central layer to the source and target ones

    vector< gp_XYZ >& fromSrcIntPnts = centerSrcIntPnts;
    vector< gp_XYZ >& fromTgtIntPnts = centerTgtIntPnts;
    vector< gp_XYZ >  toSrcIntPnts( myIntColumns.size() );
    vector< gp_XYZ >  toTgtIntPnts( myIntColumns.size() );
    vector< gp_XYZ >  srcBndError( myBndColumns.size() );
    vector< gp_XYZ >  tgtBndError( myBndColumns.size() );

    fromTgtBndPnts.swap( toTgtBndPnts );
    fromSrcBndPnts.swap( toSrcBndPnts );

    for ( ++zS, --zT; zS < zTgt; ++zS, --zT ) // vertical loop on layers
    {
      // invert transformation
      if ( !trsfOfLayer[ zS+1 ].Invert() )
        trsfOfLayer[ zS+1 ] = NSProjUtils::TrsfFinder3D(); // to recompute
      if ( !trsfOfLayer[ zT-1 ].Invert() )
        trsfOfLayer[ zT-1 ] = NSProjUtils::TrsfFinder3D();

      // project internal nodes and compute bnd error
      for ( size_t iP = 0; iP < myBndColumns.size(); ++iP )
      {
        toSrcBndPnts[ iP ] = bndPoint( iP, zS );
        toTgtBndPnts[ iP ] = bndPoint( iP, zT );
      }
      projectIntPoints( fromSrcBndPnts, toSrcBndPnts,
                        fromSrcIntPnts, toSrcIntPnts,
                        trsfOfLayer[ zS+1 ], & srcBndError );
      projectIntPoints( fromTgtBndPnts, toTgtBndPnts,
                        fromTgtIntPnts, toTgtIntPnts,
                        trsfOfLayer[ zT-1 ], & tgtBndError );

      // if ( zS == zTgt - 1 )
      // {
      //   cout << "mesh2 = smesh.Mesh()" << endl;
      //   for ( size_t iP = 0; iP < myBndColumns.size(); ++iP )
      //   {
      //     gp_XYZ fromTrsf = trsfOfLayer   [ zS+1].Transform( fromSrcBndPnts[ iP ] );
      //     cout << "mesh2.AddNode( "
      //          << fromTrsf.X() << ", "
      //          << fromTrsf.Y() << ", "
      //          << fromTrsf.Z() << ") " << endl;
      //   }
      //   for ( size_t iP = 0; iP < myIntColumns.size(); ++iP )
      //     cout << "mesh2.AddNode( "
      //          << toSrcIntPnts[ iP ].X() << ", "
      //          << toSrcIntPnts[ iP ].Y() << ", "
      //          << toSrcIntPnts[ iP ].Z() << ") " << endl;
      // }

      // sum up 2 projections
      r = zS / ( zSize - 1.);
      vector< gp_XYZ >& zSIntPnts = intPntsOfLayer[ zS ];
      vector< gp_XYZ >& zTIntPnts = intPntsOfLayer[ zT ];
      for ( size_t iP = 0; iP < myIntColumns.size(); ++iP )
      {
        zSIntPnts[ iP ] = r * zSIntPnts[ iP ]  +  ( 1 - r ) * toSrcIntPnts[ iP ];
        zTIntPnts[ iP ] = r * zTIntPnts[ iP ]  +  ( 1 - r ) * toTgtIntPnts[ iP ];
      }

      // compensate bnd error
      if ( !bndErrorIsSmall )
      {
        applyBoundaryError( toSrcBndPnts, srcBndError, bndError[ zS+1 ], r,
                            intPntsOfLayer[ zS ], int2BndDist );
        applyBoundaryError( toTgtBndPnts, tgtBndError, bndError[ zT-1 ], r,
                            intPntsOfLayer[ zT ], int2BndDist );
      }

      fromSrcBndPnts.swap( toSrcBndPnts );
      fromSrcIntPnts.swap( toSrcIntPnts );
      fromTgtBndPnts.swap( toTgtBndPnts );
      fromTgtIntPnts.swap( toTgtIntPnts );
    }
  }  // if ( !centerIntErrorIsSmall )

  else if ( !bndErrorIsSmall )
  {
    zS = zSrc + 1;
    zT = zTgt - 1;
    for ( ; zS < zT; ++zS, --zT ) // vertical loop on layers
    {
      for ( size_t iP = 0; iP < myBndColumns.size(); ++iP )
      {
        toSrcBndPnts[ iP ] = bndPoint( iP, zS );
        toTgtBndPnts[ iP ] = bndPoint( iP, zT );
      }
      // compensate bnd error
      applyBoundaryError( toSrcBndPnts, bndError[ zS-1 ], bndError[ zS-1 ], 0.5,
                          intPntsOfLayer[ zS ], int2BndDist );
      applyBoundaryError( toTgtBndPnts, bndError[ zT+1 ], bndError[ zT+1 ], 0.5,
                          intPntsOfLayer[ zT ], int2BndDist );
    }
  }

  // cout << "centerIntErrorIsSmall = " << centerIntErrorIsSmall<< endl;
  // cout << "bndErrorIsSmall = " << bndErrorIsSmall<< endl;

  // Create nodes
  for ( size_t iP = 0; iP < myIntColumns.size(); ++iP )
  {
    vector< const SMDS_MeshNode* > & nodeCol = *myIntColumns[ iP ];
    for ( size_t z = zSrc + 1; z < zTgt; ++z ) // vertical loop on layers
    {
      const gp_XYZ & xyz = intPntsOfLayer[ z ][ iP ];
      if ( !( nodeCol[ z ] = helper.AddNode( xyz.X(), xyz.Y(), xyz.Z() )))
        return false;
    }
  }

  return true;
}

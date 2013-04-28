//  SMESH SMESH : implementaion of SMESH idl descriptions
//
//  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
// File      : StdMeshers_CompositeHexa_3D.cxx
// Module    : SMESH
// Created   : Tue Nov 25 11:04:59 2008
// Author    : Edward AGAPOV (eap)
#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif // _MSC_VER
#include <cmath>

#include "StdMeshers_CompositeHexa_3D.hxx"

#include "SMDS_Mesh.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_SetIterator.hxx"
#include "SMESH_Block.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_ComputeError.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"

#include <BRepAdaptor_Surface.hxx>
#include <BRep_Tool.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_MapIteratorOfMapOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec.hxx>
#include <gp_XYZ.hxx>

#include <list>
#include <set>
#include <vector>

#ifndef PI
#define PI M_PI
#endif


#ifdef _DEBUG_

// #define DEB_FACES
// #define DEB_GRID
#define DUMP_VERT(msg,V) \
// { TopoDS_Vertex v = V; gp_Pnt p = BRep_Tool::Pnt(v);\
//   cout << msg << "( "<< p.X()<<", "<<p.Y()<<", "<<p.Z()<<" )"<<endl;}

#else

#define DUMP_VERT(msg,v)

#endif

//================================================================================
// text for message about an internal error
#define ERR_LI(txt) SMESH_Comment(txt) << ":" << __LINE__

// order corresponds to right order of edges in CASCADE face
enum EQuadSides{ Q_BOTTOM=0, Q_RIGHT, Q_TOP, Q_LEFT,   Q_CHILD, Q_PARENT };

enum EBoxSides{ B_BOTTOM=0, B_RIGHT, B_TOP, B_LEFT, B_FRONT, B_BACK, B_UNDEFINED };

//================================================================================
/*!
 * \brief Convertor of a pair of integers to a sole index
 */
struct _Indexer
{
  int _xSize, _ySize;
  _Indexer( int xSize, int ySize ): _xSize(xSize), _ySize(ySize) {}
  int size() const { return _xSize * _ySize; }
  int operator()(const int x, const int y) const { return y * _xSize + x; }
};

//================================================================================
/*!
 * \brief Wrapper of a composite or an ordinary edge.
 */
class _FaceSide
{
public:
  _FaceSide(const _FaceSide& other);
  _FaceSide(const TopoDS_Edge& edge=TopoDS_Edge());
  _FaceSide(const list<TopoDS_Edge>& edges);
  _FaceSide* GetSide(const int i);
  const _FaceSide* GetSide(const int i) const;
  int size() { return myChildren.size(); }
  int NbVertices() const;
  TopoDS_Vertex FirstVertex() const;
  TopoDS_Vertex LastVertex() const;
  TopoDS_Vertex Vertex(int i) const;
  bool Contain( const _FaceSide& side, int* which=0 ) const;
  bool Contain( const TopoDS_Vertex& vertex ) const;
  void AppendSide( const _FaceSide& side );
  void SetBottomSide( int i );
  int GetNbSegments(SMESH_Mesh& mesh) const;
  bool StoreNodes(SMESH_Mesh& mesh, vector<const SMDS_MeshNode*>& myGrid, bool reverse );
  void SetID(EQuadSides id) { myID = id; }
  static inline const TopoDS_TShape* ptr(const TopoDS_Shape& theShape)
  { return theShape.TShape().operator->(); }
  void Dump() const;

private:


  TopoDS_Edge       myEdge;

  #ifdef __BORLANDC__
  vector< _FaceSide > myChildren;
  #else
  list< _FaceSide > myChildren;
  #endif

  int               myNbChildren;

  //set<const TopoDS_TShape*> myVertices;
  TopTools_MapOfShape myVertices;

  EQuadSides        myID; // debug
};
//================================================================================
/*!
 * \brief Class corresponding to a meshed composite face of a box.
 *        Provides simplified access to it's sub-mesh data.
 */
class _QuadFaceGrid
{
  #ifdef __BORLANDC__
  typedef vector< _QuadFaceGrid > TChildren;
  #else
  typedef list< _QuadFaceGrid > TChildren;
  #endif
public:
  _QuadFaceGrid();

public: //** Methods to find and orient faces of 6 sides of the box **//
  
  //!< initialization
  bool Init(const TopoDS_Face& f);

  //!< try to unite self with other face
  bool AddContinuousFace( const _QuadFaceGrid& f );

  //!< Try to set the side as bottom hirizontal side
  bool SetBottomSide(const _FaceSide& side, int* sideIndex=0);

  //!< Return face adjacent to i-th side of this face
  _QuadFaceGrid* FindAdjacentForSide(int i, vector<_QuadFaceGrid>& faces) const; // (0<i<4)

  //!< Reverse edges in order to have the bottom edge going along axes of the unit box
  void ReverseEdges(/*int e1, int e2*/);

  bool IsComplex() const { return !myChildren.empty(); }

  typedef SMDS_SetIterator< const _QuadFaceGrid&, TChildren::const_iterator > TChildIterator;

  TChildIterator GetChildren() const
  { return TChildIterator( myChildren.begin(), myChildren.end()); }

public: //** Loading and access to mesh **//

  //!< Load nodes of a mesh
  bool LoadGrid( SMESH_Mesh& mesh );

  //!< Return number of segments on the hirizontal sides
  int GetNbHoriSegments(SMESH_Mesh& mesh, bool withBrothers=false) const;

  //!< Return number of segments on the vertical sides
  int GetNbVertSegments(SMESH_Mesh& mesh, bool withBrothers=false) const;

  //!< Return a node by its position
  const SMDS_MeshNode* GetNode(int iHori, int iVert) const;

  //!< Return node coordinates by its position
  gp_XYZ GetXYZ(int iHori, int iVert) const;

public: //** Access to member fields **//

  //!< Return i-th face side (0<i<4)
  const _FaceSide& GetSide(int i) const;

  //!< Return it's face, NULL if it is composite
  TopoDS_Face GetFace() const { return myFace; }

  //!< Return normal to the face at vertex v
  bool GetNormal( const TopoDS_Vertex& v, gp_Vec& n ) const;

  SMESH_ComputeErrorPtr GetError() const { return myError; }

  void SetID(EBoxSides id) { myID = id; }

  void DumpGrid() const;

  void DumpVertices() const;

private:

  bool error(std::string& text, int code = COMPERR_ALGO_FAILED)
  { myError = SMESH_ComputeError::New( code, text ); return false; }

  bool error(const SMESH_ComputeErrorPtr& err)
  { myError = err; return ( !myError || myError->IsOK() ); }

  bool loadCompositeGrid(SMESH_Mesh& mesh);

  bool fillGrid(SMESH_Mesh&                    theMesh,
                vector<const SMDS_MeshNode*> & theGrid,
                const _Indexer&                theIndexer,
                int                            theX,
                int                            theY);

  bool locateChildren();

  void setBrothers( set< _QuadFaceGrid* >& notLocatedBrothers );

  TopoDS_Face myFace;
  _FaceSide   mySides;
  bool        myReverse;

  TChildren   myChildren;

  _QuadFaceGrid* myLeftBottomChild;
  _QuadFaceGrid* myRightBrother;
  _QuadFaceGrid* myUpBrother;

  _Indexer    myIndexer;
  vector<const SMDS_MeshNode*>  myGrid;

  SMESH_ComputeErrorPtr         myError;

  EBoxSides   myID; // debug
};

//================================================================================
/*!
 * \brief Constructor
 */
//================================================================================

StdMeshers_CompositeHexa_3D::StdMeshers_CompositeHexa_3D(int hypId, int studyId, SMESH_Gen* gen)
  :SMESH_3D_Algo(hypId, studyId, gen)
{
  _name = "CompositeHexa_3D";
  _shapeType = (1 << TopAbs_SHELL) | (1 << TopAbs_SOLID);	// 1 bit /shape type
}

//================================================================================
/*!
 * \brief always return true
 */
//================================================================================

bool StdMeshers_CompositeHexa_3D::CheckHypothesis(SMESH_Mesh&         aMesh,
                                                  const TopoDS_Shape& aShape,
                                                  Hypothesis_Status&  aStatus)
{
  aStatus = HYP_OK;
  return true;
}

//================================================================================
/*!
 * \brief Computes hexahedral mesh on a box with composite sides
 *  \param aMesh - mesh to compute
 *  \param aShape - shape to mesh
 *  \retval bool - succes sign
 */
//================================================================================

bool StdMeshers_CompositeHexa_3D::Compute(SMESH_Mesh&         theMesh,
                                          const TopoDS_Shape& theShape)
{
  SMESH_MesherHelper helper( theMesh );
  _quadraticMesh = helper.IsQuadraticSubMesh( theShape );
  helper.SetElementsOnShape( true );

  // -------------------------
  // Try to find 6 side faces
  // -------------------------
  vector< _QuadFaceGrid > boxFaces; boxFaces.reserve( 6 );
  TopExp_Explorer exp;
  int iFace, nbFaces = 0;
  for ( exp.Init(theShape, TopAbs_FACE); exp.More(); exp.Next(), ++nbFaces )
  {
    _QuadFaceGrid f;
    if ( !f.Init( TopoDS::Face( exp.Current() )))
      return error (COMPERR_BAD_SHAPE);
    bool isContinuous = false;
    for ( int i=0; i < boxFaces.size() && !isContinuous; ++i )
      isContinuous = boxFaces[ i ].AddContinuousFace( f );
    if ( !isContinuous )
      boxFaces.push_back( f );
  }
  // Check what we have
  if ( boxFaces.size() != 6 && nbFaces != 6)
    return error
      (COMPERR_BAD_SHAPE,
       SMESH_Comment("Can't find 6 sides of a box. Number of found sides - ")<<boxFaces.size());

  if ( boxFaces.size() != 6 && nbFaces == 6 ) { // strange ordinary box with continuous faces
    boxFaces.resize( 6 );
    iFace = 0;
    for ( exp.Init(theShape, TopAbs_FACE); exp.More(); exp.Next(), ++iFace )
      boxFaces[ iFace ].Init( TopoDS::Face( exp.Current() ) );
  }
  // ----------------------------------------
  // Find out position of faces within a box
  // ----------------------------------------

  _QuadFaceGrid *fBottom, *fTop, *fFront, *fBack, *fLeft, *fRight;
  // start from a bottom face
  fBottom = &boxFaces[0];
  // find vertical faces
  fFront = fBottom->FindAdjacentForSide( Q_BOTTOM, boxFaces );
  fLeft  = fBottom->FindAdjacentForSide( Q_RIGHT, boxFaces );
  fBack  = fBottom->FindAdjacentForSide( Q_TOP, boxFaces );
  fRight = fBottom->FindAdjacentForSide( Q_LEFT, boxFaces );
  // check the found
  if ( !fFront || !fBack || !fLeft || !fRight )
    return error(COMPERR_BAD_SHAPE);
  // top face
  fTop = 0;
  for ( int i=1; i < boxFaces.size() && !fTop; ++i ) {
    fTop = & boxFaces[ i ];
    if ( fTop==fFront || fTop==fLeft || fTop==fBack || fTop==fRight )
      fTop = 0;
  }
  // set bottom of the top side
  if ( !fTop->SetBottomSide( fFront->GetSide( Q_TOP ) )) {
    if ( !fFront->IsComplex() )
      return error( ERR_LI("Error in StdMeshers_CompositeHexa_3D::Compute()"));
    else {
      _QuadFaceGrid::TChildIterator chIt = fFront->GetChildren();
      while ( chIt.more() ) {
        const _QuadFaceGrid& frontChild = chIt.next();
        if ( fTop->SetBottomSide( frontChild.GetSide( Q_TOP )))
          break;
      }
    }
  }
  if ( !fTop )
    return error(COMPERR_BAD_SHAPE);

  fBottom->SetID( B_BOTTOM );
  fBack  ->SetID( B_BACK );
  fLeft  ->SetID( B_LEFT );
  fFront ->SetID( B_FRONT );
  fRight ->SetID( B_RIGHT );
  fTop   ->SetID( B_TOP );

  // orient bottom egde of faces along axes of the unit box
  fBottom->ReverseEdges();
  fBack  ->ReverseEdges();
  fLeft  ->ReverseEdges();

  // ------------------------------------------
  // Fill columns of nodes with existing nodes
  // ------------------------------------------

  // let faces load their grids
  if ( !fBottom->LoadGrid( theMesh )) return error( fBottom->GetError() );
  if ( !fBack  ->LoadGrid( theMesh )) return error( fBack  ->GetError() );
  if ( !fLeft  ->LoadGrid( theMesh )) return error( fLeft  ->GetError() );
  if ( !fFront ->LoadGrid( theMesh )) return error( fFront ->GetError() );
  if ( !fRight ->LoadGrid( theMesh )) return error( fRight ->GetError() );
  if ( !fTop   ->LoadGrid( theMesh )) return error( fTop   ->GetError() );

  int x, xSize = fBottom->GetNbHoriSegments(theMesh) + 1, X = xSize - 1;
  int y, ySize = fBottom->GetNbVertSegments(theMesh) + 1, Y = ySize - 1;
  int z, zSize = fFront ->GetNbVertSegments(theMesh) + 1, Z = zSize - 1;
  _Indexer colIndex( xSize, ySize );
  vector< vector< const SMDS_MeshNode* > > columns( colIndex.size() );

  // fill node columns by front and back box sides
  for ( x = 0; x < xSize; ++x ) {
    vector< const SMDS_MeshNode* >& column0 = columns[ colIndex( x, 0 )];
    vector< const SMDS_MeshNode* >& column1 = columns[ colIndex( x, Y )];
    column0.resize( zSize );
    column1.resize( zSize );
    for ( z = 0; z < zSize; ++z ) {
      column0[ z ] = fFront->GetNode( x, z );
      column1[ z ] = fBack ->GetNode( x, z );
    }
  }
  // fill node columns by left and right box sides
  for ( y = 1; y < ySize-1; ++y ) {
    vector< const SMDS_MeshNode* >& column0 = columns[ colIndex( 0, y )];
    vector< const SMDS_MeshNode* >& column1 = columns[ colIndex( X, y )];
    column0.resize( zSize );
    column1.resize( zSize );
    for ( z = 0; z < zSize; ++z ) {
      column0[ z ] = fLeft ->GetNode( y, z );
      column1[ z ] = fRight->GetNode( y, z );
    }
  }
  // get nodes from top and bottom box sides
  for ( x = 1; x < xSize-1; ++x ) {
    for ( y = 1; y < ySize-1; ++y ) {
      vector< const SMDS_MeshNode* >& column = columns[ colIndex( x, y )];
      column.resize( zSize );
      column.front() = fBottom->GetNode( x, y );
      column.back()  = fTop   ->GetNode( x, y );
    }
  }

  // ----------------------------
  // Add internal nodes of a box
  // ----------------------------
  // projection points of internal nodes on box subshapes by which
  // coordinates of internal nodes are computed
  vector<gp_XYZ> pointsOnShapes( SMESH_Block::ID_Shell );

  // projections on vertices are constant
  pointsOnShapes[ SMESH_Block::ID_V000 ] = fBottom->GetXYZ( 0, 0 );
  pointsOnShapes[ SMESH_Block::ID_V100 ] = fBottom->GetXYZ( X, 0 );
  pointsOnShapes[ SMESH_Block::ID_V010 ] = fBottom->GetXYZ( 0, Y );
  pointsOnShapes[ SMESH_Block::ID_V110 ] = fBottom->GetXYZ( X, Y );
  pointsOnShapes[ SMESH_Block::ID_V001 ] = fTop->GetXYZ( 0, 0 );
  pointsOnShapes[ SMESH_Block::ID_V101 ] = fTop->GetXYZ( X, 0 );
  pointsOnShapes[ SMESH_Block::ID_V011 ] = fTop->GetXYZ( 0, Y );
  pointsOnShapes[ SMESH_Block::ID_V111 ] = fTop->GetXYZ( X, Y );

  for ( x = 1; x < xSize-1; ++x )
  {
    gp_XYZ params; // normalized parameters of internal node within a unit box
    params.SetCoord( 1, x / double(X) );
    for ( y = 1; y < ySize-1; ++y )
    {
      params.SetCoord( 2, y / double(Y) );
      // column to fill during z loop
      vector< const SMDS_MeshNode* >& column = columns[ colIndex( x, y )];
      // points projections on horizontal edges
      pointsOnShapes[ SMESH_Block::ID_Ex00 ] = fBottom->GetXYZ( x, 0 );
      pointsOnShapes[ SMESH_Block::ID_Ex10 ] = fBottom->GetXYZ( x, Y );
      pointsOnShapes[ SMESH_Block::ID_E0y0 ] = fBottom->GetXYZ( 0, y );
      pointsOnShapes[ SMESH_Block::ID_E1y0 ] = fBottom->GetXYZ( X, y );
      pointsOnShapes[ SMESH_Block::ID_Ex01 ] = fTop->GetXYZ( x, 0 );
      pointsOnShapes[ SMESH_Block::ID_Ex11 ] = fTop->GetXYZ( x, Y );
      pointsOnShapes[ SMESH_Block::ID_E0y1 ] = fTop->GetXYZ( 0, y );
      pointsOnShapes[ SMESH_Block::ID_E1y1 ] = fTop->GetXYZ( X, y );
      // points projections on horizontal faces
      pointsOnShapes[ SMESH_Block::ID_Fxy0 ] = fBottom->GetXYZ( x, y );
      pointsOnShapes[ SMESH_Block::ID_Fxy1 ] = fTop   ->GetXYZ( x, y );
      for ( z = 1; z < zSize-1; ++z ) // z loop
      {
        params.SetCoord( 3, z / double(Z) );
        // point projections on vertical edges
        pointsOnShapes[ SMESH_Block::ID_E00z ] = fFront->GetXYZ( 0, z );    
        pointsOnShapes[ SMESH_Block::ID_E10z ] = fFront->GetXYZ( X, z );    
        pointsOnShapes[ SMESH_Block::ID_E01z ] = fBack->GetXYZ( 0, z );    
        pointsOnShapes[ SMESH_Block::ID_E11z ] = fBack->GetXYZ( X, z );
        // point projections on vertical faces
        pointsOnShapes[ SMESH_Block::ID_Fx0z ] = fFront->GetXYZ( x, z );    
        pointsOnShapes[ SMESH_Block::ID_Fx1z ] = fBack ->GetXYZ( x, z );    
        pointsOnShapes[ SMESH_Block::ID_F0yz ] = fLeft ->GetXYZ( y, z );    
        pointsOnShapes[ SMESH_Block::ID_F1yz ] = fRight->GetXYZ( y, z );

        // compute internal node coordinates
        gp_XYZ coords;
        SMESH_Block::ShellPoint( params, pointsOnShapes, coords );
        column[ z ] = helper.AddNode( coords.X(), coords.Y(), coords.Z() );

#ifdef DEB_GRID
        // debug
        //cout << "----------------------------------------------------------------------"<<endl;
        //for ( int id = SMESH_Block::ID_V000; id < SMESH_Block::ID_Shell; ++id)
        //{
        //  gp_XYZ p = pointsOnShapes[ id ];
        //  SMESH_Block::DumpShapeID( id,cout)<<" ( "<<p.X()<<", "<<p.Y()<<", "<<p.Z()<<" )"<<endl;
        //}
        //cout << "Params: ( "<< params.X()<<", "<<params.Y()<<", "<<params.Z()<<" )"<<endl;
        //cout << "coords: ( "<< coords.X()<<", "<<coords.Y()<<", "<<coords.Z()<<" )"<<endl;
#endif
      }
    }
  }
  // faces no more needed, free memory
  boxFaces.clear();

  // ----------------
  // Add hexahedrons
  // ----------------
  for ( x = 0; x < xSize-1; ++x ) {
    for ( y = 0; y < ySize-1; ++y ) {
      vector< const SMDS_MeshNode* >& col00 = columns[ colIndex( x, y )];
      vector< const SMDS_MeshNode* >& col10 = columns[ colIndex( x+1, y )];
      vector< const SMDS_MeshNode* >& col01 = columns[ colIndex( x, y+1 )];
      vector< const SMDS_MeshNode* >& col11 = columns[ colIndex( x+1, y+1 )];
      for ( z = 0; z < zSize-1; ++z )
      {
        // bottom face normal of a hexa mush point outside the volume
        helper.AddVolume(col00[z],   col01[z],   col11[z],   col10[z],
                         col00[z+1], col01[z+1], col11[z+1], col10[z+1]);
      }
    }
  }
  return true;
}

//================================================================================
/*!
 * \brief constructor of non-initialized _QuadFaceGrid
 */
//================================================================================

_QuadFaceGrid::_QuadFaceGrid():
  myReverse(false), myRightBrother(0), myUpBrother(0), myIndexer(0,0), myID(B_UNDEFINED)
{
}

//================================================================================
/*!
 * \brief Initialization
 */
//================================================================================

bool _QuadFaceGrid::Init(const TopoDS_Face& f)
{
  myFace         = f;
  mySides        = _FaceSide();
  myReverse      = false;
  myLeftBottomChild = myRightBrother = myUpBrother = 0;
  myChildren.clear();
  myGrid.clear();
  //if ( myFace.Orientation() != TopAbs_FORWARD )
    //myFace.Reverse();

  TopoDS_Vertex V;
  list< TopoDS_Edge > edges;
  list< int > nbEdgesInWire;
  int nbWire = SMESH_Block::GetOrderedEdges (myFace, V, edges, nbEdgesInWire);
  if ( nbWire != 1 )
    return false;

  list< TopoDS_Edge >::iterator edgeIt = edges.begin();
  if ( nbEdgesInWire.front() == 4 ) // exactly 4 edges
  {
    for ( ; edgeIt != edges.end(); ++edgeIt )
      mySides.AppendSide( _FaceSide( *edgeIt ));
  }
  else if ( nbEdgesInWire.front() > 4 ) { // more than 4 edges - try to unite some
    list< TopoDS_Edge > sideEdges;
    while ( !edges.empty()) {
      sideEdges.clear();
      sideEdges.splice( sideEdges.end(), edges, edges.begin());// edges.front()->sideEdges.back()
      while ( !edges.empty() ) {
        if ( SMESH_Algo::IsContinuous( sideEdges.back(), edges.front() )) {
          sideEdges.splice( sideEdges.end(), edges, edges.begin());
        }
        else if ( SMESH_Algo::IsContinuous( sideEdges.front(), edges.back() )) {
          sideEdges.splice( sideEdges.begin(), edges, --edges.end());
        }
        else {
          break;
        }
      }
      mySides.AppendSide( _FaceSide( sideEdges ));
    }
  }
  if (mySides.size() != 4)
    return false;

#ifdef _DEBUG_
  mySides.GetSide( Q_BOTTOM )->SetID( Q_BOTTOM );
  mySides.GetSide( Q_RIGHT  )->SetID( Q_RIGHT );
  mySides.GetSide( Q_TOP    )->SetID( Q_TOP );
  mySides.GetSide( Q_LEFT   )->SetID( Q_LEFT );
#endif

  return true;
}

//================================================================================
/*!
 * \brief Try to unite self with other ordinary face
 */
//================================================================================

bool _QuadFaceGrid::AddContinuousFace( const _QuadFaceGrid& other )
{
  for ( int i = 0; i < 4; ++i ) {
    const _FaceSide& otherSide = other.GetSide( i );
    int iMyCommon;
    if ( mySides.Contain( otherSide, &iMyCommon ) ) {
      // check if normals of two faces are collinear at all vertices of a otherSide
      const double angleTol = PI / 180 / 2;
      int iV, nbV = otherSide.NbVertices(), nbCollinear = 0;
      for ( iV = 0; iV < nbV; ++iV )
      {
        TopoDS_Vertex v = otherSide.Vertex( iV );
        gp_Vec n1, n2;
        if ( !GetNormal( v, n1 ) || !other.GetNormal( v, n2 ))
          continue;
        if ( n1 * n2 < 0 )
          n1.Reverse();
        if ( n1.Angle(n2) < angleTol )
          nbCollinear++;
        else
          break;
      }
      if ( nbCollinear > 1 ) { // this face becomes composite if not yet is
        DUMP_VERT("Cont 1", mySides.GetSide(iMyCommon)->FirstVertex());
        DUMP_VERT("Cont 2", mySides.GetSide(iMyCommon)->LastVertex());
        DUMP_VERT("Cont 3", otherSide.FirstVertex());
        DUMP_VERT("Cont 4", otherSide.LastVertex());
        if ( myChildren.empty() ) {
          myChildren.push_back( *this );
          myFace.Nullify();
        }
        myChildren.push_back( other );
        int otherBottomIndex = ( 4 + i - iMyCommon + 2 ) % 4;
        myChildren.back().SetBottomSide( other.GetSide( otherBottomIndex ));
        // collect vertices in mySides
        mySides.AppendSide( other.GetSide(0) );
        mySides.AppendSide( other.GetSide(1) );
        mySides.AppendSide( other.GetSide(2) );
        mySides.AppendSide( other.GetSide(3) );
        return true;
      }
    }
  }
  return false;
}

//================================================================================
/*!
 * \brief Try to set the side as bottom hirizontal side
 */
//================================================================================

bool _QuadFaceGrid::SetBottomSide(const _FaceSide& bottom, int* sideIndex)
{
  myLeftBottomChild = myRightBrother = myUpBrother = 0;

  int myBottomIndex;
  if ( myChildren.empty() )
  {
    if ( mySides.Contain( bottom, &myBottomIndex )) {
      mySides.SetBottomSide( myBottomIndex );
      if ( sideIndex )
        *sideIndex = myBottomIndex;
      return true;
    }
  }
  else
  {
    TChildren::iterator childFace = myChildren.begin(), childEnd = myChildren.end();
    for ( ; childFace != childEnd; ++childFace )
    {
      if ( childFace->SetBottomSide( bottom, &myBottomIndex ))
      {
        TChildren::iterator orientedCild = childFace;
        for ( childFace = myChildren.begin(); childFace != childEnd; ++childFace ) {
          if ( childFace != orientedCild )
            childFace->SetBottomSide( childFace->GetSide( myBottomIndex ));
        }
        if ( sideIndex )
          *sideIndex = myBottomIndex;
        return true;
      }
    }
  }
  return false;
}

//================================================================================
/*!
 * \brief Return face adjacent to i-th side of this face, (0<i<4)
 */
//================================================================================

_QuadFaceGrid* _QuadFaceGrid::FindAdjacentForSide(int i, vector<_QuadFaceGrid>& faces) const
{
  for ( int iF = 0; iF < faces.size(); ++iF ) {
    _QuadFaceGrid* f  = &faces[ iF ];
    if ( f != this && f->SetBottomSide( GetSide( i )))
      return f;
  }
  return (_QuadFaceGrid*) 0;
}

//================================================================================
/*!
 * \brief Return i-th side
 */
//================================================================================

const _FaceSide& _QuadFaceGrid::GetSide(int i) const
{
  if ( myChildren.empty() )
    return *mySides.GetSide(i);

  _QuadFaceGrid* me = const_cast<_QuadFaceGrid*>(this);
  if ( !me->locateChildren() || !myLeftBottomChild )
    return *mySides.GetSide(i);

  const _QuadFaceGrid* child = myLeftBottomChild;
  switch ( i ){
  case Q_BOTTOM:
  case Q_LEFT:
    break;
  case Q_RIGHT:
    while ( child->myRightBrother )
      child = child->myRightBrother;
    break;
  case Q_TOP:
    while ( child->myUpBrother )
      child = child->myUpBrother;
    break;
  default: ;
  }
  return child->GetSide( i );
}

//================================================================================
/*!
 * \brief Reverse edges in order to have them oriented along axes of the unit box
 */
//================================================================================

void _QuadFaceGrid::ReverseEdges(/*int e1, int e2*/)
{
  myReverse = !myReverse;

// #ifdef DEB_FACES
//   if ( !myFace.IsNull() )
//     TopAbs::Print(myFace.Orientation(), cout);
// #endif

  if ( myChildren.empty() )
  {
//     mySides.GetSide( e1 )->Reverse();
//     mySides.GetSide( e2 )->Reverse();
    DumpVertices();
  }
  else
  {
    DumpVertices();
    TChildren::iterator child = myChildren.begin(), childEnd = myChildren.end();
    for ( ; child != childEnd; ++child )
      child->ReverseEdges( /*e1, e2*/ );
  }
}

//================================================================================
/*!
 * \brief Load nodes of a mesh
 */
//================================================================================

bool _QuadFaceGrid::LoadGrid( SMESH_Mesh& mesh )
{
  if ( !myChildren.empty() )
  {
    // Let child faces load their grids
    TChildren::iterator child = myChildren.begin(), childEnd = myChildren.end();
    for ( ; child != childEnd; ++child ) {
      child->SetID( myID );
      if ( !child->LoadGrid( mesh ) )
        return error( child->GetError() );
    }
    // Fill myGrid with nodes of patches
    return loadCompositeGrid( mesh );
  }

  // ---------------------------------------
  // Fill myGrid with nodes bound to myFace
  // ---------------------------------------

  if ( !myGrid.empty() )
    return true;

  myIndexer._xSize = 1 + mySides.GetSide( Q_BOTTOM )->GetNbSegments( mesh );
  myIndexer._ySize = 1 + mySides.GetSide( Q_LEFT   )->GetNbSegments( mesh );

  myGrid.resize( myIndexer.size() );

  // strore nodes bound to the bottom edge
  mySides.GetSide( Q_BOTTOM )->StoreNodes( mesh, myGrid, myReverse );

  // store the rest nodes row by row

  SMESHDS_SubMesh* faceSubMesh = mesh.GetSubMesh( myFace )->GetSubMeshDS();

  SMDS_MeshNode dummy(0,0,0);
  const SMDS_MeshElement* firstQuad = &dummy;// most left face above the last row of found nodes
  
  int nbFoundNodes = myIndexer._xSize;
  while ( nbFoundNodes != myGrid.size() )
  {
    // first and last nodes of the last filled row of nodes
    const SMDS_MeshNode* n1down = myGrid[ nbFoundNodes - myIndexer._xSize ];
    const SMDS_MeshNode* n2down = myGrid[ nbFoundNodes - myIndexer._xSize + 1];
    const SMDS_MeshNode* n1downLast = myGrid[ nbFoundNodes-1 ];

    // find the first face above the row by the first two left nodes
    //
    // n1up     n2up
    //     o---o
    //     |   |
    //     o---o  o  o  o  o
    //n1down    n2down
    //
    TIDSortedElemSet emptySet, avoidSet;
    avoidSet.insert( firstQuad );
    firstQuad = SMESH_MeshEditor::FindFaceInSet( n1down, n2down, emptySet, avoidSet);
    while ( firstQuad && !faceSubMesh->Contains( firstQuad )) {
      avoidSet.insert( firstQuad );
      firstQuad = SMESH_MeshEditor::FindFaceInSet( n1down, n2down, emptySet, avoidSet);
    }
    if ( !firstQuad || !faceSubMesh->Contains( firstQuad ))
      return error(ERR_LI("Error in _QuadFaceGrid::LoadGrid()"));

    // find the node of quad bound to the left geom edge
    int i2down = firstQuad->GetNodeIndex( n2down );
    const SMDS_MeshNode* n1up = firstQuad->GetNode(( i2down+2 ) % 4 );
    myGrid[ nbFoundNodes++ ] = n1up;
    // the 4-the node of the first quad
    int i1down = firstQuad->GetNodeIndex( n1down );
    const SMDS_MeshNode* n2up = firstQuad->GetNode(( i1down+2 ) % 4 );
    myGrid[ nbFoundNodes++ ] = n2up;

    n1down = n2down;
    n1up   = n2up;
    const SMDS_MeshElement* quad = firstQuad;

    // find the rest nodes by remaining faces above the row
    //
    //             n1up
    //     o---o--o
    //     |   |  | ->
    //     o---o--o  o  o  o
    //                      n1downLast
    //
    while ( n1down != n1downLast )
    {
      // next face
      avoidSet.clear(); avoidSet.insert( quad );
      quad = SMESH_MeshEditor::FindFaceInSet( n1down, n1up, emptySet, avoidSet );
      if ( !quad || quad->NbNodes() % 4 > 0)
        return error(ERR_LI("Error in _QuadFaceGrid::LoadGrid()"));

      // next node
      if ( quad->GetNode( i1down ) != n1down ) // check already found index
        i1down = quad->GetNodeIndex( n1down );
      n2up = quad->GetNode(( i1down+2 ) % 4 );
      myGrid[ nbFoundNodes++ ] = n2up;

      n1down = myGrid[ nbFoundNodes - myIndexer._xSize - 1 ];
      n1up   = n2up;
    }
  }

  DumpGrid(); // debug

  return true;
}

//================================================================================
/*!
 * \brief Find out mutual location of children: find their right and up brothers
 */
//================================================================================

bool _QuadFaceGrid::locateChildren()
{
  if ( myLeftBottomChild )
    return true;

  TChildren::iterator child = myChildren.begin(), childEnd = myChildren.end();

  // find a child sharing it's first bottom vertex with no other brother
  myLeftBottomChild = 0;
  for ( ; !myLeftBottomChild && child != childEnd; ++child )
  {
    TopoDS_Vertex leftVertex = child->GetSide( Q_BOTTOM ).FirstVertex();
    bool sharedVertex = false;
    TChildren::iterator otherChild = myChildren.begin();
    for ( ; otherChild != childEnd && !sharedVertex; ++otherChild )
      if ( otherChild != child )
        sharedVertex = otherChild->mySides.Contain( leftVertex );
    if ( !sharedVertex ) {
      myLeftBottomChild = & (*child);
      DUMP_VERT("0 left bottom Vertex: ",leftVertex );
    }
  }
  if (!myLeftBottomChild)
    return error(ERR_LI("Error in locateChildren()"));

  set< _QuadFaceGrid* > notLocatedChilren;
  for (child = myChildren.begin() ; child != childEnd; ++child )
    notLocatedChilren.insert( & (*child));

  // connect myLeftBottomChild to it's right and upper brothers
  notLocatedChilren.erase( myLeftBottomChild );
  myLeftBottomChild->setBrothers( notLocatedChilren );
  if ( !notLocatedChilren.empty() )
    return error(ERR_LI("Error in locateChildren()"));

  return true;
}

//================================================================================
/*!
 * \brief Fill myGrid with nodes of patches
 */
//================================================================================

bool _QuadFaceGrid::loadCompositeGrid(SMESH_Mesh& mesh)
{
  // Find out mutual location of children: find their right and up brothers
  if ( !locateChildren() )
    return false;

  // Load nodes according to mutual location of children

  // grid size
  myIndexer._xSize = 1 + myLeftBottomChild->GetNbHoriSegments(mesh, /*withBrothers=*/true);
  myIndexer._ySize = 1 + myLeftBottomChild->GetNbVertSegments(mesh, /*withBrothers=*/true);

  myGrid.resize( myIndexer.size() );

  int fromX = myReverse ? myIndexer._xSize : 0;
  if (!myLeftBottomChild->fillGrid( mesh, myGrid, myIndexer, fromX, 0 ))
    return error( myLeftBottomChild->GetError() );

  DumpGrid();

  return true;
}

//================================================================================
/*!
 * \brief Find right an upper brothers among notLocatedBrothers
 */
//================================================================================

void _QuadFaceGrid::setBrothers( set< _QuadFaceGrid* >& notLocatedBrothers )
{
  if ( !notLocatedBrothers.empty() )
  {
    // find right brother
    TopoDS_Vertex rightVertex = GetSide( Q_BOTTOM ).LastVertex();
    DUMP_VERT("1 right bottom Vertex: ",rightVertex );
    set< _QuadFaceGrid* >::iterator brIt, brEnd = notLocatedBrothers.end();
    for ( brIt = notLocatedBrothers.begin(); !myRightBrother && brIt != brEnd; ++brIt )
    {
      _QuadFaceGrid* brother = *brIt;
      TopoDS_Vertex brotherLeftVertex = brother->GetSide( Q_BOTTOM ).FirstVertex();
      DUMP_VERT( "brother left bottom: ", brotherLeftVertex );
      if ( rightVertex.IsSame( brotherLeftVertex )) {
        myRightBrother = brother;
        notLocatedBrothers.erase( myRightBrother );
      }
    }
    // find upper brother
    TopoDS_Vertex upVertex = GetSide( Q_LEFT ).FirstVertex();
    DUMP_VERT("1 left up Vertex: ",upVertex);
    brIt = notLocatedBrothers.begin(), brEnd = notLocatedBrothers.end();
    for ( ; !myUpBrother && brIt != brEnd; ++brIt )
    {
      _QuadFaceGrid* brother = *brIt;
      TopoDS_Vertex brotherLeftVertex = brother->GetSide( Q_BOTTOM ).FirstVertex();
      DUMP_VERT("brother left bottom: ", brotherLeftVertex);
      if ( upVertex.IsSame( brotherLeftVertex )) {
        myUpBrother = brother;
        notLocatedBrothers.erase( myUpBrother );
      }
    }
    // recursive call
    if ( myRightBrother )
      myRightBrother->setBrothers( notLocatedBrothers );
    if ( myUpBrother )
      myUpBrother->setBrothers( notLocatedBrothers );
  }
}

//================================================================================
/*!
 * \brief Store nodes of a simple face into grid starting from (x,y) position
 */
//================================================================================

bool _QuadFaceGrid::fillGrid(SMESH_Mesh&                    theMesh,
                             vector<const SMDS_MeshNode*> & theGrid,
                             const _Indexer&                theIndexer,
                             int                            theX,
                             int                            theY)
{
  if ( myGrid.empty() && !LoadGrid( theMesh ))
    return false;

  // store my own grid in the global grid

  int fromX = myReverse ? theX - myIndexer._xSize: theX;

  for ( int i = 0, x = fromX; i < myIndexer._xSize; ++i, ++x )
    for ( int j = 0, y = theY; j < myIndexer._ySize; ++j, ++y )
      theGrid[ theIndexer( x, y )] = myGrid[ myIndexer( i, j )];

  // store grids of my right and upper brothers

  if ( myRightBrother )
  {
    if ( myReverse )
      fromX += 1;
    else
      fromX += myIndexer._xSize - 1;
    if ( !myRightBrother->fillGrid( theMesh, theGrid, theIndexer, fromX, theY ))
      return error( myRightBrother->GetError() );
  }
  if ( myUpBrother )
  {
    if ( !myUpBrother->fillGrid( theMesh, theGrid, theIndexer,
                                 theX, theY + myIndexer._ySize - 1))
      return error( myUpBrother->GetError() );
  }
  return true;
}

//================================================================================
/*!
 * \brief Return number of segments on the hirizontal sides
 */
//================================================================================

int _QuadFaceGrid::GetNbHoriSegments(SMESH_Mesh& mesh, bool withBrothers) const
{
  int nbSegs = 0;
  if ( myLeftBottomChild )
  {
    nbSegs += myLeftBottomChild->GetNbHoriSegments( mesh, true );
  }
  else
  {
    nbSegs = mySides.GetSide( Q_BOTTOM )->GetNbSegments(mesh);
    if ( withBrothers && myRightBrother )
      nbSegs += myRightBrother->GetNbHoriSegments( mesh, withBrothers );
  }
  return nbSegs;
}

//================================================================================
/*!
 * \brief Return number of segments on the vertical sides
 */
//================================================================================

int _QuadFaceGrid::GetNbVertSegments(SMESH_Mesh& mesh, bool withBrothers) const
{
  int nbSegs = 0;
  if ( myLeftBottomChild )
  {
    nbSegs += myLeftBottomChild->GetNbVertSegments( mesh, true );
  }
  else
  {
    nbSegs = mySides.GetSide( Q_LEFT )->GetNbSegments(mesh);
    if ( withBrothers && myUpBrother )
      nbSegs += myUpBrother->GetNbVertSegments( mesh, withBrothers );
  }
  return nbSegs;
}

//================================================================================
/*!
 * \brief Return a node by its position
 */
//================================================================================

const SMDS_MeshNode* _QuadFaceGrid::GetNode(int iHori, int iVert) const
{
  return myGrid[ myIndexer( iHori, iVert )];
}

//================================================================================
/*!
 * \brief Return node coordinates by its position
 */
//================================================================================

gp_XYZ _QuadFaceGrid::GetXYZ(int iHori, int iVert) const
{
  const SMDS_MeshNode* n = myGrid[ myIndexer( iHori, iVert )];
  return gp_XYZ( n->X(), n->Y(), n->Z() );
}

//================================================================================
/*!
 * \brief Return normal to the face at vertex v
 */
//================================================================================

bool _QuadFaceGrid::GetNormal( const TopoDS_Vertex& v, gp_Vec& n ) const
{
  if ( myChildren.empty() )
  {
    if ( mySides.Contain( v )) {
      try {
        gp_Pnt2d uv = BRep_Tool::Parameters( v, myFace );
        BRepAdaptor_Surface surface( myFace );
        gp_Pnt p; gp_Vec d1u, d1v;
        surface.D1( uv.X(), uv.Y(), p, d1u, d1v );
        n = d1u.Crossed( d1v );
        return true;
      }
      catch (Standard_Failure) {
        return false;
      }
    }
  }
  else
  {
    TChildren::const_iterator child = myChildren.begin(), childEnd = myChildren.end();
    for ( ; child != childEnd; ++child )
      if ( child->GetNormal( v, n ))
        return true;
  }
  return false;
}

//================================================================================
/*!
 * \brief Dumps coordinates of grid nodes
 */
//================================================================================

void _QuadFaceGrid::DumpGrid() const
{
#ifdef DEB_GRID
  const char* names[] = { "B_BOTTOM", "B_RIGHT", "B_TOP", "B_LEFT", "B_FRONT", "B_BACK" };
  cout << "****** Face " << names[ myID ] << endl;

  if ( myChildren.empty() || !myGrid.empty() )
  {
    cout << "x size: " << myIndexer._xSize << "; y size: " << myIndexer._ySize << endl;
    for ( int y = 0; y < myIndexer._ySize; ++y ) {
      cout << "-- row " << y << endl;
      for ( int x = 0; x < myIndexer._xSize; ++x ) {
        const SMDS_MeshNode* n = myGrid[ myIndexer( x, y ) ];
        cout << x << " ( " << n->X() << ", " << n->Y() << ", " << n->Z() << " )" << endl;
      }
    }
  }
  else
  {
    cout << "Nb children: " << myChildren.size() << endl;
    TChildren::const_iterator child = myChildren.begin(), childEnd = myChildren.end();
    for ( int i=0; child != childEnd; ++child, ++i ) {
      cout << "   *** SUBFACE " << i+1 << endl;
      ((_QuadFaceGrid&)(*child)).SetID( myID );
      child->DumpGrid();
    }
  }
#endif
}

//================================================================================
/*!
 * \brief Dump vertices
 */
//================================================================================

void _QuadFaceGrid::DumpVertices() const
{
#ifdef DEB_FACES
  cout << "****** Face ";
  const char* names[] = { "B_BOTTOM", "B_RIGHT", "B_TOP", "B_LEFT", "B_FRONT", "B_BACK" };
  if ( myID >= B_BOTTOM && myID < B_BACK )
    cout << names[ myID ] << endl;
  else
    cout << "UNDEFINED" << endl;

  if ( myChildren.empty() )
  {
    for ( int i = 0; i < 4; ++i )
    {
      cout << "  Side "; mySides.GetSide( i )->Dump();
    }
  }
  else
  {
    cout << "-- Nb children: " << myChildren.size() << endl;
    TChildren::const_iterator child = myChildren.begin(), childEnd = myChildren.end();
    for ( int i=0; child != childEnd; ++child, ++i ) {
      cout << "   *** SUBFACE " << i+1 << endl;
      ((_QuadFaceGrid&)(*child)).SetID( myID );
      child->DumpVertices();
    }
  }
#endif
}

//=======================================================================
//function : _FaceSide
//purpose  : copy constructor
//=======================================================================

_FaceSide::_FaceSide(const _FaceSide& other)
{
  myEdge = other.myEdge;
  myChildren = other.myChildren;
  myNbChildren = other.myNbChildren;
  myVertices.Assign( other.myVertices );
  myID = other.myID;
}

//================================================================================
/*!
 * \brief Construct a face side of one edge
 */
//================================================================================

_FaceSide::_FaceSide(const TopoDS_Edge& edge):
  myEdge( edge ), myNbChildren(0)
{
  if ( !edge.IsNull() )
    for ( TopExp_Explorer exp( edge, TopAbs_VERTEX ); exp.More(); exp.Next() )
      //myVertices.insert( ptr ( exp.Current() ));
      myVertices.Add( exp.Current() );
}

//================================================================================
/*!
 * \brief Construct a face side of several edges
 */
//================================================================================

_FaceSide::_FaceSide(const list<TopoDS_Edge>& edges):
  myNbChildren(0)
{
  list<TopoDS_Edge>::const_iterator edge = edges.begin(), eEnd = edges.end();
  for ( ; edge != eEnd; ++edge ) {
    myChildren.push_back( _FaceSide( *edge ));
    myNbChildren++;
//     myVertices.insert( myChildren.back().myVertices.begin(),
//                        myChildren.back().myVertices.end() );
    myVertices.Add( myChildren.back().FirstVertex() );
    myVertices.Add( myChildren.back().LastVertex() );
    myChildren.back().SetID( Q_CHILD ); // not to splice them
  }
}

//=======================================================================
//function : GetSide
//purpose  : 
//=======================================================================

_FaceSide* _FaceSide::GetSide(const int i)
{
  if ( i >= myNbChildren )
    return 0;

  #ifdef __BORLANDC__
  vector< _FaceSide >::iterator side = myChildren.begin();
  #else
  list< _FaceSide >::iterator side = myChildren.begin();
  #endif

  if ( i )
    std::advance( side, i );
  return & (*side);
}

//=======================================================================
//function : GetSide
//purpose  : 
//=======================================================================

const _FaceSide* _FaceSide::GetSide(const int i) const
{
  return const_cast< _FaceSide* >(this)->GetSide(i);
}

//=======================================================================
//function : NbVertices
//purpose  : return nb of vertices in the side
//=======================================================================

int _FaceSide::NbVertices() const
{
  if ( myChildren.empty() )
    return myVertices.Extent();
//     return myVertices.size();

  return myNbChildren + 1;
}

//=======================================================================
//function : FirstVertex
//purpose  : 
//=======================================================================

TopoDS_Vertex _FaceSide::FirstVertex() const
{
  if ( myChildren.empty() )
    return TopExp::FirstVertex( myEdge, Standard_True );

  return myChildren.front().FirstVertex();
}

//=======================================================================
//function : LastVertex
//purpose  : 
//=======================================================================

TopoDS_Vertex _FaceSide::LastVertex() const
{
  if ( myChildren.empty() )
    return TopExp::LastVertex( myEdge, Standard_True );

  return myChildren.back().LastVertex();
}

//=======================================================================
//function : Vertex
//purpose  : 
//=======================================================================

TopoDS_Vertex _FaceSide::Vertex(int i) const
{
  if ( myChildren.empty() )
    return i ? LastVertex() : FirstVertex();
      
  if ( i >= myNbChildren )
    return myChildren.back().LastVertex();
  
  return GetSide(i)->FirstVertex();
}

//=======================================================================
//function : Contain
//purpose  : 
//=======================================================================

bool _FaceSide::Contain( const _FaceSide& side, int* which ) const
{
  if ( !which || myChildren.empty() )
  {
    if ( which )
      *which = 0;
    int nbCommon = 0;
//     set<const TopoDS_TShape*>::iterator v, vEnd = side.myVertices.end();
//     for ( v = side.myVertices.begin(); v != vEnd; ++v )
//       nbCommon += ( myVertices.find( *v ) != myVertices.end() );
    TopTools_MapIteratorOfMapOfShape vIt ( side.myVertices );
    for ( ; vIt.More(); vIt.Next() )
      nbCommon += ( myVertices.Contains( vIt.Key() ));
    return (nbCommon > 1);
  }

  #ifdef __BORLANDC__
  vector< _FaceSide >::const_iterator mySide = myChildren.begin(), sideEnd = myChildren.end();
  #else
  list< _FaceSide >::const_iterator mySide = myChildren.begin(), sideEnd = myChildren.end();
  #endif

  for ( int i = 0; mySide != sideEnd; ++mySide, ++i ) {
    if ( mySide->Contain( side )) {
      *which = i;
      return true;
    }
  }
  return false;
}

//=======================================================================
//function : Contain
//purpose  : 
//=======================================================================

bool _FaceSide::Contain( const TopoDS_Vertex& vertex ) const
{
  return myVertices.Contains( vertex );
//   return myVertices.find( ptr( vertex )) != myVertices.end();
}

//=======================================================================
//function : AppendSide
//purpose  : 
//=======================================================================

void _FaceSide::AppendSide( const _FaceSide& side )
{
  if ( !myEdge.IsNull() )
  {
    myChildren.push_back( *this );
    myNbChildren = 1;
    myEdge.Nullify();
  }
  myChildren.push_back( side );
  myNbChildren++;
  //myVertices.insert( side.myVertices.begin(), side.myVertices.end() );
  TopTools_MapIteratorOfMapOfShape vIt ( side.myVertices );
  for ( ; vIt.More(); vIt.Next() )
    myVertices.Add( vIt.Key() );

  myID = Q_PARENT;
  myChildren.back().SetID( EQuadSides( myNbChildren-1 ));
}

//=======================================================================
//function : SetBottomSide
//purpose  : 
//=======================================================================

void _FaceSide::SetBottomSide( int i )
{
  if ( i > 0 && myID == Q_PARENT ) {
  
    #ifdef __BORLANDC__
    list< _FaceSide > aList;
    aList.clear();

    aList.assign(myChildren.begin(), myChildren.end());
    myChildren.clear();

    list< _FaceSide >::iterator sideEnd, side = aList.begin();
    std::advance( side, i );
    aList.splice( aList.begin(), aList, side, aList.end() );
    side = aList.begin(), sideEnd = aList.end();
    for ( int i = 0; side != sideEnd; ++side, ++i ) {
      side->SetID( EQuadSides(i) );
      side->SetBottomSide(i);
    }

    myChildren.assign(aList.begin(), aList.end());
    aList.clear();

    #else

    list< _FaceSide >::iterator sideEnd, side = myChildren.begin();
    std::advance( side, i );
    myChildren.splice( myChildren.begin(), myChildren, side, myChildren.end() );
    side = myChildren.begin(), sideEnd = myChildren.end();
    for ( int i = 0; side != sideEnd; ++side, ++i ) {
      side->SetID( EQuadSides(i) );
      side->SetBottomSide(i);
    }
    
    #endif
  }
}

//=======================================================================
//function : GetNbSegments
//purpose  : 
//=======================================================================

int _FaceSide::GetNbSegments(SMESH_Mesh& mesh) const
{
  int nb = 0;
  if ( myChildren.empty() )
  {
    nb = mesh.GetSubMesh(myEdge)->GetSubMeshDS()->NbElements();
  }
  else
  {
    #ifdef __BORLANDC__
    vector< _FaceSide >::const_iterator side = myChildren.begin(), sideEnd = myChildren.end();
    #else
    list< _FaceSide >::const_iterator side = myChildren.begin(), sideEnd = myChildren.end();
    #endif

    for ( ; side != sideEnd; ++side )
      nb += side->GetNbSegments(mesh);
  }
  return nb;
}

//=======================================================================
//function : StoreNodes
//purpose  : 
//=======================================================================

bool _FaceSide::StoreNodes(SMESH_Mesh&                   mesh,
                           vector<const SMDS_MeshNode*>& myGrid,
                           bool                          reverse )
{
  list< TopoDS_Edge > edges;
  if ( myChildren.empty() )
  {
    edges.push_back( myEdge );
  }
  else
  {
    #ifdef __BORLANDC__
    vector< _FaceSide >::const_iterator side = myChildren.begin(), sideEnd = myChildren.end();
    #else
    list< _FaceSide >::const_iterator side = myChildren.begin(), sideEnd = myChildren.end();
    #endif

    for ( ; side != sideEnd; ++side )
      if ( reverse )
        edges.push_front( side->myEdge );
      else
        edges.push_back ( side->myEdge );
  }
  int nbNodes = 0;
  list< TopoDS_Edge >::iterator edge = edges.begin(), eEnd = edges.end();
  for ( ; edge != eEnd; ++edge )
  {
    map< double, const SMDS_MeshNode* > nodes;
    bool ok = SMESH_Algo::GetSortedNodesOnEdge( mesh.GetMeshDS(),
                                                *edge,
                                                /*ignoreMediumNodes=*/true,
                                                nodes);
    if ( !ok ) return false;

    bool forward = ( edge->Orientation() == TopAbs_FORWARD );
    if ( reverse ) forward = !forward;
    if ( forward )
    {
      map< double, const SMDS_MeshNode* >::iterator u_node, nEnd = nodes.end();
      for ( u_node = nodes.begin(); u_node != nEnd; ++u_node )
        myGrid[ nbNodes++ ] = u_node->second;
    }
    else 
    {
      map< double, const SMDS_MeshNode* >::reverse_iterator u_node, nEnd = nodes.rend();
      for ( u_node = nodes.rbegin(); u_node != nEnd; ++u_node )
        myGrid[ nbNodes++ ] = u_node->second;
    }
    nbNodes--; // node on vertex present in two adjacent edges
  }
  return nbNodes > 0;
}

//=======================================================================
//function : Dump
//purpose  : dump end vertices
//=======================================================================

void _FaceSide::Dump() const
{
  if ( myChildren.empty() )
  {
    const char* sideNames[] = { "Q_BOTTOM", "Q_RIGHT", "Q_TOP", "Q_LEFT", "Q_CHILD", "Q_PARENT" };
    if ( myID >= Q_BOTTOM && myID < Q_PARENT )
      cout << sideNames[ myID ] << endl;
    else
      cout << "<UNDEFINED ID>" << endl;
    TopoDS_Vertex f = FirstVertex();
    TopoDS_Vertex l = LastVertex();
    gp_Pnt pf = BRep_Tool::Pnt(f);
    gp_Pnt pl = BRep_Tool::Pnt(l);
    cout << "\t ( "<< ptr( f ) << " - " << ptr( l )<< " )"
         << "\t ( "<< pf.X()<<", "<<pf.Y()<<", "<<pf.Z()<<" ) - "
         << " ( "<< pl.X()<<", "<<pl.Y()<<", "<<pl.Z()<<" )"<<endl;
  }
  else
  {
    #ifdef __BORLANDC__
    vector< _FaceSide >::const_iterator side = myChildren.begin(), sideEnd = myChildren.end();
    #else
    list< _FaceSide >::const_iterator side = myChildren.begin(), sideEnd = myChildren.end();
    #endif

    for ( ; side != sideEnd; ++side ) {
      side->Dump();
      cout << "\t";
    }
  }
}

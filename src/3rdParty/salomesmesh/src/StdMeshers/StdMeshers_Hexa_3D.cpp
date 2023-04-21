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

//  SMESH SMESH : implementation of SMESH idl descriptions
//  File   : StdMeshers_Hexa_3D.cxx
//           Moved here from SMESH_Hexa_3D.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "StdMeshers_Hexa_3D.hxx"

#include "StdMeshers_CompositeHexa_3D.hxx"
#include "StdMeshers_FaceSide.hxx"
#include "StdMeshers_HexaFromSkin_3D.hxx"
#include "StdMeshers_Penta_3D.hxx"
#include "StdMeshers_Prism_3D.hxx"
#include "StdMeshers_Quadrangle_2D.hxx"
#include "StdMeshers_ViscousLayers.hxx"

#include "SMESH_Comment.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"

#include "SMDS_MeshNode.hxx"

#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_SequenceOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>

#include "utilities.h"
#include "Utils_ExceptHandlers.hxx"

typedef SMESH_Comment TComm;

using namespace std;

static SMESH_ComputeErrorPtr ComputePentahedralMesh(SMESH_Mesh &,
                                                    const TopoDS_Shape &,
                                                    SMESH_ProxyMesh* proxyMesh=0);

static bool EvaluatePentahedralMesh(SMESH_Mesh &, const TopoDS_Shape &,
                                    MapShapeNbElems &);

//=============================================================================
/*!
 * Constructor
 */
//=============================================================================

StdMeshers_Hexa_3D::StdMeshers_Hexa_3D(int hypId, int studyId, SMESH_Gen * gen)
  :SMESH_3D_Algo(hypId, studyId, gen)
{
  MESSAGE("StdMeshers_Hexa_3D::StdMeshers_Hexa_3D");
  _name = "Hexa_3D";
  _shapeType = (1 << TopAbs_SHELL) | (1 << TopAbs_SOLID);       // 1 bit /shape type
  _requireShape = false;
  _compatibleHypothesis.push_back("ViscousLayers");
}

//=============================================================================
/*!
 * Destructor
 */
//=============================================================================

StdMeshers_Hexa_3D::~StdMeshers_Hexa_3D()
{
  MESSAGE("StdMeshers_Hexa_3D::~StdMeshers_Hexa_3D");
}

//=============================================================================
/*!
 * Retrieves defined hypotheses
 */
//=============================================================================

bool StdMeshers_Hexa_3D::CheckHypothesis
                         (SMESH_Mesh&                          aMesh,
                          const TopoDS_Shape&                  aShape,
                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  // check nb of faces in the shape
/*  PAL16229
  aStatus = SMESH_Hypothesis::HYP_BAD_GEOMETRY;
  int nbFaces = 0;
  for (TopExp_Explorer exp(aShape, TopAbs_FACE); exp.More(); exp.Next())
    if ( ++nbFaces > 6 )
      break;
  if ( nbFaces != 6 )
    return false;
*/

  _viscousLayersHyp = NULL;

  const list<const SMESHDS_Hypothesis*>& hyps =
    GetUsedHypothesis(aMesh, aShape, /*ignoreAuxiliary=*/false);
  list <const SMESHDS_Hypothesis* >::const_iterator h = hyps.begin();
  if ( h == hyps.end())
  {
    aStatus = SMESH_Hypothesis::HYP_OK;
    return true;
  }

  // only StdMeshers_ViscousLayers can be used
  aStatus = HYP_OK;
  for ( ; h != hyps.end(); ++h )
  {
    if ( !(_viscousLayersHyp = dynamic_cast< const StdMeshers_ViscousLayers*> ( *h )))
      break;
  }
  if ( !_viscousLayersHyp )
    aStatus = HYP_INCOMPATIBLE;
  else
    error( _viscousLayersHyp->CheckHypothesis( aMesh, aShape, aStatus ));

  return aStatus == HYP_OK;
}

namespace
{
  //=============================================================================

  typedef boost::shared_ptr< FaceQuadStruct > FaceQuadStructPtr;

  // symbolic names of box sides
  enum EBoxSides{ B_BOTTOM=0, B_RIGHT, B_TOP, B_LEFT, B_FRONT, B_BACK, B_NB_SIDES };

  // symbolic names of sides of quadrangle
  enum EQuadSides{ Q_BOTTOM=0, Q_RIGHT, Q_TOP, Q_LEFT, Q_NB_SIDES };

  //=============================================================================
  /*!
   * \brief Container of nodes of structured mesh on a qudrangular geom FACE
   */
  struct _FaceGrid
  {
    // face sides
    FaceQuadStructPtr _quad;

    // map of (node parameter on EDGE) to (column (vector) of nodes)
    TParam2ColumnMap _u2nodesMap;

    // node column's taken form _u2nodesMap taking into account sub-shape orientation
    vector<TNodeColumn> _columns;

    // geometry of a cube side
    TopoDS_Face _sideF;

    const SMDS_MeshNode* GetNode(int iCol, int iRow) const
    {
      return _columns[iCol][iRow];
    }
    gp_XYZ GetXYZ(int iCol, int iRow) const
    {
      return SMESH_TNodeXYZ( GetNode( iCol, iRow ));
    }
  };

  //================================================================================
  /*!
   * \brief Converter of a pair of integers to a sole index
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
   * \brief Appends a range of node columns from a map to another map
   */
  template< class TMapIterator >
  void append( TParam2ColumnMap& toMap, TMapIterator from, TMapIterator to )
  {
    const SMDS_MeshNode* lastNode = toMap.rbegin()->second[0];
    const SMDS_MeshNode* firstNode = from->second[0];
    if ( lastNode == firstNode )
      from++;
    double u = toMap.rbegin()->first;
    for (; from != to; ++from )
    {
      u += 1;
      TParam2ColumnMap::iterator u2nn = toMap.insert( toMap.end(), make_pair ( u, TNodeColumn()));
      u2nn->second.swap( from->second );
    }
  }

  //================================================================================
  /*!
   * \brief Finds FaceQuadStruct having a side equal to a given one and rearranges
   *  the found FaceQuadStruct::side to have the given side at a Q_BOTTOM place
   */
  FaceQuadStructPtr getQuadWithBottom( StdMeshers_FaceSidePtr side,
                                       FaceQuadStructPtr      quad[ 6 ])
  {
    FaceQuadStructPtr foundQuad;
    for ( int i = 1; i < 6; ++i )
    {
      if ( !quad[i] ) continue;
      for ( unsigned iS = 0; iS < quad[i]->side.size(); ++iS )
      {
        const StdMeshers_FaceSidePtr side2 = quad[i]->side[iS];
        if (( side->FirstVertex().IsSame( side2->FirstVertex() ) ||
              side->FirstVertex().IsSame( side2->LastVertex() ))
            &&
            ( side->LastVertex().IsSame( side2->FirstVertex() ) ||
              side->LastVertex().IsSame( side2->LastVertex() ))
            )
        {
          if ( iS != Q_BOTTOM )
          {
            vector< FaceQuadStruct::Side > newSides;
            for ( unsigned j = iS; j < quad[i]->side.size(); ++j )
              newSides.push_back( quad[i]->side[j] );
            for ( unsigned j = 0; j < iS; ++j )
              newSides.push_back( quad[i]->side[j] );
            quad[i]->side.swap( newSides );
          }
          foundQuad.swap(quad[i]);
          return foundQuad;
        }
      }
    }
    return foundQuad;
  }
  //================================================================================
  /*!
   * \brief Returns true if the 1st base node of sideGrid1 belongs to sideGrid2
   */
  //================================================================================

  bool beginsAtSide( const _FaceGrid&     sideGrid1,
                     const _FaceGrid&     sideGrid2,
                     SMESH_ProxyMesh::Ptr proxymesh )
  {
    const TNodeColumn& col0  = sideGrid2._u2nodesMap.begin()->second;
    const TNodeColumn& col1  = sideGrid2._u2nodesMap.rbegin()->second;
    const SMDS_MeshNode* n00 = col0.front();
    const SMDS_MeshNode* n01 = col0.back();
    const SMDS_MeshNode* n10 = col1.front();
    const SMDS_MeshNode* n11 = col1.back();
    const SMDS_MeshNode* n = (sideGrid1._u2nodesMap.begin()->second)[0];
    if ( proxymesh )
    {
      n00 = proxymesh->GetProxyNode( n00 );
      n10 = proxymesh->GetProxyNode( n10 );
      n01 = proxymesh->GetProxyNode( n01 );
      n11 = proxymesh->GetProxyNode( n11 );
      n   = proxymesh->GetProxyNode( n );
    }
    return ( n == n00 || n == n01 || n == n10 || n == n11 );
  }
}

//=============================================================================
/*!
 * Generates hexahedron mesh on hexaedron like form using algorithm from
 * "Application de l'interpolation transfinie � la cr�ation de maillages
 *  C0 ou G1 continus sur des triangles, quadrangles, tetraedres, pentaedres
 *  et hexaedres d�form�s."
 * Alain PERONNET - 8 janvier 1999
 */
//=============================================================================

bool StdMeshers_Hexa_3D::Compute(SMESH_Mesh &         aMesh,
                                 const TopoDS_Shape & aShape)
{
  // PAL14921. Enable catching std::bad_alloc and Standard_OutOfMemory outside
  //Unexpect aCatch(SalomeException);
  MESSAGE("StdMeshers_Hexa_3D::Compute");
  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();

  // Shape verification
  // ----------------------

  // shape must be a solid (or a shell) with 6 faces
  TopExp_Explorer exp(aShape,TopAbs_SHELL);
  if ( !exp.More() )
    return error(COMPERR_BAD_SHAPE, "No SHELL in the geometry");
  if ( exp.Next(), exp.More() )
    return error(COMPERR_BAD_SHAPE, "More than one SHELL in the geometry");

  TopTools_IndexedMapOfShape FF;
  TopExp::MapShapes( aShape, TopAbs_FACE, FF);
  if ( FF.Extent() != 6)
  {
    static StdMeshers_CompositeHexa_3D compositeHexa(_gen->GetANewId(), 0, _gen);
    if ( !compositeHexa.Compute( aMesh, aShape ))
      return error( compositeHexa.GetComputeError() );
    return true;
  }

  // Find sides of a cube
  // ---------------------
  
  FaceQuadStructPtr quad[ 6 ];
  StdMeshers_Quadrangle_2D quadAlgo( _gen->GetANewId(), GetStudyId(), _gen);
  for ( int i = 0; i < 6; ++i )
  {
    if ( !( quad[i] = FaceQuadStructPtr( quadAlgo.CheckNbEdges( aMesh, FF( i+1 )))))
      return error( quadAlgo.GetComputeError() );
    if ( quad[i]->side.size() != 4 )
      return error( COMPERR_BAD_SHAPE, "Not a quadrangular box side" );
  }

  _FaceGrid aCubeSide[ 6 ];

  swap( aCubeSide[B_BOTTOM]._quad, quad[0] );
  swap( aCubeSide[B_BOTTOM]._quad->side[ Q_RIGHT],// direct the normal of bottom quad inside cube
        aCubeSide[B_BOTTOM]._quad->side[ Q_LEFT ] );

  aCubeSide[B_FRONT]._quad = getQuadWithBottom( aCubeSide[B_BOTTOM]._quad->side[Q_BOTTOM], quad );
  aCubeSide[B_RIGHT]._quad = getQuadWithBottom( aCubeSide[B_BOTTOM]._quad->side[Q_RIGHT ], quad );
  aCubeSide[B_BACK ]._quad = getQuadWithBottom( aCubeSide[B_BOTTOM]._quad->side[Q_TOP   ], quad );
  aCubeSide[B_LEFT ]._quad = getQuadWithBottom( aCubeSide[B_BOTTOM]._quad->side[Q_LEFT  ], quad );
  if ( aCubeSide[B_FRONT ]._quad )
    aCubeSide[B_TOP]._quad = getQuadWithBottom( aCubeSide[B_FRONT ]._quad->side[Q_TOP ], quad );

  for ( int i = 1; i < 6; ++i )
    if ( !aCubeSide[i]._quad )
      return error( COMPERR_BAD_SHAPE );

  // Make viscous layers
  // --------------------

  SMESH_ProxyMesh::Ptr proxymesh;
  if ( _viscousLayersHyp )
  {
    proxymesh = _viscousLayersHyp->Compute( aMesh, aShape, /*makeN2NMap=*/ true );
    if ( !proxymesh )
      return false;
  }

  // Check if there are triangles on cube sides
  // -------------------------------------------

  if ( aMesh.NbTriangles() > 0 )
  {
    for ( int i = 0; i < 6; ++i )
    {
      const TopoDS_Face& sideF = aCubeSide[i]._quad->face;
      const SMESHDS_SubMesh* smDS =
        proxymesh ? proxymesh->GetSubMesh( sideF ) : meshDS->MeshElements( sideF );
      if ( !SMESH_MesherHelper::IsSameElemGeometry( smDS, SMDSGeom_QUADRANGLE,
                                                    /*nullSubMeshRes=*/false ))
      {
        SMESH_ComputeErrorPtr err = ComputePentahedralMesh(aMesh, aShape, proxymesh.get());
        return error( err );
      }
    }
  }

  // Check presence of regular grid mesh on FACEs of the cube
  // ------------------------------------------------------------

  // tool creating quadratic elements if needed
  SMESH_MesherHelper helper (aMesh);
  _quadraticMesh = helper.IsQuadraticSubMesh(aShape);

  for ( int i = 0; i < 6; ++i )
  {
    const TopoDS_Face& F = aCubeSide[i]._quad->face;
    StdMeshers_FaceSidePtr baseQuadSide = aCubeSide[i]._quad->side[ Q_BOTTOM ];
    list<TopoDS_Edge> baseEdges( baseQuadSide->Edges().begin(), baseQuadSide->Edges().end() );

    // assure correctness of node positions on baseE:
    // helper.GetNodeU() will fix positions if they are wrong
    helper.ToFixNodeParameters( true );
    for ( int iE = 0; iE < baseQuadSide->NbEdges(); ++iE )
    {
      const TopoDS_Edge& baseE = baseQuadSide->Edge( iE );
      if ( SMESHDS_SubMesh* smDS = meshDS->MeshElements( baseE ))
      {
        bool ok;
        helper.SetSubShape( baseE );
        SMDS_ElemIteratorPtr eIt = smDS->GetElements();
        while ( eIt->more() )
        {
          const SMDS_MeshElement* e = eIt->next();
          // expect problems on a composite side
          try { helper.GetNodeU( baseE, e->GetNode(0), e->GetNode(1), &ok); }
          catch (...) {}
          try { helper.GetNodeU( baseE, e->GetNode(1), e->GetNode(0), &ok); }
          catch (...) {}
        }
      }
    }

    // load grid
    bool ok =
      helper.LoadNodeColumns( aCubeSide[i]._u2nodesMap, F, baseEdges, meshDS, proxymesh.get());
    if ( ok )
    {
      // check if the loaded grid corresponds to nb of quadrangles on the FACE
      const SMESHDS_SubMesh* faceSubMesh =
        proxymesh ? proxymesh->GetSubMesh( F ) : meshDS->MeshElements( F );
      const int nbQuads = faceSubMesh->NbElements();
      const int nbHor = aCubeSide[i]._u2nodesMap.size() - 1;
      const int nbVer = aCubeSide[i]._u2nodesMap.begin()->second.size() - 1;
      ok = ( nbQuads == nbHor * nbVer );
    }
    if ( !ok )
    {
      SMESH_ComputeErrorPtr err = ComputePentahedralMesh(aMesh, aShape, proxymesh.get());
      return error( err );
    }
  }

  // Orient loaded grids of cube sides along axis of the unitary cube coord system
  bool isReverse[6];
  isReverse[B_BOTTOM] = beginsAtSide( aCubeSide[B_BOTTOM], aCubeSide[B_RIGHT ], proxymesh );
  isReverse[B_TOP   ] = beginsAtSide( aCubeSide[B_TOP   ], aCubeSide[B_RIGHT ], proxymesh );
  isReverse[B_FRONT ] = beginsAtSide( aCubeSide[B_FRONT ], aCubeSide[B_RIGHT ], proxymesh );
  isReverse[B_BACK  ] = beginsAtSide( aCubeSide[B_BACK  ], aCubeSide[B_RIGHT ], proxymesh );
  isReverse[B_LEFT  ] = beginsAtSide( aCubeSide[B_LEFT  ], aCubeSide[B_BACK  ], proxymesh );
  isReverse[B_RIGHT ] = beginsAtSide( aCubeSide[B_RIGHT ], aCubeSide[B_BACK  ], proxymesh );
  for ( int i = 0; i < 6; ++i )
  {
    aCubeSide[i]._columns.resize( aCubeSide[i]._u2nodesMap.size() );

    int iFwd = 0, iRev = aCubeSide[i]._columns.size()-1;
    int* pi = isReverse[i] ? &iRev : &iFwd;
    TParam2ColumnMap::iterator u2nn = aCubeSide[i]._u2nodesMap.begin();
    for ( ; iFwd < aCubeSide[i]._columns.size(); --iRev, ++iFwd, ++u2nn )
      aCubeSide[i]._columns[ *pi ].swap( u2nn->second );

    aCubeSide[i]._u2nodesMap.clear();
  }
  
  if ( proxymesh )
    for ( int i = 0; i < 6; ++i )
      for ( unsigned j = 0; j < aCubeSide[i]._columns.size(); ++j)
        for ( unsigned k = 0; k < aCubeSide[i]._columns[j].size(); ++k)
        {
          const SMDS_MeshNode* & n = aCubeSide[i]._columns[j][k];
          n = proxymesh->GetProxyNode( n );
        }

  // 4) Create internal nodes of the cube
  // -------------------------------------

  helper.SetSubShape( aShape );
  helper.SetElementsOnShape(true);

  // shortcuts to sides
  _FaceGrid* fBottom = & aCubeSide[ B_BOTTOM ];
  _FaceGrid* fRight  = & aCubeSide[ B_RIGHT  ];
  _FaceGrid* fTop    = & aCubeSide[ B_TOP    ];
  _FaceGrid* fLeft   = & aCubeSide[ B_LEFT   ];
  _FaceGrid* fFront  = & aCubeSide[ B_FRONT  ];
  _FaceGrid* fBack   = & aCubeSide[ B_BACK   ];

  // cube size measured in nb of nodes
  int x, xSize = fBottom->_columns.size() , X = xSize - 1;
  int y, ySize = fLeft->_columns.size()   , Y = ySize - 1;
  int z, zSize = fLeft->_columns[0].size(), Z = zSize - 1;

  // columns of internal nodes "rising" from nodes of fBottom
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

  // projection points of the internal node on cube sub-shapes by which
  // coordinates of the internal node are computed
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
      // a column to fill in during z loop
      vector< const SMDS_MeshNode* >& column = columns[ colIndex( x, y )];
      // projection points on horizontal edges
      pointsOnShapes[ SMESH_Block::ID_Ex00 ] = fBottom->GetXYZ( x, 0 );
      pointsOnShapes[ SMESH_Block::ID_Ex10 ] = fBottom->GetXYZ( x, Y );
      pointsOnShapes[ SMESH_Block::ID_E0y0 ] = fBottom->GetXYZ( 0, y );
      pointsOnShapes[ SMESH_Block::ID_E1y0 ] = fBottom->GetXYZ( X, y );
      pointsOnShapes[ SMESH_Block::ID_Ex01 ] = fTop->GetXYZ( x, 0 );
      pointsOnShapes[ SMESH_Block::ID_Ex11 ] = fTop->GetXYZ( x, Y );
      pointsOnShapes[ SMESH_Block::ID_E0y1 ] = fTop->GetXYZ( 0, y );
      pointsOnShapes[ SMESH_Block::ID_E1y1 ] = fTop->GetXYZ( X, y );
      // projection points on horizontal faces
      pointsOnShapes[ SMESH_Block::ID_Fxy0 ] = fBottom->GetXYZ( x, y );
      pointsOnShapes[ SMESH_Block::ID_Fxy1 ] = fTop   ->GetXYZ( x, y );
      for ( z = 1; z < zSize-1; ++z ) // z loop
      {
        params.SetCoord( 3, z / double(Z) );
        // projection points on vertical edges
        pointsOnShapes[ SMESH_Block::ID_E00z ] = fFront->GetXYZ( 0, z );    
        pointsOnShapes[ SMESH_Block::ID_E10z ] = fFront->GetXYZ( X, z );    
        pointsOnShapes[ SMESH_Block::ID_E01z ] = fBack->GetXYZ( 0, z );    
        pointsOnShapes[ SMESH_Block::ID_E11z ] = fBack->GetXYZ( X, z );
        // projection points on vertical faces
        pointsOnShapes[ SMESH_Block::ID_Fx0z ] = fFront->GetXYZ( x, z );    
        pointsOnShapes[ SMESH_Block::ID_Fx1z ] = fBack ->GetXYZ( x, z );    
        pointsOnShapes[ SMESH_Block::ID_F0yz ] = fLeft ->GetXYZ( y, z );    
        pointsOnShapes[ SMESH_Block::ID_F1yz ] = fRight->GetXYZ( y, z );

        // compute internal node coordinates
        gp_XYZ coords;
        SMESH_Block::ShellPoint( params, pointsOnShapes, coords );
        column[ z ] = helper.AddNode( coords.X(), coords.Y(), coords.Z() );

      }
    }
  }

  // side data no more needed, free memory
  for ( int i = 0; i < 6; ++i )
    aCubeSide[i]._columns.clear();

  // 5) Create hexahedrons
  // ---------------------

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

//=============================================================================
/*!
 *  Evaluate
 */
//=============================================================================

bool StdMeshers_Hexa_3D::Evaluate(SMESH_Mesh & aMesh,
                                  const TopoDS_Shape & aShape,
                                  MapShapeNbElems& aResMap)
{
  vector < SMESH_subMesh * >meshFaces;
  TopTools_SequenceOfShape aFaces;
  for (TopExp_Explorer exp(aShape, TopAbs_FACE); exp.More(); exp.Next()) {
    aFaces.Append(exp.Current());
    SMESH_subMesh *aSubMesh = aMesh.GetSubMeshContaining(exp.Current());
    ASSERT(aSubMesh);
    meshFaces.push_back(aSubMesh);
  }
  if (meshFaces.size() != 6) {
    //return error(COMPERR_BAD_SHAPE, TComm(meshFaces.size())<<" instead of 6 faces in a block");
    static StdMeshers_CompositeHexa_3D compositeHexa(-10, 0, aMesh.GetGen());
    return compositeHexa.Evaluate(aMesh, aShape, aResMap);
  }
  
  int i = 0;
  for(; i<6; i++) {
    //TopoDS_Shape aFace = meshFaces[i]->GetSubShape();
    TopoDS_Shape aFace = aFaces.Value(i+1);
    SMESH_Algo *algo = _gen->GetAlgo(aMesh, aFace);
    if( !algo ) {
      std::vector<int> aResVec(SMDSEntity_Last);
      for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
      SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
      aResMap.insert(std::make_pair(sm,aResVec));
      SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
      smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,"Submesh can not be evaluated",this));
      return false;
    }
    string algoName = algo->GetName();
    bool isAllQuad = false;
    if (algoName == "Quadrangle_2D") {
      MapShapeNbElemsItr anIt = aResMap.find(meshFaces[i]);
      if( anIt == aResMap.end() ) continue;
      std::vector<int> aVec = (*anIt).second;
      int nbtri = Max(aVec[SMDSEntity_Triangle],aVec[SMDSEntity_Quad_Triangle]);
      if( nbtri == 0 )
        isAllQuad = true;
    }
    if ( ! isAllQuad ) {
      return EvaluatePentahedralMesh(aMesh, aShape, aResMap);
    }
  }
  
  // find number of 1d elems for 1 face
  int nb1d = 0;
  TopTools_MapOfShape Edges1;
  bool IsQuadratic = false;
  bool IsFirst = true;
  for (TopExp_Explorer exp(aFaces.Value(1), TopAbs_EDGE); exp.More(); exp.Next()) {
    Edges1.Add(exp.Current());
    SMESH_subMesh *sm = aMesh.GetSubMesh(exp.Current());
    if( sm ) {
      MapShapeNbElemsItr anIt = aResMap.find(sm);
      if( anIt == aResMap.end() ) continue;
      std::vector<int> aVec = (*anIt).second;
      nb1d += Max(aVec[SMDSEntity_Edge],aVec[SMDSEntity_Quad_Edge]);
      if(IsFirst) {
        IsQuadratic = (aVec[SMDSEntity_Quad_Edge] > aVec[SMDSEntity_Edge]);
        IsFirst = false;
      }
    }
  }
  // find face opposite to 1 face
  int OppNum = 0;
  for(i=2; i<=6; i++) {
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
  for(i=2; i<=6; i++) {
    if( i == OppNum ) continue;
    MapShapeNbElemsItr anIt = aResMap.find( meshFaces[i-1] );
    if( anIt == aResMap.end() ) continue;
    std::vector<int> aVec = (*anIt).second;
    nb2d += Max(aVec[SMDSEntity_Quadrangle],aVec[SMDSEntity_Quad_Quadrangle]);
  }
  
  MapShapeNbElemsItr anIt = aResMap.find( meshFaces[0] );
  std::vector<int> aVec = (*anIt).second;
  int nb2d_face0 = Max(aVec[SMDSEntity_Quadrangle],aVec[SMDSEntity_Quad_Quadrangle]);
  int nb0d_face0 = aVec[SMDSEntity_Node];

  std::vector<int> aResVec(SMDSEntity_Last);
  for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
  if(IsQuadratic) {
    aResVec[SMDSEntity_Quad_Hexa] = nb2d_face0 * ( nb2d/nb1d );
    int nb1d_face0_int = ( nb2d_face0*4 - nb1d ) / 2;
    aResVec[SMDSEntity_Node] = nb0d_face0 * ( 2*nb2d/nb1d - 1 ) - nb1d_face0_int * nb2d/nb1d;
  }
  else {
    aResVec[SMDSEntity_Node] = nb0d_face0 * ( nb2d/nb1d - 1 );
    aResVec[SMDSEntity_Hexa] = nb2d_face0 * ( nb2d/nb1d );
  }
  SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
  aResMap.insert(std::make_pair(sm,aResVec));

  return true;
}

//================================================================================
/*!
 * \brief Computes hexahedral mesh from 2D mesh of block
 */
//================================================================================

bool StdMeshers_Hexa_3D::Compute(SMESH_Mesh & aMesh, SMESH_MesherHelper* aHelper)
{
  static StdMeshers_HexaFromSkin_3D * algo = 0;
  if ( !algo ) {
    SMESH_Gen* gen = aMesh.GetGen();
    algo = new StdMeshers_HexaFromSkin_3D( gen->GetANewId(), 0, gen );
  }
  algo->InitComputeError();
  algo->Compute( aMesh, aHelper );
  return error( algo->GetComputeError());
}

//================================================================================
/*!
 * \brief Return true if the algorithm can mesh this shape
 *  \param [in] aShape - shape to check
 *  \param [in] toCheckAll - if true, this check returns OK if all shapes are OK,
 *              else, returns OK if at least one shape is OK
 */
//================================================================================

bool StdMeshers_Hexa_3D::IsApplicable( const TopoDS_Shape & aShape, bool toCheckAll )
{
  TopExp_Explorer exp0( aShape, TopAbs_SOLID );
  if ( !exp0.More() ) return false;

  for ( ; exp0.More(); exp0.Next() )
  {
    int nbFoundShells = 0;
    TopExp_Explorer exp1( exp0.Current(), TopAbs_SHELL );
    for ( ; exp1.More(); exp1.Next(), ++nbFoundShells)
      if ( nbFoundShells == 2 ) break;
    if ( nbFoundShells != 1 ) {
      if ( toCheckAll ) return false;
      continue;
    }   
    exp1.Init( exp0.Current(), TopAbs_FACE );
    int nbEdges = SMESH_MesherHelper::Count( exp1.Current(), TopAbs_EDGE, /*ignoreSame=*/true );
    bool ok = ( nbEdges > 3 );
    if ( toCheckAll && !ok ) return false;
    if ( !toCheckAll && ok ) return true;
  }
  return toCheckAll;
};

//=======================================================================
//function : ComputePentahedralMesh
//purpose  : 
//=======================================================================

SMESH_ComputeErrorPtr ComputePentahedralMesh(SMESH_Mesh &          aMesh,
                                             const TopoDS_Shape &  aShape,
                                             SMESH_ProxyMesh*      proxyMesh)
{
  SMESH_ComputeErrorPtr err = SMESH_ComputeError::New();
  if ( proxyMesh )
  {
    err->myName = COMPERR_BAD_INPUT_MESH;
    err->myComment = "Can't build pentahedral mesh on viscous layers";
    return err;
  }
  bool bOK;
  StdMeshers_Penta_3D anAlgo;
  //
  bOK=anAlgo.Compute(aMesh, aShape);
  //
  err = anAlgo.GetComputeError();
  //
  if ( !bOK && anAlgo.ErrorStatus() == 5 )
  {
    static StdMeshers_Prism_3D * aPrism3D = 0;
    if ( !aPrism3D ) {
      SMESH_Gen* gen = aMesh.GetGen();
      aPrism3D = new StdMeshers_Prism_3D( gen->GetANewId(), 0, gen );
    }
    SMESH_Hypothesis::Hypothesis_Status aStatus;
    if ( aPrism3D->CheckHypothesis( aMesh, aShape, aStatus ) ) {
      aPrism3D->InitComputeError();
      bOK = aPrism3D->Compute( aMesh, aShape );
      err = aPrism3D->GetComputeError();
    }
  }
  return err;
}


//=======================================================================
//function : EvaluatePentahedralMesh
//purpose  : 
//=======================================================================

bool EvaluatePentahedralMesh(SMESH_Mesh & aMesh,
                             const TopoDS_Shape & aShape,
                             MapShapeNbElems& aResMap)
{
  StdMeshers_Penta_3D anAlgo;
  bool bOK = anAlgo.Evaluate(aMesh, aShape, aResMap);

  //err = anAlgo.GetComputeError();
  //if ( !bOK && anAlgo.ErrorStatus() == 5 )
  if( !bOK ) {
    static StdMeshers_Prism_3D * aPrism3D = 0;
    if ( !aPrism3D ) {
      SMESH_Gen* gen = aMesh.GetGen();
      aPrism3D = new StdMeshers_Prism_3D( gen->GetANewId(), 0, gen );
    }
    SMESH_Hypothesis::Hypothesis_Status aStatus;
    if ( aPrism3D->CheckHypothesis( aMesh, aShape, aStatus ) ) {
      return aPrism3D->Evaluate(aMesh, aShape, aResMap);
    }
  }

  return bOK;
}

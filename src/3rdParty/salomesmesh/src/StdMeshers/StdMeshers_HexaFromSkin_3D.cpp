// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

// File      : StdMeshers_HexaFromSkin_3D.cxx
// Created   : Wed Jan 27 12:28:07 2010
// Author    : Edward AGAPOV (eap)

#include "StdMeshers_HexaFromSkin_3D.hxx"

#include "SMDS_VolumeOfNodes.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMESH_Block.hxx"
#include "SMESH_MeshAlgos.hxx"
#include "SMESH_MesherHelper.hxx"

#include <gp_Ax2.hxx>

#include <limits>

// Define error message and _MYDEBUG_ if needed
#ifdef _DEBUG_
#define BAD_MESH_ERR \
  error(SMESH_Comment("Can't detect block-wise structure of the input 2D mesh.\n" \
                      __FILE__ ":" )<<__LINE__)
//#define _MYDEBUG_
#else
#define BAD_MESH_ERR \
  error(SMESH_Comment("Can't detect block-wise structure of the input 2D mesh"))
#endif


// Debug output
#ifdef _MYDEBUG_
#define _DUMP_(msg) cout << msg << endl
#else
#define _DUMP_(msg)
#endif


namespace
{
  enum EBoxSides //!< sides of the block
    {
      B_BOTTOM=0, B_RIGHT, B_TOP, B_LEFT, B_FRONT, B_BACK, NB_BLOCK_SIDES
    };
#ifdef _MYDEBUG_
  const char* SBoxSides[] = //!< names of block sides -- needed for DEBUG only
    {
      "BOTTOM", "RIGHT", "TOP", "LEFT", "FRONT", "BACK", "UNDEFINED"
    };
#endif
  enum EQuadEdge //!< edges of quadrangle side
    {
      Q_BOTTOM = 0, Q_RIGHT, Q_TOP, Q_LEFT, NB_QUAD_SIDES
    };


  //================================================================================
  /*!
   * \brief return logical coordinates (i.e. min or max) of ends of edge
   */
  //================================================================================

  bool getEdgeEnds(EQuadEdge edge, bool& xMax1, bool& yMax1, bool& xMax2, bool& yMax2 )
  {
    xMax1=0, yMax1=0, xMax2=1, yMax2=1;
    switch( edge )
    {
    case Q_BOTTOM: yMax2 = 0; break;
    case Q_RIGHT:  xMax1 = 1; break;
    case Q_TOP:    yMax1 = 1; break;
    case Q_LEFT:   xMax2 = 0; break;
    default:
      return false;
    }
    return true;
  }

  //================================================================================
  /*!
   * \brief return true if a node is at block corner
   *
   * This check is valid for simple cases only
   */
  //================================================================================

  bool isCornerNode( const SMDS_MeshNode* n )
  {
    int nbF = n ? n->NbInverseElements( SMDSAbs_Face ) : 1;
    if ( nbF % 2 )
      return true;

    set<const SMDS_MeshNode*> nodesInInverseFaces;
    SMDS_ElemIteratorPtr fIt = n->GetInverseElementIterator(SMDSAbs_Face );
    while ( fIt->more() )
    {
      const SMDS_MeshElement* face = fIt->next();
      nodesInInverseFaces.insert( face->begin_nodes(), face->end_nodes() );
    }

    return nodesInInverseFaces.size() != ( 6 + (nbF/2-1)*3 );
  }

  //================================================================================
  /*!
   * \brief check element type
   */
  //================================================================================

  bool isQuadrangle(const SMDS_MeshElement* e)
  {
    return ( e && e->NbCornerNodes() == 4 );
  }

  //================================================================================
  /*!
   * \brief return opposite node of a quadrangle face
   */
  //================================================================================

  const SMDS_MeshNode* oppositeNode(const SMDS_MeshElement* quad, int iNode)
  {
    return quad->GetNode( (iNode+2) % 4 );
  }

  //================================================================================
  /*!
   * \brief Converter of a pair of integers to a sole index
   */
  struct _Indexer
  {
    int _xSize, _ySize;
    _Indexer( int xSize=0, int ySize=0 ): _xSize(xSize), _ySize(ySize) {}
    int size() const { return _xSize * _ySize; }
    int operator()(int x, int y) const { return y * _xSize + x; }
  };
  //================================================================================
  /*!
   * \brief Oriented converter of a pair of integers to a sole index 
   */
  class _OrientedIndexer : public _Indexer
  {
  public:
    enum OriFlags //!< types of block side orientation
      {
        REV_X = 1, REV_Y = 2, SWAP_XY = 4, MAX_ORI = REV_X|REV_Y|SWAP_XY
      };
    _OrientedIndexer( const _Indexer& indexer, const int oriFlags ):
      _Indexer( indexer._xSize, indexer._ySize ),
      _xSize (indexer._xSize), _ySize(indexer._ySize),
      _xRevFun((oriFlags & REV_X) ? & reverse : & lazy),
      _yRevFun((oriFlags & REV_Y) ? & reverse : & lazy),
      _swapFun((oriFlags & SWAP_XY ) ? & swap : & lazy)
    {
      (*_swapFun)( _xSize, _ySize );
    }
    //!< Return index by XY
    int operator()(int x, int y) const
    {
      (*_xRevFun)( x, const_cast<int&>( _xSize ));
      (*_yRevFun)( y, const_cast<int&>( _ySize ));
      (*_swapFun)( x, y );
      return _Indexer::operator()(x,y);
    }
    //!< Return index for a corner
    int corner(bool xMax, bool yMax) const
    {
      int x = xMax, y = yMax, size = 2;
      (*_xRevFun)( x, size );
      (*_yRevFun)( y, size );
      (*_swapFun)( x, y );
      return _Indexer::operator()(x ? _Indexer::_xSize-1 : 0 , y ? _Indexer::_ySize-1 : 0);
    }
    int xSize() const { return _xSize; }
    int ySize() const { return _ySize; }
  private:
    _Indexer _indexer;
    int _xSize, _ySize;

    typedef void (*TFun)(int& x, int& y);
    TFun _xRevFun, _yRevFun, _swapFun;
    
    static void lazy   (int&, int&) {}
    static void reverse(int& x, int& size) { x = size - x - 1; }
    static void swap   (int& x, int& y) { std::swap(x,y); }
  };
  //================================================================================
  /*!
   * \brief Structure corresponding to the meshed side of block
   */
  struct _BlockSide
  {
    vector<const SMDS_MeshNode*> _grid;
    _Indexer                     _index;
    int                          _nbBlocksExpected;
    int                          _nbBlocksFound;

#ifdef _DEBUG_ // want to get SIGSEGV in case of invalid index
#define _grid_access_(pobj, i) pobj->_grid[ ((i) < pobj->_grid.size()) ? i : int(1e100)]
#else
#define _grid_access_(pobj, i) pobj->_grid[ i ]
#endif
    //!< Return node at XY
    const SMDS_MeshNode* getNode(int x, int y) const { return _grid_access_(this, _index( x,y ));}
    //!< Set node at XY
    void setNode(int x, int y, const SMDS_MeshNode* n) { _grid_access_(this, _index( x,y )) = n; }
    //!< Return an edge
    SMESH_OrientedLink getEdge(EQuadEdge edge) const
    {
      bool x1, y1, x2, y2; getEdgeEnds( edge, x1, y1, x2, y2 );
      return SMESH_OrientedLink( getCornerNode ( x1, y1 ), getCornerNode( x2, y2 ));
    }
    //!< Return a corner node
    const SMDS_MeshNode* getCornerNode(bool isXMax, bool isYMax) const
    {
      return getNode( isXMax ? _index._xSize-1 : 0 , isYMax ? _index._ySize-1 : 0 );
    }
    const SMDS_MeshElement* getCornerFace(const SMDS_MeshNode* cornerNode) const;
    //!< True if all blocks this side belongs to have been found
    bool isBound() const { return _nbBlocksExpected <= _nbBlocksFound; }
    //!< Return coordinates of node at XY
    gp_XYZ getXYZ(int x, int y) const { return SMESH_TNodeXYZ( getNode( x, y )); }
    //!< Return gravity center of the four corners and the middle node
    gp_XYZ getGC() const
    {
      gp_XYZ xyz =
        getXYZ( 0, 0 ) +
        getXYZ( _index._xSize-1, 0 ) +
        getXYZ( 0, _index._ySize-1 ) +
        getXYZ( _index._xSize-1, _index._ySize-1 ) +
        getXYZ( _index._xSize/2, _index._ySize/2 );
      return xyz / 5;
    }
    //!< Return number of mesh faces
    int getNbFaces() const { return (_index._xSize-1) * (_index._ySize-1); }
  };
  //================================================================================
  /*!
   * \brief _BlockSide with changed orientation
   */
  struct _OrientedBlockSide
  {
    _BlockSide*       _side;
    _OrientedIndexer  _index;

    _OrientedBlockSide( _BlockSide* side=0, const int oriFlags=0 ):
      _side(side), _index(side ? side->_index : _Indexer(), oriFlags ) {}
    //!< return coordinates by XY
    gp_XYZ xyz(int x, int y) const
    {
      return SMESH_TNodeXYZ( _grid_access_(_side, _index( x, y )) );
    }
    //!< safely return a node by XY
    const SMDS_MeshNode* node(int x, int y) const
    {
      int i = _index( x, y );
      return ( i < 0 || i >= _side->_grid.size()) ? 0 : _side->_grid[i];
    }
    //!< Return an edge
    SMESH_OrientedLink edge(EQuadEdge edge) const
    {
      bool x1, y1, x2, y2; getEdgeEnds( edge, x1, y1, x2, y2 );
      return SMESH_OrientedLink( cornerNode ( x1, y1 ), cornerNode( x2, y2 ));
    }
    //!< Return a corner node
    const SMDS_MeshNode* cornerNode(bool isXMax, bool isYMax) const
    {
      return _grid_access_(_side, _index.corner( isXMax, isYMax ));
    }
    //!< return its size in nodes
    int getHoriSize() const { return _index.xSize(); }
    int getVertSize() const  { return _index.ySize(); }
    //!< True if _side has been initialized
    operator bool() const { return _side; }
    //! Direct access to _side
    const _BlockSide* operator->() const { return _side; }
    _BlockSide* operator->() { return _side; }
  };
  //================================================================================
  /*!
   * \brief Meshed skin of block
   */
  struct _Block
  {
    _OrientedBlockSide        _side[6]; // 6 sides of a sub-block
    set<const SMDS_MeshNode*> _corners;

    const _OrientedBlockSide& getSide(int i) const { return _side[i]; }
    bool setSide( int i, const _OrientedBlockSide& s)
    {
      if (( _side[i] = s ))
      {
        _corners.insert( s.cornerNode(0,0));
        _corners.insert( s.cornerNode(1,0));
        _corners.insert( s.cornerNode(0,1));
        _corners.insert( s.cornerNode(1,1));
      }
      return s;
    }
    void clear() { for (int i=0;i<6;++i) _side[i]=0; _corners.clear(); }
    bool hasSide( const _OrientedBlockSide& s) const
    {
      if ( s ) for (int i=0;i<6;++i) if ( _side[i] && _side[i]._side == s._side ) return true;
      return false;
    }
    int nbSides() const { int n=0; for (int i=0;i<6;++i) if ( _side[i] ) ++n; return n; }
    bool isValid() const;
  };
  //================================================================================
  /*!
   * \brief Skin mesh possibly containing several meshed blocks
   */
  class _Skin
  {
  public:

    int findBlocks(SMESH_Mesh& mesh);
    //!< return i-th block
    const _Block& getBlock(int i) const { return _blocks[i]; }
    //!< return error description
    const SMESH_Comment& error() const { return _error; }

  private:
    bool fillSide( _BlockSide&             side,
                   const SMDS_MeshElement* cornerQuad,
                   const SMDS_MeshNode*    cornerNode);
    bool fillRowsUntilCorner(const SMDS_MeshElement* quad,
                             const SMDS_MeshNode*    n1,
                             const SMDS_MeshNode*    n2,
                             vector<const SMDS_MeshNode*>& verRow1,
                             vector<const SMDS_MeshNode*>& verRow2,
                             bool alongN1N2 );
    _OrientedBlockSide findBlockSide( EBoxSides           startBlockSide,
                                      EQuadEdge           sharedSideEdge1,
                                      EQuadEdge           sharedSideEdge2,
                                      bool                withGeometricAnalysis,
                                      set< _BlockSide* >& sidesAround);
    //!< update own data and data of the side bound to block
    void setSideBoundToBlock( _BlockSide& side )
    {
      if ( side._nbBlocksFound++, side.isBound() )
        for ( int e = 0; e < int(NB_QUAD_SIDES); ++e )
          _edge2sides[ side.getEdge( (EQuadEdge) e ) ].erase( &side );
    }
    //!< store reason of error
    SMESH_Comment      _error;
    int error(const SMESH_Comment& reason) { _error = reason; return 0; }


    list< _BlockSide > _allSides;
    vector< _Block >   _blocks;

    //map< const SMDS_MeshNode*, set< _BlockSide* > > _corner2sides;
    map< SMESH_OrientedLink, set< _BlockSide* > > _edge2sides;
  };

  //================================================================================
  /*!
   * \brief Find and return number of submeshes corresponding to blocks
   */
  //================================================================================

  int _Skin::findBlocks(SMESH_Mesh& mesh)
  {
    SMESHDS_Mesh* meshDS = mesh.GetMeshDS();

    // Find a node at any block corner

    SMDS_NodeIteratorPtr nIt = meshDS->nodesIterator(/*idInceasingOrder=*/true);
    if ( !nIt->more() ) return error("Empty mesh");

    const SMDS_MeshNode* nCorner = 0;
    while ( nIt->more() )
    {
      nCorner = nIt->next();
      if ( isCornerNode( nCorner ))
        break;
      else
        nCorner = 0;
    }
    if ( !nCorner )
      return BAD_MESH_ERR;

    // --------------------------------------------------------------------
    // Find all block sides starting from mesh faces sharing the corner node
    // --------------------------------------------------------------------

    int nbFacesOnSides = 0;
    TIDSortedElemSet cornerFaces; // corner faces of found _BlockSide's
    list< const SMDS_MeshNode* > corners( 1, nCorner );
    list< const SMDS_MeshNode* >::iterator corner = corners.begin();
    while ( corner != corners.end() )
    {
      SMDS_ElemIteratorPtr faceIt = (*corner)->GetInverseElementIterator( SMDSAbs_Face );
      while ( faceIt->more() )
      {
        const SMDS_MeshElement* face = faceIt->next();
        if ( !cornerFaces.insert( face ).second )
          continue; // already loaded block side

        if ( !isQuadrangle( face ))
          return error("Non-quadrangle elements in the input mesh");

        if ( _allSides.empty() || !_allSides.back()._grid.empty() )
          _allSides.push_back( _BlockSide() );

        _BlockSide& side = _allSides.back();
        if ( !fillSide( side, face, *corner ) )
        {
          if ( !_error.empty() )
            return false;
        }
        else
        {
          for ( int isXMax = 0; isXMax < 2; ++isXMax )
            for ( int isYMax = 0; isYMax < 2; ++isYMax )
            {
              const SMDS_MeshNode* nCorner = side.getCornerNode(isXMax,isYMax );
              corners.push_back( nCorner );
              cornerFaces.insert( side.getCornerFace( nCorner ));
            }
          for ( int e = 0; e < int(NB_QUAD_SIDES); ++e )
            _edge2sides[ side.getEdge( (EQuadEdge) e ) ].insert( &side );

          nbFacesOnSides += side.getNbFaces();
        }
      }
      ++corner;

      // find block sides of other domains if any
      if ( corner == corners.end() && nbFacesOnSides < mesh.NbQuadrangles() )
      {
        while ( nIt->more() )
        {
          nCorner = nIt->next();
          if ( isCornerNode( nCorner ))
            corner = corners.insert( corner, nCorner );
        }
        nbFacesOnSides = mesh.NbQuadrangles();
      }
    }
    
    if ( _allSides.empty() )
      return BAD_MESH_ERR;
    if ( _allSides.back()._grid.empty() )
      _allSides.pop_back();
    _DUMP_("Nb detected sides "<< _allSides.size());

    // ---------------------------
    // Organize sides into blocks
    // ---------------------------

    // analyse sharing of sides by blocks and sort sides by nb of adjacent sides
    int nbBlockSides = 0; // total nb of block sides taking into account their sharing
    multimap<int, _BlockSide* > sortedSides;
    {
      list < _BlockSide >::iterator sideIt = _allSides.begin();
      for ( ; sideIt != _allSides.end(); ++sideIt )
      {
        _BlockSide& side = *sideIt;
        bool isSharedSide = true;
        int nbAdjacent = 0;
        for ( int e = 0; e < int(NB_QUAD_SIDES) && isSharedSide; ++e )
        {
          int nbAdj = _edge2sides[ side.getEdge( (EQuadEdge) e ) ].size();
          nbAdjacent += nbAdj;
          isSharedSide = ( nbAdj > 2 );
        }
        side._nbBlocksFound = 0;
        side._nbBlocksExpected = isSharedSide ? 2 : 1;
        nbBlockSides += side._nbBlocksExpected;
        sortedSides.insert( make_pair( nbAdjacent, & side ));
      }
    }

    // find sides of each block
    int nbBlocks = 0;
    while ( nbBlockSides >= 6 )
    {
      // get any side not bound to all blocks it belongs to
      multimap<int, _BlockSide*>::iterator i_side = sortedSides.begin();
      while ( i_side != sortedSides.end() && i_side->second->isBound())
        ++i_side;

      // start searching for block sides from the got side
      bool ok = true;
      if ( _blocks.empty() || _blocks.back()._side[B_FRONT] )
        _blocks.resize( _blocks.size() + 1 );

      _Block& block = _blocks.back();
      block.setSide( B_FRONT, i_side->second );
      setSideBoundToBlock( *i_side->second );
      nbBlockSides--;

      // edges of adjacent sides of B_FRONT corresponding to front's edges
      EQuadEdge edgeOfFront[4] = { Q_BOTTOM, Q_RIGHT, Q_TOP, Q_LEFT };
      EQuadEdge edgeOfAdj  [4] = { Q_BOTTOM, Q_LEFT, Q_BOTTOM, Q_LEFT };
      // first find all sides detectable w/o advanced analysis,
      // then repeat the search, which then may pass without advanced analysis
      set< _BlockSide* > sidesAround;
      for ( int advAnalys = 0; advAnalys < 2; ++advAnalys )
      {
        // try to find 4 sides adjacent to a FRONT side
        for ( int i = 0; (ok || !advAnalys) && i < NB_QUAD_SIDES; ++i )
          if ( !block._side[i] )
            ok = block.setSide( i, findBlockSide( B_FRONT, edgeOfFront[i], edgeOfAdj[i],
                                                  advAnalys, sidesAround));
        // try to find a BACK side by a TOP one
        if ( ok || !advAnalys)
          if ( !block._side[B_BACK] && block._side[B_TOP] )
            ok = block.setSide( B_BACK, findBlockSide( B_TOP, Q_TOP, Q_TOP,
                                                       advAnalys, sidesAround ));
        if ( !advAnalys ) ok = true;
      }
      ok = block.isValid();
      if ( ok )
      {
        // check if just found block is same as one of previously found blocks
        bool isSame = false;
        for ( int i = 1; i < _blocks.size() && !isSame; ++i )
          isSame = ( block._corners == _blocks[i-1]._corners );
        ok = !isSame;
      }

      // count the found sides
      _DUMP_(endl << "** Block " << _blocks.size() << " valid: " << block.isValid());
      for (int i = 0; i < NB_BLOCK_SIDES; ++i )
      {
        _DUMP_("\tSide "<< SBoxSides[i] <<" "<< block._side[ i ]._side);
        if ( block._side[ i ] )
        {
          if ( ok && i != B_FRONT)
          {
            setSideBoundToBlock( *block._side[ i ]._side );
            nbBlockSides--;
          }
          _DUMP_("\t corners "<<
                 block._side[ i ].cornerNode(0,0)->GetID() << ", " <<
                 block._side[ i ].cornerNode(1,0)->GetID() << ", " <<
                 block._side[ i ].cornerNode(1,1)->GetID() << ", " <<
                 block._side[ i ].cornerNode(0,1)->GetID() << ", "<<endl);
        }
        else
        {
          _DUMP_("\t not found"<<endl);
        }
      }
      if ( !ok )
        block.clear();
      else
        nbBlocks++;
    }
    _DUMP_("Nb found blocks "<< nbBlocks <<endl);

    if ( nbBlocks == 0 && _error.empty() )
      return BAD_MESH_ERR;

    return nbBlocks;
  }

  //================================================================================
  /*!
   * \brief Fill block side data starting from its corner quadrangle
   */
  //================================================================================

  bool _Skin::fillSide( _BlockSide&             side,
                        const SMDS_MeshElement* cornerQuad,
                        const SMDS_MeshNode*    nCorner)
  {
    // Find out size of block side measured in nodes and by the way find two rows
    // of nodes in two directions.

    int x, y, nbX, nbY;
    const SMDS_MeshElement* firstQuad = cornerQuad;
    {
      // get a node on block edge
      int iCorner = firstQuad->GetNodeIndex( nCorner );
      const SMDS_MeshNode* nOnEdge = firstQuad->GetNode( (iCorner+1) % 4);

      // find out size of block side
      vector<const SMDS_MeshNode*> horRow1, horRow2, verRow1, verRow2;
      if ( !fillRowsUntilCorner( firstQuad, nCorner, nOnEdge, horRow1, horRow2, true ) ||
           !fillRowsUntilCorner( firstQuad, nCorner, nOnEdge, verRow1, verRow2, false ))
        return false;
      nbX = horRow1.size(), nbY = verRow1.size();

      // store found nodes
      side._index._xSize = horRow1.size();
      side._index._ySize = verRow1.size();
      side._grid.resize( side._index.size(), NULL );

      for ( x = 0; x < horRow1.size(); ++x )
      {
        side.setNode( x, 0, horRow1[x] );
        side.setNode( x, 1, horRow2[x] );
      }
      for ( y = 0; y < verRow1.size(); ++y )
      {
        side.setNode( 0, y, verRow1[y] );
        side.setNode( 1, y, verRow2[y] );
      }
    }
    // Find the rest nodes

    y = 1; // y of the row to fill
    TIDSortedElemSet emptySet, avoidSet;
    while ( ++y < nbY )
    {
      // get next firstQuad in the next row of quadrangles
      //
      //          n2up
      //     o---o               <- y row
      //     |   |
      //     o---o  o  o  o  o   <- found nodes
      //n1down    n2down       
      //
      int i1down, i2down, i2up;
      const SMDS_MeshNode* n1down = side.getNode( 0, y-1 );
      const SMDS_MeshNode* n2down = side.getNode( 1, y-1 );
      avoidSet.clear(); avoidSet.insert( firstQuad );
      firstQuad = SMESH_MeshAlgos::FindFaceInSet( n1down, n2down, emptySet, avoidSet,
                                                   &i1down, &i2down);
      if ( !isQuadrangle( firstQuad ))
        return BAD_MESH_ERR;

      const SMDS_MeshNode* n2up = oppositeNode( firstQuad, i1down );
      avoidSet.clear(); avoidSet.insert( firstQuad );

      // find the rest nodes in the y-th row by faces in the row

      x = 1; 
      while ( ++x < nbX )
      {
        const SMDS_MeshElement* quad = SMESH_MeshAlgos::FindFaceInSet( n2up, n2down, emptySet,
                                                                        avoidSet, &i2up, &i2down);
        if ( !isQuadrangle( quad ))
          return BAD_MESH_ERR;

        n2up   = oppositeNode( quad, i2down );
        n2down = oppositeNode( quad, i2up );
        avoidSet.clear(); avoidSet.insert( quad );

        side.setNode( x, y, n2up );
      }
    }

    // check side validity
    bool ok =
      side.getCornerFace( side.getCornerNode( 0, 0 )) &&
      side.getCornerFace( side.getCornerNode( 1, 0 )) &&
      side.getCornerFace( side.getCornerNode( 0, 1 )) &&
      side.getCornerFace( side.getCornerNode( 1, 1 ));

    return ok;
  }

  //================================================================================
  /*!
   * \brief Return true if it's possible to make a loop over corner2Sides starting
   * from the startSide
   */
  //================================================================================

  bool isClosedChainOfSides( _BlockSide*                                        startSide,
                             map< const SMDS_MeshNode*, list< _BlockSide* > > & corner2Sides )
  {
    // get start and end nodes
    const SMDS_MeshNode *n1 = 0, *n2 = 0, *n;
    for ( int y = 0; y < 2; ++y )
      for ( int x = 0; x < 2; ++x )
      {
        n = startSide->getCornerNode(x,y);
        if ( !corner2Sides.count( n )) continue;
        if ( n1 )
          n2 = n;
        else
          n1 = n;
      }
    if ( !n2 ) return false;

    map< const SMDS_MeshNode*, list< _BlockSide* > >::iterator
      c2sides = corner2Sides.find( n1 );
    if ( c2sides == corner2Sides.end() ) return false;

    int nbChainLinks = 1;
    n = n1;
    _BlockSide* prevSide = startSide;
    while ( n != n2 )
    {
      // get the next side sharing n
      list< _BlockSide* > & sides = c2sides->second;
      _BlockSide* nextSide = ( sides.back() == prevSide ? sides.front() : sides.back() );
      if ( nextSide == prevSide ) return false;

      // find the next corner of the nextSide being in corner2Sides
      n1 = n;
      n = 0;
      for ( int y = 0; y < 2 && !n; ++y )
        for ( int x = 0; x < 2; ++x )
        {
          n = nextSide->getCornerNode(x,y);
          c2sides = corner2Sides.find( n );
          if ( n == n1 || c2sides == corner2Sides.end() )
            n = 0;
          else
            break;
        }
      if ( !n ) return false;

      prevSide = nextSide;
      nbChainLinks++;
    }

    return ( n == n2 && nbChainLinks == NB_QUAD_SIDES );
  }

  //================================================================================
  /*!
   * \brief Try to find a block side adjacent to the given side by given edge
   */
  //================================================================================

  _OrientedBlockSide _Skin::findBlockSide( EBoxSides           startBlockSide,
                                           EQuadEdge           sharedSideEdge1,
                                           EQuadEdge           sharedSideEdge2,
                                           bool                withGeometricAnalysis,
                                           set< _BlockSide* >& sidesAround)
  {
    _Block& block = _blocks.back();
    _OrientedBlockSide& side1 = block._side[ startBlockSide ];

    // get corner nodes of the given block edge
    SMESH_OrientedLink edge = side1.edge( sharedSideEdge1 );
    const SMDS_MeshNode* n1 = edge.node1();
    const SMDS_MeshNode* n2 = edge.node2();
    if ( edge._reversed ) swap( n1, n2 );

    // find all sides sharing both nodes n1 and n2
    set< _BlockSide* > sidesOnEdge = _edge2sides[ edge ]; // copy a set

    // exclude loaded sides of block from sidesOnEdge
    for (int i = 0; i < NB_BLOCK_SIDES; ++i )
      if ( block._side[ i ] )
        sidesOnEdge.erase( block._side[ i ]._side );

    int nbSidesOnEdge = sidesOnEdge.size();
    _DUMP_("nbSidesOnEdge "<< nbSidesOnEdge << " " << n1->GetID() << "-" << n2->GetID() );
    if ( nbSidesOnEdge == 0 )
      return 0;

    _BlockSide* foundSide = 0;
    if ( nbSidesOnEdge == 1 )
    {
      foundSide = *sidesOnEdge.begin();
    }
    else
    {
      set< _BlockSide* >::iterator sideIt = sidesOnEdge.begin();
      int nbLoadedSides = block.nbSides();
      if ( nbLoadedSides > 1 )
      {
        // Find the side having more than 2 corners common with already loaded sides
        for (; !foundSide && sideIt != sidesOnEdge.end(); ++sideIt )
        {
          _BlockSide* sideI = *sideIt;
          int nbCommonCorners =
            block._corners.count( sideI->getCornerNode(0,0)) +
            block._corners.count( sideI->getCornerNode(1,0)) +
            block._corners.count( sideI->getCornerNode(0,1)) +
            block._corners.count( sideI->getCornerNode(1,1));
          if ( nbCommonCorners > 2 )
            foundSide = sideI;
        }
      }

      if ( !foundSide )
      {
        if ( !withGeometricAnalysis )
        {
          sidesAround.insert( sidesOnEdge.begin(), sidesOnEdge.end() );
          return 0;
        }
        if ( nbLoadedSides == 1 )
        {
          // Issue 0021529. There are at least 2 sides by each edge and
          // position of block gravity center is undefined.
          // Find a side starting from which we can walk around the startBlockSide

          // fill in corner2Sides
          map< const SMDS_MeshNode*, list< _BlockSide* > > corner2Sides;
          for ( sideIt = sidesAround.begin(); sideIt != sidesAround.end(); ++sideIt )
          {
            _BlockSide* sideI = *sideIt;
            corner2Sides[ sideI->getCornerNode(0,0) ].push_back( sideI );
            corner2Sides[ sideI->getCornerNode(1,0) ].push_back( sideI );
            corner2Sides[ sideI->getCornerNode(0,1) ].push_back( sideI );
            corner2Sides[ sideI->getCornerNode(1,1) ].push_back( sideI );
          }
          // remove corners of startBlockSide from corner2Sides
          set<const SMDS_MeshNode*>::iterator nIt = block._corners.begin();
          for ( ; nIt != block._corners.end(); ++nIt )
            corner2Sides.erase( *nIt );

          // select a side
          for ( sideIt = sidesOnEdge.begin(); sideIt != sidesOnEdge.end(); ++sideIt )
          {
            if ( isClosedChainOfSides( *sideIt, corner2Sides ))
            {
              foundSide = *sideIt;
              break;
            }
          }
          if ( !foundSide )
            return 0;
        }
        else
        {
          // Select one of found sides most close to startBlockSide

          gp_XYZ p1 ( n1->X(),n1->Y(),n1->Z()),  p2 (n2->X(),n2->Y(),n2->Z());
          gp_Vec p1p2( p1, p2 );

          const SMDS_MeshElement* face1 = side1->getCornerFace( n1 );
          gp_XYZ p1Op = SMESH_TNodeXYZ( oppositeNode( face1, face1->GetNodeIndex(n1)));
          gp_Vec side1Dir( p1, p1Op );
          gp_Ax2 pln( p1, p1p2, side1Dir ); // plane with normal p1p2 and X dir side1Dir
          _DUMP_("  Select adjacent for "<< side1._side << " - side dir ("
                 << side1Dir.X() << ", " << side1Dir.Y() << ", " << side1Dir.Z() << ")" );

          map < double , _BlockSide* > angleOfSide;
          for (sideIt = sidesOnEdge.begin(); sideIt != sidesOnEdge.end(); ++sideIt )
          {
            _BlockSide* sideI = *sideIt;
            const SMDS_MeshElement* faceI = sideI->getCornerFace( n1 );
            gp_XYZ p1Op = SMESH_TNodeXYZ( oppositeNode( faceI, faceI->GetNodeIndex(n1)));
            gp_Vec sideIDir( p1, p1Op );
            // compute angle of (sideIDir projection to pln) and (X dir of pln)
            gp_Vec2d sideIDirProj( sideIDir * pln.XDirection(), sideIDir * pln.YDirection());
            double angle = sideIDirProj.Angle( gp::DX2d() );
            if ( angle < 0 ) angle += 2. * M_PI; // angle [0-2*PI]
            angleOfSide.insert( make_pair( angle, sideI ));
            _DUMP_("  "<< sideI << " - side dir ("
                   << sideIDir.X() << ", " << sideIDir.Y() << ", " << sideIDir.Z() << ")"
                   << " angle " << angle);
          }

          gp_XYZ gc(0,0,0); // gravity center of already loaded block sides
          for (int i = 0; i < NB_BLOCK_SIDES; ++i )
            if ( block._side[ i ] )
              gc += block._side[ i ]._side->getGC();
          gc /= nbLoadedSides;

          gp_Vec gcDir( p1, gc );
          gp_Vec2d gcDirProj( gcDir * pln.XDirection(), gcDir * pln.YDirection());
          double gcAngle = gcDirProj.Angle( gp::DX2d() );
          foundSide = gcAngle < 0 ? angleOfSide.rbegin()->second : angleOfSide.begin()->second;
        }
      }
      _DUMP_("  selected "<< foundSide );
    }

    // Orient the found side correctly

    // corners of found side corresponding to nodes n1 and n2
    bool xMax1, yMax1, xMax2, yMax2;
    if ( !getEdgeEnds( sharedSideEdge2, xMax1, yMax1, xMax2, yMax2 ))
      return error(SMESH_Comment("Internal error at ")<<__FILE__<<":"<<__LINE__),
        _OrientedBlockSide(0);

    for ( int ori = 0; ori < _OrientedIndexer::MAX_ORI+1; ++ori )
    {
      _OrientedBlockSide orientedSide( foundSide, ori );
      const SMDS_MeshNode* n12 = orientedSide.cornerNode( xMax1, yMax1);
      const SMDS_MeshNode* n22 = orientedSide.cornerNode( xMax2, yMax2);
      if ( n1 == n12 && n2 == n22 )
        return orientedSide;
    }
    error(SMESH_Comment("Failed to orient a block side found by edge ")<<sharedSideEdge1
          << " of side " << startBlockSide
          << " of block " << _blocks.size());
    return 0;
  }

  //================================================================================
  /*!
   * \brief: Fill rows (which are actually columns,if !alongN1N2) of nodes starting
   * from the given quadrangle until another block corner encounters.
   *  n1 and n2 are at bottom of quad, n1 is at block corner.
   */
  //================================================================================

  bool _Skin::fillRowsUntilCorner(const SMDS_MeshElement*       quad,
                                  const SMDS_MeshNode*          n1,
                                  const SMDS_MeshNode*          n2,
                                  vector<const SMDS_MeshNode*>& row1,
                                  vector<const SMDS_MeshNode*>& row2,
                                  const bool                    alongN1N2 )
  {
    const SMDS_MeshNode* corner1 = n1;

    // Store nodes of quad in the rows and find new n1 and n2 to get
    // the next face so that new n2 is on block edge
    int i1 = quad->GetNodeIndex( n1 );
    int i2 = quad->GetNodeIndex( n2 );
    row1.clear(); row2.clear();
    row1.push_back( n1 );
    if ( alongN1N2 )
    {
      row1.push_back( n2 );
      row2.push_back( oppositeNode( quad, i2 ));
      row2.push_back( n1 = oppositeNode( quad, i1 ));
    }
    else
    {
      row2.push_back( n2 );
      row1.push_back( n2 = oppositeNode( quad, i2 ));
      row2.push_back( n1 = oppositeNode( quad, i1 ));
    }

    if ( isCornerNode( row1[1] ))
      return true;

    // Find the rest nodes
    TIDSortedElemSet emptySet, avoidSet;
    while ( !isCornerNode( n2 ) )
    {
      avoidSet.clear(); avoidSet.insert( quad );
      quad = SMESH_MeshAlgos::FindFaceInSet( n1, n2, emptySet, avoidSet, &i1, &i2 );
      if ( !isQuadrangle( quad ))
        return BAD_MESH_ERR;

      row1.push_back( n2 = oppositeNode( quad, i1 ));
      row2.push_back( n1 = oppositeNode( quad, i2 ));
    }
    return n1 != corner1;
  }

  //================================================================================
  /*!
   * \brief Return a corner face by a corner node
   */
  //================================================================================

  const SMDS_MeshElement* _BlockSide::getCornerFace(const SMDS_MeshNode* cornerNode) const
  {
    int x, y, isXMax, isYMax, found = 0;
    for ( isXMax = 0; isXMax < 2; ++isXMax )
    {
      for ( isYMax = 0; isYMax < 2; ++isYMax )
      {
        x = isXMax ? _index._xSize-1 : 0;
        y = isYMax ? _index._ySize-1 : 0;
        found = ( getNode(x,y) == cornerNode );
        if ( found ) break;
      }
      if ( found ) break;
    }
    if ( !found ) return 0;
    int dx = isXMax ? -1 : +1;
    int dy = isYMax ? -1 : +1;
    const SMDS_MeshNode* n1 = getNode(x,y);
    const SMDS_MeshNode* n2 = getNode(x+dx,y);
    const SMDS_MeshNode* n3 = getNode(x,y+dy);
    const SMDS_MeshNode* n4 = getNode(x+dx,y+dy);
    return SMDS_Mesh::FindFace(n1, n2, n3, n4 );
  }

  //================================================================================
  /*!
   * \brief Checks own validity
   */
  //================================================================================

  bool _Block::isValid() const
  {
    bool ok = ( nbSides() == 6 );

    // check only corners depending on side selection
    EBoxSides adjacent[4] = { B_BOTTOM, B_RIGHT, B_TOP, B_LEFT };
    EQuadEdge edgeAdj [4] = { Q_TOP,    Q_RIGHT, Q_TOP, Q_RIGHT };
    EQuadEdge edgeBack[4] = { Q_BOTTOM, Q_RIGHT, Q_TOP, Q_LEFT };

    for ( int i=0; ok && i < NB_QUAD_SIDES; ++i )
    { 
      SMESH_OrientedLink eBack = _side[ B_BACK      ].edge( edgeBack[i] );
      SMESH_OrientedLink eAdja = _side[ adjacent[i] ].edge( edgeAdj[i] );
      ok = ( eBack == eAdja );
    }
    return ok;
  }

} // namespace

//=======================================================================
//function : StdMeshers_HexaFromSkin_3D
//purpose  : 
//=======================================================================

StdMeshers_HexaFromSkin_3D::StdMeshers_HexaFromSkin_3D(int hypId, int studyId, SMESH_Gen* gen)
  :SMESH_3D_Algo(hypId, studyId, gen)
{
  MESSAGE("StdMeshers_HexaFromSkin_3D::StdMeshers_HexaFromSkin_3D");
  _name = "HexaFromSkin_3D";
}

StdMeshers_HexaFromSkin_3D::~StdMeshers_HexaFromSkin_3D()
{
  MESSAGE("StdMeshers_HexaFromSkin_3D::~StdMeshers_HexaFromSkin_3D");
}

//================================================================================
/*!
 * \brief Main method, which generates hexaheda
 */
//================================================================================

bool StdMeshers_HexaFromSkin_3D::Compute(SMESH_Mesh & aMesh, SMESH_MesherHelper* aHelper)
{
  _Skin skin;
  int nbBlocks = skin.findBlocks(aMesh);
  if ( nbBlocks == 0 )
    return error( skin.error());

  vector< vector< const SMDS_MeshNode* > > columns;
  int x, xSize, y, ySize, z, zSize;
  _Indexer colIndex;

  for ( int i = 0; i < nbBlocks; ++i )
  {
    const _Block& block = skin.getBlock( i );

    // ------------------------------------------
    // Fill columns of nodes with existing nodes
    // ------------------------------------------

    xSize = block.getSide(B_BOTTOM).getHoriSize();
    ySize = block.getSide(B_BOTTOM).getVertSize();
    zSize = block.getSide(B_FRONT ).getVertSize();
    int X = xSize - 1, Y = ySize - 1, Z = zSize - 1;
    colIndex = _Indexer( xSize, ySize );
    columns.resize( colIndex.size() );

    // fill node columns by front and back box sides
    for ( x = 0; x < xSize; ++x ) {
      vector< const SMDS_MeshNode* >& column0 = columns[ colIndex( x, 0 )];
      vector< const SMDS_MeshNode* >& column1 = columns[ colIndex( x, Y )];
      column0.resize( zSize );
      column1.resize( zSize );
      for ( z = 0; z < zSize; ++z ) {
        column0[ z ] = block.getSide(B_FRONT).node( x, z );
        column1[ z ] = block.getSide(B_BACK) .node( x, z );
      }
    }
    // fill node columns by left and right box sides
    for ( y = 1; y < ySize-1; ++y ) {
      vector< const SMDS_MeshNode* >& column0 = columns[ colIndex( 0, y )];
      vector< const SMDS_MeshNode* >& column1 = columns[ colIndex( X, y )];
      column0.resize( zSize );
      column1.resize( zSize );
      for ( z = 0; z < zSize; ++z ) {
        column0[ z ] = block.getSide(B_LEFT) .node( y, z );
        column1[ z ] = block.getSide(B_RIGHT).node( y, z );
      }
    }
    // get nodes from top and bottom box sides
    for ( x = 1; x < xSize-1; ++x ) {
      for ( y = 1; y < ySize-1; ++y ) {
        vector< const SMDS_MeshNode* >& column = columns[ colIndex( x, y )];
        column.resize( zSize );
        column.front() = block.getSide(B_BOTTOM).node( x, y );
        column.back()  = block.getSide(B_TOP)   .node( x, y );
      }
    }

    // ----------------------------
    // Add internal nodes of a box
    // ----------------------------
    // projection points of internal nodes on box sub-shapes by which
    // coordinates of internal nodes are computed
    vector<gp_XYZ> pointOnShape( SMESH_Block::ID_Shell );

    // projections on vertices are constant
    pointOnShape[ SMESH_Block::ID_V000 ] = block.getSide(B_BOTTOM).xyz( 0, 0 );
    pointOnShape[ SMESH_Block::ID_V100 ] = block.getSide(B_BOTTOM).xyz( X, 0 );
    pointOnShape[ SMESH_Block::ID_V010 ] = block.getSide(B_BOTTOM).xyz( 0, Y );
    pointOnShape[ SMESH_Block::ID_V110 ] = block.getSide(B_BOTTOM).xyz( X, Y );
    pointOnShape[ SMESH_Block::ID_V001 ] = block.getSide(B_TOP).xyz( 0, 0 );
    pointOnShape[ SMESH_Block::ID_V101 ] = block.getSide(B_TOP).xyz( X, 0 );
    pointOnShape[ SMESH_Block::ID_V011 ] = block.getSide(B_TOP).xyz( 0, Y );
    pointOnShape[ SMESH_Block::ID_V111 ] = block.getSide(B_TOP).xyz( X, Y );

    for ( x = 1; x < xSize-1; ++x )
    {
      gp_XYZ params; // normalized parameters of internal node within a unit box
      params.SetCoord( 1, x / double(X) );
      for ( y = 1; y < ySize-1; ++y )
      {
        params.SetCoord( 2, y / double(Y) );
        // column to fill during z loop
        vector< const SMDS_MeshNode* >& column = columns[ colIndex( x, y )];
        // projections on horizontal edges
        pointOnShape[ SMESH_Block::ID_Ex00 ] = block.getSide(B_BOTTOM).xyz( x, 0 );
        pointOnShape[ SMESH_Block::ID_Ex10 ] = block.getSide(B_BOTTOM).xyz( x, Y );
        pointOnShape[ SMESH_Block::ID_E0y0 ] = block.getSide(B_BOTTOM).xyz( 0, y );
        pointOnShape[ SMESH_Block::ID_E1y0 ] = block.getSide(B_BOTTOM).xyz( X, y );
        pointOnShape[ SMESH_Block::ID_Ex01 ] = block.getSide(B_TOP).xyz( x, 0 );
        pointOnShape[ SMESH_Block::ID_Ex11 ] = block.getSide(B_TOP).xyz( x, Y );
        pointOnShape[ SMESH_Block::ID_E0y1 ] = block.getSide(B_TOP).xyz( 0, y );
        pointOnShape[ SMESH_Block::ID_E1y1 ] = block.getSide(B_TOP).xyz( X, y );
        // projections on horizontal sides
        pointOnShape[ SMESH_Block::ID_Fxy0 ] = block.getSide(B_BOTTOM).xyz( x, y );
        pointOnShape[ SMESH_Block::ID_Fxy1 ] = block.getSide(B_TOP)   .xyz( x, y );
        for ( z = 1; z < zSize-1; ++z ) // z loop
        {
          params.SetCoord( 3, z / double(Z) );
          // projections on vertical edges
          pointOnShape[ SMESH_Block::ID_E00z ] = block.getSide(B_FRONT).xyz( 0, z );    
          pointOnShape[ SMESH_Block::ID_E10z ] = block.getSide(B_FRONT).xyz( X, z );    
          pointOnShape[ SMESH_Block::ID_E01z ] = block.getSide(B_BACK).xyz( 0, z );    
          pointOnShape[ SMESH_Block::ID_E11z ] = block.getSide(B_BACK).xyz( X, z );
          // projections on vertical sides
          pointOnShape[ SMESH_Block::ID_Fx0z ] = block.getSide(B_FRONT).xyz( x, z );    
          pointOnShape[ SMESH_Block::ID_Fx1z ] = block.getSide(B_BACK) .xyz( x, z );    
          pointOnShape[ SMESH_Block::ID_F0yz ] = block.getSide(B_LEFT) .xyz( y, z );    
          pointOnShape[ SMESH_Block::ID_F1yz ] = block.getSide(B_RIGHT).xyz( y, z );

          // compute internal node coordinates
          gp_XYZ coords;
          SMESH_Block::ShellPoint( params, pointOnShape, coords );
          column[ z ] = aHelper->AddNode( coords.X(), coords.Y(), coords.Z() );

#ifdef DEB_GRID
          // debug
          //cout << "----------------------------------------------------------------------"<<endl;
          //for ( int id = SMESH_Block::ID_V000; id < SMESH_Block::ID_Shell; ++id)
          //{
          //  gp_XYZ p = pointOnShape[ id ];
          //  SMESH_Block::DumpShapeID( id,cout)<<" ( "<<p.X()<<", "<<p.Y()<<", "<<p.Z()<<" )"<<endl;
          //}
          //cout << "Params: ( "<< params.X()<<", "<<params.Y()<<", "<<params.Z()<<" )"<<endl;
          //cout << "coords: ( "<< coords.X()<<", "<<coords.Y()<<", "<<coords.Z()<<" )"<<endl;
#endif
        }
      }
    }
    // ----------------
    // Add hexahedrons
    // ----------------

    // find out orientation by a least distorted hexahedron (issue 0020855);
    // the last is defined by evaluating sum of face normals of 8 corner hexahedrons
    double badness = numeric_limits<double>::max();
    bool isForw = true;
    for ( int xMax = 0; xMax < 2; ++xMax )
      for ( int yMax = 0; yMax < 2; ++yMax )
        for ( int zMax = 0; zMax < 2; ++zMax )
        {
          x = xMax ? xSize-1 : 1;
          y = yMax ? ySize-1 : 1;
          z = zMax ? zSize-1 : 1;
          vector< const SMDS_MeshNode* >& col00 = columns[ colIndex( x-1, y-1 )];
          vector< const SMDS_MeshNode* >& col10 = columns[ colIndex( x  , y-1 )];
          vector< const SMDS_MeshNode* >& col01 = columns[ colIndex( x-1, y   )];
          vector< const SMDS_MeshNode* >& col11 = columns[ colIndex( x  , y )];
          
          const SMDS_MeshNode* n000 = col00[z-1];
          const SMDS_MeshNode* n100 = col10[z-1];
          const SMDS_MeshNode* n010 = col01[z-1];
          const SMDS_MeshNode* n110 = col11[z-1];
          const SMDS_MeshNode* n001 = col00[z];
          const SMDS_MeshNode* n101 = col10[z];
          const SMDS_MeshNode* n011 = col01[z];
          const SMDS_MeshNode* n111 = col11[z];
          SMDS_VolumeOfNodes probeVolume (n000,n010,n110,n100,
                                          n001,n011,n111,n101);
          SMDS_VolumeTool volTool( &probeVolume );
          double Nx=0.,Ny=0.,Nz=0.;
          for ( int iFace = 0; iFace < volTool.NbFaces(); ++iFace )
          {
            double nx,ny,nz;
            volTool.GetFaceNormal( iFace, nx,ny,nz );
            Nx += nx;
            Ny += ny;
            Nz += nz;
          }
          double quality = Nx*Nx + Ny*Ny + Nz*Nz;
          if ( quality < badness )
          {
            badness = quality;
            isForw = volTool.IsForward();
          }
        }

    // add elements
    for ( x = 0; x < xSize-1; ++x ) {
      for ( y = 0; y < ySize-1; ++y ) {
        vector< const SMDS_MeshNode* >& col00 = columns[ colIndex( x, y )];
        vector< const SMDS_MeshNode* >& col10 = columns[ colIndex( x+1, y )];
        vector< const SMDS_MeshNode* >& col01 = columns[ colIndex( x, y+1 )];
        vector< const SMDS_MeshNode* >& col11 = columns[ colIndex( x+1, y+1 )];
        // bottom face normal of a hexa mush point outside the volume
        if ( isForw )
          for ( z = 0; z < zSize-1; ++z )
            aHelper->AddVolume(col00[z],   col01[z],   col11[z],   col10[z],
                               col00[z+1], col01[z+1], col11[z+1], col10[z+1]);
        else
          for ( z = 0; z < zSize-1; ++z )
            aHelper->AddVolume(col00[z],   col10[z],   col11[z],   col01[z],
                               col00[z+1], col10[z+1], col11[z+1], col01[z+1]);
      }
    }
  } // loop on blocks

  return true;
}

//================================================================================
/*!
 * \brief Evaluate nb of hexa
 */
//================================================================================

bool StdMeshers_HexaFromSkin_3D::Evaluate(SMESH_Mesh &         aMesh,
                                          const TopoDS_Shape & aShape,
                                          MapShapeNbElems&     aResMap)
{
  _Skin skin;
  int nbBlocks = skin.findBlocks(aMesh);
  if ( nbBlocks == 0 )
    return error( skin.error());

  bool secondOrder = aMesh.NbFaces( ORDER_QUADRATIC );

  int entity = secondOrder ? SMDSEntity_Quad_Hexa : SMDSEntity_Hexa;
  vector<int>& nbByType = aResMap[ aMesh.GetSubMesh( aShape )];
  if ( entity >= nbByType.size() )
    nbByType.resize( SMDSEntity_Last, 0 );

  for ( int i = 0; i < nbBlocks; ++i )
  {
    const _Block& block = skin.getBlock( i );

    int nbX = block.getSide(B_BOTTOM).getHoriSize();
    int nbY = block.getSide(B_BOTTOM).getVertSize();
    int nbZ = block.getSide(B_FRONT ).getVertSize();

    int nbHexa  = (nbX-1) * (nbY-1) * (nbZ-1);
    int nbNodes = (nbX-2) * (nbY-2) * (nbZ-2);
    if ( secondOrder )
      nbNodes +=
        (nbX-2) * (nbY-2) * (nbZ-1) +
        (nbX-2) * (nbY-1) * (nbZ-2) +
        (nbX-1) * (nbY-2) * (nbZ-2);


    nbByType[ entity ] += nbHexa;
    nbByType[ SMDSEntity_Node ] += nbNodes;
  }

  return true;
}

//================================================================================
/*!
 * \brief Abstract method must be defined but does nothing
 */
//================================================================================

bool StdMeshers_HexaFromSkin_3D::CheckHypothesis(SMESH_Mesh&, const TopoDS_Shape&,
                                                 Hypothesis_Status& aStatus)
{
  aStatus = SMESH_Hypothesis::HYP_OK;
  return true;
}

//================================================================================
/*!
 * \brief Abstract method must be defined but just reports an error as this
 *  algo is not intended to work with shapes
 */
//================================================================================

bool StdMeshers_HexaFromSkin_3D::Compute(SMESH_Mesh&, const TopoDS_Shape&)
{
  return error("Algorithm can't work with geometrical shapes");
}

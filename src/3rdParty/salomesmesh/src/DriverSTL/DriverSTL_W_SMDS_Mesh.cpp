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

#include "DriverSTL_W_SMDS_Mesh.h"

#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <Basics_Utils.hxx>

#include "SMDS_FaceOfNodes.hxx"
#include "SMDS_IteratorOnIterators.hxx"
#include "SMDS_Mesh.hxx"
#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_PolygonalFaceOfNodes.hxx"
#include "SMDS_SetIterator.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMESH_File.hxx"
#include "SMESH_TypeDefs.hxx"

#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <gp_Ax2.hxx>

#include <limits>


// definition des constantes 
static const int LABEL_SIZE = 80;

DriverSTL_W_SMDS_Mesh::DriverSTL_W_SMDS_Mesh()
{
  myIsAscii = false;
}

void DriverSTL_W_SMDS_Mesh::SetIsAscii( const bool theIsAscii )
{
  myIsAscii = theIsAscii;
}

Driver_Mesh::Status DriverSTL_W_SMDS_Mesh::Perform()
{
  // Kernel_Utils::Localizer loc;

  Status aResult = DRS_OK;

  if ( !myMesh ) {
    fprintf(stderr, ">> ERROR : Mesh is null \n");
    return DRS_FAIL;
  }
  findVolumeTriangles();
  if ( myIsAscii )
    aResult = writeAscii();
  else
    aResult = writeBinary();

  return aResult;
}

//================================================================================
/*!
 * \brief Destructor deletes temporary faces
 */
//================================================================================

DriverSTL_W_SMDS_Mesh::~DriverSTL_W_SMDS_Mesh()
{
  for ( unsigned i = 0; i < myVolumeFacets.size(); ++i )
    delete myVolumeFacets[i];
}

//================================================================================
/*!
 * \brief Finds free facets of volumes for which faces are missing in the mesh
 */
//================================================================================

void DriverSTL_W_SMDS_Mesh::findVolumeTriangles()
{
  myNbVolumeTrias = 0;

  SMDS_VolumeTool theVolume;
  SMDS_VolumeIteratorPtr vIt = myMesh->volumesIterator();
  std::vector< const SMDS_MeshNode*> nodes;
  while ( vIt->more() )
  {
    theVolume.Set( vIt->next(), /*ignoreCentralNodes=*/false );
    for ( int iF = 0; iF < theVolume.NbFaces(); ++iF )
      if ( theVolume.IsFreeFace( iF ))
      {
        const SMDS_MeshNode** n = theVolume.GetFaceNodes(iF);
        int                 nbN = theVolume.NbFaceNodes(iF);
        nodes.assign( n, n+nbN );
        if ( !myMesh->FindElement( nodes, SMDSAbs_Face, /*noMedium=*/false))
        {
          if (( nbN == 9 || nbN == 7 ) &&
              ( !theVolume.IsPoly() )) // facet is bi-quaratic
          {
            int nbTria = nbN - 1;
            for ( int iT = 0; iT < nbTria; ++iT )
              myVolumeFacets.push_back( new SMDS_FaceOfNodes( n[8], n[0+iT], n[1+iT] ));
            myNbVolumeTrias += nbTria;
          }
          else
          {
            myVolumeFacets.push_back( new SMDS_PolygonalFaceOfNodes( nodes ));
            myNbVolumeTrias += nbN - 2;
          }
        }
      }
  }
}

//================================================================================
/*!
 * \brief Return iterator on both faces in the mesh and on temporary faces
 */
//================================================================================

SMDS_ElemIteratorPtr DriverSTL_W_SMDS_Mesh::getFaces() const
{
  SMDS_ElemIteratorPtr facesIter = myMesh->elementsIterator(SMDSAbs_Face);
  SMDS_ElemIteratorPtr tmpTriaIter( new SMDS_ElementVectorIterator( myVolumeFacets.begin(),
                                                                    myVolumeFacets.end()));
  typedef std::vector< SMDS_ElemIteratorPtr > TElemIterVector;
  TElemIterVector iters(2);
  iters[0] = facesIter;
  iters[1] = tmpTriaIter;
  
  typedef SMDS_IteratorOnIterators<const SMDS_MeshElement *, TElemIterVector> TItersIter;
  return SMDS_ElemIteratorPtr( new TItersIter( iters ));
}

// static methods

static void writeInteger( const Standard_Integer& theVal, SMESH_File& ofile )
{
  union {
    Standard_Integer i;
    char c[4];
  } u;

  u.i = theVal;

  Standard_Integer entier;
  entier  =  u.c[0] & 0xFF;
  entier |= (u.c[1] & 0xFF) << 0x08;
  entier |= (u.c[2] & 0xFF) << 0x10;
  entier |= (u.c[3] & 0xFF) << 0x18;

  ofile.write( entier );
}

static void writeFloat( const Standard_ShortReal& theVal, SMESH_File& ofile)
{
  union {
    Standard_ShortReal f;
    char c[4]; 
  } u;

  u.f = theVal;

  Standard_Integer entier;

  entier  =  u.c[0] & 0xFF;
  entier |= (u.c[1] & 0xFF) << 0x08;
  entier |= (u.c[2] & 0xFF) << 0x10;
  entier |= (u.c[3] & 0xFF) << 0x18;

  ofile.write( entier );
}

static gp_XYZ getNormale( const SMDS_MeshNode* n1,
                          const SMDS_MeshNode* n2,
                          const SMDS_MeshNode* n3)
{
  SMESH_TNodeXYZ xyz1( n1 );
  SMESH_TNodeXYZ xyz2( n2 );
  SMESH_TNodeXYZ xyz3( n3 );
  gp_XYZ q1 = xyz2 - xyz1;
  gp_XYZ q2 = xyz3 - xyz1;
  gp_XYZ n  = q1 ^ q2;
  double len = n.Modulus();
  if ( len > std::numeric_limits<double>::min() )
    n /= len;

  return n;
}

namespace
{
  /*!
   * \brief Vertex of a polygon. Together with 2 neighbor Vertices represents a triangle
   */
  struct PolyVertex
  {
    SMESH_TNodeXYZ _nxyz;
    gp_XY          _xy;
    PolyVertex*    _prev;
    PolyVertex*    _next;

    void SetNodeAndNext( const SMDS_MeshNode* n, PolyVertex& v )
    {
      _nxyz.Set( n );
      _next = &v;
      v._prev = this;
    }
    PolyVertex* Delete()
    {
      _prev->_next = _next;
      _next->_prev = _prev;
      return _next;
    }
    void GetTriaNodes( const SMDS_MeshNode** nodes) const
    {
      nodes[0] = _prev->_nxyz._node;
      nodes[1] =  this->_nxyz._node;
      nodes[2] = _next->_nxyz._node;
    }

    inline static double Area( const PolyVertex* v0, const PolyVertex* v1, const PolyVertex* v2 )
    {
      gp_XY vPrev = v0->_xy - v1->_xy;
      gp_XY vNext = v2->_xy - v1->_xy;
      return vNext ^ vPrev;
    }
    double TriaArea() const { return Area( _prev, this, _next ); }

    bool IsInsideTria( const PolyVertex* v )
    {
      gp_XY p = _prev->_xy - v->_xy;
      gp_XY t =  this->_xy - v->_xy;
      gp_XY n = _next->_xy - v->_xy;
      const double tol = -1e-12;
      return (( p ^ t ) >= tol &&
              ( t ^ n ) >= tol &&
              ( n ^ p ) >= tol );
      // return ( Area( _prev, this, v ) > 0 &&
      //          Area( this, _next, v ) > 0 &&
      //          Area( _next, _prev, v ) > 0 );
    }
  };

  //================================================================================
  /*!
   * \brief Triangulate a polygon. Assure correct orientation for concave polygons
   */
  //================================================================================

  bool triangulate( std::vector< const SMDS_MeshNode*>& nodes, const size_t nbNodes )
  {
    // connect nodes into a ring
    std::vector< PolyVertex > pv( nbNodes );
    for ( size_t i = 1; i < nbNodes; ++i )
      pv[i-1].SetNodeAndNext( nodes[i-1], pv[i] );
    pv[ nbNodes-1 ].SetNodeAndNext( nodes[ nbNodes-1 ], pv[0] );

    // get a polygon normal
    gp_XYZ normal(0,0,0), p0,v01,v02;
    p0  = pv[0]._nxyz;
    v01 = pv[1]._nxyz - p0;
    for ( size_t i = 2; i < nbNodes; ++i )
    {
      v02 = pv[i]._nxyz - p0;
      normal += v01 ^ v02;
      v01 = v02;
    }
    // project nodes to the found plane
    gp_Ax2 axes;
    try {
      axes = gp_Ax2( p0, normal, v01 );
    }
    catch ( Standard_Failure &) {
      return false;
    }
    for ( size_t i = 0; i < nbNodes; ++i )
    {
      gp_XYZ p = pv[i]._nxyz - p0;
      pv[i]._xy.SetX( axes.XDirection().XYZ() * p );
      pv[i]._xy.SetY( axes.YDirection().XYZ() * p );
    }

    // in a loop, find triangles with positive area and having no vertices inside
    int iN = 0, nbTria = nbNodes - 2;
    nodes.reserve( nbTria * 3 );
    const double minArea = 1e-6;
    PolyVertex* v = &pv[0], *vi;
    int nbVertices = nbNodes, nbBadTria = 0, isGoodTria;
    while ( nbBadTria < nbVertices )
    {
      if (( isGoodTria = v->TriaArea() > minArea ))
      {
        for ( vi = v->_next->_next;
              vi != v->_prev;
              vi = vi->_next )
        {
          if ( v->IsInsideTria( vi ))
            break;
        }
        isGoodTria = ( vi == v->_prev );
      }
      if ( isGoodTria )
      {
        v->GetTriaNodes( &nodes[ iN ] );
        iN += 3;
        v = v->Delete();
        if ( --nbVertices == 3 )
        {
          // last triangle remains
          v->GetTriaNodes( &nodes[ iN ] );
          return true;
        }
        nbBadTria = 0;
      }
      else
      {
        v = v->_next;
        ++nbBadTria;
      }
    }

    // the polygon is invalid; add triangles with positive area
    nbBadTria = 0;
    while ( nbBadTria < nbVertices )
    {
      isGoodTria = v->TriaArea() > minArea;
      if ( isGoodTria )
      {
        v->GetTriaNodes( &nodes[ iN ] );
        iN += 3;
        v = v->Delete();
        if ( --nbVertices == 3 )
        {
          // last triangle remains
          v->GetTriaNodes( &nodes[ iN ] );
          return true;
        }
        nbBadTria = 0;
      }
      else
      {
        v = v->_next;
        ++nbBadTria;
      }
    }

    // add all the rest triangles
    while ( nbVertices >= 3 )
    {
      v->GetTriaNodes( &nodes[ iN ] );
      iN += 3;
      v = v->Delete();
      --nbVertices;
    }

    return true;

  } // triangulate()
} // namespace

//================================================================================
/*!
 * \brief Return nb triangles in a decomposed mesh face
 *  \retval int - number of triangles
 */
//================================================================================

static int getNbTriangles( const SMDS_MeshElement* face)
{
  // WARNING: counting triangles must be coherent with getTriangles()
  switch ( face->GetEntityType() )
  {
  case SMDSEntity_BiQuad_Triangle:
  case SMDSEntity_BiQuad_Quadrangle:
    return face->NbNodes() - 1;
  // case SMDSEntity_Triangle:
  // case SMDSEntity_Quad_Triangle:
  // case SMDSEntity_Quadrangle:
  // case SMDSEntity_Quad_Quadrangle:
  // case SMDSEntity_Polygon:
  // case SMDSEntity_Quad_Polygon:
  default:
    return face->NbNodes() - 2;
  }
  return 0;
}

//================================================================================
/*!
 * \brief Decompose a mesh face into triangles
 *  \retval int - number of triangles
 */
//================================================================================

static int getTriangles( const SMDS_MeshElement*             face,
                         std::vector< const SMDS_MeshNode*>& nodes)
{
  // WARNING: decomposing into triangles must be coherent with getNbTriangles()
  int nbTria, i = 0, nbNodes = face->NbNodes();
  SMDS_NodeIteratorPtr nIt = face->interlacedNodesIterator();
  nodes.resize( nbNodes * 3 );
  nodes[ i++ ] = nIt->next();
  nodes[ i++ ] = nIt->next();

  const SMDSAbs_EntityType type = face->GetEntityType();
  switch ( type )
  {
  case SMDSEntity_BiQuad_Triangle:
  case SMDSEntity_BiQuad_Quadrangle:
    nbTria = ( type == SMDSEntity_BiQuad_Triangle ) ? 6 : 8;
    nodes[ i++ ] = face->GetNode( nbTria );
    for ( i = 3; i < 3*(nbTria-1); i += 3 )
    {
      nodes[ i+0 ] = nodes[ i-2 ];
      nodes[ i+1 ] = nIt->next();
      nodes[ i+2 ] = nodes[ 2 ];
    }
    nodes[ i+0 ] = nodes[ i-2 ];
    nodes[ i+1 ] = nodes[ 0 ];
    nodes[ i+2 ] = nodes[ 2 ];
    break;
  case SMDSEntity_Triangle:
    nbTria = 1;
    nodes[ i++ ] = nIt->next();
    break;
  default:
    // case SMDSEntity_Quad_Triangle:
    // case SMDSEntity_Quadrangle:
    // case SMDSEntity_Quad_Quadrangle:
    // case SMDSEntity_Polygon:
    // case SMDSEntity_Quad_Polygon:
    nbTria = nbNodes - 2;
    while ( nIt->more() )
      nodes[ i++ ] = nIt->next();

    if ( !triangulate( nodes, nbNodes ))
    {
      nIt = face->interlacedNodesIterator();
      nodes[ 0 ] = nIt->next();
      nodes[ 1 ] = nIt->next();
      nodes[ 2 ] = nIt->next();
      for ( i = 3; i < 3*nbTria; i += 3 )
      {
        nodes[ i+0 ] = nodes[ 0 ];
        nodes[ i+1 ] = nodes[ i-1 ];
        nodes[ i+2 ] = nIt->next();
      }
    }
    break;
  }
  return nbTria;
}

// private methods

Driver_Mesh::Status DriverSTL_W_SMDS_Mesh::writeAscii() const
{
  Status aResult = DRS_OK;
  if ( myFile.empty() ) {
    fprintf(stderr, ">> ERREOR : invalid file name \n");
    return DRS_FAIL;
  }

  SMESH_File aFile( myFile, /*openForReading=*/false );
  aFile.openForWriting();

  std::string buf("solid\n");
  aFile.writeRaw( buf.c_str(), buf.size() );

  char sval[128];
  std::vector< const SMDS_MeshNode* > triaNodes;

  SMDS_ElemIteratorPtr itFaces = getFaces();
  while ( itFaces->more() )
  {
    const SMDS_MeshElement* aFace = itFaces->next();
    int nbTria = getTriangles( aFace, triaNodes );

    for ( int iT = 0, iN = 0; iT < nbTria; ++iT )
    {
      gp_XYZ normale = getNormale( triaNodes[iN],
                                   triaNodes[iN+1],
                                   triaNodes[iN+2] );
      sprintf (sval,
               " facet normal % 12e % 12e % 12e\n"
               "   outer loop\n" ,
               normale.X(), normale.Y(), normale.Z());
      aFile.writeRaw ( sval, 70 + strlen( sval + 70 )); // at least 70 but can be more (WIN)

      for ( int jN = 0; jN < 3; ++jN, ++iN )
      {
        SMESH_TNodeXYZ node = triaNodes[iN];
        sprintf (sval,
                 "     vertex % 12e % 12e % 12e\n",
                 node.X(), node.Y(), node.Z() );
        aFile.writeRaw ( sval, 54 + strlen( sval + 54 ));
      }
      aFile.writeRaw ("   endloop\n"
                      " endfacet\n", 21 );
    }
  }
  aFile.writeRaw ("endsolid\n" , 9 );

  return aResult;
}

//================================================================================
/*!
 * \brief Writes all triangles in binary format
 *  \return Driver_Mesh::Status - DRS_FAIL if no file name is provided
 */
//================================================================================

Driver_Mesh::Status DriverSTL_W_SMDS_Mesh::writeBinary() const
{
  Status aResult = DRS_OK;

  if ( myFile.empty() ) {
    fprintf(stderr, ">> ERREOR : invalid filename \n");
    return DRS_FAIL;
  }

  SMESH_File aFile( myFile );
  aFile.openForWriting();

  // we first count the number of triangles
  int nbTri = myNbVolumeTrias;
  {
    SMDS_FaceIteratorPtr itFaces = myMesh->facesIterator();
    while ( itFaces->more() ) {
      const SMDS_MeshElement* aFace = itFaces->next();
      nbTri += getNbTriangles( aFace );
    }
  }
  std::string sval( LABEL_SIZE, ' ' );
  aFile.write( sval.c_str(), LABEL_SIZE );

  // write number of triangles
  writeInteger( nbTri, aFile );  

  // Loop writing nodes

  int dum=0;

  std::vector< const SMDS_MeshNode* > triaNodes;

  SMDS_ElemIteratorPtr itFaces = getFaces();
  while ( itFaces->more() )
  {
    const SMDS_MeshElement* aFace = itFaces->next();
    int nbTria = getTriangles( aFace, triaNodes );
    
    for ( int iT = 0, iN = 0; iT < nbTria; ++iT )
    {
      gp_XYZ normale = getNormale( triaNodes[iN],
                                   triaNodes[iN+1],
                                   triaNodes[iN+2] );
      writeFloat(normale.X(),aFile);
      writeFloat(normale.Y(),aFile);
      writeFloat(normale.Z(),aFile);

      for ( int jN = 0; jN < 3; ++jN, ++iN )
      {
        const SMDS_MeshNode* node = triaNodes[iN];
        writeFloat(node->X(),aFile);
        writeFloat(node->Y(),aFile);
        writeFloat(node->Z(),aFile);
      }
      aFile.writeRaw ( &dum, 2 );
    }
  }

  return aResult;
}

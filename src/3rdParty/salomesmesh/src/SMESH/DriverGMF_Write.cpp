// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
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
// File      : DriverGMF_Write.cxx
// Created   : Mon Sep 17 17:03:02 2012
// Author    : Edward AGAPOV (eap)

#include "DriverGMF_Write.hxx"
#include "DriverGMF.hxx"

#include "SMESHDS_GroupBase.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Comment.hxx"

#include <Basics_Utils.hxx>

#include "utilities.h"

extern "C"
{
#include "libmesh5.h"
}

#include <vector>

#define BEGIN_ELEM_WRITE( SMDSEntity, GmfKwd, elem )                    \
  elemIt = elementIterator( SMDSEntity );                               \
  if ( elemIt->more() )                                                 \
  {                                                                     \
  GmfSetKwd(meshID, GmfKwd, myMesh->GetMeshInfo().NbElements( SMDSEntity )); \
  for ( int gmfID = 1; elemIt->more(); ++gmfID )                        \
  {                                                                     \
  const SMDS_MeshElement* elem = elemIt->next();                        \
  GmfSetLin(meshID, GmfKwd,

#define BEGIN_EXTRA_VERTICES_WRITE( SMDSGeom, LinType, GmfKwd, elem )   \
  elemIt = elementIterator( SMDSGeom );                                 \
  if ( elemIt->more() )                                                 \
  {                                                                     \
  int totalNbElems  = myMesh->GetMeshInfo().NbElements( SMDSGeom );     \
  int nbLinearElems = myMesh->GetMeshInfo().NbElements( LinType );      \
  if ( totalNbElems - nbLinearElems > 0 )                               \
  {                                                                     \
  GmfSetKwd(meshID, GmfKwd, totalNbElems - nbLinearElems);              \
  for ( int gmfID = 1; elemIt->more(); ++gmfID )                        \
  {                                                                     \
  const SMDS_MeshElement* elem = elemIt->next();                        \
  if ( elem->IsQuadratic() ) {                                          \
  GmfSetLin(meshID, GmfKwd, gmfID, elem->NbNodes() - elem->NbCornerNodes(),

#define END_ELEM_WRITE( elem )                  \
  elem->getshapeId() );                         \
  }}

#define END_ELEM_WRITE_ADD_TO_MAP( elem, e2id )            \
  elem->getshapeId() );                                    \
  e2id.insert( e2id.end(), std::make_pair( elem, gmfID )); \
  }}

#define END_EXTRA_VERTICES_WRITE()           \
  );                                         \
  }}}}
  

Control_Pnt::Control_Pnt(): gp_Pnt()
{
  size=0;
}
Control_Pnt::Control_Pnt( const gp_Pnt& aPnt, 
                          double theSize): gp_Pnt( aPnt )
{
  size=theSize;
}
Control_Pnt::Control_Pnt(double theX, 
                         double theY, 
                         double theZ): gp_Pnt(theX, theY, theZ)
{
  size=0;
}
Control_Pnt::Control_Pnt(double theX, 
                         double theY, 
                         double theZ, 
                         double theSize): gp_Pnt(theX, theY, theZ)
{
  size=theSize;
}

DriverGMF_Write::DriverGMF_Write():
  Driver_SMESHDS_Mesh(), _exportRequiredGroups( true )
{
}
DriverGMF_Write::~DriverGMF_Write()
{
}

//================================================================================
/*!
 * \brief Reads a GMF file
 */
//================================================================================

Driver_Mesh::Status DriverGMF_Write::Perform()
{
  Kernel_Utils::Localizer loc;

  const int dim = 3, version = sizeof(double) < 8 ? 1 : 2;

  int meshID = GmfOpenMesh( myFile.c_str(), GmfWrite, version, dim );
  if ( !meshID )
  {
    if ( DriverGMF::isExtensionCorrect( myFile ))
      return addMessage( SMESH_Comment("Can't open for writing ") << myFile, /*fatal=*/true );
    else
      return addMessage( SMESH_Comment("Not '.mesh' or '.meshb' extension of file ") << myFile, /*fatal=*/true );
  }

  DriverGMF::MeshCloser aMeshCloser( meshID ); // An object closing GMF mesh at destruction

  // nodes
  std::map< const SMDS_MeshNode* , int > node2IdMap;
  int iN = 0, nbNodes = myMesh->NbNodes();
  GmfSetKwd( meshID, GmfVertices, nbNodes );
  double xyz[3];
  SMDS_NodeIteratorPtr nodeIt = myMesh->nodesIterator();
  while ( nodeIt->more() )
  {
    const SMDS_MeshNode* n = nodeIt->next();
    n->GetXYZ( xyz );
    GmfSetLin( meshID, GmfVertices, xyz[0], xyz[1], xyz[2], n->getshapeId() );
    node2IdMap.insert( node2IdMap.end(), std::make_pair( n, ++iN ));
  }
  if ( iN != nbNodes )
    return addMessage("Wrong nb of nodes returned by nodesIterator", /*fatal=*/true);


  SMDS_ElemIteratorPtr elemIt;
  typedef std::map< const SMDS_MeshElement*, size_t, TIDCompare > TElem2IDMap;

  // edges
  TElem2IDMap edge2IDMap;
  BEGIN_ELEM_WRITE( SMDSGeom_EDGE, GmfEdges, edge )
    node2IdMap[ edge->GetNode( 0 )],
    node2IdMap[ edge->GetNode( 1 )],
    END_ELEM_WRITE_ADD_TO_MAP( edge, edge2IDMap );

  // nodes of quadratic edges
  BEGIN_EXTRA_VERTICES_WRITE( SMDSGeom_EDGE, SMDSEntity_Edge,
                              GmfExtraVerticesAtEdges, edge )
    node2IdMap[ edge->GetNode( 2 )]
    END_EXTRA_VERTICES_WRITE();

  // triangles
  TElem2IDMap tria2IDMap;
  BEGIN_ELEM_WRITE( SMDSGeom_TRIANGLE, GmfTriangles, tria )
    node2IdMap[ tria->GetNode( 0 )],
    node2IdMap[ tria->GetNode( 1 )],
    node2IdMap[ tria->GetNode( 2 )],
    END_ELEM_WRITE_ADD_TO_MAP( tria, tria2IDMap );

  // nodes of quadratic triangles
  BEGIN_EXTRA_VERTICES_WRITE( SMDSGeom_TRIANGLE, SMDSEntity_Triangle,
                              GmfExtraVerticesAtTriangles, tria )
    node2IdMap[ tria->GetNode( 3 )],
    node2IdMap[ tria->GetNode( 4 )],
    node2IdMap[ tria->GetNode( 5 )],
    node2IdMap[ tria->GetNodeWrap( 6 )] // for TRIA7
    END_EXTRA_VERTICES_WRITE();

  // quadrangles
  TElem2IDMap quad2IDMap;
  BEGIN_ELEM_WRITE( SMDSGeom_QUADRANGLE, GmfQuadrilaterals, quad )
    node2IdMap[ quad->GetNode( 0 )],
    node2IdMap[ quad->GetNode( 1 )],
    node2IdMap[ quad->GetNode( 2 )],
    node2IdMap[ quad->GetNode( 3 )],
    END_ELEM_WRITE_ADD_TO_MAP( quad, quad2IDMap );

  // nodes of quadratic quadrangles
  BEGIN_EXTRA_VERTICES_WRITE( SMDSGeom_QUADRANGLE, SMDSEntity_Quadrangle,
                              GmfExtraVerticesAtQuadrilaterals, quad )
    node2IdMap[ quad->GetNode( 4 )],
    node2IdMap[ quad->GetNode( 5 )],
    node2IdMap[ quad->GetNode( 6 )],
    node2IdMap[ quad->GetNode( 7 )],
    node2IdMap[ quad->GetNodeWrap( 8 )] // for QUAD9
    END_EXTRA_VERTICES_WRITE();

  // terahedra
  BEGIN_ELEM_WRITE( SMDSGeom_TETRA, GmfTetrahedra, tetra )
    node2IdMap[ tetra->GetNode( 0 )],
    node2IdMap[ tetra->GetNode( 2 )],
    node2IdMap[ tetra->GetNode( 1 )],
    node2IdMap[ tetra->GetNode( 3 )],
    END_ELEM_WRITE( tetra );

  // nodes of quadratic terahedra
  BEGIN_EXTRA_VERTICES_WRITE( SMDSGeom_TETRA, SMDSEntity_Tetra,
                              GmfExtraVerticesAtTetrahedra, tetra )
    node2IdMap[ tetra->GetNode( 6 )],
    node2IdMap[ tetra->GetNode( 5 )],
    node2IdMap[ tetra->GetNode( 4 )],
    node2IdMap[ tetra->GetNode( 7 )],
    node2IdMap[ tetra->GetNode( 9 )],
    node2IdMap[ tetra->GetNode( 8 )]
    //node2IdMap[ tetra->GetNodeWrap( 10 )], // for TETRA11
    END_EXTRA_VERTICES_WRITE();

  // pyramids
  BEGIN_ELEM_WRITE( SMDSEntity_Pyramid, GmfPyramids, pyra )
    node2IdMap[ pyra->GetNode( 0 )],
    node2IdMap[ pyra->GetNode( 2 )],
    node2IdMap[ pyra->GetNode( 1 )],
    node2IdMap[ pyra->GetNode( 3 )],
    node2IdMap[ pyra->GetNode( 4 )],
    END_ELEM_WRITE( pyra );

  // hexahedra
  BEGIN_ELEM_WRITE( SMDSGeom_HEXA, GmfHexahedra, hexa )
    node2IdMap[ hexa->GetNode( 0 )],
    node2IdMap[ hexa->GetNode( 3 )],
    node2IdMap[ hexa->GetNode( 2 )],
    node2IdMap[ hexa->GetNode( 1 )],
    node2IdMap[ hexa->GetNode( 4 )],
    node2IdMap[ hexa->GetNode( 7 )],
    node2IdMap[ hexa->GetNode( 6 )],
    node2IdMap[ hexa->GetNode( 5 )],
    END_ELEM_WRITE( hexa );

  // nodes of quadratic hexahedra
  BEGIN_EXTRA_VERTICES_WRITE( SMDSGeom_HEXA, SMDSEntity_Hexa,
                              GmfExtraVerticesAtHexahedra, hexa )
    node2IdMap[ hexa->GetNode( 11 )], // HEXA20
    node2IdMap[ hexa->GetNode( 10 )],
    node2IdMap[ hexa->GetNode(  9 )],
    node2IdMap[ hexa->GetNode(  8 )],
    node2IdMap[ hexa->GetNode( 15 )],
    node2IdMap[ hexa->GetNode( 14 )],
    node2IdMap[ hexa->GetNode( 13 )],
    node2IdMap[ hexa->GetNode( 12 )],
    node2IdMap[ hexa->GetNode( 16 )],
    node2IdMap[ hexa->GetNode( 19 )],
    node2IdMap[ hexa->GetNodeWrap( 18 )], // + HEXA27
    node2IdMap[ hexa->GetNodeWrap( 17 )],
    node2IdMap[ hexa->GetNodeWrap( 20 )],
    node2IdMap[ hexa->GetNodeWrap( 24 )],
    node2IdMap[ hexa->GetNodeWrap( 23 )],
    node2IdMap[ hexa->GetNodeWrap( 22 )],
    node2IdMap[ hexa->GetNodeWrap( 21 )],
    node2IdMap[ hexa->GetNodeWrap( 25 )],
    node2IdMap[ hexa->GetNodeWrap( 26 )]
    END_EXTRA_VERTICES_WRITE();

  // prism
  BEGIN_ELEM_WRITE( SMDSEntity_Penta, GmfPrisms, prism )
    node2IdMap[ prism->GetNode( 0 )],
    node2IdMap[ prism->GetNode( 2 )],
    node2IdMap[ prism->GetNode( 1 )],
    node2IdMap[ prism->GetNode( 3 )],
    node2IdMap[ prism->GetNode( 5 )],
    node2IdMap[ prism->GetNode( 4 )],
    END_ELEM_WRITE( prism );


  if ( _exportRequiredGroups )
  {
    // required entities
    SMESH_Comment badGroups;
    const std::set<SMESHDS_GroupBase*>&      groupSet = myMesh->GetGroups();
    std::set<SMESHDS_GroupBase*>::const_iterator grIt = groupSet.begin();
    for ( ; grIt != groupSet.end(); ++grIt )
    {
      const SMESHDS_GroupBase* group = *grIt;
      std::string          groupName = group->GetStoreName();
      std::string::size_type     pos = groupName.find( "_required_" );
      if ( pos == std::string::npos ) continue;

      int                    gmfKwd;
      SMDSAbs_EntityType smdsEntity;
      std::string entity = groupName.substr( pos + strlen("_required_"));
      if      ( entity == "Vertices" ) {
        gmfKwd     = GmfRequiredVertices;
        smdsEntity = SMDSEntity_Node;
      }
      else if ( entity == "Edges" ) {
        gmfKwd     = GmfRequiredEdges;
        smdsEntity = SMDSEntity_Edge;
      }
      else if ( entity == "Triangles" ) {
        gmfKwd     = GmfRequiredTriangles;
        smdsEntity = SMDSEntity_Triangle;
      }
      else if ( entity == "Quadrilaterals" ) {
        gmfKwd     = GmfRequiredQuadrilaterals;
        smdsEntity = SMDSEntity_Quadrangle;
      }
      else {
        addMessage( SMESH_Comment("Invalig gmf entity name: ") << entity, /*fatal=*/false );
        continue;
      }

      // check elem type in the group
      int nbOkElems = 0;
      SMDS_ElemIteratorPtr elemIt = group->GetElements();
      while ( elemIt->more() )
        nbOkElems += ( elemIt->next()->GetEntityType() == smdsEntity );

      if ( nbOkElems != group->Extent() && nbOkElems == 0 )
      {
        badGroups << " " << groupName;
        continue;
      }

      // choose a TElem2IDMap
      TElem2IDMap* elem2IDMap = 0;
      if ( smdsEntity == SMDSEntity_Quadrangle    && nbOkElems != myMesh->NbFaces() )
        elem2IDMap = & quad2IDMap;
      else if ( smdsEntity == SMDSEntity_Triangle && nbOkElems != myMesh->NbFaces() )
        elem2IDMap = & tria2IDMap;
      else if ( smdsEntity == SMDSEntity_Edge     && nbOkElems != myMesh->NbEdges() )
        elem2IDMap = & edge2IDMap;

      // write the group
      GmfSetKwd( meshID, gmfKwd, nbOkElems );
      elemIt = group->GetElements();
      if ( elem2IDMap )
        for ( ; elemIt->more(); )
        {
          const SMDS_MeshElement* elem = elemIt->next();
          if ( elem->GetEntityType() == smdsEntity )
            GmfSetLin( meshID, gmfKwd, (*elem2IDMap)[ elem ] );
        }
      else
        for ( int gmfID = 1; elemIt->more(); ++gmfID)
        {
          const SMDS_MeshElement* elem = elemIt->next();
          if ( elem->GetEntityType() == smdsEntity )
            GmfSetLin( meshID, gmfKwd, gmfID );
        }

    } // loop on groups

    if ( !badGroups.empty() )
      addMessage( SMESH_Comment("Groups of elements of inappropriate geometry:")
                  << badGroups, /*fatal=*/false );
  }

  return DRS_OK;
}

Driver_Mesh::Status DriverGMF_Write::PerformSizeMap( const std::vector<Control_Pnt>& points )
{
//   const int dim = 3, version = sizeof(long) == 4 ? 2 : 3;
  const int dim = 3, version = 2; // Version 3 not supported by mg-hexa
  
  // Open files
  int verticesFileID = GmfOpenMesh( myVerticesFile.c_str(), GmfWrite, version, dim );  
  int solFileID = GmfOpenMesh( mySolFile.c_str(), GmfWrite, version, dim );
  
  int pointsNumber = points.size();
  
  // Vertices Keyword
  GmfSetKwd( verticesFileID, GmfVertices, pointsNumber );
  // SolAtVertices Keyword
  int TypTab[] = {GmfSca};
  GmfSetKwd(solFileID, GmfSolAtVertices, pointsNumber, 1, TypTab);
  
  // Read the control points information from the vector and write it into the files
  std::vector<Control_Pnt>::const_iterator points_it;
  for (points_it = points.begin(); points_it != points.end(); points_it++ )
  {
    GmfSetLin( verticesFileID, GmfVertices, points_it->X(), points_it->Y(), points_it->Z(), 0 );
    double ValTab[] = {points_it->Size()};
    GmfSetLin( solFileID, GmfSolAtVertices, ValTab);
  } 
  
  // Close Files
  GmfCloseMesh( verticesFileID );
  GmfCloseMesh( solFileID );

  return DRS_OK;
}

std::vector<std::string> DriverGMF_Write::GetSizeMapFiles()
{
  std::vector<std::string> files;
  files.push_back(myVerticesFile);
  files.push_back(mySolFile);
  return files;
}

//================================================================================
/*!
 * \brief Returns an iterator on elements of a certain type
 */
//================================================================================

SMDS_ElemIteratorPtr DriverGMF_Write::elementIterator(SMDSAbs_ElementType type)
{
  return myMesh->elementsIterator(type);
}
SMDS_ElemIteratorPtr DriverGMF_Write::elementIterator(SMDSAbs_EntityType type) 
{
  return myMesh->elementEntityIterator(type);
}
SMDS_ElemIteratorPtr DriverGMF_Write::elementIterator(SMDSAbs_GeometryType type)
{
  return myMesh->elementGeomIterator(type);
}

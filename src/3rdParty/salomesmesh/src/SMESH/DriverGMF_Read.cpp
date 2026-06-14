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
// File      : DriverGMF_Read.cxx
// Created   : Mon Sep 17 17:03:02 2012
// Author    : Edward AGAPOV (eap)

#include "DriverGMF_Read.hxx"
#include "DriverGMF.hxx"

#include "SMESHDS_Group.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_TypeDefs.hxx"

#include <Basics_Utils.hxx>

extern "C"
{
#include "libmesh5.h"
}

#include <stdarg.h>

// --------------------------------------------------------------------------------
DriverGMF_Read::DriverGMF_Read():
  Driver_SMESHDS_Mesh(),
  _makeRequiredGroups( true )
{
}
// --------------------------------------------------------------------------------
DriverGMF_Read::~DriverGMF_Read()
{
}

//================================================================================
/*!
 * \brief Read a GMF file
 */
//================================================================================

Driver_Mesh::Status DriverGMF_Read::Perform()
{
  Kernel_Utils::Localizer loc;

  Status status = DRS_OK;

  int dim, version;

  // open the file
  int meshID = GmfOpenMesh( myFile.c_str(), GmfRead, &version, &dim );
  if ( !meshID )
  {
    if ( DriverGMF::isExtensionCorrect( myFile ))
      return addMessage( SMESH_Comment("Can't open for reading ") << myFile,
                         /*fatal=*/true );
    else
      return addMessage( SMESH_Comment("Not '.mesh' or '.meshb' extension of file ") << myFile,
                         /*fatal=*/true );
  }
  DriverGMF::MeshCloser aMeshCloser( meshID ); // An object closing GMF mesh at destruction

  // Read nodes

  int nbNodes = GmfStatKwd(meshID, GmfVertices);
  if ( nbNodes < 1 )
    return addMessage( "No nodes in the mesh", /*fatal=*/true );

  GmfGotoKwd(meshID, GmfVertices);

  int ref;

  const int nodeIDShift = myMesh->GetMeshInfo().NbNodes();
  if ( version != GmfFloat )
  {
    double x, y, z;
    for ( int i = 1; i <= nbNodes; ++i )
    {
      GmfGetLin(meshID, GmfVertices, &x, &y, &z, &ref);
      myMesh->AddNodeWithID( x,y,z, nodeIDShift + i);
    }
  }
  else
  {
    float x, y, z;
    for ( int i = 1; i <= nbNodes; ++i )
    {
      GmfGetLin(meshID, GmfVertices, &x, &y, &z, &ref);
      myMesh->AddNodeWithID( x,y,z, nodeIDShift + i);
    }
  }

  // Read elements

  int iN[28]; // 28 - nb nodes in HEX27 (+ 1 for safety :)

  /* Read edges */
  const int edgeIDShift = myMesh->GetMeshInfo().NbElements();
  if ( int nbEdges = GmfStatKwd(meshID, GmfEdges))
  {
    // read extra vertices for quadratic edges
    std::vector<int> quadNodesAtEdges( nbEdges + 1, -1 );
    if ( int nbQuadEdges = GmfStatKwd(meshID, GmfExtraVerticesAtEdges))
    {
      GmfGotoKwd(meshID, GmfExtraVerticesAtEdges);
      for ( int i = 1; i <= nbQuadEdges; ++i )
      {
        GmfGetLin(meshID, GmfExtraVerticesAtEdges, &iN[0], &iN[1], &iN[2]);
        if ( iN[1] >= 1 )
          quadNodesAtEdges[ iN[0] ] = iN[2];
      }
    }
    // create edges
    GmfGotoKwd(meshID, GmfEdges);
    for ( int i = 1; i <= nbEdges; ++i )
    {
      GmfGetLin(meshID, GmfEdges, &iN[0], &iN[1], &ref);
      const int midN = quadNodesAtEdges[ i ];
      if ( midN > 0 )
      {
        if ( !myMesh->AddEdgeWithID( iN[0], iN[1], midN, edgeIDShift + i ))
          status = storeBadNodeIds( "GmfEdges + GmfExtraVerticesAtEdges",i,
                                    3, iN[0], iN[1], midN);
      }
      else
      {
        if ( !myMesh->AddEdgeWithID( iN[0], iN[1], edgeIDShift + i ))
          status = storeBadNodeIds( "GmfEdges",i, 2, iN[0], iN[1] );
      }
    }
  }

  /* Read triangles */
  const int triaIDShift = myMesh->GetMeshInfo().NbElements();
  if ( int nbTria = GmfStatKwd(meshID, GmfTriangles))
  {
    // read extra vertices for quadratic triangles
    std::vector< std::vector<int> > quadNodesAtTriangles( nbTria + 1 );
    if ( int nbQuadTria = GmfStatKwd(meshID, GmfExtraVerticesAtTriangles ))
    {
      GmfGotoKwd( meshID, GmfExtraVerticesAtTriangles );
      for ( int i = 1; i <= nbQuadTria; ++i )
      {
        GmfGetLin(meshID, GmfExtraVerticesAtTriangles,
                  &iN[0], &iN[1], &iN[2], &iN[3], &iN[4],
                  &iN[5]); // iN[5] - preview TRIA7
        if ( iN[0] <= nbTria )
        {
          std::vector<int>& nodes = quadNodesAtTriangles[ iN[0] ];
          nodes.insert( nodes.end(), & iN[2], & iN[5+1] );
          nodes.resize( iN[1] );
        }
      }
    }
    // create triangles
    GmfGotoKwd(meshID, GmfTriangles);
    for ( int i = 1; i <= nbTria; ++i )
    {
      GmfGetLin(meshID, GmfTriangles, &iN[0], &iN[1], &iN[2], &ref);
      std::vector<int>& midN = quadNodesAtTriangles[ i ];
      if ( midN.size() >= 3 )
      {
        if ( !myMesh->AddFaceWithID( iN[0],iN[1],iN[2], midN[0],midN[1],midN[2],
                                     triaIDShift + i ))
          status = storeBadNodeIds( "GmfTriangles + GmfExtraVerticesAtTriangles",i, 6,
                                    iN[0],iN[1],iN[2], midN[0],midN[1],midN[2] );
      }
      else
      {
        if ( !myMesh->AddFaceWithID( iN[0], iN[1], iN[2], triaIDShift + i ))
          status = storeBadNodeIds( "GmfTriangles",i, 3, iN[0], iN[1], iN[2] );
      }
      if ( !midN.empty() ) SMESHUtils::FreeVector( midN );
    }
  }

  /* Read quadrangles */
  const int quadIDShift = myMesh->GetMeshInfo().NbElements();
  if ( int nbQuad = GmfStatKwd(meshID, GmfQuadrilaterals))
  {
    // read extra vertices for quadratic quadrangles
    std::vector< std::vector<int> > quadNodesAtQuadrilaterals( nbQuad + 1 );
    if ( int nbQuadQuad = GmfStatKwd( meshID, GmfExtraVerticesAtQuadrilaterals ))
    {
      GmfGotoKwd(meshID, GmfExtraVerticesAtQuadrilaterals);
      for ( int i = 1; i <= nbQuadQuad; ++i )
      {
        GmfGetLin(meshID, GmfExtraVerticesAtQuadrilaterals,
                  &iN[0], &iN[1], &iN[2], &iN[3], &iN[4], &iN[5], &iN[6]);
        if ( iN[0] <= nbQuad )
        {
          std::vector<int>& nodes = quadNodesAtQuadrilaterals[ iN[0] ];
          nodes.insert( nodes.end(), & iN[2], & iN[6+1] );
          nodes.resize( iN[1] );
        }
      }
    }
    // create quadrangles
    GmfGotoKwd(meshID, GmfQuadrilaterals);
    for ( int i = 1; i <= nbQuad; ++i )
    {
      GmfGetLin(meshID, GmfQuadrilaterals, &iN[0], &iN[1], &iN[2], &iN[3], &ref);
      std::vector<int>& midN = quadNodesAtQuadrilaterals[ i ];
      if ( midN.size() == 8-4 ) // QUAD8
      {
        if ( !myMesh->AddFaceWithID( iN[0], iN[1], iN[2], iN[3],
                                     midN[0], midN[1], midN[2], midN[3],
                                     quadIDShift + i ))
          status = storeBadNodeIds( "GmfQuadrilaterals + GmfExtraVerticesAtQuadrilaterals",i, 8,
                                    iN[0], iN[1],iN[2], iN[3],
                                    midN[0], midN[1], midN[2], midN[3]);
      }
      else if ( midN.size() > 8-4 ) // QUAD9
      {
        if ( !myMesh->AddFaceWithID( iN[0], iN[1], iN[2], iN[3],
                                     midN[0], midN[1], midN[2], midN[3], midN[4],
                                     quadIDShift + i ))
          status = storeBadNodeIds( "GmfQuadrilaterals + GmfExtraVerticesAtQuadrilaterals",i, 9,
                                    iN[0], iN[1],iN[2], iN[3],
                                    midN[0], midN[1], midN[2], midN[3], midN[4]);
      }
      else // QUAD4
      {
        if ( !myMesh->AddFaceWithID( iN[0], iN[1], iN[2], iN[3], quadIDShift + i ))
          status = storeBadNodeIds( "GmfQuadrilaterals",i, 4, iN[0], iN[1],iN[2], iN[3] );
      }
      if ( !midN.empty() ) SMESHUtils::FreeVector( midN );
    }
  }

  /* Read terahedra */
  const int tetIDShift = myMesh->GetMeshInfo().NbElements();
  if ( int nbTet = GmfStatKwd( meshID, GmfTetrahedra ))
  {
    // read extra vertices for quadratic tetrahedra
    std::vector< std::vector<int> > quadNodesAtTetrahedra( nbTet + 1 );
    if ( int nbQuadTetra = GmfStatKwd( meshID, GmfExtraVerticesAtTetrahedra ))
    {
      GmfGotoKwd(meshID, GmfExtraVerticesAtTetrahedra);
      for ( int i = 1; i <= nbQuadTetra; ++i )
      {
        GmfGetLin(meshID, GmfExtraVerticesAtTetrahedra,
                  &iN[0], &iN[1], &iN[2], &iN[3], &iN[4], &iN[5], &iN[6], &iN[7]);
        if ( iN[0] <= nbTet )
        {
          std::vector<int>& nodes = quadNodesAtTetrahedra[ iN[0] ];
          nodes.insert( nodes.end(), & iN[2], & iN[7+1] );
          nodes.resize( iN[1] );
        }
      }
    }
    // create tetrahedra
    GmfGotoKwd(meshID, GmfTetrahedra);
    for ( int i = 1; i <= nbTet; ++i )
    {
      GmfGetLin(meshID, GmfTetrahedra, &iN[0], &iN[1], &iN[2], &iN[3], &ref);
      std::vector<int>& midN = quadNodesAtTetrahedra[ i ];  
      if ( midN.size() >= 10-4 ) // TETRA10
      {
        if ( !myMesh->AddVolumeWithID( iN[0], iN[2], iN[1], iN[3], 
                                       midN[2], midN[1], midN[0], midN[3], midN[5], midN[4],
                                       tetIDShift + i ))
          status = storeBadNodeIds( "GmfTetrahedra + GmfExtraVerticesAtTetrahedra",i, 10,
                                    iN[0], iN[2], iN[1], iN[3], 
                                    midN[2], midN[1], midN[0], midN[3], midN[5], midN[4] );
      }
      else // TETRA4
      {
        if ( !myMesh->AddVolumeWithID( iN[0], iN[2], iN[1], iN[3], tetIDShift + i ))
          status = storeBadNodeIds( "GmfTetrahedra" ,i, 4, iN[0], iN[2], iN[1], iN[3] );
      }
      if ( !midN.empty() ) SMESHUtils::FreeVector( midN );
    }
  }

  /* Read pyramids */
  const int pyrIDShift = myMesh->GetMeshInfo().NbElements();
  if ( int nbPyr = GmfStatKwd(meshID, GmfPyramids))
  {
    GmfGotoKwd(meshID, GmfPyramids);
    for ( int i = 1; i <= nbPyr; ++i )
    {
      GmfGetLin(meshID, GmfPyramids, &iN[0], &iN[1], &iN[2], &iN[3], &iN[4], &ref);
      if ( !myMesh->AddVolumeWithID( iN[0], iN[2], iN[1], iN[3], iN[4], pyrIDShift + i ))
        status = storeBadNodeIds( "GmfPyramids",i, 5, iN[0], iN[1],iN[2], iN[3], iN[4] );
    }
  }

  /* Read hexahedra */
  const int hexIDShift = myMesh->GetMeshInfo().NbElements();
  if ( int nbHex = GmfStatKwd(meshID, GmfHexahedra))
  {
    // read extra vertices for quadratic hexahedra
    std::vector< std::vector<int> > quadNodesAtHexahedra( nbHex + 1 );
    if ( int nbQuadHexa = GmfStatKwd( meshID, GmfExtraVerticesAtHexahedra ))
    {
      GmfGotoKwd(meshID, GmfExtraVerticesAtHexahedra);
      for ( int i = 1; i <= nbQuadHexa; ++i )
      {
        GmfGetLin(meshID, GmfExtraVerticesAtHexahedra, &iN[0], &iN[1], // Hexa Id, Nb extra vertices
                  &iN[2], &iN[3], &iN[4], &iN[5],
                  &iN[6], &iN[7], &iN[8], &iN[9],
                  &iN[10], &iN[11], &iN[12], &iN[13], // HEXA20
                  &iN[14],
                  &iN[15], &iN[16], &iN[17], &iN[18], 
                  &iN[19],
                  &iN[20]);                          // HEXA27
        if ( iN[0] <= nbHex )
        {
          std::vector<int>& nodes = quadNodesAtHexahedra[ iN[0] ];
          nodes.insert( nodes.end(), & iN[2], & iN[20+1] );
          nodes.resize( iN[1] );
        }
      }
    }
    // create hexhedra
    GmfGotoKwd(meshID, GmfHexahedra);
    for ( int i = 1; i <= nbHex; ++i )
    {
      GmfGetLin(meshID, GmfHexahedra, &iN[0], &iN[1], &iN[2], &iN[3],
                &iN[4], &iN[5], &iN[6], &iN[7], &ref);
      std::vector<int>& midN = quadNodesAtHexahedra[ i ];
      if ( midN.size() == 20-8 ) // HEXA20
      {
        if ( !myMesh->AddVolumeWithID( iN[0], iN[3], iN[2], iN[1],
                                       iN[4], iN[7], iN[6], iN[5],
                                       midN[3], midN[2], midN[1], midN[0],
                                       midN[7], midN[6], midN[5], midN[4],
                                       midN[8], midN[11], midN[10], midN[9],
                                       hexIDShift + i ))
          status = storeBadNodeIds( "GmfHexahedra + GmfExtraVerticesAtHexahedra",i, 20, 
                                    iN[0], iN[3], iN[2], iN[1],
                                    iN[4], iN[7], iN[6], iN[5],
                                    midN[3], midN[2], midN[1], midN[0],
                                    midN[7], midN[6], midN[5], midN[4],
                                    midN[8], midN[11], midN[10], midN[9]);
      }
      else if ( midN.size() >= 27-8 ) // HEXA27
      {
        if ( !myMesh->AddVolumeWithID( iN[0], iN[3], iN[2], iN[1],
                                       iN[4], iN[7], iN[6], iN[5],
                                       midN[3], midN[2], midN[1], midN[0],
                                       midN[7], midN[6], midN[5], midN[4],
                                       midN[8], midN[11], midN[10], midN[9],
                                       midN[12],
                                       midN[16], midN[15], midN[14], midN[13],
                                       midN[17],
                                       midN[18],
                                       hexIDShift + i ))
          status = storeBadNodeIds( "GmfHexahedra + GmfExtraVerticesAtHexahedra",i, 27, 
                                    iN[0], iN[3], iN[2], iN[1],
                                    iN[4], iN[7], iN[6], iN[5],
                                    midN[3], midN[2], midN[1], midN[0],
                                    midN[7], midN[6], midN[5], midN[4],
                                    midN[8], midN[11], midN[10], midN[9],
                                    midN[12],
                                    midN[16], midN[15], midN[14], midN[13],
                                    midN[17],
                                    midN[18]);
      }
      else // HEXA8
      {
        if ( !myMesh->AddVolumeWithID( iN[0], iN[3], iN[2], iN[1],
                                       iN[4], iN[7], iN[6], iN[5], hexIDShift + i ) )
          status = storeBadNodeIds( "GmfHexahedra" ,i, 8, iN[0], iN[3], iN[2], iN[1],
                                    iN[4], iN[7], iN[6], iN[5] );
      }
      if ( !midN.empty() ) SMESHUtils::FreeVector( midN );
    }
  }

  /* Read prism */
  const int prismIDShift = myMesh->GetMeshInfo().NbElements();
  if ( int nbPrism = GmfStatKwd(meshID, GmfPrisms))
  {
    GmfGotoKwd(meshID, GmfPrisms);
    for ( int i = 1; i <= nbPrism; ++i )
    {
      GmfGetLin(meshID, GmfPrisms, &iN[0], &iN[1], &iN[2], &iN[3], &iN[4], &iN[5], &ref);
      if ( !myMesh->AddVolumeWithID( iN[0], iN[2], iN[1], iN[3], iN[5], iN[4], prismIDShift + i))
        status = storeBadNodeIds( "GmfPrisms",i,
                                  6, iN[0], iN[1],iN[2], iN[3], iN[4], iN[5] );
    }
  }

  // Read required entities into groups

  if ( _makeRequiredGroups )
  {
    // get ids of existing groups
    std::set< int > groupIDs;
    const std::set<SMESHDS_GroupBase*>&          groups = myMesh->GetGroups();
    std::set<SMESHDS_GroupBase*>::const_iterator grIter = groups.begin();
    for ( ; grIter != groups.end(); ++grIter )
      groupIDs.insert( (*grIter)->GetID() );
    if ( groupIDs.empty() ) groupIDs.insert( 0 );

    const int kes[4][3] = { { GmfRequiredVertices,      SMDSAbs_Node, nodeIDShift },
                            { GmfRequiredEdges,         SMDSAbs_Edge, edgeIDShift },
                            { GmfRequiredTriangles,     SMDSAbs_Face, triaIDShift },
                            { GmfRequiredQuadrilaterals,SMDSAbs_Face, quadIDShift }};
    const char* names[4] = { "_required_Vertices"      ,
                             "_required_Edges"         ,
                             "_required_Triangles"     ,
                             "_required_Quadrilaterals" };
    for ( int i = 0; i < 4; ++i )
    {
      int                 gmfKwd = kes[i][0];
      SMDSAbs_ElementType entity = (SMDSAbs_ElementType) kes[i][1];
      int                 shift  = kes[i][2];
      if ( int nb = GmfStatKwd(meshID, gmfKwd))
      {
        const int newID = *groupIDs.rbegin() + 1;
        groupIDs.insert( newID );
        SMESHDS_Group* group = new SMESHDS_Group( newID, myMesh, entity );
        group->SetStoreName( names[i] );
        myMesh->AddGroup( group );

        GmfGotoKwd(meshID, gmfKwd);
        for ( int i = 0; i < nb; ++i )
        {
          GmfGetLin(meshID, gmfKwd, &iN[0] );
          group->Add( shift + iN[0] );
        }
      }
    }
  }

  return status;
}

//================================================================================
/*!
 * \brief Store a message about invalid IDs of nodes
 */
//================================================================================

Driver_Mesh::Status DriverGMF_Read::storeBadNodeIds(const char* gmfKwd, int elemNb, int nb, ...)
{
  if ( myStatus != DRS_OK )
    return myStatus;

  SMESH_Comment msg;

  va_list VarArg;
  va_start(VarArg, nb);

  for ( int i = 0; i < nb; ++i )
  {
    int id = va_arg(VarArg, int );
    if ( !myMesh->FindNode( id ))
      msg << " " << id;
  }
  va_end(VarArg);

  if ( !msg.empty() )
  {
    std::string nbStr;
    const char* nbNames[] = { "1-st ", "2-nd ", "3-d " };
    if ( elemNb < 3 ) nbStr = nbNames[ elemNb-1 ];
    else              nbStr = SMESH_Comment(elemNb) << "-th ";

    return addMessage
      ( SMESH_Comment("Wrong node IDs of ")<< nbStr << gmfKwd << ":" << msg,
        /*fatal=*/false );
  }
  return DRS_OK;
}

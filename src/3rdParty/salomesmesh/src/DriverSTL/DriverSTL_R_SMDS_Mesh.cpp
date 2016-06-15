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

#include "DriverSTL_R_SMDS_Mesh.h"

#include <Basics_Utils.hxx>

#include <gp_Pnt.hxx>
#include <NCollection_DataMap.hxx>
#include <Standard_NoMoreObject.hxx>

#include "SMDS_Mesh.hxx"
#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMESH_File.hxx"

namespace
{
  struct Hasher
  {
    //=======================================================================
    //function : HashCode
    //purpose  :
    //=======================================================================
    inline static Standard_Integer HashCode
    (const gp_Pnt& point,  Standard_Integer Upper)
    {
      union
      {
        Standard_Real    R[3];
        Standard_Integer I[6];
      } U;

      point.Coord( U.R[0], U.R[1], U.R[2] );

      return ::HashCode(U.I[0]/23+U.I[1]/19+U.I[2]/17+U.I[3]/13+U.I[4]/11+U.I[5]/7,Upper);
    }
    //=======================================================================
    //function : IsEqual
    //purpose  :
    //=======================================================================
    inline static Standard_Boolean IsEqual
    (const gp_Pnt& point1, const gp_Pnt& point2)
    {
      static Standard_Real tab1[3], tab2[3];
      point1.Coord(tab1[0],tab1[1],tab1[2]);
      point2.Coord(tab2[0],tab2[1],tab2[2]);
      return (memcmp(tab1,tab2,sizeof(tab1)) == 0);
    }
  };
  typedef NCollection_DataMap<gp_Pnt,SMDS_MeshNode*,Hasher> TDataMapOfPntNodePtr;

  const int HEADER_SIZE           = 84;
  const int SIZEOF_STL_FACET      = 50;
  const int ASCII_LINES_PER_FACET = 7;
  const int SIZE_OF_FLOAT         = 4;
  // const int STL_MIN_FILE_SIZE     = 284;
}

//=======================================================================
//function : DriverSTL_R_SMDS_Mesh
//purpose  :
//=======================================================================

DriverSTL_R_SMDS_Mesh::DriverSTL_R_SMDS_Mesh()
{
  myIsCreateFaces = true;
  myIsAscii = Standard_True;
}

//=======================================================================
//function : SetIsCreateFaces
//purpose  : 
//=======================================================================

void DriverSTL_R_SMDS_Mesh::SetIsCreateFaces( const bool theIsCreate )
{ myIsCreateFaces = theIsCreate; }

//=======================================================================
//function : Perform
//purpose  : 
//=======================================================================

Driver_Mesh::Status DriverSTL_R_SMDS_Mesh::Perform()
{
  // Kernel_Utils::Localizer loc;

  Status aResult = DRS_OK;

  if ( myFile.empty() ) {
    fprintf(stderr, ">> ERREOR : invalid file name \n");
    return DRS_FAIL;
  }

  SMESH_File file( myFile, /*open=*/false );
  if ( !file.open() ) {
    fprintf(stderr, ">> ERROR : cannot open file %s \n", myFile.c_str());
    if ( file.error().empty() )
      fprintf(stderr, ">> ERROR : %s \n", file.error().c_str());
    return DRS_FAIL;
  }

  // we skip the header which is in Ascii for both modes
  const char* data = file;
  data += HEADER_SIZE;

  // we check 128 characters to detect if we have a non-ascii char
  myIsAscii = Standard_True;
  for (int i = 0; i < 128; ++i, ++data) {
    if ( !isascii( *data ) && data < file.end() ) {
      myIsAscii = Standard_False;
      break;
    }
  }

  if ( !myMesh ) {
    fprintf(stderr, ">> ERREOR : cannot create mesh \n");
    return DRS_FAIL;
  }

  if ( myIsAscii )
    aResult = readAscii( file );
  else
    aResult = readBinary( file );

  return aResult;
}

// static methods

static Standard_Real readFloat(SMESH_File& theFile)
{
  union {
    Standard_Boolean i;
    Standard_ShortReal f;
  } u;

  const char* c = theFile;
  u.i  =  c[0] & 0xFF;
  u.i |= (c[1] & 0xFF) << 0x08;
  u.i |= (c[2] & 0xFF) << 0x10;
  u.i |= (c[3] & 0xFF) << 0x18;
  theFile += SIZE_OF_FLOAT;

  return u.f;
}

static SMDS_MeshNode* addNode(const gp_Pnt& P,
                              TDataMapOfPntNodePtr& uniqnodes,
                              SMDS_Mesh* theMesh)
{
  SMDS_MeshNode* node = 0;
  if ( uniqnodes.IsBound(P) ) {
    node = uniqnodes.Find(P);
  } else {
    node = theMesh->AddNode(P.X(), P.Y(), P.Z() );
    uniqnodes.Bind(P,node);
  }
  
  return node;
}                                

static SMDS_MeshNode* readNode(FILE* file,
                               TDataMapOfPntNodePtr& uniqnodes,
                               SMDS_Mesh* theMesh)
{
  Standard_ShortReal coord[3];
  // reading vertex
  fscanf(file,"%*s %f %f %f\n",&coord[0],&coord[1],&coord[2]);

  gp_Pnt P(coord[0],coord[1],coord[2]);
  return addNode( P, uniqnodes, theMesh );
}

static SMDS_MeshNode* readNode(SMESH_File& theFile,
                               TDataMapOfPntNodePtr& uniqnodes,
                               SMDS_Mesh* theMesh)
{
  gp_Pnt coord;
  coord.SetX( readFloat(theFile));
  coord.SetY( readFloat(theFile));
  coord.SetZ( readFloat(theFile));

  return addNode( coord, uniqnodes, theMesh );
}

//=======================================================================
//function : readAscii
//purpose  : 
//=======================================================================

Driver_Mesh::Status DriverSTL_R_SMDS_Mesh::readAscii(SMESH_File& theFile) const
{
  Status aResult = DRS_OK;

  // get the file size
  long filesize = theFile.size();
  theFile.close();

  // Open the file 
  FILE* file = fopen( myFile.c_str(),"r");

  // count the number of lines
  Standard_Integer nbLines = 0;
  for (long ipos = 0; ipos < filesize; ++ipos) {
    if (getc(file) == '\n')
      nbLines++;
  }

  // go back to the beginning of the file
  rewind(file);
  
  Standard_Integer nbTri = (nbLines / ASCII_LINES_PER_FACET);

  TDataMapOfPntNodePtr uniqnodes;
  // skip header
  while (getc(file) != '\n');

  // main reading
  for (Standard_Integer iTri = 0; iTri < nbTri; ++iTri) {

    // skipping the facet normal
    Standard_ShortReal normal[3];
    fscanf(file,"%*s %*s %f %f %f\n",&normal[0],&normal[1],&normal[2]);

    // skip the keywords "outer loop"
    fscanf(file,"%*s %*s");

    // reading nodes
    SMDS_MeshNode* node1 = readNode( file, uniqnodes, myMesh );
    SMDS_MeshNode* node2 = readNode( file, uniqnodes, myMesh );
    SMDS_MeshNode* node3 = readNode( file, uniqnodes, myMesh );

    if (myIsCreateFaces)
      myMesh->AddFace(node1,node2,node3);

    // skip the keywords "endloop"
    fscanf(file,"%*s");

    // skip the keywords "endfacet"
    fscanf(file,"%*s");
  }

  fclose(file);
  return aResult;
}

//=======================================================================
//function : readBinary
//purpose  : 
//=======================================================================

Driver_Mesh::Status DriverSTL_R_SMDS_Mesh::readBinary(SMESH_File& file) const
{
  Status aResult = DRS_OK;

  // the size of the file (minus the header size)
  // must be a multiple of SIZEOF_STL_FACET

  long filesize = file.size();

  if ( (filesize - HEADER_SIZE) % SIZEOF_STL_FACET !=0 
      // Commented to allow reading small files (ex: 1 face)
      /*|| (filesize < STL_MIN_FILE_SIZE)*/) {
    Standard_NoMoreObject::Raise("DriverSTL_R_SMDS_MESH::readBinary (wrong file size)");
  }

  // don't trust the number of triangles which is coded in the file
  // sometimes it is wrong, and with this technique we don't need to swap endians for integer
  Standard_Integer nbTri = ((filesize - HEADER_SIZE) / SIZEOF_STL_FACET);

  // skip the header
  file += HEADER_SIZE;

  TDataMapOfPntNodePtr uniqnodes;
  
  for (Standard_Integer iTri = 0; iTri < nbTri; ++iTri) {

    // ignore normals
    file += 3 * SIZE_OF_FLOAT;

    // read vertices
    SMDS_MeshNode* node1 = readNode( file, uniqnodes, myMesh );
    SMDS_MeshNode* node2 = readNode( file, uniqnodes, myMesh );
    SMDS_MeshNode* node3 = readNode( file, uniqnodes, myMesh );

    if (myIsCreateFaces)
      myMesh->AddFace(node1,node2,node3);

    // skip extra bytes
    file += 2;
  }
  return aResult;
}

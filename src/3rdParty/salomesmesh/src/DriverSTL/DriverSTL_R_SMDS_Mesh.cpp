//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
#include <stdio.h>
#include <gp_Pnt.hxx>
//=======================================================================
//function : HashCode
//purpose  : 
//=======================================================================
inline Standard_Integer HashCode
  (const gp_Pnt& point,  Standard_Integer Upper)
{
  union 
    {
    Standard_Real R[3];
    Standard_Integer I[6];
    } U;

  point.Coord(U.R[0],U.R[1],U.R[2]);  

  return ::HashCode(U.I[0]/23+U.I[1]/19+U.I[2]/17+U.I[3]/13+U.I[4]/11+U.I[5]/7,Upper);
}
static Standard_Real tab1[3];
static Standard_Real tab2[3];
//=======================================================================
//function : IsEqual
//purpose  : 
//=======================================================================
inline Standard_Boolean IsEqual
  (const gp_Pnt& point1, const gp_Pnt& point2)
{
  point1.Coord(tab1[0],tab1[1],tab1[2]);  
  point2.Coord(tab2[0],tab2[1],tab2[2]);  
  return (memcmp(tab1,tab2,sizeof(tab1)) == 0);
}
#include "DriverSTL_R_SMDS_Mesh.h"

#include "SMDS_Mesh.hxx"
#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"

#include <OSD_Path.hxx>
#include <OSD_File.hxx>
#include <OSD_FromWhere.hxx>
#include <OSD_Protection.hxx>
#include <OSD_SingleProtection.hxx>
#include <Standard_NoMoreObject.hxx>

#include "utilities.h"

static const int HEADER_SIZE           =  84;
static const int SIZEOF_STL_FACET      =  50;
//static const int STL_MIN_FILE_SIZE     = 284;
static const int ASCII_LINES_PER_FACET =   7;


//typedef NCollection_BaseCollection<SMDS_MeshNodePtr> DriverSTL_ColOfNodePtr;


#include <NCollection_DataMap.hxx>
typedef NCollection_DataMap<gp_Pnt,SMDS_MeshNode*> DriverSTL_DataMapOfPntNodePtr;
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
  Status aResult = DRS_OK;

  TCollection_AsciiString aFileName( (char *)myFile.c_str() );
  if ( aFileName.IsEmpty() ) {
    fprintf(stderr, ">> ERREOR : invalid file name \n");
    return DRS_FAIL;
  }

  filebuf fic;
  Standard_IStream is(&fic);
  if (!fic.open(aFileName.ToCString(),ios::in)) {
    fprintf(stderr, ">> ERROR : cannot open file %s \n", aFileName.ToCString());
    return DRS_FAIL;
  }


  OSD_Path aPath( aFileName );
  OSD_File file = OSD_File( aPath );
  file.Open(OSD_ReadOnly,OSD_Protection(OSD_RWD,OSD_RWD,OSD_RWD,OSD_RWD));
  unsigned char str[128];
  Standard_Integer lread,i;
  Standard_Address ach;
  ach = (Standard_Address)str;
  // we skip the header which is in Ascii for both modes
  file.Read(ach,HEADER_SIZE,lread);

  // we read 128 characters to detect if we have a non-ascii char
  file.Read(ach,sizeof(str),lread);
  
  myIsAscii = Standard_True;
  for (i = 0; i < lread; ++i) {
    if (str[i] > '~') {
      myIsAscii = Standard_False;
      break;
    }
  }
      
  file.Close();

  if ( !myMesh ) {
    fprintf(stderr, ">> ERREOR : cannot create mesh \n");
    return DRS_FAIL;
  }

  if ( myIsAscii )
    aResult = readAscii();
  else
    aResult = readBinary();

  return aResult;
}

// static methods

static Standard_Real readFloat(OSD_File& theFile)
{
  union {
    Standard_Boolean i; 
    Standard_ShortReal f;
  }u;

  char c[4];
  Standard_Address adr;
  adr = (Standard_Address)c;
  Standard_Integer lread;
  theFile.Read(adr,4,lread);
  u.i  =  c[0] & 0xFF;
  u.i |= (c[1] & 0xFF) << 0x08;
  u.i |= (c[2] & 0xFF) << 0x10;
  u.i |= (c[3] & 0xFF) << 0x18;

  return u.f;
}

static SMDS_MeshNode* addNode(const gp_Pnt& P,
                              DriverSTL_DataMapOfPntNodePtr& uniqnodes,
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
                               DriverSTL_DataMapOfPntNodePtr& uniqnodes,
                               SMDS_Mesh* theMesh)
{
  Standard_ShortReal coord[3];
  // reading vertex
  fscanf(file,"%*s %f %f %f\n",&coord[0],&coord[1],&coord[2]);

  gp_Pnt P(coord[0],coord[1],coord[2]);
  return addNode( P, uniqnodes, theMesh );
}

static SMDS_MeshNode* readNode(OSD_File& theFile,
                               DriverSTL_DataMapOfPntNodePtr& uniqnodes,
                               SMDS_Mesh* theMesh)
{
  Standard_ShortReal coord[3];
  coord[0] = readFloat(theFile);
  coord[1] = readFloat(theFile);
  coord[2] = readFloat(theFile);

  gp_Pnt P(coord[0],coord[1],coord[2]);
  return addNode( P, uniqnodes, theMesh );
}

//=======================================================================
//function : readAscii
//purpose  : 
//=======================================================================

Driver_Mesh::Status DriverSTL_R_SMDS_Mesh::readAscii() const
{
  Status aResult = DRS_OK;
  long ipos;
  Standard_Integer nbLines = 0;

  // Open the file 
  TCollection_AsciiString aFileName( (char *)myFile.c_str() );
  FILE* file = fopen(aFileName.ToCString(),"r");
  fseek(file,0L,SEEK_END);
  // get the file size
  long filesize = ftell(file);
  fclose(file);
  file = fopen(aFileName.ToCString(),"r");
  
  // count the number of lines
  for (ipos = 0; ipos < filesize; ++ipos) {
    if (getc(file) == '\n')
      nbLines++;
  }

  // go back to the beginning of the file
//  fclose(file);
//  file = fopen(aFileName.ToCString(),"r");
  rewind(file);
  
  Standard_Integer nbTri = (nbLines / ASCII_LINES_PER_FACET);

  DriverSTL_DataMapOfPntNodePtr uniqnodes;
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

Driver_Mesh::Status DriverSTL_R_SMDS_Mesh::readBinary() const
{
  Status aResult = DRS_OK;

  char buftest[5];
  Standard_Address adr;
  adr = (Standard_Address)buftest;

  TCollection_AsciiString aFileName( (char *)myFile.c_str() );
  OSD_File aFile = OSD_File(OSD_Path( aFileName ));
  aFile.Open(OSD_ReadOnly,OSD_Protection(OSD_RWD,OSD_RWD,OSD_RWD,OSD_RWD));

  // the size of the file (minus the header size)
  // must be a multiple of SIZEOF_STL_FACET

  // compute file size
  Standard_Integer filesize = aFile.Size();

  if ( (filesize - HEADER_SIZE) % SIZEOF_STL_FACET !=0 
      // Commented to allow reading small files (ex: 1 face)
      /*|| (filesize < STL_MIN_FILE_SIZE)*/) {
    Standard_NoMoreObject::Raise("DriverSTL_R_SMDS_MESH::readBinary (wrong file size)");
  }

  // don't trust the number of triangles which is coded in the file
  // sometimes it is wrong, and with this technique we don't need to swap endians for integer
  Standard_Integer nbTri = ((filesize - HEADER_SIZE) / SIZEOF_STL_FACET);

  // skip the header
  aFile.Seek(HEADER_SIZE,OSD_FromBeginning);

  DriverSTL_DataMapOfPntNodePtr uniqnodes;
  Standard_Integer lread;
  
  for (Standard_Integer iTri = 0; iTri < nbTri; ++iTri) {

    // ignore normals
    readFloat(aFile);
    readFloat(aFile);
    readFloat(aFile);

    // read vertices
    SMDS_MeshNode* node1 = readNode( aFile, uniqnodes, myMesh );
    SMDS_MeshNode* node2 = readNode( aFile, uniqnodes, myMesh );
    SMDS_MeshNode* node3 = readNode( aFile, uniqnodes, myMesh );

    if (myIsCreateFaces)
      myMesh->AddFace(node1,node2,node3);

    // skip extra bytes
    aFile.Read(adr,2,lread);
  }
  aFile.Close();
  return aResult;
}

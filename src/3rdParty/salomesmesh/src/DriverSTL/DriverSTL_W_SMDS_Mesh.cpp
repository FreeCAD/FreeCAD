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

#include "DriverSTL_W_SMDS_Mesh.h"

#include "SMDS_Mesh.hxx"
#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include <gp_XYZ.hxx>
#include <OSD_Path.hxx>
#include <OSD_File.hxx>
#include <OSD_FromWhere.hxx>
#include <OSD_Protection.hxx>
#include <OSD_SingleProtection.hxx>
#include <TCollection_AsciiString.hxx>
#include <TColgp_Array1OfXYZ.hxx>

#include "utilities.h"

//using namespace std;

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
  Status aResult = DRS_OK;

  if ( !myMesh ) {
    fprintf(stderr, ">> ERROR : Mesh is null \n");
    return DRS_FAIL;
  }
  if ( myIsAscii )
    aResult = writeAscii();
  else
    aResult = writeBinary();

  return aResult;
}

// static methods

static void writeInteger( const Standard_Integer& theVal,
			 OSD_File& ofile )
{
  union {
    Standard_Integer i;
    char c[4];
  }u;

  u.i = theVal;

  Standard_Integer entier;
  entier  =  u.c[0] & 0xFF;
  entier |= (u.c[1] & 0xFF) << 0x08;
  entier |= (u.c[2] & 0xFF) << 0x10;
  entier |= (u.c[3] & 0xFF) << 0x18;

  ofile.Write((char *)&entier,sizeof(u.c));
}

static void writeFloat  ( const Standard_ShortReal& theVal,
			 OSD_File& ofile)
{
  union {
    Standard_ShortReal f;
    char c[4]; 
  }u;

  u.f = theVal;

  Standard_Integer entier;

  entier  =  u.c[0] & 0xFF;
  entier |= (u.c[1] & 0xFF) << 0x08;
  entier |= (u.c[2] & 0xFF) << 0x10;
  entier |= (u.c[3] & 0xFF) << 0x18;

  ofile.Write((char *)&entier,sizeof(u.c));
}

static gp_XYZ getNormale( const SMDS_MeshFace* theFace )
{
  gp_XYZ n;
  int aNbNode = theFace->NbNodes();
  TColgp_Array1OfXYZ anArrOfXYZ(1,4);
  gp_XYZ p1, p2, p3, p4;
  SMDS_ElemIteratorPtr aNodeItr = theFace->nodesIterator();
  int i = 1;
  for ( ; aNodeItr->more() && i <= 4; i++ )
  {
    SMDS_MeshNode* aNode = (SMDS_MeshNode*)aNodeItr->next();
    anArrOfXYZ.SetValue(i, gp_XYZ( aNode->X(), aNode->Y(), aNode->Z() ) );
  }
  
  gp_XYZ q1 = anArrOfXYZ.Value(2) - anArrOfXYZ.Value(1);
  gp_XYZ q2 = anArrOfXYZ.Value(3) - anArrOfXYZ.Value(1);
  n  = q1 ^ q2;
  if ( aNbNode > 3 )
  {
    gp_XYZ q3 = anArrOfXYZ.Value(4) - anArrOfXYZ.Value(1);
    n += q2 ^ q3;
  }
  double len = n.Modulus();
  if ( len > 0 )
    n /= len;

  return n;
}

// private methods

Driver_Mesh::Status DriverSTL_W_SMDS_Mesh::writeAscii() const
{
  Status aResult = DRS_OK;
  TCollection_AsciiString aFileName( (char *)myFile.c_str() );
  if ( aFileName.IsEmpty() ) {
    fprintf(stderr, ">> ERREOR : invalid file name \n");
    return DRS_FAIL;
  }

  OSD_File aFile = OSD_File(OSD_Path(aFileName));
  aFile.Build(OSD_WriteOnly,OSD_Protection());
  TCollection_AsciiString buf = TCollection_AsciiString ("solid\n");
  aFile.Write (buf,buf.Length());buf.Clear();
  char sval[16];

  SMDS_FaceIteratorPtr itFaces = myMesh->facesIterator();
  
  for (; itFaces->more() ;) {
    SMDS_MeshFace* aFace = (SMDS_MeshFace*)itFaces->next();
    
    if (aFace->NbNodes() == 3) {
      gp_XYZ normale = getNormale( aFace );

      buf += " facet normal "; 
      sprintf (sval,"% 12e",normale.X());
      buf += sval;
      buf += " "; 
      sprintf (sval,"% 12e",normale.Y());
      buf += sval;
      buf += " "; 
      sprintf (sval,"% 12e",normale.Z());
      buf += sval;
      buf += '\n';
      aFile.Write (buf,buf.Length());buf.Clear();
      buf += "   outer loop\n"; 
      aFile.Write (buf,buf.Length());buf.Clear();
      
      SMDS_ElemIteratorPtr aNodeIter = aFace->nodesIterator();
      for (; aNodeIter->more(); ) {
	SMDS_MeshNode* node = (SMDS_MeshNode*)aNodeIter->next();
        buf += "     vertex "; 
        sprintf (sval,"% 12e",node->X());
        buf += sval;
        buf += " "; 
        sprintf (sval,"% 12e",node->Y());
        buf += sval;
        buf += " "; 
        sprintf (sval,"% 12e",node->Z());
        buf += sval;
        buf += '\n';
        aFile.Write (buf,buf.Length());buf.Clear();
      }
      buf += "   endloop\n"; 
      aFile.Write (buf,buf.Length());buf.Clear();
      buf += " endfacet\n"; 
      aFile.Write (buf,buf.Length());buf.Clear();
    } 
  }
  buf += "endsolid\n";
  aFile.Write (buf,buf.Length());buf.Clear();
  
  aFile.Close ();

  return aResult;
}

Driver_Mesh::Status DriverSTL_W_SMDS_Mesh::writeBinary() const
{
  Status aResult = DRS_OK;
  TCollection_AsciiString aFileName( (char *)myFile.c_str() );
  if ( aFileName.IsEmpty() ) {
    fprintf(stderr, ">> ERREOR : invalid filename \n");
    return DRS_FAIL;
  }

  OSD_File aFile = OSD_File(OSD_Path(aFileName));
  aFile.Build(OSD_WriteOnly,OSD_Protection());

  char sval[80];
  Standard_Integer nbTri = 0;
  SMDS_FaceIteratorPtr itFaces = myMesh->facesIterator();

  // we first count the number of triangles
  for (;itFaces->more();) {
    SMDS_MeshFace* aFace = (SMDS_MeshFace*)itFaces->next();
    if (aFace->NbNodes() == 3)
      nbTri++;
  }

  // write number of triangles
  //unsigned int NBT = nbTri;
  aFile.Write((Standard_Address)sval,LABEL_SIZE);
  writeInteger(nbTri,aFile);  

  // loop writing nodes. take face iterator again
  int dum=0;
  itFaces = myMesh->facesIterator();
  
  for (;itFaces->more();) {
    SMDS_MeshFace* aFace = (SMDS_MeshFace*)itFaces->next();
    
    if (aFace->NbNodes() == 3) {
      gp_XYZ aNorm = getNormale( aFace );
      writeFloat(aNorm.X(),aFile);
      writeFloat(aNorm.Y(),aFile);
      writeFloat(aNorm.Z(),aFile);

      SMDS_ElemIteratorPtr aNodeIter = aFace->nodesIterator();
      for (; aNodeIter->more(); ) {
	SMDS_MeshNode* node = (SMDS_MeshNode*)aNodeIter->next();
	writeFloat(node->X(),aFile);
	writeFloat(node->Y(),aFile);
	writeFloat(node->Z(),aFile);
      }
      aFile.Write (&dum,2);
    } 
  }
  aFile.Close ();

  return aResult;
}

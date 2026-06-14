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

//  SMESH SMESHDS : management of mesh data and SMESH document
//  File   : SMESHDS_Group.hxx
//  Module : SMESH
//  $Header$
//
#ifndef _SMESHDS_GroupBase_HeaderFile
#define _SMESHDS_GroupBase_HeaderFile

#include "SMESH_SMESHDS.hxx"

#include <string>
#include "SMDSAbs_ElementType.hxx"
#include "SMDS_MeshElement.hxx"

#include <Quantity_Color.hxx>
  
class SMESHDS_Mesh;

class SMESHDS_EXPORT SMESHDS_GroupBase
{
 public:

  SMESHDS_GroupBase (const int                 theID,
                     const SMESHDS_Mesh*       theMesh,
                     const SMDSAbs_ElementType theType);

  int GetID() const { return myID; }

  const SMESHDS_Mesh* GetMesh() const { return myMesh; }

  virtual void SetType(SMDSAbs_ElementType theType);

  SMDSAbs_ElementType GetType() const { return myType; }

  void SetStoreName (const char* theName) { myStoreName = theName; }

  const char* GetStoreName () const { return myStoreName.c_str(); }

  virtual int Extent() const;

  virtual bool IsEmpty();

  virtual bool Contains (const int theID);

  virtual bool Contains (const SMDS_MeshElement* elem);

  virtual SMDS_ElemIteratorPtr GetElements() const = 0;

  virtual int GetID (const int theIndex);
  // use it for iterations 1..Extent()

  virtual VTK_MTIME_TYPE GetTic() const = 0;

  virtual ~SMESHDS_GroupBase() {}

  void SetColor (const Quantity_Color& theColor)
  { myColor = theColor;}
  
  Quantity_Color GetColor() const
  { return myColor;}

  void SetColorGroup (int theColorGroup);

  int GetColorGroup() const;
  
  static void SetDefaultColor (const Quantity_Color& theColor)
  { myDefaultColor = theColor;}

 protected:
  const SMDS_MeshElement* findInMesh (const int theID) const;
  void resetIterator();

 private:
  SMESHDS_GroupBase (const SMESHDS_GroupBase& theOther);
  // prohibited copy constructor
  SMESHDS_GroupBase& operator = (const SMESHDS_GroupBase& theOther);
  // prohibited assign operator

  int                  myID;
  const SMESHDS_Mesh*  myMesh;
  SMDSAbs_ElementType  myType;
  std::string          myStoreName;
  Quantity_Color       myColor;

  // for GetID()
  int                  myCurIndex;
  int                  myCurID;
  SMDS_ElemIteratorPtr myIterator;

  static Quantity_Color myDefaultColor;
};

#endif

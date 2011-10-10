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
//  SMESH SMDS : implementaion of Salome mesh data structure
//  File   : SMDS_MeshElementIDFactory.cxx
//  Author : Jean-Michel BOULCOURT
//  Module : SMESH
//
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "SMDS_MeshElementIDFactory.hxx"
#include "SMDS_MeshElement.hxx"

using namespace std;

//=======================================================================
//function : SMDS_MeshElementIDFactory
//purpose  : 
//=======================================================================
SMDS_MeshElementIDFactory::SMDS_MeshElementIDFactory():
  SMDS_MeshIDFactory(),
  myMin(0), myMax(0)
{
}

//=======================================================================
//function : BindID
//purpose  : 
//=======================================================================
bool SMDS_MeshElementIDFactory::BindID(int ID, SMDS_MeshElement * elem)
{
  if (myIDElements.IsBound(ID))
    return false;
  myIDElements.Bind(ID,elem);
  elem->myID=ID;
  updateMinMax (ID);
  return true;
}

//=======================================================================
//function : MeshElement
//purpose  : 
//=======================================================================
SMDS_MeshElement* SMDS_MeshElementIDFactory::MeshElement(int ID)
{
  if (!myIDElements.IsBound(ID))
    return NULL;
  return myIDElements.Find(ID);
}


//=======================================================================
//function : GetFreeID
//purpose  : 
//=======================================================================
int SMDS_MeshElementIDFactory::GetFreeID()
{
  int ID;
  do {
    ID = SMDS_MeshIDFactory::GetFreeID();
  } while (myIDElements.IsBound(ID));
  return ID;
}

//=======================================================================
//function : ReleaseID
//purpose  : 
//=======================================================================
void SMDS_MeshElementIDFactory::ReleaseID(const int ID)
{
  myIDElements.UnBind(ID);
  SMDS_MeshIDFactory::ReleaseID(ID);
  if (ID == myMax)
    myMax = 0;
  if (ID == myMin)
    myMin = 0;
}

//=======================================================================
//function : GetMaxID
//purpose  : 
//=======================================================================

int SMDS_MeshElementIDFactory::GetMaxID() const
{
  if (myMax == 0)
    updateMinMax();
  return myMax;
}

//=======================================================================
//function : GetMinID
//purpose  : 
//=======================================================================

int SMDS_MeshElementIDFactory::GetMinID() const
{
  if (myMin == 0)
    updateMinMax();
  return myMin;
}

//=======================================================================
//function : updateMinMax
//purpose  : 
//=======================================================================

void SMDS_MeshElementIDFactory::updateMinMax() const
{
  myMin = IntegerLast();
  myMax = 0;
  SMDS_IdElementMap::Iterator it(myIDElements);
  for (; it.More(); it.Next())
    updateMinMax (it.Key());
  if (myMin == IntegerLast())
    myMin = 0;
}

//=======================================================================
//function : elementsIterator
//purpose  : Return an iterator on elements of the factory
//=======================================================================

class SMDS_Fact_MyElemIterator:public SMDS_ElemIterator
{
  SMDS_IdElementMap::Iterator myIterator;
 public:
  SMDS_Fact_MyElemIterator(const SMDS_IdElementMap& s):myIterator(s)
  {}

  bool more()
  {
    return myIterator.More() != Standard_False;
  }

  const SMDS_MeshElement* next()
  {
    const SMDS_MeshElement* current = myIterator.Value();
    myIterator.Next();
    return current;
  }
};

SMDS_ElemIteratorPtr SMDS_MeshElementIDFactory::elementsIterator() const
{
  return SMDS_ElemIteratorPtr
    (new SMDS_Fact_MyElemIterator(myIDElements));
}

void SMDS_MeshElementIDFactory::Clear()
{
  myIDElements.Clear();
  myMin = myMax = 0;
  SMDS_MeshIDFactory::Clear();
}

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
//
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "SMDS_MeshNode.hxx"
#include "SMDS_SpacePosition.hxx"
#include "SMDS_IteratorOfElements.hxx"

using namespace std;

//=======================================================================
//function : SMDS_MeshNode
//purpose  : 
//=======================================================================

SMDS_MeshNode::SMDS_MeshNode(double x, double y, double z):
	myX(x), myY(y), myZ(z),
	myPosition(SMDS_SpacePosition::originSpacePosition())
{
}

//=======================================================================
//function : RemoveInverseElement
//purpose  : 
//=======================================================================

void SMDS_MeshNode::RemoveInverseElement(const SMDS_MeshElement * parent)
{
  NCollection_List<const SMDS_MeshElement*>::Iterator it(myInverseElements);
  while (it.More()) {
    const SMDS_MeshElement* elem = it.Value();
    if (elem == parent)
      myInverseElements.Remove(it);
    else
      it.Next();
  }
}

//=======================================================================
//function : Print
//purpose  : 
//=======================================================================

void SMDS_MeshNode::Print(ostream & OS) const
{
	OS << "Node <" << GetID() << "> : X = " << myX << " Y = "
		<< myY << " Z = " << myZ << endl;
}

//=======================================================================
//function : SetPosition
//purpose  : 
//=======================================================================

void SMDS_MeshNode::SetPosition(const SMDS_PositionPtr& aPos)
{
	myPosition = aPos;
}

//=======================================================================
//function : GetPosition
//purpose  : 
//=======================================================================

const SMDS_PositionPtr& SMDS_MeshNode::GetPosition() const
{
	return myPosition;
}

//=======================================================================
/*!
 * \brief Iterator on list of elements
 */
//=======================================================================

class SMDS_MeshNode_MyInvIterator:public SMDS_ElemIterator
{
  NCollection_List<const SMDS_MeshElement*>::Iterator myIterator;
  SMDSAbs_ElementType                                 myType;
 public:
  SMDS_MeshNode_MyInvIterator(const NCollection_List<const SMDS_MeshElement*>& s,
                              SMDSAbs_ElementType type):
    myIterator(s), myType(type)
  {}

  bool more()
  {
    if ( myType != SMDSAbs_All ) {
      while ( myIterator.More() && myIterator.Value()->GetType() != myType)
        myIterator.Next();
    }
    return myIterator.More() != Standard_False;
  }

  const SMDS_MeshElement* next()
  {
    const SMDS_MeshElement* current=myIterator.Value();
    myIterator.Next();
    return current;
  }	
};

SMDS_ElemIteratorPtr SMDS_MeshNode::
	GetInverseElementIterator(SMDSAbs_ElementType type) const
{
  return SMDS_ElemIteratorPtr(new SMDS_MeshNode_MyInvIterator(myInverseElements,type));
}

// Same as GetInverseElementIterator but the create iterator only return
// wanted type elements.
class SMDS_MeshNode_MyIterator:public SMDS_ElemIterator
{
  NCollection_List<const SMDS_MeshElement*> mySet;
  NCollection_List<const SMDS_MeshElement*>::Iterator myIterator;
 public:
  SMDS_MeshNode_MyIterator(SMDSAbs_ElementType type,
                           const NCollection_List<const SMDS_MeshElement*>& s)
  {
    const SMDS_MeshElement * e;
    bool toInsert;
    NCollection_List<const SMDS_MeshElement*>::Iterator it(s);
    for(; it.More(); it.Next())
    {
      e=it.Value();
      switch(type)
      {
      case SMDSAbs_Edge: toInsert=true; break;
      case SMDSAbs_Face: toInsert=(e->GetType()!=SMDSAbs_Edge); break;
      case SMDSAbs_Volume: toInsert=(e->GetType()==SMDSAbs_Volume); break;
      }
      if(toInsert) mySet.Append(e);
    }
    myIterator.Init(mySet);
  }

  bool more()
  {
    return myIterator.More() != Standard_False;
  }

  const SMDS_MeshElement* next()
  {
    const SMDS_MeshElement* current=myIterator.Value();
    myIterator.Next();
    return current;
  }
};

SMDS_ElemIteratorPtr SMDS_MeshNode::
	elementsIterator(SMDSAbs_ElementType type) const
{
  if(type==SMDSAbs_Node)
    return SMDS_MeshElement::elementsIterator(SMDSAbs_Node); 
  else
    return SMDS_ElemIteratorPtr
      (new SMDS_IteratorOfElements
       (this,type,
        SMDS_ElemIteratorPtr(new SMDS_MeshNode_MyIterator(type, myInverseElements))));
}

int SMDS_MeshNode::NbNodes() const
{
	return 1;
}

double SMDS_MeshNode::X() const
{
	return myX;
}

double SMDS_MeshNode::Y() const
{
	return myY;
}

double SMDS_MeshNode::Z() const
{
	return myZ;
}

void SMDS_MeshNode::setXYZ(double x, double y, double z)
{
	myX=x;
	myY=y;
	myZ=z;	
}

SMDSAbs_ElementType SMDS_MeshNode::GetType() const
{
	return SMDSAbs_Node;
}

//=======================================================================
//function : AddInverseElement
//purpose  :
//=======================================================================
void SMDS_MeshNode::AddInverseElement(const SMDS_MeshElement* ME)
{
  NCollection_List<const SMDS_MeshElement*>::Iterator it(myInverseElements);
  for (; it.More(); it.Next()) {
    const SMDS_MeshElement* elem = it.Value();
    if (elem == ME)
      return;
  }
  myInverseElements.Append(ME);
}

//=======================================================================
//function : ClearInverseElements
//purpose  :
//=======================================================================
void SMDS_MeshNode::ClearInverseElements()
{
  myInverseElements.Clear();
}

bool SMDS_MeshNode::emptyInverseElements()
{
  return myInverseElements.IsEmpty() != Standard_False;
}

//================================================================================
/*!
 * \brief Count inverse elements of given type
 */
//================================================================================

int SMDS_MeshNode::NbInverseElements(SMDSAbs_ElementType type) const
{
  if ( type == SMDSAbs_All )
    return myInverseElements.Extent();
  int nb = 0;
  NCollection_List<const SMDS_MeshElement*>::Iterator it( myInverseElements );
  for ( ; it.More(); it.Next() )
    if ( it.Value()->GetType() == type )
      nb++;
  return nb;
}

///////////////////////////////////////////////////////////////////////////////
/// To be used with STL set
///////////////////////////////////////////////////////////////////////////////
bool operator<(const SMDS_MeshNode& e1, const SMDS_MeshNode& e2)
{
	return e1.GetID()<e2.GetID();
	/*if(e1.myX<e2.myX) return true;
	else if(e1.myX==e2.myX)
	{
	 	if(e1.myY<e2.myY) return true;
		else if(e1.myY==e2.myY) return (e1.myZ<e2.myZ);
		else return false;
	}
	else return false;*/
}


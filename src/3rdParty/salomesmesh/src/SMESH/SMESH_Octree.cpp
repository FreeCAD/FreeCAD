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
//  SMESH  SMESH_Octree : global Octree implementation
//
//  File      : SMESH_Octree.cxx
//  Created   : Tue Jan 16 16:00:00 2007
//  Author    : Nicolas Geimer & Aurélien Motteux(OCC)
//  Module    : SMESH

#include "SMESH_Octree.hxx"

//===========================================================================
/*!
 * Constructor. limit must be provided at tree root construction.
 * limit will be deleted by SMESH_Octree.
 */
//===========================================================================

SMESH_Octree::SMESH_Octree (SMESH_Octree::Limit* limit):
  myChildren(NULL),
  myFather(NULL),
  myIsLeaf( false ),
  myLimit( limit ),
  myLevel(0),
  myBox(NULL)
{
}

//================================================================================
/*!
 * \brief Compute the Octree
 */
//================================================================================

void SMESH_Octree::compute()
{
  if ( myLevel==0 )
  {
    myBox = buildRootBox();
    if ( myLimit->myMinBoxSize > 0. && maxSize() <= myLimit->myMinBoxSize )
      myIsLeaf = true;
    else
      buildChildren();
  }
}

//======================================
/*!
 * \brief SMESH_Octree Destructor
 */
//======================================

SMESH_Octree::~SMESH_Octree ()
{
  if(myChildren != NULL)
  {
    if(!isLeaf())
    {
      for(int i = 0; i<8; i++)
        delete myChildren[i];
      delete[] myChildren;
      myChildren = 0;
    }
  }
  if ( myBox )
    delete myBox;
  myBox = 0;
  if ( level() == 0 )
    delete myLimit;
  myLimit = 0;
}

//=================================================================
/*!
 * \brief Build the 8 children boxes and call buildChildrenData()
 */
//=================================================================

void SMESH_Octree::buildChildren()
{
  if ( isLeaf() ) return;

  myChildren = new SMESH_Octree*[8];

  gp_XYZ min = myBox->CornerMin();
  gp_XYZ max = myBox->CornerMax();
  gp_XYZ HSize = (max - min)/2.;
  gp_XYZ mid = min + HSize;
  gp_XYZ childHsize = HSize/2.;

  Standard_Real XminChild, YminChild, ZminChild;
  gp_XYZ minChild;
  for (int i = 0; i < 8; i++)
  {
    // We build the eight boxes, we need 2 points to do that:
    // Min and Mid
    // In binary, we can write i from 0 to 7
    // For instance :
    // 5 is 101, it corresponds here in coordinates to ZYX
    // If coordinate is 0 in Y-> box from Ymin to Ymid
    // If coordinate is 1 in Y-> box from Ymid to Ymax
    // Same scheme for X and Z
    // I need the minChild to build the Bnd_B3d box.

    XminChild = (i%2==0)?min.X():mid.X();
    YminChild = ((i%4)/2==0)?min.Y():mid.Y();
    ZminChild = (i<4)?min.Z():mid.Z();
    minChild.SetCoord(XminChild, YminChild, ZminChild);

    // The child is of the same type than its father (For instance, a SMESH_OctreeNode)
    // We allocate the memory we need for the child
    myChildren[i] = allocateOctreeChild();
    // and we assign to him its box.
    myChildren[i]->myFather = this;
    myChildren[i]->myLimit = myLimit;
    myChildren[i]->myLevel = myLevel + 1;
    myChildren[i]->myBox = new Bnd_B3d(minChild+childHsize,childHsize);
    if ( myLimit->myMinBoxSize > 0. && myChildren[i]->maxSize() <= myLimit->myMinBoxSize )
      myChildren[i]->myIsLeaf = true;
  }

  // After building the 8 boxes, we put the data into the children.
  buildChildrenData();

  //After we pass to the next level of the Octree
  for (int i = 0; i<8; i++)
    myChildren[i]->buildChildren();
}

//================================================================================
/*!
 * \brief Tell if Octree is a leaf or not
 *        An inheriting class can influence it via myIsLeaf protected field
 */
//================================================================================

bool SMESH_Octree::isLeaf() const
{
  return myIsLeaf || ((myLimit->myMaxLevel > 0) ? (level() >= myLimit->myMaxLevel) : false );
}

//===========================================================================
/*!
 * \brief Compute the bigger dimension of my box
 */
//===========================================================================

double SMESH_Octree::maxSize() const
{
  if ( myBox )
  {
    gp_XYZ min = myBox->CornerMin();
    gp_XYZ max = myBox->CornerMax();
    gp_XYZ Size = (max - min);
    double returnVal = (Size.X()>Size.Y())?Size.X():Size.Y();
    return (returnVal>Size.Z())?returnVal:Size.Z();
  }
  return 0.;
}

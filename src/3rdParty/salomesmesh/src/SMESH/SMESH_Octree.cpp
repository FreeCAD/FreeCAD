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
// File      : SMESH_Octree.cxx
// Created   : Tue Jan 16 16:00:00 2007
// Author    : Nicolas Geimer & Aurélien Motteux(OCC)
// Module    : SMESH
//
#include "SMESH_Octree.hxx"

//===========================================================================
/*!
 * \brief SMESH_Octree Constructor
 * \param maxLevel     - The level max the octree can reach (If <0 unlimited)
 */
//===========================================================================
SMESH_Octree::SMESH_Octree (const int maxLevel, const double minBoxSize):
    myChildren(NULL),
    myFather(NULL),
    myLevel(0),
    myMaxLevel(maxLevel),
    myMinBoxSize(minBoxSize),
    myIsLeaf(-1)
{
  myBox = new Bnd_B3d();
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
    if(!myIsLeaf)
    {
      for(int i = 0; i<8; i++)
        delete myChildren[i];
      delete[] myChildren ;
    }
  }
  delete myBox;
}

//===========================================================================
/*!
 * \brief Set the bounding box of the Octree
 * \param box          - 3d Bounding Box of the Octree
 */
//===========================================================================
void SMESH_Octree::setBox(const Bnd_B3d* box)
{
//   delete myBox;
//   myBox=new Bnd_B3d(*box);
  *myBox = *box;
}

//===========================================================================
/*!
 * \brief Set box to the 3d Bounding Box of the Octree
 * \param box          - Set box to the 3d Bounding Box of the Octree
 */
//===========================================================================
void SMESH_Octree::getBox(Bnd_B3d& box)
{
//   if(box != NULL)
//     delete box;
//   box = new Bnd_B3d (*myBox);
  box = *myBox;
}

//===========================================================================
/*!
 * \brief Set the max level of the Octree
 * \param maxLevel     - The level max the octree can reach (If <0 unlimited)
 */
//===========================================================================
void SMESH_Octree::setMaxLevel(const int maxLevel)
{myMaxLevel = maxLevel;}


//===========================================================================
/*!
 * \brief Compute the bigger dimension of the box
 * \param box          - 3d Box
 * \retval double - bigger dimension of the box
 */
//===========================================================================
double SMESH_Octree::maxSize(const Bnd_B3d* box)
{
  if(box ==NULL)
    return 0;
  gp_XYZ min = box->CornerMin();
  gp_XYZ max = box->CornerMax();
  gp_XYZ Size = (max - min);
  double returnVal = (Size.X()>Size.Y())?Size.X():Size.Y();
  return (returnVal>Size.Z())?returnVal:Size.Z();
}

//=============================
/*!
 * \brief Compute the Octree
 */
//=============================
void SMESH_Octree::Compute()
{
  // As soon as the Octree is a Leaf, I stop building his children
  if(!isLeaf())
    buildChildren();
}

//=================================================================
/*!
 * \brief Build the 8 children boxes and call buildChildrenData()
 */
//=================================================================
void SMESH_Octree::buildChildren()
{
  myChildren = new SMESH_Octree*[8];

  gp_XYZ min = myBox->CornerMin();
  gp_XYZ max = myBox->CornerMax();
  gp_XYZ HSize = (max - min)/2.;
  gp_XYZ mid = min + HSize;
  gp_XYZ childHsize = HSize/2.;

  Standard_Real XminChild, YminChild, ZminChild;
  Bnd_B3d* box;
  gp_XYZ minChild;
  for (int i =0; i<8; i++)
  {
    // We build the eight boxes, we need 2 points to do that.
    // Min, and Mid
    // In binary, we can write i from 0 to 7
    // For instance :
    // 5 is 101, it corresponds here in coordinates to ZYX
    // If coordinate is 0 in Y-> box from Ymin to Ymid
    // If coordinate is 1 in Y-> box from Ymid to Ymax
    // Same scheme for X and Z
    // I need the minChild to build the Bnd_B3d box.

    XminChild= (i%2==0)?min.X():mid.X();
    YminChild= ((i%4)/2==0)?min.Y():mid.Y();
    ZminChild= (i<4)?min.Z():mid.Z();
    minChild.SetCoord(XminChild, YminChild, ZminChild);

    box = new Bnd_B3d(minChild+childHsize,childHsize);
    // The child is of the same type than its father (For instance, a SMESH_OctreeNode)
    // We allocate the memory we need fot the child
    myChildren[i] = allocateOctreeChild();
    // and we assign to him its box.
    myChildren[i]->setBox(box);
    delete box;
  }

  // After building the 8 boxes, we put the data into the children..
  buildChildrenData();

  //After we pass to the next level of the Octree
  for (int i =0; i<8; i++)
    myChildren[i]->Compute();
}

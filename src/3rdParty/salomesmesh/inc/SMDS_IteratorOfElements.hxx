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

//  SMESH SMDS : implementation of Salome mesh data structure
//
#include "SMESH_SMDS.hxx"

#include "SMDS_Iterator.hxx"
#include "SMDS_MeshElement.hxx"
#include <set>


class SMDS_EXPORT SMDS_IteratorOfElements:public SMDS_ElemIterator
{
  public:
/////////////////////////////////////////////////////////////////////////////
/// Create an iterator which look for elements of type type which are linked 
/// to the element element. it is the iterator to get connectivity of element
//////////////////////////////////////////////////////////////////////////////
        SMDS_IteratorOfElements(const SMDS_MeshElement * element,
                                SMDSAbs_ElementType type,
                                const SMDS_ElemIteratorPtr& it);
        bool more();
        const SMDS_MeshElement * next();

  private:
        SMDS_ElemIteratorPtr t2Iterator;
        SMDS_ElemIteratorPtr t1Iterator;
        SMDSAbs_ElementType myType;     
        const SMDS_MeshElement * myProxyElement;
        const SMDS_MeshElement * myElement;             
        bool myReverseIteration;

        std::set<const SMDS_MeshElement*> alreadyReturnedElements;
        std::set<const SMDS_MeshElement*>::iterator itAlreadyReturned;  
        bool subMore();
        const SMDS_MeshElement * subNext();
};

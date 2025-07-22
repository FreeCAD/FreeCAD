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
#ifndef _SMDS_Iterator_HeaderFile
#define _SMDS_Iterator_HeaderFile

#include "SMESH_SMDS.hxx"

///////////////////////////////////////////////////////////////////////////////
///Abstract class for iterators
///@author Jerome Robert
///////////////////////////////////////////////////////////////////////////////
template<typename VALUE> class SMDS_Iterator
{
  public:
        /// Return true if and only if there are other object in this iterator
          virtual bool more()=0;
        
        /// Return the current object and step to the next one
          virtual VALUE next()=0;
        
        /// Delete the current element and step to the next one
        virtual void remove(){}
        
        /// Provide virtual destructor just for case if some derived iterator
        /// must have a destructor
        virtual ~SMDS_Iterator(){}
};

#endif

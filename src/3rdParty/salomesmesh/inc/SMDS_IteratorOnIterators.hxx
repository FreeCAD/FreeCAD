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
// File      : SMDS_IteratorOnIterators.hxx
// Author    : Edward AGAPOV (eap)
//
#ifndef SMDS_IteratorOnIterators_HeaderFile
#define SMDS_IteratorOnIterators_HeaderFile

#include "SMDS_Iterator.hxx"

///////////////////////////////////////////////////////////////////////////////
/// SMDS_Iterator iterating over all elements provided by other iterators
///
/// Other iterators must implement SMDS_Iterator iterface and
/// must be provided within a stl-like container
/// BE CAREFUL: iterator pointed value is static_cast'ed to VALUE
///////////////////////////////////////////////////////////////////////////////

template<typename VALUE,
         typename CONTAINER_OF_ITERATORS >
class SMDS_IteratorOnIterators : public SMDS_Iterator<VALUE>
{
protected:
  CONTAINER_OF_ITERATORS _iterators;
  typename CONTAINER_OF_ITERATORS::iterator _beg, _end;
public:
  SMDS_IteratorOnIterators(const CONTAINER_OF_ITERATORS& iterators):
    _iterators( iterators ), _beg( _iterators.begin()), _end(_iterators.end() )
  {
    while ( _beg != _end && !(*_beg)->more()) ++_beg;
  }

  /// Return true iff there are other object in this iterator
  virtual bool more() { return _beg != _end && (*_beg)->more(); }

  /// Return the current object and step to the next one
  virtual VALUE next()
  {
    VALUE __v = (VALUE)(*_beg)->next();
    while ( _beg != _end && !(*_beg)->more())
      ++_beg;
    return __v;
  }
};

#endif

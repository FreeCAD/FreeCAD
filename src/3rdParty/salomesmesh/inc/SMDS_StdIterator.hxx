// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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
// File      : SMDS_StdIterator.hxx
// Created   : Fri Feb  5 11:03:46 2010
// Author    : Edward AGAPOV (eap)
//
#ifndef __SMDS_StdIterator_HXX__
#define __SMDS_StdIterator_HXX__


///////////////////////////////////////////////////////////////////////////////
/*!
 * \brief Wrapper over pointer to SMDS_Iterator, like SMDS_ElemIteratorPtr, enabling
 *   its usage in std-like way: provide operators ++, *,  etc.
 */
///////////////////////////////////////////////////////////////////////////////

template<typename VALUE, class PtrSMDSIterator, class EqualVALUE = std::equal_to<VALUE> >
class SMDS_StdIterator
{
  VALUE           _value;
  PtrSMDSIterator _piterator;
  EqualVALUE      _EqualVALUE;

public:
  using iterator_category = std::input_iterator_tag;
  using value_type = VALUE;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;
  typedef SMDS_StdIterator<VALUE, PtrSMDSIterator> _Self;

  // constructor to use as return from begin()
  SMDS_StdIterator( PtrSMDSIterator pItr )
    : _value( pItr->more() ? (VALUE)(pItr->next()) : 0 ), _piterator(pItr)
  {}
  // constructor to use as return from end()
  SMDS_StdIterator(): _value( 0 )
  {}

  /// Return the current object
  VALUE operator*() const
  { return _value; }

  //  Step to the next one
  _Self&
  operator++()
  { _value = _piterator->more() ? VALUE( _piterator->next()) : 0; return *this; }

  //  Step to the next one
  _Self
  operator++(int)
  { _Self res = *this; _value = _piterator->more() ? VALUE( _piterator->next()) : 0; return res; }

  // Test of end
  bool
  operator!=(const _Self& __x) const
  { return !_EqualVALUE( _value, __x._value); }

  // Test of equality
  bool
  operator==(const _Self& __x) const
  { return _EqualVALUE( _value, __x._value); }

};

#endif

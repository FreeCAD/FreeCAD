/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef MESH_ITERATOR_H
#define MESH_ITERATOR_H

#include "MeshKernel.h"
#include "Elements.h"
#include <Base/Matrix.h>
#include <Base/Vector3D.h>
#include <climits>

namespace MeshCore {

class MeshKernel;
class MeshGeomFacet;
class MeshPoint;
class MeshGeomEdge;
class MeshIndexEdge;
class MeshHelpEdge;

/**
 * The MeshFacetIterator allows to iterate over the facets that
 * hold the topology of the mesh and provides access to their
 * geometric information.
 */
class MeshFacetIterator
{
public:
  /** @name Construction */
  //@{
  /// construction
  inline MeshFacetIterator (const MeshKernel &rclM);
  /// construction
  inline MeshFacetIterator (const MeshKernel &rclM, unsigned long ulPos);
  /// construction
  inline MeshFacetIterator (const MeshFacetIterator &rclI);
  //@}

  /** @name Transformation */
  //@{
  /// Transforms the returned facet points with the current transformation
  inline void Transform( const Base::Matrix4D& rclTrf );
  //@}

  /** @name Access methods */
  //@{
  /// Access to the element the iterator points to.
  const MeshGeomFacet& operator*(void)
  { return Dereference(); }
  /// Access to the element the iterator points to.
  const MeshGeomFacet* operator->(void)
  { return &Dereference(); }
  /// Increments the iterator. It points then to the next element if the
  /// end is not reached.
  const MeshFacetIterator& operator ++ (void)
  { ++_clIter; return *this; }
  /// Decrements the iterator. It points then to the previous element if the beginning
  /// is not reached.
  const MeshFacetIterator& operator -- (void)
  { --_clIter; return *this; }
  /// Increments the iterator by \a k positions.
  const MeshFacetIterator& operator += (int k)
  { _clIter += k; return *this; }
  /// Decrements the iterator by \a k positions.
  const MeshFacetIterator& operator -= (int k)
  { _clIter -= k; return *this; }
  /// Assignment.
  inline MeshFacetIterator& operator = (const MeshFacetIterator &rpI);
  /// Compares if this iterator points to a lower element than the other one.
  bool operator < (const MeshFacetIterator &rclI) const
  { return _clIter < rclI._clIter; }
  /// Compares if this iterator points to a higher element than the other one.
  bool operator > (const MeshFacetIterator &rclI) const
  { return _clIter > rclI._clIter; }
  /// Checks if the iterators points to the same element.
  bool operator == (const MeshFacetIterator &rclI) const
  { return _clIter == rclI._clIter; }
  /// Sets the iterator to the beginning of the array.
  void Begin (void)
  { _clIter = _rclFAry.begin(); }
  /// Sets the iterator to the end of the array.
  void End (void)
  { _clIter = _rclFAry.end(); }
  /// Returns the current position of the iterator in the array.
  unsigned long Position (void) const
  { return _clIter - _rclFAry.begin(); }
  /// Checks if the end is already reached.
  bool EndReached (void) const
  { return !(_clIter < _rclFAry.end()); }
  /// Sets the iterator to the beginning of the array.
  void  Init (void)
  { Begin(); }
  /// Checks if the end is not yet reached.
  bool More (void) const
  { return !EndReached(); }
  /// Increments the iterator.
  void  Next (void)
  { operator++(); }
  /// Sets the iterator to a given position.
  inline bool Set (unsigned long ulIndex);
  /// Returns the topologic facet.
  inline MeshFacet GetIndices (void) const
  { return *_clIter; }
  /// Returns the topologic facet.
  inline const MeshFacet& GetReference (void) const
  { return *_clIter; }
  /// Returns iterators pointing to the current facet's neighbours.
  inline void GetNeighbours (MeshFacetIterator &rclN0, MeshFacetIterator &rclN1, MeshFacetIterator &rclN2) const;
  /// Sets the iterator to the current facet's neighbour of the side \a usN.
  inline void SetToNeighbour (unsigned short usN);
  /// Returns the property information to the current facet.
  inline unsigned long GetProperty (void) const;
  /// Checks if the iterator points to a valid element inside the array.
  inline bool IsValid (void) const
  { return (_clIter >= _rclFAry.begin()) && (_clIter < _rclFAry.end()); }
  //@}
  /** @name Flag state
   */
  //@{
  void SetFlag (MeshFacet::TFlagType tF) const
  { this->_clIter->SetFlag(tF); }
  void ResetFlag (MeshFacet::TFlagType tF) const
  { this->_clIter->ResetFlag(tF); }
  bool IsFlag (MeshFacet::TFlagType tF) const
  { return this->_clIter->IsFlag(tF); }
  void SetProperty(unsigned long uP) const
  { this->_clIter->SetProperty(uP); }
  //@}

protected:
  inline const MeshGeomFacet& Dereference (void);

protected:
  const MeshKernel&     _rclMesh;
  const MeshFacetArray& _rclFAry;
  const MeshPointArray& _rclPAry;
  MeshFacetArray::_TConstIterator _clIter;
  MeshGeomFacet _clFacet;
  bool _bApply;
  Base::Matrix4D _clTrf;

  // friends
  friend class MeshKernel;
};

/**
 * The MeshPointIterator allows to iterate over the vertices of the mesh and provides access to their
 * geometric information.
 */
class MeshExport MeshPointIterator
{
public:
  /** @name Construction */
  //@{
  inline MeshPointIterator (const MeshKernel &rclM);
  inline MeshPointIterator (const MeshKernel &rclM, unsigned long ulPos);
  inline MeshPointIterator (const MeshPointIterator &rclI);
  //@}
 
  /** @name Transformation */
  //@{
  /// Transforms the returned points with the current transformation
  inline void Transform( const Base::Matrix4D& rclTrf );
  //@}
 
  /** @name Access methods */
  //@{
  /// Access to the element the iterator points to.
  const MeshPoint& operator*(void) const
  { return Dereference(); }
  /// Access to the element the iterator points to.
  const MeshPoint* operator->(void) const
  { return &Dereference(); }
  /// Increments the iterator. It points then to the next element if the
  /// end is not reached.
  const MeshPointIterator& operator ++ (void)
  { ++_clIter; return *this; }
  /// Decrements the iterator. It points then to the previous element if the beginning
  /// is not reached.
  const MeshPointIterator& operator -- (void)
  { --_clIter; return *this; }
  /// Assignment.
  inline MeshPointIterator& operator = (const MeshPointIterator &rpI);
  /// Compares if this iterator points to a lower element than the other one.
  bool operator < (const MeshPointIterator &rclI) const
  { return _clIter < rclI._clIter; }
  /// Compares if this iterator points to a higher element than the other one.
  bool operator > (const MeshPointIterator &rclI) const
  { return _clIter > rclI._clIter; }
  /// Checks if the iterators points to the same element.
  bool operator == (const MeshPointIterator &rclI) const
  { return _clIter == rclI._clIter; }
  /// Sets the iterator to the beginning of the array.
  void Begin (void)
  { _clIter = _rclPAry.begin(); }
  /// Sets the iterator to the end of the array.
  void End (void)
  { _clIter = _rclPAry.end(); }
  /// Returns the current position of the iterator in the array.
  unsigned long Position (void) const
  { return _clIter - _rclPAry.begin(); }
  /// Checks if the end is already reached.
  bool EndReached (void) const
  { return !(_clIter < _rclPAry.end()); }
  /// Sets the iterator to the beginning of the array.
  void  Init (void)
  { Begin(); }
  /// Checks if the end is not yet reached.
  bool More (void) const
  { return !EndReached(); }
  /// Increments the iterator.
  void  Next (void)
  { operator++(); }
  /// Sets the iterator to a given position.
  inline bool Set (unsigned long ulIndex);
  /// Checks if the iterator points to a valid element inside the array.
  inline bool IsValid (void) const
  { return (_clIter >= _rclPAry.begin()) && (_clIter < _rclPAry.end()); }
  //@}
  /** @name Flag state
   */
  //@{
  void SetFlag (MeshPoint::TFlagType tF) const
  { this->_clIter->SetFlag(tF); }
  void ResetFlag (MeshPoint::TFlagType tF) const
  { this->_clIter->ResetFlag(tF); }
  bool IsFlag (MeshPoint::TFlagType tF) const
  { return this->_clIter->IsFlag(tF); }
  void SetProperty(unsigned long uP) const
  { this->_clIter->SetProperty(uP); }
  //@}

protected:
  inline const MeshPoint& Dereference (void) const;

protected:
  const MeshKernel& _rclMesh;
  const MeshPointArray& _rclPAry;
  MeshPoint _clPoint;
  MeshPointArray::_TConstIterator _clIter;
  bool _bApply;
  Base::Matrix4D _clTrf;

  // friends
  friend class MeshKernel;
};

class MeshFastFacetIterator
{
public:
  inline MeshFastFacetIterator (const MeshKernel &rclM);
  virtual ~MeshFastFacetIterator () {}

  void Init (void) { _clIter = _rclFAry.begin(); }
  inline void Next (void);
  bool More (void) { return _clIter != _rclFAry.end(); }

  Base::Vector3f _afPoints[3];

protected:
  const MeshKernel&     _rclMesh;
  const MeshFacetArray& _rclFAry;
  const MeshPointArray& _rclPAry;
  MeshFacetArray::_TConstIterator _clIter;

private:
  MeshFastFacetIterator (const MeshFastFacetIterator&);
  void operator = (const MeshFastFacetIterator&);
};

inline MeshFastFacetIterator::MeshFastFacetIterator (const MeshKernel &rclM)
: _rclMesh(rclM),
  _rclFAry(rclM._aclFacetArray),
  _rclPAry(rclM._aclPointArray),
  _clIter(_rclFAry.begin())
{
}

inline void MeshFastFacetIterator::Next (void)
{
  const unsigned long *paulPt = _clIter->_aulPoints;
  Base::Vector3f *pfPt = _afPoints;
  *(pfPt++)      = _rclPAry[*(paulPt++)];
  *(pfPt++)      = _rclPAry[*(paulPt++)];
  *pfPt          = _rclPAry[*paulPt];
}

inline MeshFacetIterator::MeshFacetIterator (const MeshKernel &rclM)
: _rclMesh(rclM),
  _rclFAry(rclM._aclFacetArray),
  _rclPAry(rclM._aclPointArray),
  _clIter(rclM._aclFacetArray.begin()),
  _bApply(false)
{
}

inline MeshFacetIterator::MeshFacetIterator (const MeshKernel &rclM, unsigned long ulPos)
: _rclMesh(rclM),
  _rclFAry(rclM._aclFacetArray),
  _rclPAry(rclM._aclPointArray),
  _clIter(rclM._aclFacetArray.begin() + ulPos),
  _bApply(false)
{
}

inline MeshFacetIterator::MeshFacetIterator (const MeshFacetIterator &rclI)
: _rclMesh(rclI._rclMesh),
  _rclFAry(rclI._rclFAry),
  _rclPAry(rclI._rclPAry),
  _clIter(rclI._clIter),
  _bApply(rclI._bApply),
  _clTrf(rclI._clTrf)
{
}

inline void MeshFacetIterator::Transform( const Base::Matrix4D& rclTrf )
{
  _clTrf = rclTrf;
  Base::Matrix4D tmp;
  // cecks for unit matrix
  _clTrf != tmp ? _bApply = true : _bApply = false;
}

inline const MeshGeomFacet& MeshFacetIterator::Dereference (void)
{
  MeshFacet rclF             = *_clIter;
  const unsigned long *paulPt        = &(_clIter->_aulPoints[0]);
  Base::Vector3f  *pclPt = _clFacet._aclPoints;
  *(pclPt++)       = _rclPAry[*(paulPt++)];
  *(pclPt++)       = _rclPAry[*(paulPt++)];
  *pclPt           = _rclPAry[*paulPt];
  _clFacet._ulProp = rclF._ulProp;
  _clFacet._ucFlag = rclF._ucFlag;
  _clFacet.NormalInvalid();
  if ( _bApply )
  {
    _clFacet._aclPoints[0] = _clTrf * _clFacet._aclPoints[0];
    _clFacet._aclPoints[1] = _clTrf * _clFacet._aclPoints[1];
    _clFacet._aclPoints[2] = _clTrf * _clFacet._aclPoints[2];
  }
  return _clFacet;
}

inline bool MeshFacetIterator::Set (unsigned long ulIndex)
{
  if (ulIndex < _rclFAry.size())
  {
    _clIter    = _rclFAry.begin() + ulIndex;
    return true;
  }
  else
  {
    _clIter    = _rclFAry.end();
    return false;
  }
}

inline MeshFacetIterator& MeshFacetIterator::operator = (const MeshFacetIterator &rpI)
{
  _clIter  = rpI._clIter;
  _bApply = rpI._bApply;
  _clTrf = rpI._clTrf;
  return *this;
}

inline unsigned long MeshFacetIterator::GetProperty (void) const
{
  return _clIter->_ulProp;
}

inline void MeshFacetIterator::GetNeighbours (MeshFacetIterator &rclN0, MeshFacetIterator &rclN1, MeshFacetIterator &rclN2) const
{
  if (_clIter->_aulNeighbours[0] != ULONG_MAX)
    rclN0.Set(_clIter->_aulNeighbours[0]);
  else
    rclN0.End();

  if (_clIter->_aulNeighbours[1] != ULONG_MAX)
    rclN1.Set(_clIter->_aulNeighbours[1]);
  else
    rclN1.End();

  if (_clIter->_aulNeighbours[2] != ULONG_MAX)
    rclN2.Set(_clIter->_aulNeighbours[2]);
  else
    rclN2.End();
}

inline void MeshFacetIterator::SetToNeighbour (unsigned short usN)
{ 
  if (_clIter->_aulNeighbours[usN] != ULONG_MAX)
    _clIter = _rclFAry.begin() + _clIter->_aulNeighbours[usN];
  else
    End();
}

inline MeshPointIterator::MeshPointIterator (const MeshKernel &rclM)
: _rclMesh(rclM), _rclPAry(_rclMesh._aclPointArray), _bApply(false)
{
  _clIter = _rclPAry.begin();
}

inline MeshPointIterator::MeshPointIterator (const MeshKernel &rclM, unsigned long ulPos)
: _rclMesh(rclM), _rclPAry(_rclMesh._aclPointArray), _bApply(false)
{
  _clIter = _rclPAry.begin() + ulPos;
}

inline MeshPointIterator::MeshPointIterator (const MeshPointIterator &rclI)
: _rclMesh(rclI._rclMesh), _rclPAry(rclI._rclPAry), _clIter(rclI._clIter), _bApply(rclI._bApply), _clTrf(rclI._clTrf)
{
}

inline void MeshPointIterator::Transform( const Base::Matrix4D& rclTrf )
{
  _clTrf = rclTrf;
  Base::Matrix4D tmp;
  // cecks for unit matrix
  _clTrf != tmp ? _bApply = true : _bApply = false;
}

inline const MeshPoint& MeshPointIterator::Dereference (void) const
{ // We change only the value of the point but not the actual iterator
  const_cast<MeshPointIterator*>(this)->_clPoint = *_clIter;
  if ( _bApply )
    const_cast<MeshPointIterator*>(this)->_clPoint = _clTrf * _clPoint;
  return _clPoint; 
}

inline bool MeshPointIterator::Set (unsigned long ulIndex)
{
  if (ulIndex < _rclPAry.size())
  {
    _clIter = _rclPAry.begin() + ulIndex;
    return true;
  }
  else
  {
    _clIter = _rclPAry.end();
    return false;
  }
}

inline MeshPointIterator& MeshPointIterator::operator = (const MeshPointIterator &rpI)
{
  _clIter  = rpI._clIter;
  _bApply = rpI._bApply;
  _clTrf = rpI._clTrf;
  return *this;
}


} // namespace MeshCore


#endif // MESH_ITERATOR_H 

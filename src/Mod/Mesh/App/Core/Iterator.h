// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once


#include <Base/Matrix.h>

#include "MeshKernel.h"


namespace MeshCore
{

class MeshKernel;
class MeshGeomFacet;
class MeshPoint;
class MeshGeomEdge;
class MeshIndexEdge;
class MeshHelpEdge;

/**
 * The MeshFacetIterator allows one to iterate over the facets that
 * hold the topology of the mesh and provides access to their
 * geometric information.
 * \note This class is not thread-safe.
 */
class MeshFacetIterator
{
public:
    /** @name Construction */
    //@{
    /// construction
    inline explicit MeshFacetIterator(const MeshKernel& rclM);
    /// construction
    inline MeshFacetIterator(const MeshKernel& rclM, FacetIndex ulPos);
    /// construction
    inline MeshFacetIterator(const MeshFacetIterator& rclI);
    inline MeshFacetIterator(MeshFacetIterator&& rclI);
    ~MeshFacetIterator() = default;
    //@}

    /** @name Transformation */
    //@{
    /// Transforms the returned facet points with the current transformation
    inline void Transform(const Base::Matrix4D& rclTrf);
    //@}

    /** @name Access methods */
    //@{
    /// Access to the element the iterator points to.
    const MeshGeomFacet& operator*()
    {
        return Dereference();
    }
    /// Access to the element the iterator points to.
    const MeshGeomFacet* operator->()
    {
        return &Dereference();
    }
    /// Increments the iterator. It points then to the next element if the
    /// end is not reached.
    const MeshFacetIterator& operator++()
    {
        ++_clIter;
        return *this;
    }
    /// Decrements the iterator. It points then to the previous element if the beginning
    /// is not reached.
    const MeshFacetIterator& operator--()
    {
        --_clIter;
        return *this;
    }
    /// Increments the iterator by \a k positions.
    const MeshFacetIterator& operator+=(int k)
    {
        _clIter += k;
        return *this;
    }
    /// Decrements the iterator by \a k positions.
    const MeshFacetIterator& operator-=(int k)
    {
        _clIter -= k;
        return *this;
    }
    /// Assignment.
    inline MeshFacetIterator& operator=(const MeshFacetIterator& rpI);
    inline MeshFacetIterator& operator=(MeshFacetIterator&& rpI);
    /// Compares if this iterator points to a lower element than the other one.
    bool operator<(const MeshFacetIterator& rclI) const
    {
        return _clIter < rclI._clIter;
    }
    /// Compares if this iterator points to a higher element than the other one.
    bool operator>(const MeshFacetIterator& rclI) const
    {
        return _clIter > rclI._clIter;
    }
    /// Checks if the iterators points to the same element.
    bool operator==(const MeshFacetIterator& rclI) const
    {
        return _clIter == rclI._clIter;
    }
    /// Sets the iterator to the beginning of the array.
    void Begin()
    {
        _clIter = _rclFAry.begin();
    }
    /// Sets the iterator to the end of the array.
    void End()
    {
        _clIter = _rclFAry.end();
    }
    /// Returns the current position of the iterator in the array.
    FacetIndex Position() const
    {
        return _clIter - _rclFAry.begin();
    }
    /// Checks if the end is already reached.
    bool EndReached() const
    {
        return !(_clIter < _rclFAry.end());
    }
    /// Sets the iterator to the beginning of the array.
    void Init()
    {
        Begin();
    }
    /// Checks if the end is not yet reached.
    bool More() const
    {
        return !EndReached();
    }
    /// Increments the iterator.
    void Next()
    {
        operator++();
    }
    /// Sets the iterator to a given position.
    inline bool Set(FacetIndex ulIndex);
    /// Returns the topologic facet.
    inline MeshFacet GetIndices() const
    {
        return *_clIter;
    }
    /// Returns the topologic facet.
    inline const MeshFacet& GetReference() const
    {
        return *_clIter;
    }
    /// Returns iterators pointing to the current facet's neighbours.
    inline void GetNeighbours(
        MeshFacetIterator& rclN0,
        MeshFacetIterator& rclN1,
        MeshFacetIterator& rclN2
    ) const;
    /// Sets the iterator to the current facet's neighbour of the side \a usN.
    inline void SetToNeighbour(unsigned short usN);
    /// Returns the property information to the current facet.
    inline unsigned long GetProperty() const;
    /// Checks if the iterator points to a valid element inside the array.
    inline bool IsValid() const
    {
        return (_clIter >= _rclFAry.begin()) && (_clIter < _rclFAry.end());
    }
    //@}
    /** @name Flag state
     */
    //@{
    void SetFlag(MeshFacet::TFlagType tF) const
    {
        this->_clIter->SetFlag(tF);
    }
    void ResetFlag(MeshFacet::TFlagType tF) const
    {
        this->_clIter->ResetFlag(tF);
    }
    bool IsFlag(MeshFacet::TFlagType tF) const
    {
        return this->_clIter->IsFlag(tF);
    }
    void SetProperty(unsigned long uP) const
    {
        this->_clIter->SetProperty(uP);
    }
    //@}

protected:
    inline const MeshGeomFacet& Dereference();

private:
    const MeshKernel& _rclMesh;
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
 * The MeshPointIterator allows one to iterate over the vertices of the mesh and provides access to
 * their geometric information. \note This class is not thread-safe.
 */
class MeshExport MeshPointIterator
{
public:
    /** @name Construction */
    //@{
    inline explicit MeshPointIterator(const MeshKernel& rclM);
    inline MeshPointIterator(const MeshKernel& rclM, PointIndex ulPos);
    inline MeshPointIterator(const MeshPointIterator& rclI);
    inline MeshPointIterator(MeshPointIterator&& rclI);
    ~MeshPointIterator() = default;
    //@}

    /** @name Transformation */
    //@{
    /// Transforms the returned points with the current transformation
    inline void Transform(const Base::Matrix4D& rclTrf);
    //@}

    /** @name Access methods */
    //@{
    /// Access to the element the iterator points to.
    const MeshPoint& operator*() const
    {
        return Dereference();
    }
    /// Access to the element the iterator points to.
    const MeshPoint* operator->() const
    {
        return &Dereference();
    }
    /// Increments the iterator. It points then to the next element if the
    /// end is not reached.
    const MeshPointIterator& operator++()
    {
        ++_clIter;
        return *this;
    }
    /// Decrements the iterator. It points then to the previous element if the beginning
    /// is not reached.
    const MeshPointIterator& operator--()
    {
        --_clIter;
        return *this;
    }
    /// Assignment.
    inline MeshPointIterator& operator=(const MeshPointIterator& rpI);
    inline MeshPointIterator& operator=(MeshPointIterator&& rpI);
    /// Compares if this iterator points to a lower element than the other one.
    bool operator<(const MeshPointIterator& rclI) const
    {
        return _clIter < rclI._clIter;
    }
    /// Compares if this iterator points to a higher element than the other one.
    bool operator>(const MeshPointIterator& rclI) const
    {
        return _clIter > rclI._clIter;
    }
    /// Checks if the iterators points to the same element.
    bool operator==(const MeshPointIterator& rclI) const
    {
        return _clIter == rclI._clIter;
    }
    /// Sets the iterator to the beginning of the array.
    void Begin()
    {
        _clIter = _rclPAry.begin();
    }
    /// Sets the iterator to the end of the array.
    void End()
    {
        _clIter = _rclPAry.end();
    }
    /// Returns the current position of the iterator in the array.
    PointIndex Position() const
    {
        return _clIter - _rclPAry.begin();
    }
    /// Checks if the end is already reached.
    bool EndReached() const
    {
        return !(_clIter < _rclPAry.end());
    }
    /// Sets the iterator to the beginning of the array.
    void Init()
    {
        Begin();
    }
    /// Checks if the end is not yet reached.
    bool More() const
    {
        return !EndReached();
    }
    /// Increments the iterator.
    void Next()
    {
        operator++();
    }
    /// Sets the iterator to a given position.
    inline bool Set(PointIndex ulIndex);
    /// Checks if the iterator points to a valid element inside the array.
    inline bool IsValid() const
    {
        return (_clIter >= _rclPAry.begin()) && (_clIter < _rclPAry.end());
    }
    //@}
    /** @name Flag state
     */
    //@{
    void SetFlag(MeshPoint::TFlagType tF) const
    {
        this->_clIter->SetFlag(tF);
    }
    void ResetFlag(MeshPoint::TFlagType tF) const
    {
        this->_clIter->ResetFlag(tF);
    }
    bool IsFlag(MeshPoint::TFlagType tF) const
    {
        return this->_clIter->IsFlag(tF);
    }
    void SetProperty(unsigned long uP) const
    {
        this->_clIter->SetProperty(uP);
    }
    //@}

private:
    inline const MeshPoint& Dereference() const;

private:
    const MeshKernel& _rclMesh;
    const MeshPointArray& _rclPAry;
    mutable MeshPoint _clPoint;
    MeshPointArray::_TConstIterator _clIter;
    bool _bApply;
    Base::Matrix4D _clTrf;

    // friends
    friend class MeshKernel;
};

class MeshFastFacetIterator
{
public:
    inline explicit MeshFastFacetIterator(const MeshKernel& rclM);
    virtual ~MeshFastFacetIterator() = default;

    void Init()
    {
        _clIter = _rclFAry.begin();
    }
    inline void Next();
    bool More()
    {
        return _clIter != _rclFAry.end();
    }

    Base::Vector3f _afPoints[3];  // NOLINT

private:
    const MeshFacetArray& _rclFAry;
    const MeshPointArray& _rclPAry;
    MeshFacetArray::_TConstIterator _clIter;

public:
    MeshFastFacetIterator(const MeshFastFacetIterator&) = delete;
    MeshFastFacetIterator(MeshFastFacetIterator&&) = delete;
    void operator=(const MeshFastFacetIterator&) = delete;
    void operator=(MeshFastFacetIterator&&) = delete;
};

inline MeshFastFacetIterator::MeshFastFacetIterator(const MeshKernel& rclM)
    : _rclFAry(rclM._aclFacetArray)
    , _rclPAry(rclM._aclPointArray)
    , _clIter(_rclFAry.begin())
{}

inline void MeshFastFacetIterator::Next()
{
    const PointIndex* paulPt = _clIter->_aulPoints;
    Base::Vector3f* pfPt = _afPoints;
    *(pfPt++) = _rclPAry[*(paulPt++)];
    *(pfPt++) = _rclPAry[*(paulPt++)];
    *pfPt = _rclPAry[*paulPt];
}

inline MeshFacetIterator::MeshFacetIterator(const MeshKernel& rclM)
    : _rclMesh(rclM)
    , _rclFAry(rclM._aclFacetArray)
    , _rclPAry(rclM._aclPointArray)
    , _clIter(rclM._aclFacetArray.begin())
    , _bApply(false)
{}

inline MeshFacetIterator::MeshFacetIterator(const MeshKernel& rclM, FacetIndex ulPos)
    : _rclMesh(rclM)
    , _rclFAry(rclM._aclFacetArray)
    , _rclPAry(rclM._aclPointArray)
    , _clIter(rclM._aclFacetArray.begin() + ulPos)
    , _bApply(false)
{}

inline MeshFacetIterator::MeshFacetIterator(const MeshFacetIterator& rclI)
    : _rclMesh(rclI._rclMesh)
    , _rclFAry(rclI._rclFAry)
    , _rclPAry(rclI._rclPAry)
    , _clIter(rclI._clIter)
    , _bApply(rclI._bApply)
    , _clTrf(rclI._clTrf)
{}

inline MeshFacetIterator::MeshFacetIterator(MeshFacetIterator&& rclI)
    : _rclMesh(rclI._rclMesh)
    , _rclFAry(rclI._rclFAry)
    , _rclPAry(rclI._rclPAry)
    , _clIter(rclI._clIter)
    , _bApply(rclI._bApply)
    , _clTrf(rclI._clTrf)
{}

inline void MeshFacetIterator::Transform(const Base::Matrix4D& rclTrf)
{
    _clTrf = rclTrf;
    Base::Matrix4D tmp;
    // checks for unit matrix
    _clTrf != tmp ? _bApply = true : _bApply = false;
}

inline const MeshGeomFacet& MeshFacetIterator::Dereference()
{
    MeshFacet rclF = *_clIter;
    const PointIndex* paulPt = &(_clIter->_aulPoints[0]);
    Base::Vector3f* pclPt = _clFacet._aclPoints;
    *(pclPt++) = _rclPAry[*(paulPt++)];
    *(pclPt++) = _rclPAry[*(paulPt++)];
    *pclPt = _rclPAry[*paulPt];
    _clFacet._ulProp = rclF._ulProp;
    _clFacet._ucFlag = rclF._ucFlag;
    _clFacet.NormalInvalid();
    if (_bApply) {
        _clFacet._aclPoints[0] = _clTrf * _clFacet._aclPoints[0];
        _clFacet._aclPoints[1] = _clTrf * _clFacet._aclPoints[1];
        _clFacet._aclPoints[2] = _clTrf * _clFacet._aclPoints[2];
    }
    return _clFacet;
}

inline bool MeshFacetIterator::Set(FacetIndex ulIndex)
{
    if (ulIndex < _rclFAry.size()) {
        _clIter = _rclFAry.begin() + ulIndex;
        return true;
    }

    _clIter = _rclFAry.end();
    return false;
}

inline MeshFacetIterator& MeshFacetIterator::operator=(const MeshFacetIterator& rpI)
{
    _clIter = rpI._clIter;
    _bApply = rpI._bApply;
    _clTrf = rpI._clTrf;
    return *this;
}

inline MeshFacetIterator& MeshFacetIterator::operator=(MeshFacetIterator&& rpI)
{
    _clIter = rpI._clIter;
    _bApply = rpI._bApply;
    _clTrf = rpI._clTrf;
    return *this;
}

inline unsigned long MeshFacetIterator::GetProperty() const
{
    return _clIter->_ulProp;
}

inline void MeshFacetIterator::GetNeighbours(
    MeshFacetIterator& rclN0,
    MeshFacetIterator& rclN1,
    MeshFacetIterator& rclN2
) const
{
    if (_clIter->_aulNeighbours[0] != FACET_INDEX_MAX) {
        rclN0.Set(_clIter->_aulNeighbours[0]);
    }
    else {
        rclN0.End();
    }

    if (_clIter->_aulNeighbours[1] != FACET_INDEX_MAX) {
        rclN1.Set(_clIter->_aulNeighbours[1]);
    }
    else {
        rclN1.End();
    }

    if (_clIter->_aulNeighbours[2] != FACET_INDEX_MAX) {
        rclN2.Set(_clIter->_aulNeighbours[2]);
    }
    else {
        rclN2.End();
    }
}

inline void MeshFacetIterator::SetToNeighbour(unsigned short usN)
{
    if (_clIter->_aulNeighbours[usN] != FACET_INDEX_MAX) {
        _clIter = _rclFAry.begin() + _clIter->_aulNeighbours[usN];
    }
    else {
        End();
    }
}

inline MeshPointIterator::MeshPointIterator(const MeshKernel& rclM)
    : _rclMesh(rclM)
    , _rclPAry(_rclMesh._aclPointArray)
    , _bApply(false)
{
    _clIter = _rclPAry.begin();
}

inline MeshPointIterator::MeshPointIterator(const MeshKernel& rclM, PointIndex ulPos)
    : _rclMesh(rclM)
    , _rclPAry(_rclMesh._aclPointArray)
    , _bApply(false)
{
    _clIter = _rclPAry.begin() + ulPos;
}

inline MeshPointIterator::MeshPointIterator(const MeshPointIterator& rclI)
    : _rclMesh(rclI._rclMesh)
    , _rclPAry(rclI._rclPAry)
    , _clIter(rclI._clIter)
    , _bApply(rclI._bApply)
    , _clTrf(rclI._clTrf)
{}

inline MeshPointIterator::MeshPointIterator(MeshPointIterator&& rclI)
    : _rclMesh(rclI._rclMesh)
    , _rclPAry(rclI._rclPAry)
    , _clIter(rclI._clIter)
    , _bApply(rclI._bApply)
    , _clTrf(rclI._clTrf)
{}

inline void MeshPointIterator::Transform(const Base::Matrix4D& rclTrf)
{
    _clTrf = rclTrf;
    Base::Matrix4D tmp;
    // checks for unit matrix
    _clTrf != tmp ? _bApply = true : _bApply = false;
}

inline const MeshPoint& MeshPointIterator::Dereference() const
{
    // We change only the value of the point but not the actual iterator
    _clPoint = *_clIter;
    if (_bApply) {
        _clPoint = _clTrf * _clPoint;
    }
    return _clPoint;
}

inline bool MeshPointIterator::Set(PointIndex ulIndex)
{
    if (ulIndex < _rclPAry.size()) {
        _clIter = _rclPAry.begin() + ulIndex;
        return true;
    }

    _clIter = _rclPAry.end();
    return false;
}

inline MeshPointIterator& MeshPointIterator::operator=(const MeshPointIterator& rpI)
{
    _clIter = rpI._clIter;
    _bApply = rpI._bApply;
    _clTrf = rpI._clTrf;
    return *this;
}

inline MeshPointIterator& MeshPointIterator::operator=(MeshPointIterator&& rpI)
{
    _clIter = rpI._clIter;
    _bApply = rpI._bApply;
    _clTrf = rpI._clTrf;
    return *this;
}


}  // namespace MeshCore

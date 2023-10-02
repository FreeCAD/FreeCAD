/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <algorithm>
#include <sstream>
#endif

#include "Mesh.h"
#include "MeshPy.h"
#include "Segment.h"


using namespace Mesh;

Segment::Segment(const MeshObject* mesh, bool mod)
    : _mesh(mesh)
    , _save(false)
    , _modifykernel(mod)
{}

Segment::Segment(const MeshObject* mesh, const std::vector<FacetIndex>& inds, bool mod)
    : _mesh(mesh)
    , _indices(inds)
    , _save(false)
    , _modifykernel(mod)
{
    if (_modifykernel) {
        _mesh->updateMesh(inds);
    }
}

void Segment::addIndices(const std::vector<FacetIndex>& inds)
{
    _indices.insert(_indices.end(), inds.begin(), inds.end());
    std::sort(_indices.begin(), _indices.end());
    _indices.erase(std::unique(_indices.begin(), _indices.end()), _indices.end());
    if (_modifykernel) {
        _mesh->updateMesh(inds);
    }
}

void Segment::removeIndices(const std::vector<FacetIndex>& inds)
{
    // make difference
    std::vector<FacetIndex> result;
    std::set<FacetIndex> s1(_indices.begin(), _indices.end());
    std::set<FacetIndex> s2(inds.begin(), inds.end());
    std::set_difference(s1.begin(),
                        s1.end(),
                        s2.begin(),
                        s2.end(),
                        std::back_insert_iterator<std::vector<FacetIndex>>(result));

    _indices = result;
    if (_modifykernel) {
        _mesh->updateMesh();
    }
}

const std::vector<FacetIndex>& Segment::getIndices() const
{
    return _indices;
}

Segment::Segment(const Segment& s) = default;

Segment::Segment(Segment&& s) = default;

Segment& Segment::operator=(const Segment& s)
{
    // Do not copy the MeshObject pointer
    if (this != &s) {
        this->_indices = s._indices;
    }
    if (_modifykernel) {
        _mesh->updateMesh();
    }
    return *this;
}

Segment& Segment::operator=(Segment&& s)
{
    // Do not copy the MeshObject pointer
    if (this != &s) {
        this->_indices = s._indices;
    }
    if (_modifykernel) {
        _mesh->updateMesh();
    }
    return *this;
}

bool Segment::operator==(const Segment& s) const
{
    return this->_indices == s._indices;
}

// ----------------------------------------------------------------------------

// clang-format off
Segment::const_facet_iterator::const_facet_iterator
(const Segment* segm, std::vector<FacetIndex>::const_iterator it)
  : _segment(segm), _f_it(segm->_mesh->getKernel()), _it(it)
{
    this->_f_it.Set(0);
    this->_f_it.Transform(_segment->_mesh->getTransform());
    this->_facet.Mesh = _segment->_mesh;
}

Segment::const_facet_iterator::const_facet_iterator
(const Segment::const_facet_iterator& fi) = default;

Segment::const_facet_iterator::const_facet_iterator
(Segment::const_facet_iterator&& fi) = default;

Segment::const_facet_iterator::~const_facet_iterator() = default;

Segment::const_facet_iterator& Segment::const_facet_iterator::operator=
(const Segment::const_facet_iterator& fi) = default;

Segment::const_facet_iterator& Segment::const_facet_iterator::operator=
(Segment::const_facet_iterator&& fi) = default;

void Segment::const_facet_iterator::dereference() const
{
    this->_f_it.Set(*_it);
    this->_facet.MeshCore::MeshGeomFacet::operator = (*_f_it);
    this->_facet.Index = *_it;
    const MeshCore::MeshFacet& face = _f_it.GetReference();
    for (int i=0; i<3;i++) {
        this->_facet.PIndex[i] = face._aulPoints[i];
        this->_facet.NIndex[i] = face._aulNeighbours[i];
    }
}

const Facet& Segment::const_facet_iterator::operator*() const
{
    this->dereference();
    return this->_facet;
}

const Facet* Segment::const_facet_iterator::operator->() const
{
    this->dereference();
    return &(this->_facet);
}

bool Segment::const_facet_iterator::operator==(const Segment::const_facet_iterator& fi) const
{
    return (this->_segment == fi._segment) && (this->_it == fi._it);
}

bool Segment::const_facet_iterator::operator!=(const Segment::const_facet_iterator& fi) const
{
    return !operator==(fi);
}

Segment::const_facet_iterator& Segment::const_facet_iterator::operator++()
{
    ++(this->_it);
    return *this;
}

Segment::const_facet_iterator& Segment::const_facet_iterator::operator--()
{
    --(this->_it);
    return *this;
}
// clang-format on

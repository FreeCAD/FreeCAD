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

#ifndef MESH_SEGMENT_H
#define MESH_SEGMENT_H

#include <string>
#include <vector>

#include "Core/Iterator.h"

#include "Facet.h"
#include "Types.h"


namespace Mesh
{

class MeshObject;

class MeshExport Segment
{
public:
    Segment(const MeshObject*, bool mod);
    Segment(const MeshObject*, const std::vector<FacetIndex>& inds, bool mod);
    ~Segment() = default;
    void addIndices(const std::vector<FacetIndex>& inds);
    void removeIndices(const std::vector<FacetIndex>& inds);
    const std::vector<FacetIndex>& getIndices() const;
    bool isEmpty() const
    {
        return _indices.empty();
    }

    Segment(const Segment&);
    Segment(Segment&&);
    Segment& operator=(const Segment&);
    Segment& operator=(Segment&&);
    bool operator==(const Segment&) const;

    void setName(const std::string& n)
    {
        _name = n;
    }
    const std::string& getName() const
    {
        return _name;
    }

    void setColor(const std::string& c)
    {
        _color = c;
    }
    const std::string& getColor() const
    {
        return _color;
    }

    void save(bool on)
    {
        _save = on;
    }
    bool isSaved() const
    {
        return _save;
    }

    // friends
    friend class MeshObject;

private:
    const MeshObject* _mesh;
    std::vector<FacetIndex> _indices;
    std::string _name;
    std::string _color;
    bool _save;
    bool _modifykernel;

public:
    class MeshExport const_facet_iterator
    {
    public:
        const_facet_iterator(const Segment*, std::vector<FacetIndex>::const_iterator);
        const_facet_iterator(const const_facet_iterator& fi);
        const_facet_iterator(const_facet_iterator&& fi);
        ~const_facet_iterator();

        const_facet_iterator& operator=(const const_facet_iterator& fi);
        const_facet_iterator& operator=(const_facet_iterator&& fi);
        const Facet& operator*() const;
        const Facet* operator->() const;
        bool operator==(const const_facet_iterator& fi) const;
        bool operator!=(const const_facet_iterator& fi) const;
        const_facet_iterator& operator++();
        const_facet_iterator& operator--();

    private:
        void dereference() const;
        const Segment* _segment;
        mutable Facet _facet;
        mutable MeshCore::MeshFacetIterator _f_it;
        std::vector<FacetIndex>::const_iterator _it;
    };

    const_facet_iterator facets_begin() const
    {
        return {this, _indices.begin()};
    }
    const_facet_iterator facets_end() const
    {
        return {this, _indices.end()};
    }
};

}  // namespace Mesh


#endif  // MESH_SEGMENT_H

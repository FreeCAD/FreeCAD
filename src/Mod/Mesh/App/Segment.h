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

#include <vector>
#include <string>
#include "Facet.h"
#include "Core/Iterator.h"

namespace Mesh
{

class MeshObject;

class MeshExport Segment
{
public:
    Segment(MeshObject*, bool mod);
    Segment(MeshObject*, const std::vector<unsigned long>& inds, bool mod);
    void addIndices(const std::vector<unsigned long>& inds);
    void removeIndices(const std::vector<unsigned long>& inds);
    const std::vector<unsigned long>& getIndices() const;
    bool isEmpty() const { return _indices.empty(); }

    Segment(const Segment&);
    const Segment& operator = (const Segment&);
    bool operator == (const Segment&) const;

    void setName(const std::string& n) { _name = n; }
    const std::string& getName() const { return _name; }

    void setColor(const std::string& c) { _color = c; }
    const std::string& getColor() const { return _color; }

    void save(bool on) { _save = on; }
    bool isSaved() const { return _save; }

    // friends
    friend class MeshObject;

private:
    MeshObject* _mesh;
    std::vector<unsigned long> _indices;
    std::string _name;
    std::string _color;
    bool _save;
    bool _modifykernel;

public:
    class MeshExport const_facet_iterator
    {
    public:
        const_facet_iterator(const Segment*, std::vector<unsigned long>::const_iterator);
        const_facet_iterator(const const_facet_iterator& fi);
        ~const_facet_iterator();

        const_facet_iterator& operator=(const const_facet_iterator& fi);
        const Facet& operator*() const;
        const Facet* operator->() const;
        bool operator==(const const_facet_iterator& fi) const;
        bool operator!=(const const_facet_iterator& fi) const;
        const_facet_iterator& operator++();
        const_facet_iterator& operator--();
    private:
        void dereference();
        const Segment* _segment;
        Facet _facet;
        MeshCore::MeshFacetIterator _f_it;
        std::vector<unsigned long>::const_iterator _it;
    };

    const_facet_iterator facets_begin() const
    { return const_facet_iterator(this, _indices.begin()); }
    const_facet_iterator facets_end() const
    { return const_facet_iterator(this, _indices.end()); }
};

} // namespace Mesh


#endif // MESH_SEGMENT_H

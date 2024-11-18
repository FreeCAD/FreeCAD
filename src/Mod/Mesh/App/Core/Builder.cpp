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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <algorithm>
#endif

#include <Base/Exception.h>
#include <Base/Sequencer.h>

#include "Builder.h"
#include "Functional.h"
#include "MeshKernel.h"
#include <QVector>


using namespace MeshCore;


MeshBuilder::MeshBuilder(MeshKernel& kernel)
    : _meshKernel(kernel)
    , _fSaveTolerance {MeshDefinitions::_fMinPointDistanceD1}
{}

MeshBuilder::~MeshBuilder()
{
    MeshDefinitions::_fMinPointDistanceD1 = _fSaveTolerance;
    delete this->_seq;
}

void MeshBuilder::SetTolerance(float fTol)
{
    MeshDefinitions::_fMinPointDistanceD1 = fTol;
}

void MeshBuilder::Initialize(size_t ctFacets, bool deletion)
{
    if (deletion) {
        // Clear the mesh structure and free all memory
        _meshKernel.Clear();

        // Allocate new memory that is needed later on. If AddFacet() gets called exactly ctFacets
        // times there is no wastage of memory otherwise the vector reallocates ~50% of its future
        // memory usage. Note: A feature of the std::vector implementation is that it can hold more
        // memory (capacity) than it actually needs (size).
        //       This usually happens if its elements are added without specifying its final size.
        //       Later on it's a bit tricky to free the wasted memory. So we're strived to avoid the
        //       wastage of memory.
        _meshKernel._aclFacetArray.reserve(ctFacets);

        // Usually the number of vertices is the half of the number of facets. So we reserve this
        // memory with 10% surcharge To save memory we hold an array with iterators that point to
        // the right vertex (insertion order) in the set, instead of holding the vertex array twice.
        size_t ctPoints = ctFacets / 2;
        _pointsIterator.reserve(static_cast<size_t>(float(ctPoints) * 1.10f));
        _ptIdx = 0;
    }
    else {
        for (const auto& it1 : _meshKernel._aclPointArray) {
            MeshPointIterator pit = _points.insert(it1);
            _pointsIterator.push_back(pit);
        }
        _ptIdx = _points.size();

        // As we have a copy of our vertices in the set we must clear them from our array now  But
        // we can keep its memory as we reuse it later on anyway.
        _meshKernel._aclPointArray.clear();
        // additional memory
        size_t newCtFacets = _meshKernel._aclFacetArray.size() + ctFacets;
        _meshKernel._aclFacetArray.reserve(newCtFacets);
        size_t ctPoints = newCtFacets / 2;
        _pointsIterator.reserve(static_cast<size_t>(float(ctPoints) * 1.10f));
    }

    this->_seq = new Base::SequencerLauncher("create mesh structure...", ctFacets * 2);
}

void MeshBuilder::AddFacet(const MeshGeomFacet& facet, bool takeFlag, bool takeProperty)
{
    unsigned char flag = 0;
    unsigned long prop = 0;
    if (takeFlag) {
        flag = facet._ucFlag;
    }
    if (takeProperty) {
        prop = facet._ulProp;
    }

    AddFacet(facet._aclPoints[0],
             facet._aclPoints[1],
             facet._aclPoints[2],
             facet.GetNormal(),
             flag,
             prop);
}

void MeshBuilder::AddFacet(const Base::Vector3f& pt1,
                           const Base::Vector3f& pt2,
                           const Base::Vector3f& pt3,
                           const Base::Vector3f& normal,
                           unsigned char flag,
                           unsigned long prop)
{
    Base::Vector3f facetPoints[4] = {pt1, pt2, pt3, normal};
    AddFacet(facetPoints, flag, prop);
}

void MeshBuilder::AddFacet(Base::Vector3f* facetPoints, unsigned char flag, unsigned long prop)
{
    this->_seq->next(true);  // allow to cancel

    // adjust circulation direction
    if ((((facetPoints[1] - facetPoints[0]) % (facetPoints[2] - facetPoints[0])) * facetPoints[3])
        < 0.0f) {
        std::swap(facetPoints[1], facetPoints[2]);
    }

    MeshFacet mf;
    mf._ucFlag = flag;
    mf._ulProp = prop;

    int i = 0;
    for (i = 0; i < 3; i++) {
        MeshPoint pt(facetPoints[i]);
        std::set<MeshPoint>::iterator p = _points.find(pt);
        if (p == _points.end()) {
            mf._aulPoints[i] = _ptIdx;
            pt._ulProp = _ptIdx++;
            // keep an iterator to the right vertex
            MeshPointIterator it = _points.insert(pt);
            _pointsIterator.push_back(it);
        }
        else {
            mf._aulPoints[i] = p->_ulProp;
        }
    }

    // check for degenerated facet (one edge has length 0)
    if ((mf._aulPoints[0] == mf._aulPoints[1]) || (mf._aulPoints[0] == mf._aulPoints[2])
        || (mf._aulPoints[1] == mf._aulPoints[2])) {
        return;
    }

    _meshKernel._aclFacetArray.push_back(mf);
}

void MeshBuilder::SetNeighbourhood()
{
    std::set<Edge> edges;
    FacetIndex facetIdx = 0;

    for (auto& mf : _meshKernel._aclFacetArray) {
        this->_seq->next(true);  // allow to cancel
        for (int i = 0; i < 3; i++) {
            Edge edge(mf._aulPoints[i], mf._aulPoints[(i + 1) % 3], facetIdx);
            std::set<Edge>::iterator e = edges.find(edge);
            if (e != edges.end()) {  // edge exists, set neighbourhood
                MeshFacet& mf1 = _meshKernel._aclFacetArray[e->facetIdx];
                if (mf1._aulPoints[0] == edge.pt1) {
                    if (mf1._aulPoints[1] == edge.pt2) {
                        mf1._aulNeighbours[0] = facetIdx;
                    }
                    else {
                        mf1._aulNeighbours[2] = facetIdx;
                    }
                }
                else if (mf1._aulPoints[0] == edge.pt2) {
                    if (mf1._aulPoints[1] == edge.pt1) {
                        mf1._aulNeighbours[0] = facetIdx;
                    }
                    else {
                        mf1._aulNeighbours[2] = facetIdx;
                    }
                }
                else {
                    mf1._aulNeighbours[1] = facetIdx;
                }

                mf._aulNeighbours[i] = e->facetIdx;
            }
            else {  // new edge
                edges.insert(edge);
            }
        }

        facetIdx++;
    }
}

void MeshBuilder::RemoveUnreferencedPoints()
{
    _meshKernel._aclPointArray.SetFlag(MeshPoint::INVALID);
    for (const auto& it : _meshKernel._aclFacetArray) {
        for (PointIndex point : it._aulPoints) {
            _meshKernel._aclPointArray[point].ResetInvalid();
        }
    }

    unsigned long uValidPts = std::count_if(_meshKernel._aclPointArray.begin(),
                                            _meshKernel._aclPointArray.end(),
                                            [](const MeshPoint& p) {
                                                return p.IsValid();
                                            });
    if (uValidPts < _meshKernel.CountPoints()) {
        _meshKernel.RemoveInvalids();
    }
}

void MeshBuilder::Finish(bool freeMemory)
{
    // now we can resize the vertex array to the exact size and copy the vertices with their correct
    // positions in the array
    PointIndex i = 0;
    _meshKernel._aclPointArray.resize(_pointsIterator.size());
    for (const auto& it : _pointsIterator) {
        _meshKernel._aclPointArray[i++] = *(it.first);
    }

    // free all memory of the internal structures
    // Note: this scope is needed to free memory immediately
#if defined(_MSC_VER) && defined(_DEBUG)
    // Just do nothing here as it may take a long time when running the debugger
#else
    {
        std::vector<MeshPointIterator>().swap(_pointsIterator);
    }
#endif
    _points.clear();

    SetNeighbourhood();
    RemoveUnreferencedPoints();

    // if AddFacet() has been called more often (or even less) as specified in Initialize() we have
    // a wastage of memory
    if (freeMemory) {
        size_t cap = _meshKernel._aclFacetArray.capacity();
        size_t siz = _meshKernel._aclFacetArray.size();
        // wastage of more than 5%
        if (cap > siz + siz / 20) {
            try {
                FacetIndex i = 0;
                MeshFacetArray faces(siz);
                for (const auto& it : _meshKernel._aclFacetArray) {
                    faces[i++] = it;
                }
                _meshKernel._aclFacetArray.swap(faces);
            }
            catch (const Base::MemoryException&) {
                // sorry, we cannot reduce the memory
            }
        }
    }

    _meshKernel.RecalcBoundBox();
}

// ----------------------------------------------------------------------------

struct MeshFastBuilder::Private
{
    struct Vertex
    {
        Vertex()
            : x(0)
            , y(0)
            , z(0)
            , i(0)
        {}
        Vertex(float x, float y, float z)
            : x(x)
            , y(y)
            , z(z)
            , i(0)
        {}

        float x, y, z;
        size_type i;

        bool operator!=(const Vertex& rhs) const
        {
            return x != rhs.x || y != rhs.y || z != rhs.z;
        }
        bool operator<(const Vertex& rhs) const
        {
            if (x != rhs.x) {
                return x < rhs.x;
            }
            else if (y != rhs.y) {
                return y < rhs.y;
            }
            else if (z != rhs.z) {
                return z < rhs.z;
            }
            else {
                return false;
            }
        }
    };

    // Hint: Using a QVector instead of std::vector is a bit faster
    QVector<Vertex> verts;
};

MeshFastBuilder::MeshFastBuilder(MeshKernel& rclM)
    : _meshKernel(rclM)
    , p(new Private)
{}

MeshFastBuilder::~MeshFastBuilder()
{
    delete p;
}

void MeshFastBuilder::Initialize(size_type ctFacets)
{
    p->verts.reserve(ctFacets * 3);
}

void MeshFastBuilder::AddFacet(const Base::Vector3f* facetPoints)
{
    Private::Vertex v;
    for (int i = 0; i < 3; i++) {
        v.x = facetPoints[i].x;
        v.y = facetPoints[i].y;
        v.z = facetPoints[i].z;
        p->verts.push_back(v);
    }
}

void MeshFastBuilder::AddFacet(const MeshGeomFacet& facetPoints)
{
    Private::Vertex v;
    for (const auto& pnt : facetPoints._aclPoints) {
        v.x = pnt.x;
        v.y = pnt.y;
        v.z = pnt.z;
        p->verts.push_back(v);
    }
}

void MeshFastBuilder::Finish()
{
    using size_type = QVector<Private::Vertex>::size_type;
    QVector<Private::Vertex>& verts = p->verts;
    size_type ulCtPts = verts.size();
    for (size_type i = 0; i < ulCtPts; ++i) {
        verts[i].i = i;
    }

    // std::sort(verts.begin(), verts.end());
    int threads = int(std::thread::hardware_concurrency());
    MeshCore::parallel_sort(verts.begin(), verts.end(), std::less<>(), threads);

    QVector<FacetIndex> indices(ulCtPts);

    size_type vertex_count = 0;
    for (QVector<Private::Vertex>::iterator v = verts.begin(); v != verts.end(); ++v) {
        if (!vertex_count || *v != verts[vertex_count - 1]) {
            verts[vertex_count++] = *v;
        }

        indices[v->i] = static_cast<FacetIndex>(vertex_count - 1);
    }

    size_type ulCt = verts.size() / 3;
    MeshFacetArray rFacets(static_cast<FacetIndex>(ulCt));
    for (size_type i = 0; i < ulCt; ++i) {
        rFacets[static_cast<size_t>(i)]._aulPoints[0] = indices[3 * i];
        rFacets[static_cast<size_t>(i)]._aulPoints[1] = indices[3 * i + 1];
        rFacets[static_cast<size_t>(i)]._aulPoints[2] = indices[3 * i + 2];
    }

    verts.resize(vertex_count);

    MeshPointArray rPoints;
    rPoints.reserve(static_cast<size_t>(vertex_count));
    for (const auto& v : verts) {
        rPoints.push_back(MeshPoint(v.x, v.y, v.z));
    }

    _meshKernel.Adopt(rPoints, rFacets, true);
}

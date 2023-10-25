/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <queue>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Mod/Mesh/App/WildMagic4/Wm4Delaunay2.h>

#include "Approximation.h"
#include "MeshKernel.h"
#include "Triangulation.h"


using namespace MeshCore;


bool TriangulationVerifier::Accept(const Base::Vector3f& n,
                                   const Base::Vector3f& p1,
                                   const Base::Vector3f& p2,
                                   const Base::Vector3f& p3) const
{
    float ref_dist = (p2 - p1) * n;
    float tri_dist = (p3 - p1) * n;
    return (ref_dist * tri_dist <= 0.0f);
}

bool TriangulationVerifier::MustFlip(const Base::Vector3f& n1, const Base::Vector3f& n2) const
{
    return n1.Dot(n2) <= 0.0f;
}

bool TriangulationVerifierV2::Accept(const Base::Vector3f& n,
                                     const Base::Vector3f& p1,
                                     const Base::Vector3f& p2,
                                     const Base::Vector3f& p3) const
{
    float ref_dist = (p2 - p1) * n;
    float tri_dist = (p3 - p1) * n;
    float prod = ref_dist * tri_dist;
    (void)prod;
    return true;
}

bool TriangulationVerifierV2::MustFlip(const Base::Vector3f& n1, const Base::Vector3f& n2) const
{
    float dot = n1.Dot(n2);
    (void)dot;
    return false;
}

// ----------------------------------------------------------------------------

AbstractPolygonTriangulator::AbstractPolygonTriangulator()
    : _discard {false}
    , _verifier {new TriangulationVerifier()}
{}

AbstractPolygonTriangulator::~AbstractPolygonTriangulator()
{
    delete _verifier;
}

TriangulationVerifier* AbstractPolygonTriangulator::GetVerifier() const
{
    return _verifier;
}

void AbstractPolygonTriangulator::SetVerifier(TriangulationVerifier* v)
{
    delete _verifier;
    _verifier = v;
}

void AbstractPolygonTriangulator::SetPolygon(const std::vector<Base::Vector3f>& raclPoints)
{
    this->_points = raclPoints;
    if (!this->_points.empty()) {
        if (this->_points.front() == this->_points.back()) {
            this->_points.pop_back();
        }
    }
}

std::vector<Base::Vector3f> AbstractPolygonTriangulator::GetPolygon() const
{
    return _points;
}

float AbstractPolygonTriangulator::GetLength() const
{
    float len = 0.0f;
    if (_points.size() > 2) {
        for (std::vector<Base::Vector3f>::const_iterator it = _points.begin(); it != _points.end();
             ++it) {
            std::vector<Base::Vector3f>::const_iterator jt = it + 1;
            if (jt == _points.end()) {
                jt = _points.begin();
            }
            len += Base::Distance(*it, *jt);
        }
    }

    return len;
}

std::vector<Base::Vector3f> AbstractPolygonTriangulator::AddedPoints() const
{
    // Apply the inverse transformation to project back to world coordinates
    std::vector<Base::Vector3f> added;
    added.reserve(_newpoints.size());
    for (auto point : _newpoints) {
        added.push_back(_inverse * point);
    }
    return added;
}

Base::Matrix4D AbstractPolygonTriangulator::GetTransformToFitPlane() const
{
    PlaneFit planeFit;
    for (auto point : _points) {
        planeFit.AddPoint(point);
    }

    if (planeFit.Fit() >= FLOAT_MAX) {
        throw Base::RuntimeError("Plane fit failed");
    }

    Base::Vector3f bs = planeFit.GetBase();
    Base::Vector3f ex = planeFit.GetDirU();
    Base::Vector3f ey = planeFit.GetDirV();
    Base::Vector3f ez = planeFit.GetNormal();

    // build the matrix for the inverse transformation
    Base::Matrix4D rInverse;
    rInverse.setToUnity();
    rInverse[0][0] = static_cast<double>(ex.x);
    rInverse[0][1] = static_cast<double>(ey.x);
    rInverse[0][2] = static_cast<double>(ez.x);
    rInverse[0][3] = static_cast<double>(bs.x);

    rInverse[1][0] = static_cast<double>(ex.y);
    rInverse[1][1] = static_cast<double>(ey.y);
    rInverse[1][2] = static_cast<double>(ez.y);
    rInverse[1][3] = static_cast<double>(bs.y);

    rInverse[2][0] = static_cast<double>(ex.z);
    rInverse[2][1] = static_cast<double>(ey.z);
    rInverse[2][2] = static_cast<double>(ez.z);
    rInverse[2][3] = static_cast<double>(bs.z);

    return rInverse;
}

std::vector<Base::Vector3f> AbstractPolygonTriangulator::ProjectToFitPlane()
{
    std::vector<Base::Vector3f> proj = _points;
    _inverse = GetTransformToFitPlane();
    Base::Vector3f bs(static_cast<float>(_inverse[0][3]),
                      static_cast<float>(_inverse[1][3]),
                      static_cast<float>(_inverse[2][3]));
    Base::Vector3f ex(static_cast<float>(_inverse[0][0]),
                      static_cast<float>(_inverse[1][0]),
                      static_cast<float>(_inverse[2][0]));
    Base::Vector3f ey(static_cast<float>(_inverse[0][1]),
                      static_cast<float>(_inverse[1][1]),
                      static_cast<float>(_inverse[2][1]));
    for (auto& jt : proj) {
        jt.TransformToCoordinateSystem(bs, ex, ey);
    }
    return proj;
}

void AbstractPolygonTriangulator::PostProcessing(const std::vector<Base::Vector3f>& points)
{
    // For a good approximation we should have enough points, i.e. for 9 parameters
    // for the fit function we should have at least 50 points.
    unsigned int uMinPts = 50;

    PolynomialFit polyFit;
    Base::Vector3f bs(static_cast<float>(_inverse[0][3]),
                      static_cast<float>(_inverse[1][3]),
                      static_cast<float>(_inverse[2][3]));
    Base::Vector3f ex(static_cast<float>(_inverse[0][0]),
                      static_cast<float>(_inverse[1][0]),
                      static_cast<float>(_inverse[2][0]));
    Base::Vector3f ey(static_cast<float>(_inverse[0][1]),
                      static_cast<float>(_inverse[1][1]),
                      static_cast<float>(_inverse[2][1]));

    for (auto pt : points) {
        pt.TransformToCoordinateSystem(bs, ex, ey);
        polyFit.AddPoint(pt);
    }

    if (polyFit.CountPoints() >= uMinPts && polyFit.Fit() < FLOAT_MAX) {
        for (auto& newpoint : _newpoints) {
            newpoint.z = static_cast<float>(polyFit.Value(newpoint.x, newpoint.y));
        }
    }
}

MeshGeomFacet AbstractPolygonTriangulator::GetTriangle(const MeshPointArray& points,
                                                       const MeshFacet& facet) const
{
    MeshGeomFacet triangle;
    triangle._aclPoints[0] = points[facet._aulPoints[0]];
    triangle._aclPoints[1] = points[facet._aulPoints[1]];
    triangle._aclPoints[2] = points[facet._aulPoints[2]];
    return triangle;
}

bool AbstractPolygonTriangulator::TriangulatePolygon()
{
    try {
        if (!this->_indices.empty() && this->_points.size() != this->_indices.size()) {
            Base::Console().Log("Triangulation: %d points <> %d indices\n",
                                _points.size(),
                                _indices.size());
            return false;
        }
        bool ok = Triangulate();
        if (ok) {
            Done();
        }
        return ok;
    }
    catch (const Base::Exception& e) {
        Base::Console().Log("Triangulation: %s\n", e.what());
        return false;
    }
    catch (const std::exception& e) {
        Base::Console().Log("Triangulation: %s\n", e.what());
        return false;
    }
    catch (...) {
        return false;
    }
}

std::vector<PointIndex> AbstractPolygonTriangulator::GetInfo() const
{
    return _info;
}

void AbstractPolygonTriangulator::Discard()
{
    if (!_discard) {
        _discard = true;
        _info.pop_back();
    }
}

void AbstractPolygonTriangulator::Reset()
{}

void AbstractPolygonTriangulator::Done()
{
    _info.push_back(_points.size());
    _discard = false;
}

// -------------------------------------------------------------

EarClippingTriangulator::EarClippingTriangulator() = default;

bool EarClippingTriangulator::Triangulate()
{
    _facets.clear();
    _triangles.clear();

    std::vector<Base::Vector3f> pts = ProjectToFitPlane();
    std::vector<PointIndex> result;

    //  Invoke the triangulator to triangulate this polygon.
    Triangulate::Process(pts, result);

    // print out the results.
    size_t tcount = result.size() / 3;

    bool ok = tcount + 2 == _points.size();
    if (tcount > _points.size()) {
        return false;  // no valid triangulation
    }

    MeshGeomFacet clFacet;
    MeshFacet clTopFacet;
    for (size_t i = 0; i < tcount; i++) {
        if (Triangulate::_invert) {
            clFacet._aclPoints[0] = _points[result[i * 3 + 0]];
            clFacet._aclPoints[2] = _points[result[i * 3 + 1]];
            clFacet._aclPoints[1] = _points[result[i * 3 + 2]];
            clTopFacet._aulPoints[0] = result[i * 3 + 0];
            clTopFacet._aulPoints[2] = result[i * 3 + 1];
            clTopFacet._aulPoints[1] = result[i * 3 + 2];
        }
        else {
            clFacet._aclPoints[0] = _points[result[i * 3 + 0]];
            clFacet._aclPoints[1] = _points[result[i * 3 + 1]];
            clFacet._aclPoints[2] = _points[result[i * 3 + 2]];
            clTopFacet._aulPoints[0] = result[i * 3 + 0];
            clTopFacet._aulPoints[1] = result[i * 3 + 1];
            clTopFacet._aulPoints[2] = result[i * 3 + 2];
        }

        _triangles.push_back(clFacet);
        _facets.push_back(clTopFacet);
    }

    return ok;
}

float EarClippingTriangulator::Triangulate::Area(const std::vector<Base::Vector3f>& contour)
{
    int n = contour.size();

    float A = 0.0f;

    for (int p = n - 1, q = 0; q < n; p = q++) {
        A += contour[p].x * contour[q].y - contour[q].x * contour[p].y;
    }
    return A * 0.5f;
}

/*
  InsideTriangle decides if a point P is Inside of the triangle
  defined by A, B, C.
*/
bool EarClippingTriangulator::Triangulate::InsideTriangle(float Ax,
                                                          float Ay,
                                                          float Bx,
                                                          float By,
                                                          float Cx,
                                                          float Cy,
                                                          float Px,
                                                          float Py)
{
    float ax {}, ay {}, bx {}, by {}, cx {}, cy {}, apx {}, apy {}, bpx {}, bpy {}, cpx {}, cpy {};
    float cCROSSap {}, bCROSScp {}, aCROSSbp {};

    ax = Cx - Bx;
    ay = Cy - By;
    bx = Ax - Cx;
    by = Ay - Cy;
    cx = Bx - Ax;
    cy = By - Ay;
    apx = Px - Ax;
    apy = Py - Ay;
    bpx = Px - Bx;
    bpy = Py - By;
    cpx = Px - Cx;
    cpy = Py - Cy;

    aCROSSbp = ax * bpy - ay * bpx;
    cCROSSap = cx * apy - cy * apx;
    bCROSScp = bx * cpy - by * cpx;

    return ((aCROSSbp >= FLOAT_EPS) && (bCROSScp >= FLOAT_EPS) && (cCROSSap >= FLOAT_EPS));
}

bool EarClippingTriangulator::Triangulate::Snip(const std::vector<Base::Vector3f>& contour,
                                                int u,
                                                int v,
                                                int w,
                                                int n,
                                                int* V)
{
    int p {};
    float Ax {}, Ay {}, Bx {}, By {}, Cx {}, Cy {}, Px {}, Py {};

    Ax = contour[V[u]].x;
    Ay = contour[V[u]].y;

    Bx = contour[V[v]].x;
    By = contour[V[v]].y;

    Cx = contour[V[w]].x;
    Cy = contour[V[w]].y;

    if (FLOAT_EPS > (((Bx - Ax) * (Cy - Ay)) - ((By - Ay) * (Cx - Ax)))) {
        return false;
    }

    for (p = 0; p < n; p++) {
        if ((p == u) || (p == v) || (p == w)) {
            continue;
        }
        Px = contour[V[p]].x;
        Py = contour[V[p]].y;
        if (InsideTriangle(Ax, Ay, Bx, By, Cx, Cy, Px, Py)) {
            return false;
        }
    }

    return true;
}

bool EarClippingTriangulator::Triangulate::_invert = false;

bool EarClippingTriangulator::Triangulate::Process(const std::vector<Base::Vector3f>& contour,
                                                   std::vector<PointIndex>& result)
{
    /* allocate and initialize list of Vertices in polygon */

    int n = contour.size();
    if (n < 3) {
        return false;
    }

    int* V = new int[n];

    /* we want a counter-clockwise polygon in V */

    if (0.0f < Area(contour)) {
        for (int v = 0; v < n; v++) {
            V[v] = v;
        }
        _invert = true;
    }
    //    for(int v=0; v<n; v++) V[v] = (n-1)-v;
    else {
        for (int v = 0; v < n; v++) {
            V[v] = (n - 1) - v;
        }
        _invert = false;
    }

    int nv = n;

    /*  remove nv-2 Vertices, creating 1 triangle every time */
    int count = 2 * nv; /* error detection */

    for (int v = nv - 1; nv > 2;) {
        /* if we loop, it is probably a non-simple polygon */
        if (0 >= (count--)) {
            //** Triangulate: ERROR - probable bad polygon!
            delete[] V;
            return false;
        }

        /* three consecutive vertices in current polygon, <u,v,w> */
        int u = v;
        if (nv <= u) {
            u = 0; /* previous */
        }
        v = u + 1;
        if (nv <= v) {
            v = 0; /* new v    */
        }
        int w = v + 1;
        if (nv <= w) {
            w = 0; /* next     */
        }

        if (Snip(contour, u, v, w, nv, V)) {
            int a {}, b {}, c {}, s {}, t {};

            /* true names of the vertices */
            a = V[u];
            b = V[v];
            c = V[w];

            /* output Triangle */
            result.push_back(a);
            result.push_back(b);
            result.push_back(c);

            /* remove v from remaining polygon */
            for (s = v, t = v + 1; t < nv; s++, t++) {
                V[s] = V[t];
            }

            nv--;

            /* reset error detection counter */
            count = 2 * nv;
        }
    }

    delete[] V;

    return true;
}

// -------------------------------------------------------------

QuasiDelaunayTriangulator::QuasiDelaunayTriangulator() = default;

bool QuasiDelaunayTriangulator::Triangulate()
{
    if (!EarClippingTriangulator::Triangulate()) {
        return false;  // no valid triangulation
    }

    // For each internal edge get the adjacent facets. When doing an edge swap we must update
    // this structure.
    std::map<std::pair<PointIndex, PointIndex>, std::vector<FacetIndex>> aEdge2Face;
    for (std::vector<MeshFacet>::iterator pI = _facets.begin(); pI != _facets.end(); ++pI) {
        for (int i = 0; i < 3; i++) {
            PointIndex ulPt0 = std::min<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
            PointIndex ulPt1 = std::max<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
            // ignore borderlines of the polygon
            if ((ulPt1 - ulPt0) % (_points.size() - 1) > 1) {
                aEdge2Face[std::pair<PointIndex, PointIndex>(ulPt0, ulPt1)].push_back(
                    pI - _facets.begin());
            }
        }
    }

    // fill up this list with all internal edges and perform swap edges until this list is empty
    std::list<std::pair<PointIndex, PointIndex>> aEdgeList;
    std::map<std::pair<PointIndex, PointIndex>, std::vector<FacetIndex>>::iterator pE;
    for (pE = aEdge2Face.begin(); pE != aEdge2Face.end(); ++pE) {
        aEdgeList.push_back(pE->first);
    }

    // to be sure to avoid an endless loop
    size_t uMaxIter = 5 * aEdge2Face.size();

    // Perform a swap edge where needed
    while (!aEdgeList.empty() && uMaxIter > 0) {
        // get the first edge and remove it from the list
        std::pair<PointIndex, PointIndex> aEdge = aEdgeList.front();
        aEdgeList.pop_front();
        uMaxIter--;

        // get the adjacent facets to this edge
        pE = aEdge2Face.find(aEdge);

        // this edge has been removed some iterations before
        if (pE == aEdge2Face.end()) {
            continue;
        }

        MeshFacet& rF1 = _facets[pE->second[0]];
        MeshFacet& rF2 = _facets[pE->second[1]];
        unsigned short side1 = rF1.Side(aEdge.first, aEdge.second);

        Base::Vector3f cP1 = _points[rF1._aulPoints[side1]];
        Base::Vector3f cP2 = _points[rF1._aulPoints[(side1 + 1) % 3]];
        Base::Vector3f cP3 = _points[rF1._aulPoints[(side1 + 2) % 3]];

        unsigned short side2 = rF2.Side(aEdge.first, aEdge.second);
        Base::Vector3f cP4 = _points[rF2._aulPoints[(side2 + 2) % 3]];

        MeshGeomFacet cT1(cP1, cP2, cP3);
        float fMax1 = cT1.MaximumAngle();
        MeshGeomFacet cT2(cP2, cP1, cP4);
        float fMax2 = cT2.MaximumAngle();
        MeshGeomFacet cT3(cP4, cP3, cP1);
        float fMax3 = cT3.MaximumAngle();
        MeshGeomFacet cT4(cP3, cP4, cP2);
        float fMax4 = cT4.MaximumAngle();

        float fMax12 = std::max<float>(fMax1, fMax2);
        float fMax34 = std::max<float>(fMax3, fMax4);

        // We must make sure that the two adjacent triangles builds a convex polygon, otherwise
        // the swap edge operation is illegal
        Base::Vector3f cU = cP2 - cP1;
        Base::Vector3f cV = cP4 - cP3;
        // build a helper plane through cP1 that must separate cP3 and cP4
        Base::Vector3f cN1 = (cU % cV) % cU;
        if (((cP3 - cP1) * cN1) * ((cP4 - cP1) * cN1) >= 0.0f) {
            continue;  // not convex
        }
        // build a helper plane through cP3 that must separate cP1 and cP2
        Base::Vector3f cN2 = (cU % cV) % cV;
        if (((cP1 - cP3) * cN2) * ((cP2 - cP3) * cN2) >= 0.0f) {
            continue;  // not convex
        }

        // ok, here we should perform a swap edge to minimize the maximum angle
        if (fMax12 > fMax34) {
            rF1._aulPoints[(side1 + 1) % 3] = rF2._aulPoints[(side2 + 2) % 3];
            rF2._aulPoints[(side2 + 1) % 3] = rF1._aulPoints[(side1 + 2) % 3];

            // adjust the edge list
            for (int i = 0; i < 3; i++) {
                std::map<std::pair<PointIndex, PointIndex>, std::vector<FacetIndex>>::iterator it;
                // first facet
                PointIndex ulPt0 =
                    std::min<PointIndex>(rF1._aulPoints[i], rF1._aulPoints[(i + 1) % 3]);
                PointIndex ulPt1 =
                    std::max<PointIndex>(rF1._aulPoints[i], rF1._aulPoints[(i + 1) % 3]);
                it = aEdge2Face.find(std::make_pair(ulPt0, ulPt1));
                if (it != aEdge2Face.end()) {
                    if (it->second[0] == pE->second[1]) {
                        it->second[0] = pE->second[0];
                    }
                    else if (it->second[1] == pE->second[1]) {
                        it->second[1] = pE->second[0];
                    }
                    aEdgeList.push_back(it->first);
                }

                // second facet
                ulPt0 = std::min<PointIndex>(rF2._aulPoints[i], rF2._aulPoints[(i + 1) % 3]);
                ulPt1 = std::max<PointIndex>(rF2._aulPoints[i], rF2._aulPoints[(i + 1) % 3]);
                it = aEdge2Face.find(std::make_pair(ulPt0, ulPt1));
                if (it != aEdge2Face.end()) {
                    if (it->second[0] == pE->second[0]) {
                        it->second[0] = pE->second[1];
                    }
                    else if (it->second[1] == pE->second[0]) {
                        it->second[1] = pE->second[1];
                    }
                    aEdgeList.push_back(it->first);
                }
            }

            // Now we must remove the edge and replace it through the new edge
            PointIndex ulPt0 = std::min<PointIndex>(rF1._aulPoints[(side1 + 1) % 3],
                                                    rF2._aulPoints[(side2 + 1) % 3]);
            PointIndex ulPt1 = std::max<PointIndex>(rF1._aulPoints[(side1 + 1) % 3],
                                                    rF2._aulPoints[(side2 + 1) % 3]);
            std::pair<PointIndex, PointIndex> aNewEdge = std::make_pair(ulPt0, ulPt1);
            aEdge2Face[aNewEdge] = pE->second;
            aEdge2Face.erase(pE);
        }
    }

    return true;
}

// -------------------------------------------------------------

namespace MeshCore
{
namespace Triangulation
{
struct Vertex2d_Less
{
    bool operator()(const Base::Vector3f& p, const Base::Vector3f& q) const
    {
        if (fabs(p.x - q.x) < MeshDefinitions::_fMinPointDistanceD1) {
            if (fabs(p.y - q.y) < MeshDefinitions::_fMinPointDistanceD1) {
                return false;
            }
            else {
                return p.y < q.y;
            }
        }
        else {
            return p.x < q.x;
        }
    }
};
struct Vertex2d_EqualTo
{
    bool operator()(const Base::Vector3f& p, const Base::Vector3f& q) const
    {
        if (fabs(p.x - q.x) < MeshDefinitions::_fMinPointDistanceD1
            && fabs(p.y - q.y) < MeshDefinitions::_fMinPointDistanceD1) {
            return true;
        }

        return false;
    }
};
}  // namespace Triangulation
}  // namespace MeshCore

DelaunayTriangulator::DelaunayTriangulator() = default;

bool DelaunayTriangulator::Triangulate()
{
    // before starting the triangulation we must make sure that all polygon
    // points are different
    std::vector<Base::Vector3f> aPoints = _points;
    // sort the points ascending x,y coordinates
    std::sort(aPoints.begin(), aPoints.end(), Triangulation::Vertex2d_Less());
    // if there are two adjacent points whose distance is less then an epsilon
    if (std::adjacent_find(aPoints.begin(), aPoints.end(), Triangulation::Vertex2d_EqualTo())
        < aPoints.end()) {
        return false;
    }

    _facets.clear();
    _triangles.clear();

    std::vector<Wm4::Vector2d> akVertex;
    akVertex.reserve(_points.size());
    for (const auto& point : _points) {
        akVertex.emplace_back(static_cast<double>(point.x), static_cast<double>(point.y));
    }

    Wm4::Delaunay2d del(static_cast<int>(akVertex.size()),
                        &(akVertex[0]),
                        0.001,
                        false,
                        Wm4::Query::QT_INT64);
    int iTQuantity = del.GetSimplexQuantity();
    std::vector<int> aiTVertex(static_cast<size_t>(3 * iTQuantity));

    bool succeeded = false;
    if (iTQuantity > 0) {
        size_t uiSize = static_cast<size_t>(3 * iTQuantity) * sizeof(int);
        Wm4::System::Memcpy(&(aiTVertex[0]), uiSize, del.GetIndices(), uiSize);

        // If H is the number of hull edges and N is the number of vertices,
        // then the triangulation must have 2*N-2-H triangles and 3*N-3-H
        // edges.
        int iEQuantity = 0;
        int* aiIndex = nullptr;
        del.GetHull(iEQuantity, aiIndex);
        int iUniqueVQuantity = del.GetUniqueVertexQuantity();
        int iTVerify = 2 * iUniqueVQuantity - 2 - iEQuantity;
        (void)iTVerify;  // avoid warning in release build
        succeeded = (iTVerify == iTQuantity);
        int iEVerify = 3 * iUniqueVQuantity - 3 - iEQuantity;
        (void)iEVerify;  // avoid warning about unused variable
        delete[] aiIndex;
    }

    MeshGeomFacet triangle;
    MeshFacet facet;
    for (int i = 0; i < iTQuantity; i++) {
        for (int j = 0; j < 3; j++) {
            size_t index = static_cast<size_t>(aiTVertex[static_cast<size_t>(3 * i + j)]);
            facet._aulPoints[j] = static_cast<PointIndex>(index);
            triangle._aclPoints[j].x = static_cast<float>(akVertex[index].X());
            triangle._aclPoints[j].y = static_cast<float>(akVertex[index].Y());
        }

        _triangles.push_back(triangle);
        _facets.push_back(facet);
    }

    return succeeded;
}

// -------------------------------------------------------------

FlatTriangulator::FlatTriangulator() = default;

bool FlatTriangulator::Triangulate()
{
    _newpoints.clear();
    // before starting the triangulation we must make sure that all polygon
    // points are different
    std::vector<Base::Vector3f> aPoints = ProjectToFitPlane();
    std::vector<Base::Vector3f> tmp = aPoints;
    // sort the points ascending x,y coordinates
    std::sort(tmp.begin(), tmp.end(), Triangulation::Vertex2d_Less());
    // if there are two adjacent points whose distance is less then an epsilon
    if (std::adjacent_find(tmp.begin(), tmp.end(), Triangulation::Vertex2d_EqualTo()) < tmp.end()) {
        return false;
    }

    _facets.clear();
    _triangles.clear();

    // Todo: Implement algorithm for constraint delaunay triangulation
    QuasiDelaunayTriangulator tria;
    tria.SetPolygon(this->GetPolygon());
    bool succeeded = tria.TriangulatePolygon();
    this->_facets = tria.GetFacets();
    this->_triangles = tria.GetTriangles();

    return succeeded;
}

void FlatTriangulator::PostProcessing(const std::vector<Base::Vector3f>&)
{}

// -------------------------------------------------------------

ConstraintDelaunayTriangulator::ConstraintDelaunayTriangulator(float area)
    : fMaxArea(area)
{
    // silent warning: -Wunused-private-field
    (void)fMaxArea;
}

bool ConstraintDelaunayTriangulator::Triangulate()
{
    _newpoints.clear();
    // before starting the triangulation we must make sure that all polygon
    // points are different
    std::vector<Base::Vector3f> aPoints = ProjectToFitPlane();
    std::vector<Base::Vector3f> tmp = aPoints;
    // sort the points ascending x,y coordinates
    std::sort(tmp.begin(), tmp.end(), Triangulation::Vertex2d_Less());
    // if there are two adjacent points whose distance is less then an epsilon
    if (std::adjacent_find(tmp.begin(), tmp.end(), Triangulation::Vertex2d_EqualTo()) < tmp.end()) {
        return false;
    }

    _facets.clear();
    _triangles.clear();

    // Todo: Implement algorithm for constraint delaunay triangulation
    QuasiDelaunayTriangulator tria;
    tria.SetPolygon(this->GetPolygon());
    bool succeeded = tria.TriangulatePolygon();
    this->_facets = tria.GetFacets();
    this->_triangles = tria.GetTriangles();

    return succeeded;
}

// -------------------------------------------------------------

#if 0
Triangulator::Triangulator(const MeshKernel& k, bool flat) : _kernel(k)
{
}

Triangulator::~Triangulator()
{
}

bool Triangulator::Triangulate()
{
    return false;
}

MeshGeomFacet Triangulator::GetTriangle(const MeshPointArray&,
                                        const MeshFacet& facet) const
{
    return MeshGeomFacet();
}

void Triangulator::PostProcessing(const std::vector<Base::Vector3f>&)
{
}

void Triangulator::Discard()
{
    AbstractPolygonTriangulator::Discard();
}

void Triangulator::Reset()
{
}
#endif

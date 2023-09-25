/***************************************************************************
 *   Copyright (c) 2012 Imetric 3D GmbH                                    *
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

#include "Algorithm.h"
#include "Approximation.h"
#include "Segmentation.h"

using namespace MeshCore;

void MeshSurfaceSegment::Initialize(FacetIndex)
{}

bool MeshSurfaceSegment::TestInitialFacet(FacetIndex) const
{
    return true;
}

void MeshSurfaceSegment::AddFacet(const MeshFacet&)
{}

void MeshSurfaceSegment::AddSegment(const std::vector<FacetIndex>& segm)
{
    if (segm.size() >= minFacets) {
        segments.push_back(segm);
    }
}

MeshSegment MeshSurfaceSegment::FindSegment(FacetIndex index) const
{
    for (const auto& segment : segments) {
        if (std::find(segment.begin(), segment.end(), index) != segment.end()) {
            return segment;
        }
    }

    return {};
}

// --------------------------------------------------------

MeshDistancePlanarSegment::MeshDistancePlanarSegment(const MeshKernel& mesh,
                                                     unsigned long minFacets,
                                                     float tol)
    : MeshDistanceSurfaceSegment(mesh, minFacets, tol)
    , fitter(new PlaneFit)
{}

MeshDistancePlanarSegment::~MeshDistancePlanarSegment()
{
    delete fitter;
}

void MeshDistancePlanarSegment::Initialize(FacetIndex index)
{
    fitter->Clear();

    MeshGeomFacet triangle = kernel.GetFacet(index);
    basepoint = triangle.GetGravityPoint();
    normal = triangle.GetNormal();
    fitter->AddPoint(triangle._aclPoints[0]);
    fitter->AddPoint(triangle._aclPoints[1]);
    fitter->AddPoint(triangle._aclPoints[2]);
}

bool MeshDistancePlanarSegment::TestFacet(const MeshFacet& face) const
{
    if (!fitter->Done()) {
        fitter->Fit();
    }
    MeshGeomFacet triangle = kernel.GetFacet(face);
    for (auto pnt : triangle._aclPoints) {
        if (fabs(fitter->GetDistanceToPlane(pnt)) > tolerance) {
            return false;
        }
    }

    return true;
}

void MeshDistancePlanarSegment::AddFacet(const MeshFacet& face)
{
    MeshGeomFacet triangle = kernel.GetFacet(face);
    fitter->AddPoint(triangle.GetGravityPoint());
}

// --------------------------------------------------------

PlaneSurfaceFit::PlaneSurfaceFit()
    : fitter(new PlaneFit)
{}

PlaneSurfaceFit::PlaneSurfaceFit(const Base::Vector3f& b, const Base::Vector3f& n)
    : basepoint(b)
    , normal(n)
    , fitter(nullptr)
{}

PlaneSurfaceFit::~PlaneSurfaceFit()
{
    delete fitter;
}

void PlaneSurfaceFit::Initialize(const MeshCore::MeshGeomFacet& tria)
{
    if (fitter) {
        basepoint = tria.GetGravityPoint();
        normal = tria.GetNormal();

        fitter->Clear();

        fitter->AddPoint(tria._aclPoints[0]);
        fitter->AddPoint(tria._aclPoints[1]);
        fitter->AddPoint(tria._aclPoints[2]);
        fitter->Fit();
    }
}

bool PlaneSurfaceFit::TestTriangle(const MeshGeomFacet&) const
{
    return true;
}

void PlaneSurfaceFit::AddTriangle(const MeshCore::MeshGeomFacet& tria)
{
    if (fitter) {
        fitter->AddPoint(tria.GetGravityPoint());
    }
}

bool PlaneSurfaceFit::Done() const
{
    if (!fitter) {
        return true;
    }
    else {
        return fitter->Done();
    }
}

float PlaneSurfaceFit::Fit()
{
    if (!fitter) {
        return 0;
    }
    else {
        return fitter->Fit();
    }
}

float PlaneSurfaceFit::GetDistanceToSurface(const Base::Vector3f& pnt) const
{
    if (!fitter) {
        return pnt.DistanceToPlane(basepoint, normal);
    }
    else {
        return fitter->GetDistanceToPlane(pnt);
    }
}

std::vector<float> PlaneSurfaceFit::Parameters() const
{
    Base::Vector3f base = basepoint;
    Base::Vector3f norm = normal;
    if (fitter) {
        base = fitter->GetBase();
        norm = fitter->GetNormal();
    }

    std::vector<float> c;
    c.push_back(base.x);
    c.push_back(base.y);
    c.push_back(base.z);
    c.push_back(norm.x);
    c.push_back(norm.y);
    c.push_back(norm.z);
    return c;
}

// --------------------------------------------------------

CylinderSurfaceFit::CylinderSurfaceFit()
    : radius(FLOAT_MAX)
    , fitter(new CylinderFit)
{
    axis.Set(0, 0, 0);
}

/*!
 * \brief CylinderSurfaceFit::CylinderSurfaceFit
 * Set a predefined cylinder. Internal cylinder fits are not done, then.
 */
CylinderSurfaceFit::CylinderSurfaceFit(const Base::Vector3f& b, const Base::Vector3f& a, float r)
    : basepoint(b)
    , axis(a)
    , radius(r)
    , fitter(nullptr)
{}

CylinderSurfaceFit::~CylinderSurfaceFit()
{
    delete fitter;
}

void CylinderSurfaceFit::Initialize(const MeshCore::MeshGeomFacet& tria)
{
    if (fitter) {
        fitter->Clear();
        fitter->AddPoint(tria._aclPoints[0]);
        fitter->AddPoint(tria._aclPoints[1]);
        fitter->AddPoint(tria._aclPoints[2]);
    }
}

void CylinderSurfaceFit::AddTriangle(const MeshCore::MeshGeomFacet& tria)
{
    if (fitter) {
        fitter->AddPoint(tria._aclPoints[0]);
        fitter->AddPoint(tria._aclPoints[1]);
        fitter->AddPoint(tria._aclPoints[2]);
    }
}

bool CylinderSurfaceFit::TestTriangle(const MeshGeomFacet& tria) const
{
    // This is to filter out triangles whose points lie on the cylinder and
    // that whose normals are more or less parallel to the cylinder axis
    float dot = axis.Dot(tria.GetNormal());
    return fabs(dot) < 0.5f;
}

bool CylinderSurfaceFit::Done() const
{
    if (fitter) {
        return fitter->Done();
    }

    return true;
}

float CylinderSurfaceFit::Fit()
{
    if (!fitter) {
        return 0;
    }

    float fit = fitter->Fit();
    if (fit < FLOAT_MAX) {
        basepoint = fitter->GetBase();
        axis = fitter->GetAxis();
        radius = fitter->GetRadius();
    }
    return fit;
}

float CylinderSurfaceFit::GetDistanceToSurface(const Base::Vector3f& pnt) const
{
    if (fitter && !fitter->Done()) {
        // collect some points
        return 0;
    }
    float dist = pnt.DistanceToLine(basepoint, axis);
    return (dist - radius);
}

std::vector<float> CylinderSurfaceFit::Parameters() const
{
    Base::Vector3f base = basepoint;
    Base::Vector3f norm = axis;
    float radval = radius;
    if (fitter) {
        base = fitter->GetBase();
        norm = fitter->GetAxis();
        radval = fitter->GetRadius();
    }

    std::vector<float> c;
    c.push_back(base.x);
    c.push_back(base.y);
    c.push_back(base.z);
    c.push_back(norm.x);
    c.push_back(norm.y);
    c.push_back(norm.z);
    c.push_back(radval);
    return c;
}

// --------------------------------------------------------

SphereSurfaceFit::SphereSurfaceFit()
    : radius(FLOAT_MAX)
    , fitter(new SphereFit)
{
    center.Set(0, 0, 0);
}

SphereSurfaceFit::SphereSurfaceFit(const Base::Vector3f& c, float r)
    : center(c)
    , radius(r)
    , fitter(nullptr)
{}

SphereSurfaceFit::~SphereSurfaceFit()
{
    delete fitter;
}

void SphereSurfaceFit::Initialize(const MeshCore::MeshGeomFacet& tria)
{
    if (fitter) {
        fitter->Clear();
        fitter->AddPoint(tria._aclPoints[0]);
        fitter->AddPoint(tria._aclPoints[1]);
        fitter->AddPoint(tria._aclPoints[2]);
    }
}

void SphereSurfaceFit::AddTriangle(const MeshCore::MeshGeomFacet& tria)
{
    if (fitter) {
        fitter->AddPoint(tria._aclPoints[0]);
        fitter->AddPoint(tria._aclPoints[1]);
        fitter->AddPoint(tria._aclPoints[2]);
    }
}

bool SphereSurfaceFit::TestTriangle(const MeshGeomFacet&) const
{
    // Already handled by GetDistanceToSurface
    return true;
}

bool SphereSurfaceFit::Done() const
{
    if (fitter) {
        return fitter->Done();
    }

    return true;
}

float SphereSurfaceFit::Fit()
{
    if (!fitter) {
        return 0;
    }

    float fit = fitter->Fit();
    if (fit < FLOAT_MAX) {
        center = fitter->GetCenter();
        radius = fitter->GetRadius();
    }
    return fit;
}

float SphereSurfaceFit::GetDistanceToSurface(const Base::Vector3f& pnt) const
{
    float dist = Base::Distance(pnt, center);
    return (dist - radius);
}

std::vector<float> SphereSurfaceFit::Parameters() const
{
    Base::Vector3f base = center;
    float radval = radius;
    if (fitter) {
        base = fitter->GetCenter();
        radval = fitter->GetRadius();
    }

    std::vector<float> c;
    c.push_back(base.x);
    c.push_back(base.y);
    c.push_back(base.z);
    c.push_back(radval);
    return c;
}

// --------------------------------------------------------

MeshDistanceGenericSurfaceFitSegment::MeshDistanceGenericSurfaceFitSegment(AbstractSurfaceFit* fit,
                                                                           const MeshKernel& mesh,
                                                                           unsigned long minFacets,
                                                                           float tol)
    : MeshDistanceSurfaceSegment(mesh, minFacets, tol)
    , fitter(fit)
{}

MeshDistanceGenericSurfaceFitSegment::~MeshDistanceGenericSurfaceFitSegment()
{
    delete fitter;
}

void MeshDistanceGenericSurfaceFitSegment::Initialize(FacetIndex index)
{
    MeshGeomFacet triangle = kernel.GetFacet(index);
    fitter->Initialize(triangle);
}

bool MeshDistanceGenericSurfaceFitSegment::TestInitialFacet(FacetIndex index) const
{
    MeshGeomFacet triangle = kernel.GetFacet(index);
    for (auto pnt : triangle._aclPoints) {
        if (fabs(fitter->GetDistanceToSurface(pnt)) > tolerance) {
            return false;
        }
    }
    return fitter->TestTriangle(triangle);
}

bool MeshDistanceGenericSurfaceFitSegment::TestFacet(const MeshFacet& face) const
{
    if (!fitter->Done()) {
        fitter->Fit();
    }
    MeshGeomFacet triangle = kernel.GetFacet(face);
    for (auto ptIndex : triangle._aclPoints) {
        if (fabs(fitter->GetDistanceToSurface(ptIndex)) > tolerance) {
            return false;
        }
    }

    return fitter->TestTriangle(triangle);
}

void MeshDistanceGenericSurfaceFitSegment::AddFacet(const MeshFacet& face)
{
    MeshGeomFacet triangle = kernel.GetFacet(face);
    fitter->AddTriangle(triangle);
}

std::vector<float> MeshDistanceGenericSurfaceFitSegment::Parameters() const
{
    return fitter->Parameters();
}

// --------------------------------------------------------

bool MeshCurvaturePlanarSegment::TestFacet(const MeshFacet& rclFacet) const
{
    for (PointIndex ptIndex : rclFacet._aulPoints) {
        const CurvatureInfo& ci = GetInfo(ptIndex);
        if (fabs(ci.fMinCurvature) > tolerance) {
            return false;
        }
        if (fabs(ci.fMaxCurvature) > tolerance) {
            return false;
        }
    }

    return true;
}

bool MeshCurvatureCylindricalSegment::TestFacet(const MeshFacet& rclFacet) const
{
    for (PointIndex ptIndex : rclFacet._aulPoints) {
        const CurvatureInfo& ci = GetInfo(ptIndex);
        float fMax = std::max<float>(fabs(ci.fMaxCurvature), fabs(ci.fMinCurvature));
        float fMin = std::min<float>(fabs(ci.fMaxCurvature), fabs(ci.fMinCurvature));
        if (fMin > toleranceMin) {
            return false;
        }
        if (fabs(fMax - curvature) > toleranceMax) {
            return false;
        }
    }

    return true;
}

bool MeshCurvatureSphericalSegment::TestFacet(const MeshFacet& rclFacet) const
{
    for (PointIndex ptIndex : rclFacet._aulPoints) {
        const CurvatureInfo& ci = GetInfo(ptIndex);
        if (ci.fMaxCurvature * ci.fMinCurvature < 0) {
            return false;
        }
        float diff {};
        diff = fabs(ci.fMinCurvature) - curvature;
        if (fabs(diff) > tolerance) {
            return false;
        }
        diff = fabs(ci.fMaxCurvature) - curvature;
        if (fabs(diff) > tolerance) {
            return false;
        }
    }

    return true;
}

bool MeshCurvatureFreeformSegment::TestFacet(const MeshFacet& rclFacet) const
{
    for (PointIndex ptIndex : rclFacet._aulPoints) {
        const CurvatureInfo& ci = GetInfo(ptIndex);
        if (fabs(ci.fMinCurvature - c2) > toleranceMin) {
            return false;
        }
        if (fabs(ci.fMaxCurvature - c1) > toleranceMax) {
            return false;
        }
    }

    return true;
}

// --------------------------------------------------------

MeshSurfaceVisitor::MeshSurfaceVisitor(MeshSurfaceSegment& segm, std::vector<FacetIndex>& indices)
    : indices(indices)
    , segm(segm)
{}

bool MeshSurfaceVisitor::AllowVisit(const MeshFacet& face,
                                    const MeshFacet&,
                                    FacetIndex,
                                    unsigned long,
                                    unsigned short)
{
    return segm.TestFacet(face);
}

bool MeshSurfaceVisitor::Visit(const MeshFacet& face,
                               const MeshFacet&,
                               FacetIndex ulFInd,
                               unsigned long)
{
    indices.push_back(ulFInd);
    segm.AddFacet(face);
    return true;
}

// --------------------------------------------------------

void MeshSegmentAlgorithm::FindSegments(std::vector<MeshSurfaceSegmentPtr>& segm)
{
    // reset VISIT flags
    FacetIndex startFacet {};
    MeshCore::MeshAlgorithm cAlgo(myKernel);
    cAlgo.ResetFacetFlag(MeshCore::MeshFacet::VISIT);

    const MeshCore::MeshFacetArray& rFAry = myKernel.GetFacets();
    MeshCore::MeshFacetArray::_TConstIterator iCur = rFAry.begin();
    MeshCore::MeshFacetArray::_TConstIterator iBeg = rFAry.begin();
    MeshCore::MeshFacetArray::_TConstIterator iEnd = rFAry.end();

    // start from the first not visited facet
    cAlgo.CountFacetFlag(MeshCore::MeshFacet::VISIT);
    std::vector<FacetIndex> resetVisited;

    for (auto& it : segm) {
        cAlgo.ResetFacetsFlag(resetVisited, MeshCore::MeshFacet::VISIT);
        resetVisited.clear();

        MeshCore::MeshIsNotFlag<MeshCore::MeshFacet> flag;
        iCur = std::find_if(iBeg, iEnd, [flag](const MeshFacet& f) {
            return flag(f, MeshFacet::VISIT);
        });
        if (iCur < iEnd) {
            startFacet = iCur - iBeg;
        }
        else {
            startFacet = FACET_INDEX_MAX;
        }
        while (startFacet != FACET_INDEX_MAX) {
            // collect all facets of the same geometry
            std::vector<FacetIndex> indices;
            it->Initialize(startFacet);
            if (it->TestInitialFacet(startFacet)) {
                indices.push_back(startFacet);
            }
            MeshSurfaceVisitor pv(*it, indices);
            myKernel.VisitNeighbourFacets(pv, startFacet);

            // add or discard the segment
            if (indices.size() <= 1) {
                resetVisited.push_back(startFacet);
            }
            else {
                it->AddSegment(indices);
            }

            // search for the next start facet
            iCur = std::find_if(iCur, iEnd, [flag](const MeshFacet& f) {
                return flag(f, MeshFacet::VISIT);
            });
            if (iCur < iEnd) {
                startFacet = iCur - iBeg;
            }
            else {
                startFacet = FACET_INDEX_MAX;
            }
        }
    }
}

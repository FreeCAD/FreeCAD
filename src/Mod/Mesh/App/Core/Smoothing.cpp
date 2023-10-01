/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Base/Tools.h>

#include "Algorithm.h"
#include "Approximation.h"
#include "Iterator.h"
#include "MeshKernel.h"
#include "Smoothing.h"


using namespace MeshCore;


AbstractSmoothing::AbstractSmoothing(MeshKernel& m)
    : kernel(m)
{}

AbstractSmoothing::~AbstractSmoothing() = default;

void AbstractSmoothing::initialize(Component comp, Continuity cont)
{
    this->component = comp;
    this->continuity = cont;
}

PlaneFitSmoothing::PlaneFitSmoothing(MeshKernel& m)
    : AbstractSmoothing(m)
{}

void PlaneFitSmoothing::Smooth(unsigned int iterations)
{
    MeshCore::MeshPoint center;
    MeshCore::MeshPointArray PointArray = kernel.GetPoints();

    MeshCore::MeshPointIterator v_it(kernel);
    MeshCore::MeshRefPointToPoints vv_it(kernel);
    MeshCore::MeshPointArray::_TConstIterator v_beg = kernel.GetPoints().begin();

    for (unsigned int i = 0; i < iterations; i++) {
        Base::Vector3f N, L;
        for (v_it.Begin(); v_it.More(); v_it.Next()) {
            MeshCore::PlaneFit pf;
            pf.AddPoint(*v_it);
            center = *v_it;
            const std::set<PointIndex>& cv = vv_it[v_it.Position()];
            if (cv.size() < 3) {
                continue;
            }

            std::set<PointIndex>::const_iterator cv_it;
            for (cv_it = cv.begin(); cv_it != cv.end(); ++cv_it) {
                pf.AddPoint(v_beg[*cv_it]);
                center += v_beg[*cv_it];
            }

            float scale = 1.0f / (static_cast<float>(cv.size()) + 1.0f);
            center.Scale(scale, scale, scale);

            // get the mean plane of the current vertex with the surrounding vertices
            pf.Fit();
            N = pf.GetNormal();
            N.Normalize();

            // look in which direction we should move the vertex
            L.Set(v_it->x - center.x, v_it->y - center.y, v_it->z - center.z);
            if (N * L < 0.0f) {
                N.Scale(-1.0, -1.0, -1.0);
            }

            // maximum value to move is distance to mean plane
            float d = std::min<float>(fabs(this->maximum), fabs(N * L));
            N.Scale(d, d, d);

            PointArray[v_it.Position()].Set(v_it->x - N.x, v_it->y - N.y, v_it->z - N.z);
        }

        // assign values without affecting iterators
        PointIndex count = kernel.CountPoints();
        for (PointIndex idx = 0; idx < count; idx++) {
            kernel.SetPoint(idx, PointArray[idx]);
        }
    }
}

void PlaneFitSmoothing::SmoothPoints(unsigned int iterations,
                                     const std::vector<PointIndex>& point_indices)
{
    MeshCore::MeshPoint center;
    MeshCore::MeshPointArray PointArray = kernel.GetPoints();

    MeshCore::MeshPointIterator v_it(kernel);
    MeshCore::MeshRefPointToPoints vv_it(kernel);
    MeshCore::MeshPointArray::_TConstIterator v_beg = kernel.GetPoints().begin();

    for (unsigned int i = 0; i < iterations; i++) {
        Base::Vector3f N, L;
        for (PointIndex it : point_indices) {
            v_it.Set(it);
            MeshCore::PlaneFit pf;
            pf.AddPoint(*v_it);
            center = *v_it;
            const std::set<PointIndex>& cv = vv_it[v_it.Position()];
            if (cv.size() < 3) {
                continue;
            }

            std::set<PointIndex>::const_iterator cv_it;
            for (cv_it = cv.begin(); cv_it != cv.end(); ++cv_it) {
                pf.AddPoint(v_beg[*cv_it]);
                center += v_beg[*cv_it];
            }

            float scale = 1.0f / (static_cast<float>(cv.size()) + 1.0f);
            center.Scale(scale, scale, scale);

            // get the mean plane of the current vertex with the surrounding vertices
            pf.Fit();
            N = pf.GetNormal();
            N.Normalize();

            // look in which direction we should move the vertex
            L.Set(v_it->x - center.x, v_it->y - center.y, v_it->z - center.z);
            if (N * L < 0.0f) {
                N.Scale(-1.0, -1.0, -1.0);
            }

            // maximum value to move is distance to mean plane
            float d = std::min<float>(fabs(this->maximum), fabs(N * L));
            N.Scale(d, d, d);

            PointArray[v_it.Position()].Set(v_it->x - N.x, v_it->y - N.y, v_it->z - N.z);
        }

        // assign values without affecting iterators
        PointIndex count = kernel.CountPoints();
        for (PointIndex idx = 0; idx < count; idx++) {
            kernel.SetPoint(idx, PointArray[idx]);
        }
    }
}

LaplaceSmoothing::LaplaceSmoothing(MeshKernel& m)
    : AbstractSmoothing(m)
{}

void LaplaceSmoothing::Umbrella(const MeshRefPointToPoints& vv_it,
                                const MeshRefPointToFacets& vf_it,
                                double stepsize)
{
    const MeshCore::MeshPointArray& points = kernel.GetPoints();
    MeshCore::MeshPointArray::_TConstIterator v_it, v_beg = points.begin(), v_end = points.end();

    PointIndex pos = 0;
    for (v_it = points.begin(); v_it != v_end; ++v_it, ++pos) {
        const std::set<PointIndex>& cv = vv_it[pos];
        if (cv.size() < 3) {
            continue;
        }
        if (cv.size() != vf_it[pos].size()) {
            // do nothing for border points
            continue;
        }

        size_t n_count = cv.size();
        double w {};
        w = 1.0 / double(n_count);

        double delx = 0.0, dely = 0.0, delz = 0.0;
        std::set<PointIndex>::const_iterator cv_it;
        for (cv_it = cv.begin(); cv_it != cv.end(); ++cv_it) {
            delx += w * static_cast<double>((v_beg[*cv_it]).x - v_it->x);
            dely += w * static_cast<double>((v_beg[*cv_it]).y - v_it->y);
            delz += w * static_cast<double>((v_beg[*cv_it]).z - v_it->z);
        }

        float x = static_cast<float>(static_cast<double>(v_it->x) + stepsize * delx);
        float y = static_cast<float>(static_cast<double>(v_it->y) + stepsize * dely);
        float z = static_cast<float>(static_cast<double>(v_it->z) + stepsize * delz);
        kernel.SetPoint(pos, x, y, z);
    }
}

void LaplaceSmoothing::Umbrella(const MeshRefPointToPoints& vv_it,
                                const MeshRefPointToFacets& vf_it,
                                double stepsize,
                                const std::vector<PointIndex>& point_indices)
{
    const MeshCore::MeshPointArray& points = kernel.GetPoints();
    MeshCore::MeshPointArray::_TConstIterator v_beg = points.begin();

    for (PointIndex it : point_indices) {
        const std::set<PointIndex>& cv = vv_it[it];
        if (cv.size() < 3) {
            continue;
        }
        if (cv.size() != vf_it[it].size()) {
            // do nothing for border points
            continue;
        }

        size_t n_count = cv.size();
        double w {};
        w = 1.0 / double(n_count);

        double delx = 0.0, dely = 0.0, delz = 0.0;
        std::set<PointIndex>::const_iterator cv_it;
        for (cv_it = cv.begin(); cv_it != cv.end(); ++cv_it) {
            delx += w * static_cast<double>((v_beg[*cv_it]).x - (v_beg[it]).x);
            dely += w * static_cast<double>((v_beg[*cv_it]).y - (v_beg[it]).y);
            delz += w * static_cast<double>((v_beg[*cv_it]).z - (v_beg[it]).z);
        }

        float x = static_cast<float>(static_cast<double>((v_beg[it]).x) + stepsize * delx);
        float y = static_cast<float>(static_cast<double>((v_beg[it]).y) + stepsize * dely);
        float z = static_cast<float>(static_cast<double>((v_beg[it]).z) + stepsize * delz);
        kernel.SetPoint(it, x, y, z);
    }
}

void LaplaceSmoothing::Smooth(unsigned int iterations)
{
    MeshCore::MeshRefPointToPoints vv_it(kernel);
    MeshCore::MeshRefPointToFacets vf_it(kernel);

    for (unsigned int i = 0; i < iterations; i++) {
        Umbrella(vv_it, vf_it, lambda);
    }
}

void LaplaceSmoothing::SmoothPoints(unsigned int iterations,
                                    const std::vector<PointIndex>& point_indices)
{
    MeshCore::MeshRefPointToPoints vv_it(kernel);
    MeshCore::MeshRefPointToFacets vf_it(kernel);

    for (unsigned int i = 0; i < iterations; i++) {
        Umbrella(vv_it, vf_it, lambda, point_indices);
    }
}

TaubinSmoothing::TaubinSmoothing(MeshKernel& m)
    : LaplaceSmoothing(m)
{}

void TaubinSmoothing::Smooth(unsigned int iterations)
{
    MeshCore::MeshRefPointToPoints vv_it(kernel);
    MeshCore::MeshRefPointToFacets vf_it(kernel);

    // Theoretically Taubin does not shrink the surface
    iterations = (iterations + 1) / 2;  // two steps per iteration
    for (unsigned int i = 0; i < iterations; i++) {
        Umbrella(vv_it, vf_it, GetLambda());
        Umbrella(vv_it, vf_it, -(GetLambda() + micro));
    }
}

void TaubinSmoothing::SmoothPoints(unsigned int iterations,
                                   const std::vector<PointIndex>& point_indices)
{
    MeshCore::MeshRefPointToPoints vv_it(kernel);
    MeshCore::MeshRefPointToFacets vf_it(kernel);

    // Theoretically Taubin does not shrink the surface
    iterations = (iterations + 1) / 2;  // two steps per iteration
    for (unsigned int i = 0; i < iterations; i++) {
        Umbrella(vv_it, vf_it, GetLambda(), point_indices);
        Umbrella(vv_it, vf_it, -(GetLambda() + micro), point_indices);
    }
}

namespace
{
using AngleNormal = std::pair<double, Base::Vector3d>;
inline Base::Vector3d find_median(std::vector<AngleNormal>& container)
{
    auto compare_angle_normal = [](const AngleNormal& an1, const AngleNormal& an2) {
        return an1.first < an2.first;
    };
    size_t n = container.size() / 2;
    std::nth_element(container.begin(),
                     container.begin() + n,
                     container.end(),
                     compare_angle_normal);

    if ((container.size() % 2) == 1) {
        return container[n].second;
    }
    else {
        // even sized vector -> average the two middle values
        auto max_it =
            std::max_element(container.begin(), container.begin() + n, compare_angle_normal);
        Base::Vector3d vec = (max_it->second + container[n].second) / 2.0;
        vec.Normalize();
        return vec;
    }
}
}  // namespace

MedianFilterSmoothing::MedianFilterSmoothing(MeshKernel& m)
    : AbstractSmoothing(m)
{}

void MedianFilterSmoothing::Smooth(unsigned int iterations)
{
    std::vector<unsigned long> point_indices(kernel.CountPoints());
    std::generate(point_indices.begin(), point_indices.end(), Base::iotaGen<unsigned long>(0));
    MeshCore::MeshRefFacetToFacets ff_it(kernel);
    MeshCore::MeshRefPointToFacets vf_it(kernel);

    for (unsigned int i = 0; i < iterations; i++) {
        UpdatePoints(ff_it, vf_it, point_indices);
    }
}

void MedianFilterSmoothing::SmoothPoints(unsigned int iterations,
                                         const std::vector<PointIndex>& point_indices)
{
    MeshCore::MeshRefFacetToFacets ff_it(kernel);
    MeshCore::MeshRefPointToFacets vf_it(kernel);

    for (unsigned int i = 0; i < iterations; i++) {
        UpdatePoints(ff_it, vf_it, point_indices);
    }
}

void MedianFilterSmoothing::UpdatePoints(const MeshRefFacetToFacets& ff_it,
                                         const MeshRefPointToFacets& vf_it,
                                         const std::vector<PointIndex>& point_indices)
{
    const MeshCore::MeshPointArray& points = kernel.GetPoints();
    const MeshCore::MeshFacetArray& facets = kernel.GetFacets();

    // Initialize the array with the real normals
    std::vector<Base::Vector3d> faceNormals;
    faceNormals.reserve(facets.size());
    MeshCore::MeshFacetIterator iter(kernel);
    for (iter.Init(); iter.More(); iter.Next()) {
        faceNormals.emplace_back(Base::toVector<double>(iter->GetNormal()));
    }

    // Step 1: determine face normals
    for (FacetIndex pos = 0; pos < facets.size(); pos++) {
        iter.Set(pos);
        Base::Vector3d refNormal = Base::toVector<double>(iter->GetNormal());
        const std::set<FacetIndex>& cv = ff_it[pos];
        const MeshCore::MeshFacet& facet = facets[pos];

        std::vector<AngleNormal> anglesWithFaces;
        for (auto fi : cv) {
            iter.Set(fi);
            Base::Vector3d faceNormal = Base::toVector<double>(iter->GetNormal());
            double angle = refNormal.GetAngle(faceNormal);

            int absWeight = std::abs(weights);
            if (absWeight > 1 && facet.IsNeighbour(fi)) {
                if (weights < 0) {
                    angle = -angle;
                }
                for (int i = 0; i < absWeight; i++) {
                    anglesWithFaces.emplace_back(angle, faceNormal);
                }
            }
            else {
                anglesWithFaces.emplace_back(angle, faceNormal);
            }
        }

        faceNormals[pos] = find_median(anglesWithFaces);
    }

    // Step 2: move vertices
    for (auto pos : point_indices) {
        Base::Vector3d P = Base::toVector<double>(points[pos]);
        const std::set<FacetIndex>& cv = vf_it[pos];

        double totalArea = 0.0;
        Base::Vector3d totalvT;
        for (auto it : cv) {
            iter.Set(it);

            double faceArea = iter->Area();
            totalArea += faceArea;

            Base::Vector3d C = Base::toVector<double>(iter->GetGravityPoint());

            Base::Vector3d PC = C - P;
            Base::Vector3d mT = faceNormals[it];
            Base::Vector3d vT = (PC * mT) * mT;
            totalvT += vT * faceArea;
        }

        P = P + totalvT / totalArea;
        kernel.SetPoint(pos, Base::toVector<float>(P));
    }
}

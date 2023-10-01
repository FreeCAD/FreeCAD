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
#include <functional>
#endif

#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentMap>

#include <Base/Sequencer.h>
#include <Base/Tools.h>

// #define OPTIMIZE_CURVATURE
#ifdef OPTIMIZE_CURVATURE
#include <Eigen/Eigenvalues>
#else
#include <Mod/Mesh/App/WildMagic4/Wm4MeshCurvature.h>
#endif

#include "Approximation.h"
#include "Curvature.h"
#include "Iterator.h"
#include "MeshKernel.h"
#include "Tools.h"


using namespace MeshCore;
namespace sp = std::placeholders;

MeshCurvature::MeshCurvature(const MeshKernel& kernel)
    : myKernel(kernel)
    , myMinPoints(20)
    , myRadius(0.5f)
{
    mySegment.resize(kernel.CountFacets());
    std::generate(mySegment.begin(), mySegment.end(), Base::iotaGen<FacetIndex>(0));
}

MeshCurvature::MeshCurvature(const MeshKernel& kernel, std::vector<FacetIndex> segm)
    : myKernel(kernel)
    , myMinPoints(20)
    , myRadius(0.5f)
    , mySegment(std::move(segm))
{}

void MeshCurvature::ComputePerFace(bool parallel)
{
    myCurvature.clear();
    MeshRefPointToFacets search(myKernel);
    FacetCurvature face(myKernel, search, myRadius, myMinPoints);

    if (!parallel) {
        Base::SequencerLauncher seq("Curvature estimation", mySegment.size());
        for (FacetIndex it : mySegment) {
            CurvatureInfo info = face.Compute(it);
            myCurvature.push_back(info);
            seq.next();
        }
    }
    else {
        // NOLINTBEGIN
        QFuture<CurvatureInfo> future =
            QtConcurrent::mapped(mySegment, std::bind(&FacetCurvature::Compute, &face, sp::_1));
        // NOLINTEND
        QFutureWatcher<CurvatureInfo> watcher;
        watcher.setFuture(future);
        watcher.waitForFinished();
        for (const auto& it : future) {
            myCurvature.push_back(it);
        }
    }
}

#ifdef OPTIMIZE_CURVATURE
namespace MeshCore
{
void GenerateComplementBasis(Eigen::Vector3f& rkU, Eigen::Vector3f& rkV, const Eigen::Vector3f& rkW)
{
    float fInvLength;

    if (fabs(rkW[0]) >= fabs(rkW[1])) {
        // W.x or W.z is the largest magnitude component, swap them
        fInvLength = 1.0 / sqrt(rkW[0] * rkW[0] + rkW[2] * rkW[2]);
        rkU[0] = -rkW[2] * fInvLength;
        rkU[1] = 0.0;
        rkU[2] = +rkW[0] * fInvLength;
        rkV[0] = rkW[1] * rkU[2];
        rkV[1] = rkW[2] * rkU[0] - rkW[0] * rkU[2];
        rkV[2] = -rkW[1] * rkU[0];
    }
    else {
        // W.y or W.z is the largest magnitude component, swap them
        fInvLength = 1.0 / sqrt(rkW[1] * rkW[1] + rkW[2] * rkW[2]);
        rkU[0] = 0.0;
        rkU[1] = +rkW[2] * fInvLength;
        rkU[2] = -rkW[1] * fInvLength;
        rkV[0] = rkW[1] * rkU[2] - rkW[2] * rkU[1];
        rkV[1] = -rkW[0] * rkU[2];
        rkV[2] = rkW[0] * rkU[1];
    }
}
}  // namespace MeshCore

void MeshCurvature::ComputePerVertex()
{
    // get all points
    const MeshPointArray& pts = myKernel.GetPoints();

    MeshCore::MeshRefPointToFacets pt2f(myKernel);
    MeshCore::MeshRefPointToPoints pt2p(myKernel);
    unsigned long numPoints = myKernel.CountPoints();

    myCurvature.clear();
    myCurvature.reserve(numPoints);

    std::vector<Eigen::Vector3f> akNormal(numPoints);
    std::vector<Eigen::Vector3f> akVertex(numPoints);
    for (unsigned long i = 0; i < numPoints; i++) {
        Base::Vector3f n = pt2f.GetNormal(i);
        akNormal[i][0] = n.x;
        akNormal[i][1] = n.y;
        akNormal[i][2] = n.z;
        const Base::Vector3f& p = pts[i];
        akVertex[i][0] = p.x;
        akVertex[i][1] = p.y;
        akVertex[i][2] = p.z;
    }

    // One could iterate over the triangles and then for each vertex of a triangle compute the
    // derivates. One could also iterate over the points and then for each adjacent point calculate
    // the derivates. Both methods must lead to the same values in the above matrices.
    //
    // Iterate over the vertexes
    for (unsigned long i = 0; i < numPoints; i++) {
        Eigen::Matrix3f akDNormal;
        akDNormal.setZero();
        Eigen::Matrix3f akWWTrn;
        akWWTrn.setZero();
        Eigen::Matrix3f akDWTrn;
        akDWTrn.setZero();

        int iV0 = i;
        int iV1;
        const std::set<unsigned long>& nb = pt2p[i];
        for (std::set<unsigned long>::const_iterator it = nb.begin(); it != nb.end(); ++it) {
            iV1 = *it;

            // Compute edge from V0 to V1, project to tangent plane of vertex,
            // and compute difference of adjacent normals.
            Eigen::Vector3f kE = akVertex[iV1] - akVertex[iV0];
            Eigen::Vector3f kW = kE - (kE.dot(akNormal[iV0])) * akNormal[iV0];
            Eigen::Vector3f kD = akNormal[iV1] - akNormal[iV0];
            for (int iRow = 0; iRow < 3; iRow++) {
                for (int iCol = 0; iCol < 3; iCol++) {
                    akWWTrn(iRow, iCol) += 2 * kW[iRow] * kW[iCol];
                    akDWTrn(iRow, iCol) += 2 * kD[iRow] * kW[iCol];
                }
            }
        }

        // Add in N*N^T to W*W^T for numerical stability.  In theory 0*0^T gets
        // added to D*W^T, but of course no update needed in the implementation.
        // Compute the matrix of normal derivatives.
        for (int iRow = 0; iRow < 3; iRow++) {
            for (int iCol = 0; iCol < 3; iCol++) {
                akWWTrn(iRow, iCol) =
                    0.5 * akWWTrn(iRow, iCol) + akNormal[i][iRow] * akNormal[i][iCol];
                akDWTrn(iRow, iCol) *= 0.5;
            }
        }

        akDNormal = akDWTrn * akWWTrn.inverse();

        // If N is a unit-length normal at a vertex, let U and V be unit-length
        // tangents so that {U, V, N} is an orthonormal set.  Define the matrix
        // J = [U | V], a 3-by-2 matrix whose columns are U and V.  Define J^T
        // to be the transpose of J, a 2-by-3 matrix.  Let dN/dX denote the
        // matrix of first-order derivatives of the normal vector field.  The
        // shape matrix is
        //   S = (J^T * J)^{-1} * J^T * dN/dX * J = J^T * dN/dX * J
        // where the superscript of -1 denotes the inverse.  (The formula allows
        // for J built from non-perpendicular vectors.) The matrix S is 2-by-2.
        // The principal curvatures are the eigenvalues of S.  If k is a principal
        // curvature and W is the 2-by-1 eigenvector corresponding to it, then
        // S*W = k*W (by definition).  The corresponding 3-by-1 tangent vector at
        // the vertex is called the principal direction for k, and is J*W.
        // compute U and V given N
        float minCurvature;
        float maxCurvature;
        Base::Vector3f minDirection;
        Base::Vector3f maxDirection;

        Eigen::Vector3f kU, kV;
        Eigen::Vector3f kN = akNormal[i];
        float len = kN.squaredNorm();
        if (len == 0) {
            continue;  // skip
        }
        MeshCore::GenerateComplementBasis(kU, kV, kN);

        // Compute S = J^T * dN/dX * J.  In theory S is symmetric, but
        // because we have estimated dN/dX, we must slightly adjust our
        // calculations to make sure S is symmetric.
        float fS01 = kU.dot(akDNormal * kV);
        float fS10 = kV.dot(akDNormal * kU);
        float fSAvr = 0.5 * (fS01 + fS10);
        Eigen::Matrix2f kS;
        kS(0, 0) = kU.dot(akDNormal * kU);
        kS(0, 1) = fSAvr;
        kS(1, 0) = fSAvr;
        kS(1, 1) = kV.dot(akDNormal * kV);

        // compute the eigenvalues of S (min and max curvatures)
        float fTrace = kS(0, 0) + kS(1, 1);
        float fDet = kS(0, 0) * kS(1, 1) - kS(0, 1) * kS(1, 0);
        float fDiscr = fTrace * fTrace - (4.0) * fDet;
        float fRootDiscr = sqrt(fabs(fDiscr));
        minCurvature = (0.5) * (fTrace - fRootDiscr);
        maxCurvature = (0.5) * (fTrace + fRootDiscr);

        // compute the eigenvectors of S
        Eigen::Vector2f kW0(kS(0, 1), minCurvature - kS(0, 0));
        Eigen::Vector2f kW1(minCurvature - kS(1, 1), kS(1, 0));
        if (kW0.squaredNorm() >= kW1.squaredNorm()) {
            float len = kW0.squaredNorm();
            if (len > 0 && len != 1) {
                kW0.normalize();
            }
            Eigen::Vector3f v = kU * kW0[0] + kV * kW0[1];
            minDirection.Set(v[0], v[1], v[2]);
        }
        else {
            float len = kW1.squaredNorm();
            if (len > 0 && len != 1) {
                kW1.normalize();
            }
            Eigen::Vector3f v = kU * kW1[0] + kV * kW1[1];
            minDirection.Set(v[0], v[1], v[2]);
        }

        kW0 = Eigen::Vector2f(kS(0, 1), maxCurvature - kS(0, 0));
        kW1 = Eigen::Vector2f(maxCurvature - kS(1, 1), kS(1, 0));
        if (kW0.squaredNorm() >= kW1.squaredNorm()) {
            float len = kW0.squaredNorm();
            if (len > 0 && len != 1) {
                kW0.normalize();
            }
            Eigen::Vector3f v = kU * kW0[0] + kV * kW0[1];
            maxDirection.Set(v[0], v[1], v[2]);
        }
        else {
            float len = kW1.squaredNorm();
            if (len > 0 && len != 1) {
                kW1.normalize();
            }
            Eigen::Vector3f v = kU * kW1[0] + kV * kW1[1];
            maxDirection.Set(v[0], v[1], v[2]);
        }

        CurvatureInfo ci;
        ci.fMaxCurvature = maxCurvature;
        ci.cMaxCurvDir = maxDirection;
        ci.fMinCurvature = minCurvature;
        ci.cMinCurvDir = minDirection;
        myCurvature.push_back(ci);
    }
}
#else
void MeshCurvature::ComputePerVertex()
{
    myCurvature.clear();

    // get all points
    std::vector<Wm4::Vector3<double>> aPnts;
    aPnts.reserve(myKernel.CountPoints());
    MeshPointIterator cPIt(myKernel);
    for (cPIt.Init(); cPIt.More(); cPIt.Next()) {
        Wm4::Vector3<double> cP(cPIt->x, cPIt->y, cPIt->z);
        aPnts.push_back(cP);
    }

    // get all point connections
    std::vector<int> aIdx;
    aIdx.reserve(3 * myKernel.CountFacets());
    const MeshFacetArray& raFts = myKernel.GetFacets();
    for (const auto& it : raFts) {
        for (PointIndex point : it._aulPoints) {
            aIdx.push_back(int(point));
        }
    }

    // in case of an empty mesh no curvature can be calculated
    if (myKernel.CountPoints() == 0 || myKernel.CountFacets() == 0) {
        return;
    }

    // compute vertex based curvatures
    Wm4::MeshCurvature<double> meshCurv(myKernel.CountPoints(),
                                        &(aPnts[0]),
                                        myKernel.CountFacets(),
                                        &(aIdx[0]));

    // get curvature information now
    const Wm4::Vector3<double>* aMaxCurvDir = meshCurv.GetMaxDirections();
    const Wm4::Vector3<double>* aMinCurvDir = meshCurv.GetMinDirections();
    const double* aMaxCurv = meshCurv.GetMaxCurvatures();
    const double* aMinCurv = meshCurv.GetMinCurvatures();

    myCurvature.reserve(myKernel.CountPoints());
    for (unsigned long i = 0; i < myKernel.CountPoints(); i++) {
        CurvatureInfo ci;
        ci.cMaxCurvDir = Base::Vector3f((float)aMaxCurvDir[i].X(),
                                        (float)aMaxCurvDir[i].Y(),
                                        (float)aMaxCurvDir[i].Z());
        ci.cMinCurvDir = Base::Vector3f((float)aMinCurvDir[i].X(),
                                        (float)aMinCurvDir[i].Y(),
                                        (float)aMinCurvDir[i].Z());
        ci.fMaxCurvature = (float)aMaxCurv[i];
        ci.fMinCurvature = (float)aMinCurv[i];
        myCurvature.push_back(ci);
    }
}
#endif  // OPTIMIZE_CURVATURE

// --------------------------------------------------------

namespace MeshCore
{
class FitPointCollector: public MeshCollector
{
public:
    explicit FitPointCollector(std::set<PointIndex>& ind)
        : indices(ind)
    {}
    void Append(const MeshCore::MeshKernel& kernel, FacetIndex index) override
    {
        PointIndex ulP1 {}, ulP2 {}, ulP3 {};
        kernel.GetFacetPoints(index, ulP1, ulP2, ulP3);
        indices.insert(ulP1);
        indices.insert(ulP2);
        indices.insert(ulP3);
    }

private:
    std::set<PointIndex>& indices;
};
}  // namespace MeshCore

// --------------------------------------------------------

FacetCurvature::FacetCurvature(const MeshKernel& kernel,
                               const MeshRefPointToFacets& search,
                               float r,
                               unsigned long pt)
    : myKernel(kernel)
    , mySearch(search)
    , myMinPoints(pt)
    , myRadius(r)
{}

CurvatureInfo FacetCurvature::Compute(FacetIndex index) const
{
    Base::Vector3f rkDir0, rkDir1;
    Base::Vector3f rkNormal;

    MeshGeomFacet face = myKernel.GetFacet(index);
    Base::Vector3f face_gravity = face.GetGravityPoint();
    Base::Vector3f face_normal = face.GetNormal();
    std::set<PointIndex> point_indices;
    FitPointCollector collect(point_indices);

    float searchDist = myRadius;
    int attempts = 0;
    do {
        mySearch.Neighbours(index, searchDist, collect);
        if (point_indices.empty()) {
            break;
        }
        float min_points = myMinPoints;
        float use_points = point_indices.size();
        searchDist = searchDist * sqrt(min_points / use_points);
    } while ((point_indices.size() < myMinPoints) && (attempts++ < 3));

    std::vector<Base::Vector3f> fitPoints;
    const MeshPointArray& verts = myKernel.GetPoints();
    fitPoints.reserve(point_indices.size());
    for (PointIndex it : point_indices) {
        fitPoints.push_back(verts[it] - face_gravity);
    }

    float fMin {}, fMax {};
    if (fitPoints.size() >= myMinPoints) {
        SurfaceFit surf_fit;
        surf_fit.AddPoints(fitPoints);
        surf_fit.Fit();
        rkNormal = surf_fit.GetNormal();
        double dMin {}, dMax {}, dDistance {};
        if (surf_fit.GetCurvatureInfo(0.0, 0.0, 0.0, dMin, dMax, rkDir1, rkDir0, dDistance)) {
            fMin = (float)dMin;
            fMax = (float)dMax;
        }
        else {
            fMin = FLT_MAX;
            fMax = FLT_MAX;
        }
    }
    else {
        // too few points => cannot calc any properties
        fMin = FLT_MAX;
        fMax = FLT_MAX;
    }

    CurvatureInfo info;
    if (fMin < fMax) {
        info.fMaxCurvature = fMax;
        info.fMinCurvature = fMin;
        info.cMaxCurvDir = rkDir1;
        info.cMinCurvDir = rkDir0;
    }
    else {
        info.fMaxCurvature = fMin;
        info.fMinCurvature = fMax;
        info.cMaxCurvDir = rkDir0;
        info.cMinCurvDir = rkDir1;
    }

    // Reverse the direction of the normal vector if required
    // (Z component of "local" normal vectors should be opposite in sign to the "local" view vector)
    if (rkNormal * face_normal < 0.0) {
        // Note: Changing the normal directions is similar to flipping over the object.
        // In this case we must adjust the curvature information as well.
        std::swap(info.cMaxCurvDir, info.cMinCurvDir);
        std::swap(info.fMaxCurvature, info.fMinCurvature);
        info.fMaxCurvature *= (-1.0);
        info.fMinCurvature *= (-1.0);
    }

    return info;
}

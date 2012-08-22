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
# include <algorithm>
#endif

#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentMap>
#include <boost/bind.hpp>

#include <Mod/Mesh/App/WildMagic4/Wm4Vector3.h>
#include <Mod/Mesh/App/WildMagic4/Wm4MeshCurvature.h>

#include "Curvature.h"
#include "Algorithm.h"
#include "Approximation.h"
#include "MeshKernel.h"
#include "Iterator.h"
#include "Tools.h"
#include <Base/Sequencer.h>
#include <Base/Tools.h>

using namespace MeshCore;

MeshCurvature::MeshCurvature(const MeshKernel& kernel)
  : myKernel(kernel), myMinPoints(20), myRadius(0.5f)
{
    mySegment.resize(kernel.CountFacets());
    std::generate(mySegment.begin(), mySegment.end(), Base::iotaGen<unsigned long>(0));
}

MeshCurvature::MeshCurvature(const MeshKernel& kernel, const std::vector<unsigned long>& segm)
  : myKernel(kernel), myMinPoints(20), myRadius(0.5f), mySegment(segm)
{
}

void MeshCurvature::ComputePerFace(bool parallel)
{
    Base::Vector3f rkDir0, rkDir1, rkPnt;
    Base::Vector3f rkNormal;
    myCurvature.clear();
    MeshRefPointToFacets search(myKernel);
    FacetCurvature face(myKernel, search, myRadius, myMinPoints);

    if (!parallel) {
        Base::SequencerLauncher seq("Curvature estimation", mySegment.size());
        for (std::vector<unsigned long>::iterator it = mySegment.begin(); it != mySegment.end(); ++it) {
            CurvatureInfo info = face.Compute(*it);
            myCurvature.push_back(info);
            seq.next();
        }
    }
    else {
        QFuture<CurvatureInfo> future = QtConcurrent::mapped
            (mySegment, boost::bind(&FacetCurvature::Compute, &face, _1));
        QFutureWatcher<CurvatureInfo> watcher;
        watcher.setFuture(future);
        watcher.waitForFinished();
        for (QFuture<CurvatureInfo>::const_iterator it = future.begin(); it != future.end(); ++it) {
            myCurvature.push_back(*it);
        }
    }
}

void MeshCurvature::ComputePerVertex()
{
    myCurvature.clear();

    // get all points
    std::vector< Wm4::Vector3<double> > aPnts;
    aPnts.reserve(myKernel.CountPoints());
    MeshPointIterator cPIt(myKernel);
    for (cPIt.Init(); cPIt.More(); cPIt.Next()) {
        Wm4::Vector3<double> cP(cPIt->x, cPIt->y, cPIt->z);
        aPnts.push_back(cP);
    }

    // get all point connections
    std::vector<int> aIdx;
    aIdx.reserve(3*myKernel.CountFacets());
    const MeshFacetArray& raFts = myKernel.GetFacets();
    for (MeshFacetArray::const_iterator jt = raFts.begin(); jt != raFts.end(); ++jt) {
        for (int i=0; i<3; i++) {
            aIdx.push_back((int)jt->_aulPoints[i]);
        }
    }

    // compute vertex based curvatures
    Wm4::MeshCurvature<double> meshCurv(myKernel.CountPoints(), &(aPnts[0]), myKernel.CountFacets(), &(aIdx[0]));

    // get curvature information now
    const Wm4::Vector3<double>* aMaxCurvDir = meshCurv.GetMaxDirections();
    const Wm4::Vector3<double>* aMinCurvDir = meshCurv.GetMinDirections();
    const double* aMaxCurv = meshCurv.GetMaxCurvatures();
    const double* aMinCurv = meshCurv.GetMinCurvatures();

    myCurvature.reserve(myKernel.CountPoints());
    for (unsigned long i=0; i<myKernel.CountPoints(); i++) {
        CurvatureInfo ci;
        ci.cMaxCurvDir = Base::Vector3f((float)aMaxCurvDir[i].X(), (float)aMaxCurvDir[i].Y(), (float)aMaxCurvDir[i].Z());
        ci.cMinCurvDir = Base::Vector3f((float)aMinCurvDir[i].X(), (float)aMinCurvDir[i].Y(), (float)aMinCurvDir[i].Z());
        ci.fMaxCurvature = (float)aMaxCurv[i];
        ci.fMinCurvature = (float)aMinCurv[i];
        myCurvature.push_back(ci);
    }
}

// --------------------------------------------------------

namespace MeshCore {
class FitPointCollector : public MeshCollector
{
public:
    FitPointCollector(std::set<unsigned long>& ind) : indices(ind){}
    virtual void Append(const MeshCore::MeshKernel& kernel, unsigned long index)
    {
        unsigned long ulP1, ulP2, ulP3;
        kernel.GetFacetPoints(index, ulP1, ulP2, ulP3);
        indices.insert(ulP1);
        indices.insert(ulP2);
        indices.insert(ulP3);
    }

private:
    std::set<unsigned long>& indices;
};
}

// --------------------------------------------------------

FacetCurvature::FacetCurvature(const MeshKernel& kernel, const MeshRefPointToFacets& search, float r, unsigned long pt)
  : myKernel(kernel), mySearch(search), myMinPoints(pt), myRadius(r)
{
}

CurvatureInfo FacetCurvature::Compute(unsigned long index) const
{
    Base::Vector3f rkDir0, rkDir1, rkPnt;
    Base::Vector3f rkNormal;

    MeshGeomFacet face = myKernel.GetFacet(index);
    Base::Vector3f face_gravity = face.GetGravityPoint();
    Base::Vector3f face_normal = face.GetNormal();
    std::set<unsigned long> point_indices;
    FitPointCollector collect(point_indices);

    float searchDist = myRadius;
    int attempts=0;
    do {
        mySearch.Neighbours(index, searchDist, collect);
        if (point_indices.empty())
            break;
        float min_points = myMinPoints;
        float use_points = point_indices.size();
        searchDist = searchDist * sqrt(min_points/use_points);
    }
    while((point_indices.size() < myMinPoints) && (attempts++ < 3));

    std::vector<Base::Vector3f> fitPoints;
    const MeshPointArray& verts = myKernel.GetPoints();
    fitPoints.reserve(point_indices.size());
    for (std::set<unsigned long>::iterator it = point_indices.begin(); it != point_indices.end(); ++it) {
        fitPoints.push_back(verts[*it] - face_gravity);
    }

    float fMin, fMax;
    if (fitPoints.size() >= myMinPoints) {
        SurfaceFit surf_fit;
        surf_fit.AddPoints(fitPoints);
        surf_fit.Fit();
        rkNormal = surf_fit.GetNormal();
        double dMin, dMax, dDistance;
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
        std::swap(info.cMaxCurvDir,info.cMinCurvDir);
        std::swap(info.fMaxCurvature,info.fMinCurvature);
        info.fMaxCurvature *= (-1.0);
        info.fMinCurvature *= (-1.0);
    }

    return info;
}

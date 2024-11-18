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

#include "Algorithm.h"
#include "Approximation.h"
#include "MeshKernel.h"  // must be before Visitor.h
#include "Visitor.h"


using namespace MeshCore;


unsigned long MeshKernel::VisitNeighbourFacets(MeshFacetVisitor& rclFVisitor,
                                               FacetIndex ulStartFacet) const
{
    unsigned long ulVisited = 0, ulLevel = 0;
    unsigned long ulCount = _aclFacetArray.size();
    std::vector<FacetIndex> clCurrentLevel, clNextLevel;
    std::vector<FacetIndex>::iterator clCurrIter;
    MeshFacetArray::_TConstIterator clCurrFacet, clNBFacet;

    if (ulStartFacet >= _aclFacetArray.size()) {
        return 0;
    }

    // pick up start point
    clCurrentLevel.push_back(ulStartFacet);
    _aclFacetArray[ulStartFacet].SetFlag(MeshFacet::VISIT);

    // as long as free neighbours
    while (!clCurrentLevel.empty()) {
        // visit all neighbours of the current level
        for (clCurrIter = clCurrentLevel.begin(); clCurrIter < clCurrentLevel.end(); ++clCurrIter) {
            clCurrFacet = _aclFacetArray.begin() + *clCurrIter;

            // visit all neighbours of the current level if not yet done
            for (unsigned short i = 0; i < 3; i++) {
                auto j = clCurrFacet->_aulNeighbours[i];  // index to neighbour facet
                if (j == FACET_INDEX_MAX) {
                    continue;  // no neighbour facet
                }

                if (j >= ulCount) {
                    continue;  // error in data structure
                }

                clNBFacet = _aclFacetArray.begin() + j;

                if (!rclFVisitor.AllowVisit(*clNBFacet, *clCurrFacet, j, ulLevel, i)) {
                    continue;
                }
                if (clNBFacet->IsFlag(MeshFacet::VISIT)) {
                    continue;  // neighbour facet already visited
                }
                else {
                    // visit and mark
                    ulVisited++;
                    clNextLevel.push_back(j);
                    clNBFacet->SetFlag(MeshFacet::VISIT);
                    if (!rclFVisitor.Visit(*clNBFacet, *clCurrFacet, j, ulLevel)) {
                        return ulVisited;
                    }
                }
            }
        }

        clCurrentLevel = clNextLevel;
        clNextLevel.clear();
        ulLevel++;
    }

    return ulVisited;
}

unsigned long MeshKernel::VisitNeighbourFacetsOverCorners(MeshFacetVisitor& rclFVisitor,
                                                          FacetIndex ulStartFacet) const
{
    unsigned long ulVisited = 0, ulLevel = 0;
    MeshRefPointToFacets clRPF(*this);
    const MeshFacetArray& raclFAry = _aclFacetArray;
    MeshFacetArray::_TConstIterator pFBegin = raclFAry.begin();
    std::vector<FacetIndex> aclCurrentLevel, aclNextLevel;

    if (ulStartFacet >= _aclFacetArray.size()) {
        return 0;
    }

    aclCurrentLevel.push_back(ulStartFacet);
    raclFAry[ulStartFacet].SetFlag(MeshFacet::VISIT);

    while (!aclCurrentLevel.empty()) {
        // visit all neighbours of the current level
        for (std::vector<FacetIndex>::iterator pCurrFacet = aclCurrentLevel.begin();
             pCurrFacet < aclCurrentLevel.end();
             ++pCurrFacet) {
            for (int i = 0; i < 3; i++) {
                const MeshFacet& rclFacet = raclFAry[*pCurrFacet];
                const std::set<FacetIndex>& raclNB = clRPF[rclFacet._aulPoints[i]];
                for (FacetIndex pINb : raclNB) {
                    if (!pFBegin[pINb].IsFlag(MeshFacet::VISIT)) {
                        // only visit if VISIT Flag not set
                        ulVisited++;
                        FacetIndex ulFInd = pINb;
                        aclNextLevel.push_back(ulFInd);
                        pFBegin[pINb].SetFlag(MeshFacet::VISIT);
                        if (!rclFVisitor.Visit(pFBegin[pINb],
                                               raclFAry[*pCurrFacet],
                                               ulFInd,
                                               ulLevel)) {
                            return ulVisited;
                        }
                    }
                }
            }
        }
        aclCurrentLevel = aclNextLevel;
        aclNextLevel.clear();
        ulLevel++;
    }

    return ulVisited;
}

unsigned long MeshKernel::VisitNeighbourPoints(MeshPointVisitor& rclPVisitor,
                                               PointIndex ulStartPoint) const
{
    unsigned long ulVisited = 0, ulLevel = 0;
    std::vector<PointIndex> aclCurrentLevel, aclNextLevel;
    std::vector<PointIndex>::iterator clCurrIter;
    MeshPointArray::_TConstIterator pPBegin = _aclPointArray.begin();
    MeshRefPointToPoints clNPs(*this);

    aclCurrentLevel.push_back(ulStartPoint);
    (pPBegin + ulStartPoint)->SetFlag(MeshPoint::VISIT);

    while (!aclCurrentLevel.empty()) {
        // visit all neighbours of the current level
        for (clCurrIter = aclCurrentLevel.begin(); clCurrIter < aclCurrentLevel.end();
             ++clCurrIter) {
            const std::set<PointIndex>& raclNB = clNPs[*clCurrIter];
            for (PointIndex pINb : raclNB) {
                if (!pPBegin[pINb].IsFlag(MeshPoint::VISIT)) {
                    // only visit if VISIT Flag not set
                    ulVisited++;
                    PointIndex ulPInd = pINb;
                    aclNextLevel.push_back(ulPInd);
                    pPBegin[pINb].SetFlag(MeshPoint::VISIT);
                    if (!rclPVisitor.Visit(pPBegin[pINb],
                                           *(pPBegin + (*clCurrIter)),
                                           ulPInd,
                                           ulLevel)) {
                        return ulVisited;
                    }
                }
            }
        }
        aclCurrentLevel = aclNextLevel;
        aclNextLevel.clear();
        ulLevel++;
    }

    return ulVisited;
}

// -------------------------------------------------------------------------

MeshSearchNeighbourFacetsVisitor::MeshSearchNeighbourFacetsVisitor(const MeshKernel& rclMesh,
                                                                   float fRadius,
                                                                   FacetIndex ulStartFacetIdx)
    : _rclMeshBase(rclMesh)
    , _clCenter(rclMesh.GetFacet(ulStartFacetIdx).GetGravityPoint())
    , _fRadius(fRadius)
{}

std::vector<FacetIndex> MeshSearchNeighbourFacetsVisitor::GetAndReset()
{
    MeshAlgorithm(_rclMeshBase).ResetFacetsFlag(_vecFacets, MeshFacet::VISIT);
    return _vecFacets;
}

// -------------------------------------------------------------------------

MeshPlaneVisitor::MeshPlaneVisitor(const MeshKernel& mesh,
                                   FacetIndex index,
                                   float deviation,
                                   std::vector<FacetIndex>& indices)
    : mesh(mesh)
    , indices(indices)
    , max_deviation(deviation)
    , fitter(new PlaneFit)
{
    MeshGeomFacet triangle = mesh.GetFacet(index);
    basepoint = triangle.GetGravityPoint();
    normal = triangle.GetNormal();
    fitter->AddPoint(triangle._aclPoints[0]);
    fitter->AddPoint(triangle._aclPoints[1]);
    fitter->AddPoint(triangle._aclPoints[2]);
}

MeshPlaneVisitor::~MeshPlaneVisitor()
{
    delete fitter;
}

bool MeshPlaneVisitor::AllowVisit(const MeshFacet& face,
                                  const MeshFacet&,
                                  FacetIndex,
                                  unsigned long,
                                  unsigned short)
{
    if (!fitter->Done()) {
        fitter->Fit();
    }
    MeshGeomFacet triangle = mesh.GetFacet(face);
    for (const auto& pnt : triangle._aclPoints) {
        if (fabs(fitter->GetDistanceToPlane(pnt)) > max_deviation) {
            return false;
        }
    }
    return true;
}

bool MeshPlaneVisitor::Visit(const MeshFacet& face,
                             const MeshFacet&,
                             FacetIndex ulFInd,
                             unsigned long)
{
    MeshGeomFacet triangle = mesh.GetFacet(face);
    indices.push_back(ulFInd);
    fitter->AddPoint(triangle.GetGravityPoint());
    return true;
}

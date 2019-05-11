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
# include <algorithm>
# include <map>
#endif

#include "Projection.h"
#include "MeshKernel.h"
#include "Iterator.h"
#include "Algorithm.h"
#include "Grid.h"

#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Sequencer.h>


using namespace MeshCore;


// ------------------------------------------------------------------------

MeshProjection::MeshProjection(const MeshKernel& mesh)
  : kernel(mesh)
{
}

MeshProjection::~MeshProjection()
{
}

bool MeshProjection::bboxInsideRectangle(const Base::BoundBox3f& bbox,
                                         const Base::Vector3f& p1,
                                         const Base::Vector3f& p2,
                                         const Base::Vector3f& view) const
{
    Base::Vector3f dir(p2 - p1);
    Base::Vector3f base(p1), normal(view % dir);
    normal.Normalize(); 

    if (bbox.IsCutPlane(base, normal)) {
        dir.Normalize();
        Base::Vector3f cnt(bbox.GetCenter());

        return (fabs(cnt.DistanceToPlane(p1, dir)) +  fabs(cnt.DistanceToPlane(p2, dir))) <=
               (bbox.CalcDiagonalLength() + (p2 - p1).Length());
    }

    return false;
}

bool MeshProjection::isPointInsideDistance (const Base::Vector3f& p1,
                                            const Base::Vector3f& p2,
                                            const Base::Vector3f& pt) const
{
    // project point on line
    Base::Vector3f proj, dir(p2 - p1);
    Base::Vector3f move(pt - p1);
    proj.ProjectToLine(move, dir);
    proj = pt + proj;
    return (((p1 - proj) * (p2 - proj)) < 0.0f);
}

bool MeshProjection::connectLines(std::list< std::pair<Base::Vector3f, Base::Vector3f> >& cutLines,
                                  const Base::Vector3f& startPoint, const Base::Vector3f& endPoint,
                                  std::vector<Base::Vector3f>& polyline) const
{
    const float fMaxDist = float(sqrt(FLOAT_MAX)); // max. length of a gap
    const float fMinEps  = 1.0e-4f;

    polyline.clear();
    polyline.push_back(startPoint);

    Base::Vector3f curr(startPoint);
    while ((curr != endPoint) && (!cutLines.empty())) {
        std::list< std::pair<Base::Vector3f, Base::Vector3f> >::iterator it, pCurr = cutLines.end();

        // get nearest line
        float  fMin  = fMaxDist * fMaxDist;

        bool  bPos = false;
        for (it = cutLines.begin(); it != cutLines.end(); ++it) {
            float fD1 = Base::DistanceP2(curr, it->first);
            float fD2 = Base::DistanceP2(curr, it->second);
            if (std::min<float>(fD1, fD2) < fMin) {
                pCurr = it;
                bPos  = fD1 < fD2;
                fMin  = std::min<float>(fD1, fD2);
                if (fMin < fMinEps) // abort because next line already found
                    break;
            }
        }

        if (pCurr != cutLines.end()) {
            if (bPos) {
                if (fMin > fMinEps) // gap, insert point
                    polyline.push_back(pCurr->first);
                polyline.push_back(pCurr->second);
                curr = pCurr->second;
            }
            else {
                if (fMin > fMinEps) // gap, insert point
                    polyline.push_back(pCurr->second);
                polyline.push_back(pCurr->first);
                curr = pCurr->first;
            }
        }
        else {
            return false;  // abort because no line was found
        }

        cutLines.erase(pCurr); 
    }

    return true;
}

bool MeshProjection::projectLineOnMesh(const MeshFacetGrid& grid,
                                       const Base::Vector3f& v1, unsigned long f1,
                                       const Base::Vector3f& v2, unsigned long f2,
                                       const Base::Vector3f& vd,
                                       std::vector<Base::Vector3f>& polyline)
{
    Base::Vector3f dir(v2 - v1);
    Base::Vector3f base(v1), normal(vd % dir);
    normal.Normalize();
    dir.Normalize();


    std::vector<unsigned long> facets;

    // special case: start and endpoint inside same facet
    if (f1 == f2) {
        polyline.push_back(v1);
        polyline.push_back(v2);
        return true;
    }

    // cut all facets between the two endpoints
    MeshGridIterator gridIter(grid);
    for (gridIter.Init(); gridIter.More(); gridIter.Next()) {
        // bbox cuts plane
        if (bboxInsideRectangle(gridIter.GetBoundBox(), v1, v2, vd))
            gridIter.GetElements(facets);
    }

    std::sort(facets.begin(), facets.end());
    facets.erase(std::unique(facets.begin(), facets.end()), facets.end());

    // cut all facets with plane
    std::list< std::pair<Base::Vector3f, Base::Vector3f> > cutLine;
    //unsigned long start = 0, end = 0;
    for (std::vector<unsigned long>::iterator it = facets.begin(); it != facets.end(); ++it) {
        Base::Vector3f e1, e2;
        MeshGeomFacet tria = kernel.GetFacet(*it);
        if (bboxInsideRectangle(tria.GetBoundBox(), v1, v2, vd)) {
            if (tria.IntersectWithPlane(base, normal, e1, e2)) {
                if ((*it != f1) && (*it != f2)) {
                    // inside cut line
                    if ((isPointInsideDistance(v1, v2, e1) == false) ||
                        (isPointInsideDistance(v1, v2, e2) == false)) {
                        continue;
                    }

                    cutLine.push_back(std::pair<Base::Vector3f, Base::Vector3f>(e1, e2));
                }
                else {
                    if (*it == f1) { // start facet
                        if (((e2 - v1) * dir) > 0.0f)
                            cutLine.push_back(std::pair<Base::Vector3f, Base::Vector3f>(v1, e2));
                        else
                            cutLine.push_back(std::pair<Base::Vector3f, Base::Vector3f>(v1, e1));

                        //start = it - facets.begin();
                    }

                    if (*it == f2) { // end facet
                        if (((e2 - v2) * -dir) > 0.0f)
                            cutLine.push_back(std::pair<Base::Vector3f, Base::Vector3f>(v2, e2));
                        else
                            cutLine.push_back(std::pair<Base::Vector3f, Base::Vector3f>(v2, e1));

                        //end = it - facets.begin();
                    }
                }
            }
        }
    }

    return connectLines(cutLine, v1, v2, polyline);
}

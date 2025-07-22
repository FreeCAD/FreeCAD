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

#ifndef MESHCORE_CURVATURE_H
#define MESHCORE_CURVATURE_H

#include "Definitions.h"
#include <Base/Vector3D.h>
#include <vector>

namespace MeshCore
{

class MeshKernel;
class MeshRefPointToFacets;

/** Curvature information. */
struct MeshExport CurvatureInfo
{
    float fMaxCurvature, fMinCurvature;
    Base::Vector3f cMaxCurvDir, cMinCurvDir;
};

class MeshExport FacetCurvature
{
public:
    FacetCurvature(const MeshKernel& kernel,
                   const MeshRefPointToFacets& search,
                   float,
                   unsigned long);
    CurvatureInfo Compute(FacetIndex index) const;

private:
    const MeshKernel& myKernel;
    const MeshRefPointToFacets& mySearch;
    unsigned long myMinPoints;
    float myRadius;
};

class MeshExport MeshCurvature
{
public:
    explicit MeshCurvature(const MeshKernel& kernel);
    MeshCurvature(const MeshKernel& kernel, std::vector<FacetIndex> segm);
    float GetRadius() const
    {
        return myRadius;
    }
    void SetRadius(float r)
    {
        myRadius = r;
    }
    void ComputePerFace(bool parallel);
    void ComputePerVertex();
    const std::vector<CurvatureInfo>& GetCurvature() const
    {
        return myCurvature;
    }

private:
    const MeshKernel& myKernel;
    unsigned long myMinPoints;
    float myRadius;
    std::vector<FacetIndex> mySegment;
    std::vector<CurvatureInfo> myCurvature;
};

}  // namespace MeshCore

#endif  // MESHCORE_CURVATURE_H

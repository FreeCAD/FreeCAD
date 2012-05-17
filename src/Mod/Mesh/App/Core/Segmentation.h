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

#ifndef MESHCORE_SEGMENTATION_H
#define MESHCORE_SEGMENTATION_H

#include "MeshKernel.h"
#include "Curvature.h"
#include "Visitor.h"
#include <vector>

namespace MeshCore {

class PlaneFit;
class MeshFacet;
typedef std::vector<unsigned long> MeshSegment;

class MeshExport MeshSurfaceSegment
{
public:
    MeshSurfaceSegment(unsigned long minFacets)
        : minFacets(minFacets) {}
    virtual ~MeshSurfaceSegment() {}
    virtual bool TestFacet (const MeshFacet &rclFacet) const = 0;
    virtual void PrepareFacet(unsigned long);
    void AddSegment(const std::vector<unsigned long>&);
    const std::vector<MeshSegment> GetSegments() const { return segments; }

protected:
    std::vector<MeshSegment> segments;
    unsigned long minFacets;
};

// --------------------------------------------------------

class MeshExport MeshDistanceSurfaceSegment : public MeshSurfaceSegment
{
public:
    MeshDistanceSurfaceSegment(const MeshKernel& mesh, unsigned long minFacets, float tol)
        : MeshSurfaceSegment(minFacets), kernel(mesh), tolerance(tol) {}

protected:
    const MeshKernel& kernel;
    float tolerance;
};

class MeshExport MeshDistancePlanarSegment : public MeshDistanceSurfaceSegment
{
public:
    MeshDistancePlanarSegment(const MeshKernel& mesh, unsigned long minFacets, float tol);
    virtual ~MeshDistancePlanarSegment();
    bool TestFacet (const MeshFacet &rclFacet) const;
    void PrepareFacet(unsigned long);

protected:
    Base::Vector3f basepoint;
    Base::Vector3f normal;
    PlaneFit* fitter;
};

// --------------------------------------------------------

class MeshExport MeshCurvatureSurfaceSegment : public MeshSurfaceSegment
{
public:
    MeshCurvatureSurfaceSegment(const std::vector<CurvatureInfo>& ci, unsigned long minFacets, float tol)
        : MeshSurfaceSegment(minFacets), info(ci), tolerance(tol) {}

protected:
    const std::vector<CurvatureInfo>& info;
    float tolerance;
};

class MeshExport MeshCurvaturePlanarSegment : public MeshCurvatureSurfaceSegment
{
public:
    MeshCurvaturePlanarSegment(const std::vector<CurvatureInfo>& ci, unsigned long minFacets, float tol)
        : MeshCurvatureSurfaceSegment(ci, minFacets, tol) {}
    virtual bool TestFacet (const MeshFacet &rclFacet) const;
};

class MeshExport MeshCurvatureCylindricalSegment : public MeshCurvatureSurfaceSegment
{
public:
    MeshCurvatureCylindricalSegment(const std::vector<CurvatureInfo>& ci, unsigned long minFacets, float tol, float radius)
        : MeshCurvatureSurfaceSegment(ci, minFacets, tol) { curvature = 1/radius;}
    virtual bool TestFacet (const MeshFacet &rclFacet) const;

private:
    float curvature;
};

class MeshExport MeshCurvatureSphericalSegment : public MeshCurvatureSurfaceSegment
{
public:
    MeshCurvatureSphericalSegment(const std::vector<CurvatureInfo>& ci, unsigned long minFacets, float tol, float radius)
        : MeshCurvatureSurfaceSegment(ci, minFacets, tol) { curvature = 1/radius;}
    virtual bool TestFacet (const MeshFacet &rclFacet) const;

private:
    float curvature;
};

class MeshExport MeshCurvatureFreeformSegment : public MeshCurvatureSurfaceSegment
{
public:
    MeshCurvatureFreeformSegment(const std::vector<CurvatureInfo>& ci, unsigned long minFacets, float tol, float c1, float c2)
        : MeshCurvatureSurfaceSegment(ci, minFacets, tol), c1(c1), c2(c2) {}
    virtual bool TestFacet (const MeshFacet &rclFacet) const;

private:
    float c1, c2;
};

class MeshExport MeshSurfaceVisitor : public MeshFacetVisitor
{
public:
    MeshSurfaceVisitor (const MeshSurfaceSegment& segm, std::vector<unsigned long> &indices);
    virtual ~MeshSurfaceVisitor ();
    bool AllowVisit (const MeshFacet& face, const MeshFacet&, 
                     unsigned long, unsigned long, unsigned short neighbourIndex);
    bool Visit (const MeshFacet & face, const MeshFacet &,
                unsigned long ulFInd, unsigned long);

protected:
    std::vector<unsigned long>  &indices;
    const MeshSurfaceSegment& segm;
};

class MeshExport MeshSegmentAlgorithm
{
public:
    MeshSegmentAlgorithm(const MeshKernel& kernel) : myKernel(kernel) {}
    void FindSegments(std::vector<MeshSurfaceSegment*>&);

private:
    const MeshKernel& myKernel;
};

} // MeshCore

#endif // MESHCORE_SEGMENTATION_H

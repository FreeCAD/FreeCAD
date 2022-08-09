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
#include <memory>

namespace MeshCore {

class PlaneFit;
class CylinderFit;
class SphereFit;
class MeshFacet;
typedef std::vector<FacetIndex> MeshSegment;

class MeshExport MeshSurfaceSegment
{
public:
    MeshSurfaceSegment(unsigned long minFacets)
        : minFacets(minFacets) {}
    virtual ~MeshSurfaceSegment() {}
    virtual bool TestFacet (const MeshFacet &rclFacet) const = 0;
    virtual const char* GetType() const = 0;
    virtual void Initialize(FacetIndex);
    virtual bool TestInitialFacet(FacetIndex) const;
    virtual void AddFacet(const MeshFacet& rclFacet);
    void AddSegment(const std::vector<FacetIndex>&);
    const std::vector<MeshSegment>& GetSegments() const { return segments; }
    MeshSegment FindSegment(FacetIndex) const;

protected:
    std::vector<MeshSegment> segments;
    unsigned long minFacets;
};
typedef std::shared_ptr<MeshSurfaceSegment> MeshSurfaceSegmentPtr;

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
    ~MeshDistancePlanarSegment() override;
    bool TestFacet (const MeshFacet& rclFacet) const override;
    const char* GetType() const override { return "Plane"; }
    void Initialize(FacetIndex) override;
    void AddFacet(const MeshFacet& rclFacet) override;

protected:
    Base::Vector3f basepoint;
    Base::Vector3f normal;
    PlaneFit* fitter;
};

class MeshExport AbstractSurfaceFit
{
public:
    AbstractSurfaceFit(){}
    virtual ~AbstractSurfaceFit(){}
    virtual const char* GetType() const = 0;
    virtual void Initialize(const MeshGeomFacet&) = 0;
    virtual bool TestTriangle(const MeshGeomFacet&) const = 0;
    virtual void AddTriangle(const MeshGeomFacet&) = 0;
    virtual bool Done() const = 0;
    virtual float Fit() = 0;
    virtual float GetDistanceToSurface(const Base::Vector3f&) const = 0;
    virtual std::vector<float> Parameters() const = 0;
};

class MeshExport PlaneSurfaceFit : public AbstractSurfaceFit
{
public:
    PlaneSurfaceFit();
    PlaneSurfaceFit(const Base::Vector3f& b, const Base::Vector3f& n);
    ~PlaneSurfaceFit() override;
    const char* GetType() const override { return "Plane"; }
    void Initialize(const MeshGeomFacet&) override;
    bool TestTriangle(const MeshGeomFacet&) const override;
    void AddTriangle(const MeshGeomFacet&) override;
    bool Done() const override;
    float Fit() override;
    float GetDistanceToSurface(const Base::Vector3f&) const override;
    std::vector<float> Parameters() const override;

private:
    Base::Vector3f basepoint;
    Base::Vector3f normal;
    PlaneFit* fitter;
};

class MeshExport CylinderSurfaceFit : public AbstractSurfaceFit
{
public:
    CylinderSurfaceFit();
    CylinderSurfaceFit(const Base::Vector3f& b, const Base::Vector3f& a, float r);
    ~CylinderSurfaceFit() override;
    const char* GetType() const override { return "Cylinder"; }
    void Initialize(const MeshGeomFacet&) override;
    bool TestTriangle(const MeshGeomFacet&) const override;
    void AddTriangle(const MeshGeomFacet&) override;
    bool Done() const override;
    float Fit() override;
    float GetDistanceToSurface(const Base::Vector3f&) const override;
    std::vector<float> Parameters() const override;

private:
    Base::Vector3f basepoint;
    Base::Vector3f axis;
    float radius;
    CylinderFit* fitter;
};

class MeshExport SphereSurfaceFit : public AbstractSurfaceFit
{
public:
    SphereSurfaceFit();
    SphereSurfaceFit(const Base::Vector3f& c, float r);
    ~SphereSurfaceFit() override;
    const char* GetType() const override { return "Sphere"; }
    void Initialize(const MeshGeomFacet&) override;
    bool TestTriangle(const MeshGeomFacet&) const override;
    void AddTriangle(const MeshGeomFacet&) override;
    bool Done() const override;
    float Fit() override;
    float GetDistanceToSurface(const Base::Vector3f&) const override;
    std::vector<float> Parameters() const override;

private:
    Base::Vector3f center;
    float radius;
    SphereFit* fitter;
};

class MeshExport MeshDistanceGenericSurfaceFitSegment : public MeshDistanceSurfaceSegment
{
public:
    MeshDistanceGenericSurfaceFitSegment(AbstractSurfaceFit*, const MeshKernel& mesh,
                                         unsigned long minFacets, float tol);
    ~MeshDistanceGenericSurfaceFitSegment() override;
    bool TestFacet (const MeshFacet& rclFacet) const override;
    const char* GetType() const override { return fitter->GetType(); }
    void Initialize(FacetIndex) override;
    bool TestInitialFacet(FacetIndex) const override;
    void AddFacet(const MeshFacet& rclFacet) override;
    std::vector<float> Parameters() const;

protected:
    AbstractSurfaceFit* fitter;
};

// --------------------------------------------------------

class MeshExport MeshCurvatureSurfaceSegment : public MeshSurfaceSegment
{
public:
    MeshCurvatureSurfaceSegment(const std::vector<CurvatureInfo>& ci, unsigned long minFacets)
        : MeshSurfaceSegment(minFacets), info(ci) {}

protected:
    const std::vector<CurvatureInfo>& info;
};

class MeshExport MeshCurvaturePlanarSegment : public MeshCurvatureSurfaceSegment
{
public:
    MeshCurvaturePlanarSegment(const std::vector<CurvatureInfo>& ci, unsigned long minFacets, float tol)
        : MeshCurvatureSurfaceSegment(ci, minFacets), tolerance(tol) {}
    bool TestFacet (const MeshFacet &rclFacet) const override;
    const char* GetType() const override { return "Plane"; }

private:
    float tolerance;
};

class MeshExport MeshCurvatureCylindricalSegment : public MeshCurvatureSurfaceSegment
{
public:
    MeshCurvatureCylindricalSegment(const std::vector<CurvatureInfo>& ci, unsigned long minFacets,
                                    float tolMin, float tolMax, float curv)
        : MeshCurvatureSurfaceSegment(ci, minFacets), toleranceMin(tolMin), toleranceMax(tolMax) { curvature = curv;}
    bool TestFacet (const MeshFacet &rclFacet) const override;
    const char* GetType() const override { return "Cylinder"; }

private:
    float curvature;
    float toleranceMin;
    float toleranceMax;
};

class MeshExport MeshCurvatureSphericalSegment : public MeshCurvatureSurfaceSegment
{
public:
    MeshCurvatureSphericalSegment(const std::vector<CurvatureInfo>& ci, unsigned long minFacets, float tol, float curv)
        : MeshCurvatureSurfaceSegment(ci, minFacets), tolerance(tol) { curvature = curv;}
    bool TestFacet (const MeshFacet &rclFacet) const override;
    const char* GetType() const override { return "Sphere"; }

private:
    float curvature;
    float tolerance;
};

class MeshExport MeshCurvatureFreeformSegment : public MeshCurvatureSurfaceSegment
{
public:
    MeshCurvatureFreeformSegment(const std::vector<CurvatureInfo>& ci, unsigned long minFacets,
                                 float tolMin, float tolMax, float c1, float c2)
        : MeshCurvatureSurfaceSegment(ci, minFacets), c1(c1), c2(c2),
          toleranceMin(tolMin), toleranceMax(tolMax) {}
    bool TestFacet (const MeshFacet &rclFacet) const override;
    const char* GetType() const override { return "Freeform"; }

private:
    float c1, c2;
    float toleranceMin;
    float toleranceMax;
};

class MeshExport MeshSurfaceVisitor : public MeshFacetVisitor
{
public:
    MeshSurfaceVisitor (MeshSurfaceSegment& segm, std::vector<FacetIndex> &indices);
    ~MeshSurfaceVisitor () override;
    bool AllowVisit (const MeshFacet& face, const MeshFacet&, 
                     FacetIndex, unsigned long, unsigned short neighbourIndex) override;
    bool Visit (const MeshFacet & face, const MeshFacet &,
                FacetIndex ulFInd, unsigned long) override;

protected:
    std::vector<FacetIndex>  &indices;
    MeshSurfaceSegment& segm;
};

class MeshExport MeshSegmentAlgorithm
{
public:
    MeshSegmentAlgorithm(const MeshKernel& kernel) : myKernel(kernel) {}
    void FindSegments(std::vector<MeshSurfaceSegmentPtr>&);

private:
    const MeshKernel& myKernel;
};

} // MeshCore

#endif // MESHCORE_SEGMENTATION_H

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
class CylinderFit;
class SphereFit;
class MeshFacet;
typedef std::vector<unsigned long> MeshSegment;

class MeshExport MeshSurfaceSegment
{
public:
    MeshSurfaceSegment(unsigned long minFacets)
        : minFacets(minFacets) {}
    virtual ~MeshSurfaceSegment() {}
    virtual bool TestFacet (const MeshFacet &rclFacet) const = 0;
    virtual const char* GetType() const = 0;
    virtual void Initialize(unsigned long);
    virtual bool TestInitialFacet(unsigned long) const;
    virtual void AddFacet(const MeshFacet& rclFacet);
    void AddSegment(const std::vector<unsigned long>&);
    const std::vector<MeshSegment>& GetSegments() const { return segments; }
    MeshSegment FindSegment(unsigned long) const;

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
    bool TestFacet (const MeshFacet& rclFacet) const;
    const char* GetType() const { return "Plane"; }
    void Initialize(unsigned long);
    void AddFacet(const MeshFacet& rclFacet);

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
};

class MeshExport PlaneSurfaceFit : public AbstractSurfaceFit
{
public:
    PlaneSurfaceFit();
    PlaneSurfaceFit(const Base::Vector3f& b, const Base::Vector3f& n);
    ~PlaneSurfaceFit();
    const char* GetType() const { return "Plane"; }
    void Initialize(const MeshGeomFacet&);
    bool TestTriangle(const MeshGeomFacet&) const;
    void AddTriangle(const MeshGeomFacet&);
    bool Done() const;
    float Fit();
    float GetDistanceToSurface(const Base::Vector3f&) const;

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
    ~CylinderSurfaceFit();
    const char* GetType() const { return "Cylinder"; }
    void Initialize(const MeshGeomFacet&);
    bool TestTriangle(const MeshGeomFacet&) const;
    void AddTriangle(const MeshGeomFacet&);
    bool Done() const;
    float Fit();
    float GetDistanceToSurface(const Base::Vector3f&) const;

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
    ~SphereSurfaceFit();
    const char* GetType() const { return "Sphere"; }
    void Initialize(const MeshGeomFacet&);
    bool TestTriangle(const MeshGeomFacet&) const;
    void AddTriangle(const MeshGeomFacet&);
    bool Done() const;
    float Fit();
    float GetDistanceToSurface(const Base::Vector3f&) const;

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
    virtual ~MeshDistanceGenericSurfaceFitSegment();
    bool TestFacet (const MeshFacet& rclFacet) const;
    const char* GetType() const { return fitter->GetType(); }
    void Initialize(unsigned long);
    bool TestInitialFacet(unsigned long) const;
    void AddFacet(const MeshFacet& rclFacet);

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
    virtual bool TestFacet (const MeshFacet &rclFacet) const;
    virtual const char* GetType() const { return "Plane"; }

private:
    float tolerance;
};

class MeshExport MeshCurvatureCylindricalSegment : public MeshCurvatureSurfaceSegment
{
public:
    MeshCurvatureCylindricalSegment(const std::vector<CurvatureInfo>& ci, unsigned long minFacets,
                                    float tolMin, float tolMax, float curv)
        : MeshCurvatureSurfaceSegment(ci, minFacets), toleranceMin(tolMin), toleranceMax(tolMax) { curvature = curv;}
    virtual bool TestFacet (const MeshFacet &rclFacet) const;
    virtual const char* GetType() const { return "Cylinder"; }

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
    virtual bool TestFacet (const MeshFacet &rclFacet) const;
    virtual const char* GetType() const { return "Sphere"; }

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
    virtual bool TestFacet (const MeshFacet &rclFacet) const;
    virtual const char* GetType() const { return "Freeform"; }

private:
    float c1, c2;
    float toleranceMin;
    float toleranceMax;
};

class MeshExport MeshSurfaceVisitor : public MeshFacetVisitor
{
public:
    MeshSurfaceVisitor (MeshSurfaceSegment& segm, std::vector<unsigned long> &indices);
    virtual ~MeshSurfaceVisitor ();
    bool AllowVisit (const MeshFacet& face, const MeshFacet&, 
                     unsigned long, unsigned long, unsigned short neighbourIndex);
    bool Visit (const MeshFacet & face, const MeshFacet &,
                unsigned long ulFInd, unsigned long);

protected:
    std::vector<unsigned long>  &indices;
    MeshSurfaceSegment& segm;
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

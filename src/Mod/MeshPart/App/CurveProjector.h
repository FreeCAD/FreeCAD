/***************************************************************************
 *   Copyright (c) 2008 Juergen Riegel <juergen.riegel@web.de>             *
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


#ifndef _CurveProjector_h_
#define _CurveProjector_h_

#ifdef FC_USE_GTS
#  include <gts.h>
#endif

#include <gp_Pln.hxx>
#include <TopoDS_Edge.hxx>

#include <Base/Vector3D.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/MeshPart/MeshPartGlobal.h>

namespace MeshCore
{
class MeshKernel;
class MeshGeomFacet;
class MeshFacetGrid;
}

using MeshCore::MeshKernel;
using MeshCore::MeshGeomFacet;

namespace MeshPart
{

/** The father of all projection algorithms
 */
class MeshPartExport CurveProjector
{
public:
  CurveProjector(const TopoDS_Shape &aShape, const MeshKernel &pMesh);
  virtual ~CurveProjector() {}

  struct FaceSplitEdge
  {
    MeshCore::FacetIndex ulFaceIndex;
    Base::Vector3f p1,p2;
  };

  template<class T>
    struct TopoDSLess {
    bool operator()(const T& x, const T& y) const { 
      return x.HashCode(INT_MAX-1) < y.HashCode(INT_MAX-1);
    }
  };

  typedef std::map<TopoDS_Edge, std::vector<FaceSplitEdge>,TopoDSLess<TopoDS_Edge> > result_type;


  result_type &result(void) {return  mvEdgeSplitPoints;}

  void writeIntersectionPointsToFile(const char *name="export_pts.asc");

protected:
  virtual void Do()=0;
  const TopoDS_Shape &_Shape;
  const MeshKernel &_Mesh;
  result_type mvEdgeSplitPoints;

};


/** Project by intersection face planes with the curve
 */
class MeshPartExport CurveProjectorShape: public CurveProjector
{
public:
  CurveProjectorShape(const TopoDS_Shape &aShape, const MeshKernel &pMesh);
  virtual ~CurveProjectorShape() {}

  void projectCurve(const TopoDS_Edge& aEdge,
                    std::vector<FaceSplitEdge> &vSplitEdges);

  bool findStartPoint(const MeshKernel &MeshK,const Base::Vector3f &Pnt,Base::Vector3f &Rslt,MeshCore::FacetIndex &FaceIndex);



protected:
  virtual void Do();
};



/** Project by projecting a sampled curve to the mesh
 */
class MeshPartExport CurveProjectorSimple: public CurveProjector
{
public:
  CurveProjectorSimple(const TopoDS_Shape &aShape, const MeshKernel &pMesh);
  virtual ~CurveProjectorSimple() {}

  /// helper to discredicice a Edge...
  void GetSampledCurves( const TopoDS_Edge& aEdge, std::vector<Base::Vector3f>& rclPoints, unsigned long ulNbOfPoints = 30);


  void projectCurve(const TopoDS_Edge& aEdge,
                    const std::vector<Base::Vector3f> &rclPoints,
                    std::vector<FaceSplitEdge> &vSplitEdges);

  bool findStartPoint(const MeshKernel &MeshK,const Base::Vector3f &Pnt,Base::Vector3f &Rslt,MeshCore::FacetIndex &FaceIndex);



protected:
  virtual void Do();
};

/** Project by projecting a sampled curve to the mesh
 */
class MeshPartExport CurveProjectorWithToolMesh: public CurveProjector
{
public:
  struct LineSeg {
    Base::Vector3f p;
    Base::Vector3f n;
  };

  CurveProjectorWithToolMesh(const TopoDS_Shape &aShape, const MeshKernel &pMesh,MeshKernel &rToolMesh);
  virtual ~CurveProjectorWithToolMesh() {}


  void makeToolMesh(const TopoDS_Edge& aEdge,std::vector<MeshGeomFacet> &cVAry );


  MeshKernel &ToolMesh;

protected:
  virtual void Do();
};

/**
 * The MeshProjection class projects a shape onto a mesh.
 * @author Werner Mayer
 */
class MeshPartExport MeshProjection
{
public:
    /// Helper class
    struct SplitEdge
    {
        MeshCore::PointIndex uE0, uE1; /**< start and endpoint of an edge */
        Base::Vector3f cPt; /**< Point on edge (\a uE0, \a uE1) */
    };
    struct Edge
    {
        Base::Vector3f cPt1;
        Base::Vector3f cPt2;
    };
    struct PolyLine
    {
        std::vector<Base::Vector3f> points;
    };

    /// Construction
    MeshProjection(const MeshKernel& rMesh);
    /// Destruction
    ~MeshProjection();

    /**
     * @brief findSectionParameters
     * Find the parameters of the edge where when projecting the corresponding point will lie
     * on an edge of the mesh.
     * @param edge
     * @param dir
     * @param parameters
     */
    void findSectionParameters(const TopoDS_Edge& edge, const Base::Vector3f& dir, std::set<double>& parameters) const;
    void discretize(const TopoDS_Edge& aEdge, std::vector<Base::Vector3f>& polyline, std::size_t minPoints=2) const;
    /**
     * Searches all edges that intersect with the projected curve \a aShape. Therefore \a aShape must
     * contain shapes of type TopoDS_Edge, other shape types are ignored. A possible solution is
     * taken if the distance between the curve point and the projected point is <= \a fMaxDist.
     */
    void projectToMesh (const TopoDS_Shape &aShape, float fMaxDist, std::vector<PolyLine>& rPolyLines) const;
    /**
     * @brief projectOnMesh
     * Projects the given points onto the mesh along a given direction. The points can can be projected
     * will be saved to \a pointsOut
     * @brief projectOnMesh
     * @param pointsIn
     * @param dir
     * @param tolerance
     * @param pointsOut
     */
    void projectOnMesh(const std::vector<Base::Vector3f>& pointsIn, const Base::Vector3f& dir,
                       float tolerance, std::vector<Base::Vector3f>& pointsOut) const;
    /**
     * Project all edges of the shape onto the mesh using parallel projection.
     */
    void projectParallelToMesh (const TopoDS_Shape &aShape, const Base::Vector3f& dir, std::vector<PolyLine>& rPolyLines) const;
    /**
     * Project all polylines onto the mesh using parallel projection.
     */
    void projectParallelToMesh (const std::vector<PolyLine>& aEdges, const Base::Vector3f& dir, std::vector<PolyLine>& rPolyLines) const;
    /**
     * Cuts the mesh at the curve defined by \a aShape. This method call @ref projectToMesh() to get the
     * split the facet at the found points. @see projectToMesh() for more details.
     */
    void splitMeshByShape (const TopoDS_Shape &aShape, float fMaxDist) const;

protected:
    void projectEdgeToEdge(const TopoDS_Edge &aCurve, float fMaxDist, const MeshCore::MeshFacetGrid& rGrid,
                           std::vector<SplitEdge>& rSplitEdges) const;
    bool findIntersection(const Edge&, const Edge&, const Base::Vector3f& dir, Base::Vector3f& res) const;

private:
    const MeshKernel& _rcMesh;
};

} // namespace MeshPart

#endif

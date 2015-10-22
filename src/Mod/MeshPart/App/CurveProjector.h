/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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

#include <Base/Vector3D.h>


class TopoDS_Edge;
class TopoDS_Shape;
#include <TopoDS_Edge.hxx>

#include <Mod/Mesh/App/Mesh.h>

namespace MeshCore
{
class MeshKernel;
class MeshGeomFacet;
};

using MeshCore::MeshKernel;
using MeshCore::MeshGeomFacet;

namespace MeshPart
{

/** The father of all projection algorithems
 */
class MeshPartExport CurveProjector
{
public:
  CurveProjector(const TopoDS_Shape &aShape, const MeshKernel &pMesh);
  virtual ~CurveProjector() {}

  struct FaceSplitEdge
  {
    unsigned long ulFaceIndex;
    Base::Vector3f p1,p2;
  };

  template<class T>
    struct TopoDSLess : public std::binary_function<T, T, bool> {
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

  bool findStartPoint(const MeshKernel &MeshK,const Base::Vector3f &Pnt,Base::Vector3f &Rslt,unsigned long &FaceIndex);



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

  bool findStartPoint(const MeshKernel &MeshK,const Base::Vector3f &Pnt,Base::Vector3f &Rslt,unsigned long &FaceIndex);



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

} // namespace MeshPart

#endif

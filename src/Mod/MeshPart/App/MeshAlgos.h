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

#ifndef _MeshAlgos_h_
#define _MeshAlgos_h_

#ifdef FC_USE_GTS
#include <gts.h>
#endif

#include <vector>

#include "CurveProjector.h"


class TopoDS_Edge;
class TopoDS_Shape;

namespace MeshCore
{
class MeshKernel;
}

using MeshCore::MeshKernel;

namespace MeshPart
{

/** The mesh algorithms container class
 */
class MeshPartExport MeshAlgos
{
public:
    /** Calculate per Vertex normals and adds the Normal property bag
     */
    static void offset(MeshCore::MeshKernel* Mesh, float fSize);
    static void offsetSpecial2(MeshCore::MeshKernel* Mesh, float fSize);
    static void offsetSpecial(MeshCore::MeshKernel* Mesh, float fSize, float zmax, float zmin);

    /** Coarsen the mesh
     */
    static void coarsen(MeshCore::MeshKernel* Mesh, float f);

    /** makes a boolean add
     * The int Type stears the boolean oberation: 0=add;1=intersection;2=diff
     */
    static MeshCore::MeshKernel* boolean(MeshCore::MeshKernel* Mesh1,
                                         MeshCore::MeshKernel* Mesh2,
                                         MeshCore::MeshKernel* pResult,
                                         int Type = 0);

#ifdef FC_USE_GTS

    /** Creates a GTS Surface from a MeshKernel
     */
    static GtsSurface* createGTSSurface(MeshCore::MeshKernel* Mesh);

    /** Creates a GTS Surface from a MeshKernel
     */

    static void fillMeshFromGTSSurface(MeshCore::MeshKernel* pMesh, GtsSurface* pSurface);
#endif

    static void cutByShape(const TopoDS_Shape& aShape,
                           const MeshCore::MeshKernel* pMesh,
                           MeshCore::MeshKernel* pToolMesh);

    /// helper to discredicice a Edge...
    static void GetSampledCurves(const TopoDS_Edge& aEdge,
                                 std::vector<Base::Vector3f>& rclPoints,
                                 unsigned long ulNbOfPoints = 30);

    /// creates a mesh loft on base of a curve and an up vector
    static void LoftOnCurve(MeshCore::MeshKernel& ResultMesh,
                            const TopoDS_Shape& Shape,
                            const std::vector<Base::Vector3f>& poly,
                            const Base::Vector3f& up = Base::Vector3f(0, 0, 1),
                            float MaxSize = 0.1);

    /*
    struct FaceSplitEdge
    {
      unsigned long ulFaceIndex;
      Base::Vector3f p1,p2;
    };

    static void projectCurve( MeshWithProperty* pMesh,
                                         const TopoDS_Edge& aEdge,
                                         const std::vector<Base::Vector3f> &rclPoints,
                                         std::vector<FaceSplitEdge> &vSplitEdges);
  */
    static void cutByCurve(MeshCore::MeshKernel* pMesh,
                           const std::vector<CurveProjector::FaceSplitEdge>& vSplitEdges);
    /*
      static bool projectPointToMesh(MeshKernel &MeshK,const Base::Vector3f &Pnt,Base::Vector3f
      &Rslt,unsigned long &FaceIndex);
    */
};

}  // namespace MeshPart

#endif

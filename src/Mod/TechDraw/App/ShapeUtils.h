/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAW_SHAPEUTILS_H
#define TECHDRAW_SHAPEUTILS_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <memory>
#include <string>
#include <vector>

#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>

#include <Base/Vector3D.h>

//! a class to contain useful shape manipulations. these methods were originally
//  in GeometryObject.

namespace TechDraw
{
class DrawViewPart;
class DrawViewDetail;
class DrawView;
class CosmeticVertex;
class CosmeticEdge;
class BaseGeom;
class Vector;
class Face;
class Vertex;

class TechDrawExport ShapeUtils
{
public:
    //! scales & mirrors a shape about a center
    static TopoDS_Shape mirrorShapeVec( const TopoDS_Shape& input,
                                                       const Base::Vector3d& inputCenter = Base::Vector3d(0.0, 0.0, 0.0),
                                                       double scale = 1.0);
//! scales & mirrors a shape about a center
    static TopoDS_Shape  mirrorShapeVec(TopoDS_Shape& input, const Base::Vector3d& inputCenter = Base::Vector3d(0.0, 0.0, 0.0),
    double scale = 1.0);

//! mirrors a shape around the y axis.  this is need to deal with Qt's odd coordinate choice where the Y-axis
//! has increasing values in the down direction. also performs a scaling of the input shaft
    static TopoDS_Shape mirrorShape(const TopoDS_Shape& input,
                                        const gp_Pnt& inputCenter = gp_Pnt(0.0, 0.0, 0.0),
                                        double scale = 1.0);
    //! another mirroring routine that modifies the shape to conform with the Qt coordinate system.
    static TopoDS_Shape invertGeometry(const TopoDS_Shape s);

//! scales a shape uniformly in all directions
    static TopoDS_Shape scaleShape(const TopoDS_Shape& input, double scale);

//! rotates a shape around the Z axis of a coordinate system
    static TopoDS_Shape rotateShape(const TopoDS_Shape& input, const gp_Ax2& coordSys,
                                        double rotAngle);

//! moves a shape in a direction and distance specified by the motion parameter
    static TopoDS_Shape moveShape(const TopoDS_Shape& input, const Base::Vector3d& motion);

//! move a shape such that its centroid is aligned with the origin point of a CoordinateSystem
    static TopoDS_Shape centerShapeXY(const TopoDS_Shape& inShape, const gp_Ax2& coordSys);

//! Returns the centroid of shape, as viewed according to direction
    static gp_Pnt findCentroid(const TopoDS_Shape& shape);
    static gp_Pnt findCentroid(const TopoDS_Shape& shape, const Base::Vector3d& direction);
    static gp_Pnt findCentroid(const TopoDS_Shape& shape, const gp_Ax2& viewAxis);
    static Base::Vector3d findCentroidVec(const TopoDS_Shape& shape,
                                              const Base::Vector3d& direction);
    static Base::Vector3d findCentroidVec(const TopoDS_Shape& shape, const gp_Ax2& cs);
    static gp_Pnt findCentroidXY(const TopoDS_Shape& shape, const gp_Ax2& coordSys);


//! creates a RH coordinate system with the origin at origin and the Z axis along direction.
//  the flip will cause the Z axis to be the reversed of direction
    static gp_Ax2 getViewAxis(const Base::Vector3d origin, const Base::Vector3d& direction,
                                  const bool flip = true);
    static  gp_Ax2 getViewAxis(const Base::Vector3d origin, const Base::Vector3d& direction,
                                  const Base::Vector3d& xAxis, const bool flip = true);
    static gp_Ax2 legacyViewAxis1(const Base::Vector3d origin, const Base::Vector3d& direction,
                                      const bool flip = true);

    static TopoDS_Shape projectSimpleShape(const TopoDS_Shape& shape, const gp_Ax2& CS);
    static TopoDS_Shape simpleProjection(const TopoDS_Shape& shape, const gp_Ax2& projCS);
    static TopoDS_Shape projectFace(const TopoDS_Shape& face, const gp_Ax2& CS);

    static std::pair<Base::Vector3d, Base::Vector3d> getEdgeEnds(TopoDS_Edge edge);

    static bool isShapeReallyNull(TopoDS_Shape shape);
};

}
#endif

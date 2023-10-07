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

//! a class to contain useful shape manipulations. these methods were originally
//  in GeometryObject.


#include "PreCompiled.h"

#ifndef _PreComp_
#include <BRepAlgo_NormalProjection.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRepLProp_CurveTool.hxx>
#include <BRepLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <HLRBRep_PolyAlgo.hxx>
#include <HLRBRep_PolyHLRToShape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#endif// #ifndef _PreComp_

#include <algorithm>
#include <chrono>

#include <Base/Console.h>

#include "DrawUtil.h"
#include "ShapeUtils.h"

using namespace TechDraw;
using namespace std;

using DU = DrawUtil;


//! gets a coordinate system that matches view system used in 3D with +Z up (or +Y up if necessary)
//! used for individual views, but not secondary views in projection groups
//! flip determines Y mirror or not.
// getViewAxis 1
gp_Ax2 ShapeUtils::getViewAxis(const Base::Vector3d origin, const Base::Vector3d& direction,
                             const bool flip)
{
    //    Base::Console().Message("GO::getViewAxis() - 1 - use only with getLegacyX\n");
    (void)flip;
    gp_Ax2 viewAxis;
    gp_Pnt inputCenter(origin.x, origin.y, origin.z);
    Base::Vector3d stdZ(0.0, 0.0, 1.0);
    Base::Vector3d stdOrg(0.0, 0.0, 0.0);
    Base::Vector3d cross = direction;
    if (DU::checkParallel(direction, stdZ)) {
        cross = Base::Vector3d(1.0, 0.0, 0.0);
    }
    else {
        cross.Normalize();
        cross = cross.Cross(stdZ);
    }

    if (cross.IsEqual(stdOrg, FLT_EPSILON)) {
        viewAxis = gp_Ax2(inputCenter, gp_Dir(direction.x, direction.y, direction.z));
        return viewAxis;
    }

    viewAxis = gp_Ax2(inputCenter, gp_Dir(direction.x, direction.y, direction.z),
                      gp_Dir(cross.x, cross.y, cross.z));
    return viewAxis;
}

//! gets a coordinate system specified by Z and X directions
//getViewAxis 2
gp_Ax2 ShapeUtils::getViewAxis(const Base::Vector3d origin, const Base::Vector3d& direction,
                             const Base::Vector3d& xAxis, const bool flip)
{
    //    Base::Console().Message("GO::getViewAxis() - 2\n");
    (void)flip;
    gp_Pnt inputCenter(origin.x, origin.y, origin.z);
    return gp_Ax2(inputCenter,
                  gp_Dir(direction.x, direction.y, direction.z),
                  gp_Dir(xAxis.x, xAxis.y, xAxis.z));
}

// was getViewAxis 1
// getViewAxis as used before XDirection property adopted
gp_Ax2 ShapeUtils::legacyViewAxis1(const Base::Vector3d origin, const Base::Vector3d& direction,
                                 const bool flip)
{
    //    Base::Console().Message("GO::legacyViewAxis1()\n");
    gp_Pnt inputCenter(origin.x, origin.y, origin.z);
    Base::Vector3d stdZ(0.0, 0.0, 1.0);
    Base::Vector3d stdOrg(0.0, 0.0, 0.0);
    Base::Vector3d flipDirection(direction.x, -direction.y, direction.z);
    if (!flip) {
        flipDirection = Base::Vector3d(direction.x, direction.y, direction.z);
    }
    Base::Vector3d cross = flipDirection;
    //    //special case
    if (DU::checkParallel(flipDirection, stdZ)) {
        cross = Base::Vector3d(1.0, 0.0, 0.0);
    }
    else {
        cross.Normalize();
        cross = cross.Cross(stdZ);
    }

    if (cross.IsEqual(stdOrg, FLT_EPSILON)) {
        return gp_Ax2(inputCenter, gp_Dir(flipDirection.x, flipDirection.y, flipDirection.z));
    }

    gp_Ax2 viewAxis = gp_Ax2(inputCenter,
                             gp_Dir(flipDirection.x, flipDirection.y, flipDirection.z),
                             gp_Dir(cross.x, cross.y, cross.z));

    //this bit is to handle the old mirror Y logic, but it messes up
    //some old files.
    gp_Trsf mirrorXForm;
    gp_Ax2 mirrorCS(inputCenter, gp_Dir(0, -1, 0));
    mirrorXForm.SetMirror(mirrorCS);
    viewAxis = viewAxis.Transformed(mirrorXForm);

    return viewAxis;
}

//! Returns the centroid of shape based on R3
gp_Pnt ShapeUtils::findCentroid(const TopoDS_Shape& shape)
{
    Bnd_Box tBounds;
    tBounds.SetGap(0.0);
    BRepBndLib::AddOptimal(shape, tBounds, true, false);

    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    tBounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    Standard_Real x = (xMin + xMax) / 2.0, y = (yMin + yMax) / 2.0, z = (zMin + zMax) / 2.0;

    return gp_Pnt(x, y, z);
}

//! Returns the centroid of shape, as viewed according to direction
gp_Pnt ShapeUtils::findCentroid(const TopoDS_Shape& shape, const Base::Vector3d& direction)
{
    //    Base::Console().Message("GO::findCentroid() - 1\n");
    Base::Vector3d origin(0.0, 0.0, 0.0);
    gp_Ax2 viewAxis = getViewAxis(origin, direction);
    return findCentroid(shape, viewAxis);
}

//! Returns the centroid of shape, as viewed according to direction
gp_Pnt ShapeUtils::findCentroid(const TopoDS_Shape& shape, const gp_Ax2& viewAxis)
{
    //    Base::Console().Message("GO::findCentroid() - 2\n");

    gp_Trsf tempTransform;
    tempTransform.SetTransformation(viewAxis);
    BRepBuilderAPI_Transform builder(shape, tempTransform);

    Bnd_Box tBounds;
    tBounds.SetGap(0.0);
    BRepBndLib::AddOptimal(builder.Shape(), tBounds, true, false);

    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    tBounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    Standard_Real x = (xMin + xMax) / 2.0, y = (yMin + yMax) / 2.0, z = (zMin + zMax) / 2.0;

    // Get centroid back into object space
    tempTransform.Inverted().Transforms(x, y, z);

    return gp_Pnt(x, y, z);
}

Base::Vector3d ShapeUtils::findCentroidVec(const TopoDS_Shape& shape, const Base::Vector3d& direction)
{
    //    Base::Console().Message("GO::findCentroidVec() - 1\n");
    gp_Pnt p = ShapeUtils::findCentroid(shape, direction);
    return Base::Vector3d(p.X(), p.Y(), p.Z());
}

Base::Vector3d ShapeUtils::findCentroidVec(const TopoDS_Shape& shape, const gp_Ax2& cs)
{
    //    Base::Console().Message("GO::findCentroidVec() - 2\n");
    gp_Pnt p = ShapeUtils::findCentroid(shape, cs);
    return Base::Vector3d(p.X(), p.Y(), p.Z());
}

//! Returns the XY plane center of shape with respect to coordSys
gp_Pnt ShapeUtils::findCentroidXY(const TopoDS_Shape& shape, const gp_Ax2& coordSys)
{
    //    Base::Console().Message("GO::findCentroid() - 2\n");

    gp_Trsf tempTransform;
    tempTransform.SetTransformation(coordSys);
    BRepBuilderAPI_Transform builder(shape, tempTransform);

    Bnd_Box tBounds;
    tBounds.SetGap(0.0);
    BRepBndLib::AddOptimal(builder.Shape(), tBounds, true, false);

    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    tBounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    Standard_Real x = (xMin + xMax) / 2.0, y = (yMin + yMax) / 2.0, z = 0.0;

    // Get "centroid" back into object space
    tempTransform.Inverted().Transforms(x, y, z);

    return gp_Pnt(x, y, z);
}

//!scales & mirrors a shape about a center
TopoDS_Shape ShapeUtils::mirrorShapeVec(const TopoDS_Shape& input, const Base::Vector3d& inputCenter,
                                      double scale)
{
    gp_Pnt gInput(inputCenter.x, inputCenter.y, inputCenter.z);
    return ShapeUtils::mirrorShape(input, gInput, scale);
}

TopoDS_Shape ShapeUtils::mirrorShape(const TopoDS_Shape& input, const gp_Pnt& inputCenter,
                                   double scale)
{
    TopoDS_Shape transShape;
    if (input.IsNull()) {
        return transShape;
    }
    try {
        // Make tempTransform scale the object around it's centre point and
        // mirror about the Y axis
        gp_Trsf tempTransform;
        //BRepBuilderAPI_Transform will loop forever if asked to use 0.0 as scale
        if (scale <= 0.0) {
            tempTransform.SetScale(inputCenter, 1.0);
        }
        else {
            tempTransform.SetScale(inputCenter, scale);
        }
        gp_Trsf mirrorTransform;
        mirrorTransform.SetMirror(gp_Ax2(inputCenter, gp_Dir(0, -1, 0)));
        tempTransform.Multiply(mirrorTransform);

        // Apply that transform to the shape.  This should preserve the centre.
        BRepBuilderAPI_Transform mkTrf(input, tempTransform);
        transShape = mkTrf.Shape();
    }
    catch (...) {
        return transShape;
    }
    return transShape;
}

//!rotates a shape about a viewAxis
TopoDS_Shape ShapeUtils::rotateShape(const TopoDS_Shape& input, const gp_Ax2& viewAxis,
                                   double rotAngle)
{
    TopoDS_Shape transShape;
    if (input.IsNull()) {
        return transShape;
    }

    gp_Ax1 rotAxis = viewAxis.Axis();
    double rotation = rotAngle * M_PI / 180.0;

    try {
        gp_Trsf tempTransform;
        tempTransform.SetRotation(rotAxis, rotation);
        BRepBuilderAPI_Transform mkTrf(input, tempTransform);
        transShape = mkTrf.Shape();
    }
    catch (...) {
        return transShape;
    }
    return transShape;
}

//!scales a shape about origin
TopoDS_Shape ShapeUtils::scaleShape(const TopoDS_Shape& input, double scale)
{
    TopoDS_Shape transShape;
    try {
        gp_Trsf scaleTransform;
        scaleTransform.SetScale(gp_Pnt(0, 0, 0), scale);

        BRepBuilderAPI_Transform mkTrf(input, scaleTransform);
        transShape = mkTrf.Shape();
    }
    catch (...) {
        return transShape;
    }
    return transShape;
}

//!moves a shape
TopoDS_Shape ShapeUtils::moveShape(const TopoDS_Shape& input, const Base::Vector3d& motion)
{
    TopoDS_Shape transShape;
    try {
        gp_Trsf xlate;
        xlate.SetTranslation(gp_Vec(motion.x, motion.y, motion.z));

        BRepBuilderAPI_Transform mkTrf(input, xlate);
        transShape = mkTrf.Shape();
    }
    catch (...) {
        return transShape;
    }
    return transShape;
}

TopoDS_Shape ShapeUtils::centerShapeXY(const TopoDS_Shape& inShape, const gp_Ax2& coordSys)
{
    gp_Pnt inputCenter = findCentroidXY(inShape, coordSys);
    Base::Vector3d centroid = DrawUtil::toVector3d(inputCenter);
    return ShapeUtils::moveShape(inShape, centroid * -1.0);
}

std::pair<Base::Vector3d, Base::Vector3d> ShapeUtils::getEdgeEnds(TopoDS_Edge edge)
{
    std::pair<Base::Vector3d, Base::Vector3d> result;
    TopoDS_Vertex tvFirst, tvLast;
    TopExp::Vertices(edge, tvFirst, tvLast);
    gp_Pnt gpFirst = BRep_Tool::Pnt(tvFirst);
    gp_Pnt gpLast = BRep_Tool::Pnt(tvLast);

    result.first = DU::toVector3d(gpFirst);
    result.second = DU::toVector3d(gpLast);
    return result;
}

//! check for shape is null or shape has no subshapes(vertex/edge/face/etc)
//! this handles the case of an empty compound which is not IsNull, but has no
//! content.
bool  ShapeUtils::isShapeReallyNull(TopoDS_Shape shape)
{
    // if the shape is null or it has no subshapes, then it is really null
    return shape.IsNull() || !TopoDS_Iterator(shape).More();
}


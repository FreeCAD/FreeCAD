/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef _TECHDRAW_GEOMETRYOBJECT_H
#define _TECHDRAW_GEOMETRYOBJECT_H

#include <TopoDS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include <HLRBRep_Data.hxx>
#include <gp_Pnt.hxx>

#include <Base/Vector3D.h>
#include <string>
#include <vector>

#include "Geometry.h"

class HLRBRep_Algo;
class HLRBRep_EdgeData;
class TopoDS_Wire;
class HLRBRep_HLRToShape;

namespace TechDrawGeometry
{

class BaseGeom;

//! scales & mirrors a shape about a center
TopoDS_Shape TechDrawExport mirrorShape(const TopoDS_Shape &input,
                        const gp_Pnt& inputCenter,
                        double scale);

//! Returns the centroid of shape, as viewed according to direction and xAxis
gp_Pnt TechDrawExport findCentroid(const TopoDS_Shape &shape,
                        const Base::Vector3d &direction,
                        const Base::Vector3d &xAxis);

class TechDrawExport GeometryObject
{
public:
    /// Constructor
    GeometryObject();
    virtual ~GeometryObject();

    void clear();

    void setTolerance(double value);
    void setScale(double value);

    //! Returns 2D bounding box
    Base::BoundBox3d calcBoundingBox() const;

    const std::vector<Vertex *>   & getVertexGeometry() const { return vertexGeom; };
    const std::vector<BaseGeom *> & getEdgeGeometry() const { return edgeGeom; };
    const std::vector<Face *>     & getFaceGeometry() const { return faceGeom; };

    void projectShape(const TopoDS_Shape &input,
                                 const gp_Pnt& inputCenter,
                                 const Base::Vector3d &direction,
                                 const Base::Vector3d &xAxis);
    void extractGeometry(edgeClass category, bool visible);
    void addFaceGeom(Face * f);
    void clearFaceGeom();

protected:
    //HLR output
    TopoDS_Shape visHard;
    TopoDS_Shape visOutline;
    TopoDS_Shape visSmooth;
    TopoDS_Shape visSeam;
    TopoDS_Shape visIso;
    TopoDS_Shape hidHard;
    TopoDS_Shape hidOutline;
    TopoDS_Shape hidSmooth;
    TopoDS_Shape hidSeam;
    TopoDS_Shape hidIso;

    void addGeomFromCompound(TopoDS_Shape edgeCompound, edgeClass category, bool visible);

    /// Helper for calcBoundingBox()
    /*! Note that the name of this function isn't totally accurate due to
     *  TechDraw::Bsplines being composed of BezierSegments.
     */
    Base::BoundBox3d boundingBoxOfBspline(const BSpline *spline) const;

    /// Helper for calcBoundingBox()
    /*!
     * AOE = arc of ellipse.  Defaults allow this to be used for regular
     * ellipses as well as arcs.
     */
    Base::BoundBox3d boundingBoxOfAoe(const Ellipse *aoe, double start = 0,
                                      double end = 2 * M_PI, bool cw = false) const;

    /// Helper for boundingBoxOf(Aoc|Aoe)()
    /*!
     * Returns true iff angle theta is in [first, last], where the arc goes
     * clockwise (cw=true) or counterclockwise (cw=false) from first to last.
     */
    bool isWithinArc(double theta, double first, double last, bool cw) const;

    // Geometry
    std::vector<BaseGeom *> edgeGeom;
    std::vector<Vertex *> vertexGeom;
    std::vector<Face *> faceGeom;

    bool findVertex(Base::Vector2D v);

    double Tolerance;
    double Scale;
};

} //namespace TechDrawGeometry

#endif

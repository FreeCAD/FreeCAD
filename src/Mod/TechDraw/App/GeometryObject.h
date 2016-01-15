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
#include <gp_Pnt.hxx>

#include <Base/Vector3D.h>
#include <string>
#include <vector>

#include "Geometry.h"

class HLRBRep_Algo;
class Handle_HLRBRep_Data;
class HLRBRep_EdgeData;
class TopoDS_Wire;

namespace TechDrawGeometry
{

class BaseGeom;
/** Algo class for projecting shapes and creating SVG output of it
 */
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

    const std::vector<int> & getVertexRefs() const { return vertexReferences; };
    const std::vector<int> & getEdgeRefs()   const { return edgeReferences; };
    const std::vector<int> & getFaceRefs()   const { return faceReferences; };

    TechDrawGeometry::BaseGeom * projectEdge(const TopoDS_Shape &edge,
                                            const TopoDS_Shape &support,
                                            const Base::Vector3d &direction,
                                            const Base::Vector3d &projXAxis) const;
    TechDrawGeometry::Vertex   * projectVertex(const TopoDS_Shape &vert,
                                              const TopoDS_Shape &support,
                                              const Base::Vector3d &direction,
                                              const Base::Vector3d &projXAxis) const;

    void projectSurfaces(const TopoDS_Shape   &face,
                         const TopoDS_Shape   &support,
                         const Base::Vector3d &direction,
                         const Base::Vector3d &xaxis,
                         std::vector<TechDrawGeometry::Face *> &result) const;

    /// Process 3D shape to get 2D geometry
    /*!
     * Applies a projection to the input based on direction and vAxis, then
     * calls extractEdges (which in turn calls extractVerts) and extractFaces
     * to populate vectors used by getVertexRefs(), getEdgeRefs(), and
     * getFaceRefs()
     */
    void extractGeometry(const TopoDS_Shape &input,const Base::Vector3d &direction, bool extractHidden = false, const Base::Vector3d &vAxis = Base::Vector3d(0.,0.,0.));

protected:
    bool shouldDraw(const bool inFace, const int typ,HLRBRep_EdgeData& ed);
    bool isSameCurve(const BRepAdaptor_Curve &c1, const BRepAdaptor_Curve &c2) const;

    /// Reimplements HLRBRep Drawing Algorithms to satisfy Drawing Workbench requirements
    void drawFace(const bool visible, const int iface, Handle_HLRBRep_Data & DS, TopoDS_Shape& Result) const;

    /// Add (visible) intervals of ed to Result as Edges
    void drawEdge(HLRBRep_EdgeData& ed, TopoDS_Shape& Result, const bool visible) const;

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

    void extractVerts(HLRBRep_Algo *myAlgo, const TopoDS_Shape &S, HLRBRep_EdgeData& ed, int ie, ExtractionType extractionType);
    void extractEdges(HLRBRep_Algo *myAlgo, const TopoDS_Shape &S, int type, bool visible, ExtractionType extractionType);

    void extractFaces(HLRBRep_Algo *myAlgo,
                      const TopoDS_Shape &S,
                      bool visible,
                      ExtractionType extractionType,
                      std::vector<TechDrawGeometry::Face *> &projFaces,
                      std::vector<int> &faceRefs) const;

    int calculateGeometry(const TopoDS_Shape &input, ExtractionType extractionType, std::vector<BaseGeom *> &geoms) const;

    /// Accumulate edges from input and store them in wires
    void createWire(const TopoDS_Shape &input, std::vector<TopoDS_Wire> &wiresOut) const;

    // Geometry
    std::vector<BaseGeom *> edgeGeom;
    std::vector<Vertex *> vertexGeom;
    std::vector<Face *> faceGeom;

    bool findVertex(Base::Vector2D v);

    // indexes to geometry in Source object
    std::vector<int> vertexReferences;
    std::vector<int> edgeReferences;
    std::vector<int> faceReferences;

    HLRBRep_Algo *brep_hlr;
    double Tolerance;
    double Scale;

    /// Returns the centroid of shape, as viewed according to direction and xAxis
    gp_Pnt findCentroid(const TopoDS_Shape &shape,
                        const Base::Vector3d &direction,
                        const Base::Vector3d &xAxis) const;
};

} //namespace TechDrawGeometry

#endif

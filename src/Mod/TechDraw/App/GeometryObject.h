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

#ifndef TECHDRAW_GEOMETRYOBJECT_H
#define TECHDRAW_GEOMETRYOBJECT_H

//! a class to the projection of shapes, removal/identifying hidden lines and
//  converting the output for OCC HLR into the BaseGeom intermediate representation.


#include <Mod/TechDraw/TechDrawGlobal.h>

#include <memory>
#include <string>
#include <vector>

#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>

#include <Base/BoundBox.h>
#include <Base/Vector3D.h>

#include "Geometry.h"
#include "ShapeUtils.h"


namespace TechDraw
{
class DrawViewPart;
class DrawViewDetail;
class DrawView;
class CosmeticVertex;
class CosmeticEdge;
}// namespace TechDraw

namespace TechDraw
{
class BaseGeom;
class Vector;
class Face;
class Vertex;

class TechDrawExport GeometryObject
{
public:
    /// Constructor
    GeometryObject(const std::string& parent, TechDraw::DrawView* parentObj);
    virtual ~GeometryObject();

    void clear();

    //! Returns 2D bounding box
    Base::BoundBox3d calcBoundingBox() const;

    const std::vector<VertexPtr>& getVertexGeometry() const { return vertexGeom; }
    const BaseGeomPtrVector& getEdgeGeometry() const { return edgeGeom; }
    const BaseGeomPtrVector getVisibleFaceEdges(bool smooth, bool seam) const;
    const std::vector<FacePtr>& getFaceGeometry() const { return faceGeom; }

    void setVertexGeometry(std::vector<VertexPtr> newVerts) { vertexGeom = newVerts; }
    void setEdgeGeometry(BaseGeomPtrVector newGeoms) { edgeGeom = newGeoms; }

    void projectShape(const TopoDS_Shape& input, const gp_Ax2& viewAxis);
    void projectShapeWithPolygonAlgo(const TopoDS_Shape& input, const gp_Ax2& viewAxis);
    static TopoDS_Shape projectSimpleShape(const TopoDS_Shape& shape, const gp_Ax2& CS);
    static TopoDS_Shape simpleProjection(const TopoDS_Shape& shape, const gp_Ax2& projCS);
    static TopoDS_Shape projectFace(const TopoDS_Shape& face, const gp_Ax2& CS);
    void makeTDGeometry();
    void extractGeometry(edgeClass category, bool visible);
    void addFaceGeom(FacePtr f);
    void clearFaceGeom();
    void setIsoCount(int i) { m_isoCount = i; }
    void setParentName(std::string n);//for debug messages
    void isPerspective(bool b) { m_isPersp = b; }
    bool isPerspective() { return m_isPersp; }
    void usePolygonHLR(bool b) { m_usePolygonHLR = b; }
    bool usePolygonHLR() const { return m_usePolygonHLR; }
    void setFocus(double f) { m_focus = f; }
    double getFocus() { return m_focus; }
    void setScrubCount(int count) { m_scrubCount = count; }


    void pruneVertexGeom(Base::Vector3d center, double radius);

    TopoDS_Shape getVisHard() { return visHard; }
    TopoDS_Shape getVisOutline() { return visOutline; }
    TopoDS_Shape getVisSmooth() { return visSmooth; }
    TopoDS_Shape getVisSeam() { return visSeam; }
    TopoDS_Shape getVisIso() { return visIso; }
    TopoDS_Shape getHidHard() { return hidHard; }
    TopoDS_Shape getHidOutline() { return hidOutline; }
    TopoDS_Shape getHidSmooth() { return hidSmooth; }
    TopoDS_Shape getHidSeam() { return hidSeam; }
    TopoDS_Shape getHidIso() { return hidIso; }

    void addVertex(TechDraw::VertexPtr v);
    void addEdge(TechDraw::BaseGeomPtr bg);


    int addCosmeticVertex(CosmeticVertex* cv);
    int addCosmeticVertex(Base::Vector3d pos);
    int addCosmeticVertex(Base::Vector3d pos, std::string tagString);

    int addCosmeticEdge(CosmeticEdge* ce);
    int addCosmeticEdge(Base::Vector3d start, Base::Vector3d end);
    int addCosmeticEdge(Base::Vector3d start, Base::Vector3d end, std::string tagString);
    int addCosmeticEdge(TechDraw::BaseGeomPtr base, std::string tagString);

    int addCenterLine(TechDraw::BaseGeomPtr bg, std::string tag);

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
    TechDraw::DrawViewDetail* isParentDetail();

    //similar function in Geometry?
    /*!
     * Returns true iff angle theta is in [first, last], where the arc goes
     * clockwise (cw=true) or counterclockwise (cw=false) from first to last.
     */
    bool isWithinArc(double theta, double first, double last, bool cw) const;

    // Geometry
    BaseGeomPtrVector edgeGeom;
    std::vector<VertexPtr> vertexGeom;
    std::vector<FacePtr> faceGeom;

    bool findVertex(Base::Vector3d v);

    std::string m_parentName;
    TechDraw::DrawView* m_parent;
    int m_isoCount;
    bool m_isPersp;
    double m_focus;
    bool m_usePolygonHLR;
    int m_scrubCount;
};

using GeometryObjectPtr = std::shared_ptr<GeometryObject>;

}//namespace TechDraw

#endif


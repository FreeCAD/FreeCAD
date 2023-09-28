/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
 *   Copyright (c) 2022 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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

#include "PreCompiled.h"
#ifndef _PreComp_
    #include <boost/uuid/uuid_io.hpp>
    #include <boost/uuid/uuid_generators.hpp>
    #include <BRepBuilderAPI_MakeEdge.hxx>
    #include <BRepBndLib.hxx>
    #include <Bnd_Box.hxx>
    #include <TopoDS_Shape.hxx>
#endif

#include <BRepTools.hxx>

#include <Base/Console.h>

#include "CenterLine.h"
#include "DrawUtil.h"
#include "DrawViewPart.h"
#include "Geometry.h"
#include "CenterLinePy.h"
#include "ShapeUtils.h"

using namespace TechDraw;
using DU = DrawUtil;

TYPESYSTEM_SOURCE(TechDraw::CenterLine, Base::Persistence)

CenterLine::CenterLine()
{
    m_start = Base::Vector3d(0.0, 0.0, 0.0);
    m_end = Base::Vector3d(0.0, 0.0, 0.0);
    m_mode = CLMODE::VERTICAL;
    m_hShift = 0.0;
    m_vShift = 0.0;
    m_rotate = 0.0;
    m_extendBy = 0.0;
    m_type = CLTYPE::FACE;
    m_flip2Line = false;

    m_geometry = std::make_shared<TechDraw::BaseGeom> ();

    initialize();
}

CenterLine::CenterLine(const TechDraw::CenterLine* cl)
{
    m_start = cl->m_start;
    m_end = cl->m_end;
    m_mode = cl->m_mode;
    m_hShift = cl->m_hShift;
    m_vShift = cl->m_vShift;
    m_rotate = cl->m_rotate;
    m_extendBy = cl->m_extendBy;
    m_type = cl->m_type;
    m_flip2Line = cl->m_flip2Line;

    m_geometry = cl->m_geometry;    //new BaseGeom(cl->m_geometry);??

    initialize();
}

CenterLine::CenterLine(const TechDraw::BaseGeomPtr& bg,
                       const int m,
                       const double h,
                       const double v,
                       const double r,
                       const double x)
{
    m_start = bg->getStartPoint();
    m_end = bg->getEndPoint();
    m_mode = m;
    m_hShift = h;
    m_vShift = v;
    m_rotate = r;
    m_extendBy = x;
    m_type = CLTYPE::FACE;
    m_flip2Line = false;

    m_geometry = bg;

    initialize();
}

CenterLine::CenterLine(const Base::Vector3d& pt1,
                       const Base::Vector3d& pt2,
                       const int m,
                       const double h,
                       const double v,
                       const double r,
                       const double x)
{
    m_start = pt1;
    m_end = pt2;
    m_mode = m;
    m_hShift = h;
    m_vShift = v;
    m_rotate = r;
    m_extendBy = x;
    m_type = CLTYPE::FACE;
    m_flip2Line = false;

    m_geometry = BaseGeomPtrFromVectors(pt1, pt2);

    initialize();

}

CenterLine::~CenterLine()
{
}

void CenterLine::initialize()
{
    m_geometry->setClassOfEdge(ecHARD);
    m_geometry->setHlrVisible( true);
    m_geometry->setCosmetic(true);
    m_geometry->source(CENTERLINE);

    createNewTag();
    m_geometry->setCosmeticTag(getTagAsString());
}

TechDraw::BaseGeomPtr CenterLine::BaseGeomPtrFromVectors(Base::Vector3d pt1, Base::Vector3d pt2)
{
    // Base::Console().Message("CE::CE(p1, p2)\n");
    Base::Vector3d p1 = DrawUtil::invertY(pt1);
    Base::Vector3d p2 = DrawUtil::invertY(pt2);
    gp_Pnt gp1(p1.x, p1.y, p1.z);
    gp_Pnt gp2(p2.x, p2.y, p2.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
    TechDraw::BaseGeomPtr bg = TechDraw::BaseGeom::baseFactory(e);
    return bg;
}

CenterLine* CenterLine::CenterLineBuilder(const DrawViewPart* partFeat,
                                          const std::vector<std::string>& subNames,
                                          const int mode,
                                          const bool flip)
{
//    Base::Console().Message("CL::CLBuilder()\n - subNames: %d\n", subNames.size());
    std::pair<Base::Vector3d, Base::Vector3d> ends;
    std::vector<std::string> faces;
    std::vector<std::string> edges;
    std::vector<std::string> verts;

    std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(subNames.front());
    int type = CLTYPE::FACE;
    if (geomType == "Face") {
        type = CLTYPE::FACE;
        ends = TechDraw::CenterLine::calcEndPoints(partFeat,
                             subNames,
                             mode,
                             0.0,
                             0.0, 0.0, 0.0);
        faces = subNames;
    } else if (geomType == "Edge") {
        type = CLTYPE::EDGE;
        ends = TechDraw::CenterLine::calcEndPoints2Lines(partFeat,
                         subNames,
                         mode,
                         0.0,
                         0.0, 0.0, 0.0, flip);
        edges = subNames;
    } else if (geomType == "Vertex") {
        type = CLTYPE::VERTEX;
        ends = TechDraw::CenterLine::calcEndPoints2Points(partFeat,
                         subNames,
                         mode,
                         0.0,
                         0.0, 0.0, 0.0, flip);
        verts = subNames;
    }
    if ((ends.first).IsEqual(ends.second, Precision::Confusion())) {
        Base::Console().Warning("CenterLineBuilder - endpoints are equal: %s\n",
                              DrawUtil::formatVector(ends.first).c_str());
        Base::Console().Warning("CenterLineBuilder - check V/H/A and/or Flip parameters\n");
        return nullptr;
    }


    TechDraw::CenterLine* cl = new TechDraw::CenterLine(ends.first, ends.second);
    if (cl) {
        cl->m_type = type;
        cl->m_mode = mode;
        cl->m_faces = faces;
        cl->m_edges = edges;
        cl->m_verts = verts;
        cl->m_flip2Line = flip;
    }
    return cl;
}

TechDraw::BaseGeomPtr CenterLine::scaledGeometry(const TechDraw::DrawViewPart* partFeat)
{
//    Base::Console().Message("CL::scaledGeometry() - m_type: %d\n", m_type);
    double scale = partFeat->getScale();
    double viewAngleDeg = partFeat->Rotation.getValue();
    std::pair<Base::Vector3d, Base::Vector3d> ends;
    try {
        if (m_faces.empty() &&
            m_edges.empty() &&
            m_verts.empty() ) {
//            Base::Console().Message("CL::scaledGeometry - no geometry to scale!\n");
            //CenterLine was created by points without a geometry reference,
            ends = calcEndPointsNoRef(m_start, m_end, scale, m_extendBy,
                                      m_hShift, m_vShift, m_rotate, viewAngleDeg);
        } else if (m_type == CLTYPE::FACE) {
            ends = calcEndPoints(partFeat,
                                 m_faces,
                                 m_mode, m_extendBy,
                                 m_hShift, m_vShift, m_rotate);
        } else if (m_type == CLTYPE::EDGE) {
            ends = calcEndPoints2Lines(partFeat,
                                       m_edges,
                                       m_mode,
                                       m_extendBy,
                                       m_hShift, m_vShift, m_rotate, m_flip2Line);
        } else if (m_type == CLTYPE::VERTEX) {
            ends = calcEndPoints2Points(partFeat,
                                        m_verts,
                                        m_mode,
                                        m_extendBy,
                                        m_hShift, m_vShift, m_rotate, m_flip2Line);
        }
    }

    catch (...) {
        Base::Console().Error("CL::scaledGeometry - failed to calculate endpoints!\n");
        return nullptr;
    }

    Base::Vector3d p1 = ends.first;
    Base::Vector3d p2 = ends.second;
    if (p1.IsEqual(p2, 0.00001)) {
        Base::Console().Warning("Centerline endpoints are equal. Could not draw.\n");
        //what to do here?  //return current geom?
        return m_geometry;
    }

    gp_Pnt gp1(p1.x, p1.y, p1.z);
    gp_Pnt gp2(p2.x, p2.y, p2.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
    TopoDS_Shape s = ShapeUtils::scaleShape(e, scale);
    TopoDS_Edge newEdge = TopoDS::Edge(s);
    TechDraw::BaseGeomPtr newGeom = TechDraw::BaseGeom::baseFactory(newEdge);
    newGeom->setClassOfEdge(ecHARD);
    newGeom->setHlrVisible( true);
    newGeom->setCosmetic(true);
    newGeom->source(CENTERLINE);
    newGeom->setCosmeticTag(getTagAsString());

    return newGeom;
}

TechDraw::BaseGeomPtr CenterLine::scaledAndRotatedGeometry(TechDraw::DrawViewPart* partFeat)
{
//    Base::Console().Message("CL::scaledGeometry() - m_type: %d\n", m_type);
    double scale = partFeat->getScale();
    double viewAngleDeg = partFeat->Rotation.getValue();
    std::pair<Base::Vector3d, Base::Vector3d> ends;
    try {
        if (m_faces.empty() &&
            m_edges.empty() &&
            m_verts.empty() ) {
//            Base::Console().Message("CL::scaledGeometry - no geometry to scale!\n");
            //CenterLine was created by points without a geometry reference,
            ends = calcEndPointsNoRef(m_start, m_end, scale, m_extendBy,
                                      m_hShift, m_vShift, m_rotate, viewAngleDeg);
        } else if (m_type == CLTYPE::FACE) {
            ends = calcEndPoints(partFeat,
                                 m_faces,
                                 m_mode, m_extendBy,
                                 m_hShift, m_vShift, m_rotate);
        } else if (m_type == CLTYPE::EDGE) {
            ends = calcEndPoints2Lines(partFeat,
                                       m_edges,
                                       m_mode,
                                       m_extendBy,
                                       m_hShift, m_vShift, m_rotate, m_flip2Line);
        } else if (m_type == CLTYPE::VERTEX) {
            ends = calcEndPoints2Points(partFeat,
                                        m_verts,
                                        m_mode,
                                        m_extendBy,
                                        m_hShift, m_vShift, m_rotate, m_flip2Line);
        }
    }

    catch (...) {
        Base::Console().Error("CL::scaledGeometry - failed to calculate endpoints!\n");
        return nullptr;
    }

    // inversion here breaks face cl.
    Base::Vector3d p1 = ends.first;
    Base::Vector3d p2 = ends.second;
    if (p1.IsEqual(p2, 0.00001)) {
        Base::Console().Warning("Centerline endpoints are equal. Could not draw.\n");
        //what to do here?  //return current geom?
        return m_geometry;
    }

    TopoDS_Edge newEdge;
    if (getType() == CLTYPE::FACE ) {
        gp_Pnt gp1(DU::togp_Pnt(p1));
        gp_Pnt gp2(DU::togp_Pnt(p2));
        TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
        // Mirror shape in Y and scale
        TopoDS_Shape s = ShapeUtils::mirrorShape(e, gp_Pnt(0.0, 0.0, 0.0), scale);
        // rotate using OXYZ as the coordinate system
        s = ShapeUtils::rotateShape(s, gp_Ax2(), - partFeat->Rotation.getValue());
        newEdge = TopoDS::Edge(s);
    } else if (getType() == CLTYPE::EDGE  ||
               getType() == CLTYPE::VERTEX) {
        gp_Pnt gp1(DU::togp_Pnt(DU::invertY(p1 * scale)));
        gp_Pnt gp2(DU::togp_Pnt(DU::invertY(p2 * scale)));
        newEdge = BRepBuilderAPI_MakeEdge(gp1, gp2);
    }

    TechDraw::BaseGeomPtr newGeom = TechDraw::BaseGeom::baseFactory(newEdge);
    if (!newGeom) {
        throw Base::RuntimeError("Failed to create center line");
    }
    newGeom->setClassOfEdge(ecHARD);
    newGeom->setHlrVisible( true);
    newGeom->setCosmetic(true);
    newGeom->source(CENTERLINE);
    newGeom->setCosmeticTag(getTagAsString());

    return newGeom;
}

std::string CenterLine::toString() const
{
    std::stringstream ss;
    ss << m_start.x << ", " <<
          m_start.y << ", " <<
          m_start.z << ", " <<
          m_end.x << ", " <<
          m_end.y << ", " <<
          m_end.z << ", " <<
          m_mode << ", " <<
          m_type << ", " <<
          m_hShift << ", " <<
          m_vShift << ", " <<
          m_rotate << ", " <<
          m_flip2Line << ", " <<
          m_extendBy << ", " <<
          m_faces.size();
    if (!m_faces.empty()) {
        for (auto& f: m_faces) {
            if (!f.empty()) {
                ss << ", " << f ;
            }
        }
    }

    std::string clCSV = ss.str();
    std::string fmtCSV = m_format.toString();
    return clCSV + ", $$$, " + fmtCSV;
}

void CenterLine::dump(const char* title)
{
    Base::Console().Message("CL::dump - %s \n", title);
    Base::Console().Message("CL::dump - %s \n", toString().c_str());
}

//! rotate a notional 2d vector from p1 to p2 around its midpoint by angleDeg
std::pair<Base::Vector3d, Base::Vector3d> CenterLine::rotatePointsAroundMid(const Base::Vector3d& p1,
                                  const Base::Vector3d& p2,
                                  const Base::Vector3d& mid,
                                  const double angleDeg)
{
    std::pair<Base::Vector3d, Base::Vector3d> result;
    double angleRad = angleDeg * M_PI / 180.0;

    result.first.x = ((p1.x - mid.x) * cos(angleRad)) - ((p1.y - mid.y) * sin(angleRad)) + mid.x;
    result.first.y = ((p1.x - mid.x) * sin(angleRad)) + ((p1.y - mid.y) * cos(angleRad)) + mid.y;
    result.first.z = 0.0;

    result.second.x = ((p2.x - mid.x) * cos(angleRad)) - ((p2.y - mid.y) * sin(angleRad)) + mid.x;
    result.second.y = ((p2.x - mid.x) * sin(angleRad)) + ((p2.y - mid.y) * cos(angleRad)) + mid.y;
    result.second.z = 0.0;

    return result;
}


//end points for centerline with no geometry reference
std::pair<Base::Vector3d, Base::Vector3d> CenterLine::calcEndPointsNoRef(const Base::Vector3d& start,
                                                      const Base::Vector3d& end,
                                                      const double scale,
                                                      const double ext,
                                                      const double hShift,
                                                      const double vShift,
                                                      const double rotate, const double viewAngleDeg)
{
//    Base::Console().Message("CL::calcEndPointsNoRef()\n");
    Base::Vector3d p1 = start;
    Base::Vector3d p2 = end;
    Base::Vector3d mid = (p1 + p2) / 2.0;

    //extend
    Base::Vector3d clDir = p2 - p1;
    clDir.Normalize();
    p1 = p1 - (clDir * ext);
    p2 = p2 + (clDir * ext);

    //rotate
    if (!DrawUtil::fpCompare(rotate, 0.0)) {
        //rotate p1, p2 about mid point
        std::tie(p1, p2) = rotatePointsAroundMid(p1, p2, mid, rotate);
    }

    //shift
    if (!DrawUtil::fpCompare(hShift, 0.0)) {
        double hss = hShift * scale;
        p1.x = p1.x + hss;
        p2.x = p2.x + hss;
    }
    if (!DrawUtil::fpCompare(vShift, 0.0)) {
        double vss = vShift * scale;
        p1.y = p1.y + vss;
        p2.y = p2.y + vss;
    }

    // rotate the endpoints so that when the View's Rotation is applied, the
    // centerline is aligned correctly
    std::pair<Base::Vector3d, Base::Vector3d> result;
    result.first = p1 / scale;
    result.second = p2 / scale;
    Base::Vector3d midpoint = (result.first + result.second) / 2.0;
    result = rotatePointsAroundMid(result.first, result.second, midpoint, viewAngleDeg * -1.0);

    return result;
}

//end points for face centerline
std::pair<Base::Vector3d, Base::Vector3d> CenterLine::calcEndPoints(const DrawViewPart* partFeat,
                                                      const std::vector<std::string>& faceNames,
                                                      const int mode,
                                                      const double ext,
                                                      const double hShift,
                                                      const double vShift,
                                                      const double rotate)
{
//    Base::Console().Message("CL::calcEndPoints()\n");
    if (faceNames.empty()) {
        Base::Console().Warning("CL::calcEndPoints - no faces!\n");
        return std::pair<Base::Vector3d, Base::Vector3d>();
    }

    Bnd_Box faceBox;
    faceBox.SetGap(0.0);

    double scale = partFeat->getScale();

    std::vector<TopoDS_Edge> faceEdgesAll;
    for (auto& fn: faceNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(fn) != "Face") {
            continue;
        }
        int idx = TechDraw::DrawUtil::getIndexFromName(fn);
        std::vector<TechDraw::BaseGeomPtr> faceGeoms =
                                                partFeat->getFaceEdgesByIndex(idx);
        if (!faceGeoms.empty()) {
            for (auto& fe: faceGeoms) {
                if (!fe->getCosmetic()) {
                    faceEdgesAll.push_back(fe->getOCCEdge());
                }
            }
        }
    }
    TopoDS_Shape faceEdgeCompound = DU::vectorToCompound(faceEdgesAll);

    if (partFeat->Rotation.getValue() != 0.0) {
        // unrotate the input shape to align with the cardinal axes so we can use bbox to
        // get size measurements
        faceEdgeCompound = ShapeUtils::rotateShape(faceEdgeCompound,
                                                   partFeat->getProjectionCS(),
                                                   partFeat->Rotation.getValue() * -1.0);
    }
    // get the center of the unrotated, scaled face
    Base::Vector3d faceCenter = ShapeUtils::findCentroidVec(faceEdgeCompound, partFeat->getProjectionCS());
    // we need to move the edges to the origin here to get the right limits from the bounding box
    faceEdgeCompound = ShapeUtils::moveShape(faceEdgeCompound, faceCenter * -1.0);

    // get the bounding box of the centered and unrotated face
    BRepBndLib::AddOptimal(faceEdgeCompound, faceBox);

    if (faceBox.IsVoid()) {
        Base::Console().Error("CL::calcEndPoints - faceBox is void!\n");
        throw Base::IndexError("CenterLine wrong number of faces.");
    }

    double Xmin, Ymin, Zmin, Xmax, Ymax, Zmax;
    faceBox.Get(Xmin, Ymin, Zmin, Xmax, Ymax, Zmax);

    double Xspan = fabs(Xmax - Xmin);
    Xspan = (Xspan / 2.0);
    double Xmid = 0.0;
    Xmax = Xmid + Xspan;
    Xmin = Xmid - Xspan;
    double Yspan = fabs(Ymax - Ymin);
    Yspan = (Yspan / 2.0);
    double Ymid = 0.0;
    Ymax = Ymid + Yspan;
    Ymin = Ymid - Yspan;

    Base::Vector3d p1, p2;
    if (mode == CenterLine::VERTICAL) {                    //vertical
        p1 = Base::Vector3d(Xmid, Ymax, 0.0);
        p2 = Base::Vector3d(Xmid, Ymin, 0.0);
    } else if (mode == CenterLine::HORIZONTAL) {            //horizontal
        p1 = Base::Vector3d(Xmin, Ymid, 0.0);
        p2 = Base::Vector3d(Xmax, Ymid, 0.0);
    } else {      //vert == CenterLine::ALIGNED //aligned, but aligned doesn't make sense for face(s) bbox
        Base::Console().Message("CL::calcEndPoints - aligned is not applicable to Face center lines\n");
        p1 = Base::Vector3d(Xmid, Ymax, 0.0);
        p2 = Base::Vector3d(Xmid, Ymin, 0.0);
    }

    // now move the extents back to the face center.  this should give us the scaled,
    // unrotated ends of the cl (but in 3d coordinates, which will be handled at the time
    // the cl is added to the view)
    p1 += faceCenter;
    p2 += faceCenter;

    Base::Vector3d mid = (p1 + p2) / 2.0;

    //extend
    Base::Vector3d clDir = p2 - p1;
    clDir.Normalize();
    p1 = p1 - (clDir * ext);
    p2 = p2 + (clDir * ext);

    //rotate
    if (!DrawUtil::fpCompare(rotate, 0.0)) {
        //rotate p1, p2 about mid point
        std::tie(p1, p2) = rotatePointsAroundMid(p1, p2, mid, rotate);
    }

    //shift
    if (!DrawUtil::fpCompare(hShift, 0.0)) {
        double hss = hShift * scale;
        p1.x = p1.x + hss;
        p2.x = p2.x + hss;
    }
    if (!DrawUtil::fpCompare(vShift, 0.0)) {
        double vss = vShift * scale;
        p1.y = p1.y + vss;
        p2.y = p2.y + vss;
    }

    std::pair<Base::Vector3d, Base::Vector3d> result;
    result.first = p1 / scale;
    result.second = p2 / scale;
    return result;
}

std::pair<Base::Vector3d, Base::Vector3d> CenterLine::calcEndPoints2Lines(const DrawViewPart* partFeat,
                                                      const std::vector<std::string>& edgeNames,
                                                      const int mode,
                                                      const double ext,
                                                      const double hShift,
                                                      const double vShift,
                                                      const double rotate,
                                                      const bool flip)

{
    Q_UNUSED(flip)

//    Base::Console().Message("CL::calc2Lines() - mode: %d flip: %d edgeNames: %d\n", mode, flip, edgeNames.size());
    std::pair<Base::Vector3d, Base::Vector3d> result;
    if (edgeNames.empty()) {
        Base::Console().Warning("CL::calcEndPoints2Lines - no edges!\n");
        return result;
    }

    double scale = partFeat->getScale();

    std::vector<TechDraw::BaseGeomPtr> edges;
    for (auto& en: edgeNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(en) != "Edge") {
            continue;
        }
        int idx = TechDraw::DrawUtil::getIndexFromName(en);
        TechDraw::BaseGeomPtr bg = partFeat->getGeomByIndex(idx);
        if (bg) {
            edges.push_back(bg);
        } else {
            Base::Console().Message("CL::calcEndPoints2Lines - no geom for index: %d\n", idx);
        }
    }
    if (edges.size() != 2) {
        Base::Console().Message("CL::calcEndPoints2Lines - wrong number of edges: %d!\n", edges.size());
        throw Base::IndexError("CenterLine wrong number of edges.");
    }

    // these points are centered, rotated, scaled and inverted.
    // invert the points so the math works correctly
    Base::Vector3d l1p1 = DU::invertY(edges.front()->getStartPoint());
    Base::Vector3d l1p2 = DU::invertY(edges.front()->getEndPoint());
    Base::Vector3d l2p1 = DU::invertY(edges.back()->getStartPoint());
    Base::Vector3d l2p2 = DU::invertY(edges.back()->getEndPoint());

    // The centerline is drawn using the midpoints of the two lines that connect l1p1-l2p1 and l1p2-l2p2.
    // However, we don't know which point should be l1p1 to get a geometrically correct result, see
    // https://wiki.freecad.org/File:TD-CenterLineFlip.png for an illustration of the problem.
    // Thus we test this by a circulation test, see this post for a brief explanation:
    // https://forum.freecad.org/viewtopic.php?p=505733#p505615
    if (DrawUtil::circulation(l1p1, l1p2, l2p1) != DrawUtil::circulation(l1p2, l2p2, l2p1)) {
        Base::Vector3d temp; // reverse line 1
        temp = l1p1;
        l1p1 = l1p2;
        l1p2 = temp;
    }

    Base::Vector3d p1 = (l1p1 + l2p1) / 2.0;
    Base::Vector3d p2   = (l1p2 + l2p2) / 2.0;
    Base::Vector3d mid = (p1 + p2) / 2.0;

    // if the proposed end points prevent the creation of a vertical or horizontal centerline, we need
    // to prevent the "orientation" code below from creating identical endpoints.  This would create a
    // zero length edge and cause problems later.
    bool inhibitVertical = false;
    bool inhibitHorizontal = false;
    if (DU::fpCompare(p1.y, p2.y, EWTOLERANCE)) {
        // proposed end points are aligned vertically, so we can't draw a vertical line to connect them
        inhibitVertical = true;
    }
    if (DU::fpCompare(p1.x, p2.x, EWTOLERANCE)) {
        // proposed end points are aligned horizontally, so we can't draw a horizontal line to connect them
        inhibitHorizontal = true;
    }

    //orientation
    if (partFeat->Rotation.getValue() == 0.0) {
        // if the view is rotated, then horizontal and vertical lose their meaning
        if (mode == 0 && !inhibitVertical) {           //Vertical
            p1.x = mid.x;
            p2.x = mid.x;
        } else if (mode == 1 && !inhibitHorizontal) {    //Horizontal
            p1.y = mid.y;
            p2.y = mid.y;
        } else if (mode == 2) {    //Aligned
            // no op
        }
    }

    //extend
    Base::Vector3d clDir = p2 - p1;
    clDir.Normalize();
    p1 = p1 - (clDir * ext);
    p2 = p2 + (clDir * ext);

    //rotate
    if (!DrawUtil::fpCompare(rotate, 0.0)) {
        //rotate p1, p2 about mid
        std::tie(p1, p2) = rotatePointsAroundMid(p1, p2, mid, rotate);
    }

    //shift
    if (!DrawUtil::fpCompare(hShift, 0.0)) {
        double hss = hShift * scale;
        p1.x = p1.x + hss;
        p2.x = p2.x + hss;
    }
    if (!DrawUtil::fpCompare(vShift, 0.0)) {
        double vss = vShift * scale;
        p1.y = p1.y + vss;
        p2.y = p2.y + vss;
    }

    // the cl will be scaled when drawn, so unscale now.
    result.first = p1 / scale;
    result.second = p2 / scale;
    return result;
}

std::pair<Base::Vector3d, Base::Vector3d> CenterLine::calcEndPoints2Points(const DrawViewPart* partFeat,
                                                      const std::vector<std::string>& vertNames,
                                                      const int mode,
                                                      const double ext,
                                                      const double hShift,
                                                      const double vShift,
                                                      const double rotate,
                                                      const bool flip)

{
//    Base::Console().Message("CL::calc2Points() - mode: %d\n", mode);
    if (vertNames.empty()) {
        Base::Console().Warning("CL::calcEndPoints2Points - no points!\n");
        return std::pair<Base::Vector3d, Base::Vector3d>();
    }

    double scale = partFeat->getScale();

    std::vector<TechDraw::VertexPtr> points;
    for (auto& vn: vertNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(vn) != "Vertex") {
            continue;
        }
        int idx = TechDraw::DrawUtil::getIndexFromName(vn);
        TechDraw::VertexPtr v = partFeat->getProjVertexByIndex(idx);
        if (v) {
            points.push_back(v);
        }
    }
    if (points.size() != 2) {
        throw Base::IndexError("CenterLine wrong number of points.");
    }

    Base::Vector3d v1 = DU::invertY(points.front()->point());
    Base::Vector3d v2 = DU::invertY(points.back()->point());
    Base::Vector3d mid = (v1 + v2) / 2.0;
    Base::Vector3d dir = v2 - v1;

    // if the proposed end points prevent the creation of a vertical or horizontal centerline, we need
    // to prevent the "orientation" code below from creating identical endpoints.  This would create a
    // zero length edge and cause problems later.

    bool inhibitVertical = false;
    bool inhibitHorizontal = false;
    if (DU::fpCompare(v1.y, v2.y, EWTOLERANCE)) {
        // proposed end points are aligned horizontally, can not draw horizontal CL
        inhibitHorizontal = true;
    }
    if (DU::fpCompare(v1.x, v2.x, EWTOLERANCE)) {
        // proposed end points are aligned vertically, can not draw vertical CL
        inhibitVertical = true;
    }

    //orientation
    if (partFeat->Rotation.getValue() == 0.0) {
        // if the view is rotated, then horizontal and vertical lose their meaning
        if (mode == CenterLine::VERTICAL  && !inhibitVertical) {
            //Vertical
            v1.x = mid.x;
            v2.x = mid.x;
        } else if (mode == CenterLine::HORIZONTAL && !inhibitHorizontal) {
            //Horizontal
            v1.y = mid.y;
            v2.y = mid.y;
        } else if (mode == CenterLine::ALIGNED) {    //Aligned
            // no op
        }
    }

    double length = dir.Length();
    dir.Normalize();
    Base::Vector3d clDir(dir.y, -dir.x, dir.z);

    Base::Vector3d p1 = mid + clDir * (length / 2.0);
    Base::Vector3d p2 = mid - clDir * (length / 2.0);

    if (flip) {                   //is flip relevant to 2 point???
        Base::Vector3d temp = p1;
        p1 = p2;
        p2 = temp;
    }

    //extend
    p1 = p1 + (clDir * ext);
    p2 = p2 - (clDir * ext);

    //rotate
    if (!DrawUtil::fpCompare(rotate, 0.0)) {
        //rotate p1, p2 about mid
        std::tie(p1, p2) = rotatePointsAroundMid(p1, p2, mid, rotate);
    }

    //shift
    if (!DrawUtil::fpCompare(hShift, 0.0)) {
        double hss = hShift * scale;
        p1.x = p1.x + hss;
        p2.x = p2.x + hss;
    }
    if (!DrawUtil::fpCompare(vShift, 0.0)) {
        double vss = vShift * scale;
        p1.y = p1.y + vss;
        p2.y = p2.y + vss;
    }

    std::pair<Base::Vector3d, Base::Vector3d> result;
    result.first = p1 / scale;
    result.second = p2 / scale;

    return result;
}

// Persistence implementers
unsigned int CenterLine::getMemSize () const
{
    return 1;
}

void CenterLine::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Start "
                << "X=\"" <<  m_start.x <<
                "\" Y=\"" <<  m_start.y <<
                "\" Z=\"" <<  m_start.z <<
                 "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<End "
                << "X=\"" <<  m_end.x <<
                "\" Y=\"" <<  m_end.y <<
                "\" Z=\"" <<  m_end.z <<
                 "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Mode value=\"" << m_mode <<"\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<HShift value=\"" << m_hShift <<"\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<VShift value=\"" << m_vShift <<"\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Rotate value=\"" << m_rotate <<"\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Extend value=\"" << m_extendBy <<"\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Type value=\"" << m_type <<"\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Flip value=\"" << m_flip2Line <<"\"/>" << std::endl;
    writer.Stream()
         << writer.ind()
             << "<Faces "
                << "FaceCount=\"" <<  m_faces.size() <<
             "\">" << std::endl;

    writer.incInd();
    for (auto& f: m_faces) {
        writer.Stream()
            << writer.ind()
            << "<Face value=\"" << f <<"\"/>" << std::endl;
    }
    writer.decInd();

    writer.Stream() << writer.ind() << "</Faces>" << std::endl;

    writer.Stream()
         << writer.ind()
             << "<Edges "
                << "EdgeCount=\"" <<  m_edges.size() <<
             "\">" << std::endl;

    writer.incInd();
    for (auto& e: m_edges) {
        writer.Stream()
            << writer.ind()
            << "<Edge value=\"" << e <<"\"/>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Edges>" << std::endl;

    writer.Stream()
         << writer.ind()
             << "<CLPoints "
                << "CLPointCount=\"" <<  m_verts.size() <<
             "\">" << std::endl;

    writer.incInd();
    for (auto& p: m_verts) {
        writer.Stream()
            << writer.ind()
            << "<CLPoint value=\"" << p <<"\"/>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</CLPoints>" << std::endl ;

    writer.Stream() << writer.ind() << "<Style value=\"" <<  m_format.m_style << "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Weight value=\"" <<  m_format.m_weight << "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Color value=\"" <<  m_format.m_color.asHexString() << "\"/>" << std::endl;
    const char v = m_format.m_visible?'1':'0';
    writer.Stream() << writer.ind() << "<Visible value=\"" <<  v << "\"/>" << std::endl;

//stored geometry
    if (!m_geometry) {
        return Base::Console().Error("CL::Save - m_geometry is null\n");
    }

    writer.Stream() << writer.ind() << "<GeometryType value=\"" << m_geometry->getGeomType() <<"\"/>" << std::endl;
    if (m_geometry->getGeomType() == TechDraw::GeomType::GENERIC) {
        GenericPtr gen = std::static_pointer_cast<Generic>(m_geometry);
        gen->Save(writer);
    } else if (m_geometry->getGeomType() == TechDraw::GeomType::CIRCLE) {
        TechDraw::CirclePtr circ = std::static_pointer_cast<TechDraw::Circle>(m_geometry);
        circ->Save(writer);
    } else if (m_geometry->getGeomType() == TechDraw::GeomType::ARCOFCIRCLE) {
        TechDraw::AOCPtr aoc = std::static_pointer_cast<TechDraw::AOC>(m_geometry);
        aoc->Save(writer);
    } else {
        Base::Console().Message("CL::Save - unimplemented geomType: %d\n", static_cast<int>(m_geometry->getGeomType()));
    }
}

void CenterLine::Restore(Base::XMLReader &reader)
{
    if (!CosmeticVertex::restoreCosmetic()) {
        return;
    }
//    Base::Console().Message("CL::Restore - reading elements\n");
    // read my Element
    reader.readElement("Start");
    // get the value of my Attribute
    m_start.x = reader.getAttributeAsFloat("X");
    m_start.y = reader.getAttributeAsFloat("Y");
    m_start.z = reader.getAttributeAsFloat("Z");

    reader.readElement("End");
    m_end.x = reader.getAttributeAsFloat("X");
    m_end.y = reader.getAttributeAsFloat("Y");
    m_end.z = reader.getAttributeAsFloat("Z");

    reader.readElement("Mode");
    m_mode = reader.getAttributeAsInteger("value");

    reader.readElement("HShift");
    m_hShift = reader.getAttributeAsFloat("value");
    reader.readElement("VShift");
    m_vShift = reader.getAttributeAsFloat("value");
    reader.readElement("Rotate");
    m_rotate = reader.getAttributeAsFloat("value");
    reader.readElement("Extend");
    m_extendBy = reader.getAttributeAsFloat("value");
    reader.readElement("Type");
    m_type = reader.getAttributeAsInteger("value");
    reader.readElement("Flip");
    m_flip2Line = (bool)reader.getAttributeAsInteger("value")==0?false:true;

    reader.readElement("Faces");
    int count = reader.getAttributeAsInteger("FaceCount");

    int i = 0;
    for ( ; i < count; i++) {
        reader.readElement("Face");
        std::string f = reader.getAttribute("value");
        m_faces.push_back(f);
    }
    reader.readEndElement("Faces");

    reader.readElement("Edges");
    count = reader.getAttributeAsInteger("EdgeCount");

    i = 0;
    for ( ; i < count; i++) {
        reader.readElement("Edge");
        std::string e = reader.getAttribute("value");
        m_edges.push_back(e);
    }
    reader.readEndElement("Edges");

    reader.readElement("CLPoints");
    count = reader.getAttributeAsInteger("CLPointCount");

    i = 0;
    for ( ; i < count; i++) {
        reader.readElement("CLPoint");
        std::string p = reader.getAttribute("value");
        m_verts.push_back(p);
    }
    reader.readEndElement("CLPoints");

    reader.readElement("Style");
    m_format.m_style = reader.getAttributeAsInteger("value");
    reader.readElement("Weight");
    m_format.m_weight = reader.getAttributeAsFloat("value");
    reader.readElement("Color");
    std::string temp = reader.getAttribute("value");
    m_format.m_color.fromHexString(temp);
    reader.readElement("Visible");
    m_format.m_visible = (int)reader.getAttributeAsInteger("value")==0?false:true;

//stored geometry
    reader.readElement("GeometryType");
    TechDraw::GeomType gType = static_cast<TechDraw::GeomType>(reader.getAttributeAsInteger("value"));
    if (gType == TechDraw::GeomType::GENERIC) {
        TechDraw::GenericPtr gen = std::make_shared<TechDraw::Generic> ();
        gen->Restore(reader);
        gen->setOCCEdge(GeometryUtils::edgeFromGeneric(gen));
        m_geometry = gen;
    } else if (gType == TechDraw::GeomType::CIRCLE) {
        TechDraw::CirclePtr circ = std::make_shared<TechDraw::Circle> ();
        circ->Restore(reader);
        circ->setOCCEdge(GeometryUtils::edgeFromCircle(circ));
        m_geometry = circ;
    } else if (gType == TechDraw::GeomType::ARCOFCIRCLE) {
        TechDraw::AOCPtr aoc = std::make_shared<TechDraw::AOC> ();
        aoc->Restore(reader);
        aoc->setOCCEdge(GeometryUtils::edgeFromCircleArc(aoc));
        m_geometry = aoc;
    } else {
        Base::Console().Warning("CL::Restore - unimplemented geomType: %d\n", static_cast<int>(gType));
    }
}

CenterLine* CenterLine::copy() const
{
    CenterLine* newCL = new CenterLine();
    newCL->m_start = m_start;
    newCL->m_end = m_end;
    newCL->m_mode = m_mode;
    newCL->m_hShift = m_hShift;
    newCL->m_vShift = m_vShift;
    newCL->m_rotate = m_rotate;
    newCL->m_extendBy = m_extendBy;
    newCL->m_type = m_type;
    newCL->m_flip2Line = m_flip2Line;

    newCL->m_faces = m_faces;
    newCL->m_edges = m_edges;
    newCL->m_verts = m_verts;

    TechDraw::BaseGeomPtr newGeom = m_geometry->copy();
    newCL->m_geometry = newGeom;

    newCL->m_format = m_format;

    return newCL;
}

boost::uuids::uuid CenterLine::getTag() const
{
    return tag;
}

std::string CenterLine::getTagAsString() const
{
    return boost::uuids::to_string(getTag());
}

void CenterLine::createNewTag()
{
    // Initialize a random number generator, to avoid Valgrind false positives.
    static boost::mt19937 ran;
    static bool seeded = false;

    if (!seeded) {
        ran.seed(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }
    static boost::uuids::basic_random_generator<boost::mt19937> gen(&ran);


    tag = gen();
}

void CenterLine::assignTag(const TechDraw::CenterLine* ce)
{
    if(ce->getTypeId() == this->getTypeId())
        this->tag = ce->tag;
    else
        throw Base::TypeError("CenterLine tag can not be assigned as types do not match.");
}

CenterLine *CenterLine::clone() const
{
    CenterLine* cpy = this->copy();
    cpy->tag = this->tag;

    return cpy;
}

// To do: make const
PyObject* CenterLine::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new CenterLinePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}


void CenterLine::setShifts(const double h, const double v)
{
    m_hShift = h;
    m_vShift = v;
}

double CenterLine::getHShift() const
{
    return m_hShift;
}

double CenterLine::getVShift() const
{
    return m_vShift;
}

void CenterLine::setRotate(const double r)
{
    m_rotate = r;
}

double CenterLine::getRotate() const
{
    return m_rotate;
}

void CenterLine::setExtend(const double e)
{
    m_extendBy = e;
}

double CenterLine::getExtend() const
{
    return m_extendBy;
}

void CenterLine::setFlip(const bool f)
{
    m_flip2Line = f;
}

bool CenterLine::getFlip() const
{
    return m_flip2Line;
}


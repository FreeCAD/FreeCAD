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

# include "PreCompiled.h"
#ifndef _PreComp_

# include <gp_Ax2.hxx>
# include <gp_Circ.hxx>
# include <gp_Dir.hxx>
# include <gp_Elips.hxx>
# include <gp_Pln.hxx>
# include <gp_Vec.hxx>

# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>

# include <HLRTopoBRep_OutLiner.hxx>
# include <HLRBRep.hxx>
# include <HLRBRep_Algo.hxx>
# include <HLRBRep_Data.hxx>
# include <HLRBRep_EdgeData.hxx>
# include <HLRAlgo_EdgeIterator.hxx>
# include <HLRBRep_HLRToShape.hxx>
# include <HLRAlgo_Projector.hxx>
# include <HLRBRep_ShapeBounds.hxx>

# include <Poly_Polygon3D.hxx>
# include <Poly_Triangulation.hxx>
# include <Poly_PolygonOnTriangulation.hxx>

# include <TopoDS.hxx>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Wire.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Builder.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
# include <TopTools_ListOfShape.hxx>
# include <TColgp_Array1OfPnt2d.hxx>

# include <BRep_Tool.hxx>
# include <BRepMesh.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <BRep_Builder.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepTools_WireExplorer.hxx>
# include <ShapeFix_Wire.hxx>
# include <BRepProj_Projection.hxx>

# include <BRepAdaptor_HCurve.hxx>
# include <BRepAdaptor_CompCurve.hxx>

// # include <Handle_BRepAdaptor_HCompCurve.hxx>
# include <Approx_Curve3d.hxx>

# include <BRepAdaptor_HCurve.hxx>
# include <Handle_HLRBRep_Data.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Geom_BezierCurve.hxx>
# include <GeomConvert_BSplineCurveToBezierCurve.hxx>
# include <GeomConvert_BSplineCurveKnotSplitting.hxx>
# include <Geom2d_BSplineCurve.hxx>

#include <ProjLib_Plane.hxx>
#endif  // #ifndef _PreComp_

#include <algorithm>

# include <Base/Console.h>
# include <Base/Exception.h>
# include <Base/FileInfo.h>
# include <Base/Tools.h>

# include <Mod/Part/App/PartFeature.h>

# include "GeometryObject.h"

//#include <QDebug>

using namespace TechDrawGeometry;

struct EdgePoints {
    gp_Pnt v1, v2;
    TopoDS_Edge edge;
};

GeometryObject::GeometryObject() : brep_hlr(0), Tolerance(0.05f), Scale(1.f)
{
}

GeometryObject::~GeometryObject()
{
    clear();
}

void GeometryObject::setTolerance(double value)
{
    Tolerance = value;
}

void GeometryObject::setScale(double value)
{
    Scale = value;
}

void GeometryObject::clear()
{

    for(std::vector<BaseGeom *>::iterator it = edgeGeom.begin(); it != edgeGeom.end(); ++it) {
        delete *it;
        *it = 0;
    }

    for(std::vector<Face *>::iterator it = faceGeom.begin(); it != faceGeom.end(); ++it) {
        delete *it;
        *it = 0;
    }

    for(std::vector<Vertex *>::iterator it = vertexGeom.begin(); it != vertexGeom.end(); ++it) {
        delete *it;
        *it = 0;
    }

    vertexGeom.clear();
    vertexReferences.clear();

    faceGeom.clear();
    faceReferences.clear();

    edgeGeom.clear();
    edgeReferences.clear();
}

void GeometryObject::drawFace (const bool visible, const int iface,
                               Handle_HLRBRep_Data & DS,
                               TopoDS_Shape& Result) const
{
// add all the edges for this face(iface) to Result
    HLRBRep_FaceIterator Itf;

    for (Itf.InitEdge(DS->FDataArray().ChangeValue(iface)); Itf.MoreEdge(); Itf.NextEdge()) {
        int ie = Itf.Edge();
   //     if (std::find(used.begin(),used.end(),ie) == used.end()) {              //only use an edge once
            HLRBRep_EdgeData& edf = DS->EDataArray().ChangeValue(ie);
            if(edf.Status().NbVisiblePart() > 0) {
                drawEdge(edf, Result, visible);
            }
//            double first = edf.Geometry().FirstParameter();
//            double last = edf.Geometry().LastParameter();
//            gp_Pnt p0 = edf.Geometry().Value3D(first);
//            gp_Pnt p1 = edf.Geometry().Value3D(last);
//            qDebug()<<p0.X()<<','<<p0.Y()<<','<<p0.Z()<<"\t - \t"<<p1.X()<<','<<p1.Y()<<','<<p1.Z();
//
 //           used.push_back(ie);
 //       }
    }
}

void GeometryObject::drawEdge(HLRBRep_EdgeData& ed, TopoDS_Shape& Result, const bool visible) const
{
    double sta,end;
    float tolsta,tolend;

    BRep_Builder B;
    TopoDS_Edge E;
    HLRAlgo_EdgeIterator It;

    if (visible) {
        for(It.InitVisible(ed.Status()); It.MoreVisible(); It.NextVisible()) {
            It.Visible(sta,tolsta,end,tolend);
            E = HLRBRep::MakeEdge(ed.Geometry(),sta,end);
            if (!E.IsNull()) {
                B.Add(Result,E);
            }
        }
    } else {
        for(It.InitHidden(ed.Status()); It.MoreHidden(); It.NextHidden()) {
            It.Hidden(sta,tolsta,end,tolend);
            E = HLRBRep::MakeEdge(ed.Geometry(),sta,end);
            if (!E.IsNull()) {
                B.Add(Result,E);
            }
        }
    }
}

//! only ever called from FVP::getVertex
DrawingGeometry::Vertex * GeometryObject::projectVertex(const TopoDS_Shape &vert,
                                                        const TopoDS_Shape &support,
                                                        const Base::Vector3d &direction,
                                                        const Base::Vector3d &projXAxis) const
{
    if(vert.IsNull())
        throw Base::Exception("Projected vertex is null");

    gp_Pnt supportCentre = findCentroid(support, direction, projXAxis);

    // mirror+scale vert around centre of support
    gp_Trsf mat;
    mat.SetMirror(gp_Ax2(supportCentre, gp_Dir(0, 1, 0)));
    gp_Trsf matScale;
    matScale.SetScale(supportCentre, Scale);
    mat.Multiply(matScale);

    //TODO: See if it makes sense to use gp_Trsf::Transforms() instead
    BRepBuilderAPI_Transform mkTrfScale(vert, mat);
    const TopoDS_Vertex &refVert = TopoDS::Vertex(mkTrfScale.Shape());

    gp_Ax2 transform;
    transform = gp_Ax2(supportCentre,
                       gp_Dir(direction.x, direction.y, direction.z),
                       gp_Dir(projXAxis.x, projXAxis.y, projXAxis.z));

    HLRAlgo_Projector projector = HLRAlgo_Projector( transform );
    projector.Scaled(true);
    // If the index was found and is unique, the point is projected using the HLR Projector Algorithm
    gp_Pnt2d prjPnt;
    projector.Project(BRep_Tool::Pnt(refVert), prjPnt);
    DrawingGeometry::Vertex *myVert = new Vertex(prjPnt.X(), prjPnt.Y());
    return myVert;
}

//only used by DrawViewSection so far
void GeometryObject::projectSurfaces(const TopoDS_Shape &face,
                                     const TopoDS_Shape &support,
                                     const Base::Vector3d &direction,
                                     const Base::Vector3d &xaxis,
                                     std::vector<DrawingGeometry::Face *> &projFaces) const
{
    if(face.IsNull())
        throw Base::Exception("Projected shape is null");

    gp_Pnt supportCentre = findCentroid(support, direction, xaxis);

    // TODO: We used to invert Y twice here, make sure that wasn't intentional
    gp_Trsf mat;
    mat.SetMirror(gp_Ax2(supportCentre, gp_Dir(0, 1, 0)));
    gp_Trsf matScale;
    matScale.SetScale(supportCentre, Scale);
    mat.Multiply(matScale);

    BRepBuilderAPI_Transform mkTrfScale(face, mat);

    gp_Ax2 transform;
    transform = gp_Ax2(supportCentre,
                       gp_Dir(direction.x, direction.y, direction.z),
                       gp_Dir(xaxis.x, xaxis.y, xaxis.z));

    HLRBRep_Algo *brep_hlr = new HLRBRep_Algo();
    brep_hlr->Add(mkTrfScale.Shape());

    HLRAlgo_Projector projector( transform );
    brep_hlr->Projector(projector);
    brep_hlr->Update();
    brep_hlr->Hide();

    Base::Console().Log("GeometryObject::projectSurfaces - projecting face\n");

    // Extract Faces
    std::vector<int> projFaceRefs;

    extractFaces(brep_hlr, mkTrfScale.Shape(), true, WithSmooth, projFaces, projFaceRefs);
    delete brep_hlr;
}

Base::BoundBox3d GeometryObject::boundingBoxOfBspline(const BSpline *spline) const
{
    Base::BoundBox3d bb;
    for (std::vector<BezierSegment>::const_iterator segItr( spline->segments.begin() );
         segItr != spline->segments.end(); ++segItr) {
        switch (segItr->poles) {
            case 0:   // Degenerate, but safe ignore
                break;
            case 2:   // Degenerate - straight line
                bb.Add(Base::Vector3d( segItr->pnts[1].fX,
                                       segItr->pnts[1].fY,
                                       0 ));
                // fall through
            case 1:   // Degenerate - just a point
                bb.Add(Base::Vector3d( segItr->pnts[0].fX,
                                       segItr->pnts[0].fY,
                                       0 ));
                break;
            case 3: {
                double
                    px[3] = { segItr->pnts[0].fX,
                              segItr->pnts[1].fX,
                              segItr->pnts[2].fX },
                    py[3] = { segItr->pnts[0].fY,
                              segItr->pnts[1].fY,
                              segItr->pnts[2].fY },
                    slns[4] = { 0, 1 }; // Consider the segment's end points

                // if's are to prevent problems with divide-by-0
                if ((2 * px[1] - px[0] - px[2]) == 0) {
                    slns[2] = -1;
                } else {
                    slns[2] = (px[1] - px[0]) / (2 * px[1] - px[0] - px[2]);
                }
                if ((2 * py[1] - py[0] - py[2]) == 0) {
                    slns[3] = -1;
                } else {
                    slns[3] = (py[1] - py[0]) / (2 * py[1] - py[0] - py[2]);
                }

                // evaluate B(t) at the endpoints and zeros
                for (int s(0); s < 4; ++s) {
                    double t( slns[s] );
                    if (t < 0 || t > 1) {
                        continue;
                    }
                    double tx( px[0] * (1 - t) * (1 - t) +
                               px[1] * 2 * (1 - t) * t +
                               px[2] * t * t ),
                           ty( py[0] * (1 - t) * (1 - t) +
                               py[1] * 2 * (1 - t) * t +
                               py[2] * t * t );
                    bb.Add( Base::Vector3d(tx, ty, 0) );
                }
                    } break;
            case 4: {
                double
                    px[4] = { segItr->pnts[0].fX,
                              segItr->pnts[1].fX,
                              segItr->pnts[2].fX,
                              segItr->pnts[3].fX },
                    py[4] = { segItr->pnts[0].fY,
                              segItr->pnts[1].fY,
                              segItr->pnts[2].fY,
                              segItr->pnts[3].fY },
                    // If B(t) is the cubic Bezier, find t where B'(t) == 0
                    //
                    // For control points P0-P3, B'(t) works out to be:
                    // B'(t) = t^2 * (-3P0 + 9P1 - 9P2 + 3P3) +
                    //          t  * (6P0 - 12P1 + 6P2) +
                    //          3  * (P1 - P0)
                    //
                    // So, we use the quadratic formula!
                    ax = -3 * px[0] + 9 * px[1] - 9 * px[2] + 3 * px[3],
                    ay = -3 * py[0] + 9 * py[1] - 9 * py[2] + 3 * py[3],
                    bx = 6 * px[0] - 12 * px[1] + 6 * px[2],
                    by = 6 * py[0] - 12 * py[1] + 6 * py[2],
                    cx = 3 * px[1] - 3 * px[0],
                    cy = 3 * py[1] - 3 * py[0],

                    slns[6] = { 0, 1 }; // Consider the segment's end points

                // if's are to prevent problems with divide-by-0 and NaN
                if ( (2 * ax) == 0 || (bx * bx - 4 * ax * cx) < 0 ) {
                    slns[2] = -1;
                    slns[3] = -1;
                } else {
                    slns[2] = (-bx + sqrt(bx * bx - 4 * ax * cx)) / (2 * ax);
                    slns[3] = (-bx - sqrt(bx * bx - 4 * ax * cx)) / (2 * ax);
                }
                if ((2 * ay) == 0 || (by * by - 4 * ay * cy) < 0 ) {
                    slns[4] = -1;
                    slns[5] = -1;
                } else {
                    slns[4] = (-by + sqrt(by * by - 4 * ay * cy)) / (2 * ay);
                    slns[5] = (-by - sqrt(by * by - 4 * ay * cy)) / (2 * ay);
                }

                // evaluate B(t) at the endpoints and zeros
                for (int s(0); s < 6; ++s) {
                    double t( slns[s] );
                    if (t < 0 || t > 1) {
                        continue;
                    }
                    double tx( px[0] * (1 - t) * (1 - t) * (1 - t) +
                               px[1] * 3 * (1 - t) * (1 - t) * t +
                               px[2] * 3 * (1 - t) * t * t +
                               px[3] * t * t * t ),
                           ty( py[0] * (1 - t) * (1 - t) * (1 - t) +
                               py[1] * 3 * (1 - t) * (1 - t) * t +
                               py[2] * 3 * (1 - t) * t * t +
                               py[3] * t * t * t );
                    bb.Add( Base::Vector3d(tx, ty, 0) );
                }

                } break;
            default:
                throw Base::Exception("Invalid degree bezier segment in GeometryObject::calcBoundingBox");
        }
    }
    return bb;
}

Base::BoundBox3d GeometryObject::boundingBoxOfAoe(const Ellipse *aoe,
                                                  double start,
                                                  double end, bool cw) const
{
    // Using the ellipse form:
    // (xc, yc) = centre of ellipse
    // phi = angle of ellipse major axis off X axis
    // a, b = half of major, minor axes
    //
    // x(theta) = xc + a*cos(theta)*cos(phi) - b*sin(theta)*sin(phi)
    // y(theta) = yc + a*cos(theta)*sin(phi) + b*sin(theta)*cos(phi)

    double a (aoe->major / 2.0),
           b (aoe->minor / 2.0),
           phi (aoe->angle),
           xc (aoe->center.fX),
           yc (aoe->center.fY);

    if (a == 0 || b == 0) {
        // Degenerate case - TODO: handle as line instead of throwing
        throw Base::Exception("Ellipse with invalid major axis in GeometryObject::boundingBoxOfAoe()");
    }

    // Calculate points of interest for the bounding box.  These are points
    // where d(x)/d(theta) and d(y)/d(theta) = 0 (where the x and y extremes
    // of the ellipse would be if it were complete), and arc endpoints.
    double testAngles[6] = { atan(tan(phi) * (-b / a)),
                             testAngles[0] + M_PI };
    if (tan(phi) == 0) {
        testAngles[2] = M_PI / 2.0;
        testAngles[3] = 3.0 * M_PI / 2.0;
    } else {
        testAngles[2] = atan((1.0 / tan(phi)) * (b / a));
        testAngles[3] = testAngles[2] + M_PI;
    }
    testAngles[4] = start;
    testAngles[5] = end;

    // Add extremes to bounding box, if they are within the arc
    Base::BoundBox3d bb;
    for (int ai(0); ai < 6; ++ai) {
        double theta(testAngles[ai]);
        if (isWithinArc(theta, start, end, cw) ) {
            bb.Add( Base::Vector3d(xc + a*cos(theta)*cos(phi) - b*sin(theta)*sin(phi),
                                   yc + a*cos(theta)*sin(phi) - b*sin(theta)*cos(phi),
                                   0) );
        }
    }

    return bb;
}

bool GeometryObject::isWithinArc(double theta, double first,
                                 double last, bool cw) const
{
    if (fabs(last - first) >= 2 * M_PI) {
        return true;
    }

    // Put params within [0, 2*pi) - not totally sure this is necessary
    theta = fmod(theta, 2 * M_PI);
    if (theta < 0) {
        theta += 2 * M_PI;
    }

    first = fmod(first, 2 * M_PI);
    if (first < 0) {
        first += 2 * M_PI;
    }

    last = fmod(last, 2 * M_PI);
    if (last < 0) {
        last += 2 * M_PI;
    }

    if (cw) {
        if (first > last) {
            return theta <= first && theta >= last;
        } else {
            return theta <= first || theta >= last;
        }
    } else {
        if (first > last) {
            return theta >= first || theta <= last;
        } else {
            return theta >= first && theta <= last;
        }
    }
}

Base::BoundBox3d GeometryObject::calcBoundingBox() const
{
    Base::BoundBox3d bbox;

    // First calculate bounding box based on vertices
    for(std::vector<Vertex *>::const_iterator it( vertexGeom.begin() );
            it != vertexGeom.end(); ++it) {
        bbox.Add( Base::Vector3d((*it)->pnt.fX, (*it)->pnt.fY, 0.) );
    }

    // Now, consider geometry where vertices don't define bounding box eg circles
    for (std::vector<BaseGeom *>::const_iterator it( edgeGeom.begin() );
            it != edgeGeom.end(); ++it) {
        switch ((*it)->geomType) {
          case CIRCLE: {
              Circle *c = static_cast<Circle *>(*it);
              bbox.Add( Base::BoundBox3d(-c->radius + c->center.fX,
                                         -c->radius + c->center.fY,
                                         0,
                                         c->radius + c->center.fX,
                                         c->radius + c->center.fY,
                                         0) );
          } break;

          case ARCOFCIRCLE: {
              AOC *arc = static_cast<AOC *>(*it);

              // Endpoints of arc
              bbox.Add( Base::Vector3d(arc->radius * cos(arc->startAngle),
                                       arc->radius * sin(arc->startAngle),
                                       0.0) );
              bbox.Add( Base::Vector3d(arc->radius * cos(arc->endAngle),
                                       arc->radius * sin(arc->endAngle),
                                       0.0) );

              // Extreme X and Y values if they're within the arc
              for (double theta = 0.0; theta < 6.5; theta += M_PI / 2.0) {
                  if (isWithinArc(theta, arc->startAngle, arc->endAngle, arc->cw)) {
                      bbox.Add( Base::Vector3d(arc->radius * cos(theta),
                                arc->radius * sin(theta),
                                0.0) );
                  }
              }
          } break;

          case ELLIPSE: {
              bbox.Add( boundingBoxOfAoe(static_cast<Ellipse *>(*it)) );
          } break;

          case ARCOFELLIPSE: {
              AOE *aoe = static_cast<AOE *>(*it);
              double start = aoe->startAngle,
                     end = aoe->endAngle;
              bool cw = aoe->cw;
              bbox.Add( boundingBoxOfAoe(static_cast<Ellipse *>(*it), start, end, cw) );
          } break;

          case BSPLINE: {
              bbox.Add( boundingBoxOfBspline(static_cast<BSpline *>(*it)) );
          } break;

          case GENERIC: {
              // this case ends up just drawing line segments between points
              Generic *gen = static_cast<Generic *>(*it);
              for (std::vector<Base::Vector2D>::const_iterator segIt = gen->points.begin();
                      segIt != gen->points.end(); ++segIt) {
                  bbox.Add( Base::Vector3d(segIt->fX, segIt->fY, 0) );
              }
          } break;

          default:
              throw Base::Exception("Unknown geomType in GeometryObject::calcBoundingBox()");
        }
    }
    return bbox;
}

//! only ever called from fvp::getCompleteEdge
DrawingGeometry::BaseGeom * GeometryObject::projectEdge(const TopoDS_Shape &edge,
                                                        const TopoDS_Shape &support,
                                                        const Base::Vector3d &direction,
                                                        const Base::Vector3d &projXAxis) const
{
    if(edge.IsNull())
        throw Base::Exception("Projected edge is null");
    // Invert y function using support to calculate bounding box

    gp_Pnt supportCentre = findCentroid(support, direction, projXAxis);

    gp_Trsf mat;
    mat.SetMirror(gp_Ax2(supportCentre, gp_Dir(0, 1, 0)));
    gp_Trsf matScale;
    matScale.SetScale(supportCentre, Scale);
    mat.Multiply(matScale);
    BRepBuilderAPI_Transform mkTrfScale(edge, mat);

    const TopoDS_Edge &refEdge = TopoDS::Edge(mkTrfScale.Shape());

    gp_Ax2 transform;
    transform = gp_Ax2(supportCentre,
                       gp_Dir(direction.x, direction.y, direction.z),
                       gp_Dir(projXAxis.x, projXAxis.y, projXAxis.z));

    BRepAdaptor_Curve refCurve(refEdge);
    HLRAlgo_Projector projector = HLRAlgo_Projector( transform );
    projector.Scaled(true);

    if (refCurve.GetType() == GeomAbs_Line) {
        // Use the simpler algorithm for lines
        gp_Pnt p1 = refCurve.Value(refCurve.FirstParameter());
        gp_Pnt p2 = refCurve.Value(refCurve.LastParameter());

        // Project the points
        gp_Pnt2d pnt1, pnt2;
        projector.Project(p1, pnt1);
        projector.Project(p2, pnt2);

        DrawingGeometry::Generic *line = new DrawingGeometry::Generic();

        line->points.push_back(Base::Vector2D(pnt1.X(), pnt1.Y()));
        line->points.push_back(Base::Vector2D(pnt2.X(), pnt2.Y()));

        return line;

    } else {

        HLRBRep_Curve curve;
        curve.Curve(refEdge);

        curve.Projector(&projector);

        DrawingGeometry::BaseGeom *result = 0;
        switch(HLRBRep_BCurveTool::GetType(curve.Curve()))
        {
        case GeomAbs_Line: {
              DrawingGeometry::Generic *line = new DrawingGeometry::Generic();

              gp_Pnt2d pnt1 = curve.Value(curve.FirstParameter());
              gp_Pnt2d pnt2 = curve.Value(curve.LastParameter());

              line->points.push_back(Base::Vector2D(pnt1.X(), pnt1.Y()));
              line->points.push_back(Base::Vector2D(pnt2.X(), pnt2.Y()));

              result = line;
            }break;
        case GeomAbs_Circle: {
              DrawingGeometry::Circle *circle = new DrawingGeometry::Circle();
                gp_Circ2d prjCirc = curve.Circle();

                double f = curve.FirstParameter();
                double l = curve.LastParameter();
                gp_Pnt2d s = curve.Value(f);
                gp_Pnt2d e = curve.Value(l);

                if (fabs(l-f) > 1.0 && s.SquareDistance(e) < 0.001) {
                      circle->radius = prjCirc.Radius();
                      circle->center = Base::Vector2D(prjCirc.Location().X(), prjCirc.Location().Y());
                      result = circle;
                } else {
                      AOC *aoc = new AOC();
                      aoc->radius = prjCirc.Radius();
                      aoc->center = Base::Vector2D(prjCirc.Location().X(), prjCirc.Location().Y());
                      double ax = s.X() - aoc->center.fX;
                      double ay = s.Y() - aoc->center.fY;
                      double bx = e.X() - aoc->center.fX;
                      double by = e.Y() - aoc->center.fY;

                      aoc->startAngle = atan2(ay,ax);
                      float range = atan2(-ay*bx+ax*by, ax*bx+ay*by);

                      aoc->endAngle = aoc->startAngle + range;
                      result = aoc;
                }
              } break;
        case GeomAbs_Ellipse: {
                gp_Elips2d prjEllipse = curve.Ellipse();

                double f = curve.FirstParameter();
                double l = curve.LastParameter();
                gp_Pnt2d s = curve.Value(f);
                gp_Pnt2d e = curve.Value(l);

                if (fabs(l-f) > 1.0 && s.SquareDistance(e) < 0.001) {
                      Ellipse *ellipse = new Ellipse();
                      ellipse->major = prjEllipse.MajorRadius();
                      ellipse->minor = prjEllipse.MinorRadius();
                      ellipse->center = Base::Vector2D(prjEllipse.Location().X(),
                                                       prjEllipse.Location().Y());
                      result = ellipse;
                      // TODO: Finish implementing ellipsey stuff
                } else {
                      // TODO implement this correctly
                      AOE *aoe = new AOE();
                      aoe->major = prjEllipse.MajorRadius();
                      aoe->minor = prjEllipse.MinorRadius();
                      aoe->center = Base::Vector2D(prjEllipse.Location().X(),
                                                   prjEllipse.Location().Y());
                      result =  aoe;
                }
              } break;
        case GeomAbs_BSplineCurve: {
                  // TODO: Project BSpline here?
              } break;

        default:
              break;
        }

        return result;
    }
}

/* TODO: Clean this up when faces are actually working properly...
void debugEdge(const TopoDS_Edge &e)
{
    gp_Pnt p0 = BRep_Tool::Pnt(TopExp::FirstVertex(e));
    gp_Pnt p1 = BRep_Tool::Pnt(TopExp::LastVertex(e));
    qDebug()<<p0.X()<<','<<p0.Y()<<','<<p0.Z()<<"\t - \t"<<p1.X()<<','<<p1.Y()<<','<<p1.Z();
}*/

void GeometryObject::extractFaces(HLRBRep_Algo *myAlgo,
                                  const TopoDS_Shape &S,
                                  bool visible,
                                  ExtractionType extractionType,
                                  std::vector<DrawingGeometry::Face *> &projFaces,
                                  std::vector<int> &faceRefs) const
{
#if MOD_TECHDRAW_HANDLE_FACES
    if(!myAlgo)
        return;

    Handle_HLRBRep_Data DS = myAlgo->DataStructure();
    if (DS.IsNull()) {
        Base::Console().Log("TechDraw::GeometryObject::extractFaces - DS is Null\n");
        return;
    }

    DS->Projector().Scaled(true);

    int f1 = 1;
    int f2 = DS->NbFaces();

    /* This block seems to set f1 and f2 to indices using a HLRBRep_ShapeBounds
     * object based that's based on myAlgo, but DS is also based on myAlgo too,
     * so I don't think this is required. IR
    if (!S.IsNull()) {
        int e1 = 1;
        int e2 = DS->NbEdges();

        Standard_Integer v1,v2;
        Standard_Integer index = myAlgo->Index(S);
        if(index == 0)  {
            Base::Console().Log("TechDraw::GeometryObject::extractFaces - myAlgo->Index(S) == 0\n");
            return;
        }

        myAlgo->ShapeBounds(index).Bounds(v1, v2, e1, e2, f1, f2);
    } */

    TopTools_IndexedMapOfShape anfIndices;
    TopTools_IndexedMapOfShape& Faces = DS->FaceMap();
    TopExp::MapShapes(S, TopAbs_FACE, anfIndices);

    BRep_Builder B;

    /* ----------------- Extract Faces ------------------ */
    for (int iface = f1; iface <= f2; iface++) {
        // Why oh why does Hiding() == true mean that a face is visible...
        if (! DS->FDataArray().ChangeValue(iface).Hiding()) {
            continue;
        }

        TopoDS_Shape face;
        B.MakeCompound(TopoDS::Compound(face));

        // Generate a set of new wires based on face
        // TODO: Do these end up with input face's geometry as a base?
        drawFace(visible, iface, DS, face);
        std::vector<TopoDS_Wire> possibleFaceWires;
        createWire(face, possibleFaceWires);

        DrawingGeometry::Face *myFace = NULL;

        // Process each wire - if we can make at least one face with it, then
        // send it down the road toward rendering
        for (std::vector<TopoDS_Wire>::iterator wireIt = possibleFaceWires.begin();
            wireIt != possibleFaceWires.end(); ++wireIt) {

            // Try making a face out of the wire, before doing anything else with it
            BRepBuilderAPI_MakeFace testFace(*wireIt);
            if (testFace.IsDone()) {
                if (myFace == NULL) {
                   myFace = new DrawingGeometry::Face();
                }
                DrawingGeometry::Wire *genWire = new DrawingGeometry::Wire();

                // See createWire regarding BRepTools_WireExplorer vs TopExp_Explorer
                BRepTools_WireExplorer explr(*wireIt);
                while (explr.More()) {
                    BRep_Builder builder;
                    TopoDS_Compound comp;
                    builder.MakeCompound(comp);
                    builder.Add(comp, explr.Current());

                    calculateGeometry(comp, Plain, genWire->geoms);
                    explr.Next();
                }
                myFace->wires.push_back(genWire);
            }
        }

        if (myFace != NULL) {
            projFaces.push_back(myFace);
        }

        // I'm pretty sure this doesn't do what it's intended to do.  IR
        int idxFace;
        for (int i = 1; i <= anfIndices.Extent(); i++) {
            idxFace = Faces.FindIndex(anfIndices(iface));
            if (idxFace != 0) {
                break;
            }
        }

        if(idxFace == 0)
            idxFace = -1; // If Face not found - select hidden

        // Push the found face index onto references stack
        faceRefs.push_back(idxFace);
    }

    DS->Projector().Scaled(false);
#endif //#if MOD_TECHDRAW_HANDLE_FACES
}

bool GeometryObject::shouldDraw(const bool inFace, const int typ, HLRBRep_EdgeData& ed)
{
    bool todraw = false;
    if(inFace)
        todraw = true;
    else if (typ == 3)
        todraw = ed.Rg1Line() && !ed.RgNLine();   //smooth + !contour?
    else if (typ == 4)
        todraw = ed.RgNLine();                    //contour?
    else
        todraw =!ed.Rg1Line();                    //!smooth?

    return todraw;
}

void GeometryObject::extractVerts(HLRBRep_Algo *myAlgo, const TopoDS_Shape &S, HLRBRep_EdgeData& ed, int ie, ExtractionType extractionType)
{
    if(!myAlgo)
        return;

    Handle_HLRBRep_Data DS = myAlgo->DataStructure();

    if (DS.IsNull())
      return;

    DS->Projector().Scaled(true);

    TopTools_IndexedMapOfShape anIndices;
    TopTools_IndexedMapOfShape anvIndices;

    TopExp::MapShapes(S, TopAbs_EDGE, anIndices);
    TopExp::MapShapes(S, TopAbs_VERTEX, anvIndices);

    // Load the edge
    if(ie < 0) {

    } else {
        TopoDS_Shape shape = anIndices.FindKey(ie);
        TopoDS_Edge edge = TopoDS::Edge(shape);

        // Gather a list of points associated with this curve
        std::list<TopoDS_Shape> edgePoints;

        TopExp_Explorer xp;
        xp.Init(edge,TopAbs_VERTEX);
        while(xp.More()) {
            edgePoints.push_back(xp.Current());
            xp.Next();
        }
        for(std::list<TopoDS_Shape>::const_iterator it = edgePoints.begin(); it != edgePoints.end(); ++it) {

            // Should share topological data structure so can reference
            int iv = anvIndices.FindIndex(*it); // Index of the found vertex

            if(iv < 0)
                continue;

            // Check if vertex has already been addded
            std::vector<int>::iterator vert;
            vert = std::find(vertexReferences.begin(), vertexReferences.end(), iv);

            if(vert == vertexReferences.end()) {

                // If the index wasnt found and is unique, the point is projected using the HLR Projector Algorithm
                gp_Pnt2d prjPnt;
                DS->Projector().Project(BRep_Tool::Pnt(TopoDS::Vertex(*it)), prjPnt);

                // Check if this point lies on a visible section of the projected curve
                double sta,end;
                float tolsta,tolend;

                // There will be multiple edges that form the total edge so collect these
                BRep_Builder B;
                TopoDS_Compound comp;
                B.MakeCompound(comp);

                TopoDS_Edge E;
                HLRAlgo_EdgeIterator It;

                for(It.InitVisible(ed.Status()); It.MoreVisible(); It.NextVisible()) {
                    It.Visible(sta,tolsta,end,tolend);

                    E = HLRBRep::MakeEdge(ed.Geometry(),sta,end);
                    if (!E.IsNull()) {
                        B.Add(comp,E);
                    }
                }

                bool vertexVisible = false;
                TopExp_Explorer exp;
                exp.Init(comp,TopAbs_VERTEX);
                while(exp.More()) {

                    gp_Pnt pnt = BRep_Tool::Pnt(TopoDS::Vertex(exp.Current()));
                    gp_Pnt2d edgePnt(pnt.X(), pnt.Y());
                    if(edgePnt.SquareDistance(prjPnt) < Precision::Confusion()) {
                        vertexVisible = true;
                        break;
                    }
                    exp.Next();
                }

                if(vertexVisible) {
                    Vertex *myVert = new Vertex(prjPnt.X(), prjPnt.Y());
                    vertexGeom.push_back(myVert);
                    vertexReferences.push_back(iv);
                }
            }
        }
    }
}

void GeometryObject::extractEdges(HLRBRep_Algo *myAlgo, const TopoDS_Shape &S, int type, bool visible, ExtractionType extractionType)
{
    if (!myAlgo)
      return;
    Handle_HLRBRep_Data DS = myAlgo->DataStructure();

    if (DS.IsNull())
        return;

    DS->Projector().Scaled(true);

    int e1 = 1;
    int e2 = DS->NbEdges();
    int f1 = 1;
    int f2 = DS->NbFaces();

    if (!S.IsNull()) {
        int v1,v2;
        int index = myAlgo->Index(S);
        if(index == 0) {
            Base::Console().Log("TechDraw::GeometryObject::extractEdges - myAlgo->Index(S) == 0\n");
            return;
        }
        myAlgo->ShapeBounds(index).Bounds(v1,v2,e1,e2,f1,f2);
    }

    HLRBRep_EdgeData* ed = &(DS->EDataArray().ChangeValue(e1 - 1));

    // Get map of edges and faces from projected geometry
    TopTools_IndexedMapOfShape& Edges = DS->EdgeMap();
    TopTools_IndexedMapOfShape anIndices;

    TopExp::MapShapes(S, TopAbs_EDGE, anIndices);

    for (int j = e1; j <= e2; j++) {
        ed++;
        if (ed->Selected() && !ed->Vertical()) {
            ed->Used(false);
            ed->HideCount(0);

        } else {
            ed->Used(true);
        }
    }

    BRep_Builder B;

    std::list<int> notFound;
    /* ----------------- Extract Edges ------------------ */
    for (int i = 1; i <= anIndices.Extent(); i++) {
        int ie = Edges.FindIndex(anIndices(i));
        if (ie != 0) {

            HLRBRep_EdgeData& ed = DS->EDataArray().ChangeValue(ie);
            if(!ed.Used()) {
                if(true) {

                    TopoDS_Shape result;
                    B.MakeCompound(TopoDS::Compound(result));

                    drawEdge(ed, result, visible);

                    // Extract and Project Vertices for current Edge
                    extractVerts(myAlgo, S, ed, i, extractionType);

                    int edgesAdded = calculateGeometry(result, extractionType, edgeGeom);

                    // Push the edge references
                    while(edgesAdded--)
                        edgeReferences.push_back(i);
                }

                ed.Used(Standard_True);
            }
        } else {
            notFound.push_back(i);
        }
    }



    // Add any remaining edges that couldn't be found
    int edgeIdx = -1; // Negative index for edge references
    for (int ie = e1; ie <= e2; ie++) {
      // Co
      HLRBRep_EdgeData& ed = DS->EDataArray().ChangeValue(ie);
      if (!ed.Used()) {
          if(shouldDraw(false, type, ed)) {
              const TopoDS_Shape &shp = Edges.FindKey(ie);

              //Compares original shape to see if match
              if(!shp.IsNull()) {
                  const TopoDS_Edge& edge = TopoDS::Edge(shp);
                  BRepAdaptor_Curve adapt1(edge);
                  for (std::list<int>::iterator it= notFound.begin(); it!= notFound.end(); ++it){
                      BRepAdaptor_Curve adapt2(TopoDS::Edge(anIndices(*it)));
                      if(isSameCurve(adapt1, adapt2)) {
                          edgeIdx = *it;
//                           notFound.erase(it);
                          break;
                      }
                  }
              }

              TopoDS_Shape result;
              B.MakeCompound(TopoDS::Compound(result));

              drawEdge(ed, result, visible);
              int edgesAdded = calculateGeometry(result, extractionType, edgeGeom);

              // Push the edge references
              while(edgesAdded--)
                  edgeReferences.push_back(edgeIdx);
          }
          ed.Used(true);
      }
    }

    DS->Projector().Scaled(false);
}

/**
 * Note projected edges are broken up so start and end parameters differ.
 */
bool GeometryObject::isSameCurve(const BRepAdaptor_Curve &c1, const BRepAdaptor_Curve &c2) const
{


    if(c1.GetType() != c2.GetType())
        return false;
#if 0
    const gp_Pnt& p1S = c1.Value(c1.FirstParameter());
    const gp_Pnt& p1E = c1.Value(c1.LastParameter());

    const gp_Pnt& p2S = c2.Value(c2.FirstParameter());
    const gp_Pnt& p2E = c2.Value(c2.LastParameter());

    bool state =  (p1S.IsEqual(p2S, Precision::Confusion()) && p1E.IsEqual(p2E, Precision::Confusion()));

    if( s ||
        (p1S.IsEqual(p2E, Precision::Confusion()) && p1E.IsEqual(p2S, Precision::Confusion())) ){
        switch(c1.GetType()) {
          case GeomAbs_Circle: {

                  gp_Circ circ1 = c1.Circle();
                  gp_Circ circ2 = c2.Circle();

                  const gp_Pnt& p = circ1.Location();
                  const gp_Pnt& p2 = circ2.Location();

                  double radius1 = circ1.Radius();
                  double radius2 = circ2.Radius();
                  double f1 = c1.FirstParameter();
                  double f2 = c2.FirstParameter();
                  double l1 = c1.LastParameter();
                  double l2 = c2.LastParameter();
                  c1.Curve().Curve()->
                  if( p.IsEqual(p2,Precision::Confusion()) &&
                  radius2 - radius1 < Precision::Confusion()) {
                      return true;
                  }
          } break;
          default: break;
        }
    }
#endif
    return false;
}

//only used by extractFaces
void GeometryObject::createWire(const TopoDS_Shape &input,
                                std::vector<TopoDS_Wire> &wiresOut) const
{
    //input is a compound of edges?  there is edgesToWire logic in Part?
    if (input.IsNull()) {
        Base::Console().Log("TechDraw::GeometryObject::createWire input is NULL\n");
        return; // There is no OpenCascade Geometry to be calculated
    }

    std::list<TopoDS_Edge> edgeList;

    // make a list of all the edges in the input shape
    TopExp_Explorer edges(input, TopAbs_EDGE);
    while (edges.More()) {
        edgeList.push_back(TopoDS::Edge(edges.Current()));
        edges.Next();
    }
    // Combine connected edges into wires.

    // BRepBuilderAPI_MakeWire has an annoying behaviour where the only [sane]
    // way to test whether an edge connects to a wire is to attempt adding
    // the edge.  But, if the last added edge was not connected to the wire,
    // BRepBuilderAPI_MakeWire::Wire() will throw an exception.  So, we need
    // to hang on to the last successfully added edge to "reset" scapegoat.
    //
    // ...and why do we need scapegoat?  Because the resetting adds a duplicate
    // edge (which can be problematic down the road), but it's not easy to
    // remove the edge from the BRepBuilderAPI_MakeWire.
    bool lastAddFailed;
    TopoDS_Edge lastGoodAdd;

    while (edgeList.size() > 0) {
        // add and erase first edge
        BRepBuilderAPI_MakeWire scapegoat, mkWire;
        scapegoat.Add(edgeList.front());
        mkWire.Add(edgeList.front());
        lastAddFailed = false;
        lastGoodAdd = edgeList.front();
        edgeList.pop_front();

        // try to connect remaining edges to the wire, the wire is complete if no more egdes are connectible
        bool found;
        do {
            found = false;
            for (std::list<TopoDS_Edge>::iterator pE = edgeList.begin(); pE != edgeList.end(); ++pE) {
                // Try adding edge - this doesn't necessarily add it
                scapegoat.Add(*pE);
                if (scapegoat.Error() != BRepBuilderAPI_DisconnectedWire) {
                    mkWire.Add(*pE);
                    // Edge added!  Remember it, so we can reset scapegoat
                    lastAddFailed = false;
                    lastGoodAdd = *pE;

                    // ...remove it from edgeList,
                    edgeList.erase(pE);

                    // ...and start searching for the next edge
                    found = true;
                    break;           //exit for loop
                } else {
                    lastAddFailed = true;
                }
            }
        } while (found);

        // See note above re: BRepBuilderAPI_MakeWire annoying behaviour
        if (lastAddFailed) {
            scapegoat.Add(lastGoodAdd);
        }

        if (scapegoat.Error() == BRepBuilderAPI_WireDone) {
            // BRepTools_WireExplorer finds 1st n connected edges, while
            // TopExp_Explorer finds all edges.  Since we built mkWire using
            // TopExp_Explorer, and want to run BRepTools_WireExplorer over
            // it, we need to reorder the wire.
            ShapeFix_Wire fix;
            fix.Load(mkWire.Wire());
            fix.FixReorder();
            fix.Perform();

            wiresOut.push_back(fix.Wire());
        } else if(scapegoat.Error() == BRepBuilderAPI_DisconnectedWire) {
            Standard_Failure::Raise("Fatal error occurred in GeometryObject::createWire()");
        }
    }
}

gp_Pnt GeometryObject::findCentroid(const TopoDS_Shape &shape,
                                    const Base::Vector3d &direction,
                                    const Base::Vector3d &xAxis) const
{
    gp_Ax2 viewAxis;
    viewAxis = gp_Ax2(gp_Pnt(0, 0, 0),
                      gp_Dir(direction.x, -direction.y, direction.z),
                      gp_Dir(xAxis.x, -xAxis.y, xAxis.z)); // Y invert warning!

    gp_Trsf tempTransform;
    tempTransform.SetTransformation(viewAxis);
    BRepBuilderAPI_Transform builder(shape, tempTransform);

    Bnd_Box tBounds;
    BRepBndLib::Add(builder.Shape(), tBounds);

    tBounds.SetGap(0.0);
    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    tBounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    Standard_Real x = (xMin + xMax) / 2.0,
                  y = (yMin + yMax) / 2.0,
                  z = (zMin + zMax) / 2.0;

    // Get centroid back into object space
    tempTransform.Inverted().Transforms(x, y, z);

    return gp_Pnt(x, y, z);
}

void GeometryObject::extractGeometry(const TopoDS_Shape &input,
                                     const Base::Vector3d &direction,
                                     bool extractHidden,
                                     const Base::Vector3d &xAxis)
{
    // Clear previous Geometry and References that may have been stored
    clear();

    ///TODO: Consider whether it would be possible/beneficial to cache some of this effort (eg don't do scale in OpenCASCADE land) IR
    TopoDS_Shape transShape;
    HLRBRep_Algo *brep_hlr = NULL;
    gp_Pnt inputCentre;
    try {
        inputCentre = findCentroid(input, direction, xAxis);
    }
    catch (...) {
        Base::Console().Log("GeometryObject::extractGeometry - findCentroid failed.\n");
        return;
    }
    try {
        // Make tempTransform scale the object around it's centre point and
        // mirror about the Y axis
        gp_Trsf tempTransform;
        tempTransform.SetScale(inputCentre, Scale);
        gp_Trsf mirrorTransform;
        mirrorTransform.SetMirror( gp_Ax2(inputCentre, gp_Dir(0, 1, 0)) );
        tempTransform.Multiply(mirrorTransform);

        // Apply that transform to the shape.  This should preserve the centre.
        BRepBuilderAPI_Transform mkTrf(input, tempTransform);
        transShape = mkTrf.Shape();

        brep_hlr = new HLRBRep_Algo();
        brep_hlr->Add(transShape);

        // Project the shape into view space with the object's centroid
        // at the origin.
        gp_Ax2 viewAxis;
        viewAxis = gp_Ax2(inputCentre,
                          gp_Dir(direction.x, direction.y, direction.z),
                          gp_Dir(xAxis.x, xAxis.y, xAxis.z));
        HLRAlgo_Projector projector( viewAxis );
        brep_hlr->Projector(projector);
        brep_hlr->Update();
        brep_hlr->Hide();

    }
    catch (...) {
        Standard_Failure::Raise("GeometryObject::extractGeometry - error occurred while projecting shape");
    }

    // extracting the result sets:

    //TODO: What is this? IR
    // need HLRBRep_HLRToShape aHLRToShape(shapes);
    // then TopoDS_Shape V = shapes.VCompound();   //V is a compound of edges
    // V  = shapes.VCompound       ();// hard edge visibly    - real edges in original shape
    // V1 = shapes.Rg1LineVCompound();// Smoth edges visibly  - "transition edges between two surfaces"
    // VN = shapes.RgNLineVCompound();// contour edges visibly  - "sewn edges"?
    // VO = shapes.OutLineVCompound();// contours apparents visibly  - ?edge in projection but not in original shape?
    // VI = shapes.IsoLineVCompound();// isoparamtriques   visibly   - ?constant u,v sort of like lat/long
    // H  = shapes.HCompound       ();// hard edge       invisibly
    // H1 = shapes.Rg1LineHCompound();// Smoth edges  invisibly
    // HN = shapes.RgNLineHCompound();// contour edges invisibly
    // HO = shapes.OutLineHCompound();// contours apparents invisibly
    // HI = shapes.IsoLineHCompound();// isoparamtriques   invisibly

    // Extract Hidden Edges
    if(extractHidden)
        extractEdges(brep_hlr, transShape, 5, false, WithHidden);// Hard Edge
//     calculateGeometry(extractCompound(brep_hlr, invertShape, 2, false), WithHidden); // Outline
//     calculateGeometry(extractCompound(brep_hlr, invertShape, 3, false), (ExtractionType)(WithSmooth | WithHidden)); // Smooth

    // Extract Visible Edges
    extractEdges(brep_hlr, transShape, 5, true, WithSmooth);  // Hard Edge

    //get endpoints of visible projected edges and add to vertexGeom with ref = -1
    //this could get slow for big models?
    const std::vector<BaseGeom *> &edgeGeom = getEdgeGeometry();
    std::vector<BaseGeom*>::const_iterator iEdge = edgeGeom.begin();
    for (; iEdge != edgeGeom.end(); iEdge++) {
        if ((*iEdge)->extractType == DrawingGeometry::WithHidden) {               //only use visible edges
            continue;
        }
        std::vector<Base::Vector2D> ends = (*iEdge)->findEndPoints();
        if (!ends.empty()) {
            if (!findVertex(ends[0])) {
                Vertex* v0 = new Vertex(ends[0]);
                v0->extractType = DrawingGeometry::Plain;
                vertexGeom.push_back(v0);
                vertexReferences.push_back(-1);
            }
            if (!findVertex(ends[1])) {
                Vertex* v1 = new Vertex(ends[1]);
                v1->extractType = DrawingGeometry::Plain;
                vertexGeom.push_back(v1);
                vertexReferences.push_back(-1);
            }
        }
    }

//     calculateGeometry(extractCompound(brep_hlr, invertShape, 2, true), Plain);  // Outline
//     calculateGeometry(extractCompound(brep_hlr, invertShape, 3, true), WithSmooth); // Smooth Edge

    // Extract Faces
    //algorithm,shape,visible/hidden,smooth edges(show flat/curve transition,facewires,index of face in shape?
    extractFaces(brep_hlr, transShape, true, WithSmooth, faceGeom, faceReferences);

    // House Keeping
    delete brep_hlr;
}

//translate all the edges in "input" into BaseGeoms
int GeometryObject::calculateGeometry(const TopoDS_Shape &input,
                                      const ExtractionType extractionType,
                                      std::vector<BaseGeom *> &geom) const
{
    if(input.IsNull()) {
        Base::Console().Log("TechDraw::GeometryObject::calculateGeometry input is NULL\n");
        return 0; // There is no OpenCascade Geometry to be calculated
    }

    // build a mesh to explore the shape
    //BRepMesh::Mesh(input, Tolerance);   //OCC has removed BRepMesh::Mesh() as of v6.8.0.oce-0.17-dev
    BRepMesh_IncrementalMesh(input, Tolerance);    //making a mesh turns edges into multilines?

    int geomsAdded = 0;

    // Explore all edges of input and calculate base geometry representation
    TopExp_Explorer edges(input, TopAbs_EDGE);
    for (int i = 1 ; edges.More(); edges.Next(),i++) {
        const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());
        BRepAdaptor_Curve adapt(edge);

        switch(adapt.GetType()) {
          case GeomAbs_Circle: {
            double f = adapt.FirstParameter();
            double l = adapt.LastParameter();
            gp_Pnt s = adapt.Value(f);
            gp_Pnt e = adapt.Value(l);

            if (fabs(l-f) > 1.0 && s.SquareDistance(e) < 0.001) {
                  Circle *circle = new Circle(adapt);
                  circle->extractType = extractionType;
                  geom.push_back(circle);
            } else {
                  AOC *aoc = new AOC(adapt);
                  aoc->extractType = extractionType;
                  geom.push_back(aoc);
            }
          } break;
          case GeomAbs_Ellipse: {
            double f = adapt.FirstParameter();
            double l = adapt.LastParameter();
            gp_Pnt s = adapt.Value(f);
            gp_Pnt e = adapt.Value(l);
            if (fabs(l-f) > 1.0 && s.SquareDistance(e) < 0.001) {
                  Ellipse *ellipse = new Ellipse(adapt);
                  ellipse->extractType = extractionType;
                  geom.push_back(ellipse);
            } else {
                  AOE *aoe = new AOE(adapt);
                  aoe->extractType = extractionType;
                  geom.push_back(aoe);
            }
          } break;
          case GeomAbs_BSplineCurve: {
            BSpline *bspline = 0;
            Generic* gen = NULL;
            try {
                bspline = new BSpline(adapt);
                bspline->extractType = extractionType;
                if (bspline->isLine()) {
                    Base::Vector2D start,end;
                    start = bspline->segments.front().pnts[0];
                    end = bspline->segments.back().pnts[1];
                    gen = new Generic(start,end);
                    gen->extractType = extractionType;
                    geom.push_back(gen);
                    delete bspline;
                } else {
                    geom.push_back(bspline);
                }
                break;
            }
            catch (Standard_Failure) {
                delete bspline;
                delete gen;
                bspline = 0;
                // Move onto generating a primitive
            }
          }
          default: {
            Generic *primitive = new Generic(adapt);
            primitive->extractType = extractionType;
            geom.push_back(primitive);
          }  break;
        }
        geomsAdded++;
    }
    return geomsAdded;
}

//! does this GeometryObject already have this vertex
bool GeometryObject::findVertex(Base::Vector2D v)
{
    bool found = false;
    std::vector<Vertex*>::iterator it = vertexGeom.begin();
    for (; it != vertexGeom.end(); it++) {
        double dist = (v - (*it)->pnt).Length();
        if (dist < Precision::Confusion()) {
            found = true;
            break;
        }
    }
    return found;
}

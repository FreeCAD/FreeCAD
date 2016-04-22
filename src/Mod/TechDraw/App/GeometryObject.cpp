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
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepTools_WireExplorer.hxx>
# include <ShapeFix_Wire.hxx>
# include <BRepProj_Projection.hxx>
#include <BRepLib.hxx>

# include <BRepAdaptor_HCurve.hxx>
# include <BRepAdaptor_CompCurve.hxx>

#include <BRepLProp_CurveTool.hxx>
#include <BRepLProp_CLProps.hxx>

// # include <Handle_BRepAdaptor_HCompCurve.hxx>
# include <Approx_Curve3d.hxx>

# include <BRepAdaptor_HCurve.hxx>
#include <Handle_HLRBRep_Algo.hxx>
#include <Handle_HLRBRep_Data.hxx>
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

#include "GeometryObject.h"

//#include <QDebug>

using namespace TechDrawGeometry;

struct EdgePoints {
    gp_Pnt v1, v2;
    TopoDS_Edge edge;
};

//debugging routine signatures
void _dumpEdgeData(char* label, int i, HLRBRep_EdgeData& ed);
const char* _printBool(bool b);
void _dumpEdge(char* label, int i, TopoDS_Edge e);


GeometryObject::GeometryObject() : Tolerance(0.05f), Scale(1.f)
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
    faceGeom.clear();
    edgeGeom.clear();
}

//!set up a hidden line remover and project a shape with it
void GeometryObject::projectShape(const TopoDS_Shape& input,
                             const gp_Pnt& inputCenter,
                             const Base::Vector3d& direction,
                             const Base::Vector3d& xAxis)
{
    // Clear previous Geometry
    clear();

    Handle_HLRBRep_Algo brep_hlr = NULL;
    try {
        brep_hlr = new HLRBRep_Algo();                //leak? when does this get freed? handle/smart pointer?
        brep_hlr->Add(input);

        // Project the shape into view space with the object's centroid
        // at the origin.
        gp_Ax2 viewAxis;
        viewAxis = gp_Ax2(inputCenter,
                          gp_Dir(direction.x, direction.y, direction.z),
                          gp_Dir(xAxis.x, xAxis.y, xAxis.z));
        HLRAlgo_Projector projector( viewAxis );
        brep_hlr->Projector(projector);
        brep_hlr->Update();
        brep_hlr->Hide();
    }
    catch (...) {
        Standard_Failure::Raise("GeometryObject::projectShape - error occurred while projecting shape");
    }
    try {
        HLRBRep_HLRToShape hlrToShape(brep_hlr);

        visHard    = hlrToShape.VCompound();
        visSmooth  = hlrToShape.Rg1LineVCompound();
        visSeam    = hlrToShape.RgNLineVCompound();
        visOutline = hlrToShape.OutLineVCompound();
        visIso     = hlrToShape.IsoLineVCompound();
        hidHard    = hlrToShape.HCompound();
        hidSmooth  = hlrToShape.Rg1LineHCompound();
        hidSeam    = hlrToShape.RgNLineHCompound();
        hidOutline = hlrToShape.OutLineHCompound();
        hidIso     = hlrToShape.IsoLineHCompound();

        BRepLib::BuildCurves3d(visHard);
        BRepLib::BuildCurves3d(visSmooth);
        BRepLib::BuildCurves3d(visSeam);
        BRepLib::BuildCurves3d(visOutline);
        BRepLib::BuildCurves3d(visIso);
        BRepLib::BuildCurves3d(hidHard);
        BRepLib::BuildCurves3d(hidSmooth);
        BRepLib::BuildCurves3d(hidSeam);
        BRepLib::BuildCurves3d(hidOutline);
        BRepLib::BuildCurves3d(hidIso);
    }
    catch (...) {
        Standard_Failure::Raise("GeometryObject::projectShape - error occurred while extracting edges");
    }

}

//!add edges meeting filter criteria for category, visibility
void GeometryObject::extractGeometry(edgeClass category, bool visible)
{
    TopoDS_Shape filtEdges;
    if (visible) {
        switch (category) {
            case ecHARD:
                filtEdges = visHard;
                break;
            case ecOUTLINE:
                filtEdges = visOutline;
                break;
            case ecSMOOTH:
                filtEdges = visSmooth;
                break;
            case ecSEAM:
                filtEdges = visSeam;
                break;
            default:
                Base::Console().Warning("GeometryObject::ExtractGeometry - unsupported visible edgeClass: %d\n",category);
                return;
        }
    } else {
        switch (category) {
            case ecHARD:
                filtEdges = hidHard;
                break;
            //more cases here?
            default:
                Base::Console().Warning("GeometryObject::ExtractGeometry - unsupported hidden edgeClass: %d\n",category);
                return;
        }
    }

    addGeomFromCompound(filtEdges, category, visible);
}

//! update edgeGeom and vertexGeom from Compound of edges
void GeometryObject::addGeomFromCompound(TopoDS_Shape edgeCompound, edgeClass category, bool visible)
{
    if(edgeCompound.IsNull()) {
        Base::Console().Log("TechDraw::GeometryObject::addGeomFromCompound edgeCompound is NULL\n");
        return; // There is no OpenCascade Geometry to be calculated
    }

    // build a mesh to explore the shape
    BRepMesh_IncrementalMesh(edgeCompound, Tolerance);    //no idea why we need to mesh shape

    // Explore all edges of edgeCompound and calculate base geometry representation
    BaseGeom* base;
    TopExp_Explorer edges(edgeCompound, TopAbs_EDGE);
    for (int i = 1 ; edges.More(); edges.Next(),i++) {
        const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());
        if (edge.IsNull()) {
            Base::Console().Log("INFO - GO::addGeomFromCompound - edge: %d is NULL\n",i);
            continue;
        }
        base = BaseGeom::baseFactory(edge);
        base->classOfEdge = category;
        base->visible = visible;
        edgeGeom.push_back(base);

        //add vertices of new edge if not already in list
        if (visible) {
            BaseGeom* lastAdded = edgeGeom.back();
            //if (edgeGeom.empty()) {horrible_death();} //back() undefined behavior (can't happen? baseFactory always returns a Base?)
            bool v1Add = true, v2Add = true;
            TechDrawGeometry::Vertex* v1 = new TechDrawGeometry::Vertex(lastAdded->getStartPoint());
            TechDrawGeometry::Vertex* v2 = new TechDrawGeometry::Vertex(lastAdded->getEndPoint());
            std::vector<Vertex *>::iterator itVertex = vertexGeom.begin();
            for (; itVertex != vertexGeom.end(); itVertex++) {
                if ((*itVertex)->isEqual(v1,Tolerance)) {
                    v1Add = false;
                }
                if ((*itVertex)->isEqual(v2,Tolerance)) {
                    v2Add = false;
                }
            }
            if (v1Add) {
                vertexGeom.push_back(v1);
                v1->visible = true;
            } else {
                delete v1;
            }
            if (v2Add) {
                vertexGeom.push_back(v2);
                v2->visible = true;
            } else {
                delete v2;
            }
        }

    }
}

//! empty Face geometry
void GeometryObject::clearFaceGeom()
{
    faceGeom.clear();
}

//! add a Face to Face Geometry
void GeometryObject::addFaceGeom(Face* f)
{
    faceGeom.push_back(f);
}

/////////////// bbox routines

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

/// utility non-class member functions
//! Returns the centroid of shape, as viewed according to direction and xAxis
gp_Pnt TechDrawGeometry::findCentroid(const TopoDS_Shape &shape,
                                    const Base::Vector3d &direction,
                                    const Base::Vector3d &xAxis)
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

//!scales & mirrors a shape about a center
TopoDS_Shape TechDrawGeometry::mirrorShape(const TopoDS_Shape &input,
                             const gp_Pnt& inputCenter,
                             double scale)
{
    TopoDS_Shape transShape;
    try {
        // Make tempTransform scale the object around it's centre point and
        // mirror about the Y axis
        // TODO: is this really always Y axis? sb whatever is vertical direction in projection?
        gp_Trsf tempTransform;
        tempTransform.SetScale(inputCenter, scale);
        gp_Trsf mirrorTransform;
        mirrorTransform.SetMirror( gp_Ax2(inputCenter, gp_Dir(0, 1, 0)) );
        tempTransform.Multiply(mirrorTransform);

        // Apply that transform to the shape.  This should preserve the centre.
        BRepBuilderAPI_Transform mkTrf(input, tempTransform);
        transShape = mkTrf.Shape();
    }
    catch (...) {
        Base::Console().Log("GeometryObject::mirrorShape - mirror/scale failed.\n");
        return transShape;
    }
    return transShape;
}

/// debug functions
void _dumpEdgeData(char* label, int i, HLRBRep_EdgeData& ed)
{
    Base::Console().Message("Dump of EdgeData for %s Edge: %d\n",label,i);
    Base::Console().Message("Selected:%s Smooth(Rg1):%s Sewn(RgN):%s OutSta:%s OutEnd:%s\n",
                            _printBool(ed.Selected()),_printBool(ed.Rg1Line()),_printBool(ed.RgNLine()),_printBool(ed.OutLVSta()),_printBool(ed.OutLVEnd()));
    Base::Console().Message("Vertical:%s Simple:%s Used:%s HideCount:%d\n",
                            _printBool(ed.Vertical()),_printBool(ed.Simple()),_printBool(ed.Used()),ed.HideCount());
}

/* TODO: Clean this up when faces are actually working properly...

void debugEdge(const TopoDS_Edge &e)

{

    gp_Pnt p0 = BRep_Tool::Pnt(TopExp::FirstVertex(e));

    gp_Pnt p1 = BRep_Tool::Pnt(TopExp::LastVertex(e));

    qDebug()<<p0.X()<<','<<p0.Y()<<','<<p0.Z()<<"\t - \t"<<p1.X()<<','<<p1.Y()<<','<<p1.Z();

}*/


void _dumpEdge(char* label, int i, TopoDS_Edge e)
{
    BRepAdaptor_Curve adapt(e);
    double start = BRepLProp_CurveTool::FirstParameter(adapt);
    double end = BRepLProp_CurveTool::LastParameter(adapt);
    BRepLProp_CLProps propStart(adapt,start,0,Precision::Confusion());
    const gp_Pnt& vStart = propStart.Value();
    BRepLProp_CLProps propEnd(adapt,end,0,Precision::Confusion());
    const gp_Pnt& vEnd = propEnd.Value();
    Base::Console().Message("%s edge:%d start:(%.3f,%.3f,%.3f) end:(%.2f,%.3f,%.3f)\n",label,i,
                            vStart.X(),vStart.Y(),vStart.Z(),vEnd.X(),vEnd.Y(),vEnd.Z());
}

const char* _printBool(bool b)
{
    return (b ? "True" : "False");
}

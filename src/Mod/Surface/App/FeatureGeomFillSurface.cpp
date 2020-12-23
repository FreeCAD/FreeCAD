/***************************************************************************
 *   Copyright (c) 2014-2015 Nathan Miller    <Nathan.A.Mill[at]gmail.com> *
 *                           Balázs Bámer                                  *
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
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_NurbsConvert.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <GeomFill.hxx>
#include <GeomFill_BezierCurves.hxx>
#include <GeomFill_BSplineCurves.hxx>
#include <gp_Trsf.hxx>
#include <ShapeConstruct_Curve.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeExtend_WireData.hxx>
#include <Standard_ConstructionError.hxx>
#include <StdFail_NotDone.hxx>
#include <TopExp_Explorer.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Tools.h>

#include "FeatureGeomFillSurface.h"

using namespace Surface;

ShapeValidator::ShapeValidator()
{
   initValidator();
}

void ShapeValidator::initValidator(void)
{
    willBezier = true;
    edgeCount = 0;
}

// shows error message if the shape is not an edge
void ShapeValidator::checkEdge(const TopoDS_Shape& shape)
{
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) {
        Standard_Failure::Raise("Shape is not an edge.\n");
    }

    TopoDS_Edge etmp = TopoDS::Edge(shape);   //Curve TopoDS_Edge
    TopLoc_Location heloc; // this will be output
    Standard_Real u0;// contains output
    Standard_Real u1;// contains output
    Handle(Geom_Curve) c_geom = BRep_Tool::Curve(etmp,heloc,u0,u1); //The geometric curve
    Handle(Geom_BezierCurve) bez_geom = Handle(Geom_BezierCurve)::DownCast(c_geom); //Try to get Bezier curve

    // if not a Bezier then try to create a B-spline surface from the edges
    if (bez_geom.IsNull()) {
        willBezier = false;
    }

    edgeCount++;
}

void ShapeValidator::checkAndAdd(const TopoDS_Shape &shape, Handle(ShapeExtend_WireData) *aWD)
{
    checkEdge(shape);
    if (aWD != NULL) {
        BRepBuilderAPI_Copy copier(shape);
        // make a copy of the shape and the underlying geometry to avoid to affect the input shapes
        (*aWD)->Add(TopoDS::Edge(copier.Shape()));
    }
}

void ShapeValidator::checkAndAdd(const Part::TopoShape &ts, const char *subName, Handle(ShapeExtend_WireData) *aWD)
{
    try {
        if (subName != NULL && *subName != '\0') {
            //we want only the subshape which is linked
            checkAndAdd(ts.getSubShape(subName), aWD);
        }
        else if (!ts.getShape().IsNull() && ts.getShape().ShapeType() == TopAbs_WIRE) {
            TopoDS_Wire wire = TopoDS::Wire(ts.getShape());
            for (TopExp_Explorer xp(wire, TopAbs_EDGE); xp.More(); xp.Next()) {
                checkAndAdd(xp.Current(), aWD);
            }
        }
        else {
            checkAndAdd(ts.getShape(), aWD);
        }
    }
    catch (Standard_Failure&) { // any OCC exception means an unappropriate shape in the selection
        Standard_Failure::Raise("Wrong shape type.\n");
    }
}


PROPERTY_SOURCE(Surface::GeomFillSurface, Part::Spline)

const char* GeomFillSurface::FillTypeEnums[]    = {"Stretched", "Coons", "Curved", NULL};

GeomFillSurface::GeomFillSurface(): Spline()
{
    ADD_PROPERTY(FillType, ((long)0));
    ADD_PROPERTY(BoundaryList, (0, "Dummy"));
    ADD_PROPERTY(ReversedList, (false));
    FillType.setEnums(FillTypeEnums);
    BoundaryList.setScope(App::LinkScope::Global);
}


//Check if any components of the surface have been modified
short GeomFillSurface::mustExecute() const
{
    if (BoundaryList.isTouched() ||
        ReversedList.isTouched() ||
        FillType.isTouched()) {
        return 1;
    }
    return Spline::mustExecute();
}

void GeomFillSurface::onChanged(const App::Property* prop)
{
    if (isRestoring()) {
        if (prop == &BoundaryList) {
            // auto-adjusting size of this list
            if (BoundaryList.getSize() != ReversedList.getSize()) {
                ReversedList.setSize(BoundaryList.getSize());
            }
        }
    }
    Part::Spline::onChanged(prop);
}

App::DocumentObjectExecReturn *GeomFillSurface::execute(void)
{
    try {
        TopoDS_Wire aWire;

        //Gets the healed wire
        if (getWire(aWire)) {
            createBezierSurface(aWire);
        }
        else {
            createBSplineSurface(aWire);
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_ConstructionError&) {
        // message is in a Latin language, show a normal one
        return new App::DocumentObjectExecReturn("Curves are disjoint.");
    }
    catch (StdFail_NotDone&) {
        return new App::DocumentObjectExecReturn("A curve was not a B-spline and could not be converted into one.");
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

GeomFill_FillingStyle GeomFillSurface::getFillingStyle()
{
    //Identify filling style
    switch (FillType.getValue()) {
    case GeomFill_StretchStyle:
    case GeomFill_CoonsStyle:
    case GeomFill_CurvedStyle:
        return static_cast<GeomFill_FillingStyle>(FillType.getValue());
    default:
        Standard_Failure::Raise("Filling style must be 0 (Stretch), 1 (Coons), or 2 (Curved).\n");
        throw; // this is to shut up the compiler
    }
}

bool GeomFillSurface::getWire(TopoDS_Wire& aWire)
{
    Handle(ShapeFix_Wire) aShFW = new ShapeFix_Wire;
    Handle(ShapeExtend_WireData) aWD = new ShapeExtend_WireData;

    std::vector<App::PropertyLinkSubList::SubSet> boundary = BoundaryList.getSubListValues();
    if (boundary.size() > 4) {// if too many not even try
        Standard_Failure::Raise("Only 2-4 curves are allowed\n");
    }

    ShapeValidator validator;
    for(std::size_t i = 0; i < boundary.size(); i++) {
        App::PropertyLinkSubList::SubSet set = boundary[i];

        if (set.first->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            for (auto jt: set.second) {
                const Part::TopoShape &ts = static_cast<Part::Feature*>(set.first)->Shape.getShape();
                validator.checkAndAdd(ts, jt.c_str(), &aWD);
            }
        }
        else {
            Standard_Failure::Raise("Curve not from Part::Feature\n");
        }
    }

    if (validator.numEdges() < 2 || validator.numEdges() > 4) {
        Standard_Failure::Raise("Only 2-4 curves are allowed\n");
    }

    //Reorder the curves and fix the wire if required

    aShFW->Load(aWD); //Load in the wire
    aShFW->FixReorder(); //Fix the order of the edges if required
    aShFW->ClosedWireMode() = Standard_True; //Enables closed wire mode
    aShFW->FixConnected(); //Fix connection between wires
    aShFW->FixSelfIntersection(); //Fix Self Intersection
    aShFW->Perform(); //Perform the fixes

    aWire = aShFW->Wire(); //Healed Wire

    if (aWire.IsNull()) {
        Standard_Failure::Raise("Wire unable to be constructed\n");
    }

    return validator.isBezier();
}

void GeomFillSurface::createFace(const Handle(Geom_BoundedSurface) &aSurface)
{
    BRepBuilderAPI_MakeFace aFaceBuilder;
    Standard_Real u1, u2, v1, v2;
    // transfer surface bounds to face
    aSurface->Bounds(u1, u2, v1, v2);
    aFaceBuilder.Init(aSurface, u1, u2, v1, v2, Precision::Confusion());

    TopoDS_Face aFace = aFaceBuilder.Face();

    if (!aFaceBuilder.IsDone()) {
        Standard_Failure::Raise("Face unable to be constructed\n");
    }
    if (aFace.IsNull()) {
        Standard_Failure::Raise("Resulting Face is null\n");
    }
    this->Shape.setValue(aFace);
}

void GeomFillSurface::createBezierSurface(TopoDS_Wire& aWire)
{
    std::vector<Handle(Geom_BezierCurve)> curves;
    curves.reserve(4);

    Standard_Real u1, u2; // contains output
    TopExp_Explorer anExp (aWire, TopAbs_EDGE);
    for (; anExp.More(); anExp.Next()) {
        const TopoDS_Edge hedge = TopoDS::Edge (anExp.Current());
        TopLoc_Location heloc; // this will be output
        Handle(Geom_Curve) c_geom = BRep_Tool::Curve(hedge, heloc, u1, u2); //The geometric curve
        Handle(Geom_BezierCurve) bezier = Handle(Geom_BezierCurve)::DownCast(c_geom); //Try to get Bezier curve

        if (!bezier.IsNull()) {
            gp_Trsf transf = heloc.Transformation();
            bezier->Transform(transf); // apply original transformation to control points
            //Store Underlying Geometry
            curves.push_back(bezier);
        }
        else {
            Standard_Failure::Raise("Curve not a Bezier Curve");
        }
    }

    GeomFill_FillingStyle fstyle = getFillingStyle();
    GeomFill_BezierCurves aSurfBuilder; //Create Surface Builder

    std::size_t edgeCount = curves.size();
    const boost::dynamic_bitset<>& booleans = ReversedList.getValues();
    if (edgeCount == booleans.size()) {
        for (std::size_t i=0; i<edgeCount; i++) {
            if (booleans[i])
                curves[i]->Reverse();
        }
    }

    if (edgeCount == 2) {
        aSurfBuilder.Init(curves[0], curves[1], fstyle);
    }
    else if (edgeCount == 3) {
        aSurfBuilder.Init(curves[0], curves[1], curves[2], fstyle);
    }
    else if (edgeCount == 4) {
        aSurfBuilder.Init(curves[0], curves[1], curves[2], curves[3], fstyle);
    }

    createFace(aSurfBuilder.Surface());
}

void GeomFillSurface::createBSplineSurface(TopoDS_Wire& aWire)
{
    std::vector<Handle(Geom_BSplineCurve)> curves;
    curves.reserve(4);
    Standard_Real u1, u2; // contains output
    TopExp_Explorer anExp (aWire, TopAbs_EDGE);
    for (; anExp.More(); anExp.Next()) {
        const TopoDS_Edge& edge = TopoDS::Edge (anExp.Current());
        TopLoc_Location heloc; // this will be output
        Handle(Geom_Curve) c_geom = BRep_Tool::Curve(edge, heloc, u1, u2); //The geometric curve
        Handle(Geom_BSplineCurve) bspline = Handle(Geom_BSplineCurve)::DownCast(c_geom); //Try to get BSpline curve

        if (!bspline.IsNull()) {
            gp_Trsf transf = heloc.Transformation();
            bspline->Transform(transf); // apply original transformation to control points
            //Store Underlying Geometry
            curves.push_back(bspline);
        }
        else {
            // try to convert it into a B-spline
            BRepBuilderAPI_NurbsConvert mkNurbs(edge);
            TopoDS_Edge nurbs = TopoDS::Edge(mkNurbs.Shape());
            // avoid copying
            TopLoc_Location heloc2; // this will be output
            Handle(Geom_Curve) c_geom2 = BRep_Tool::Curve(nurbs, heloc2, u1, u2); //The geometric curve
            Handle(Geom_BSplineCurve) bspline2 = Handle(Geom_BSplineCurve)::DownCast(c_geom2); //Try to get BSpline curve

            if (!bspline2.IsNull()) {
                gp_Trsf transf = heloc2.Transformation();
                bspline2->Transform(transf); // apply original transformation to control points
                //Store Underlying Geometry
                curves.push_back(bspline2);
            }
            else {
                // BRepBuilderAPI_NurbsConvert failed, try ShapeConstruct_Curve now
                ShapeConstruct_Curve scc;
                Handle(Geom_BSplineCurve) spline = scc.ConvertToBSpline(c_geom, u1, u2, Precision::Confusion());
                if (spline.IsNull())
                    Standard_Failure::Raise("A curve was not a B-spline and could not be converted into one.");
                gp_Trsf transf = heloc2.Transformation();
                spline->Transform(transf); // apply original transformation to control points
                curves.push_back(spline);
            }
        }
    }

    GeomFill_FillingStyle fstyle = getFillingStyle();
    GeomFill_BSplineCurves aSurfBuilder; //Create Surface Builder

    std::size_t edgeCount = curves.size();
    const boost::dynamic_bitset<>& booleans = ReversedList.getValues();
    if (edgeCount == booleans.size()) {
        for (std::size_t i=0; i<edgeCount; i++) {
            if (booleans[i])
                curves[i]->Reverse();
        }
    }
    if (edgeCount == 2) {
        aSurfBuilder.Init(curves[0], curves[1], fstyle);
    }
    else if (edgeCount == 3) {
        aSurfBuilder.Init(curves[0], curves[1], curves[2], fstyle);
    }
    else if (edgeCount == 4) {
        aSurfBuilder.Init(curves[0], curves[1], curves[2], curves[3], fstyle);
    }

    createFace(aSurfBuilder.Surface());
}

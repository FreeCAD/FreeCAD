/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/
#include "PreCompiled.h"

#ifndef _PreComp_
# include <cmath>
# include <cstdlib>
# include <sstream>
# include <QString>
# include <BRepLib.hxx>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepBuilderAPI_FindPlane.hxx>
# include <BRepLib_FindSurface.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepTools.hxx>
# include <BRepTools_WireExplorer.hxx>
# include <TopTools_MapOfShape.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <GeomAbs_JoinType.hxx>
# include <Geom_Circle.hxx>
# include <Geom_Ellipse.hxx>
# include <Geom_Line.hxx>
# include <Geom_Parabola.hxx>
# include <Geom_Plane.hxx>
# include <Standard_Failure.hxx>
# include <gp_Circ.hxx>
# include <gp_GTrsf.hxx>
# include <Standard_Version.hxx>
#endif
# include <GCPnts_UniformDeflection.hxx>

#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Base/Console.h>

#include "TopoShape.h"
#include "FaceMakerBullseye.h"
#include "LibArea.h"

using namespace Part;

Libarea::Libarea(const gp_Pln& plane)
:myPlane(plane)
{
    myMat.SetTransformation(plane.Position());
}

void Libarea::Add(const TopoDS_Shape &shape, double deflection) {
    if (shape.ShapeType() == TopAbs_WIRE){
        Add(TopoDS::Wire(shape),deflection);
        return;
    }
    bool haveShape = false;
    for (TopExp_Explorer it(shape, TopAbs_FACE); it.More(); it.Next()) {
        haveShape = true;
        Add(TopoDS::Face(it.Current()),deflection);
    }
    if(haveShape) return;
    for (TopExp_Explorer it(shape, TopAbs_WIRE); it.More(); it.Next()) {
        haveShape = true;
        Add(TopoDS::Wire(it.Current()),deflection);
    }
    if(haveShape) return;
    for (TopExp_Explorer it(shape, TopAbs_EDGE); it.More(); it.Next())
        Add(BRepBuilderAPI_MakeWire(
            TopoDS::Edge(it.Current())).Wire(),deflection);
}

void Libarea::Add(const TopoDS_Face &face, double deflection) {
    TopExp_Explorer it;
    Libarea area(myPlane);
    for (it.Init(face, TopAbs_WIRE); it.More(); it.Next())
        area.Add(TopoDS::Wire(it.Current()),deflection);
    bool fit_arcs = CArea::m_fit_arcs;
    CArea::m_fit_arcs = false;
    myArea.Union(area.myArea);
    CArea::m_fit_arcs = fit_arcs;
}

void Libarea::Add(const TopoDS_Wire& wire, double deflection) {
    CCurve ccurve;
    BRepTools_WireExplorer xp(TopoDS::Wire(
                wire.Moved(TopLoc_Location(myMat))));

    gp_Pnt p = BRep_Tool::Pnt(xp.CurrentVertex());
    ccurve.append(CVertex(Point(p.X(),p.Y())));

    for (;xp.More();xp.Next()) {
        BRepAdaptor_Curve curve(xp.Current());
        bool reversed = (xp.Current().Orientation()==TopAbs_REVERSED);

        p = curve.Value(reversed?curve.FirstParameter():curve.LastParameter());

        switch (curve.GetType()) {
        case GeomAbs_Line: {
            ccurve.append(CVertex(Point(p.X(),p.Y())));
            break;
        } case GeomAbs_Circle:{
            double first = curve.FirstParameter();
            double last = curve.LastParameter();
            gp_Circ circle = curve.Circle();
            gp_Ax1 axis = circle.Axis();
            int dir = axis.Direction().Z()<0?-1:1;
            if(reversed) dir = -dir;
            gp_Pnt loc = axis.Location();
            if(fabs(first-last)>M_PI) {
                // Split arc(circle) larger than half circle. This is
                // translated from PathUtil code. Not sure why it is
                // needed. 
                gp_Pnt mid = curve.Value((last-first)*0.5+first);
                ccurve.append(CVertex(dir,Point(mid.X(),mid.Y()),
                            Point(loc.X(),loc.Y())));
            }
            ccurve.append(CVertex(dir,Point(p.X(),p.Y()),
                        Point(loc.X(),loc.Y())));
            break;
        } default: {
            // Discretize all other type of curves
            GCPnts_UniformDeflection discretizer(curve, deflection, 
                    curve.FirstParameter(), curve.LastParameter());
            if (discretizer.IsDone () && discretizer.NbPoints () > 0) {
                int nbPoints = discretizer.NbPoints ();
                for (int i=1; i<=nbPoints; i++) {
                    gp_Pnt pt = discretizer.Value (i);
                    ccurve.append(CVertex(Point(pt.X(),pt.Y())));
                }
            }else
                Standard_Failure::Raise("Curve discretization failed");
        }}
    }
    if(BRep_Tool::IsClosed(wire) && !ccurve.IsClosed()) {
        cout << "warning: ccurve not closed" << endl;
        ccurve.append(ccurve.m_vertices.front());
    }
    myArea.append(ccurve);
}

TopoDS_Shape Libarea::Offset(double offset, 
    short algo, GeomAbs_JoinType join, bool allowOpenResult, bool fill)
{
    bool fit_arcs = !(algo&1);
    algo >>= 1;

    if(fabs(offset)<Precision::Confusion()) {
        if(!fit_arcs)
            return this->ToShape(myArea.m_curves,fill);
        CArea area(myArea);
        area.FitArcs();
        return this->ToShape(area.m_curves,fill);
    }

    // Curves are copied so that the underlying shape is intact, and new
    // shapes can be added later. New offset can be done by repeatedly
    // calling this function.
    CArea area;
    CArea areaOpen;
    if(algo == 0) {
        for(const CCurve &c : myArea.m_curves) {
            if(c.IsClosed())
                area.append(c);
            else
                areaOpen.append(c);
        }
    }else
        area = myArea;

    ClipperLib::JoinType joinType;
    ClipperLib::EndType endType;
    endType=ClipperLib::etOpenSquare;
    if(join == GeomAbs_Arc){
        joinType = ClipperLib::jtRound;
        endType = ClipperLib::etOpenRound;
    }else if(join == GeomAbs_Intersection)
        joinType = ClipperLib::jtMiter;
    else
        joinType = ClipperLib::jtSquare;

    if(!allowOpenResult)
        endType = ClipperLib::etClosedLine;

    bool fit_arcs_save = CArea::m_fit_arcs;
    CArea::m_fit_arcs = fit_arcs;
    if(algo==0){
        // libarea somehow fails offset without Reorder, but ClipperOffset
        // works okay. Don't know why
        area.Reorder();
        area.Offset(-offset);

        if(areaOpen.m_curves.size()) {
            // CCurve::Offset doesn't seem to work
            // 
            // bool failed = false;
            // for(CCurve &c : areaOpen.m_curves) {
            //     if(!c.Offset(-offset))
            //         failed = true;
            // }
            // if(failed)
            //     cout << "warning: offset failed for some wire" << endl;
            areaOpen.OffsetWithClipper(offset,joinType,endType);
            area.Union(areaOpen);
        }
    }else if(algo==1)
        area.OffsetWithClipper(offset,joinType,endType);
    else{
        CArea::m_fit_arcs = fit_arcs_save;
        throw Base::ValueError("makeOffset2D: unknown algo");
    }
    CArea::m_fit_arcs = fit_arcs_save;
    return this->ToShape(area.m_curves,fill);
}

static inline bool IsLeft(const gp_Pnt &a, const gp_Pnt &b, const gp_Pnt &c) {
    return ((b.X() - a.X())*(c.Y() - a.Y()) - (b.Y() - a.Y())*(c.X() - a.X())) > 0;
}

TopoDS_Shape Libarea::ToShape(const std::list<CCurve> &curves,bool fill) {
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    TopLoc_Location loc(myMat.Inverted());

    for(const CCurve &c : curves) {
        BRepBuilderAPI_MakeWire mkWire;
        gp_Pnt pstart,pt;
        bool first = true;
        for(const CVertex &v : c.m_vertices){
            if(first){
                first = false;
                pstart = pt = gp_Pnt(v.m_p.x,v.m_p.y,0);
                continue;
            }
            gp_Pnt pnext(v.m_p.x,v.m_p.y,0);
            if(v.m_type == 0) {
                mkWire.Add(BRepBuilderAPI_MakeEdge(pt,pnext).Edge());
            } else {
                gp_Pnt center(v.m_c.x,v.m_c.y,0);
                double r = center.Distance(pt);
                double r2 = center.Distance(pnext);
                if(fabs(r-r2) > Precision::Confusion()) {
                    double d = pt.Distance(pnext);
                    double q = sqrt(r*r - d*d*0.25);
                    double x = (pt.X()+pnext.X())*0.5;
                    double y = (pt.Y()+pnext.Y())*0.5;
                    double dx = q*(pt.Y()-pnext.Y())/d;
                    double dy = q*(pnext.X()-pt.X())/d;
                    gp_Pnt newCenter(x + dx, y + dy,0);
                    if(IsLeft(pt,pnext,center) != IsLeft(pt,pnext,newCenter)) {
                        newCenter.SetX(x - dx);
                        newCenter.SetY(y - dy);
                    }
                    Base::Console().Warning(
                            "Arc correction: %lf,%lf, center(%lf,%lf)->(%lf,%lf)\n", 
                            r,r2,center.X(),center.Y(),newCenter.X(),newCenter.Y());
                    center = newCenter;
                }
                gp_Ax2 axis(center, gp_Dir(0,0,v.m_type));
                mkWire.Add(BRepBuilderAPI_MakeEdge(gp_Circ(axis,r),pt,pnext).Edge());
            }
            pt = pnext;
        }
        if(c.IsClosed()){
            if(!BRep_Tool::IsClosed(mkWire.Wire())){
                // This should never happen after changing libarea's
                // Point::tolerance to be the same as Precision::Confusion().  
                // Just leave it here in case.
                BRepAdaptor_Curve curve(mkWire.Edge());
                gp_Pnt p1(curve.Value(curve.FirstParameter()));
                gp_Pnt p2(curve.Value(curve.LastParameter()));
                std::stringstream str;
                str<< "warning: patch open wire" << 
                    c.m_vertices.back().m_type << endl << 
                    '(' << p1.X() << ',' << p1.Y() << ')' << endl << 
                    '(' << p2.X() << ',' << p2.Y() << ')' << endl << 
                    '(' << pt.X() << ',' << pt.Y() << ')' << endl <<
                    '(' << pstart.X() << ',' <<pstart.Y() <<')' <<endl;
                Base::Console().Warning(str.str().c_str());
                mkWire.Add(BRepBuilderAPI_MakeEdge(pt,pstart).Edge());
            }
        }

        builder.Add(compound,mkWire.Wire().Moved(loc));
    }
    
    if(fill) {
        try{
            FaceMakerBullseye mkFace;
            mkFace.setPlane(myPlane);
            TopExp_Explorer it;
            for (it.Init(compound, TopAbs_WIRE); it.More(); it.Next())
                mkFace.addWire(TopoDS::Wire(it.Current()));
            mkFace.Build();
            if (mkFace.Shape().IsNull())
                Base::Console().Warning(
                        "FaceMakerBullseye returns null shape");
            else
                return mkFace.Shape();
        }catch (Base::Exception &e){
            Base::Console().Warning("FaceMakerBullseye failed: %s\n", e.what());
        }
    }
    return compound;
}

TopoDS_Shape Libarea::makeOffset(const TopoDS_Shape &shape, 
                                double offset, 
                                short joinType, 
                                bool fill, 
                                bool allowOpenResult, 
                                int algo)
{
    BRepLib_FindSurface planefinder(shape, -1, Standard_True);
    if (!planefinder.Found())
        throw Base::Exception(
                "makeOffset2D: wires are nonplanar or noncoplanar");
    Libarea area(GeomAdaptor_Surface(planefinder.Surface()).Plane());
    area.Add(shape);
    return area.Offset(
            offset,algo,GeomAbs_JoinType(joinType),allowOpenResult,fill);
}

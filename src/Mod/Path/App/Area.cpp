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
#endif

#include <BRepLib.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_FindPlane.hxx>
#include <BRepLib_FindSurface.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <GeomAbs_JoinType.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <Standard_Failure.hxx>
#include <gp_Circ.hxx>
#include <gp_GTrsf.hxx>
#include <Standard_Version.hxx>
#include <GCPnts_UniformDeflection.hxx>

#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Base/Console.h>

#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/FaceMakerBullseye.h>
#include "Area.h"
#include "../libarea/Area.h"

using namespace Path;

CAreaParams::CAreaParams()
    :PARAM_INIT(NAME,AREA_PARAMS_CAREA)
{}

AreaParams::AreaParams()
    :PARAM_INIT(NAME,AREA_PARAMS_BASE)
{}

CAreaConfig::CAreaConfig(const CAreaParams &p, bool noFitArcs)
    :params(p)
{
    // Arc fitting is lossy. we shall reduce the number of unecessary fit
    if(noFitArcs)
        params.FitArcs=false;

#define AREA_CONF_SAVE_AND_APPLY(_param) \
    PARAM_FNAME(_param) = BOOST_PP_CAT(CArea::get_,PARAM_FARG(_param))();\
    BOOST_PP_CAT(CArea::set_,PARAM_FARG(_param))(params.PARAM_FNAME(_param));

    PARAM_FOREACH(AREA_CONF_SAVE_AND_APPLY,AREA_PARAMS_CAREA)
}

CAreaConfig::~CAreaConfig() {

#define AREA_CONF_RESTORE(_param) \
    BOOST_PP_CAT(CArea::set_,PARAM_FARG(_param))(PARAM_FNAME(_param));

    PARAM_FOREACH(AREA_CONF_RESTORE,AREA_PARAMS_CAREA)
}

//////////////////////////////////////////////////////////////////////////////

TYPESYSTEM_SOURCE(Path::Area, Base::BaseClass);

Area::Area(const AreaParams *params)
:myArea(NULL)
,myAreaOpen(NULL)
,myHaveFace(false)
{
    if(params)
        setParams(*params);
}

Area::~Area() {
    clean();
}

void Area::setPlane(const TopoDS_Shape &shape) {
    myWorkPlane = shape;
}

void Area::add(CArea &area, const TopoDS_Shape &shape, const gp_Trsf *trsf, 
                double deflection, CArea *areaOpen, bool to_edges, bool reorder) 
{
    bool haveShape = false;

    for (TopExp_Explorer it(shape, TopAbs_FACE); it.More(); it.Next()) {
        haveShape = true;
        const TopoDS_Face &face = TopoDS::Face(it.Current());
        for (TopExp_Explorer it(face, TopAbs_WIRE); it.More(); it.Next())
            add(area,TopoDS::Wire(it.Current()),trsf,deflection);
    }

    if(haveShape) return;

    CArea _area;
    CArea _areaOpen;

    for (TopExp_Explorer it(shape, TopAbs_WIRE); it.More(); it.Next()) {
        haveShape = true;
        const TopoDS_Wire &wire = TopoDS::Wire(it.Current());
        if(BRep_Tool::IsClosed(wire))
            add(_area,wire,trsf,deflection);
        else if(to_edges) {
            for (TopExp_Explorer it(wire, TopAbs_EDGE); it.More(); it.Next())
                add(_areaOpen,BRepBuilderAPI_MakeWire(
                    TopoDS::Edge(it.Current())).Wire(),trsf,deflection);
        }else
            add(_areaOpen,wire,trsf,deflection);
    }

    if(!haveShape) {
        for (TopExp_Explorer it(shape, TopAbs_EDGE); it.More(); it.Next()) {
            add(_areaOpen,BRepBuilderAPI_MakeWire(
                TopoDS::Edge(it.Current())).Wire(),trsf,deflection);
        }
    }

    if(reorder)
        _area.Reorder();
    area.m_curves.splice(area.m_curves.end(),_area.m_curves);
    if(areaOpen)
        areaOpen->m_curves.splice(areaOpen->m_curves.end(),_areaOpen.m_curves);
    else
        area.m_curves.splice(area.m_curves.end(),_areaOpen.m_curves);
}

void Area::add(CArea &area, const TopoDS_Wire& wire,
        const gp_Trsf *trsf, double deflection) 
{
    CCurve ccurve;
    BRepTools_WireExplorer xp(trsf?TopoDS::Wire(
                wire.Moved(TopLoc_Location(*trsf))):wire);

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
        Base::Console().Warning("ccurve not closed\n");
        ccurve.append(ccurve.m_vertices.front());
    }
    area.append(ccurve);
}


void Area::clean(bool deleteShapes) {
    myShape.Nullify();
    delete myArea;
    myArea = NULL;
    delete myAreaOpen;
    myAreaOpen = NULL;
    if(deleteShapes)
        myShapes.clear();
}

void Area::add(const TopoDS_Shape &shape,short op) {
#define AREA_SRC_OP(_v) op
    PARAM_ENUM_CONVERT(AREA_SRC_OP,,PARAM_ENUM_EXCEPT,AREA_PARAMS_OPCODE);
    TopExp_Explorer it(shape, TopAbs_SHELL); 
    if(it.More()) 
        throw Base::ValueError("not a 2D shape");
    clean();
    if(myShapes.empty())
        Operation = ClipperLib::ctUnion;
    myShapes.push_back(Shape((short)Operation,shape));
}


void Area::setParams(const AreaParams &params) {
#define AREA_SRC2(_v) params._v
    // Validate all enum type of parameters
    PARAM_ENUM_CHECK(AREA_SRC2,PARAM_ENUM_EXCEPT,AREA_PARAMS_CONF);
    if(params!=myParams)
        clean();
    myParams = params;
}

void Area::addToBuild(CArea &area, const TopoDS_Shape &shape) {
    if(!myHaveFace) {
        TopExp_Explorer it(shape, TopAbs_FACE);
        myHaveFace = it.More();
    }
    CArea areaOpen;
    add(area,shape,&myTrsf,myParams.Deflection,&areaOpen,
            myParams.OpenMode==OpenModeEdges,myParams.Reorder);
    if(areaOpen.m_curves.size()) {
        if(&area == myArea || myParams.OpenMode == OpenModeNone)
            myAreaOpen->m_curves.splice(myAreaOpen->m_curves.end(),areaOpen.m_curves);
        else
            Base::Console().Warning("open wires discarded in clipping shapes\n");
    }
}

void Area::build() {
    if(myArea) return;

    if(myShapes.empty())
        throw Base::ValueError("Null shape");

#define AREA_SRC(_v) myParams._v
    PARAM_ENUM_CONVERT(AREA_SRC,,PARAM_ENUM_EXCEPT,AREA_PARAMS_CLIPPER_FILL);

    TopoDS_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    if(!myWorkPlane.IsNull())
        builder.Add(comp,myWorkPlane);
    else {
        for(const Shape &s : myShapes)
            builder.Add(comp, s.shape);
    }
    BRepLib_FindSurface planeFinder(comp,-1,Standard_True);
    if (!planeFinder.Found())
        throw Base::ValueError("shapes are not coplanar");

    myTrsf.SetTransformation(GeomAdaptor_Surface(
                planeFinder.Surface()).Plane().Position());

    myArea = new CArea();
    myAreaOpen = new CArea();

    CAreaConfig conf(myParams);
    CArea areaClip;

    short op = ClipperLib::ctUnion;
    bool pending = false;
    for(const Shape &s : myShapes) {
        if(op!=s.op) {
            if(myParams.OpenMode!=OpenModeNone)
                myArea->m_curves.splice(myArea->m_curves.end(),myAreaOpen->m_curves);
            pending = false;
            myArea->Clip((ClipperLib::ClipType)op,&areaClip,SubjectFill,ClipFill);
            areaClip.m_curves.clear();
            op=s.op;
        }
        addToBuild(op==ClipperLib::ctUnion?*myArea:areaClip,s.shape);
        pending = true;
    }
    if(pending){
        if(myParams.OpenMode!=OpenModeNone)
            myArea->m_curves.splice(myArea->m_curves.end(),myAreaOpen->m_curves);
        myArea->Clip((ClipperLib::ClipType)op,&areaClip,SubjectFill,ClipFill);
    }
    myArea->m_curves.splice(myArea->m_curves.end(),myAreaOpen->m_curves);
}

TopoDS_Shape Area::toShape(CArea &area, short fill) {
    gp_Trsf trsf(myTrsf.Inverted());
    bool bFill;
    switch(fill){
    case Area::FillAuto:
        bFill = myHaveFace;
        break;
    case Area::FillFace:
        bFill = true;
        break;
    default:
        bFill = false;
    }
    if(myParams.FitArcs) {
        if(&area == myArea) {
            CArea copy(area);
            copy.FitArcs();
            return toShape(copy,bFill,&trsf);
        }
        area.FitArcs();
    }
    return toShape(area,bFill,&trsf);
}

const TopoDS_Shape &Area::getShape() {
    if(myShape.IsNull()) {
        build();
        CAreaConfig conf(myParams);
        myShape = toShape(*myArea,myParams.Fill);
    }
    return myShape;
}

TopoDS_Shape Area::makeOffset(PARAM_ARGS(ARG,AREA_PARAMS_OFFSET)) {
    std::list<TopoDS_Shape> shapes;
    makeOffset(shapes,PARAM_FIELDS(ARG,AREA_PARAMS_OFFSET));
    if(shapes.empty())
        return TopoDS_Shape();
    if(shapes.size()==1)
        return shapes.front();
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    for(const TopoDS_Shape &s : shapes)
        builder.Add(compound,s);
    return compound;
}

void Area::makeOffset(std::list<TopoDS_Shape> &shapes,
                      PARAM_ARGS(ARG,AREA_PARAMS_OFFSET))
{
    if(fabs(offset)<Precision::Confusion()){
        shapes.push_back(getShape());
        return;
    }

    build();
    CAreaConfig conf(myParams);

    if(myParams.Thicken) {
        CArea area(*myArea);
        area.Thicken(fabs(offset));
        shapes.push_back(toShape(area,myParams.Fill));
        return;
    }

    long count = 1;
    if(extra_pass) {
        if(fabs(stepover)<Precision::Confusion())
            stepover = offset;
        if(extra_pass > 0) {
            count += extra_pass;
        }else{
            if(stepover>0 || offset>0)
                throw Base::ValueError("invalid extra count");
            // In this case, we loop until no outputs from clipper
            count=-1;
        }
    }

    PARAM_ENUM_CONVERT(AREA_SRC,,PARAM_ENUM_EXCEPT,AREA_PARAMS_OFFSET_CONF);
#ifdef AREA_OFFSET_ALGO
    PARAM_ENUM_CONVERT(AREA_SRC,,PARAM_ENUM_EXCEPT,AREA_PARAMS_CLIPPER_FILL);
#endif

    for(int i=0;count<0||i<count;++i,offset+=stepover) {
        CArea area;
        CArea areaOpen;
#ifdef AREA_OFFSET_ALGO
        if(myParams.Algo == Area::Algolibarea) {
            for(const CCurve &c : myArea->m_curves) {
                if(c.IsClosed())
                    area.append(c);
                else
                    areaOpen.append(c);
            }
        }else
#endif
            area = *myArea;

#ifdef AREA_OFFSET_ALGO
        switch(myParams.Algo){
        case Area::Algolibarea:
            // libarea somehow fails offset without Reorder, but ClipperOffset
            // works okay. Don't know why
            area.Reorder();
            area.Offset(-offset);
            if(areaOpen.m_curves.size()) {
                areaOpen.Thicken(offset);
                area.Clip(ClipperLib::ctUnion,&areaOpen,SubjectFill,ClipFill);
            }
            break;
        case Area::AlgoClipperOffset:
#endif
            area.OffsetWithClipper(offset,JoinType,EndType,
                    myParams.MiterLimit,myParams.RoundPreceision);
#ifdef AREA_OFFSET_ALGO
            break;
        }
#endif

        if(area.m_curves.empty())
            return;

        if(count == 1) {
            shapes.push_back(toShape(area,myParams.Fill));
            return;
        }
        shapes.push_back(toShape(area,Area::FillNone));
    }
}

TopoDS_Shape Area::makePocket(PARAM_ARGS(ARG,AREA_PARAMS_POCKET)) {
    if(tool_radius < Precision::Confusion()) 
        throw Base::ValueError("tool radius too small");

    if(stepover == 0.0)
        stepover = tool_radius;

    if(stepover < Precision::Confusion())
        throw Base::ValueError("stepover too small");

    if(mode == Area::PocketModeNone)
        return TopoDS_Shape();

    PocketMode pm;
    switch(mode) {
    case Area::PocketModeZigZag:
        pm = ZigZagPocketMode;
        break;
    case Area::PocketModeSpiral:
        pm = SpiralPocketMode;
        break;
    case Area::PocketModeOffset: {
        PARAM_DECLARE_INIT(NAME,AREA_PARAMS_OFFSET);
        Offset = -tool_radius-extra_offset;
        ExtraPass = -1;
        Stepover = -stepover;
        return makeOffset(PARAM_FIELDS(NAME,AREA_PARAMS_OFFSET));
    }case Area::PocketModeZigZagOffset:
	    pm = ZigZagThenSingleOffsetPocketMode;
        break;
    default:
        throw Base::ValueError("unknown poket mode");
    }

    build();
    CAreaConfig conf(myParams);
    CAreaPocketParams params(
            tool_radius,extra_offset,stepover,from_center,pm,zig_angle);
    CArea in(*myArea),out;
    // MakePcoketToolPath internally uses libarea Offset which somehow demands
    // reorder before input, otherwise nothing is shown.
    in.Reorder();
    in.MakePocketToolpath(out.m_curves,params);
    return toShape(out,FillNone);
}

static inline bool IsLeft(const gp_Pnt &a, const gp_Pnt &b, const gp_Pnt &c) {
    return ((b.X() - a.X())*(c.Y() - a.Y()) - (b.Y() - a.Y())*(c.X() - a.X())) > 0;
}

TopoDS_Shape Area::toShape(const CArea &area, bool fill, const gp_Trsf *trsf) {
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    for(const CCurve &c : area.m_curves) {
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
        if(c.IsClosed() && !BRep_Tool::IsClosed(mkWire.Wire())){
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

        if(trsf)
            builder.Add(compound,mkWire.Wire().Moved(TopLoc_Location(*trsf)));
        else
            builder.Add(compound,mkWire.Wire());
    }
    
    if(fill) {
        try{
            Part::FaceMakerBullseye mkFace;
            if(trsf)
                mkFace.setPlane(gp_Pln().Transformed(*trsf));
            for(TopExp_Explorer it(compound, TopAbs_WIRE); it.More(); it.Next())
                mkFace.addWire(TopoDS::Wire(it.Current()));
            mkFace.Build();
            if (mkFace.Shape().IsNull())
                Base::Console().Warning("FaceMakerBullseye returns null shape\n");
            return mkFace.Shape();
        }catch (Base::Exception &e){
            Base::Console().Warning("FaceMakerBullseye failed: %s\n", e.what());
        }
    }
    return compound;
}


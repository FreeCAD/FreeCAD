/****************************************************************************
 *   Copyright (c) 2018 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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
# include <BSplCLib.hxx>
# include <Bnd_Box.hxx>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_CompCurve.hxx>
# include <BRepAdaptor_HCurve.hxx>
# include <BRepAdaptor_HCompCurve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgo_Fuse.hxx>
# include <BRepAlgoAPI_Section.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_FindPlane.hxx>
# include <BRepLib_FindSurface.hxx>
# include <BRepBuilderAPI_GTransform.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakePolygon.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_MakeShell.hxx>
# include <BRepBuilderAPI_NurbsConvert.hxx>
# include <BRepBuilderAPI_FaceError.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepCheck_ListIteratorOfListOfStatus.hxx>
# include <BRepCheck_Result.hxx>
# include <BRepClass_FaceClassifier.hxx>
# include <BRepFilletAPI_MakeFillet.hxx>
# include <BRepFilletAPI_MakeChamfer.hxx>
# include <BRepGProp.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <BRepMesh_Triangle.hxx>
# include <BRepMesh_Edge.hxx>
# include <BRepOffsetAPI_MakeThickSolid.hxx>
# include <BRepOffsetAPI_MakeOffsetShape.hxx>
# include <BRepOffsetAPI_MakeOffset.hxx>
# include <BRepOffsetAPI_MakePipe.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <BRepOffsetAPI_Sewing.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepTools.hxx>
# include <BRepTools_ReShape.hxx>
# include <BRepTools_ShapeSet.hxx>
# include <BRepTools_WireExplorer.hxx>
# include <BRepFill_CompatibleWires.hxx>
# include <GCE2d_MakeSegment.hxx>
# include <GCPnts_AbscissaPoint.hxx>
# include <GCPnts_UniformAbscissa.hxx>
# include <Geom2d_Line.hxx>
# include <Geom2d_TrimmedCurve.hxx>
# include <GeomLProp_SLProps.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <GeomFill_CorrectedFrenet.hxx>
# include <GeomFill_CurveAndTrihedron.hxx>
# include <GeomFill_EvolvedSection.hxx>
# include <GeomFill_Pipe.hxx>
# include <GeomFill_SectionLaw.hxx>
# include <GeomFill_Sweep.hxx>
# include <GeomLib.hxx>
# include <GProp_GProps.hxx>
# include <Law_BSpFunc.hxx>
# include <Law_BSpline.hxx>
# include <Law_BSpFunc.hxx>
# include <Law_Constant.hxx>
# include <Law_Linear.hxx>
# include <Law_S.hxx>
# include <TopTools_HSequenceOfShape.hxx>
# include <Interface_Static.hxx>
# include <IGESControl_Controller.hxx>
# include <IGESControl_Writer.hxx>
# include <IGESControl_Reader.hxx>
# include <IGESData_GlobalSection.hxx>
# include <IGESData_IGESModel.hxx>
# include <STEPControl_Writer.hxx>
# include <STEPControl_Reader.hxx>
# include <TopTools_MapOfShape.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
# include <Geom2d_Ellipse.hxx>
# include <Geom_BezierCurve.hxx>
# include <Geom_BezierSurface.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Geom_SurfaceOfLinearExtrusion.hxx>
# include <Geom_SurfaceOfRevolution.hxx>
# include <Geom_Circle.hxx>
# include <Geom_ConicalSurface.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom_Ellipse.hxx>
# include <Geom_Hyperbola.hxx>
# include <Geom_Line.hxx>
# include <Geom_Parabola.hxx>
# include <Geom_Plane.hxx>
# include <Geom_CartesianPoint.hxx>
# include <Geom_SphericalSurface.hxx>
# include <Geom_ToroidalSurface.hxx>
# include <Poly_Triangulation.hxx>
# include <Standard_Failure.hxx>
# include <StlAPI_Writer.hxx>
# include <Standard_Failure.hxx>
# include <gp_GTrsf.hxx>
# include <ShapeAnalysis_Shell.hxx>
# include <ShapeBuild_ReShape.hxx>
# include <ShapeExtend_Explorer.hxx>
# include <ShapeFix_Edge.hxx>
# include <ShapeFix_Face.hxx>
# include <ShapeFix_Shell.hxx>
# include <ShapeFix_Solid.hxx>
# include <ShapeUpgrade_ShellSewing.hxx>
# include <ShapeUpgrade_RemoveInternalWires.hxx>
# include <Standard_Version.hxx>
# include <ShapeFix_Wire.hxx>
# include <ShapeAnalysis.hxx>
# include <BRepFill.hxx>
# include <BRepOffsetAPI_DraftAngle.hxx>
#endif
# include <ShapeAnalysis_FreeBoundsProperties.hxx>
# include <ShapeAnalysis_FreeBoundData.hxx>
# include <ShapeAnalysis_FreeBounds.hxx>
# include <BRepOffsetAPI_MakeFilling.hxx>

#include <array>
#include <deque>
#include <boost/algorithm/string/predicate.hpp>
#include <Base/Exception.h>
#include <Base/Console.h>


#include "PartPyCXX.h"
#include "TopoShape.h"
#include "TopoShapeOpCode.h"
#include "CrossSection.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeVertexPy.h"
#include "modelRefine.h"
#include "Tools.h"
#include "FaceMaker.h"

#define TOPOP_VERSION 6

FC_LOG_LEVEL_INIT("TopoShape",true,true);

using namespace Part;

#define _HANDLE_NULL_SHAPE(_msg,_throw) do {\
    if(_throw) {\
        FC_ERR(_msg);\
        Standard_Failure::Raise(_msg);\
    }\
    FC_WARN(_msg);\
}while(0)

#define HANDLE_NULL_SHAPE _HANDLE_NULL_SHAPE("Null shape",true)
#define HANDLE_NULL_INPUT _HANDLE_NULL_SHAPE("Null input shape",true)
#define WARN_NULL_INPUT _HANDLE_NULL_SHAPE("Null input shape",false)

static void expandCompound(const TopoShape &shape, std::vector<TopoShape> &res) {
    if(shape.isNull())
        HANDLE_NULL_INPUT;
    if(shape.getShape().ShapeType() != TopAbs_COMPOUND) {
        res.push_back(shape);
        return;
    }
    for(auto &s : shape.getSubTopoShapes())
        expandCompound(s,res);
}

void TopoShape::mapSubElement(TopAbs_ShapeEnum type,
        const TopoShape &other, const char *op, bool mapAll, bool appendTag) 
{
    if(!canMapElement(other)) 
        return;
    TopTools_IndexedMapOfShape shapeMap;
    TopExp::MapShapes(_Shape, type, shapeMap);
    mapSubElement(type,shapeMap,other,op,mapAll,appendTag);
}

void TopoShape::mapSubElement(TopAbs_ShapeEnum type, const TopoShape &other,
        const TopTools_IndexedMapOfShape &otherMap, const char *op, bool mapAll, bool appendTag)
{
    if(!canMapElement(other)) 
        return;
    TopTools_IndexedMapOfShape shapeMap;
    if(otherMap.Extent())
        TopExp::MapShapes(_Shape, type, shapeMap);
    mapSubElement(type,shapeMap,other,otherMap,op,mapAll,appendTag);
}

void TopoShape::mapSubElement(TopAbs_ShapeEnum type, 
        const TopTools_IndexedMapOfShape &shapeMap, 
        const TopoShape &other, const char *op, bool mapAll,bool appendTag)
{
    if(!canMapElement(other)) 
        return;
    TopTools_IndexedMapOfShape otherMap;
    if(shapeMap.Extent())
        TopExp::MapShapes(other._Shape, type, otherMap);
    mapSubElement(type,shapeMap,other,otherMap,op,mapAll,appendTag);
}

void TopoShape::mapSubElement(TopAbs_ShapeEnum type, 
        const TopTools_IndexedMapOfShape &shapeMap, const TopoShape &other, 
        const TopTools_IndexedMapOfShape &otherMap, const char *op, bool mapAll, bool appendTag)
{
    if(!canMapElement(other))
        return;
    if(shapeMap.Extent() && otherMap.Extent()) {
        switch(type) {
        case TopAbs_EDGE:
        case TopAbs_VERTEX:
        case TopAbs_FACE:
            break;
        default:
            Standard_Failure::Raise("invlaid shape type");
            return;
        }
        if(other.Hasher) {
            if(Hasher) {
                if(other.Hasher!=Hasher)
                    throw Base::RuntimeError("hasher mismatch");
            }else
                Hasher = other.Hasher;
        }
        const char *shapetype = shapeName(type).c_str();
        std::ostringstream ss;
        for(int i=1;i<=otherMap.Extent();++i) {
            auto shape = otherMap.FindKey(i);
            int idx = shapeMap.FindIndex(shape);
            if(!idx) continue;
            ss.str("");
            ss << shapetype << idx;
            std::string element = ss.str();
            ss.str("");
            ss << shapetype << i;
            for(auto &v : other.getElementMappedNames(ss.str().c_str(),true)) {
                auto &name = v.first;
                auto &sids = v.second;
                if(sids.size() && !other.Hasher) 
                    throw Base::RuntimeError("missing hasher");
                ss.str("");
                processName(name,ss,sids,op,appendTag,other.Tag,true);
                setElementName(element.c_str(),name.c_str(),ss.str().c_str(),&sids);
            }
        }
    }
    if(mapAll) {
        if(type == TopAbs_FACE)
            mapSubElement(TopAbs_EDGE,other,op,true,appendTag);
        else if(type == TopAbs_EDGE)
            mapSubElement(TopAbs_VERTEX,other,op,true,appendTag);
    }
}

void TopoShape::mapSubElementsTo(TopAbs_ShapeEnum type, 
        std::vector<TopoShape> &shapes, const char *op, bool mapAll, bool appendTag) const
{
    if(!canMapElement())
        return;
    TopTools_IndexedMapOfShape myMap;
    TopExp::MapShapes(_Shape, type, myMap);
    if(myMap.Extent()) {
        for(auto &shape : shapes)
            shape.mapSubElement(type,*this,myMap,op,false,appendTag);
    }
    if(mapAll) {
        if(type == TopAbs_FACE)
            mapSubElementsTo(TopAbs_EDGE,shapes,op,true,appendTag);
        else if(type == TopAbs_EDGE)
            mapSubElementsTo(TopAbs_VERTEX,shapes,op,true,appendTag);
    }
}

std::vector<TopoShape> TopoShape::getSubTopoShapes(TopAbs_ShapeEnum type) const {
    std::vector<TopoShape> ret;
    if(isNull()) return ret;
    if(type == TopAbs_SHAPE) {
        for(TopoDS_Iterator it(_Shape);it.More();it.Next()) {
            ret.emplace_back(it.Value());
            ret.back().Tag = Tag;
        }
    }else{
        TopTools_IndexedMapOfShape M;
        for(TopExp_Explorer it(_Shape,type);it.More();it.Next())
            M.Add(it.Current());

        ret.reserve(M.Extent());
        for(int i=1;i<=M.Extent();++i) {
            ret.emplace_back(M(i));
            ret.back().Tag = Tag;
        }
    }
    mapSubElementsTo(TopAbs_FACE,ret);
    return ret;
}

std::pair<TopAbs_ShapeEnum,int> TopoShape::shapeTypeAndIndex(const char *name) {
    int idx = 0;
    TopAbs_ShapeEnum type = TopAbs_SHAPE;
    try {
        type = shapeType(name);
        idx = std::atoi(name+shapeName(type).size());
    }catch(Standard_Failure &)
    {}
    return std::make_pair(type,idx);
}

TopoShape TopoShape::getSubTopoShape(const char *Type) const {
    Type = getElementName(Type);
    auto type = shapeType(Type);
    int idx = std::atoi(Type+shapeName(type).size());
    return getSubTopoShape(type,idx);
}

TopoShape TopoShape::getSubTopoShape(TopAbs_ShapeEnum type, int idx) const {
    if(idx <= 0)
        Standard_Failure::Raise("Invalid shape index");
    if (this->_Shape.IsNull())
        return TopoShape(Tag);

    TopTools_IndexedMapOfShape anIndices;
    TopExp::MapShapes(this->_Shape, type, anIndices);
    if(idx > anIndices.Extent())
        Standard_Failure::Raise("Shape index out of bound");

    TopoShape ret(Tag,Hasher,anIndices.FindKey(idx));
    if(type == TopAbs_VERTEX || type == TopAbs_EDGE || type == TopAbs_FACE)
        ret.mapSubElement(type,*this,anIndices);
    else if(anIndices.Extent()==1) 
        ret.resetElementMap(_ElementMap);
    else
        ret.mapSubElement(TopAbs_FACE,*this);
    return ret;
}

std::string TopoShape::getElementMapVersion() const{
    std::ostringstream ss;
    ss << TOPOP_VERSION << '.' << std::hex << OCC_VERSION_HEX 
        << '.' << Data::ComplexGeoData::getElementMapVersion();
    return ss.str();
}

void TopoShape::mapSubElement(TopAbs_ShapeEnum type, const std::vector<TopoShape> &shapes, 
        const char *op, bool mapAll, bool appendTag) 
{
    TopTools_IndexedMapOfShape shapeMap;
    TopExp::MapShapes(_Shape, type, shapeMap);
    if(shapeMap.Extent()) {
        for(auto s : shapes) {
            if(!s.isNull()) 
                mapSubElement(type,shapeMap,s,op,false,appendTag);
        }
    }
    if(mapAll) {
        if(type == TopAbs_FACE)
            mapSubElement(TopAbs_EDGE,shapes,op,true,appendTag);
        else if(type == TopAbs_EDGE)
            mapSubElement(TopAbs_VERTEX,shapes,op,true,appendTag);
    }
}

TopoShape &TopoShape::makECompound(const std::vector<TopoShape> &shapes, 
        bool appendTag, const char *op, bool force)
{
    if(appendTag && op==0)
        op = TOPOP_COMPOUND;

    _Shape.Nullify();
    resetElementMap();

    if(shapes.empty()) 
        HANDLE_NULL_SHAPE;

    if(!force && shapes.size()==1) {
        *this = shapes[0];
        return *this;
    }

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    int count = 0;
    for(auto &s : shapes) {
        if(s.isNull()) {
            WARN_NULL_INPUT;
            continue;
        }
        builder.Add(comp,s.getShape());
        ++count; 
    }
    if(!count) 
        HANDLE_NULL_SHAPE;
    _Shape = comp;
    mapSubElement(TopAbs_FACE,shapes,op,true,appendTag);
    return *this;
}

TopoShape &TopoShape::makETransform(const TopoShape &shape, 
        const Base::Matrix4D &rclTrf, const char *op, bool appendTag, bool checkScale) 
{
    if(checkScale) {
        bool gtrsf = false;
        // check for uniform scaling
        //
        // scaling factors are the colum vector length. We use square distance and
        // ignore the actual scaling signess
        //
        double dx = Base::Vector3d(rclTrf[0][0],rclTrf[1][0],rclTrf[2][0]).Sqr();
        double dy = Base::Vector3d(rclTrf[0][1],rclTrf[1][1],rclTrf[2][1]).Sqr();
        if(fabs(dx-dy)>Precision::SquareConfusion())
            gtrsf = true;
        else {
            double dz = Base::Vector3d(rclTrf[0][2],rclTrf[1][2],rclTrf[2][2]).Sqr();
            if(fabs(dy-dz)>Precision::SquareConfusion())
                gtrsf = true;
        }
        if(gtrsf)
            return makEGTransform(shape,rclTrf,op,appendTag);
    }

    resetElementMap();

    // location transformation
    BRepBuilderAPI_Transform mkTrf(shape.getShape(), convert(rclTrf), Standard_False);
    TopoShape tmp(shape);
    tmp._Shape = mkTrf.Shape();
    if(op || (appendTag && shape.Tag && shape.Tag!=Tag)) {
        _Shape = tmp._Shape;
        mapSubElement(TopAbs_FACE,tmp,op,true,appendTag);
    } else
        *this = tmp;
    return *this;
}

TopoShape &TopoShape::makEGTransform(const TopoShape &shape, 
        const Base::Matrix4D &rclTrf, const char *op, bool appendTag)
{
    if(!op) op = TOPOP_GTRANSFORM;
    gp_GTrsf mat;
    mat.SetValue(1,1,rclTrf[0][0]);
    mat.SetValue(2,1,rclTrf[1][0]);
    mat.SetValue(3,1,rclTrf[2][0]);
    mat.SetValue(1,2,rclTrf[0][1]);
    mat.SetValue(2,2,rclTrf[1][1]);
    mat.SetValue(3,2,rclTrf[2][1]);
    mat.SetValue(1,3,rclTrf[0][2]);
    mat.SetValue(2,3,rclTrf[1][2]);
    mat.SetValue(3,3,rclTrf[2][2]);
    mat.SetValue(1,4,rclTrf[0][3]);
    mat.SetValue(2,4,rclTrf[1][3]);
    mat.SetValue(3,4,rclTrf[2][3]);

    // geometric transformation
    BRepBuilderAPI_GTransform mkTrf(shape.getShape(), mat);
    return makEShape(mkTrf,shape,op,appendTag);
}


TopoShape &TopoShape::makECopy(const TopoShape &shape, const char *op, bool appendTag)
{
    _Shape.Nullify();
    resetElementMap();

    if(shape.isNull()) 
        return *this;

    TopoShape tmp(shape);
    tmp._Shape = BRepBuilderAPI_Copy(shape.getShape()).Shape();
    if(op || (appendTag && shape.Tag && shape.Tag!=Tag)) {
        _Shape = tmp._Shape;
        mapSubElement(TopAbs_FACE,tmp,op,true,appendTag);
    }else
        *this = tmp;
    return *this;
}

static std::vector<TopoShape> prepareProfiles(const std::vector<TopoShape> &shapes,size_t offset=0) {
    std::vector<TopoShape> ret;
    for(size_t i=offset;i<shapes.size();++i) {
        auto &sh = shapes[i];
        if(sh.isNull())
            HANDLE_NULL_INPUT;
        auto shape = sh.getShape();
        // Extract first element of a compound
        if (shape.ShapeType() == TopAbs_COMPOUND) {
            TopoDS_Iterator it(shape);
            for (; it.More(); it.Next()) {
                if (it.Value().IsNull())
                    HANDLE_NULL_INPUT;
                shape = it.Value();
                break;
            }
        } else if (shape.ShapeType() == TopAbs_FACE) {
            shape = ShapeAnalysis::OuterWire(TopoDS::Face(shape));
        } else if (shape.ShapeType() == TopAbs_WIRE) {
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Wire(shape));
            shape = mkWire.Wire();
        } else if (shape.ShapeType() == TopAbs_EDGE) {
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(shape));
            shape = mkWire.Wire();
        } else if (shape.ShapeType() != TopAbs_VERTEX) {
            Standard_Failure::Raise("Profile shape is not a vertex, edge, wire nor face.");
        }
        ret.push_back(shape);
    }
    if(ret.empty())
        Standard_Failure::Raise("No profile");
    return ret;
}

TopoShape &TopoShape::makEPipeShell( const std::vector<TopoShape> &shapes,
        const Standard_Boolean make_solid, const Standard_Boolean isFrenet, int transition,
        const char *op, bool appendTag)
{
    if(!op) op = TOPOP_PIPE_SHELL;
    _Shape.Nullify();
    resetElementMap();

    if(shapes.size()<2)
        Standard_Failure::Raise("Not enough input shape");

    auto spine = shapes.front().makEWires();
    if(spine.isNull())
        HANDLE_NULL_INPUT;
    if(spine.getShape().ShapeType()!=TopAbs_WIRE)
        Standard_Failure::Raise("Spine shape cannot form a single wire");

    BRepOffsetAPI_MakePipeShell mkPipeShell(TopoDS::Wire(spine.getShape()));
    BRepBuilderAPI_TransitionMode transMode;
    switch (transition) {
        case 1: transMode = BRepBuilderAPI_RightCorner;
            break;
        case 2: transMode = BRepBuilderAPI_RoundCorner;
            break;
        default: transMode = BRepBuilderAPI_Transformed;
            break;
    }
    mkPipeShell.SetMode(isFrenet);
    mkPipeShell.SetTransitionMode(transMode);

    for(auto &sh : prepareProfiles(shapes,1))
        mkPipeShell.Add(sh.getShape());

    if (!mkPipeShell.IsReady()) 
        Standard_Failure::Raise("shape is not ready to build");
    else 
        mkPipeShell.Build();

    if (make_solid)	mkPipeShell.MakeSolid();

    return makEShape(mkPipeShell,shapes,op,appendTag);
}

TopoShape &TopoShape::makERuledSurface(const std::vector<TopoShape> &shapes,
        int orientation, const char *op, bool appendTag)
{
    if(!op) op = TOPOP_RULED_SURFACE;
    _Shape.Nullify();
    resetElementMap();

    if(shapes.size()!=2)
        Standard_Failure::Raise("Wrong number of input shape");

    std::array<TopoDS_Shape,2> curves;
    int i=0;
    for(auto &s : shapes) {
        if(s.isNull())
            HANDLE_NULL_INPUT;
        const auto &shape = s.getShape();
        auto type = shape.ShapeType();
        if(type == TopAbs_WIRE || type == TopAbs_EDGE) {
            curves[i++] = shape;
            continue;
        }
        auto count = s.countSubShapes(TopAbs_WIRE);
        if(count>1)
            Standard_Failure::Raise("Input shape has more than one wire");
        if(count==1) {
            curves[i++] = s.getSubShape(TopAbs_WIRE,1);
            continue;
        }
        count = s.countSubShapes(TopAbs_EDGE);
        if(count==0)
            Standard_Failure::Raise("Input shape has no edge"); 
        if(count == 1) {
            curves[i++] = s.getSubShape(TopAbs_EDGE,1);
            continue;
        }
        curves[i] = s.makEWires().getShape();
        if(curves[i].IsNull())
            HANDLE_NULL_INPUT;
        if(curves[i].ShapeType()!=TopAbs_WIRE)
            Standard_Failure::Raise("Input shape forms more than one wire");
        ++i;
    }

    if(curves[0].ShapeType()!=curves[1].ShapeType()) {
        for(auto &curve : curves) {
            if(curve.ShapeType() == TopAbs_EDGE)
                curve = BRepLib_MakeWire(TopoDS::Edge(curve));
        }
    }

    auto &S1 = curves[0];
    auto &S2 = curves[1];
    bool isWire = S1.ShapeType()==TopAbs_WIRE;

    // https://forum.freecadweb.org/viewtopic.php?f=8&t=24052
    //
    // if both shapes are sub-elements of one common shape then the fill algorithm
    // leads to problems if the shape has set a placement
    // The workaround is to reset the placement before calling BRepFill and then
    // applying the placement to the output shape
    TopLoc_Location Loc = S1.Location();
    if(!Loc.IsIdentity() && Loc==S2.Location()) {
        S1.Location(TopLoc_Location());
        S2.Location(TopLoc_Location());
    }else
        Loc = TopLoc_Location();

    if (orientation == 0) {
        // Automatic
        Handle(Adaptor3d_HCurve) a1;
        Handle(Adaptor3d_HCurve) a2;
        if (!isWire) {
            BRepAdaptor_Curve adapt1(TopoDS::Edge(S1));
            BRepAdaptor_Curve adapt2(TopoDS::Edge(S2));
            a1 = new BRepAdaptor_HCurve(adapt1);
            a2 = new BRepAdaptor_HCurve(adapt2);
        }
        else {
            BRepAdaptor_CompCurve adapt1(TopoDS::Wire(S1));
            BRepAdaptor_CompCurve adapt2(TopoDS::Wire(S2));
            a1 = new BRepAdaptor_HCompCurve(adapt1);
            a2 = new BRepAdaptor_HCompCurve(adapt2);
        }

        if (!a1.IsNull() && !a2.IsNull()) {
            // get end points of 1st curve
            gp_Pnt p1 = a1->Value(a1->FirstParameter());
            gp_Pnt p2 = a1->Value(a1->LastParameter());
            if (S1.Orientation() == TopAbs_REVERSED) {
                std::swap(p1, p2);
            }

            // get end points of 2nd curve
            gp_Pnt p3 = a2->Value(a2->FirstParameter());
            gp_Pnt p4 = a2->Value(a2->LastParameter());
            if (S2.Orientation() == TopAbs_REVERSED) {
                std::swap(p3, p4);
            }

            // Form two triangles (P1,P2,P3) and (P4,P3,P2) and check their normals.
            // If the dot product is negative then it's assumed that the resulting face
            // is twisted, hence the 2nd edge is reversed.
            gp_Vec v1(p1, p2);
            gp_Vec v2(p1, p3);
            gp_Vec n1 = v1.Crossed(v2);

            gp_Vec v3(p4, p3);
            gp_Vec v4(p4, p2);
            gp_Vec n2 = v3.Crossed(v4);

            if (n1.Dot(n2) < 0) {
                S2.Reverse();
            }
        }
    }
    else if (orientation == 2) {
        // Reverse
        S2.Reverse();
    }

    TopoDS_Shape ruledShape;
    if (!isWire) {
        ruledShape = BRepFill::Face(TopoDS::Edge(S1), TopoDS::Edge(S2));
    }
    else {
        ruledShape = BRepFill::Shell(TopoDS::Wire(S1), TopoDS::Wire(S2));
    }

    // re-apply the placement in case we reset it
    if (!Loc.IsIdentity())
        ruledShape.Move(Loc);

    // Use empty mapper and let makEShape name the created surface with lower elements.
    return makESHAPE(ruledShape,Mapper(),shapes,op,appendTag);
}

struct MapperMaker: Part::TopoShape::Mapper {
    BRepBuilderAPI_MakeShape &maker;
    MapperMaker(BRepBuilderAPI_MakeShape &maker)
        :maker(maker)
    {}
    virtual std::vector<TopoDS_Shape> modified(const TopoDS_Shape &s) const override {
        std::vector<TopoDS_Shape> ret;
        TopTools_ListIteratorOfListOfShape it;
        for (it.Initialize(maker.Modified(s)); it.More(); it.Next())
            ret.push_back(it.Value());
        return ret;
    }
    virtual std::vector<TopoDS_Shape> generated(const TopoDS_Shape &s) const override {
        std::vector<TopoDS_Shape> ret;
        TopTools_ListIteratorOfListOfShape it;
        for (it.Initialize(maker.Generated(s)); it.More(); it.Next())
            ret.push_back(it.Value());
        return ret;
    }
};

struct MapperThruSections: MapperMaker {
    TopTools_IndexedMapOfShape firstMap;
    TopoDS_Shape firstProfile;
    TopTools_IndexedMapOfShape lastMap;
    TopoDS_Shape lastProfile;
    
    MapperThruSections(BRepOffsetAPI_ThruSections &tmaker, 
            const std::vector<TopoShape> &profiles)
        :MapperMaker(tmaker)
    {
        if(!tmaker.FirstShape().IsNull()) {
            firstProfile = profiles.front().getShape();
            TopExp::MapShapes(firstProfile, TopAbs_EDGE, firstMap);
        }
        if(!tmaker.LastShape().IsNull()) {
            lastProfile = profiles.back().getShape();
            TopExp::MapShapes(lastProfile, TopAbs_EDGE, lastMap);
        }
    }
    virtual std::vector<TopoDS_Shape> generated(const TopoDS_Shape &s) const override {
        std::vector<TopoDS_Shape> ret = MapperMaker::generated(s);
        if(ret.size()) return ret;
        auto &tmaker = static_cast<BRepOffsetAPI_ThruSections&>(maker);
        auto shape = tmaker.GeneratedFace(s);
        if(!shape.IsNull())
            ret.push_back(shape);
        if(firstProfile.IsSame(s) || firstMap.FindIndex(s))
            ret.push_back(tmaker.FirstShape());
        else if(lastProfile.IsSame(s) || lastMap.FindIndex(s))
            ret.push_back(tmaker.LastShape());
        return ret;
    }
};

TopoShape &TopoShape::makEShape(BRepOffsetAPI_ThruSections &mk, const TopoShape &source,
        const char *op, bool appendTag)
{
    if(!op) op = TOPOP_THRU_SECTIONS;
    return makEShape(mk,std::vector<TopoShape>(1,source),op,appendTag);
}

TopoShape &TopoShape::makEShape(BRepOffsetAPI_ThruSections &mk, const std::vector<TopoShape> &sources,
        const char *op, bool appendTag)
{
    if(!op) op = TOPOP_THRU_SECTIONS;
    return makESHAPE(mk.Shape(),MapperThruSections(mk,sources),sources,op,appendTag);
}

TopoShape &TopoShape::makELoft(const std::vector<TopoShape> &shapes,
                               Standard_Boolean isSolid,
                               Standard_Boolean isRuled,
                               Standard_Boolean isClosed,
                               Standard_Integer maxDegree,
                               const char *op, bool appendTag)
{
    if(!op) op = TOPOP_LOFT;
    _Shape.Nullify();
    resetElementMap();

    // http://opencascade.blogspot.com/2010/01/surface-modeling-part5.html
    BRepOffsetAPI_ThruSections aGenerator (isSolid,isRuled);
    aGenerator.SetMaxDegree(maxDegree);

    auto profiles = prepareProfiles(shapes);
    if (shapes.size() < 2)
        Standard_Failure::Raise("Need at least two vertices, edges or wires to create loft face");

    for(auto &sh : profiles) {
        const auto &shape = sh.getShape();
        if(shape.ShapeType() == TopAbs_VERTEX)
            aGenerator.AddVertex(TopoDS::Vertex (shape));
        else
            aGenerator.AddWire(TopoDS::Wire (shape));
    }
    // close loft by duplicating initial profile as last profile.  not perfect. 
    if (isClosed) {
    /* can only close loft in certain combinations of Vertex/Wire(Edge):
        - V1-W1-W2-W3-V2  ==> V1-W1-W2-W3-V2-V1  invalid closed
        - V1-W1-W2-W3     ==> V1-W1-W2-W3-V1     valid closed
        - W1-W2-W3-V1     ==> W1-W2-W3-V1-W1     invalid closed
        - W1-W2-W3        ==> W1-W2-W3-W1        valid closed*/
        if (profiles.back().getShape().ShapeType() == TopAbs_VERTEX) {
            Base::Console().Message("TopoShape::makeLoft: can't close Loft with Vertex as last profile. 'Closed' ignored.\n");
        }
        else {
            // repeat Add logic above for first profile
            const TopoDS_Shape& firstProfile = profiles.front().getShape();
            if (firstProfile.ShapeType() == TopAbs_VERTEX)  {
                aGenerator.AddVertex(TopoDS::Vertex (firstProfile));
            }
            else if (firstProfile.ShapeType() == TopAbs_EDGE)  {
                aGenerator.AddWire(BRepBuilderAPI_MakeWire(TopoDS::Edge(firstProfile)).Wire());
            }
            else if (firstProfile.ShapeType() == TopAbs_WIRE)  {
                aGenerator.AddWire(TopoDS::Wire (firstProfile));
            }
        }     
    }

    Standard_Boolean anIsCheck = Standard_True;
    aGenerator.CheckCompatibility (anIsCheck);   // use BRepFill_CompatibleWires on profiles. force #edges, orientation, "origin" to match.

    aGenerator.Build();
    return makESHAPE(aGenerator.Shape(),MapperThruSections(aGenerator,profiles),shapes,op,appendTag);
}

TopoShape &TopoShape::makEPrism(const TopoShape &base, const gp_Vec& vec, const char *op, bool appendTag) {
    if(!op) op = TOPOP_EXTRUDE;
    _Shape.Nullify();
    resetElementMap();
    if(base.isNull()) 
        HANDLE_NULL_SHAPE;
    BRepPrimAPI_MakePrism mkPrism(base.getShape(), vec);
    return makEShape(mkPrism,base,op,appendTag);
}

TopoShape &TopoShape::makERevolve(const TopoShape &_base, const gp_Ax1& axis, 
        double d, const char *face_maker, const char *op, bool appendTag)
{
    if(!op) op = TOPOP_REVOLVE;
    _Shape.Nullify();
    resetElementMap();

    TopoShape base(_base);
    if(base.isNull()) 
        HANDLE_NULL_SHAPE;

    if(face_maker && !base.hasSubShape(TopAbs_FACE)) {
        if(!base.hasSubShape(TopAbs_WIRE))
            base = base.makEWires();
        base = base.makEFace(0,face_maker);
    }
    BRepPrimAPI_MakeRevol mkRevol(base.getShape(), axis,d);
    return makEShape(mkRevol,base,op,appendTag);
}

TopoShape &TopoShape::makEMirror(const TopoShape &shape, const gp_Ax2 &ax2, const char *op, bool appendTag) {
    if(!op) op = TOPOP_MIRROR;
    _Shape.Nullify();
    resetElementMap();

    if(shape.isNull()) 
        HANDLE_NULL_SHAPE;

    gp_Trsf mat;
    mat.SetMirror(ax2);
    TopLoc_Location loc = shape.getShape().Location();
    gp_Trsf placement = loc.Transformation();
    mat = placement * mat;
    BRepBuilderAPI_Transform mkTrf(shape.getShape(), mat);
    return makEShape(mkTrf,shape,op,appendTag);
}

struct MapperSewing: Part::TopoShape::Mapper {
    BRepBuilderAPI_Sewing &maker;
    MapperSewing(BRepBuilderAPI_Sewing &maker)
        :maker(maker)
    {}
    virtual std::vector<TopoDS_Shape> modified(const TopoDS_Shape &s) const override {
        std::vector<TopoDS_Shape> ret;
        auto shape = maker.Modified(s);
        if(!shape.IsNull() && !shape.IsSame(s))
            ret.push_back(shape);
        auto sshape = maker.ModifiedSubShape(s);
        if(!sshape.IsNull() && !shape.IsSame(s) && !sshape.IsSame(shape))
            ret.push_back(sshape);
        return ret;
    }
};

TopoShape &TopoShape::makEShape(BRepBuilderAPI_Sewing &mk, const std::vector<TopoShape> &shapes,
        const char *op, bool appendTag)
{
    if(!op) op = TOPOP_SEWING;
    return makESHAPE(mk.SewedShape(),MapperSewing(mk),shapes,op,appendTag);
}

TopoShape &TopoShape::makEShape(BRepBuilderAPI_Sewing &mkShape,
            const TopoShape &source, const char *op, bool appendTag)
{
    if(!op) op = TOPOP_SEWING;
    return makEShape(mkShape,std::vector<TopoShape>(1,source),op,appendTag);
}

TopoShape &TopoShape::makEOffset(const TopoShape &shape, 
        double offset, double tol, bool intersection, bool selfInter, 
        short offsetMode, short join, bool fill, const char *op, bool appendTag)
{
    if(!op) op = TOPOP_OFFSET;
    _Shape.Nullify();
    resetElementMap();

#if OCC_VERSION_HEX < 0x070200
    BRepOffsetAPI_MakeOffsetShape mkOffset(shape.getShape(), offset, tol, BRepOffset_Mode(offsetMode),
        intersection ? Standard_True : Standard_False,
        selfInter ? Standard_True : Standard_False,
        GeomAbs_JoinType(join));
#else
    BRepOffsetAPI_MakeOffsetShape mkOffset;
    mkOffset.PerformByJoin(shape.getShape(), offset, tol, BRepOffset_Mode(offsetMode),
                           intersection ? Standard_True : Standard_False,
                           selfInter ? Standard_True : Standard_False,
                           GeomAbs_JoinType(join));
#endif

    if (!mkOffset.IsDone())
        Standard_Failure::Raise("BRepOffsetAPI_MakeOffsetShape not done");

    TopoShape res(Tag,Hasher);
    res.makEShape(mkOffset,shape,op,appendTag);
    if(shape.hasSubShape(TopAbs_SOLID) && !res.hasSubShape(TopAbs_SOLID)) {
        try {
            res = res.makESolid();
        }catch (Standard_Failure &e) {
            FC_WARN("failed to make solid: " << e.GetMessageString());
        }
    }
    if (!fill) {
        *this = res;
        return *this;
    }

    //get perimeter wire of original shape.
    //Wires returned seem to have edges in connection order.
    ShapeAnalysis_FreeBoundsProperties freeCheck(shape.getShape());
    freeCheck.Perform();
    if (freeCheck.NbClosedFreeBounds() < 1)
    {
        Standard_Failure::Raise("no closed bounds");
    }

    BRep_Builder builder;
    std::vector<TopoShape> shapes;
    for (int index = 1; index <= freeCheck.NbClosedFreeBounds(); ++index)
    {
        TopoShape originalWire(shape.Tag,shape.Hasher,freeCheck.ClosedFreeBound(index)->FreeBound());
        originalWire.mapSubElement(TopAbs_EDGE,shape);
        const BRepAlgo_Image& img = mkOffset.MakeOffset().OffsetEdgesFromShapes();

        //build offset wire.
        TopoDS_Wire offsetWire;
        builder.MakeWire(offsetWire);
        TopExp_Explorer xp;
        for (xp.Init(originalWire.getShape(), TopAbs_EDGE); xp.More(); xp.Next())
        {
            if (!img.HasImage(xp.Current()))
            {
                Standard_Failure::Raise("no image for shape");
            }
            const TopTools_ListOfShape& currentImage = img.Image(xp.Current());
            TopTools_ListIteratorOfListOfShape listIt;
            int edgeCount(0);
            TopoDS_Edge mappedEdge;
            for (listIt.Initialize(currentImage); listIt.More(); listIt.Next())
            {
                if (listIt.Value().ShapeType() != TopAbs_EDGE)
                    continue;
                edgeCount++;
                mappedEdge = TopoDS::Edge(listIt.Value());
            }

            if (edgeCount != 1)
            {
                std::ostringstream stream;
                stream << "wrong edge count: " << edgeCount << std::endl;
                Standard_Failure::Raise(stream.str().c_str());
            }
            builder.Add(offsetWire, mappedEdge);
        }
        std::vector<TopoShape> wires;
        wires.push_back(originalWire);
        wires.push_back(TopoShape(Tag,Hasher,offsetWire));
        wires.back().mapSubElement(TopAbs_EDGE,res);

        //It would be nice if we could get thruSections to build planar faces
        //in all areas possible, so we could run through refine. I tried setting
        //ruled to standard_true, but that didn't have the desired affect.
        BRepOffsetAPI_ThruSections aGenerator;
        aGenerator.AddWire(TopoDS::Wire(originalWire.getShape()));
        aGenerator.AddWire(offsetWire);
        aGenerator.Build();
        if (!aGenerator.IsDone())
        {
            Standard_Failure::Raise("ThruSections failed");
        }

        shapes.push_back(TopoShape(Tag,Hasher).makEShape(aGenerator,wires));
    }

    TopoShape perimeterCompound(Tag,Hasher);
    perimeterCompound.makECompound(shapes,false,op);

    //still had to sew. not using the passed in parameter for sew.
    //Sew has it's own default tolerance. Opinions?
    BRepBuilderAPI_Sewing sewTool;
    sewTool.Add(shape.getShape());
    sewTool.Add(perimeterCompound.getShape());
    sewTool.Add(res.getShape());
    sewTool.Perform(); //Perform Sewing

    TopoDS_Shape outputShape = sewTool.SewedShape();
    if ((outputShape.ShapeType() == TopAbs_SHELL) && (outputShape.Closed()))
    {
        BRepBuilderAPI_MakeSolid solidMaker(TopoDS::Shell(outputShape));
        if (solidMaker.IsDone())
        {
            TopoDS_Solid temp = solidMaker.Solid();
            //contrary to the occ docs the return value OrientCloseSolid doesn't
            //indicate whether the shell was open or not. It returns true with an
            //open shell and we end up with an invalid solid.
            if (BRepLib::OrientClosedSolid(temp))
                outputShape = temp;
        }
    }

    shapes.clear();
    shapes.push_back(shape);
    shapes.push_back(res);
    shapes.push_back(perimeterCompound);
    *this = TopoShape(Tag,Hasher).makESHAPE(outputShape,MapperSewing(sewTool),shapes,op,false);
    return *this;
}

TopoShape &TopoShape::makEOffset2D(const TopoShape &shape, double offset, short joinType, 
        bool fill, bool allowOpenResult, bool intersection, const char *op, bool appendTag)
{
    if(!op) op = TOPOP_OFFSET2D;
    _Shape.Nullify();
    resetElementMap();

    if(shape.isNull())
        throw Base::ValueError("makeOffset2D: input shape is null!");
    if (allowOpenResult && OCC_VERSION_HEX < 0x060900)
        throw Base::AttributeError("openResult argument is not supported on OCC < 6.9.0.");

    // OUTLINE OF MAKEOFFSET2D
    // * Prepare shapes to process
    // ** if _Shape is a compound, recursively call this routine for all subcompounds
    // ** if intrsection, dump all non-compound children into shapes to process; otherwise call this routine recursively for all children
    // ** if _shape isn't a compound, dump it straight to shapes to process
    // * Test for shape types, and convert them all to wires
    // * find plane
    // * OCC call (BRepBuilderAPI_MakeOffset)
    // * postprocessing (facemaking):
    // ** convert offset result back to faces, if inputs were faces
    // ** OR do offset filling:
    // *** for closed wires, simply feed source wires + offset wires to smart facemaker
    // *** for open wires, try to connect source anf offset result by creating new edges (incomplete implementation)
    // ** actual call to FaceMakerBullseye, unified for all facemaking.

    std::vector<TopoShape> shapesToProcess;
    std::vector<TopoShape> shapesToReturn;
    bool forceOutputCompound = false;

    if (shape.getShape().ShapeType() == TopAbs_COMPOUND){
        if (!intersection){
            //simply recursively process the children, independently
            expandCompound(shape,shapesToProcess);
            forceOutputCompound = true;
        } else {
            //collect non-compounds from this compound for collective offset. Process other shapes independently.
            for(auto &s : shape.getSubTopoShapes()) {
                if(s.getShape().ShapeType() == TopAbs_COMPOUND){
                    //recursively process subcompounds
                    shapesToReturn.push_back(TopoShape(Tag,Hasher).makEOffset2D(
                                s, offset, joinType, fill, allowOpenResult, intersection, op, appendTag));
                    forceOutputCompound = true;
                } else {
                    shapesToProcess.push_back(s);
                }
            }
        }
    } else {
        shapesToProcess.push_back(shape);
    }

    if(shapesToProcess.size() > 0){
        TopoShape res(Tag,Hasher);

        //although 2d offset supports offsetting a face directly, it seems there is
        //no way to do a collective offset of multiple faces. So, we are doing it
        //by getting all wires from the faces, and applying offsets to them, and
        //reassembling the faces later.
        std::vector<TopoShape> sourceWires;
        bool haveWires = false;
        bool haveFaces = false;
        for(auto &s : shapesToProcess){
            const auto &sh = s.getShape();
            switch (sh.ShapeType()) {
                case TopAbs_EDGE:
                    sourceWires.push_back(s.makEWires());
                    haveWires = true;
                    break;
                case TopAbs_WIRE:
                    sourceWires.push_back(s);
                    haveWires = true;
                    break;
                case TopAbs_FACE:{
                    const auto &wires = s.getSubTopoShapes(TopAbs_WIRE);
                    sourceWires.insert(sourceWires.end(),wires.begin(),wires.end());
                    haveFaces = true;
                }break;
                default:
                    throw Base::TypeError("makeOffset2D: input shape is not an edge, wire or face or compound of those.");
                break;
            }
        }
        if (haveWires && haveFaces)
            throw Base::TypeError("makeOffset2D: collective offset of a mix of wires and faces is not supported");
        if (haveFaces)
            allowOpenResult = false;

        //find plane.
        gp_Pln workingPlane;
        TopoDS_Compound compoundSourceWires;
        {
            BRep_Builder builder;
            builder.MakeCompound(compoundSourceWires);
            for(auto &w : sourceWires)
                builder.Add(compoundSourceWires, w.getShape());
            BRepLib_FindSurface planefinder(compoundSourceWires, -1, Standard_True);
            if (!planefinder.Found())
                throw Base::Exception("makeOffset2D: wires are nonplanar or noncoplanar");
            if (haveFaces){
                //extract plane from first face (useful for preserving the plane of face precisely if dealing with only one face)
                workingPlane = BRepAdaptor_Surface(TopoDS::Face(shapesToProcess[0].getShape())).Plane();
            } else {
                workingPlane = GeomAdaptor_Surface(planefinder.Surface()).Plane();
            }
        }

        //do the offset..
        TopoShape offsetShape;
        BRepOffsetAPI_MakeOffset mkOffset(TopoDS::Wire(sourceWires[0].getShape()), GeomAbs_JoinType(joinType)
#if OCC_VERSION_HEX >= 0x060900
                                                , allowOpenResult
#endif
                                               );
        for(auto &w : sourceWires)
            if (&w != &(sourceWires[0])) //filter out first wire - it's already added
                mkOffset.AddWire(TopoDS::Wire(w.getShape()));

        if (fabs(offset) > Precision::Confusion()){
            try {
    #if defined(__GNUC__) && defined (FC_OS_LINUX)
                Base::SignalException se;
    #endif
                mkOffset.Perform(offset);
            }
            catch (Standard_Failure &){
                throw;
            }
            catch (...) {
                throw Base::Exception("BRepOffsetAPI_MakeOffset has crashed! (Unknown exception caught)");
            }
            //Copying shape to fix strange orientation behavior, OCC7.0.0. See bug #2699
            // http://www.freecadweb.org/tracker/view.php?id=2699
            offsetShape = shape.makEShape(mkOffset,op,appendTag).makECopy();

            if(offsetShape.isNull())
                throw Base::Exception("makeOffset2D: result of offsetting is null!");
        } else {
            offsetShape = TopoShape(Tag,Hasher).makECompound(sourceWires,false,0,false);
        }

        std::vector<TopoShape> offsetWires;
        //interestingly, if wires are removed, empty compounds are returned by MakeOffset (as of OCC 7.0.0)
        //so, we just extract all nesting
        expandCompound(offsetShape,offsetWires);
        if (offsetWires.empty())
            throw Base::Exception("makeOffset2D: offset result has no wires.");

        std::vector<TopoShape> wiresForMakingFaces;
        if (!fill){
            if (haveFaces){
                wiresForMakingFaces.insert(wiresForMakingFaces.end(), offsetWires.begin(),offsetWires.end());
            } else {
                shapesToReturn.insert(shapesToReturn.end(),offsetWires.begin(),offsetWires.end());
            }
        } else {
            //fill offset
            if (fabs(offset) < Precision::Confusion())
                throw Base::ValueError("makeOffset2D: offset distance is zero. Can't fill offset.");

            //filling offset. There are three major cases to consider:
            // 1. source wires and result wires are closed (simplest) -> make face
            // from source wire + offset wire
            //
            // 2. source wire is open, but offset wire is closed (if not
            // allowOpenResult). -> throw away source wire and make face right from
            // offset result.
            //
            // 3. source and offset wire are both open (note that there may be
            // closed islands in offset result) -> need connecting offset result to
            // source wire with new edges

            //first, lets split apart closed and open wires.
            std::vector<TopoShape> closedWires;
            std::vector<TopoShape> openWires;
            for(auto &w : sourceWires)
                if (BRep_Tool::IsClosed(TopoDS::Wire(w.getShape())))
                    closedWires.push_back(w);
                else
                    openWires.push_back(w);
            for(auto &w : offsetWires)
                if (BRep_Tool::IsClosed(TopoDS::Wire(w.getShape())))
                    closedWires.push_back(w);
                else
                    openWires.push_back(w);

            wiresForMakingFaces.insert(wiresForMakingFaces.end(),closedWires.begin(),closedWires.end());
            if (!allowOpenResult || openWires.size() == 0){
                //just ignore all open wires
            } else {
                //We need to connect open wires to form closed wires.

                //for now, only support offsetting one open wire -> there should be exactly two open wires for connecting
                if (openWires.size() != 2)
                    throw Base::Exception("makeOffset2D: collective offset with filling of multiple wires is not supported yet.");

                TopoShape openWire1 = openWires.front();
                TopoShape openWire2 = openWires.back();

                //find open vertices
                BRepTools_WireExplorer xp;
                xp.Init(TopoDS::Wire(openWire1.getShape()));
                TopoDS_Vertex v1 = xp.CurrentVertex();
                for(;xp.More();xp.Next()){};
                TopoDS_Vertex v2 = xp.CurrentVertex();

                //find open vertices
                xp.Init(TopoDS::Wire(openWire2.getShape()));
                TopoDS_Vertex v3 = xp.CurrentVertex();
                for(;xp.More();xp.Next()){};
                TopoDS_Vertex v4 = xp.CurrentVertex();

                //check
                if (v1.IsNull())  throw Base::Exception("v1 is null");
                if (v2.IsNull())  throw Base::Exception("v2 is null");
                if (v3.IsNull())  throw Base::Exception("v3 is null");
                if (v4.IsNull())  throw Base::Exception("v4 is null");

                //assemble new wire

                //we want the connection order to be
                //v1 -> openWire1 -> v2 -> (new edge) -> v4 -> openWire2(rev) -> v3 -> (new edge) -> v1
                //let's check if it's the case. If not, we reverse one wire and swap its endpoints.

                if (fabs(gp_Vec(BRep_Tool::Pnt(v2), BRep_Tool::Pnt(v3)).Magnitude() - fabs(offset)) <= BRep_Tool::Tolerance(v2) + BRep_Tool::Tolerance(v3)){
                    openWire2._Shape.Reverse();
                    std::swap(v3, v4);
                    v3.Reverse();
                    v4.Reverse();
                } else if ((fabs(gp_Vec(BRep_Tool::Pnt(v2), BRep_Tool::Pnt(v4)).Magnitude() - fabs(offset)) <= BRep_Tool::Tolerance(v2) + BRep_Tool::Tolerance(v4))){
                    //orientation is as expected, nothing to do
                } else {
                    throw Base::Exception("makeOffset2D: fill offset: failed to establish open vertex relationship.");
                }

                //now directions of open wires are aligned. Finally. make new wire!
                BRepBuilderAPI_MakeWire mkWire;
                //add openWire1
                BRepTools_WireExplorer it;
                for(it.Init(TopoDS::Wire(openWire1.getShape())); it.More(); it.Next()){
                    mkWire.Add(it.Current());
                }
                //add first joining edge
                mkWire.Add(BRepBuilderAPI_MakeEdge(v2,v4).Edge());
                //add openWire2, in reverse order
                openWire2._Shape.Reverse();
                for(it.Init(TopoDS::Wire(openWire2.getShape())); it.More(); it.Next()){
                    mkWire.Add(it.Current());
                }
                //add final joining edge
                mkWire.Add(BRepBuilderAPI_MakeEdge(v3,v1).Edge());

                mkWire.Build();

                wiresForMakingFaces.push_back(TopoShape(Tag,Hasher).makEShape(mkWire,openWires,op));
            }
        }

        //make faces
        if (wiresForMakingFaces.size()>0)
            expandCompound(TopoShape(Tag,Hasher).makEFace(wiresForMakingFaces,op),shapesToReturn);
    }

    return makECompound(shapesToReturn,false,op,forceOutputCompound);
}

TopoShape &TopoShape::makEThickSolid(const TopoShape &shape, 
        const std::vector<TopoShape> &faces,
        double offset, double tol, bool intersection,
        bool selfInter, short offsetMode, short join,
        const char *op, bool appendTag)
{
    if(!op) op = TOPOP_THICKEN;

    _Shape.Nullify();
    resetElementMap();

    if(shape.isNull())
        HANDLE_NULL_SHAPE;

    if(faces.empty())
        HANDLE_NULL_INPUT;

    if(fabs(offset) <= 2*tol) {
        *this = shape;
        return *this;
    }

    TopTools_IndexedMapOfShape fmap;
    TopExp::MapShapes(shape.getShape(), TopAbs_FACE, fmap);
    TopTools_ListOfShape remFace;
    for(auto &face : faces) {
        if(face.isNull())
            HANDLE_NULL_INPUT;
        if(!fmap.FindIndex(face.getShape()))
            Standard_Failure::Raise("face does not belong to the shape");
        remFace.Append(face.getShape());
    }
#if OCC_VERSION_HEX < 0x070200
    BRepOffsetAPI_MakeThickSolid mkThick(shape.getShape(), remFace, offset, tol, BRepOffset_Mode(offsetMode),
        intersection ? Standard_True : Standard_False,
        selfInter ? Standard_True : Standard_False,
        GeomAbs_JoinType(join));
#else
    BRepOffsetAPI_MakeThickSolid mkThick;
    mkThick.MakeThickSolidByJoin(shape.getShape(), remFace, offset, tol, BRepOffset_Mode(offsetMode),
        intersection ? Standard_True : Standard_False,
        selfInter ? Standard_True : Standard_False,
        GeomAbs_JoinType(join));
#endif
    return makEShape(mkThick,shape,op,appendTag);
}

TopoShape &TopoShape::makEWires(const std::vector<TopoShape> &shapes, const char *op, bool fix, double tol) {
    if(shapes.empty())
        HANDLE_NULL_SHAPE;
    if(shapes.size() == 1)
        return makEWires(shapes[0],op);
    bool appendTag = false;
    long tag = 0;
    for(auto &s : shapes) {
        if(!s.Tag || s.Tag==tag) continue;
        if(tag) {
            appendTag = true;
            break;
        }
        tag = s.Tag;
    }
    return makEWires(TopoShape(Tag).makECompound(shapes,appendTag),op,fix,tol);
}

TopoShape &TopoShape::makEWires(const TopoShape &shape, const char *op, bool fix, double tol) 
{
    _Shape.Nullify();
    resetElementMap();

    if(!op) op = TOPOP_WIRE;
    if(tol<Precision::Confusion()) tol = Precision::Confusion();

    // Can't user ShapeAnalysis_FreeBounds. It seems the output edges are
    // modified some how, and it is not obvious how to map the resulting edges.
#if 0

    Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
    Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
    for(TopExp_Explorer xp(shape.getShape(),TopAbs_EDGE);xp.More();xp.Next())
        hEdges->Append(xp.Current());
    if(!hEdges->Length())
        HANDLE_NULL_SHAPE;
    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges, tol, Standard_False, hWires);
    if(!hWires->Length())
        HANDLE_NULL_SHAPE;

    std::vector<TopoShape> wires;
    for (int i=1; i<=hWires->Length(); i++) {
        auto wire = hWires->Value(i);
        if(fix) {
            // Fix any topological issues of the wire
            ShapeFix_Wire aFix;
            aFix.SetPrecision(Precision::Confusion());
            aFix.Load(TopoDS::Wire(wire));
            aFix.FixReorder();
            aFix.FixConnected();
            aFix.FixClosed();
            wire = aFix.Wire();
        }
        wires.push_back(TopoShape(Tag,Hasher,wire));
    }
    shape.mapSubElementsTo(TopAbs_EDGE,wires);
    return makECompound(wires,false,op,false);
#else
    (void)fix;
    std::vector<TopoShape> edges;
    std::list<TopoShape> edge_list;
    std::vector<TopoShape> wires;

    for(auto &e : shape.getSubTopoShapes(TopAbs_EDGE))
        edge_list.push_back(e);

    edges.reserve(edge_list.size());
    wires.reserve(edge_list.size());

    // sort them together to wires
    while (edge_list.size() > 0) {
        BRepBuilderAPI_MakeWire mkWire;
        // add and erase first edge
        edges.push_back(edge_list.front());
        edge_list.pop_front();
        mkWire.Add(TopoDS::Edge(edges.back().getShape()));
        edges.back().setShape(mkWire.Edge(),false);

        TopoDS_Wire new_wire = mkWire.Wire(); // current new wire

        // try to connect each edge to the wire, the wire is complete if no more edges are connectible
        bool found = false;
        do {
            found = false;
            for (auto it=edge_list.begin();it!=edge_list.end();++it) {
                mkWire.Add(TopoDS::Edge(it->getShape()));
                if (mkWire.Error() != BRepBuilderAPI_DisconnectedWire) {
                    // edge added ==> remove it from list
                    found = true;
                    edges.push_back(*it);
                    edges.back().setShape(mkWire.Edge(),false);
                    edge_list.erase(it);
                    new_wire = mkWire.Wire();
                    break;
                }
            }
        } while (found);

        wires.push_back(new_wire);
        wires.back().mapSubElement(TopAbs_EDGE,edges,op);

        // Fix any topological issues of the wire
        ShapeFix_Wire aFix;
        aFix.SetPrecision(tol);
        aFix.Load(new_wire);
        aFix.FixReorder();
        aFix.FixConnected();
        aFix.FixClosed();
        wires.back().setShape(aFix.Wire(),false);
    }
    return makECompound(wires,false,0,false);
#endif
}

TopoShape &TopoShape::makEFace(const TopoShape &shape, const char *op, const char *maker)
{
    std::vector<TopoShape> shapes;
    if(shape.getShape().ShapeType() == TopAbs_COMPOUND)
        shapes = shape.getSubTopoShapes();
    else
        shapes.push_back(shape);
    return makEFace(shapes,op,maker);
}

TopoShape &TopoShape::makEFace(const std::vector<TopoShape> &shapes, const char *op, const char *maker)
{
    _Shape.Nullify();
    resetElementMap();

    if(!maker || !maker[0]) maker = "Part::FaceMakerBullseye";
    std::unique_ptr<FaceMaker> mkFace = FaceMaker::ConstructFromType(maker);
    mkFace->MyHasher = Hasher;
    mkFace->MyOp = op;

    for(auto &s : shapes) {
        if (s.getShape().ShapeType() == TopAbs_COMPOUND)
            mkFace->useTopoCompound(s);
        else
            mkFace->addTopoShape(s);
    }
    mkFace->Build();
    const auto &ret = mkFace->getTopoShape();
    _Shape = ret._Shape;
    _ElementMap = ret._ElementMap;
    Hasher = ret.Hasher;
    return *this;
}

TopoShape &TopoShape::makERefine(const TopoShape &shape, const char *op, bool no_fail) {
    _Shape.Nullify();
    resetElementMap();
    if(shape.isNull()) {
        if(!no_fail) 
            HANDLE_NULL_SHAPE;
        return *this;
    }
    if(!op) op = TOPOP_REFINE;
    try {
        BRepBuilderAPI_RefineModel mkRefine(shape.getShape());
        return makEShape(mkRefine,shape,op);
    }catch (Standard_Failure &) {
        if(!no_fail) throw;
    }
    *this = shape;
    return *this;
}

TopoShape &TopoShape::makEShape(const char *maker, 
        const TopoShape &shape, const char *op, bool appendTag, double tol)
{
    return makEShape(maker,std::vector<TopoShape>(1,shape),op,appendTag,tol);
}

TopoShape &TopoShape::makEShape(const char *maker, 
        const std::vector<TopoShape> &_shapes, const char *op, bool appendTag, double tol)
{
    if(!maker)
        Standard_Failure::Raise("no maker");

    if(!op) op = maker;
    _Shape.Nullify();
    resetElementMap();

    std::vector<TopoShape> shapes;
    for(auto &s : _shapes)
        expandCompound(s,shapes);

    if(shapes.empty())
        HANDLE_NULL_SHAPE;

    if(strcmp(maker,TOPOP_COMPOUND)==0) {
        return makECompound(shapes,appendTag,op,false);
    } else if(strcmp(maker,TOPOP_REFINE)==0) {
        for(auto &s : shapes)
            s = s.makERefine(op);
        return makECompound(shapes,appendTag,op,false);
    } else if(boost::starts_with(maker,TOPOP_FACE)) {
        std::string prefix(TOPOP_FACE);
        prefix += '.';
        const char *face_maker = 0;
        if(boost::starts_with(maker,prefix))
            face_maker = maker+prefix.size();
        return makEFace(shapes,op,face_maker);
    } else if(strcmp(maker, TOPOP_WIRE)==0) 
        return makEWires(shapes,op);
    else if(strcmp(maker, TOPOP_COMPSOLID)==0) {
        BRep_Builder builder;
        TopoDS_CompSolid Comp;
        builder.MakeCompSolid(Comp);
        for(auto &s : shapes) {
            if (!s.isNull())
                builder.Add(Comp, s.getShape());
        }
        _Shape = Comp;
        mapSubElement(TopAbs_FACE,shapes,op,true,appendTag);
        return *this;
    }

    if(shapes.size() == 1) {
        *this = shapes[0];
        return *this;
    }

    if(strcmp(maker,TOPOP_PIPE)==0) {
        if(shapes.size()!=2)
            Standard_Failure::Raise("Not enough input shapes");
        if (shapes[0].isNull() || shapes[1].isNull())
            Standard_Failure::Raise("Cannot sweep along empty spine");
        if (shapes[0].getShape().ShapeType() != TopAbs_WIRE)
            Standard_Failure::Raise("Spine shape is not a wire");
        BRepOffsetAPI_MakePipe mkPipe(TopoDS::Wire(shapes[0].getShape()), shapes[1].getShape());
        return makEShape(mkPipe,shapes,op,appendTag);
    }

    if(strcmp(maker,TOPOP_SHELL)==0) {
        BRep_Builder builder;
        TopoDS_Shell shell;
        builder.MakeShell(shell);
        for(auto &s : shapes)
            builder.Add(shell,s.getShape());
        _Shape = shell;
        mapSubElement(TopAbs_FACE,shapes,op,true,appendTag);
        BRepCheck_Analyzer check(shell);
        if (!check.IsValid()) {
            ShapeUpgrade_ShellSewing sewShell;
            _Shape = sewShell.ApplySewing(shell);
            // TODO confirm the above won't change OCCT topological naming
        }
        return *this;
    }

    std::unique_ptr<BRepAlgoAPI_BooleanOperation> mk;
    if(strcmp(maker, TOPOP_FUSE)==0) 
        mk.reset(new BRepAlgoAPI_Fuse);
    else if(strcmp(maker, TOPOP_CUT)==0) 
        mk.reset(new BRepAlgoAPI_Cut);
    else if(strcmp(maker, TOPOP_COMMON)==0)
        mk.reset(new BRepAlgoAPI_Common);
    else if(strcmp(maker, TOPOP_SECTION)==0)
        mk.reset(new BRepAlgoAPI_Section);
    else
        Standard_Failure::Raise("Unknown maker");
        
# if OCC_VERSION_HEX >= 0x060900
    mk->SetRunParallel(true);
# endif
#if OCC_VERSION_HEX >= 0x070000
    mk->SetNonDestructive(Standard_True);
#endif
    TopTools_ListOfShape shapeArguments,shapeTools;
    shapeArguments.Append(shapes.front().getShape());
    for(size_t i=1;i<shapes.size();++i) {
        TopoShape &shape = shapes[i];
        if (tol > 0.0) {
            // workaround for http://dev.opencascade.org/index.php?q=node/1056#comment-520
            shape = TopoShape(shape.Tag).makECopy(shape);
        }
        shapeTools.Append(shape.getShape());
    }
    mk->SetArguments(shapeArguments);
    mk->SetTools(shapeTools);
    if (tol > 0.0)
        mk->SetFuzzyValue(tol);
    mk->Build();
    return makEShape(*mk,shapes,op,appendTag);
}

TopoShape &TopoShape::makEShape(BRepBuilderAPI_MakeShape &mkShape, 
        const TopoShape &source, const char *op, bool appendTag) 
{
    std::vector<TopoShape> sources;
    sources.emplace_back(source);
    return makEShape(mkShape,sources,op,appendTag);
}

struct ShapeInfo {
    TopTools_IndexedMapOfShape shapeMap;
    std::vector<TopTools_IndexedMapOfShape> otherMaps;
    const char *shapetype;
    TopAbs_ShapeEnum type;

    ShapeInfo(TopAbs_ShapeEnum type, TopoShape &shape, 
            const std::vector<TopoShape> &others, const char *op, bool appendTag) 
    {
        this->type = type;
        shapeMap.Clear();
        TopExp::MapShapes(shape.getShape(), type, shapeMap);
        otherMaps.clear();
        otherMaps.resize(others.size());
        size_t i = 0;
        for(auto &other : others) {
            auto &otherMap = otherMaps[i++];
            TopExp::MapShapes(other.getShape(), type, otherMap);
            shape.mapSubElement(type,shapeMap,other,otherMap,op,false,appendTag);
        }
        shapetype = TopoShape::shapeName(type).c_str();
    }
};

TopoShape &TopoShape::makEShape(BRepBuilderAPI_MakeShape &mkShape, 
        const std::vector<TopoShape> &shapes, const char *op, bool appendTag)
{
    return makESHAPE(mkShape.Shape(),MapperMaker(mkShape),shapes,op,appendTag);
}

static size_t findTag(const std::string &name, long *tag=0, size_t *len=0) {
    size_t pos = name.rfind(TopoShape::tagPostfix());
    if(pos==std::string::npos)
        return pos;
    size_t offset = pos + TopoShape::tagPostfix().size();
    std::istringstream iss(name.c_str()+offset);
    long _tag = -1;
    int _len = -1;
    char sep = 0;
    iss >> _tag >>  sep >> _len;
    if(_tag<0 || _len<0 || sep!=':' || !iss.eof())
        return std::string::npos;
    if(tag)
        *tag = _tag;
    if(len)
        *len = (size_t)_len;
    return pos;
}

// try to hash element name while preserving the source tag
void TopoShape::processName(std::string &name, std::ostringstream &ss, 
        std::vector<App::StringIDRef> &sids, const char* op, bool appendTag, 
        long tag, bool preserveTag)
{
    if(op) {
        if(ss.tellp())
            ss << elementMapPrefix();
        ss << op;
    }
    long inputTag = 0;
    if(appendTag && tag && tag!=Tag)
        inputTag = tag;
    else if(preserveTag) {
        if(!ss.tellp()) {
            ss << name;
            name.clear();
            return;
        }
        findTag(name.c_str(),&inputTag);
    }
    if(Hasher)
        name = hashElementName(name.c_str(),sids);
    if(inputTag)
        ss << tagPostfix() << inputTag << ':' << name.size();
}

// Extract tag from a hashed element name and de-hash the name
long TopoShape::getElementHistory(const std::string &name, 
        std::string *original, std::vector<std::string> *history) const 
{
    long tag = 0;
    size_t len = 0;
    auto pos = findTag(name,&tag,&len);
    if(pos==std::string::npos || (!original&&!history))
        return tag;

    std::string tmp;
    std::string &ret = original?*original:tmp;
    bool first = true;
    while(1) {
        if(!len || len>=pos) {
            FC_WARN("invalid name length " << name);
            return 0;
        }
        if(first) {
            first = false;
            ret = name.substr(0,len);
        }else
            ret = ret.substr(0,len);
        if(!Hasher) 
            return tag;
        ret = dehashElementName(ret.c_str());
        long tag2 = 0;
        pos = findTag(ret,&tag2,&len);
        if(pos==std::string::npos || tag2!=tag)
            return tag;
        if(history)
            history->push_back(ret);
    }
}

struct NameKey {
    std::string name;
    long tag;

    NameKey(const char *n, long t=0)
        :name(n),tag(t)
    {}
    NameKey(const std::string &n, long t=0)
        :name(n),tag(t)
    {}
    bool operator<(const NameKey &other) const {
        return tag<other.tag || (tag==other.tag && name<other.name);
    }
};

struct NameInfo {
    int index;
    std::vector<App::StringIDRef> sids;
};

TopoShape &TopoShape::makESHAPE(const TopoDS_Shape &shape, const Mapper &mapper, 
        const std::vector<TopoShape> &shapes, const char *op, bool appendTag)
{
    resetElementMap();
    _Shape = shape;
    if(shape.IsNull())
        HANDLE_NULL_SHAPE;

    if(shapes.empty())
        return *this;

    if(!op) op = TOPOP_MAKER;
    std::string _op = op;
    _op += '_';

    const char *mapOp = appendTag?op:0;
    ShapeInfo vinfo(TopAbs_VERTEX,*this,shapes,mapOp,appendTag);
    ShapeInfo einfo(TopAbs_EDGE,*this,shapes,mapOp,appendTag);
    ShapeInfo finfo(TopAbs_FACE,*this,shapes,mapOp,appendTag);

    std::array<ShapeInfo*,3> infos = {&vinfo,&einfo,&finfo};

    std::map<TopAbs_ShapeEnum,ShapeInfo*> infoMap;
    infoMap[TopAbs_VERTEX] = &vinfo;
    infoMap[TopAbs_EDGE] = &einfo;
    infoMap[TopAbs_WIRE] = &einfo;
    infoMap[TopAbs_FACE] = &finfo;
    infoMap[TopAbs_SHELL] = &finfo;
    infoMap[TopAbs_SOLID] = &finfo;
    infoMap[TopAbs_COMPOUND] = &finfo;
    infoMap[TopAbs_COMPSOLID] = &finfo;

    std::ostringstream ss;
    std::string prefix,postfix,newName;

    std::map<std::string,std::map<NameKey,NameInfo> > newNames;

    // First, collect names from other shapes that generates or modifies the
    // new shape
    for(auto &pinfo : infos) {
        auto &info = *pinfo;
        if(!info.shapeMap.Extent())
            continue;

        for(size_t n=0;n<shapes.size();++n) {
            const auto &other = shapes[n];
            auto &otherMap = info.otherMaps[n];

            if(!otherMap.Extent() || !canMapElement(other))
                continue;

            for (int i=1; i<=otherMap.Extent(); i++) {
                const auto &otherElement = otherMap(i);
                // Find all new objects that are a modification of the old object
                ss.str("");
                ss << info.shapetype << i;
                std::vector<App::StringIDRef> sids;
                std::string name = other.getElementName(ss.str().c_str(),true,&sids);

                int k=0;
                for(auto &newShape : mapper.modified(otherElement)) {
                    ++k;
                    auto itMap = infoMap.find(newShape.ShapeType());
                    if(itMap==infoMap.end()) {
                        FC_ERR("unknown modified shape type " << newShape.ShapeType() 
                                << " from " << info.shapetype << i);
                        continue;
                    }
                    auto &newInfo = *itMap->second;
                    if(newInfo.type != newShape.ShapeType()) {
                        FC_WARN("modified shape type " << shapeName(newShape.ShapeType())
                                << " mismatch with " << info.shapetype << i);
                        continue;
                    }
                    int j = newInfo.shapeMap.FindIndex(newShape);
                    if(!j) {
                        // This warning occurs in makERevolve. It generates
                        // some shape from a vertex that never made into the
                        // final shape
                        if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                            FC_WARN("Cannot find " << op << " modified " <<
                                newInfo.shapetype << " from " << info.shapetype << i);
                        continue;
                    }
                    ss.str("");
                    ss << newInfo.shapetype << j;
                    std::string element = ss.str();
                    ss.str("");
                    if(newShape.ShapeType() == info.type)
                        ss << ' '; // make sure the same type shape appears first
                    ss << name;
                    auto &info = newNames[element][NameKey(ss.str(),other.Tag)];
                    info.sids = sids;
                    info.index = k;
                }

                // Find all new objects that were generated from an old object
                // (e.g. a face generated from an edge)
                k=0;
                for(auto &newShape : mapper.generated(otherElement)) {
                    auto itMap = infoMap.find(newShape.ShapeType());
                    if(itMap==infoMap.end()) {
                        FC_ERR("unknown generated shape type " << newShape.ShapeType() 
                                << " from " << info.shapetype << i);
                        continue;
                    }
                    auto &newInfo = *itMap->second;
                    std::vector<TopoDS_Shape> newShapes;
                    if(newInfo.type == newShape.ShapeType())
                        newShapes.push_back(newShape);
                    else {
                        // It is possible for the maker to report generating a
                        // higher level shape, such as shell or solid.
                        for(TopExp_Explorer xp(newShape,newInfo.type);xp.More();xp.Next())
                            newShapes.push_back(xp.Current());
                    }
                    for(auto &newShape : newShapes) {
                        ++k;
                        int j = newInfo.shapeMap.FindIndex(newShape);
                        if(!j) {
                            if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                                FC_WARN("Cannot find " << op << " generated " <<
                                        newInfo.shapetype << " from " << info.shapetype << i);
                            continue;
                        }
                        ss.str("");
                        ss << newInfo.shapetype << j;
                        auto &info = newNames[ss.str()][NameKey(name,other.Tag)];
                        info.sids = sids;
                        info.index = -k;
                    }
                }
            }
        }
    }

    // Second, construct the names for modification/generation info collected in
    // the previous step
    for(auto &v : newNames) {
        // We treat the first modified/generated source shape name specially.
        // If case there are more than one source shape. We hash the first
        // source name separately, and then obtain the second string id by
        // hashing all the source names together.  We then use the second
        // string id as the postfix for our name. 
        //
        // In this way, we can associate the same source that are modified by
        // multiple other shapes.

        auto &element = v.first;
        auto &names = v.second;
        const auto &first_key = names.begin()->first;
        auto &first_info = names.begin()->second;
        int name_type = first_info.index>0?1:2; // index>0 means modified, or else generated
        std::string first_name(first_key.name[0]==' '?first_key.name.c_str()+1:first_key.name.c_str());

        std::vector<App::StringIDRef> sids(first_info.sids);

        postfix.clear();
        if(names.size()>1) {
            ss.str("");
            ss << '(';
            bool first = true;
            for(auto it=names.begin();it!=names.end();++it) {
                if(it == names.begin()) continue; // skip the first name
                if(first)
                    first = false;
                else
                    ss << ',';
                auto &other_key = it->first;
                auto &other_info = it->second;
                if(appendTag && other_key.tag && other_key.tag!=Tag)
                    ss << other_key.tag << '_';
                if(other_key.name[0] == ' ')
                    ss << other_key.name.c_str()+1;
                else
                    ss << other_key.name;
                if(other_info.index!=1)
                    ss << elementMapPrefix() <<  other_info.index;
                if((name_type==1 && other_info.index<0) || 
                   (name_type==2 && other_info.index>0)) {
                    FC_WARN("element is both generated and modified");
                    name_type = 0;
                }
                sids.insert(sids.end(),other_info.sids.begin(),other_info.sids.end());
            }
            ss <<')';
            if(Hasher) {
                sids.push_back(Hasher->getID(ss.str().c_str()));
                ss.str("");
                ss << '#' << std::hex << sids.back()->value() << std::dec;
            }
            postfix = ss.str();
        }

        ss.str("");
        if(name_type==2)
            ss << genPostfix();
        else if(name_type==1)
            ss << modPostfix();
        else
            ss << modgenPostfix();
        if(abs(first_info.index)>1)
           ss << abs(first_info.index);
        ss << postfix;
        processName(first_name,ss,sids,op,appendTag,first_key.tag,true);
        setElementName(element.c_str(),first_name.c_str(),ss.str().c_str(),&sids);
    }

    // Now, the reverse pass. Starting from the highest level element, i.e.
    // Face, for any element that are named, assign names for its lower unamed
    // elements. For example, if Edge1 is named E1, and its vertexes are not
    // named, then name them as E1;U1, E1;U2, etc.
    //
    // In order to make the name as stable as possible, we may assign multiple
    // names (which must be sorted, because we may use the first one to name
    // upper element in the final pass) to lower element if it appears in
    // multiple higher elements, e.g. same edge in multiple faces.

    for(size_t ifo=infos.size()-1;ifo!=0;--ifo) {
        newNames.clear();
        auto &info = *infos[ifo];
        auto &next = *infos[ifo-1];
        for(int i=1;i<=info.shapeMap.Extent();++i) {
            ss.str("");
            ss << info.shapetype << i;
            std::string element = ss.str();
            std::vector<App::StringIDRef> sids;
            const char *mapped = getElementName(element.c_str(),true,&sids);
            if(mapped == element.c_str()) 
                continue;

            TopTools_IndexedMapOfShape submap;
            TopExp::MapShapes(info.shapeMap(i), next.type, submap);
            for(int j=1,n=1;j<=submap.Extent();++j) {
                ss.str("");
                int k = next.shapeMap.FindIndex(submap(j));
                assert(k);
                ss << next.shapetype << k;
                std::string element = ss.str();
                if(getElementName(element.c_str(),true) != element.c_str())
                    continue;
                auto &info = newNames[element][mapped];
                info.index = n++;
                info.sids = sids;
            }
        }
        // Assign the actual names
        for(auto &v : newNames) {
#ifndef FC_ELEMENT_MAP_ALL 
            // Do we really want multiple names for an element in this case?
            // If not, we just pick the name in the first sorting order here.
            auto &name = *v.second.begin();
#else
            for(auto &name : v.second) 
#endif
            {
                auto &info = name.second;
                auto &sids = info.sids;
                newName = name.first.name;
                ss.str("");
                ss << upperPostfix();
                if(info.index>1)
                    ss << info.index;
                processName(newName,ss,sids,op,appendTag,0,true);
                setElementName(v.first.c_str(),newName.c_str(),ss.str().c_str(),&sids);
            }
        }
    }

    // Finally, the forward pass. For any elements that are not named, try
    // construct its name from the lower elements
    for(size_t ifo=1;ifo<infos.size();++ifo) {
        auto &info = *infos[ifo];
        auto &prev = *infos[ifo-1];
        for(int i=1;i<=info.shapeMap.Extent();++i) {
            ss.str("");
            ss << info.shapetype << i;
            std::string element = ss.str();
            const char *name = getElementName(element.c_str(),true);
            if(name != element.c_str()) 
                continue;
            std::vector<App::StringIDRef> sids;
            std::map<std::string,std::string> names;
            TopExp_Explorer xp;
            if(info.type == TopAbs_FACE)
                xp.Init(ShapeAnalysis::OuterWire(TopoDS::Face(info.shapeMap(i))),TopAbs_EDGE);
            else
                xp.Init(info.shapeMap(i),prev.type);
            for(;xp.More();xp.Next()) {
                int j = prev.shapeMap.FindIndex(xp.Current());
                assert(j);
                ss.str("");
                ss << prev.shapetype << j;
                std::string element = ss.str();
                std::vector<App::StringIDRef> sid;
                name = getElementName(element.c_str(),true,&sid);
                if(name == element.c_str()) {
                    // only assign name if all lower elements are named
                    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                        FC_WARN("unnamed lower element " << element);
                    names.clear();
                    break;
                }
                auto res = names.emplace(name,element);
                if(res.second)
                    sids.insert(sids.end(),sid.begin(),sid.end());
                else if(element!=res.first->second) {
                    // The seam edge will appear twice, which is normal. We
                    // only warn if the mapped element names are different.
                    FC_WARN("lower element " << element << " and " <<
                            res.first->second << " has duplicated name " << name 
                            << " for " << info.shapetype << i );
                }
            }
            if(names.empty())
                continue;
            ss.str("");
            auto it = names.begin();
            newName = it->first;
            ss << lowerPostfix();
            bool first = true;
            for(++it;it!=names.end();++it) {
                if(first) {
                    ss << '(';
                    first = false;
                } else
                    ss << ',';
                ss << it->first;
            }
            if(!first)
                ss << ')';
            processName(newName,ss,sids,op,appendTag,0,true);
            setElementName(element.c_str(),newName.c_str(),ss.str().c_str(),&sids);
        }
    }
    return *this;
}

const std::string &TopoShape::modPostfix() {
    static std::string postfix(elementMapPrefix() + ":M");
    return postfix;
}

const std::string &TopoShape::modgenPostfix() {
    static std::string postfix(modPostfix() + "G");
    return postfix;
}

const std::string &TopoShape::genPostfix() {
    static std::string postfix(elementMapPrefix() + ":G");
    return postfix;
}

const std::string &TopoShape::upperPostfix() {
    static std::string postfix(elementMapPrefix() + ":U");
    return postfix;
}

const std::string &TopoShape::lowerPostfix() {
    static std::string postfix(elementMapPrefix() + ":L");
    return postfix;
}

const std::string &TopoShape::tagPostfix() {
    static std::string postfix(elementMapPrefix() + ":T");
    return postfix;
}

TopoShape &TopoShape::makESlice(const TopoShape &shape, 
        const Base::Vector3d& dir, double d, const char *op) 
{
    _Shape.Nullify();
    resetElementMap();
    if(shape.isNull())
        HANDLE_NULL_SHAPE;
    TopoCrossSection cs(dir.x, dir.y, dir.z,shape,op);
    TopoShape res = cs.slice(1,d);
    resetElementMap(res._ElementMap);
    _Shape = res._Shape;
    Hasher = res.Hasher;
    return *this;
}

TopoShape &TopoShape::makESlice(const TopoShape &shape, 
        const Base::Vector3d& dir, const std::vector<double> &d, const char *op)
{
    std::vector<TopoShape> wires;
    TopoCrossSection cs(dir.x, dir.y, dir.z, shape,op);
    int i=0;
    for(auto &dd : d) 
        cs.slice(++i,dd,wires);
    return makECompound(wires,false,op,false);
}

TopoShape &TopoShape::makEFilledFace(const std::vector<TopoShape> &_shapes,
        const TopoShape &surface, const char *op, bool appendTag)
{
    if(!op) op = TOPOP_FILLED_FACE;
    BRepOffsetAPI_MakeFilling maker;
    if (!surface.isNull() && surface.getShape().ShapeType() == TopAbs_FACE)
        maker.LoadInitSurface(TopoDS::Face(surface.getShape()));
    std::vector<TopoShape> shapes;
    for(auto &s : _shapes) 
        expandCompound(s,shapes);
    int count = 0;
    for(auto &s : shapes) {
        if(s.isNull()) continue;
        const auto &sh = s.getShape();
        if (sh.ShapeType() == TopAbs_EDGE) {
            maker.Add(TopoDS::Edge(sh), GeomAbs_C0);
            ++count;
        }
        else if (sh.ShapeType() == TopAbs_FACE) {
            maker.Add(TopoDS::Face(sh), GeomAbs_C0);
            ++count;
        }
        else if (sh.ShapeType() == TopAbs_VERTEX) {
            const TopoDS_Vertex& v = TopoDS::Vertex(sh);
            gp_Pnt pnt = BRep_Tool::Pnt(v);
            maker.Add(pnt);
            ++count;
        }
    }
    if (!count) 
        Standard_Failure::Raise("Failed to created face with no constraints");
    return makEShape(maker,_shapes,op,appendTag);
}

TopoShape &TopoShape::makESolid(const std::vector<TopoShape> &shapes, const char *op, bool appendTag) {
    return makESolid(TopoShape().makECompound(shapes,appendTag),op,appendTag);
}

TopoShape &TopoShape::makESolid(const TopoShape &shape, const char *op, bool appendTag) {
    if(!op && appendTag) op = TOPOP_SOLID;
    _Shape.Nullify();
    resetElementMap();

    if(shape.isNull())
        HANDLE_NULL_SHAPE;

    //first, if we were given a compsolid, try making a solid out of it
    TopExp_Explorer CSExp (shape.getShape(), TopAbs_COMPSOLID);
    TopoDS_CompSolid compsolid;
    int count=0;
    for (; CSExp.More(); CSExp.Next()) {
        ++count;
        compsolid = TopoDS::CompSolid(CSExp.Current());
        if (count > 1)
            break;
    }
    if (count == 0) {
        //no compsolids. Get shells...
        BRepBuilderAPI_MakeSolid mkSolid;
        TopExp_Explorer anExp (shape.getShape(), TopAbs_SHELL);
        count=0;
        for (; anExp.More(); anExp.Next()) {
            ++count;
            mkSolid.Add(TopoDS::Shell(anExp.Current()));
        }

        if (count == 0)//no shells?
            Standard_Failure::Raise("No shells or compsolids found in shape");

        makEShape(mkSolid,shape,op,appendTag);
        BRepLib::OrientClosedSolid(TopoDS::Solid(_Shape));
    } else if (count == 1) {
        BRepBuilderAPI_MakeSolid mkSolid(compsolid);
        makEShape(mkSolid,shape,op,appendTag);
    } else { // if (count > 1)
        Standard_Failure::Raise("Only one compsolid can be accepted. "
                "Provided shape has more than one compsolid.");
    }
    return *this;
}

TopoShape &TopoShape::replacEShape(const TopoShape &shape, 
        const std::vector<std::pair<TopoShape,TopoShape> > &s)
{
    resetElementMap();
    _Shape.Nullify();
    if(shape.isNull())
        HANDLE_NULL_SHAPE;
    BRepTools_ReShape reshape;
    std::vector<TopoShape> shapes;
    shapes.reserve(s.size()+1);
    for (auto &v : s) {
        if(v.first.isNull() || v.second.isNull())
            HANDLE_NULL_INPUT;
        reshape.Replace(v.first.getShape(), v.second.getShape());
        shapes.push_back(v.second);
    }
    shapes.push_back(shape);
    _Shape = reshape.Apply(shape.getShape(),TopAbs_SHAPE);
    mapSubElement(TopAbs_FACE,shapes);
    return *this;
}

TopoShape &TopoShape::removEShape(const TopoShape &shape, const std::vector<TopoShape>& s) 
{
    resetElementMap();
    _Shape.Nullify();
    if(shape.isNull())
        HANDLE_NULL_SHAPE;
    BRepTools_ReShape reshape;
    for(auto &sh : s) {
        if(sh.isNull())
            HANDLE_NULL_INPUT;
        reshape.Remove(sh.getShape());
    }
    _Shape = reshape.Apply(shape.getShape(), TopAbs_SHAPE);
    mapSubElement(TopAbs_FACE,shape);
    return *this;
}

TopoShape &TopoShape::makEFillet(const TopoShape &shape, const std::vector<TopoShape> &edges,
        double radius1, double radius2, const char *op, bool appendTag)
{
    if(!op) op = TOPOP_FILLET;
    resetElementMap();
    _Shape.Nullify();
    if(shape.isNull())
        HANDLE_NULL_SHAPE;

    if(edges.empty())
        HANDLE_NULL_INPUT;

    TopTools_IndexedMapOfShape emap;
    TopExp::MapShapes(shape.getShape(), TopAbs_EDGE, emap);
    BRepFilletAPI_MakeFillet mkFillet(shape.getShape());
    for(auto &e : edges) {
        if(e.isNull())
            HANDLE_NULL_INPUT;
        const auto &edge = e.getShape();
        if(!emap.FindIndex(edge))
            Standard_Failure::Raise("edge does not belong to the shape");
        mkFillet.Add(radius1, radius2, TopoDS::Edge(edge));
    }
    return makEShape(mkFillet,shape,op,appendTag);
}

TopoShape &TopoShape::makEChamfer(const TopoShape &shape, const std::vector<TopoShape> &edges,
        double radius1, double radius2, const char *op, bool appendTag)
{
    if(!op) op = TOPOP_FILLET;
    resetElementMap();
    _Shape.Nullify();
    if(shape.isNull())
        HANDLE_NULL_SHAPE;

    if(edges.empty())
        HANDLE_NULL_INPUT;

    TopTools_IndexedMapOfShape emap;
    TopExp::MapShapes(shape.getShape(), TopAbs_EDGE, emap);
    TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
    TopExp::MapShapesAndAncestors(shape.getShape(), TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
    BRepFilletAPI_MakeChamfer mkChamfer(shape.getShape());
    for(auto &e : edges) {
        const auto &edge = e.getShape();
        if(e.isNull())
            HANDLE_NULL_INPUT;
        if(!emap.FindIndex(edge))
            Standard_Failure::Raise("edge does not belong to the shape");
        //Add edge to fillet algorithm
        const TopoDS_Face& face = TopoDS::Face(mapEdgeFace.FindFromKey(edge).First());
        mkChamfer.Add(radius1, radius2, TopoDS::Edge(edge), face);
    }
    return makEShape(mkChamfer,shape,op,appendTag);
}

TopoShape &TopoShape::makEGeneralFuse(const std::vector<TopoShape> &_shapes,
        std::vector<std::vector<TopoShape> > &modifies, double tol, const char *op, bool appendTag)
{
    if(!op) op = TOPOP_GENERAL_FUSE;
    resetElementMap();
    _Shape.Nullify();

    if(_shapes.empty())
        HANDLE_NULL_INPUT;

    std::vector<TopoShape> shapes(_shapes);

    BRepAlgoAPI_BuilderAlgo mkGFA;
    mkGFA.SetRunParallel(true);
    TopTools_ListOfShape GFAArguments;
    for(auto &shape : shapes) {
        if(shape.isNull())
            HANDLE_NULL_INPUT;
        if (tol > 0.0) {
            // workaround for http://dev.opencascade.org/index.php?q=node/1056#comment-520
            shape = shape.makECopy();
        }
        GFAArguments.Append(shape.getShape());
    }
    mkGFA.SetArguments(GFAArguments);
    if (tol > 0.0)
        mkGFA.SetFuzzyValue(tol);
#if OCC_VERSION_HEX >= 0x070000
    mkGFA.SetNonDestructive(Standard_True);
#endif
    mkGFA.Build();
    if (!mkGFA.IsDone())
        Standard_Failure::Raise("GeneralFuse failed");
    makEShape(mkGFA,shapes,op,appendTag);
    modifies.resize(shapes.size());
    int i=0;
    for(auto &s : shapes) {
        auto &mod = modifies[i++];
        for(TopTools_ListIteratorOfListOfShape it(mkGFA.Modified(s.getShape())); it.More(); it.Next()) {
            TopoShape res(Tag);
            res.setShape(it.Value());
            mod.push_back(res);
        }
        mapSubElementsTo(TopAbs_FACE,mod);
    }
    return *this;
}

TopoShape &TopoShape::makEFuse(const std::vector<TopoShape> &shapes, 
        const char *op, bool appendTag, double tol)
{
    return makEShape(TOPOP_FUSE,shapes,op,appendTag,tol);
}

TopoShape &TopoShape::makECut(const std::vector<TopoShape> &shapes, 
        const char *op, bool appendTag, double tol)
{
    return makEShape(TOPOP_CUT,shapes,op,appendTag,tol);
}


TopoShape &TopoShape::makEShape(BRepOffsetAPI_MakePipeShell &mkShape, 
        const std::vector<TopoShape> &source, const char *op, bool appendTag)
{
    if(!op) op = TOPOP_PIPE_SHELL;
    return makEShape(mkShape,source,op,appendTag);
}

TopoShape &TopoShape::makEShape(BRepFeat_MakePrism &mkShape, 
        const TopoShape &source, const char *op, bool appendTag) 
{
    if(!op) op = TOPOP_PRISM;
    return makEShape(mkShape,source,op,appendTag);
}

TopoShape &TopoShape::makEDraft(const TopoShape &shape, const std::vector<TopoShape> &_faces,
        const gp_Dir &pullDirection, double angle, const gp_Pln &neutralPlane,
        bool retry, const char *op, bool appendTag)
{
    if(!op) op = TOPOP_DRAFT;

    resetElementMap();
    _Shape.Nullify();
    if(shape.isNull())
        HANDLE_NULL_SHAPE;

    std::vector<TopoShape> faces(_faces);
    bool done = true;
    BRepOffsetAPI_DraftAngle mkDraft;
    do {
        if(faces.empty())
            HANDLE_NULL_INPUT;

        mkDraft.Init(shape.getShape());
        for(auto it=faces.begin();it!=faces.end();++it) {
            // TODO: What is the flag for?
            mkDraft.Add(TopoDS::Face(it->getShape()), pullDirection, angle, neutralPlane);
            if (!mkDraft.AddDone()) {
                // Note: the function ProblematicShape returns the face on which the error occurred
                // Note: mkDraft.Remove() stumbles on a bug in Draft_Modification::Remove() and is
                //       therefore unusable. See http://forum.freecadweb.org/viewtopic.php?f=10&t=3209&start=10#p25341
                //       The only solution is to discard mkDraft and start over without the current face
                // mkDraft.Remove(face);
                FC_ERR("Failed to add some face for drafting, skip");
                done = false;
                faces.erase(it);
                break;
            }
        }
    }while(retry && !done);

    mkDraft.Build();
    return makEShape(mkDraft,shape,op,appendTag);
}

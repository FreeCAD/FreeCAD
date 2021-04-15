/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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
# if OCC_VERSION_HEX < 0x070600
#   include <BRepAdaptor_HCurve.hxx>
#   include <BRepAdaptor_HCompCurve.hxx>
# endif
# include <BRepAdaptor_Surface.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
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
# include <BRepFeat_MakePrism.hxx>
# include <BRepTools.hxx>
# include <BRepTools_ReShape.hxx>
# include <BRepTools_ShapeSet.hxx>
# include <BRepTools_WireExplorer.hxx>
# include <BRepFill_CompatibleWires.hxx>
# include <BRepProj_Projection.hxx>
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
# include <ShapeConstruct_Curve.hxx>
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
# include <GeomLib_IsPlanarSurface.hxx>
# include <GeomConvert.hxx>
# include <Poly_Triangulation.hxx>
# include <Standard_Failure.hxx>
# include <StlAPI_Writer.hxx>
# include <gp_GTrsf.hxx>
# include <ShapeAnalysis_Shell.hxx>
# include <ShapeBuild_ReShape.hxx>
# include <ShapeExtend_Explorer.hxx>
# include <ShapeFix_Edge.hxx>
# include <ShapeFix_Face.hxx>
# include <ShapeFix_Shell.hxx>
# include <ShapeFix_Solid.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeFix_ShapeTolerance.hxx>
# include <ShapeUpgrade_ShellSewing.hxx>
# include <ShapeUpgrade_RemoveInternalWires.hxx>
# include <Standard_Version.hxx>
# include <ShapeFix_Wire.hxx>
# include <ShapeAnalysis.hxx>
# include <BRepFill.hxx>
# include <BRepOffsetAPI_DraftAngle.hxx>
# include <BRepPrimAPI_MakeHalfSpace.hxx>
# include <gp_Pln.hxx>
# include <BRepLProp_SLProps.hxx>
# include <BRepGProp_Face.hxx>
#endif
#include <ShapeAnalysis_FreeBoundsProperties.hxx>
#include <ShapeAnalysis_FreeBoundData.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <BRepOffsetAPI_MakeFilling.hxx>
#include <TopTools_DataMapIteratorOfDataMapOfShapeListOfShape.hxx>
#include <GeomFill_FillingStyle.hxx>
#include <GeomFill_BSplineCurves.hxx>
#include <GeomFill_BezierCurves.hxx>
#include <BRepFill_Generator.hxx>
#include <BRepOffsetAPI_MakeEvolved.hxx>

#if OCC_VERSION_HEX >= 0x070500
#   include <OSD_Parallel.hxx>
#endif

#include <array>
#include <deque>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include <Base/Exception.h>
#include <Base/Console.h>
#include <App/MappedElement.h>
#include <App/Application.h>
#include <App/Document.h>
#include "PartFeature.h"

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
#include "BRepOffsetAPI_MakeOffsetFix.h"
#include "Geometry.h"
#include "FaceMakerBullseye.h"
#include "PartParams.h"

FC_LOG_LEVEL_INIT("TopoShape",true,2);

#if OCC_VERSION_HEX >= 0x070600
using Adaptor3d_HCurve = Adaptor3d_Curve;
using BRepAdaptor_HCurve = BRepAdaptor_Curve;
using BRepAdaptor_HCompCurve = BRepAdaptor_CompCurve;
#endif

using namespace Part;
using namespace Data;
namespace bio = boost::iostreams;

#define _HANDLE_NULL_SHAPE(_msg,_throw) do {\
    if(_throw) {\
        FC_THROWM(NullShapeException,_msg);\
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

void ShapeMapper::expand(const TopoDS_Shape &d, std::vector<TopoDS_Shape> &shapes)
{
    if (d.IsNull()) return;
    for(TopExp_Explorer xp(d, TopAbs_FACE);xp.More();xp.Next())
        shapes.push_back(xp.Current());
    for(TopExp_Explorer xp(d, TopAbs_EDGE, TopAbs_FACE);xp.More();xp.Next())
        shapes.push_back(xp.Current());
    for(TopExp_Explorer xp(d, TopAbs_VERTEX, TopAbs_EDGE);xp.More();xp.Next())
        shapes.push_back(xp.Current());
}

void ShapeMapper::populate(bool generated,
                           const TopTools_ListOfShape &src,
                           const TopTools_ListOfShape &dst)
{
    for(TopTools_ListIteratorOfListOfShape it(src);it.More();it.Next())
        populate(generated, it.Value(), dst);
}

void ShapeMapper::populate(bool generated,
                           const TopoShape &src,
                           const TopTools_ListOfShape &dst)
{
    if(src.isNull())
        return;
    std::vector<TopoDS_Shape> dstShapes;
    for(TopTools_ListIteratorOfListOfShape it(dst);it.More();it.Next())
        expand(it.Value(), dstShapes);
    insert(generated, src.getShape(), dstShapes);
}

void ShapeMapper::insert(bool generated, const TopoDS_Shape &s, const TopoDS_Shape &d)
{
    if (s.IsNull() || d.IsNull()) return;
    // Prevent an element shape from being both generated and modified
    if (generated) {
        if (_modifiedShapes.count(d))
            return;
        _generatedShapes.insert(d);
    } else {
        if( _generatedShapes.count(d))
            return;
        _modifiedShapes.insert(d);
    }
    auto &entry = generated?_generated[s]:_modified[s];
    if(entry.shapeSet.insert(d).second)
        entry.shapes.push_back(d);
};

void ShapeMapper::insert(bool generated, const TopoDS_Shape &s, const std::vector<TopoDS_Shape> &d)
{
    if (s.IsNull() || d.empty()) return;
    auto &entry = generated?_generated[s]:_modified[s];
    for(auto &shape : d) {
        // Prevent an element shape from being both generated and modified
        if (generated) {
            if (_modifiedShapes.count(shape))
                continue;
            _generatedShapes.insert(shape);
        } else {
            if( _generatedShapes.count(shape))
                continue;
            _modifiedShapes.insert(shape);
        }
        if(entry.shapeSet.insert(shape).second)
            entry.shapes.push_back(shape);
    }
};

struct ShapeRelationKey {
    Data::MappedName name;
    bool sameType;

    ShapeRelationKey(const Data::MappedName & name, bool sameType)
        :name(name), sameType(sameType)
    {}

    bool operator<(const ShapeRelationKey &other) const {
        if(sameType != other.sameType)
            return sameType;
        return name < other.name;
    }
};

#define INIT_SHAPE_CACHE() initCache(0,__FILE__,__LINE__)

struct ShapeSource {
    TopoShape shape;
    QByteArray op;
};

struct ShapeSources {
    std::vector<ShapeSource> shapes;
    bool expanded = false;
};

class TopoShape::Cache: public std::enable_shared_from_this<TopoShape::Cache>
{
public:
    ElementMapPtr cachedElementMap;
    TopLoc_Location subLocation;
    
    TopoDS_Shape shape;
    TopLoc_Location loc;
    TopLoc_Location locInv;

    std::size_t memsize = 0;

    struct AncestorInfo {
        bool inited = false;
        TopTools_IndexedDataMapOfShapeListOfShape shapes;
    };
    class Info {
    private:
        Cache *owner = 0;
        TopTools_IndexedMapOfShape shapes;
        std::vector<TopoShape> topoShapes;
        std::array<AncestorInfo, TopAbs_SHAPE+1> ancestors;

        TopoShape _getTopoShape(const TopoShape &parent, int index) {
            auto &s = topoShapes[index-1];
            if(s.isNull()) {
                s.setShape(shapes.FindKey(index), true);
                s.INIT_SHAPE_CACHE();
                s._Cache->subLocation = s._Shape.Location();
            }

            if (s._Shape.IsEqual(parent._Cache->shape))
                return parent;

            TopoShape res(s);
            res.Tag = parent.Tag;
            res.Hasher = parent.Hasher;

            if(!parent.getShape().Location().IsIdentity())
                res.setShape(moved(res._Shape,parent.getShape().Location()),false);

            if (s._Cache->cachedElementMap)
                res.resetElementMap(s._Cache->cachedElementMap);
            else if (parent._ParentCache) {
                // If no cachedElementMap exists, we use _ParentCache for
                // delayed generation of sub element map so that we don't need
                // to always generate a full map whenever we returns a sub
                // shape.  To simplify the mapping and avoid circular
                // dependency, we do not chain parent and grand parent.
                // Instead, we always use the cache from the top parent. And to
                // make it work, we must accumulate the TopLoc_Location along
                // the lineage, which is required for OCCT shape mapping to
                // work.
                //
                // Cache::subLocation is shared and only contains the location
                // in the direct parent shape, while TopoShape::_SubLocation is
                // used to accumulate locations in higher ancestors. We
                // separate these two to avoid invalidating cache.

                res._SubLocation = parent._SubLocation * parent._Cache->subLocation;
                res._ParentCache = parent._ParentCache;
            } else
                res._ParentCache = owner->shared_from_this();
            return res;
        }

    public:
        void clear()
        {
            topoShapes.clear();
        }

        TopoShape getTopoShape(const TopoShape &parent, int index) {
            TopoShape res;
            if(index<=0 || index>shapes.Extent())
                return res;
            topoShapes.resize(shapes.Extent());
            return _getTopoShape(parent,index);
        }

        std::vector<TopoShape> getTopoShapes(const TopoShape &parent) {
            int count = shapes.Extent();
            std::vector<TopoShape> res;
            res.reserve(count);
            topoShapes.resize(count);
            for(int i=1;i<=count;++i)
                res.push_back(_getTopoShape(parent,i));
            return res;
        }

        TopoDS_Shape stripLocation(const TopoDS_Shape &parent, const TopoDS_Shape &child) {
            if(parent.Location() != owner->loc) {
                owner->loc = parent.Location();
                owner->locInv = parent.Location().Inverted();
            }
            return TopoShape::located(child,owner->locInv*child.Location());
        }

        int find(const TopoDS_Shape &parent, const TopoDS_Shape &subshape) {
            if(parent.Location().IsIdentity())
                return shapes.FindIndex(subshape);
            return shapes.FindIndex(stripLocation(parent,subshape));
        }

        TopoDS_Shape find(const TopoDS_Shape &parent, int index) {
            if(index<=0 || index>shapes.Extent())
                return TopoDS_Shape();
            if(parent.Location().IsIdentity())
                return shapes.FindKey(index);
            else
                return moved(shapes.FindKey(index),parent.Location());
        }

        int count() const {
            return shapes.Extent();
        }

        friend Cache;
    };

    std::array<Info,TopAbs_SHAPE+1> infos;
    std::map<ShapeRelationKey,QVector<Data::MappedElement> > relations;

    Cache(const TopoDS_Shape &s)
        :shape(s.Located(TopLoc_Location()))
    {}

    void insertRelation(const ShapeRelationKey &key, const QVector<Data::MappedElement> &value)
    {
        auto res = relations.insert(std::make_pair(key, value));
        if (res.second)
            res.first->first.name.compact();
        else
            res.first->second = value;
    }

    bool isTouched(const TopoDS_Shape &s)
    {
        return !this->shape.IsPartner(s) ||
            this->shape.Orientation() != s.Orientation();
    }

    Info &getInfo(TopAbs_ShapeEnum type) {
        auto &info = infos[type];
        if(!info.owner) {
            info.owner = this;
            if(!shape.IsNull()) {
                if(type == TopAbs_SHAPE) {
                    for(TopoDS_Iterator it(shape);it.More();it.Next())
                        info.shapes.Add(it.Value());
                }else
                    TopExp::MapShapes(shape, type, info.shapes);
            }
        }
        return info;
    }

    int countShape(TopAbs_ShapeEnum type) {
        if(shape.IsNull())
            return 0;
        return getInfo(type).count();
    }

    int findShape(const TopoDS_Shape &parent, const TopoDS_Shape &subshape) {
        if(shape.IsNull() || subshape.IsNull())
            return 0;
        return getInfo(subshape.ShapeType()).find(parent,subshape);
    }

    TopoDS_Shape findShape(const TopoDS_Shape &parent, TopAbs_ShapeEnum type, int index) {
        if(!shape.IsNull())
            return getInfo(type).find(parent,index);
        return TopoDS_Shape();
    }

    TopoDS_Shape findAncestor(const TopoDS_Shape &parent, const TopoDS_Shape &subshape,
            TopAbs_ShapeEnum type, std::vector<TopoDS_Shape> *ancestors=0)
    {
        TopoDS_Shape ret;
        if(shape.IsNull() || subshape.IsNull() || type==TopAbs_SHAPE)
            return ret;

        auto &info = getInfo(type);

        auto &ainfo = info.ancestors[subshape.ShapeType()];
        if(!ainfo.inited) {
            ainfo.inited = true;
            TopExp::MapShapesAndAncestors(shape, subshape.ShapeType(), type, ainfo.shapes);
        }
        int index;
        if(parent.Location().IsIdentity())
            index = ainfo.shapes.FindIndex(subshape);
        else
            index = ainfo.shapes.FindIndex(info.stripLocation(parent,subshape));
        if(!index)
            return ret;
        const auto &shapes = ainfo.shapes.FindFromIndex(index);
        if(!shapes.Extent())
            return ret;

        if(ancestors) {
            ancestors->reserve(ancestors->size()+shapes.Extent());
            for(TopTools_ListIteratorOfListOfShape it(shapes);it.More();it.Next())
                ancestors->push_back(moved(it.Value(),parent.Location()));
        }
        return moved(shapes.First(),parent.Location());
    }

    std::size_t getMemSize();
};

void TopoShape::initCache(int reset, const char *file, int line) const{
    if(reset>0 || !_Cache || _Cache->isTouched(_Shape)) {
        if(_Cache && reset==0) {
            if(file)
                _FC_TRACE(file,line,"invalidate cache");
            else
                FC_TRACE("invalidate cache");
        }
        if (_ParentCache) {
            _ParentCache.reset();
            _SubLocation.Identity();
        }
        _Cache = std::make_shared<Cache>(_Shape);
    }
}

Data::ElementMapPtr TopoShape::resetElementMap(Data::ElementMapPtr elementMap)
{
    if (_Cache && elementMap != this->elementMap(false)) {
        for (auto &info : _Cache->infos)
            info.clear();
    } else
        INIT_SHAPE_CACHE();
    if (elementMap) {
        _Cache->cachedElementMap = elementMap;
        _Cache->subLocation.Identity();
        _SubLocation.Identity();
        _ParentCache.reset();
    }
    return Data::ComplexGeoData::resetElementMap(elementMap);
}

void TopoShape::flushElementMap() const
{
    INIT_SHAPE_CACHE();
    if (!elementMap(false) && this->_Cache) {
        if (this->_Cache->cachedElementMap) {
            const_cast<TopoShape*>(this)->resetElementMap(this->_Cache->cachedElementMap);
        }
        else if (this->_ParentCache) {
            TopoShape parent(this->Tag, this->Hasher, this->_ParentCache->shape);
            parent._Cache = _ParentCache;
            parent.flushElementMap();
            TopoShape self(this->Tag, this->Hasher,
                    this->_Shape.Located(this->_SubLocation * this->_Cache->subLocation));
            self._Cache = _Cache;
            self.mapSubElement(parent);
            this->_ParentCache.reset();
            this->_SubLocation.Identity();
            const_cast<TopoShape*>(this)->resetElementMap(self.elementMap());
        }
    }
}

unsigned long TopoShape::getElementMapReserve() const
{
    if (isNull())
        return 0;
    return countSubShapes(TopAbs_VERTEX) 
        + countSubShapes(TopAbs_EDGE)
        + countSubShapes(TopAbs_FACE);
}

TopoShape::TopoShape(const TopoShape& shape)
    : _Shape(*this)
{
    *this = shape;
}

void TopoShape::setShape(const TopoDS_Shape & shape, bool resetElementMap)
{
    if(resetElementMap)
        this->resetElementMap();
    else if (_Cache && _Cache->isTouched(shape))
        this->flushElementMap();
    _Shape._Shape = shape;
    if (_Cache)
        INIT_SHAPE_CACHE();
}

bool TopoShape::hasPendingElementMap() const
{
    return !elementMap(false)
        && this->_Cache
        && (this->_ParentCache || this->_Cache->cachedElementMap);
}

void TopoShape::operator = (const TopoShape& sh)
{
    if (this != &sh) {
        this->setShape(sh._Shape, true);
        this->Tag = sh.Tag;
        this->Hasher = sh.Hasher;
        this->_Cache = sh._Cache;
        this->_ParentCache = sh._ParentCache;
        this->_SubLocation = sh._SubLocation;
        resetElementMap(sh.elementMap(false));
    }
}

int TopoShape::findShape(const TopoDS_Shape &subshape) const {
    INIT_SHAPE_CACHE();
    return _Cache->findShape(_Shape,subshape);
}

static const std::string _SubShape("SubShape");

TopoDS_Shape TopoShape::findShape(const char *name) const {
    if(!name)
        return TopoDS_Shape();

    Data::MappedElement res = getElementName(name);
    if (!res.index)
        return TopoDS_Shape();

    auto idx = shapeTypeAndIndex(name);
    if(!idx.second)
        return TopoDS_Shape();
    INIT_SHAPE_CACHE();
    return _Cache->findShape(_Shape,idx.first,idx.second);
}

TopoDS_Shape TopoShape::findShape(TopAbs_ShapeEnum type, int idx) const {
    INIT_SHAPE_CACHE();
    return _Cache->findShape(_Shape,type,idx);
}

std::vector<TopoShape> TopoShape::searchSubShape(
        const TopoShape &subshape, std::vector<std::string> *names,
        bool checkGeometry, double tol, double atol) const
{
    std::vector<TopoShape> res;
    if(subshape.isNull() || this->isNull())
        return res;
    double tol2 = tol*tol;
    int i=0;
    TopAbs_ShapeEnum shapeType = subshape.shapeType();
    switch(shapeType) {
    case TopAbs_VERTEX:
        // Vertex search will do comparison with tolerance to account for
        // rounding error inccured through transformation.
        for(auto &s : getSubTopoShapes(TopAbs_VERTEX)) {
            ++i;
            if(BRep_Tool::Pnt(TopoDS::Vertex(s.getShape())).SquareDistance(
                        BRep_Tool::Pnt(TopoDS::Vertex(subshape.getShape()))) <= tol2)
            {
                if(names)
                    names->push_back(std::string("Vertex")+std::to_string(i));
                res.push_back(s);
            }
        }
        break;
    case TopAbs_EDGE:
    case TopAbs_FACE: {
        std::unique_ptr<Geometry> g;
        bool isLine = false;
        bool isPlane = false;

        std::vector<TopoDS_Shape> vertices;
        TopoShape wire;
        if(shapeType == TopAbs_FACE) {
            wire = subshape.splitWires();
            vertices = wire.getSubShapes(TopAbs_VERTEX);
        } else
            vertices = subshape.getSubShapes(TopAbs_VERTEX);

        if(vertices.empty() || checkGeometry) {
            g = Geometry::fromShape(subshape.getShape());
            if(!g)
                return res;
            if (shapeType == TopAbs_EDGE)
                isLine = (g->isDerivedFrom(GeomLine::getClassTypeId())
                            || g->isDerivedFrom(GeomLineSegment::getClassTypeId()));
            else
                isPlane = g->isDerivedFrom(GeomPlane::getClassTypeId());
        }

        auto compareGeometry = [&](const TopoShape &s, bool strict) {
            std::unique_ptr<Geometry> g2(Geometry::fromShape(s.getShape()));
            if (!g2)
                return false;
            if (isLine && !strict) {
                // For lines, don't compare geometry, just check the
                // vertices below instead, because the exact same edge
                // may have different geometrical representation.
                if (!g2->isDerivedFrom(GeomLine::getClassTypeId())
                        && !g2->isDerivedFrom(GeomLineSegment::getClassTypeId()))
                    return false;
            } else if (isPlane && !strict) {
                // For planes, don't compare geometry either, so that
                // we don't need to worry about orientation and so on.
                // Just check the edges.
                if (!g2->isDerivedFrom(GeomPlane::getClassTypeId()))
                    return false;
            } else if(!g2 || !g2->isSame(*g,tol,atol))
                return false;
            return true;
        };

        if(vertices.empty()) {
            // Probably an infinite shape, so we have to search by geometry
            int idx = 0;
            for (auto &s : getSubTopoShapes(shapeType)) {
                ++idx;
                if (!s.countSubShapes(TopAbs_VERTEX) && compareGeometry(s, true)) {
                    if(names)
                        names->push_back(shapeName(shapeType) + std::to_string(idx));
                    res.push_back(s);
                }
            }
            break;
        }

        // The basic idea of shape search is about the same for both edge and face.
        // * Search the first vertex, which is done with tolerance.
        // * Find the ancestor shape of the found vertex
        // * Compare each vertex of the ancestor shape and the input shape
        // * Perform geometry comparison of the ancestor and input shape.
        //      * For face, perform addition geometry comparison of each edges.
        std::unordered_set<TopoShape> shapeSet;
        for(auto &v : searchSubShape(vertices[0],nullptr,checkGeometry,tol,atol)) {
            for(auto idx : findAncestors(v.getShape(), shapeType)) {
                auto s = getSubTopoShape(shapeType, idx);
                if(!shapeSet.insert(s).second)
                    continue;
                TopoShape otherWire;
                std::vector<TopoDS_Shape> otherVertices;
                if (shapeType == TopAbs_FACE) {
                    otherWire = s.splitWires();
                    if (wire.countSubShapes(TopAbs_EDGE) != otherWire.countSubShapes(TopAbs_EDGE))
                        continue;
                    otherVertices = otherWire.getSubShapes(TopAbs_VERTEX);
                } else
                    otherVertices = s.getSubShapes(TopAbs_VERTEX);
                if (otherVertices.size() != vertices.size())
                    continue;
                if(checkGeometry && !compareGeometry(s, false))
                    continue;
                unsigned i = 0;
                bool matched = true;
                for(auto &v : vertices) {
                    bool found = false;
                    for (unsigned j=0; j<otherVertices.size(); ++j) {
                        auto & v1 = otherVertices[i];
                        if (++i == otherVertices.size())
                            i = 0;
                        if(BRep_Tool::Pnt(TopoDS::Vertex(v)).SquareDistance(
                                    BRep_Tool::Pnt(TopoDS::Vertex(v1))) <= tol2)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        matched = false;
                        break;
                    }
                }
                if(!matched)
                    continue;

                if(shapeType == TopAbs_FACE && checkGeometry) {
                    // Is it really necessary to check geometries of each edge of a face?
                    // Right now we only do outer wire check
                    auto otherEdges = otherWire.getSubShapes(TopAbs_EDGE);
                    std::vector<std::unique_ptr<Geometry> > geos;
                    geos.resize(otherEdges.size());
                    bool matched = true;
                    unsigned i = 0;
                    auto edges = wire.getSubShapes(TopAbs_EDGE);
                    for(auto &e : edges) {
                        std::unique_ptr<Geometry> g(Geometry::fromShape(e));
                        if(!g) {
                            matched = false;
                            break;
                        }
                        bool isLine = false;
                        gp_Pnt pt1, pt2;
                        if (g->isDerivedFrom(GeomLine::getClassTypeId())
                                || g->isDerivedFrom(GeomLineSegment::getClassTypeId()))
                        {
                            pt1 = BRep_Tool::Pnt(TopExp::FirstVertex(TopoDS::Edge(e)));
                            pt2 = BRep_Tool::Pnt(TopExp::LastVertex(TopoDS::Edge(e)));
                            isLine = true;
                        }
                        // We will tolerate on edge reordering
                        bool found = false;
                        for (unsigned j=0; j<otherEdges.size(); j++) {
                            auto & e1 = otherEdges[i];
                            auto & g1 = geos[i];
                            if (++i >= otherEdges.size())
                                i = 0;
                            if (!g1) {
                                g1 = Geometry::fromShape(e1);
                                if (!g1)
                                    break;
                            }
                            if (isLine) {
                                if(g1->isDerivedFrom(GeomLine::getClassTypeId())
                                        || g1->isDerivedFrom(GeomLineSegment::getClassTypeId()))
                                {
                                    auto p1 = BRep_Tool::Pnt(TopExp::FirstVertex(TopoDS::Edge(e1)));
                                    auto p2 = BRep_Tool::Pnt(TopExp::LastVertex(TopoDS::Edge(e1)));
                                    if((p1.SquareDistance(pt1) <= tol2
                                                && p2.SquareDistance(pt2) <= tol2)
                                            || (p1.SquareDistance(pt2) <= tol2
                                                && p2.SquareDistance(pt1) <= tol2))
                                    {
                                        found = true;
                                        break;
                                    }
                                }
                                continue;
                            }

                            if(g1->isSame(*g,tol,atol)) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            matched = false;
                            break;
                        }
                    }
                    if (!matched)
                        continue;
                }
                if(names)
                    names->push_back(shapeName(shapeType) + std::to_string(idx));
                res.push_back(s);
            }
        }
        break;
    }
    default:
        break;
    }
    return res;
}

int TopoShape::findAncestor(const TopoDS_Shape &subshape, TopAbs_ShapeEnum type) const {
    INIT_SHAPE_CACHE();
    return _Cache->findShape(_Shape,_Cache->findAncestor(_Shape,subshape,type));
}

TopoDS_Shape TopoShape::findAncestorShape(const TopoDS_Shape &subshape, TopAbs_ShapeEnum type) const {
    INIT_SHAPE_CACHE();
    return _Cache->findAncestor(_Shape,subshape,type);
}

std::vector<int> TopoShape::findAncestors(const TopoDS_Shape &subshape, TopAbs_ShapeEnum type) const {
    const auto &shapes = findAncestorsShapes(subshape,type);
    std::vector<int> ret;
    ret.reserve(shapes.size());
    for(const auto &shape : shapes)
        ret.push_back(findShape(shape));
    return ret;
}

std::vector<TopoDS_Shape> TopoShape::findAncestorsShapes(
        const TopoDS_Shape &subshape, TopAbs_ShapeEnum type) const
{
    INIT_SHAPE_CACHE();
    std::vector<TopoDS_Shape> shapes;
    _Cache->findAncestor(_Shape,subshape,type,&shapes);
    return shapes;
}

bool TopoShape::canMapElement(const TopoShape &other) const {
    if(isNull() || other.isNull() || this == &other || other.Tag == -1 || Tag == -1)
        return false;
    if(!other.Tag
            && !other.elementMap(false)
            && !other.hasPendingElementMap())
        return false;
    INIT_SHAPE_CACHE();
    other.INIT_SHAPE_CACHE();
    _Cache->relations.clear();
    return true;
}

void TopoShape::mapSubElement(const std::vector<TopoShape> &shapes, const char *op) {
    if (shapes.empty())
        return;

    if (shapeType(true) == TopAbs_COMPOUND) {
        int count = 0;
        for (auto & s : shapes) {
            if (s.isNull())
                continue;
            if (!getSubShape(TopAbs_SHAPE, ++count, true).IsPartner(s._Shape)) {
                count = 0;
                break;
            }
        }
        if (count) {
            std::vector<Data::MappedChildElements> children;
            children.reserve(count*3);
            TopAbs_ShapeEnum types[] = {TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE};
            for (unsigned i=0; i<sizeof(types)/sizeof(types[0]); ++i) {
                int offset = 0;
                for (auto & s : shapes) {
                    if (s.isNull())
                        continue;
                    int count = s.countSubShapes(types[i]);
                    if (!count)
                        continue;
                    children.emplace_back();
                    auto & child = children.back();
                    child.indexedName = Data::IndexedName::fromConst(shapeName(types[i]).c_str(), 1);
                    child.offset = offset;
                    offset += count;
                    child.count = count;
                    child.elementMap = s.elementMap();
                    child.tag = s.Tag;
                    if (op)
                        child.postfix = op;
                }
            }
            setMappedChildElements(children);
            return;
        }
    }

    for(auto &shape : shapes)
        mapSubElement(shape,op);
}

void TopoShape::mapSubElementsTo(std::vector<TopoShape> &shapes, const char *op) const {
    for(auto &shape : shapes)
        shape.mapSubElement(*this,op);
}

void TopoShape::copyElementMap(const TopoShape &s, const char *op)
{
    if (s.isNull() || isNull())
        return;
    std::vector<Data::MappedChildElements> children;
    TopAbs_ShapeEnum types[] = {TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE};
    for (unsigned i=0; i<sizeof(types)/sizeof(types[0]); ++i) {
        int count = countSubShapes(types[i]);
        int other = s.countSubShapes(types[i]);
        if (count != other) {
            FC_WARN("sub shape mismatch");
            if (count > other)
                count = other;
        }
        if (!count)
            continue;
        children.emplace_back();
        auto & child = children.back();
        child.indexedName = Data::IndexedName::fromConst(shapeName(types[i]).c_str(), 1);
        child.offset = 0;
        child.count = count;
        child.elementMap = s.elementMap();
        if (this->Tag != s.Tag)
            child.tag = s.Tag;
        else
            child.tag = 0;
        if (op)
            child.postfix = op;
    }
    resetElementMap();
    if (!Hasher)
        Hasher = s.Hasher;
    setMappedChildElements(children);
}

void TopoShape::mapSubElement(const TopoShape &other, const char *op, bool forceHasher) {
    if(!canMapElement(other))
        return;

    if (!getElementMapSize(false) && this->_Shape.IsPartner(other._Shape)) {
        if (!this->Hasher)
            this->Hasher = other.Hasher;
        copyElementMap(other, op);
        return;
    }

    bool warned = false;
    static const std::array<TopAbs_ShapeEnum,3> types = 
        {TopAbs_VERTEX,TopAbs_EDGE,TopAbs_FACE};

    auto checkHasher = [this](const TopoShape &other) {
        if(Hasher) {
            if(other.Hasher!=Hasher) {
                if(!getElementMapSize(false)) {
                    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                        FC_WARN("hasher mismatch");
                }else {
                    // FC_THROWM(Base::RuntimeError, "hasher mismatch");
                    FC_ERR("hasher mismatch");
                }
                Hasher = other.Hasher;
            }
        }else
            Hasher = other.Hasher;
    };

    for(auto type : types) {
        auto &shapeMap = _Cache->getInfo(type);
        auto &otherMap = other._Cache->getInfo(type);
        if(!shapeMap.count() || !otherMap.count())
            continue;
        if(!forceHasher && other.Hasher) {
            forceHasher = true;
            checkHasher(other);
        }
        const char *shapetype = shapeName(type).c_str();
        std::ostringstream ss;

        bool forward;
        int count;
        if(otherMap.count()<=shapeMap.count()) {
            forward = true;
            count = otherMap.count();
        }else{
            forward = false;
            count = shapeMap.count();
        }
        for(int k=1;k<=count;++k) {
            int i,idx;
            if(forward) {
                i = k;
                idx = shapeMap.find(_Shape,otherMap.find(other._Shape,k));
                if(!idx) continue;
            } else {
                idx = k;
                i = otherMap.find(other._Shape,shapeMap.find(_Shape,k));
                if(!i) continue;
            }
            Data::IndexedName element = Data::IndexedName::fromConst(shapetype, idx);
            for(auto &v : other.getElementMappedNames(
                        Data::IndexedName::fromConst(shapetype,i),true))
            {
                auto &name = v.first;
                auto &sids = v.second;
                if(sids.size()) {
                    if (!Hasher)
                        Hasher = sids[0].getHasher();
                    else if (!sids[0].isFromSameHasher(Hasher)) {
                        if (!warned) {
                            warned = true;
                            FC_WARN("hasher mismatch");
                        }
                        sids.clear();
                    }
                }
                ss.str("");
                encodeElementName(shapetype[0],name,ss,&sids,op,other.Tag);
                setElementName(element,name,&sids);
            }
        }
    }
}

std::vector<TopoDS_Shape> TopoShape::getSubShapes(TopAbs_ShapeEnum type, TopAbs_ShapeEnum avoid) const {
    std::vector<TopoDS_Shape> ret;
    if(isNull())
        return ret;
    if (avoid != TopAbs_SHAPE) {
        for (TopExp_Explorer exp(getShape(), type, avoid); exp.More(); exp.Next())
            ret.push_back(exp.Current());
        return ret;
    }
    INIT_SHAPE_CACHE();
    auto &info = _Cache->getInfo(type);
    int count = info.count();
    ret.reserve(count);
    for(int i=1;i<=count;++i)
        ret.push_back(info.find(_Shape,i));
    return ret;
}

std::vector<TopoShape> TopoShape::getSubTopoShapes(TopAbs_ShapeEnum type, TopAbs_ShapeEnum avoid) const {
    if(isNull())
        return std::vector<TopoShape>();
    INIT_SHAPE_CACHE();

    auto res = _Cache->getInfo(type).getTopoShapes(*this);
    if (avoid != TopAbs_SHAPE && hasSubShape(avoid)) {
        for (auto it = res.begin(); it != res.end(); ) {
            if (_Cache->findAncestor(_Shape, it->getShape(), avoid).IsNull())
                ++it;
            else
                it = res.erase(it);
        }
    }
    return res;
}

std::vector<TopoShape> TopoShape::getOrderedEdges(bool mapElement) const
{
    if(isNull())
        return std::vector<TopoShape>();

    std::vector<TopoShape> shapes;
    if (shapeType() == TopAbs_WIRE) {
        BRepTools_WireExplorer xp(TopoDS::Wire(getShape()));
        while (xp.More()) {
            shapes.push_back(TopoShape(xp.Current()));
            xp.Next();
        }
    }
    else {
        INIT_SHAPE_CACHE();
        for (const auto &w : getSubShapes(TopAbs_WIRE)) {
            BRepTools_WireExplorer xp(TopoDS::Wire(w));
            while (xp.More()) {
                shapes.push_back(TopoShape(xp.Current()));
                xp.Next();
            }
        }
    }
    if (mapElement)
        mapSubElementsTo(shapes);
    return shapes;
}

std::vector<TopoShape> TopoShape::getOrderedVertexes(bool mapElement) const
{
    if(isNull())
        return std::vector<TopoShape>();

    std::vector<TopoShape> shapes;

    auto collect = [&](const TopoDS_Shape &s) {
        auto wire = TopoDS::Wire(s);
        BRepTools_WireExplorer xp(wire);
        while (xp.More()) {
            shapes.push_back(TopoShape(xp.CurrentVertex()));
            xp.Next();
        }
        // special treatment for open wires
        TopoDS_Vertex Vfirst, Vlast;
        TopExp::Vertices(wire, Vfirst, Vlast);
        if (!Vfirst.IsNull() && !Vlast.IsNull()) {
            if (!Vfirst.IsSame(Vlast)) {
                shapes.push_back(TopoShape(Vlast));
            }
        }
    };

    if (shapeType() == TopAbs_WIRE)
        collect(getShape());
    else {
        INIT_SHAPE_CACHE();
        for (const auto &s : getSubShapes(TopAbs_WIRE))
            collect(s);
    }
    if (mapElement)
        mapSubElementsTo(shapes);
    return shapes;
}

std::pair<TopAbs_ShapeEnum,int>
TopoShape::shapeTypeAndIndex(const Data::IndexedName & element)
{
    if (!element)
        return std::make_pair(TopAbs_SHAPE, 0);
    if (boost::equals(element.getType(), _SubShape))
        return std::make_pair(TopAbs_SHAPE, element.getIndex());
    TopAbs_ShapeEnum shapetype = shapeType(element.getType(), true);
    if (shapetype == TopAbs_SHAPE)
        return std::make_pair(TopAbs_SHAPE, 0);
    return std::make_pair(shapetype, element.getIndex());
}

std::pair<TopAbs_ShapeEnum,int>
TopoShape::shapeTypeAndIndex(const char *name)
{
    int idx = 0;
    auto type = shapeType(name,true);
    size_t len;
    if(type != TopAbs_SHAPE)
        len = shapeName(type).size();
    else {
        if(boost::starts_with(name,_SubShape))
            len = _SubShape.size();
        else
            len = 0;
    }

    if(len) {
        bio::stream<bio::array_source> iss(name+len, std::strlen(name+len));
        iss >> idx;
        if(!iss.eof())
            idx = 0;
    }
    if(!idx)
        type = TopAbs_SHAPE;
    return std::make_pair(type,idx);
}

unsigned long TopoShape::countSubShapes(TopAbs_ShapeEnum type) const {
    INIT_SHAPE_CACHE();
    return _Cache->countShape(type);
}

unsigned long TopoShape::countSubShapes(const char* Type) const {
    auto type = shapeType(Type, true);
    if(type == TopAbs_SHAPE) {
        if(Type && _SubShape == Type)
            return countSubShapes(type);
        return 0;
    }
    return countSubShapes(type);
}

bool TopoShape::hasSubShape(TopAbs_ShapeEnum type) const {
    return countSubShapes(type)!=0;
}

bool TopoShape::hasSubShape(const char *Type) const {
    auto idx = shapeTypeAndIndex(Type);
    return idx.second>0 && idx.second<=(int)countSubShapes(idx.first);
}

TopoShape TopoShape::getSubTopoShape(const char *Type, bool silent) const {
    if (!Type || !Type[0]) {
        switch (shapeType(true)) {
        case TopAbs_COMPOUND:
        case TopAbs_COMPSOLID:
            if (countSubShapes(TopAbs_SOLID) == 1)
                return getSubTopoShape(TopAbs_SOLID, 1);
            if (countSubShapes(TopAbs_SHELL) == 1)
                return getSubTopoShape(TopAbs_SHELL, 1);
            if (countSubShapes(TopAbs_FACE) == 1)
                return getSubTopoShape(TopAbs_FACE, 1);
            if (countSubShapes(TopAbs_WIRE) == 1)
                return getSubTopoShape(TopAbs_WIRE, 1);
            if (countSubShapes(TopAbs_EDGE) == 1)
                return getSubTopoShape(TopAbs_EDGE, 1);
            if (countSubShapes(TopAbs_VERTEX) == 1)
                return getSubTopoShape(TopAbs_VERTEX, 1);
            break;
        default:
            break;
        }
        return *this;
    }

    Data::MappedElement mapped = getElementName(Type);
    if (!mapped.index && boost::starts_with(Type,elementMapPrefix())) {
        if(!silent)
            FC_THROWM(Base::CADKernelError, "Mapped element not found: " << Type);
        return TopoShape();
    }

    auto res = shapeTypeAndIndex(mapped.index);
    if(res.second<=0) {
        if(!silent)
            FC_THROWM(Base::CADKernelError,"Invalid shape name " << (Type?Type:""));
        return TopoShape();
    }
    return getSubTopoShape(res.first,res.second,silent);
}

TopoShape TopoShape::getSubTopoShape(TopAbs_ShapeEnum type, int idx, bool silent) const {
    if(isNull()) {
        if(!silent)
            FC_THROWM(NullShapeException,"null shape");
        return TopoShape();
    }
    if(idx <= 0) {
        if(!silent)
            FC_THROWM(Base::ValueError,"Invalid shape index " << idx);
        return TopoShape();
    }
    if(type<0 || type>TopAbs_SHAPE) {
        if(!silent)
            FC_THROWM(Base::ValueError,"Invalid shape type " << type);
        return TopoShape();
    }
    INIT_SHAPE_CACHE();
    auto &info = _Cache->getInfo(type);
    if(idx > info.count()) {
        if(!silent)
            FC_THROWM(Base::ValueError,"Shape index " << idx << " out of bound "  << info.count());
        return TopoShape();
    }

    return info.getTopoShape(*this,idx);
}

static const std::string & _getElementMapVersion()
{
    static std::string _ver;
    if (_ver.empty()) {
        std::ostringstream ss;
        unsigned occ_ver;
        if((OCC_VERSION_HEX & 0xFF0000) == 0x070000)
            occ_ver = 0x070200;
        else
            occ_ver = OCC_VERSION_HEX;
        ss << OpCodes::Version << '.' << std::hex << occ_ver << '.';
        _ver = ss.str();
    }
    return _ver;
}

std::string TopoShape::getElementMapVersion() const{
    return _getElementMapVersion() + Data::ComplexGeoData::getElementMapVersion();
}

bool TopoShape::checkElementMapVersion(const char * ver) const
{
    if (!boost::starts_with(ver, _getElementMapVersion()))
        return true;

    return Data::ComplexGeoData::checkElementMapVersion(
            ver + _getElementMapVersion().size());
}

TopoShape &TopoShape::makECompound(const std::vector<TopoShape> &shapes, const char *op, bool force)
{
    if(!force && shapes.size()==1) {
        *this = shapes[0];
        return *this;
    }

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);

    if(shapes.empty()) {
        setShape(comp);
        return *this;
    }

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
    setShape(comp);
    INIT_SHAPE_CACHE();

    mapSubElement(shapes,op);
    return *this;
}

bool TopoShape::_makETransform(const TopoShape &shape,
        const Base::Matrix4D &rclTrf, const char *op, bool checkScale, bool copy)
{
    if(checkScale) {
        auto scaleType = rclTrf.hasScale();
        if (scaleType != Base::ScaleType::NoScaling && scaleType != Base::ScaleType::Uniform) {
            makEGTransform(shape,rclTrf,op,copy);
            return true;
        }
    }
    makETransform(shape,convert(rclTrf),op,copy);
    return false;
}

TopoShape &TopoShape::makETransform(const TopoShape &shape, const gp_Trsf &trsf, const char *op, bool copy) {
    if(!copy) {
        // OCCT checks the ScaleFactor against gp::Resolution() which is DBL_MIN!!!
        copy = trsf.ScaleFactor()*trsf.HVectorialPart().Determinant() < 0. ||
               Abs(Abs(trsf.ScaleFactor()) - 1) > Precision::Confusion();
    }
    TopoShape tmp(shape);
    if(copy) {
        if(shape.isNull())
            HANDLE_NULL_INPUT;

        BRepBuilderAPI_Transform mkTrf(shape.getShape(), trsf, Standard_True);
        // TODO: calling Moved() is to make sure the shape has some Location,
        // which is necessary for STEP export to work. However, if we reach
        // here, it porabably means BRepBuilderAPI_Transform has modified
        // underlying shapes (because of scaling), it will break compound child
        // parent relationship anyway. In short, STEP import/export will most
        // likely break badly if there is any scaling involved
        tmp.setShape(mkTrf.Shape().Moved(gp_Trsf()), false);
    } else
        tmp.move(trsf);

    if(op || (shape.Tag && shape.Tag!=Tag)) {
        setShape(tmp._Shape);
        INIT_SHAPE_CACHE();
        if (!Hasher)
            Hasher = tmp.Hasher;
        copyElementMap(tmp, op);
    } else
        *this = tmp;
    return *this;
}

TopoShape &TopoShape::makEGTransform(const TopoShape &shape,
        const Base::Matrix4D &rclTrf, const char *op, bool copy)
{
    if(shape.isNull())
        HANDLE_NULL_INPUT;

    // if(!op) op = Part::OpCodes::Gtransform;
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
    TopoShape tmp(shape);
    BRepBuilderAPI_GTransform mkTrf(shape.getShape(), mat, copy);
    tmp.setShape(mkTrf.Shape(), false);
    if(op || (shape.Tag && shape.Tag!=Tag)) {
        setShape(tmp._Shape);
        INIT_SHAPE_CACHE();
        if (!Hasher)
            Hasher = tmp.Hasher;
        copyElementMap(tmp, op);
    } else
        *this = tmp;
    return *this;
}


TopoShape &TopoShape::makECopy(const TopoShape &shape, const char *op, bool copyGeom, bool copyMesh)
{
    if(shape.isNull())
        return *this;

    TopoShape tmp(shape);
#if OCC_VERSION_HEX >= 0x070000
    tmp.setShape(BRepBuilderAPI_Copy(shape.getShape(),copyGeom,copyMesh).Shape(), false);
#else
    tmp.setShape(BRepBuilderAPI_Copy(shape.getShape()).Shape(), false);
#endif
    if(op || (shape.Tag && shape.Tag!=Tag)) {
        setShape(tmp._Shape);
        INIT_SHAPE_CACHE();
        if (!Hasher)
            Hasher = tmp.Hasher;
        copyElementMap(tmp, op);
    }else
        *this = tmp;
    return *this;
}

static std::vector<TopoShape> prepareProfiles(const std::vector<TopoShape> &shapes,size_t offset=0) {
    std::vector<TopoShape> ret;
    for(size_t i=offset;i<shapes.size();++i) {
        auto sh = shapes[i];
        if(sh.isNull())
            HANDLE_NULL_INPUT;
        auto shape = sh.getShape();
        // Allow compounds with a single face, wire or vertex or
        // if there are only edges building one wire
        if (shape.ShapeType() == TopAbs_COMPOUND) {
            sh = sh.makEWires();
            if(sh.isNull())
                HANDLE_NULL_INPUT;
            shape = sh.getShape();
        }
        if (shape.ShapeType() == TopAbs_FACE) {
            shape = sh.splitWires().getShape();
        } else if (shape.ShapeType() == TopAbs_WIRE) {
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Wire(shape));
            shape = mkWire.Wire();
        } else if (shape.ShapeType() == TopAbs_EDGE) {
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(shape));
            shape = mkWire.Wire();
        } else if (shape.ShapeType() != TopAbs_VERTEX) {
            FC_THROWM(Base::CADKernelError,"Profile shape is not a vertex, edge, wire nor face.");
        }
        ret.push_back(shape);
    }
    if(ret.empty())
        FC_THROWM(Base::CADKernelError,"No profile");
    return ret;
}

TopoShape &TopoShape::makEPipeShell( const std::vector<TopoShape> &shapes,
                                     const Standard_Boolean make_solid,
                                     const Standard_Boolean isFrenet,
                                     TransitionMode transition,
                                     const char *op,
                                     double tol3d,
                                     double tolBound,
                                     double tolAngular)
{
    if(!op) op = Part::OpCodes::PipeShell;

    if(shapes.size()<2)
        FC_THROWM(Base::CADKernelError,"Not enough input shape");

    auto spine = shapes.front().makEWires();
    if(spine.isNull())
        HANDLE_NULL_INPUT;
    if(spine.getShape().ShapeType()!=TopAbs_WIRE)
        FC_THROWM(Base::CADKernelError,"Spine shape cannot form a single wire");

    BRepOffsetAPI_MakePipeShell mkPipeShell(TopoDS::Wire(spine.getShape()));
    BRepBuilderAPI_TransitionMode transMode;
    switch (transition) {
        case TransitionMode::RightCorner: transMode = BRepBuilderAPI_RightCorner;
            break;
        case TransitionMode::RoundCorner: transMode = BRepBuilderAPI_RoundCorner;
            break;
        default: transMode = BRepBuilderAPI_Transformed;
            break;
    }
    mkPipeShell.SetMode(isFrenet);
    mkPipeShell.SetTransitionMode(transMode);
    if (tol3d != 0.0 || tolBound != 0.0 || tolAngular != 0.0) {
        if (tol3d == 0.0)
            tol3d = 1e-4;
        if (tolBound == 0.0)
            tolBound = 1e-4;
        if (tolAngular == 0.0)
            tolAngular = 1e-2;
        mkPipeShell.SetTolerance(tol3d, tolBound, tolAngular);
    }

    for(auto &sh : prepareProfiles(shapes,1))
        mkPipeShell.Add(sh.getShape());

    if (!mkPipeShell.IsReady())
        FC_THROWM(Base::CADKernelError,"shape is not ready to build");
    else
        mkPipeShell.Build();

    if (make_solid)	mkPipeShell.MakeSolid();

    return makEShape(mkPipeShell,shapes,op);
}

TopoShape &TopoShape::makEEvolve(const TopoShape &spine,
                                 const TopoShape &profile,
                                 JoinType join,
                                 bool axeProf,
                                 bool solid,
                                 bool profOnSpine,
                                 double tol,
                                 const char *op)
{
    if (!op)
        op = Part::OpCodes::Evolve;
    if (tol == 0.0)
        tol = 1e-6;

    GeomAbs_JoinType joinType;
    switch (join) {
        case JoinType::Arc:
        joinType = GeomAbs_Tangent;
        break;
    case JoinType::Intersection:
        joinType = GeomAbs_Intersection;
        break;
    default:
        joinType = GeomAbs_Arc;
        break;
    }

    TopoDS_Shape spineShape;
    if (spine.countSubShapes(TopAbs_FACE) > 0)
        spineShape = spine.getSubShape(TopAbs_FACE, 1);
    else if (spine.countSubShapes(TopAbs_WIRE) > 0)
        spineShape = spine.getSubShape(TopAbs_WIRE, 1);
    else if (spine.countSubShapes(TopAbs_EDGE) > 0)
        spineShape = BRepBuilderAPI_MakeWire(TopoDS::Edge(spine.getSubShape(TopAbs_EDGE, 1))).Wire();
    if (spineShape.IsNull() || !BRepBuilderAPI_FindPlane(spineShape).Found())
        FC_THROWM(Base::CADKernelError, "Expect the the spine to be a planar wire or face");

    TopoDS_Shape profileShape;
    if (profile.countSubShapes(TopAbs_FACE) > 0 || profile.countSubShapes(TopAbs_WIRE) > 0)
        profileShape = profile.getSubShape(TopAbs_WIRE, 1);
    else if (profile.countSubShapes(TopAbs_EDGE) > 0)
        profileShape = BRepBuilderAPI_MakeWire(TopoDS::Edge(profile.getSubShape(TopAbs_EDGE, 1))).Wire();
    if (profileShape.IsNull() || !BRepBuilderAPI_FindPlane(profileShape).Found()) {
        if (profileShape.IsNull()
                || profile.countSubShapes(TopAbs_EDGE) > 1
                || !profile.getSubTopoShape(TopAbs_EDGE, 1).isLinearEdge())
        {
            FC_THROWM(Base::CADKernelError, "Expect the the profile to be a planar wire or face or a line");
        }
    }
    if (spineShape.ShapeType() == TopAbs_FACE) {
        BRepOffsetAPI_MakeEvolved maker(TopoDS::Face(spineShape),
                TopoDS::Wire(profileShape), joinType,
                axeProf ? Standard_True : Standard_False,
                solid ? Standard_True : Standard_False,
                profOnSpine ? Standard_True : Standard_False,
                tol);
        return makEShape(maker, {spine, profile}, op);
    }
    else {
        BRepOffsetAPI_MakeEvolved maker(TopoDS::Wire(spineShape),
                TopoDS::Wire(profileShape), joinType,
                axeProf ? Standard_True : Standard_False,
                solid ? Standard_True : Standard_False,
                profOnSpine ? Standard_True : Standard_False,
                tol);
        return makEShape(maker, {spine, profile}, op);
    }
}

TopoShape &TopoShape::makERuledSurface(const std::vector<TopoShape> &shapes,
        int orientation, const char *op)
{
    if(!op)
        op = Part::OpCodes::RuledSurface;

    if(shapes.size()!=2)
        FC_THROWM(Base::CADKernelError,"Wrong number of input shape");

    std::vector<TopoShape> curves(2);
    int i=0;
    for(auto &s : shapes) {
        if(s.isNull())
            HANDLE_NULL_INPUT;
        auto type = s.shapeType();
        if(type == TopAbs_WIRE || type == TopAbs_EDGE) {
            curves[i++] = s;
            continue;
        }
        auto count = s.countSubShapes(TopAbs_WIRE);
        if(count>1)
            FC_THROWM(Base::CADKernelError,"Input shape has more than one wire");
        if(count==1) {
            curves[i++] = s.getSubTopoShape(TopAbs_WIRE,1);
            continue;
        }
        count = s.countSubShapes(TopAbs_EDGE);
        if(count==0)
            FC_THROWM(Base::CADKernelError,"Input shape has no edge");
        if(count == 1) {
            curves[i++] = s.getSubTopoShape(TopAbs_EDGE,1);
            continue;
        }
        curves[i] = s.makEWires();
        if(curves[i].isNull())
            HANDLE_NULL_INPUT;
        if(curves[i].shapeType()!=TopAbs_WIRE)
            FC_THROWM(Base::CADKernelError,"Input shape forms more than one wire");
        ++i;
    }

    if(curves[0].shapeType()!=curves[1].shapeType()) {
        for(auto &curve : curves) {
            if(curve.shapeType() == TopAbs_EDGE)
                curve = curve.makEWires();
        }
    }

    auto &S1 = curves[0];
    auto &S2 = curves[1];
    bool isWire = S1.shapeType()==TopAbs_WIRE;

    // https://forum.freecadweb.org/viewtopic.php?f=8&t=24052
    //
    // if both shapes are sub-elements of one common shape then the fill
    // algorithm leads to problems if the shape has set a placement. The
    // workaround is to copy the sub-shape
    S1 = S1.makECopy();
    S2 = S2.makECopy();

    if (orientation == 0) {
        // Automatic
        Handle(Adaptor3d_HCurve) a1;
        Handle(Adaptor3d_HCurve) a2;
        if (!isWire) {
            BRepAdaptor_Curve adapt1(TopoDS::Edge(S1.getShape()));
            BRepAdaptor_Curve adapt2(TopoDS::Edge(S2.getShape()));
            a1 = new BRepAdaptor_HCurve(adapt1);
            a2 = new BRepAdaptor_HCurve(adapt2);
        }
        else {
            BRepAdaptor_CompCurve adapt1(TopoDS::Wire(S1.getShape()));
            BRepAdaptor_CompCurve adapt2(TopoDS::Wire(S2.getShape()));
            a1 = new BRepAdaptor_HCompCurve(adapt1);
            a2 = new BRepAdaptor_HCompCurve(adapt2);
        }

        if (!a1.IsNull() && !a2.IsNull()) {
            // get end points of 1st curve
            gp_Pnt p1 = a1->Value(a1->FirstParameter());
            gp_Pnt p2 = a1->Value(a1->LastParameter());
            if (S1.getShape().Orientation() == TopAbs_REVERSED) {
                std::swap(p1, p2);
            }

            // get end points of 2nd curve
            gp_Pnt p3 = a2->Value(a2->FirstParameter());
            gp_Pnt p4 = a2->Value(a2->LastParameter());
            if (S2.getShape().Orientation() == TopAbs_REVERSED) {
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
                S2.setShape(S2.getShape().Reversed(), false);
            }
        }
    }
    else if (orientation == 2) {
        // Reverse
        S2.setShape(S2.getShape().Reversed(), false);
    }

    TopoDS_Shape ruledShape;
    if (!isWire) {
        ruledShape = BRepFill::Face(TopoDS::Edge(S1.getShape()), TopoDS::Edge(S2.getShape()));
    }
    else {
        ruledShape = BRepFill::Shell(TopoDS::Wire(S1.getShape()), TopoDS::Wire(S2.getShape()));
    }
    
    // Both BRepFill::Face() and Shell() modifies the original input edges
    // without any API to provide relationship to the output edges. So we have
    // to use searchSubShape() to build the relationship by ourselves.

    TopoShape res(ruledShape.Located(TopLoc_Location()));
    std::vector<TopoShape> edges;
    for (const auto &c : curves) {
        for (const auto &e : c.getSubTopoShapes(TopAbs_EDGE)) {
            auto found = res.searchSubShape(e);
            if (found.size() > 0) {
                found.front().resetElementMap(e.elementMap());
                edges.push_back(found.front());
            }
        }
    }
    // Use empty mapper and let makEShape name the created surface with lower elements.
    return makESHAPE(res.getShape(),Mapper(),edges,op);
}

const std::vector<TopoDS_Shape> &
MapperMaker::modified(const TopoDS_Shape &s) const
{
    _res.clear();
    try {
        TopTools_ListIteratorOfListOfShape it;
        for (it.Initialize(maker.Modified(s)); it.More(); it.Next())
            _res.push_back(it.Value());
    } catch (const Standard_Failure & e) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("Exception on shape mapper: " << e.GetMessageString());
    }
    return _res;
}

const std::vector<TopoDS_Shape> &
MapperMaker::generated(const TopoDS_Shape &s) const
{
    _res.clear();
    try {
        TopTools_ListIteratorOfListOfShape it;
        for (it.Initialize(maker.Generated(s)); it.More(); it.Next())
            _res.push_back(it.Value());
    } catch (const Standard_Failure & e) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("Exception on shape mapper: " << e.GetMessageString());
    }
    return _res;
}

MapperHistory::MapperHistory(const Handle(BRepTools_History) &history)
    :history(history)
{}

MapperHistory::MapperHistory(const Handle(BRepTools_ReShape) &reshape)
{
    if (reshape)
        history = reshape->History();
}

MapperHistory::MapperHistory(ShapeFix_Root &fix)
{
    if (fix.Context())
        history = fix.Context()->History();
}
    
const std::vector<TopoDS_Shape> &
MapperHistory::modified(const TopoDS_Shape &s) const
{
    _res.clear();
    try {
        if (history) {
            TopTools_ListIteratorOfListOfShape it;
            for (it.Initialize(history->Modified(s)); it.More(); it.Next())
                _res.push_back(it.Value());
        }
    } catch (const Standard_Failure & e) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("Exception on shape mapper: " << e.GetMessageString());
    }
    return _res;
}

const std::vector<TopoDS_Shape> &
MapperHistory::generated(const TopoDS_Shape &s) const
{
    _res.clear();
    try {
        if (history) {
            TopTools_ListIteratorOfListOfShape it;
            for (it.Initialize(history->Generated(s)); it.More(); it.Next())
                _res.push_back(it.Value());
        }
    } catch (const Standard_Failure & e) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("Exception on shape mapper: " << e.GetMessageString());
    }
    return _res;
}

struct MapperThruSections: MapperMaker {
    TopoShape firstProfile;
    TopoShape lastProfile;

    MapperThruSections(BRepOffsetAPI_ThruSections &tmaker,
            const std::vector<TopoShape> &profiles)
        :MapperMaker(tmaker)
    {
        if(!tmaker.FirstShape().IsNull())
            firstProfile = profiles.front();
        if(!tmaker.LastShape().IsNull())
            lastProfile = profiles.back();
    }
    virtual const std::vector<TopoDS_Shape> &generated(const TopoDS_Shape &s) const override {
        MapperMaker::generated(s);
        if(_res.size()) return _res;
        try {
            auto &tmaker = static_cast<BRepOffsetAPI_ThruSections&>(maker);
            auto shape = tmaker.GeneratedFace(s);
            if(!shape.IsNull())
                _res.push_back(shape);
            if(firstProfile.getShape().IsSame(s) || firstProfile.findShape(s))
                _res.push_back(tmaker.FirstShape());
            else if(lastProfile.getShape().IsSame(s) || lastProfile.findShape(s))
                _res.push_back(tmaker.LastShape());
        } catch (const Standard_Failure & e) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_WARN("Exception on shape mapper: " << e.GetMessageString());
        }
        return _res;
    }
};

TopoShape &TopoShape::makEShape(BRepOffsetAPI_ThruSections &mk, const TopoShape &source,
        const char *op)
{
    if(!op) op = Part::OpCodes::ThruSections;
    return makEShape(mk,std::vector<TopoShape>(1,source),op);
}

TopoShape &TopoShape::makEShape(BRepOffsetAPI_ThruSections &mk, const std::vector<TopoShape> &sources,
        const char *op)
{
    if(!op) op = Part::OpCodes::ThruSections;
    return makESHAPE(mk.Shape(),MapperThruSections(mk,sources),sources,op);
}

TopoShape &TopoShape::makELoft(const std::vector<TopoShape> &shapes,
                               Standard_Boolean isSolid,
                               Standard_Boolean isRuled,
                               Standard_Boolean isClosed,
                               Standard_Integer maxDegree,
                               const char *op)
{
    if(!op) op = Part::OpCodes::Loft;

    // http://opencascade.blogspot.com/2010/01/surface-modeling-part5.html
    BRepOffsetAPI_ThruSections aGenerator (isSolid,isRuled);
    aGenerator.SetMaxDegree(maxDegree);

    auto profiles = prepareProfiles(shapes);
    if (shapes.size() < 2)
        FC_THROWM(Base::CADKernelError,"Need at least two vertices, edges or wires to create loft face");

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
    return makESHAPE(aGenerator.Shape(),MapperThruSections(aGenerator,profiles),shapes,op);
}

TopoShape &TopoShape::makEPrism(const TopoShape &base, const gp_Vec& vec, const char *op) {
    if(!op) op = Part::OpCodes::Extrude;
    if(base.isNull())
        HANDLE_NULL_SHAPE;
    BRepPrimAPI_MakePrism mkPrism(base.getShape(), vec);
    return makEShape(mkPrism,base,op);
}

void GenericShapeMapper::init(const TopoShape &src, const TopoDS_Shape &dst)
{
    for (TopExp_Explorer exp(dst, TopAbs_FACE); exp.More(); exp.Next()) {
        const TopoDS_Shape &dstFace = exp.Current();
        if (src.findShape(dstFace))
            continue;

        std::unordered_map<TopoDS_Shape, int> map;
        bool found = false;

        // Try to find a face in the src that shares at least two edges (or one
        // closed edge) with dstFace.
        // TODO: consider degenerative cases of two or more edges on the same line.
        for (TopExp_Explorer it(dstFace, TopAbs_EDGE); it.More(); it.Next()) {
            int idx = src.findShape(it.Current());
            if (!idx)
                continue;
            TopoDS_Edge e = TopoDS::Edge(it.Current());
#if OCC_VERSION_HEX >= 0x070000
            if(BRep_Tool::IsClosed(e))
#else
            p1 = BRep_Tool::Pnt(TopExp::FirstVertex(e));
            p2 = BRep_Tool::Pnt(TopExp::LastVertex(e));
            if(p1.SquareDistance(p2)<1e-14)
#endif
            {
                // closed edge, one face is enough
                TopoDS_Shape face = src.findAncestorShape(
                        src.getSubShape(TopAbs_EDGE,idx), TopAbs_FACE);
                if (!face.IsNull()) {
                    this->insert(false, face, dstFace);
                    found = true;
                    break;
                }
                continue;
            }
            for (auto &face : src.findAncestorsShapes(src.getSubShape(TopAbs_EDGE,idx), TopAbs_FACE)) {
                int &cnt = map[face];
                if (++cnt == 2) {
                    this->insert(false, face, dstFace);
                    found = true;
                    break;
                }
                if (found)
                break;
            }
        }

        if (found) continue;

        // if no face matches, try search by geometry surface
        std::unique_ptr<Geometry> g(Geometry::fromShape(dstFace));
        if (!g) continue;

        for (auto &v : map) {
            std::unique_ptr<Geometry> g2(Geometry::fromShape(v.first));
            if (g2 && g2->isSame(*g,1e-7,1e-12)) {
                this->insert(false, v.first, dstFace);
                break;
            }
        }
    }
}

TopoShape &TopoShape::makEPrismUntil(const TopoShape &_base,
                                     const TopoShape& profile,
                                     const TopoShape& supportFace,
                                     const TopoShape& __uptoface,
                                     const gp_Dir& direction,
                                     PrismMode Mode,
                                     Standard_Boolean checkLimits,
                                     const char *op)
{
    if(!op) op = Part::OpCodes::Prism;

    BRepFeat_MakePrism PrismMaker;

    TopoShape _uptoface(__uptoface);
    if (checkLimits && _uptoface.shapeType(true) == TopAbs_FACE
                    && !BRep_Tool::NaturalRestriction(TopoDS::Face(_uptoface.getShape()))) {
        // When using the face with BRepFeat_MakePrism::Perform(const TopoDS_Shape& Until)
        // then the algorithm expects that the 'NaturalRestriction' flag is set in order
        // to work as expected.
        BRep_Builder builder;
        _uptoface = _uptoface.makECopy();
        builder.NaturalRestriction(TopoDS::Face(_uptoface.getShape()), Standard_True);
    }

    TopoShape uptoface(_uptoface);
    TopoShape base(_base);

    if (base.isNull()) {
        Mode = PrismMode::None;
        base = profile;
    }

    // Check whether the face has limits or not. Unlimited faces have no wire
    // Note: Datum planes are always unlimited
    if (checkLimits && uptoface.hasSubShape(TopAbs_WIRE)) {
        TopoDS_Face face = TopoDS::Face(uptoface.getShape());
        bool remove_limits = false;
        // Remove the limits of the upToFace so that the extrusion works even if profile is larger
        // than the upToFace
        for (auto &sketchface : profile.getSubTopoShapes(TopAbs_FACE)) {
            // Get outermost wire of sketch face
            TopoShape outerWire = sketchface.splitWires();
            BRepProj_Projection proj(TopoDS::Wire(outerWire.getShape()), face, direction);
            if (!proj.More() || !proj.Current().Closed()) {
                remove_limits = true;
                break;
            }
        }

        // It must also be checked that all projected inner wires of the upToFace
        // lie outside the sketch shape. If this is not the case then the sketch
        // shape is not completely covered by the upToFace. See #0003141
        if (!remove_limits) {
            std::vector<TopoShape> wires;
            uptoface.splitWires(&wires);
            for (auto & w : wires) {
                BRepProj_Projection proj(TopoDS::Wire(w.getShape()), profile.getShape(), -direction);
                if (proj.More()) {
                    remove_limits = true;
                    break;
                }
            }
        }

        if (remove_limits) {
            // Note: Using an unlimited face every time gives unnecessary failures for concave faces
            TopLoc_Location loc = face.Location();
            BRepAdaptor_Surface adapt(face, Standard_False);
            // use the placement of the adapter, not of the upToFace
            loc = TopLoc_Location(adapt.Trsf());
            BRepBuilderAPI_MakeFace mkFace(adapt.Surface().Surface()
#if OCC_VERSION_HEX >= 0x060502
                    , Precision::Confusion()
#endif
            );
            if (!mkFace.IsDone()) 
                remove_limits = false;
            else
                uptoface.setShape(located(mkFace.Shape(),loc), false);
        }
    }

    TopoShape uptofaceCopy = uptoface;
    bool checkBase = false;
    auto retry = [&]() {
        if (!uptoface.isSame(_uptoface)) {
            // retry using the original up to face in case unnecessary failure
            // due to removing the limits
            uptoface = _uptoface;
            return true;
        }
        if ((!_base.isNull() && base.isSame(_base))
                || (_base.isNull() && base.isSame(profile))) {
            // It is unclear under exactly what condition extrude up to face
            // can fail. Either the support face or the up to face must be part
            // of the base, or maybe some thing else.
            // 
            // To deal with it, we retry again by disregard the supplied base,
            // and use up to face to extrude our own base. Later on, use the
            // supplied base (i.e. _base) to calculate the final shape if the
            // mode is FuseWithBase or CutWithBase, 
            checkBase = true;
            uptoface = uptofaceCopy;
            base.makEPrism(_uptoface, direction);
            return true;
        }
        return false;
    };

    std::vector<TopoShape> srcShapes;
    TopoShape result;
    for (;;) {
        try {
            result = base;

            // We do not rely on BRepFeat_MakePrism to perform fuse or cut for
            // us because of its poor support of shape history.
            auto mode = PrismMode::None;

            for (auto &face : profile.getSubTopoShapes(
                        profile.hasSubShape(TopAbs_FACE)?TopAbs_FACE:TopAbs_WIRE)) {
                srcShapes.clear();
                if (!profile.isNull() && !result.findShape(profile.getShape()))
                    srcShapes.push_back(profile);
                if (!supportFace.isNull() && !result.findShape(supportFace.getShape()))
                    srcShapes.push_back(supportFace);

                // DO NOT include uptoface for element mapping. Because OCCT
                // BRepFeat_MakePrism will report all top extruded face being
                // modified by the uptoface. If there are more than one face in
                // the profile, this will cause uncessary duplicated element
                // mapped name. And will also disrupte element history tracing
                // back to the profile sketch.
                //
                // if (!uptoface.isNull() && !this->findShape(uptoface.getShape()))
                //     srcShapes.push_back(uptoface);

                srcShapes.push_back(result);

                PrismMaker.Init(result.getShape(), face.getShape(),
                        TopoDS::Face(supportFace.getShape()), direction, mode, Standard_False);
                mode = PrismMode::FuseWithBase;

                PrismMaker.Perform(uptoface.getShape());

                if (!PrismMaker.IsDone() || PrismMaker.Shape().IsNull())
                    FC_THROWM(Base::CADKernelError,"BRepFeat_MakePrism: extrusion failed");

                result.makEShape(PrismMaker, srcShapes, uptoface, op);
            }
            break;
        } catch (Base::Exception &) {
            if (!retry()) throw;
        } catch (Standard_Failure &) {
            if (!retry()) throw;
        }
    }

    if (!_base.isNull() && Mode != PrismMode::None) {
        if (Mode == PrismMode::FuseWithBase)
            result.makEFuse({_base, result});
        else
            result.makECut({_base, result});
    }

    *this = result;
    return *this;
}

TopoShape &TopoShape::makERevolve(const TopoShape &_base, const gp_Ax1& axis,
        double d, const char *face_maker, const char *op)
{
    if(!op) op = Part::OpCodes::Revolve;

    TopoShape base(_base);
    if(base.isNull())
        HANDLE_NULL_SHAPE;

    if(face_maker && !base.hasSubShape(TopAbs_FACE)) {
        if(!base.hasSubShape(TopAbs_WIRE))
            base = base.makEWires();
        base = base.makEFace(0,face_maker);
    }
    BRepPrimAPI_MakeRevol mkRevol(base.getShape(), axis,d);
    return makEShape(mkRevol,base,op);
}

TopoShape &TopoShape::makEMirror(const TopoShape &shape, const gp_Ax2 &ax2, const char *op) {
    if(!op) op = Part::OpCodes::Mirror;

    if(shape.isNull())
        HANDLE_NULL_SHAPE;

    gp_Trsf mat;
    mat.SetMirror(ax2);
    TopLoc_Location loc = shape.getShape().Location();
    gp_Trsf placement = loc.Transformation();
    mat = placement * mat;
    BRepBuilderAPI_Transform mkTrf(shape.getShape(), mat);
    return makEShape(mkTrf,shape,op);
}

struct MapperSewing: Part::TopoShape::Mapper {
    BRepBuilderAPI_Sewing &maker;
    MapperSewing(BRepBuilderAPI_Sewing &maker)
        :maker(maker)
    {}
    virtual const std::vector<TopoDS_Shape> &modified(const TopoDS_Shape &s) const override {
        _res.clear();
        try {
            const auto &shape = maker.Modified(s);
            if(!shape.IsNull() && !shape.IsSame(s))
                _res.push_back(shape);
            else {
                const auto &sshape = maker.ModifiedSubShape(s);
                if(!sshape.IsNull() && !sshape.IsSame(s))
                    _res.push_back(sshape);
            }
        } catch (const Standard_Failure & e) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_WARN("Exception on shape mapper: " << e.GetMessageString());
        }
        return _res;
    }
};

TopoShape &TopoShape::makEShape(BRepBuilderAPI_Sewing &mk, const std::vector<TopoShape> &shapes,
        const char *op)
{
    if(!op) op = Part::OpCodes::Sewing;
    return makESHAPE(mk.SewedShape(),MapperSewing(mk),shapes,op);
}

TopoShape &TopoShape::makEShape(BRepBuilderAPI_Sewing &mkShape,
            const TopoShape &source, const char *op)
{
    if(!op) op = Part::OpCodes::Sewing;
    return makEShape(mkShape,std::vector<TopoShape>(1,source),op);
}

TopoShape &TopoShape::makEOffset(const TopoShape &shape,
        double offset, double tol, bool intersection, bool selfInter,
        short offsetMode, JoinType join, bool fill, const char *op)
{
    if(!op) op = Part::OpCodes::Offset;

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
        FC_THROWM(Base::CADKernelError,"BRepOffsetAPI_MakeOffsetShape not done");

    TopoShape res(Tag,Hasher);
    res.makEShape(mkOffset,shape,op);
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
        FC_THROWM(Base::CADKernelError,"no closed bounds");
    }

    BRep_Builder builder;
    std::vector<TopoShape> shapes;
    for (int index = 1; index <= freeCheck.NbClosedFreeBounds(); ++index)
    {
        TopoShape originalWire(shape.Tag,shape.Hasher,freeCheck.ClosedFreeBound(index)->FreeBound());
        originalWire.mapSubElement(shape);
        const BRepAlgo_Image& img = mkOffset.MakeOffset().OffsetEdgesFromShapes();

        //build offset wire.
        TopoDS_Wire offsetWire;
        builder.MakeWire(offsetWire);
        for(const auto &s : originalWire.getSubShapes(TopAbs_EDGE)) {
            if (!img.HasImage(s))
            {
                FC_THROWM(Base::CADKernelError,"no image for shape");
            }
            const TopTools_ListOfShape& currentImage = img.Image(s);
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
                FC_THROWM(Base::CADKernelError,stream.str().c_str());
            }
            builder.Add(offsetWire, mappedEdge);
        }
        std::vector<TopoShape> wires;
        wires.push_back(originalWire);
        wires.push_back(TopoShape(Tag,Hasher,offsetWire));
        wires.back().mapSubElement(res);

        //It would be nice if we could get thruSections to build planar faces
        //in all areas possible, so we could run through refine. I tried setting
        //ruled to standard_true, but that didn't have the desired affect.
        BRepOffsetAPI_ThruSections aGenerator;
        aGenerator.AddWire(TopoDS::Wire(originalWire.getShape()));
        aGenerator.AddWire(offsetWire);
        aGenerator.Build();
        if (!aGenerator.IsDone())
        {
            FC_THROWM(Base::CADKernelError,"ThruSections failed");
        }

        shapes.push_back(TopoShape(Tag,Hasher).makEShape(aGenerator,wires));
    }

    TopoShape perimeterCompound(Tag,Hasher);
    perimeterCompound.makECompound(shapes,op);

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
    *this = TopoShape(Tag,Hasher).makESHAPE(outputShape,MapperSewing(sewTool),shapes,op);
    return *this;
}

TopoShape &TopoShape::makEOffsetFace(const TopoShape &shape,
                                     double offset,
                                     double innerOffset,
                                     JoinType joinType,
                                     JoinType innerJoinType,
                                     const char *op)
{
    if (std::abs(innerOffset) < Precision::Confusion()
            && std::abs(offset) < Precision::Confusion()) {
        *this = shape;
        return *this;
    }

    if (shape.isNull())
        FC_THROWM(Base::ValueError, "makeOffsetFace: input shape is null!");
    if (!shape.hasSubShape(TopAbs_FACE))
        FC_THROWM(Base::ValueError, "makeOffsetFace: no face found");

    std::vector<TopoShape> res;
    for (auto & face : shape.getSubTopoShapes(TopAbs_FACE)) {
        std::vector<TopoShape> wires;
        TopoShape outerWire = face.splitWires(&wires, ReorientForward);
        if (wires.empty()) {
            res.push_back(makEOffset2D(face, offset, joinType, false, false, false, op));
            continue;
        }
        if (outerWire.isNull())
            FC_THROWM(Base::CADKernelError, "makeOffsetFace: missing outer wire!");

        if (std::abs(offset) > Precision::Confusion())
            outerWire = outerWire.makEOffset2D(offset, joinType, false, false, false, op);

        if (std::abs(innerOffset) > Precision::Confusion()) {
            TopoShape innerWires(0, Hasher);
            innerWires.makECompound(wires, "", false);
            innerWires = innerWires.makEOffset2D(innerOffset, innerJoinType, false, false, true, op);
            wires = innerWires.getSubTopoShapes(TopAbs_WIRE);
        }
        wires.push_back(outerWire);
        gp_Pln pln;
        res.push_back(TopoShape(0, Hasher).makEFace(wires,
                                                    nullptr,
                                                    nullptr,
                                                    face.findPlane(pln) ? &pln : nullptr));
    }
    return makECompound(res, "", false);
}

TopoShape &TopoShape::makEOffset2D(const TopoShape &shape, double offset, JoinType joinType,
        bool fill, bool allowOpenResult, bool intersection, const char *op)
{
    if(!op) op = Part::OpCodes::Offset2D;

    if(shape.isNull())
        FC_THROWM(Base::ValueError, "makeOffset2D: input shape is null!");
    if (allowOpenResult && OCC_VERSION_HEX < 0x060900)
        FC_THROWM(Base::AttributeError, "openResult argument is not supported on OCC < 6.9.0.");

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
                                s, offset, joinType, fill, allowOpenResult, intersection, op));
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
                    auto outerWire = s.splitWires(&sourceWires);
                    sourceWires.push_back(outerWire);
                    haveFaces = true;
                }break;
                default:
                    FC_THROWM(Base::TypeError, "makeOffset2D: input shape is not an edge, wire or face or compound of those.");
                break;
            }
        }
        if (haveWires && haveFaces)
            FC_THROWM(Base::TypeError, "makeOffset2D: collective offset of a mix of wires and faces is not supported");
        if (haveFaces)
            allowOpenResult = false;

        //find plane.
        gp_Pln workingPlane;
        if (!TopoShape().makECompound(sourceWires,"",false).findPlane(workingPlane))
            FC_THROWM(Base::CADKernelError,"makeOffset2D: wires are nonplanar or noncoplanar");

        //do the offset..
        TopoShape offsetShape;
        if (fabs(offset) > Precision::Confusion()){
            BRepOffsetAPI_MakeOffsetFix mkOffset(GeomAbs_JoinType(joinType), allowOpenResult);
            for(auto &w : sourceWires) {
                mkOffset.AddWire(TopoDS::Wire(w.getShape()));
            }
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
                FC_THROWM(Base::CADKernelError,"BRepOffsetAPI_MakeOffset has crashed! (Unknown exception caught)");
            }
            if(mkOffset.Shape().IsNull())
                FC_THROWM(NullShapeException, "makeOffset2D: result of offsetting is null!");

            //Copying shape to fix strange orientation behavior, OCC7.0.0. See bug #2699
            // http://www.freecadweb.org/tracker/view.php?id=2699
            offsetShape = shape.makEShape(mkOffset,op).makECopy();

        } else {
            offsetShape = TopoShape(Tag,Hasher).makECompound(sourceWires,0,false);
        }

        std::vector<TopoShape> offsetWires;
        //interestingly, if wires are removed, empty compounds are returned by MakeOffset (as of OCC 7.0.0)
        //so, we just extract all nesting
        expandCompound(offsetShape,offsetWires);
        if (offsetWires.empty())
            FC_THROWM(Base::CADKernelError, "makeOffset2D: offset result has no wires.");

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
                FC_THROWM(Base::ValueError, "makeOffset2D: offset distance is zero. Can't fill offset.");

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
                    FC_THROWM(Base::CADKernelError, "makeOffset2D: collective offset with filling of multiple wires is not supported yet.");

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
                if (v1.IsNull())  FC_THROWM(NullShapeException, "v1 is null");
                if (v2.IsNull())  FC_THROWM(NullShapeException, "v2 is null");
                if (v3.IsNull())  FC_THROWM(NullShapeException, "v3 is null");
                if (v4.IsNull())  FC_THROWM(NullShapeException, "v4 is null");

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
                    FC_THROWM(Base::CADKernelError, "makeOffset2D: fill offset: failed to establish open vertex relationship.");
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
        if (wiresForMakingFaces.size()>0) {
            TopoShape face(0, Hasher);
            face.makEFace(wiresForMakingFaces, nullptr, nullptr, &workingPlane);
            expandCompound(face, shapesToReturn);
        }
    }

    return makECompound(shapesToReturn,op,forceOutputCompound);
}

TopoShape &TopoShape::makEThickSolid(const TopoShape &shape,
        const std::vector<TopoShape> &faces, double offset, double tol, bool intersection,
        bool selfInter, short offsetMode, JoinType join, const char *op)
{
    if(!op) op = Part::OpCodes::Thicken;

    //we do not offer tangent join type
    switch(join) {
    case TopoShape::JoinType::Arc:
    case TopoShape::JoinType::Intersection:
        break;
    default:
        join = TopoShape::JoinType::Intersection;
    }

    if(shape.isNull())
        HANDLE_NULL_SHAPE;

    if(faces.empty())
        HANDLE_NULL_INPUT;

    if(fabs(offset) <= 2*tol) {
        *this = shape;
        return *this;
    }

    TopTools_ListOfShape remFace;
    for(auto &face : faces) {
        if(face.isNull())
            HANDLE_NULL_INPUT;
        if(!shape.findShape(face.getShape()))
            FC_THROWM(Base::CADKernelError,"face does not belong to the shape");
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
    return makEShape(mkThick,shape,op);
}

TopoShape &TopoShape::makEWires(const std::vector<TopoShape> &shapes,
                                const char *op,
                                double tol,
                                bool shared,
                                TopoShapeMap *output)
{
    if(shapes.empty())
        HANDLE_NULL_SHAPE;
    if(shapes.size() == 1)
        return makEWires(shapes[0],op,tol,shared,output);
    return makEWires(TopoShape(Tag).makECompound(shapes),op,tol,shared,output);
}

struct EdgePoints {
    gp_Pnt v1, v2;
    std::list<TopoShape>::iterator it;
    const TopoShape &edge;
    bool closed;

    EdgePoints(std::list<TopoShape>::iterator it, double tol)
        :it(it), edge(*it), closed(false)
    {
        TopExp_Explorer xp(it->getShape(),TopAbs_VERTEX);
        v1 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        xp.Next();
        if (xp.More()) {
            v2 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
            closed = (v2.SquareDistance(v1) <= tol);
        } else {
            v2 = v1;
            closed = true;
        }
    }
};

std::deque<TopoShape>
TopoShape::sortEdges(std::list<TopoShape>& edges, bool keepOrder, double tol)
{
    if (tol<Precision::Confusion()) tol = Precision::Confusion();
    double tol3d = tol * tol;

    std::list<EdgePoints>  edge_points;
    for (auto it = edges.begin(); it != edges.end(); ++it)
        edge_points.emplace_back(it, tol3d);

    std::deque<TopoShape> sorted;
    if (edge_points.empty())
        return sorted;

    gp_Pnt first, last;
    first = edge_points.front().v1;
    last  = edge_points.front().v2;

    sorted.push_back(edge_points.front().edge);
    edges.erase(edge_points.front().it);
    if (edge_points.front().closed)
        return sorted;

    edge_points.erase(edge_points.begin());

    auto reverseEdge = [](const TopoShape &edge) {
        Standard_Real first, last;
        const Handle(Geom_Curve) & curve = BRep_Tool::Curve(TopoDS::Edge(edge.getShape()), first, last);
        first = curve->ReversedParameter(first);
        last = curve->ReversedParameter(last);
        TopoShape res(BRepBuilderAPI_MakeEdge(curve->Reversed(), last, first));
        auto edgeName = Data::IndexedName::fromConst("Edge", 1);
        if (auto mapped = edge.getMappedName(edgeName))
            res.setElementName(edgeName, mapped);
        auto v1Name = Data::IndexedName::fromConst("Vertex", 1);
        auto v2Name = Data::IndexedName::fromConst("Vertex", 2);
        auto v1 = edge.getMappedName(v1Name);
        auto v2 = edge.getMappedName(v2Name);
        if (v1 && v2) {
            res.setElementName(v1Name, v2);
            res.setElementName(v2Name, v1);
        }
        else if (v1 && edge.countSubShapes(TopAbs_EDGE) == 1) {
            // It's possible an edge has only one vertex, so no need to reverse
            // the name
            res.setElementName(v1Name, v1);
        }
        else if (v1)
            res.setElementName(v2Name, v1);
        else if (v2)
            res.setElementName(v1Name, v2);
        return res;
    };

    while (!edge_points.empty()) {
        // search for adjacent edge
        std::list<EdgePoints>::iterator pEI;
        for (pEI = edge_points.begin(); pEI != edge_points.end(); ++pEI) {
            if (pEI->closed)
                continue;

            if (keepOrder && sorted.size() == 1) {
                if (pEI->v2.SquareDistance(first) <= tol3d
                        || pEI->v1.SquareDistance(first) <= tol3d) {
                    sorted[0] = reverseEdge(sorted[0]);
                    std::swap(first, last);
                }
            }

            if (pEI->v1.SquareDistance(last) <= tol3d) {
                last = pEI->v2;
                sorted.push_back(pEI->edge);
                edges.erase(pEI->it);
                edge_points.erase(pEI);
                pEI = edge_points.begin();
                break;
            }
            else if (pEI->v2.SquareDistance(first) <= tol3d) {
                sorted.push_front(pEI->edge);
                first = pEI->v1;
                edges.erase(pEI->it);
                edge_points.erase(pEI);
                pEI = edge_points.begin();
                break;
            }
            else if (pEI->v2.SquareDistance(last) <= tol3d) {
                last = pEI->v1;
                sorted.push_back(reverseEdge(pEI->edge));
                edges.erase(pEI->it);
                edge_points.erase(pEI);
                pEI = edge_points.begin();
                break;
            }
            else if (pEI->v1.SquareDistance(first) <= tol3d) {
                first = pEI->v2;
                sorted.push_back(reverseEdge(pEI->edge));
                edges.erase(pEI->it);
                edge_points.erase(pEI);
                pEI = edge_points.begin();
                break;
            }
        }

        if ((pEI == edge_points.end()) || (last.SquareDistance(first) <= tol3d)) {
            // no adjacent edge found or polyline is closed
            return sorted;
        }
    }

    return sorted;
}

TopoShape &TopoShape::makEOrderedWires(const std::vector<TopoShape> &shapes,
                                       const char *op,
                                       double tol,
                                       TopoShapeMap *output)
{
    if(!op) op = Part::OpCodes::Wire;
    if(tol<Precision::Confusion()) tol = Precision::Confusion();

    std::vector<TopoShape> wires;
    std::list<TopoShape> edge_list;

    auto shape = TopoShape().makECompound(shapes, "", false);
    for(auto &e : shape.getSubTopoShapes(TopAbs_EDGE))
        edge_list.push_back(e);

    while(edge_list.size()) {
        BRepBuilderAPI_MakeWire mkWire;
        std::vector<TopoShape> edges;
        for (auto &edge : sortEdges(edge_list, true, tol)) {
            edges.push_back(edge);
            mkWire.Add(TopoDS::Edge(edge.getShape()));
            // MakeWire will replace vertex of connected edge, which
            // effectively creat a new edge. So we need to update the shape
            // in order to preserve element mapping.
            edges.back().setShape(mkWire.Edge(), false);
            if (output)
                (*output)[edges.back()] = edge;
        }
        wires.push_back(mkWire.Wire());
        wires.back().mapSubElement(edges,op);
    }
    return makECompound(wires,0,false);
}

TopoShape &TopoShape::makEWires(const TopoShape &shape,
                                const char *op,
                                double tol,
                                bool shared,
                                TopoShapeMap *output)
{
    if(!op) op = Part::OpCodes::Wire;
    if(tol<Precision::Confusion()) tol = Precision::Confusion();

    if (shared) {
        // Can't use ShapeAnalysis_FreeBounds if not shared. It seems the output
        // edges are modified some how, and it is not obvious how to map the
        // resulting edges.
        Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
        Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
        for(TopExp_Explorer xp(shape.getShape(),TopAbs_EDGE);xp.More();xp.Next())
            hEdges->Append(xp.Current());
        if(!hEdges->Length())
            HANDLE_NULL_SHAPE;
        ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges, tol, Standard_True, hWires);
        if(!hWires->Length())
            HANDLE_NULL_SHAPE;

        std::vector<TopoShape> wires;
        for (int i=1; i<=hWires->Length(); i++) {
            auto wire = hWires->Value(i);
            wires.push_back(TopoShape(Tag,Hasher,wire));
        }
        shape.mapSubElementsTo(wires,op);
        return makECompound(wires,"",false);
    }

    std::vector<TopoShape> wires;
    std::list<TopoShape> edge_list;

    for(auto &e : shape.getSubTopoShapes(TopAbs_EDGE))
        edge_list.emplace_back(e);

    std::vector<TopoShape> edges;
    edges.reserve(edge_list.size());
    wires.reserve(edge_list.size());

    // sort them together to wires
    while (edge_list.size() > 0) {
        BRepBuilderAPI_MakeWire mkWire;
        // add and erase first edge
        edges.clear();
        edges.push_back(edge_list.front());
        mkWire.Add(TopoDS::Edge(edges.back().getShape()));
        edges.back().setShape(mkWire.Edge(),false);
        if (output)
            (*output)[edges.back()] = edge_list.front();
        edge_list.pop_front();

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
                    // MakeWire will replace vertex of connected edge, which
                    // effectively creat a new edge. So we need to update the
                    // shape in order to preserve element mapping.
                    edges.back().setShape(mkWire.Edge(),false);
                    if (output)
                        (*output)[edges.back()] = *it;
                    edge_list.erase(it);
                    new_wire = mkWire.Wire();
                    break;
                }
            }
        } while (found);

        // Fix any topological issues of the wire
        ShapeFix_Wire aFix;
        aFix.SetPrecision(tol);
        aFix.Load(new_wire);

        aFix.FixReorder();
        // Assuming FixReorder() just reorder and don't change the underlying
        // edges, we get the wire and do a name mapping now, as the following
        // two operations (FixConnected and FixClosed) may change the edges.
        wires.push_back(aFix.Wire());
        wires.back().mapSubElement(edges,op);

        aFix.FixConnected();
        aFix.FixClosed();
        // Now retrieve the shape and set it without touching element map
        wires.back().setShape(aFix.Wire(),false);
    }
    return makECompound(wires,0,false);
}

TopoShape &TopoShape::makEFace(const TopoShape &shape,
                               const char *op,
                               const char *maker,
                               const gp_Pln *pln)
{
    std::vector<TopoShape> shapes;
    if (shape.isNull())
        HANDLE_NULL_SHAPE;
    if(shape.getShape().ShapeType() == TopAbs_COMPOUND)
        shapes = shape.getSubTopoShapes();
    else
        shapes.push_back(shape);
    return makEFace(shapes,op,maker,pln);
}

TopoShape &TopoShape::makEFace(const std::vector<TopoShape> &shapes,
                               const char *op,
                               const char *maker,
                               const gp_Pln *pln)
{
    if(!maker || !maker[0]) maker = "Part::FaceMakerBullseye";
    std::unique_ptr<FaceMaker> mkFace = FaceMaker::ConstructFromType(maker);
    mkFace->MyHasher = Hasher;
    mkFace->MyOp = op;
    if (pln)
        mkFace->setPlane(*pln);

    for(auto &s : shapes) {
        if (s.getShape().ShapeType() == TopAbs_COMPOUND)
            mkFace->useTopoCompound(s);
        else
            mkFace->addTopoShape(s);
    }
    mkFace->Build();

    const auto &ret = mkFace->getTopoShape();
    setShape(ret._Shape);
    Hasher = ret.Hasher;
    resetElementMap(ret.elementMap());
    if (!isValid()) {
        ShapeFix_ShapeTolerance aSFT;
        aSFT.LimitTolerance(getShape(),
                    Precision::Confusion(), Precision::Confusion(), TopAbs_SHAPE);

        // In some cases, the OCC reports the returned shape having invalid
        // tolerance. Not sure about the real cause.
        //
        // Update: one of the cause is related to OCC bug in
        // BRepBuilder_FindPlane, A possible call sequence is,
        //
        //      makEOffset2D() -> TopoShape::findPlane() -> BRepLib_FindSurface
        //
        // See code comments in findPlane() for the description of the bug and
        // work around.

        ShapeFix_Shape fixer(getShape());
        fixer.Perform();
        setShape(fixer.Shape(), false);

        if (!isValid())
            FC_WARN("makEFace: resulting face is invalid");
    }
    return *this;
}

class MyRefineMaker : public BRepBuilderAPI_RefineModel
{
public:
    MyRefineMaker(const TopoDS_Shape &s)
        :BRepBuilderAPI_RefineModel(s)
    {}

    void populate(ShapeMapper &mapper)
    {
        for (TopTools_DataMapIteratorOfDataMapOfShapeListOfShape it(this->myModified); it.More(); it.Next())
        {
            if (it.Key().IsNull()) continue;
            mapper.populate(false, it.Key(), it.Value());
        }
    }
};

TopoShape &TopoShape::makERefine(const TopoShape &shape, const char *op, bool no_fail) {
    if(shape.isNull()) {
        if(!no_fail)
            HANDLE_NULL_SHAPE;
        _Shape.Nullify();
        return *this;
    }
    if(!op) op = Part::OpCodes::Refine;
    bool closed = shape.isClosed();
    try {
#if 1
        MyRefineMaker mkRefine(shape.getShape());
        GenericShapeMapper mapper;
        mkRefine.populate(mapper);
        mapper.init(shape, mkRefine.Shape());
        makESHAPE(mkRefine.Shape(), mapper, {shape}, op);
        // For some reason, refine operation may reverse the solid
        fixSolidOrientation();
        if (isClosed() == closed)
            return *this;
#else
        BRepBuilderAPI_RefineModel mkRefine(shape.getShape());
        return makEShape(mkRefine,shape,op);
#endif
    }catch (Standard_Failure &) {
        if(!no_fail) throw;
    }
    *this = shape;
    return *this;
}

TopoShape &TopoShape::makEBoolean(const char *maker,
        const TopoShape &shape, const char *op, double tol)
{
    return makEBoolean(maker,std::vector<TopoShape>(1,shape),op,tol);
}

// topo naming conterpart of TopoShape::makeShell()
TopoShape &TopoShape::makEShell(bool silent, const char *op) {
    if(silent) {
        if (isNull())
            return *this;

        if (shapeType(true) != TopAbs_COMPOUND)
            return *this;

        // we need a compound that consists of only faces
        TopExp_Explorer it;
        // no shells
        if (hasSubShape(TopAbs_SHELL))
            return *this;

        // no wires outside a face
        it.Init(_Shape, TopAbs_WIRE, TopAbs_FACE);
        if (it.More())
            return *this;

        // no edges outside a wire
        it.Init(_Shape, TopAbs_EDGE, TopAbs_WIRE);
        if (it.More())
            return *this;

        // no vertexes outside an edge
        it.Init(_Shape, TopAbs_VERTEX, TopAbs_EDGE);
        if (it.More())
            return *this;
    } else if (!hasSubShape(TopAbs_FACE)) {
        FC_THROWM(Base::CADKernelError,"Cannot make shell without face");
    }

    BRep_Builder builder;
    TopoDS_Shape shape;
    TopoDS_Shell shell;
    builder.MakeShell(shell);

    try {
        for (const auto &face : getSubShapes(TopAbs_FACE))
            builder.Add(shell, face);

        TopoShape tmp(Tag, Hasher, shell);
        tmp.mapSubElement(*this, op);

        shape = shell;
        BRepCheck_Analyzer check(shell);
        if (!check.IsValid()) {
            ShapeUpgrade_ShellSewing sewShell;
            shape = sewShell.ApplySewing(shell);
            // TODO confirm the above won't change OCCT topological naming
        }

        if (shape.IsNull()) {
            if (silent)
                return *this;
            FC_THROWM(NullShapeException, "Failed to make shell");
        }

        if (shape.ShapeType() != TopAbs_SHELL) {
            if (silent)
                return *this;
            FC_THROWM(Base::CADKernelError, "Failed to make shell: unexpected output shape type "
                    << shapeType(shape.ShapeType(), true));
        }

        setShape(shape);
        resetElementMap(tmp.elementMap());
    }
    catch (Standard_Failure &e) {
        if(!silent)
            FC_THROWM(Base::CADKernelError, "Failed to make shell: " << e.GetMessageString());
    }

    return *this;
}

struct MapperFill: Part::TopoShape::Mapper {
    BRepFill_Generator &maker;
    MapperFill(BRepFill_Generator &maker)
        :maker(maker)
    {}
    virtual const std::vector<TopoDS_Shape> &generated(const TopoDS_Shape &s) const override {
        _res.clear();
        try {
            TopTools_ListIteratorOfListOfShape it;
            for (it.Initialize(maker.GeneratedShapes(s)); it.More(); it.Next())
                _res.push_back(it.Value());
        } catch (const Standard_Failure & e) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_WARN("Exception on shape mapper: " << e.GetMessageString());
        }
        return _res;
    }
};

TopoShape &TopoShape::makEShellFromWires(const std::vector<TopoShape> &wires, bool silent, const char *op)
{
    BRepFill_Generator maker;
    for (auto &w : wires) {
        if (w.shapeType(silent) == TopAbs_WIRE)
            maker.AddWire(TopoDS::Wire(w.getShape()));
    }
    if (wires.empty()) {
        if (silent) {
            _Shape.Nullify();
            return *this;
        }
        FC_THROWM(NullShapeException,"No input shapes");
    }
    maker.Perform();
    this->makESHAPE(maker.Shell(), MapperFill(maker), wires, op);
    return *this;
}

TopoShape &TopoShape::makEBoolean(const char *maker,
        const std::vector<TopoShape> &shapes, const char *op, double tol)
{
#if OCC_VERSION_HEX <= 0x060800
    if (tol > 0.0)
        Standard_Failure::Raise("Fuzzy Booleans are not supported in this version of OCCT");
#endif

    if(!maker)
        FC_THROWM(Base::CADKernelError,"no maker");

    if(!op) op = maker;

    if(shapes.empty())
        HANDLE_NULL_SHAPE;

    if(strcmp(maker,Part::OpCodes::Compound)==0) {
        return makECompound(shapes,op,false);
    } else if(boost::starts_with(maker,Part::OpCodes::Face)) {
        std::string prefix(Part::OpCodes::Face);
        prefix += '.';
        const char *face_maker = 0;
        if(boost::starts_with(maker,prefix))
            face_maker = maker+prefix.size();
        return makEFace(shapes,op,face_maker);
    } else if(strcmp(maker, Part::OpCodes::Wire)==0)
        return makEWires(shapes,op);
    else if(strcmp(maker, Part::OpCodes::Compsolid)==0) {
        BRep_Builder builder;
        TopoDS_CompSolid Comp;
        builder.MakeCompSolid(Comp);
        for(auto &s : shapes) {
            if (!s.isNull())
                builder.Add(Comp, s.getShape());
        }
        setShape(Comp);
        mapSubElement(shapes,op);
        return *this;
    }

    if(strcmp(maker,Part::OpCodes::Pipe)==0) {
        if(shapes.size()!=2)
            FC_THROWM(Base::CADKernelError,"Not enough input shapes");
        if (shapes[0].isNull() || shapes[1].isNull())
            FC_THROWM(Base::CADKernelError,"Cannot sweep along empty spine");
        if (shapes[0].getShape().ShapeType() != TopAbs_WIRE)
            FC_THROWM(Base::CADKernelError,"Spine shape is not a wire");
        BRepOffsetAPI_MakePipe mkPipe(TopoDS::Wire(shapes[0].getShape()), shapes[1].getShape());
        return makEShape(mkPipe,shapes,op);
    }

    if(strcmp(maker,Part::OpCodes::Shell)==0) {
        BRep_Builder builder;
        TopoDS_Shell shell;
        builder.MakeShell(shell);
        for(auto &s : shapes)
            builder.Add(shell,s.getShape());
        setShape(shell);
        mapSubElement(shapes,op);
        BRepCheck_Analyzer check(shell);
        if (!check.IsValid()) {
            ShapeUpgrade_ShellSewing sewShell;
            setShape(sewShell.ApplySewing(shell), false);
            // TODO confirm the above won't change OCCT topological naming
        }
        return *this;
    }

    bool buildShell = true;

    std::vector<TopoShape> _shapes;
    if(strcmp(maker, Part::OpCodes::Fuse)==0) {
        for(auto it=shapes.begin();it!=shapes.end();++it) {
            auto &s = *it;
            if(s.isNull())
                HANDLE_NULL_INPUT;
            if(s.shapeType() == TopAbs_COMPOUND) {
                if(_shapes.empty())
                    _shapes.insert(_shapes.end(),shapes.begin(),it);
                expandCompound(s,_shapes);
            }else if(_shapes.size())
                _shapes.push_back(s);
        }
    }
    else if (strcmp(maker, Part::OpCodes::Cut)==0) {
        for(unsigned i=1; i<shapes.size(); ++i) {
            auto &s = shapes[i];
            if(s.isNull())
                HANDLE_NULL_INPUT;
            if(s.shapeType() == TopAbs_COMPOUND) {
                if(_shapes.empty())
                    _shapes.insert(_shapes.end(),shapes.begin(),shapes.begin()+i);
                expandCompound(s,_shapes);
            }else if(_shapes.size())
                _shapes.push_back(s);
        }
    }

    if (tol > 0.0 &&  _shapes.empty())
        _shapes = shapes;

    const auto &inputs = _shapes.size()?_shapes:shapes;
    if(inputs.empty())
        HANDLE_NULL_INPUT;
    if(inputs.size()==1) {
        *this = inputs[0];
        if (shapes.size() == 1) {
            // _shapes has fewer items than shapes due to compound expansion.
            // Only warn if the caller paseses one shape.
            FC_WARN("Boolean operation with only one shape input");
        }
        return *this;
    }

#if OCC_VERSION_HEX <= 0x060800
    TopoShape resShape = inputs[0];
    if (resShape.isNull())
        FC_THROWM(Base::ValueError,"Object shape is null");
    if(strcmp(maker, Part::OpCodes::Fuse)==0) {
        for(size_t i=1;i<inputs.size();++i) {
            const auto &s = inputs[i];
            if (s.isNull())
                FC_THROWM(NullShapeException,"Input shape is null");
            BRepAlgoAPI_Fuse mk(resShape.getShape(), s.getShape());
            if (!mk.IsDone())
                FC_THROWM(Base::CADKernelError,"Fusion failed");
            resShape = makEShape(mk,{resShape,s},op);
        }
    } else if(strcmp(maker, Part::OpCodes::Cut)==0) {
        for(size_t i=1;i<inputs.size();++i) {
            const auto &s = inputs[i];
            if (s.isNull())
                FC_THROWM(NullShapeException,"Input shape is null");
            BRepAlgoAPI_Cut mk(resShape.getShape(), s.getShape());
            if (!mk.IsDone())
                FC_THROWM(Base::CADKernelError,"Cut failed");
            resShape = makEShape(mk,{resShape,s},op);
        }
    } else if(strcmp(maker, Part::OpCodes::Common)==0) {
        for(size_t i=1;i<inputs.size();++i) {
            const auto &s = inputs[i];
            if (s.isNull())
                FC_THROWM(NullShapeException,"Input shape is null");
            BRepAlgoAPI_Common mk(resShape.getShape(), s.getShape());
            if (!mk.IsDone())
                FC_THROWM(Base::CADKernelError,"Common failed");
            resShape = makEShape(mk,{resShape,s},op);
        }
    } else if(strcmp(maker, Part::OpCodes::Section)==0) {
        buildShell = false;
        for(size_t i=1;i<inputs.size();++i) {
            const auto &s = inputs[i];
            if (s.isNull())
                FC_THROWM(NullShapeException,"Input shape is null");
            BRepAlgoAPI_Section mk(resShape.getShape(), s.getShape());
            if (!mk.IsDone())
                FC_THROWM(Base::CADKernelError,"Section failed");
            resShape = makEShape(mk,{resShape,s},op);
        }
    } else
        FC_THROWM(Base::CADKernelError,"Unknown maker");

    if(buildShell)
        makEShell();
    return *this;
#else

    std::unique_ptr<BRepAlgoAPI_BooleanOperation> mk;
    if(strcmp(maker, Part::OpCodes::Fuse)==0)
        mk.reset(new BRepAlgoAPI_Fuse);
    else if(strcmp(maker, Part::OpCodes::Cut)==0)
        mk.reset(new BRepAlgoAPI_Cut);
    else if(strcmp(maker, Part::OpCodes::Common)==0)
        mk.reset(new BRepAlgoAPI_Common);
    else if(strcmp(maker, Part::OpCodes::Section)==0) {
        mk.reset(new BRepAlgoAPI_Section);
        buildShell = false;
    } else
        FC_THROWM(Base::CADKernelError,"Unknown maker");

    TopTools_ListOfShape shapeArguments,shapeTools;

    int i=-1;
    for(const auto &shape : inputs) {
        if(shape.isNull())
            HANDLE_NULL_INPUT;
        if(++i == 0)
            shapeArguments.Append(shape.getShape());
        else if (tol > 0.0) {
            auto & s = _shapes[i];
            // workaround for http://dev.opencascade.org/index.php?q=node/1056#comment-520
            s.setShape(BRepBuilderAPI_Copy(s.getShape()).Shape(), false);
            shapeTools.Append(s.getShape());
        } else
            shapeTools.Append(shape.getShape());
    }

# if OCC_VERSION_HEX >= 0x060900
#   if OCC_VERSION_HEX >= 0x070500
    if (PartParams::getParallelRunThreshold() > 0) {
        mk->SetRunParallel(Standard_True);
        OSD_Parallel::SetUseOcctThreads(Standard_True);
    }
#   else
    // Only run parallel 
    if (shapeArguments.Size() + shapeTools.Size() > 2)
        mk->SetRunParallel(true);
    else if (PartParams::getParallelRunThreshold() > 0) {
        int total = 0;
        for (const auto &shape : inputs) {
            total += shape.countSubShapes(TopAbs_FACE);
            if (total > PartParams::getParallelRunThreshold()) {
                mk->SetRunParallel(true);
                break;
            }
        }
    }
#   endif
# endif

    mk->SetArguments(shapeArguments);
    mk->SetTools(shapeTools);
    if (tol > 0.0)
        mk->SetFuzzyValue(tol);
    mk->Build();
    makEShape(*mk,inputs,op);

    if(buildShell)
        makEShell();
    return *this;
#endif
}

TopoShape &TopoShape::makEShape(BRepBuilderAPI_MakeShape &mkShape,
        const TopoShape &source, const char *op)
{
    std::vector<TopoShape> sources(1, source);
    return makEShape(mkShape,sources,op);
}

struct ShapeInfo {
    const TopoDS_Shape &shape;
    TopoShape::Cache::Info &cache;
    TopAbs_ShapeEnum type;
    const char *shapetype;

    ShapeInfo(const TopoDS_Shape &shape, TopAbs_ShapeEnum type, TopoShape::Cache::Info &cache)
        :shape(shape),cache(cache),type(type),shapetype(TopoShape::shapeName(type).c_str())
    {}

    int count() const {
        return cache.count();
    }

    TopoDS_Shape find(int index) {
        return cache.find(shape,index);
    }

    int find(const TopoDS_Shape &subshape) {
        return cache.find(shape,subshape);
    }
};

TopoShape &TopoShape::makEShape(BRepBuilderAPI_MakeShape &mkShape,
        const std::vector<TopoShape> &shapes, const char *op)
{
    return makESHAPE(mkShape.Shape(),MapperMaker(mkShape),shapes,op);
}

Data::MappedName TopoShape::setElementComboName(const Data::IndexedName & element,
                                                const std::vector<Data::MappedName> &names,
                                                const char *marker,
                                                const char *op,
                                                const Data::ElementIDRefs *_sids)
{
    if(names.empty())
        return Data::MappedName();
    std::string _marker;
    if(!marker)
        marker = elementMapPrefix().c_str();
    else if(!boost::starts_with(marker,elementMapPrefix())){
        _marker = elementMapPrefix() + marker;
        marker = _marker.c_str();
    }
    auto it = names.begin();
    Data::MappedName newName = *it;
    std::ostringstream ss;
    Data::ElementIDRefs sids;
    if (_sids)
        sids = *_sids;
    if(names.size() == 1) 
        ss << marker;
    else {
        bool first = true;
        ss.str("");
        if(!Hasher)
            ss << marker;
        ss << '(';
        for(++it;it!=names.end();++it) {
            if(first)
                first = false;
            else
                ss << '|';
            ss << *it;
        }
        ss << ')';
        if(Hasher) {
            sids.push_back(Hasher->getID(ss.str().c_str()));
            ss.str("");
            ss << marker << sids.back().toString();
        }
    }
    encodeElementName(element[0],newName,ss,&sids,op);
    return setElementName(element,newName,&sids);
}

std::vector<Data::MappedName>
TopoShape::decodeElementComboName(const Data::IndexedName &element,
                                  const Data::MappedName &name,
                                  const char *marker,
                                  std::string *postfix) const
{
    std::vector<Data::MappedName> names;
    if (!element)
        return names;
    if (!marker)
        marker = "";
    int plen = (int)elementMapPrefix().size();
    int markerLen = strlen(marker);
    int len;
    int pos = findTagInElementName(name, nullptr, &len);
    if (pos < 0) {
        // It is possible to encode combo name without using a tag, e.g.
        // Sketcher object creates wire using edges that are created by itself,
        // so there will be no tag to encode.
        //
        // In this case, just search for the brackets
        len = name.find("(");
        if (len < 0) {
            // No bracket is also possible, if there is only one name in the combo
            pos = len = name.size();
        } else {
            pos = name.find(")");
            if (pos < 0) {
                // non closing bracket?
                return {};
            }
            ++pos;
        }
        if (len <= (int)markerLen)
            return {};
        len -= markerLen+plen;
    }

    if (name.find(elementMapPrefix(), len) != len
            || name.find(marker, len+plen) != len+plen)
        return {};

    names.emplace_back(name, 0, len);

    std::string text;
    len += plen + markerLen;
    name.toString(text, len, pos-len);

    if (this->Hasher) {
        if (auto id = App::StringID::fromString(names.back().toRawBytes())) {
            if (App::StringIDRef sid = this->Hasher->getID(id)) {
                names.pop_back();
                names.emplace_back(sid);
            }
            else
                return names;
        }
        if (auto id = App::StringID::fromString(text.c_str())) {
            if (App::StringIDRef sid = this->Hasher->getID(id))
                text = sid.dataToText();
            else
                return names;
        }
    }
    if (text.empty() || text[0] != '(')
        return names;
    auto endPos = text.rfind(')');
    if (endPos == std::string::npos)
        return names;

    if (postfix)
        *postfix = text.substr(endPos+1);

    text.resize(endPos);
    std::istringstream iss(text.c_str()+1);
    std::string token;
    while(std::getline(iss, token, '|')) 
        names.emplace_back(token);
    return names;
}

struct NameKey {
    Data::MappedName name;
    long tag = 0;
    int shapetype = 0;

    NameKey()
    {}
    NameKey(const Data::MappedName & n)
        :name(n)
    {}
    NameKey(int type, const Data::MappedName & n)
        :name(n)
    {
        // Order the shape type from vertex < edge < face < other.  We'll rely
        // on this for sorting when we name the geometry element.
        switch(type) {
        case TopAbs_VERTEX:
            shapetype = 0;
            break;
        case TopAbs_EDGE:
            shapetype = 1;
            break;
        case TopAbs_FACE:
            shapetype = 2;
            break;
        default:
            shapetype = 3;
        }
    }
    bool operator<(const NameKey &other) const {
        if(shapetype < other.shapetype)
            return true;
        if(shapetype > other.shapetype)
            return false;
        if(tag < other.tag)
            return true;
        if(tag > other.tag)
            return false;
        return Data::ElementNameComp()(name,other.name);
    }
};

struct NameInfo {
    int index;
    Data::ElementIDRefs sids;
    const char *shapetype;
};

TopoShape &TopoShape::makESHAPE(const TopoDS_Shape &shape, const Mapper &mapper,
        const std::vector<TopoShape> &shapes, const char *op)
{
    setShape(shape);
    if(shape.IsNull())
        HANDLE_NULL_SHAPE;

    if(shapes.empty())
        return *this;

    size_t canMap=0;
    for(auto &shape : shapes) {
        if(canMapElement(shape))
            ++canMap;
    }
    if(!canMap)
        return *this;
    if(canMap!=shapes.size() && FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
        FC_WARN("Not all input shapes are mappable");

    if(!op) op = Part::OpCodes::Maker;
    std::string _op = op;
    _op += '_';

    INIT_SHAPE_CACHE();
    ShapeInfo vinfo(_Shape,TopAbs_VERTEX,_Cache->getInfo(TopAbs_VERTEX));
    ShapeInfo einfo(_Shape,TopAbs_EDGE,_Cache->getInfo(TopAbs_EDGE));
    ShapeInfo finfo(_Shape,TopAbs_FACE,_Cache->getInfo(TopAbs_FACE));
    mapSubElement(shapes);

    std::array<ShapeInfo*,3> infos = {&vinfo,&einfo,&finfo};

    std::array<ShapeInfo*,TopAbs_SHAPE> infoMap;
    infoMap[TopAbs_VERTEX] = &vinfo;
    infoMap[TopAbs_EDGE] = &einfo;
    infoMap[TopAbs_WIRE] = &einfo;
    infoMap[TopAbs_FACE] = &finfo;
    infoMap[TopAbs_SHELL] = &finfo;
    infoMap[TopAbs_SOLID] = &finfo;
    infoMap[TopAbs_COMPOUND] = &finfo;
    infoMap[TopAbs_COMPSOLID] = &finfo;

    std::ostringstream ss;
    std::string postfix;
    Data::MappedName newName;

    std::map<Data::IndexedName, std::map<NameKey,NameInfo> > newNames;

    // First, collect names from other shapes that generates or modifies the
    // new shape
    for(auto &pinfo : infos) {
        auto &info = *pinfo;
        for(size_t n=0;n<shapes.size();++n) {
            const auto &other = shapes[n];
            if(!canMapElement(other))
                continue;
            auto &otherMap = other._Cache->getInfo(info.type);
            if(!otherMap.count())
                continue;

            for (int i=1; i<=otherMap.count(); i++) {
                const auto &otherElement = otherMap.find(other._Shape,i);
                // Find all new objects that are a modification of the old object
                Data::ElementIDRefs sids;
                NameKey key(info.type, other.getMappedName(
                            Data::IndexedName::fromConst(info.shapetype, i),true,&sids));

                int k=0;
                for(auto &newShape : mapper.modified(otherElement)) {
                    ++k;
                    if(newShape.ShapeType()>=TopAbs_SHAPE) {
                        FC_ERR("unknown modified shape type " << newShape.ShapeType()
                                << " from " << info.shapetype << i);
                        continue;
                    }
                    auto &newInfo = *infoMap[newShape.ShapeType()];
                    if(newInfo.type != newShape.ShapeType()) {
                        if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                            // TODO: it seems modified shape may report higher
                            // level shape type just like generated shape below.
                            // Maybe we shall do the same for name construction.
                            FC_WARN("modified shape type " << shapeName(newShape.ShapeType())
                                    << " mismatch with " << info.shapetype << i);
                        }
                        continue;
                    }
                    int j = newInfo.find(newShape);
                    if(!j) {
                        // This warning occurs in makERevolve. It generates
                        // some shape from a vertex that never made into the
                        // final shape. There may be other cases there.
                        if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                            FC_WARN("Cannot find " << op << " modified " <<
                                newInfo.shapetype << " from " << info.shapetype << i);
                        continue;
                    }

                    Data::IndexedName element = Data::IndexedName::fromConst(newInfo.shapetype, j);
                    if(getMappedName(element))
                        continue;

                    key.tag = other.Tag;
                    auto &name_info = newNames[element][key];
                    name_info.sids = sids;
                    name_info.index = k;
                    name_info.shapetype = info.shapetype;
                }

                int checkParallel = -1;
                gp_Pln pln;

                // Find all new objects that were generated from an old object
                // (e.g. a face generated from an edge)
                k=0;
                for(auto &newShape : mapper.generated(otherElement)) {
                    if(newShape.ShapeType()>=TopAbs_SHAPE) {
                        FC_ERR("unknown generated shape type " << newShape.ShapeType()
                                << " from " << info.shapetype << i);
                        continue;
                    }

                    int parallelFace = -1;
                    int coplanarFace = -1;
                    auto &newInfo = *infoMap[newShape.ShapeType()];
                    std::vector<TopoDS_Shape> newShapes;
                    int shapeOffset = 0;
                    if(newInfo.type == newShape.ShapeType()) {
                        newShapes.push_back(newShape);
                    } else {
                        // It is possible for the maker to report generating a
                        // higher level shape, such as shell or solid. For
                        // example, when extruding, OCC will report the
                        // extruding face generating the entire solid. However,
                        // it will also report the edges of the extruding face
                        // generating the side faces. In this case, too much
                        // information is bad for us. We don't want the name of
                        // the side face (and its edges) to be coupled with
                        // other (unrelated) edges in the extruding face.
                        //
                        // shapeOffset below is used to make sure the higher
                        // level mapped names comes late after sorting. We'll
                        // ignore those names if there are more precise mapping
                        // available.
                        shapeOffset = 3;

                        if(info.type==TopAbs_FACE && checkParallel<0) {
                            if(!TopoShape(otherElement).findPlane(pln))
                                checkParallel = 0;
                            else
                                checkParallel = 1;
                        }
                        for(TopExp_Explorer xp(newShape,newInfo.type);xp.More();xp.Next()) {
                            newShapes.push_back(xp.Current());

                            if((parallelFace<0||coplanarFace<0) && checkParallel>0) {
                                // Specialized checking for high level mapped
                                // face that are either coplanar or parallel
                                // with the source face, which are common in
                                // operations like extrusion. Once found, the
                                // first coplanar face will assign an index of
                                // INT_MIN+1, and the first parallel face
                                // INT_MIN. The purpose of these special
                                // indexing is to make the name more stable for
                                // those generated faces.
                                //
                                // For example, the top or bottom face of an
                                // extrusion will be named using the extruding
                                // face. With a fixed index, the name is no
                                // longer affected by adding/removing of holes
                                // inside the extruding face/sketch.
                                gp_Pln plnOther;
                                if(TopoShape(newShapes.back()).findPlane(plnOther)) {
                                    if(pln.Axis().IsParallel(plnOther.Axis(),Precision::Angular())) {
                                        if(coplanarFace<0) {
                                            gp_Vec vec(pln.Axis().Location(),plnOther.Axis().Location());
                                            Standard_Real D1 = gp_Vec(pln.Axis().Direction()).Dot(vec);
                                            if (D1 < 0) D1 = - D1;
                                            Standard_Real D2 = gp_Vec(plnOther.Axis().Direction()).Dot(vec);
                                            if (D2 < 0) D2 = - D2;
                                            if(D1 <= Precision::Confusion() && D2 <= Precision::Confusion()) {
                                                coplanarFace = (int)newShapes.size();
                                                continue;
                                            }
                                        }
                                        if(parallelFace<0)
                                            parallelFace = (int)newShapes.size();
                                    }
                                }
                            }
                        }
                    }
                    key.shapetype += shapeOffset;
                    for(auto &newShape : newShapes) {
                        ++k;
                        int j = newInfo.find(newShape);
                        if(!j) {
                            if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                                FC_WARN("Cannot find " << op << " generated " <<
                                        newInfo.shapetype << " from " << info.shapetype << i);
                            continue;
                        }

                        Data::IndexedName element = Data::IndexedName::fromConst(newInfo.shapetype, j);
                        auto mapped = getMappedName(element);
                        if (mapped)
                            continue;

                        key.tag = other.Tag;
                        auto &name_info = newNames[element][key];
                        name_info.sids = sids;
                        if(k == parallelFace)
                            name_info.index = INT_MIN;
                        else if(k == coplanarFace)
                            name_info.index = INT_MIN+1;
                        else
                            name_info.index = -k;
                        name_info.shapetype = info.shapetype;
                    }
                    key.shapetype -= shapeOffset;
                }
            }
        }
    }

    // We shall first exclude those names generated from high level mapping. If
    // there are still any unnamed elements left after we go through the process
    // below, we set delayed=true, and start using those excluded names.
    bool delayed = false;

    while(true) {

        // Construct the names for modification/generation info collected in
        // the previous step
        for(auto itName=newNames.begin(),itNext=itName; itNext!=newNames.end(); itName=itNext) {
            // We treat the first modified/generated source shape name specially.
            // If case there are more than one source shape. We hash the first
            // source name separately, and then obtain the second string id by
            // hashing all the source names together.  We then use the second
            // string id as the postfix for our name.
            //
            // In this way, we can associate the same source that are modified by
            // multiple other shapes.

            ++itNext;

            auto &element = itName->first;
            auto &names = itName->second;
            const auto &first_key = names.begin()->first;
            auto &first_info = names.begin()->second;

            if(!delayed && first_key.shapetype>=3 && first_info.index>INT_MIN+1) {
                // This name is mapped from high level (shell, solid, etc.)
                // Delay till next round.
                //
                // index>INT_MAX+1 is for checking generated coplanar and
                // parallel face mapping, which has special fixed index to make
                // name stable.  These names are not delayed.
                continue;
            }else if(!delayed && getMappedName(element)) {
                newNames.erase(itName);
                continue;
            }

            int name_type = first_info.index>0?1:2; // index>0 means modified, or else generated
            Data::MappedName first_name = first_key.name;

            Data::ElementIDRefs sids(first_info.sids);

            postfix.clear();
            if(names.size()>1) {
                ss.str("");
                ss << '(';
                bool first = true;
                auto it = names.begin();
                int count = 0;
                for(++it;it!=names.end();++it) {
                    auto &other_key = it->first;
                    if(other_key.shapetype>=3 && first_key.shapetype<3) {
                        // shapetype>=3 means its a high level mapping (e.g. a face
                        // generates a solid). We don't want that if there are more
                        // precise low level mapping available. See comments above
                        // for more details.
                        break;
                    }
                    if(first)
                        first = false;
                    else
                        ss << '|';
                    auto &other_info = it->second;
                    std::ostringstream ss2;
                    if(other_info.index!=1) {
                        // 'K' marks the additional source shape of this
                        // generate (or modified) shape.
                        ss2 << elementMapPrefix() << 'K';
                        if(other_info.index == INT_MIN)
                            ss2 << '0';
                        else if(other_info.index == INT_MIN+1)
                            ss2 << "00";
                        else {
                            // The same source shape may generate or modify
                            // more than one shape. The index here marks the
                            // position it is reported by OCC. Including the
                            // index here is likely to degrade name stablilty,
                            // but is unfortunately a necessity to avoid
                            // duplicate names.
                            ss2 << other_info.index;
                        }
                    }
                    Data::MappedName other_name = other_key.name;
                    encodeElementName(other_info.shapetype[0],other_name,ss2,&sids,0,other_key.tag);
                    ss << other_name;
                    if((name_type==1 && other_info.index<0) 
                            || (name_type==2 && other_info.index>0)) 
                    {
                        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                            FC_WARN("element is both generated and modified");
                        name_type = 0;
                    }
                    sids += other_info.sids;
                    // To avoid the name becoming to long, just put some limit here
                    if (++count == 4)
                        break;
                }
                if(!first) {
                    ss <<')';
                    if(Hasher) {
                        sids.push_back(Hasher->getID(ss.str().c_str()));
                        ss.str("");
                        ss << sids.back().toString();
                    }
                    postfix = ss.str();
                }
            }

            ss.str("");
            if(name_type==2)
                ss << genPostfix();
            else if(name_type==1)
                ss << modPostfix();
            else
                ss << modgenPostfix();
            if(first_info.index == INT_MIN)
                ss << '0';
            else if(first_info.index == INT_MIN+1)
                ss << "00";
            else if(abs(first_info.index)>1)
                ss << abs(first_info.index);
            ss << postfix;
            encodeElementName(element[0],first_name,ss,&sids,op,first_key.tag);
            setElementName(element,first_name,&sids);

            if(!delayed && first_key.shapetype<3)
                newNames.erase(itName);
        }

        // The reverse pass. Starting from the highest level element, i.e.
        // Face, for any element that are named, assign names for its lower unnamed
        // elements. For example, if Edge1 is named E1, and its vertexes are not
        // named, then name them as E1;U1, E1;U2, etc.
        //
        // In order to make the name as stable as possible, we may assign multiple
        // names (which must be sorted, because we may use the first one to name
        // upper element in the final pass) to lower element if it appears in
        // multiple higher elements, e.g. same edge in multiple faces.

        for(size_t ifo=infos.size()-1;ifo!=0;--ifo) {
            std::map<Data::IndexedName,
                     std::map<Data::MappedName, NameInfo, Data::ElementNameComp> > names;
            auto &info = *infos[ifo];
            auto &next = *infos[ifo-1];
            int i = 1;
            auto it = newNames.end();
            if(delayed)
                it = newNames.upper_bound(Data::IndexedName::fromConst(info.shapetype, 0));
            for(;;++i) {
                Data::IndexedName element;
                if(!delayed) {
                    if(i>info.count())
                        break;
                    element = Data::IndexedName::fromConst(info.shapetype, i);
                    if(newNames.count(element))
                        continue;
                }else if(it==newNames.end() ||
                         !boost::starts_with(it->first.getType(),info.shapetype))
                    break;
                else {
                    element = it->first;
                    ++it;
                    i = element.getIndex();
                    if(i==0 || i>info.count())
                        continue;
                }
                Data::ElementIDRefs sids;
                Data::MappedName mapped = getMappedName(element, false, &sids);
                if(!mapped)
                    continue;

                TopTools_IndexedMapOfShape submap;
                TopExp::MapShapes(info.find(i), next.type, submap);
                for(int j=1,n=1;j<=submap.Extent();++j) {
                    ss.str("");
                    int k = next.find(submap(j));
                    assert(k);
                    Data::IndexedName e = Data::IndexedName::fromConst(next.shapetype, k);
                    if(getMappedName(e))
                        continue;
                    auto &info = names[e][mapped];
                    info.index = n++;
                    info.sids = sids;
                }
            }
            // Assign the actual names
            for(auto &v : names) {
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
                    newName = name.first;
                    ss.str("");
                    ss << upperPostfix();
                    if(info.index>1)
                        ss << info.index;
                    encodeElementName(v.first[0],newName,ss,&sids,op);
                    setElementName(v.first,newName,&sids);
                }
            }
        }

        // The forward pass. For any elements that are not named, try construct its
        // name from the lower elements
        bool hasUnnamed = false;
        for(size_t ifo=1;ifo<infos.size();++ifo) {
            auto &info = *infos[ifo];
            auto &prev = *infos[ifo-1];
            for(int i=1;i<=info.count();++i) {
                Data::IndexedName element = Data::IndexedName::fromConst(info.shapetype, i);
                if (getMappedName(element))
                    continue;

                Data::ElementIDRefs sids;
                std::map<Data::MappedName, Data::IndexedName, Data::ElementNameComp> names;
                TopExp_Explorer xp;
                if(info.type == TopAbs_FACE)
                    xp.Init(BRepTools::OuterWire(TopoDS::Face(info.find(i))),TopAbs_EDGE);
                else
                    xp.Init(info.find(i),prev.type);
                for(;xp.More();xp.Next()) {
                    int j = prev.find(xp.Current());
                    assert(j);
                    Data::IndexedName prevElement = Data::IndexedName::fromConst(prev.shapetype, j);
                    if(!delayed && newNames.count(prevElement)) {
                        names.clear();
                        break;
                    }
                    Data::ElementIDRefs sid;
                    Data::MappedName name = getMappedName(prevElement, false, &sid);
                    if(!name) {
                        // only assign name if all lower elements are named
                        if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                            FC_WARN("unnamed lower element " << prevElement);
                        names.clear();
                        break;
                    }
                    auto res = names.emplace(name,prevElement);
                    if(res.second)
                        sids += sid;
                    else if(prevElement!=res.first->second) {
                        // The seam edge will appear twice, which is normal. We
                        // only warn if the mapped element names are different.
                        FC_WARN("lower element " << prevElement << " and " <<
                                res.first->second << " has duplicated name " << name
                                << " for " << info.shapetype << i );
                    }
                }
                if(names.empty()) {
                    hasUnnamed = true;
                    continue;
                }
                auto it = names.begin();
                newName = it->first;
                if(names.size() == 1)
                    ss << lowerPostfix();
                else {
                    bool first = true;
                    ss.str("");
                    if(!Hasher)
                        ss << lowerPostfix();
                    ss << '(';
                    int count = 0;
                    for(++it;it!=names.end();++it) {
                        if(first)
                            first = false;
                        else
                            ss << '|';
                        ss << it->first;

                        // To avoid the name becoming to long, just put some limit here
                        if (++count == 4)
                            break;
                    }
                    ss << ')';
                    if(Hasher) {
                        sids.push_back(Hasher->getID(ss.str().c_str()));
                        ss.str("");
                        ss << lowerPostfix() << sids.back().toString();
                    }
                }
                encodeElementName(element[0],newName,ss,&sids,op);
                setElementName(element,newName,&sids);
            }
        }
        if(!hasUnnamed || delayed || newNames.empty())
            break;
        delayed = true;
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

TopoShape &TopoShape::makESlice(const TopoShape &shape,
        const Base::Vector3d& dir, double d, const char *op)
{
    if(shape.isNull())
        HANDLE_NULL_SHAPE;
    TopoCrossSection cs(dir.x, dir.y, dir.z,shape,op);
    TopoShape res = cs.slice(1,d);
    setShape(res._Shape);
    Hasher = res.Hasher;
    resetElementMap(res.elementMap());
    return *this;
}

TopoShape &TopoShape::makESlices(const TopoShape &shape,
        const Base::Vector3d& dir, const std::vector<double> &d, const char *op)
{
    std::vector<TopoShape> wires;
    TopoCrossSection cs(dir.x, dir.y, dir.z, shape,op);
    int i=0;
    for(auto &dd : d)
        cs.slice(++i,dd,wires);
    return makECompound(wires,op,false);
}

TopoShape &TopoShape::makEFilledFace(const std::vector<TopoShape> &_shapes,
                                     const BRepFillingParams &params,
                                     const char *op)
{
    if(!op)
        op = Part::OpCodes::FilledFace;
    BRepOffsetAPI_MakeFilling maker(params.degree,
                                    params.ptsoncurve,
                                    params.numiter,
                                    params.anisotropy,
                                    params.tol2d,
                                    params.tol3d,
                                    params.tolG1,
                                    params.tolG2,
                                    params.maxdeg,
                                    params.maxseg);

    if (!params.surface.isNull() && params.surface.getShape().ShapeType() == TopAbs_FACE)
        maker.LoadInitSurface(TopoDS::Face(params.surface.getShape()));

    std::vector<TopoShape> shapes;
    for(auto &s : _shapes)
        expandCompound(s,shapes);

    TopoShapeMap output;
    auto getOrder = [&](const TopoDS_Shape &s) {
        auto it = params.orders.find(s);
        if (it == params.orders.end()) {
            auto iter = output.find(s);
            if (iter != output.end())
                it = params.orders.find(iter->second.getShape());
        }
        if (it != params.orders.end())
            return static_cast<GeomAbs_Shape>(it->second);
        return GeomAbs_C0;
    };

    auto getSupport = [&](const TopoDS_Shape &s) {
        TopoDS_Face support;
        auto it = params.supports.find(s);
        if (it == params.supports.end()) {
            auto iter = output.find(s);
            if (iter != output.end())
                it = params.supports.find(iter->second.getShape());
        }
        if (it != params.supports.end()) {
            if (!it->second.IsNull() && it->second.ShapeType() == TopAbs_FACE)
                support = TopoDS::Face(it->second);
        }
        return support;
    };
    
    auto findBoundary = [](std::vector<TopoShape> &shapes) -> TopoShape {
        // Find a wire (preferably a closed one) to be used as the boundary.
        int i = -1;
        int boundIdx = -1;
        for (auto &s : shapes) {
            ++i;
            if(s.isNull() || !s.hasSubShape(TopAbs_EDGE) || s.shapeType()!=TopAbs_WIRE)
                continue;
            if (BRep_Tool::IsClosed(TopoDS::Wire(s.getShape()))) {
                boundIdx = i;
                break;
            } else if (boundIdx < 0)
                boundIdx = i;
        }
        if (boundIdx >= 0) {
            auto res = shapes[boundIdx];
            shapes.erase(shapes.begin() + boundIdx);
            return res;
        }
        return TopoShape();
    };

    TopoShape bound;
    std::vector<TopoShape> wires;
    if (params.boundary_begin >= 0
            && params.boundary_end > params.boundary_begin
            && params.boundary_end <= (int)shapes.size())
    {
        if (params.boundary_end-1 != params.boundary_begin
                || shapes[params.boundary_begin].shapeType() != TopAbs_WIRE)
        {
            std::vector<TopoShape> edges;
            edges.insert(edges.end(),
                    shapes.begin()+params.boundary_begin,
                    shapes.begin()+params.boundary_end);
            wires = TopoShape(0, Hasher).makEWires(edges,"",0.0,false,&output).getSubTopoShapes(TopAbs_WIRE);
            shapes.erase(shapes.begin()+params.boundary_begin,
                         shapes.begin()+params.boundary_end);
        }
    } else {
        bound = findBoundary(shapes);
        if (bound.isNull()) {
            // If no boundry is found, then try to build one.
            std::vector<TopoShape> edges;
            for(auto it=shapes.begin(); it!=shapes.end();) {
                if (it->shapeType(true) == TopAbs_EDGE) {
                    edges.push_back(*it);
                    it = shapes.erase(it);
                } else
                    ++it;
            }
            if(edges.size())
                wires = TopoShape(0, Hasher).makEWires(edges,"",0.0,false,&output).getSubTopoShapes(TopAbs_WIRE);
        }
    }

    if (bound.isNull())
        bound = findBoundary(wires);

    if (bound.isNull())
        FC_THROWM(Base::CADKernelError,"No boundary wire");

    // Since we've only selected one wire for boundary, return all the
    // other edges in shapes to be added as non boundary constraints
    shapes.insert(shapes.end(), wires.begin(), wires.end());

    // Must fix wire connection to avoid OCC crash in BRepFill_Filling.cxx WireFromList()
    // https://github.com/Open-Cascade-SAS/OCCT/blob/1c96596ae7ba120a678021db882857e289c73947/src/BRepFill/BRepFill_Filling.cxx#L133
    // The reason of crash is because the wire connection tolerance is too big.
    // The crash can be fixed by simply checking itl.More() before calling Remove().
    bound.fix(Precision::Confusion(),
              Precision::Confusion(),
              Precision::Confusion());

    for (const auto &e : bound.getOrderedEdges()) {
        maker.Add(TopoDS::Edge(e.getShape()),
                  getSupport(e.getShape()),
                  getOrder(e.getShape()),
                  /*IsBound*/Standard_True);
    }

    for(const auto &s : shapes) {
        if(s.isNull())
            continue;
        const auto &sh = s.getShape();
        if (sh.ShapeType() == TopAbs_WIRE) {
            for (const auto &e : s.getSubShapes(TopAbs_EDGE))
                maker.Add(TopoDS::Edge(e), 
                          getSupport(e),
                          getOrder(e),
                          /*IsBound*/Standard_False);
        }
        else if (sh.ShapeType() == TopAbs_EDGE)
            maker.Add(TopoDS::Edge(sh),
                      getSupport(sh),
                      getOrder(sh),
                      /*IsBound*/Standard_False);
        else if (sh.ShapeType() == TopAbs_FACE)
            maker.Add(TopoDS::Face(sh), getOrder(sh));
        else if (sh.ShapeType() == TopAbs_VERTEX)
            maker.Add(BRep_Tool::Pnt(TopoDS::Vertex(sh)));
    }
    
    maker.Build();
    if (!maker.IsDone())
        FC_THROWM(Base::CADKernelError,"Failed to created face by filling edges");
    return makEShape(maker,_shapes,op);
}

TopoShape &TopoShape::makESolid(const std::vector<TopoShape> &shapes, const char *op) {
    return makESolid(TopoShape().makECompound(shapes),op);
}

bool TopoShape::fixSolidOrientation()
{
    if (isNull())
        return false;

    if (shapeType() == TopAbs_SOLID) {
        TopoDS_Solid solid = TopoDS::Solid(_Shape);
        BRepLib::OrientClosedSolid(solid);
        if (solid.IsEqual(_Shape))
            return false;
        setShape(solid, false);
        return true;
    }

    if (shapeType() == TopAbs_COMPOUND
            || shapeType() == TopAbs_COMPSOLID)
    {
        auto shapes = getSubTopoShapes();
        bool touched = false;
        for (auto &s : shapes) {
            if (s.fixSolidOrientation())
                touched = true;
        }
        if (!touched)
            return false;

        BRep_Builder builder;
        if (shapeType() == TopAbs_COMPOUND) {
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            for(auto &s : shapes) {
                if (!s.isNull())
                    builder.Add(comp, s.getShape());
            }
            setShape(comp, false);
        } else {
            TopoDS_CompSolid comp;
            builder.MakeCompSolid(comp);
            for(auto &s : shapes) {
                if (!s.isNull())
                    builder.Add(comp, s.getShape());
            }
            setShape(comp, false);
        }
        return true;
    }

    return false;
}

TopoShape &TopoShape::makESolid(const TopoShape &shape, const char *op) {
    if(!op) op = Part::OpCodes::Solid;

    if(shape.isNull())
        HANDLE_NULL_SHAPE;

    //first, if we were given a compsolid, try making a solid out of it
    TopoDS_CompSolid compsolid;
    int count=0;
    for(const auto &s : shape.getSubShapes(TopAbs_COMPSOLID)) {
        ++count;
        compsolid = TopoDS::CompSolid(s);
        if (count > 1)
            break;
    }
    if (count == 0) {
        //no compsolids. Get shells...
        BRepBuilderAPI_MakeSolid mkSolid;
        count=0;
        for (const auto &s : shape.getSubShapes(TopAbs_SHELL)) {
            ++count;
            mkSolid.Add(TopoDS::Shell(s));
        }

        if (count == 0)//no shells?
            FC_THROWM(Base::CADKernelError,"No shells or compsolids found in shape");

        makEShape(mkSolid,shape,op);

        TopoDS_Solid solid = TopoDS::Solid(_Shape);
        BRepLib::OrientClosedSolid(solid);
        setShape(solid, false);

    } else if (count == 1) {
        BRepBuilderAPI_MakeSolid mkSolid(compsolid);
        makEShape(mkSolid,shape,op);
    } else { // if (count > 1)
        FC_THROWM(Base::CADKernelError,"Only one compsolid can be accepted. "
                "Provided shape has more than one compsolid.");
    }
    return *this;
}

TopoShape &TopoShape::replacEShape(const TopoShape &shape,
        const std::vector<std::pair<TopoShape,TopoShape> > &s)
{
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
    setShape(reshape.Apply(shape.getShape(),TopAbs_SHAPE));
    mapSubElement(shapes);
    return *this;
}

TopoShape &TopoShape::removEShape(const TopoShape &shape, const std::vector<TopoShape>& s)
{
    if(shape.isNull())
        HANDLE_NULL_SHAPE;
    BRepTools_ReShape reshape;
    for(auto &sh : s) {
        if(sh.isNull())
            HANDLE_NULL_INPUT;
        reshape.Remove(sh.getShape());
    }
    setShape(reshape.Apply(shape.getShape(), TopAbs_SHAPE));
    mapSubElement(shape);
    return *this;
}

TopoShape &TopoShape::makEFillet(const TopoShape &shape, const std::vector<TopoShape> &edges,
        double radius1, double radius2, const char *op)
{
    if(!op) op = Part::OpCodes::Fillet;
    if(shape.isNull())
        HANDLE_NULL_SHAPE;

    if(edges.empty())
        HANDLE_NULL_INPUT;

    BRepFilletAPI_MakeFillet mkFillet(shape.getShape());
    for(auto &e : edges) {
        if(e.isNull())
            HANDLE_NULL_INPUT;
        const auto &edge = e.getShape();
        if(!shape.findShape(edge))
            FC_THROWM(Base::CADKernelError,"edge does not belong to the shape");
        mkFillet.Add(radius1, radius2, TopoDS::Edge(edge));
    }
    return makEShape(mkFillet,shape,op);
}

TopoShape &TopoShape::makEChamfer(const TopoShape &shape, const std::vector<TopoShape> &edges,
        double radius1, double radius2, const char *op, bool flipDirection, bool asAngle)
{
    if(!op) op = Part::OpCodes::Chamfer;
    if(shape.isNull())
        HANDLE_NULL_SHAPE;

    if(edges.empty())
        HANDLE_NULL_INPUT;

    BRepFilletAPI_MakeChamfer mkChamfer(shape.getShape());
    for(auto &e : edges) {
        const auto &edge = e.getShape();
        if(e.isNull())
            HANDLE_NULL_INPUT;
        if(!shape.findShape(edge))
            FC_THROWM(Base::CADKernelError,"edge does not belong to the shape");
        //Add edge to fillet algorithm
        TopoDS_Shape face;
        if(flipDirection)
            face = shape.findAncestorsShapes(edge,TopAbs_FACE).back();
        else
            face = shape.findAncestorShape(edge,TopAbs_FACE);
        if(asAngle)
            mkChamfer.AddDA(radius1, radius2, TopoDS::Edge(edge), TopoDS::Face(face));
        else
            mkChamfer.Add(radius1, radius2, TopoDS::Edge(edge), TopoDS::Face(face));
    }
    return makEShape(mkChamfer,shape,op);
}

TopoShape &TopoShape::makEGeneralFuse(const std::vector<TopoShape> &_shapes,
        std::vector<std::vector<TopoShape> > &modifies, double tol, const char *op)
{
#if OCC_VERSION_HEX < 0x060900
    (void)_shapes;
    (void)modifies;
    (void)tol;
    (void)op;
    FC_THROWM(Base::NotImplementedError,"GFA is available only in OCC 6.9.0 and up.");
#else
    if(!op) op = Part::OpCodes::GeneralFuse;

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
        FC_THROWM(Base::CADKernelError,"GeneralFuse failed");
    makEShape(mkGFA,shapes,op);
    modifies.resize(shapes.size());
    int i=0;
    for(auto &s : shapes) {
        auto &mod = modifies[i++];
        for(TopTools_ListIteratorOfListOfShape it(mkGFA.Modified(s.getShape())); it.More(); it.Next()) {
            TopoShape res(Tag);
            res.setShape(it.Value());
            mod.push_back(res);
        }
        mapSubElementsTo(mod);
    }
    return *this;
#endif
}

TopoShape &TopoShape::makEFuse(const std::vector<TopoShape> &shapes,
        const char *op, double tol)
{
    return makEBoolean(Part::OpCodes::Fuse,shapes,op,tol);
}

TopoShape &TopoShape::makECut(const std::vector<TopoShape> &shapes,
        const char *op, double tol)
{
    return makEBoolean(Part::OpCodes::Cut,shapes,op,tol);
}


TopoShape &TopoShape::makEShape(BRepOffsetAPI_MakePipeShell &mkShape,
        const std::vector<TopoShape> &source, const char *op)
{
    if(!op) op = Part::OpCodes::PipeShell;
    return makESHAPE(mkShape.Shape(),MapperMaker(mkShape),source,op);
}

struct MapperPrism: MapperMaker {
    std::unordered_map<TopoDS_Shape, TopoDS_Shape, ShapeHasher, ShapeHasher> vertexMap;
    ShapeMapper::ShapeMap edgeMap;

    MapperPrism(BRepFeat_MakePrism &maker, const TopoShape &upto)
        :MapperMaker(maker)
    {
        (void)upto;

        std::vector<TopoShape> shapes;
        for(TopTools_ListIteratorOfListOfShape it(maker.FirstShape());it.More();it.Next())
            shapes.push_back(it.Value());

        if (shapes.size()) {
            // It seems that BRepFeat_MakePrism::newEdges() does not return
            // edges generated by extruding the profile vertices. The following
            // code assumes BRepFeat_MakePrism::myFShape is the profile, and
            // FirstShape() returns the corresponding faces in the new shape,
            // i.e. the bottom profile, and add all edges that shares a
            // vertex with the profiles as new edges.

            std::unordered_set<TopoDS_Shape,ShapeHasher,ShapeHasher> edgeSet;
            TopoShape bottom;
            bottom.makECompound(shapes, nullptr, false);
            TopoShape shape(maker.Shape());
            for (auto &vertex : bottom.getSubShapes(TopAbs_VERTEX)) {
                for (auto &e : shape.findAncestorsShapes(vertex, TopAbs_EDGE)) {
                    // Make sure to not visit the the same edge twice.
                    // And check only edge that are not found in the bottom profile
                    if (!edgeSet.insert(e).second && !bottom.findShape(e)) {
                        auto otherVertex = TopExp::FirstVertex(TopoDS::Edge(e));
                        if (otherVertex.IsSame(vertex))
                            otherVertex = TopExp::LastVertex(TopoDS::Edge(e));
                        vertexMap[vertex] = otherVertex;
                    }
                }
            }

            // Now map each edge in the bottom profile to the extrueded top
            // profile. vertexMap created above gives us each pair of vertexes
            // of the bottom and top profile. We use it to find the
            // corresponding edges in the top profile, what an extra criteria
            // for disambiguation. That is, the pair of edges (bottom and top)
            // must belong to the same face.
            for (auto &edge : bottom.getSubShapes(TopAbs_EDGE)) {
                std::vector<int> indices;
                auto first = TopExp::FirstVertex(TopoDS::Edge(edge));
                auto last = TopExp::LastVertex(TopoDS::Edge(edge));
                auto itFirst = vertexMap.find(first);
                auto itLast = vertexMap.find(last);
                if (itFirst == vertexMap.end() || itLast ==vertexMap.end())
                    continue;
                std::vector<TopoShape> faces;
                for (int idx : shape.findAncestors(edge, TopAbs_FACE))
                    faces.push_back(shape.getSubTopoShape(TopAbs_FACE, idx));
                if (faces.empty())
                    continue;
                for (int idx : shape.findAncestors(itFirst->second, TopAbs_EDGE)) {
                    auto e = shape.getSubTopoShape(TopAbs_EDGE, idx);
                    if (!e.findShape(itLast->second))
                        continue;
                    for (auto &face : faces) {
                        if (!face.findShape(e.getShape()))
                            continue;
                        auto &entry = edgeMap[edge];
                        if (entry.shapeSet.insert(e.getShape()).second)
                            entry.shapes.push_back(e.getShape());
                    }
                }
            }
        }
    }
    virtual const std::vector<TopoDS_Shape> &generated(const TopoDS_Shape &s) const override {
        _res.clear();
        switch(s.ShapeType()) {
        case TopAbs_VERTEX: {
            auto it = vertexMap.find(s);
            if (it != vertexMap.end()) {
                _res.push_back(it->second);
                return _res;
            }
            break;
        }
        case TopAbs_EDGE: {
            auto it = edgeMap.find(s);
            if (it != edgeMap.end())
                return it->second.shapes;
            break;
        }
        default:
            break;
        }
        MapperMaker::generated(s);
        return _res;
    }
};

TopoShape &TopoShape::makEShape(BRepFeat_MakePrism &mkShape,
                                const std::vector<TopoShape> &sources,
                                const TopoShape &upto,
                                const char *op)
{
    if(!op) op = Part::OpCodes::Prism;
    MapperPrism mapper(mkShape, upto);
    makESHAPE(mkShape.Shape(),mapper,sources,op);
    return *this;
}

TopoShape &TopoShape::makEShape(BRepPrimAPI_MakeHalfSpace &mkShape,
        const TopoShape &source, const char *op)
{
    if(!op) op = Part::OpCodes::HalfSpace;
    return makESHAPE(mkShape.Solid(),MapperMaker(mkShape),{source},op);
}

TopoShape &TopoShape::makEDraft(const TopoShape &shape, const std::vector<TopoShape> &_faces,
        const gp_Dir &pullDirection, double angle, const gp_Pln &neutralPlane,
        bool retry, const char *op)
{
    if(!op) op = Part::OpCodes::Draft;

    if(shape.isNull())
        HANDLE_NULL_SHAPE;

    std::vector<TopoShape> faces(_faces);
    bool done = true;
    BRepOffsetAPI_DraftAngle mkDraft;
    do {
        if(faces.empty())
            FC_THROWM(Base::CADKernelError,"no faces can be used");

        mkDraft.Init(shape.getShape());
        done = true;
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
    return makEShape(mkDraft,shape,op);
}

// deprecated, see Part::Feature::getRelatedElements()
#if 0
const std::vector<Data::MappedElement> &
TopoShape::getRelatedElements(const char *_name, bool sameType) const {

    INIT_SHAPE_CACHE();
    ShapeRelationKey key(getElementName(_name).name, sameType);
    auto it = _Cache->relations.find(key);
    if(it!=_Cache->relations.end())
        return it->second;

    Data::ElementIDRefs sids;
    QVector<Data::MappedElement> ret;
    const Data::MappedName &name = key.name;
    long tag;
    int len;
    std::string postfix;
    char type;
    // extract tag and source element name length
    if(findTagInElementName(name,&tag,&len,&postfix,&type,true) < 0)
        return ret;

    // recover the original element name
    Data::MappedName original(name.toBytes().constData(), len);

    std::ostringstream ss;

    // First, search the name in the previous modeling step.
    auto dehashed = dehashElementName(original);
    long tag2;
    char type2;
    if(findTagInElementName(dehashed,&tag2,0,0,&type2,true) < 0) {
        ss.str("");
        encodeElementName(type,dehashed,ss,&sids,0,tag);
    }else if(tag2!=tag && tag2!=-tag) {
        // Here means the dehashed element belongs to some other shape.
        // So just reset to original with middle markers stripped.
        dehashed = original + postfix;
    }
    auto element = getIndexedName(dehashed);
    if(element && (!sameType || type==element[0])) {
        ret.emplace_back(dehashed,element);
       _Cache->insertRelation(key, ret);
       return ret;
    }

    // Then, search any element that is modified from the given name
    ss.str("");
    ss << modPostfix();
    Data::MappedName modName(name);
    encodeElementName(type,modName,ss,&sids);
    bool found = false;
    if(findTagInElementName(modName,&tag,&len,nullptr,nullptr,true) >= 0) {
        std::string prefix;
        modName.toString(prefix, 0, len + (int)modPostfix().size());
        for(auto &v : getElementNamesWithPrefix(prefix.c_str())) {
            if((!sameType||type==v.index[0]) && v.name.endsWith(postfix)) {
                found = true;
                ret.push_back(v);
            }
        }
    }

    // Finally, search any element that are modified from the same source of
    // the given name
    if(!found) {
        std::string prefix;
        original.toString(prefix, Part::TopoShape::modPostfix());
        for(auto &v : getElementNamesWithPrefix(prefix.c_str())) {
            if((!sameType||type==v.index[0]) && v.name.endsWith(postfix))
                ret.push_back(v);
        }
    }
    _Cache->insertRelation(key, ret);
    return ret;
}
#endif

long TopoShape::isElementGenerated(const Data::MappedName &_name, int depth) const
{
    long res = 0;
    long tag = 0;
    traceElement(_name,
        [&] (const Data::MappedName &name, int offset, long tag2, long) {
            (void)offset;
            if(tag2 < 0)
                tag2 = -tag2;
            if(tag && tag2!=tag) {
                if(--depth < 1)
                    return true;
            }
            tag = tag2;
            if(depth==1 && name.startsWith(genPostfix(), offset)) {
                res = tag;
                return true;
            }
            return false;
        });

    return res;
}

void TopoShape::reTagElementMap(long tag, App::StringHasherRef hasher, const char *postfix) {
    if(!tag) {
        FC_WARN("invalid shape tag for re-tagging");
        return;
    }

    if(_Shape.IsNull())
        return;

    TopoShape tmp(*this);
    initCache(1);
    Hasher = hasher;
    Tag = tag;
    resetElementMap();
    copyElementMap(tmp, postfix);
}

void TopoShape::cacheRelatedElements(const Data::MappedName &name,
                                     bool sameType,
                                     const QVector<Data::MappedElement> & names) const
{
    INIT_SHAPE_CACHE();
    _Cache->insertRelation(ShapeRelationKey(name,sameType), names);
}

bool TopoShape::getRelatedElementsCached(const Data::MappedName &name,
                                         bool sameType,
                                         QVector<Data::MappedElement> &names) const
{
    if(!_Cache)
        return false;
    auto it = _Cache->relations.find(ShapeRelationKey(name,sameType));
    if(it == _Cache->relations.end())
        return false;
    names = it->second;
    return true;
}

bool TopoShape::findPlane(gp_Pln &pln, double tol, double atol) const {
    if(_Shape.IsNull())
        return false;
    if (tol < 0.0)
        tol = Precision::Confusion();
    if (atol < 0.0)
        atol = Precision::Angular();
    TopoDS_Shape shape;
    if (countSubShapes(TopAbs_EDGE) == 1) {
        // To deal with OCCT bug of wrong edge transformation
        shape = BRepBuilderAPI_Copy(_Shape).Shape();
    } else
        shape = _Shape;
    try {
        bool found = false;
        // BRepLib_FindSurface only really works on edges. We'll deal face first
        for (auto &shape : getSubShapes(TopAbs_FACE)) {
            gp_Pln plane;
            auto face = TopoDS::Face(shape);
            BRepAdaptor_Surface adapt(face);
            if (adapt.GetType() == GeomAbs_Plane) {
                plane = adapt.Plane();
            } else {
                TopLoc_Location loc;
                Handle(Geom_Surface) surf = BRep_Tool::Surface(face, loc);
                GeomLib_IsPlanarSurface check(surf);
                if (check.IsPlanar())
                    plane = check.Plan();
                else
                    return false;
            }
            if (!found) {
                found = true;
                pln = plane;
            } else if (!pln.Position().IsCoplanar(plane.Position(), tol, atol))
                return false;
        }

        // Check if there is free edges (i.e. edges does not belong to any face)
        if (TopExp_Explorer(getShape(), TopAbs_EDGE, TopAbs_FACE).More()) {
            // Copy shape to work around OCC transformation bug, that is, if
            // edge has transformation, but underlying geometry does not (or the
            // other way round), BRepLib_FindSurface returns a plane with the
            // wrong transformation
            BRepLib_FindSurface finder(BRepBuilderAPI_Copy(shape).Shape(),tol,Standard_True);
            if (!finder.Found())
                return false;
            pln = GeomAdaptor_Surface(finder.Surface()).Plane();
            found = true;
        }

        // Check for free vertexes
        auto vertexes = getSubShapes(TopAbs_VERTEX, TopAbs_EDGE);
        if (vertexes.size()) {
            if (!found && vertexes.size() > 2) {
                BRep_Builder builder;
                TopoDS_Compound comp;
                builder.MakeCompound(comp);
                for (int i=0, c=(int)vertexes.size()-1; i<c; ++i) {
                    builder.Add(comp, 
                            BRepBuilderAPI_MakeEdge(TopoDS::Vertex(vertexes[i]),
                                                    TopoDS::Vertex(vertexes[i+1])).Edge());
                }
                BRepLib_FindSurface finder(comp,tol,Standard_True);
                if (!finder.Found())
                    return false;
                pln = GeomAdaptor_Surface(finder.Surface()).Plane();
                return true;
            }

            double tt = tol * tol;
            for (auto &v : vertexes) {
                if (pln.SquareDistance(BRep_Tool::Pnt(TopoDS::Vertex(v))) > tt)
                    return false;
            }
        }

        // To make the returned plane normal more stable, if the shape has any
        // face, use the normal of the first face.
        if (hasSubShape(TopAbs_FACE)) {
            shape = getSubShape(TopAbs_FACE, 1);
            BRepAdaptor_Surface adapt(TopoDS::Face(shape));
            double u = adapt.FirstUParameter()
                + (adapt.LastUParameter() - adapt.FirstUParameter())/2.;
            double v = adapt.FirstVParameter()
                + (adapt.LastVParameter() - adapt.FirstVParameter())/2.;
            BRepLProp_SLProps prop(adapt,u,v,2,Precision::Confusion());
            if(prop.IsNormalDefined()) {
                gp_Pnt pnt; gp_Vec vec;
                // handles the orientation state of the shape
                BRepGProp_Face(TopoDS::Face(shape)).Normal(u,v,pnt,vec);
                pln = gp_Pln(pnt, gp_Dir(vec));
            }
        }
        return true;
    }catch (Standard_Failure &e) {
        // For some reason the above BRepBuilderAPI_Copy failed to copy
        // the geometry of some edge, causing exception with message
        // BRepAdaptor_Curve::No geometry. However, without the above
        // copy, circular edges often have the wrong transformation!
        FC_LOG("failed to find surface: " << e.GetMessageString());
        return false;
    }
}

bool TopoShape::isCoplanar(const TopoShape &other, double tol, double atol) const {
    if(isNull() || other.isNull())
        return false;
    if(_Shape.IsEqual(other._Shape))
        return true;
    gp_Pln pln1,pln2;
    if(!findPlane(pln1,tol) || !other.findPlane(pln2,tol))
        return false;
    if(tol<0.0)
        tol = Precision::Confusion();
    if (atol<0.0)
        atol = Precision::Angular();
    return pln1.Position().IsCoplanar(pln2.Position(),tol,atol);
}

std::vector<Data::IndexedName>
TopoShape::getHigherElements(const char *element, bool silent) const
{
    TopoShape shape = getSubTopoShape(element, silent);
    if(shape.isNull())
        return {};
    
    std::vector<Data::IndexedName> res;
    int type = shape.shapeType();
    for(;;) {
        if(--type < 0)
            break;
        const char *shapetype = shapeName((TopAbs_ShapeEnum)type).c_str();
        for(int idx : findAncestors(shape.getShape(), (TopAbs_ShapeEnum)type))
            res.emplace_back(shapetype, idx);
    }
    return res;
}

bool TopoShape::isSame(const Data::ComplexGeoData &_other) const
{
    if(!_other.isDerivedFrom(TopoShape::getClassTypeId()))
        return false;

    const auto &other = static_cast<const TopoShape &>(_other);
    return Tag == other.Tag
        && Hasher == other.Hasher
        && _Shape.IsEqual(other._Shape);
}

TopoShape & TopoShape::makEBSplineFace(const TopoShape & shape,
                                       FillingStyle style,
                                       bool keepBezier,
                                       const char *op)
{
    std::vector<TopoShape> input(1, shape);
    return makEBSplineFace(input, style, keepBezier, op);
}

TopoShape & TopoShape::makEBSplineFace(const std::vector<TopoShape> &input,
                                       FillingStyle style,
                                       bool keepBezier,
                                       const char *op)
{
    std::vector<TopoShape> edges;
    for (auto &s : input) {
        auto e = s.getSubTopoShapes(TopAbs_EDGE);
        edges.insert(edges.end(), e.begin(), e.end());
    }

    if (edges.size() == 1 && edges[0].isClosed()) {
        auto edge = edges[0].getSubShape(TopAbs_EDGE, 1);
        auto e = TopoDS::Edge(edge);
        auto v = TopExp::FirstVertex(e);
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(e, first, last);

        BRepBuilderAPI_MakeEdge mk1,mk2,mk3,mk4;
        Handle(Geom_BSplineCurve) bspline = Handle(Geom_BSplineCurve)::DownCast(curve);
        if (bspline.IsNull()) {
            ShapeConstruct_Curve scc;
            bspline = scc.ConvertToBSpline(curve, first, last, Precision::Confusion());
            if (bspline.IsNull())
                FC_THROWM(Base::CADKernelError, "Failed to convert edge to bspline");
            first = bspline->FirstParameter();
            last = bspline->LastParameter();
        }
        auto step = (last - first) * 0.25;
        auto m1 = first + step;
        auto m2 = m1 + step;
        auto m3 = m2 + step;
        auto c1 = GeomConvert::SplitBSplineCurve(bspline, first, m1, Precision::Confusion());
        auto c2 = GeomConvert::SplitBSplineCurve(bspline, m1, m2, Precision::Confusion());
        auto c3 = GeomConvert::SplitBSplineCurve(bspline, m2, m3, Precision::Confusion());
        auto c4 = GeomConvert::SplitBSplineCurve(bspline, m3, last, Precision::Confusion());
        mk1.Init(c1);
        mk2.Init(c2);
        mk3.Init(c3);
        mk4.Init(c4);

        if(!mk1.IsDone() || !mk2.IsDone() || !mk3.IsDone() || !mk4.IsDone())
            FC_THROWM(Base::CADKernelError, "Failed to split edge");

        auto e1 = mk1.Edge();
        auto e2 = mk2.Edge();
        auto e3 = mk3.Edge();
        auto e4 = mk4.Edge();

        ShapeMapper mapper;
        mapper.populate(true, e, {e1, e2, e3, e4});
        mapper.populate(false, v, {TopExp::FirstVertex(e1)});
        mapper.populate(false, v, {TopExp::LastVertex(e4)});

        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);
        builder.Add(comp, e1);
        builder.Add(comp, e2);
        builder.Add(comp, e3);
        builder.Add(comp, e4);

        TopoShape s;
        s.makESHAPE(comp, mapper, edges, Part::OpCodes::Split);
        return makEBSplineFace(s, style, op);
    }

    if (edges.size() < 2 || edges.size() > 4)
        FC_THROWM(Base::CADKernelError, "Require minimum one, maximum four edges");

    GeomFill_FillingStyle fstyle;
    switch (style) {
    case FillingStyle_Coons:
        fstyle = GeomFill_CoonsStyle;
        break;
    case FillingStyle_Curved:
        fstyle = GeomFill_CurvedStyle;
        break;
    default:
        fstyle = GeomFill_StretchStyle;
    }

    Handle(Geom_Surface) aSurface;

    Standard_Real u1, u2;
    if (keepBezier) {
        std::vector<Handle(Geom_BezierCurve)> curves;
        curves.reserve(4);
        for (const auto &e : edges) {
            const TopoDS_Edge& edge = TopoDS::Edge (e.getShape());
            TopLoc_Location heloc; // this will be output
            Handle(Geom_Curve) c_geom = BRep_Tool::Curve(edge, heloc, u1, u2);
            Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast(c_geom);
            if (!curve)
                break;
            curve->Transform(heloc.Transformation()); // apply original transformation to control points
            curves.push_back(curve);
        }
        if (curves.size() == edges.size()) {
            GeomFill_BezierCurves aSurfBuilder; //Create Surface Builder

            if (edges.size() == 2) {
                aSurfBuilder.Init(curves[0], curves[1], fstyle);
            }
            else if (edges.size() == 3) {
                aSurfBuilder.Init(curves[0], curves[1], curves[2], fstyle);
            }
            else if (edges.size() == 4) {
                aSurfBuilder.Init(curves[0], curves[1], curves[2], curves[3], fstyle);
            }
            aSurface = aSurfBuilder.Surface();
        }
    }

    if (aSurface.IsNull()) {
        std::vector<Handle(Geom_BSplineCurve)> curves;
        curves.reserve(4);
        for (const auto & e : edges) {
            const TopoDS_Edge& edge = TopoDS::Edge (e.getShape());
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

        GeomFill_BSplineCurves aSurfBuilder; //Create Surface Builder

        if (edges.size() == 2) {
            aSurfBuilder.Init(curves[0], curves[1], fstyle);
        }
        else if (edges.size() == 3) {
            aSurfBuilder.Init(curves[0], curves[1], curves[2], fstyle);
        }
        else if (edges.size() == 4) {
            aSurfBuilder.Init(curves[0], curves[1], curves[2], curves[3], fstyle);
        }

        aSurface = aSurfBuilder.Surface();
    }

    BRepBuilderAPI_MakeFace aFaceBuilder;
    Standard_Real v1, v2;
    // transfer surface bounds to face
    aSurface->Bounds(u1, u2, v1, v2);

    aFaceBuilder.Init(aSurface, u1, u2, v1, v2, Precision::Confusion());

    TopoShape aFace(0, Hasher, aFaceBuilder.Face());

    if (!aFaceBuilder.IsDone()) {
        FC_THROWM(Base::CADKernelError, "Face unable to be constructed");
    }
    if (aFace.isNull()) {
        FC_THROWM(Base::CADKernelError, "Resulting Face is null");
    }

    auto newEdges = aFace.getSubTopoShapes(TopAbs_EDGE);
    if (newEdges.size() != edges.size())
        FC_WARN("Face edge count mismatch");
    else {
        int i = 0;
        for (auto &edge : newEdges)
            edge.resetElementMap(edges[i++].elementMap());
        aFace.mapSubElement(newEdges);
    }

    Data::ElementIDRefs sids;
    Data::MappedName edgeName = aFace.getMappedName(
            Data::IndexedName::fromConst("Edge",1), true, &sids);
    aFace.setElementComboName(Data::IndexedName::fromConst("Face",1),
                              {edgeName},
                              Part::OpCodes::BSplineFace,
                              op,
                              &sids);
    *this = aFace;
    return *this;
}

static std::size_t
TopoShape_RefCountShapes(std::unordered_set<TopoDS_Shape> &shapeSet,
                         const TopoDS_Shape& aShape)
{
    std::size_t size = sizeof(Base::Placement); // rough estimate of location size
    if (aShape.IsNull())
        return size;

    // Recurse only for distinctive geometry shape. Other shapes are only
    // referenced with a location
    if (!shapeSet.insert(aShape.Located(TopLoc_Location())).second)
        return size;

    TopoDS_Iterator it;
    // go through all direct children
    for (it.Initialize(aShape, false, false);it.More(); it.Next()) {
        size += TopoShape_RefCountShapes(shapeSet, aShape);
    }

    return size;
}

std::size_t TopoShape::Cache::getMemSize()
{
    if (this->memsize || this->shape.IsNull())
        return this->memsize;

    std::unordered_set<TopoDS_Shape> shapeSet;
    this->memsize = TopoShape_RefCountShapes(shapeSet, this->shape);

    for (const auto & shape : shapeSet) {
        // Only check geometrical element for non compound shapes
        if (TopoDS_Iterator(shape, false, false).More())
            continue;

        // add the size of the underlying geomtric data
        Handle(TopoDS_TShape) tshape = shape.TShape();
        this->memsize += tshape->DynamicType()->Size();

        switch (shape.ShapeType())
        {
        case TopAbs_FACE:
            {
                // first, last, tolerance
                this->memsize += 5*sizeof(Standard_Real);
                const TopoDS_Face& face = TopoDS::Face(shape);
                // if no geometry is attached to a face an exception is raised
                BRepAdaptor_Surface surface;
                try {
                    surface.Initialize(face);
                }
                catch (const Standard_Failure&) {
                    continue;
                }
                switch (surface.GetType())
                {
                case GeomAbs_Plane:
                    this->memsize += sizeof(Geom_Plane);
                    break;
                case GeomAbs_Cylinder:
                    this->memsize += sizeof(Geom_CylindricalSurface);
                    break;
                case GeomAbs_Cone:
                    this->memsize += sizeof(Geom_ConicalSurface);
                    break;
                case GeomAbs_Sphere:
                    this->memsize += sizeof(Geom_SphericalSurface);
                    break;
                case GeomAbs_Torus:
                    this->memsize += sizeof(Geom_ToroidalSurface);
                    break;
                case GeomAbs_BezierSurface:
                    this->memsize += sizeof(Geom_BezierSurface);
                    this->memsize += (surface.NbUPoles()*surface.NbVPoles()) * sizeof(Standard_Real);
                    this->memsize += (surface.NbUPoles()*surface.NbVPoles()) * sizeof(Geom_CartesianPoint);
                    break;
                case GeomAbs_BSplineSurface:
                    this->memsize += sizeof(Geom_BSplineSurface);
                    this->memsize += (surface.NbUKnots()+surface.NbVKnots()) * sizeof(Standard_Real);
                    this->memsize += (surface.NbUPoles()*surface.NbVPoles()) * sizeof(Standard_Real);
                    this->memsize += (surface.NbUPoles()*surface.NbVPoles()) * sizeof(Geom_CartesianPoint);
                    break;
                case GeomAbs_SurfaceOfRevolution:
                    this->memsize += sizeof(Geom_SurfaceOfRevolution);
                    break;
                case GeomAbs_SurfaceOfExtrusion:
                    this->memsize += sizeof(Geom_SurfaceOfLinearExtrusion);
                    break;
                case GeomAbs_OtherSurface:
                    // What kind of surface should this be?
                    this->memsize += sizeof(Geom_Surface);
                    break;
                default:
                    break;
                }
            } break;
        case TopAbs_EDGE:
            {
                // first, last, tolerance
                this->memsize += 3*sizeof(Standard_Real);
                // if no geometry is attached to an edge an exception is raised
                const TopoDS_Edge& edge = TopoDS::Edge(shape);
                BRepAdaptor_Curve curve;
                try {
                    curve.Initialize(edge);
                }
                catch (const Standard_Failure&) {
                    continue;
                }
                switch (curve.GetType())
                {
                case GeomAbs_Line:
                    this->memsize += sizeof(Geom_Line);
                    break;
                case GeomAbs_Circle:
                    this->memsize += sizeof(Geom_Circle);
                    break;
                case GeomAbs_Ellipse:
                    this->memsize += sizeof(Geom_Ellipse);
                    break;
                case GeomAbs_Hyperbola:
                    this->memsize += sizeof(Geom_Hyperbola);
                    break;
                case GeomAbs_Parabola:
                    this->memsize += sizeof(Geom_Parabola);
                    break;
                case GeomAbs_BezierCurve:
                    this->memsize += sizeof(Geom_BezierCurve);
                    this->memsize += curve.NbPoles() * sizeof(Standard_Real);
                    this->memsize += curve.NbPoles() * sizeof(Geom_CartesianPoint);
                    break;
                case GeomAbs_BSplineCurve:
                    this->memsize += sizeof(Geom_BSplineCurve);
                    this->memsize += curve.NbKnots() * sizeof(Standard_Real);
                    this->memsize += curve.NbPoles() * sizeof(Standard_Real);
                    this->memsize += curve.NbPoles() * sizeof(Geom_CartesianPoint);
                    break;
                case GeomAbs_OtherCurve:
                    // What kind of curve should this be?
                    this->memsize += sizeof(Geom_Curve);
                    break;
                default:
                    break;
                }
            } break;
        case TopAbs_VERTEX:
            {
                // tolerance
                this->memsize += sizeof(Standard_Real);
                this->memsize += sizeof(Geom_CartesianPoint);
            } break;
        default:
            break;
        }
    }

    return this->memsize;
}

unsigned int TopoShape::getMemSize (void) const
{
    INIT_SHAPE_CACHE();
    return _Cache->getMemSize() + Data::ComplexGeoData::getMemSize();
}

TopoShape TopoShape::splitWires(std::vector<TopoShape> *inner,
                                SplitWireReorient reorient) const
{
    // ShapeAnalysis::OuterWire() is un-reliable for some reason. OCC source
    // code shows it works by creating face using each wire, and then test using
    // BRepTopAdaptor_FClass2d::PerformInfinitePoint() to check if it is an out
    // bound wire. And practice shows it sometimes returns the in correct
    // result.  Need more investigation. Note that this may be related to
    // unreliable solid face orientation
    // (https://forum.freecadweb.org/viewtopic.php?p=446006#p445674)
    //
    // Use BrepTools::OuterWire() instead. OCC source code shows it is
    // implemented using simple bound box checking. This should be a
    // reliable method, especially so for a planar face.

    TopoDS_Shape tmp;
    if (shapeType(true) == TopAbs_FACE)
        tmp = BRepTools::OuterWire(TopoDS::Face(_Shape));
    else if (countSubShapes(TopAbs_FACE) == 1)
        tmp = BRepTools::OuterWire(
                TopoDS::Face(getSubShape(TopAbs_FACE, 1)));
    if (tmp.IsNull())
        return TopoShape();
    const auto & wires = getSubTopoShapes(TopAbs_WIRE);
    auto it = wires.begin();

    TopAbs_Orientation orientOuter, orientInner;
    switch(reorient) {
    case ReorientReversed:
        orientOuter = orientInner = TopAbs_REVERSED;
        break;
    case ReorientForward:
        orientOuter = orientInner = TopAbs_FORWARD;
        break;
    default:
        orientOuter = TopAbs_FORWARD;
        orientInner = TopAbs_REVERSED;
        break;
    }

    auto doReorient = [](TopoShape &s, TopAbs_Orientation orient) {
        // Special case of single edge wire. Make sure the edge is in the
        // required orientation. This is necessary because BRepFill_OffsetWire
        // has special handling of circular edge offset, which seem to only
        // respect the edge orientation and disregard the wire orientation. The
        // orientation is used to determine whether to shrink or expand.
        if (s.countSubShapes(TopAbs_EDGE) == 1) {
            TopoDS_Shape e = s.getSubShape(TopAbs_EDGE, 1);
            if (e.Orientation() == orient) {
                if (s._Shape.Orientation() == orient)
                    return;
            } else
                e = e.Oriented(orient);
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(e));
            s.setShape(mkWire.Shape(), false);
        }
        else if (s._Shape.Orientation() != orient)
            s.setShape(s._Shape.Oriented(orient), false);
    };

    for (; it != wires.end(); ++it) {
        auto & wire = *it;
        if (wire.getShape().IsSame(tmp)) {
            if (inner) {
                for (++it; it != wires.end(); ++it) {
                    inner->push_back(*it);
                    if (reorient)
                        doReorient(inner->back(), orientInner);
                }
            }
            auto res = wire;
            if (reorient)
                doReorient(res, orientOuter);
            return res;
        }
        if (inner) {
            inner->push_back(wire);
            if (reorient)
                doReorient(inner->back(), orientInner);
        }
    }
    return TopoShape();
}

bool TopoShape::isLinearEdge(Base::Vector3d *dir, Base::Vector3d *base) const
{
    if (isNull() || getShape().ShapeType() != TopAbs_EDGE)
        return false;

    if (!GeomCurve::isLinear(BRepAdaptor_Curve(TopoDS::Edge(getShape())).Curve().Curve(), dir, base))
        return false;
    if (dir || base) {
        auto pla = getPlacement();
        if (dir) {
            Base::Vector3d p0, p1;
            pla.multVec(Base::Vector3d(), p0);
            pla.multVec(*dir, p1);
            *dir = p1 - p0;
        }
        if (base)
            pla.multVec(*base, *base);
    }
    return true;
}

bool TopoShape::isPlanarFace(double tol) const
{
    if (isNull() || getShape().ShapeType() != TopAbs_FACE)
        return false;

    return GeomSurface::isPlanar(
            BRepAdaptor_Surface(TopoDS::Face(getShape())).Surface().Surface(), nullptr, tol);
}

bool TopoShape::linearize(bool face, bool edge)
{
    bool touched = false;
    BRep_Builder builder;
    // Note: changing edge geometry seems to mess up with face (or shell, or solid)
    // Probably need to do some fix afterwards.
    if (edge) {
        for (auto & edge : getSubTopoShapes(TopAbs_EDGE)) {
            TopoDS_Edge e = TopoDS::Edge(edge.getShape());
            BRepAdaptor_Curve curve(e);
            if (curve.GetType() == GeomAbs_Line || !edge.isLinearEdge())
                continue;
            std::unique_ptr<Geometry> geo(
                    Geometry::fromShape(e.Located(TopLoc_Location()).Oriented(TopAbs_FORWARD)));
            std::unique_ptr<Geometry> gline(static_cast<GeomCurve*>(geo.get())->toLine());
            if (gline) {
                touched = true;
                builder.UpdateEdge(e,
                                   Handle(Geom_Curve)::DownCast(gline->handle()),
                                   e.Location(),
                                   BRep_Tool::Tolerance(e));
            }
        }
    }
    if (face) {
        for (auto & face : getSubTopoShapes(TopAbs_FACE)) {
            TopoDS_Face f = TopoDS::Face(face.getShape());
            BRepAdaptor_Surface surf(f);
            if (surf.GetType() == GeomAbs_Plane || !face.isPlanarFace())
                continue;
            std::unique_ptr<Geometry> geo(
                    Geometry::fromShape(f.Located(TopLoc_Location()).Oriented(TopAbs_FORWARD)));
            std::unique_ptr<Geometry> gplane(static_cast<GeomSurface*>(geo.get())->toPlane());
            if (gplane) {
                touched = true;
                builder.UpdateFace(f, 
                                Handle(Geom_Surface)::DownCast(gplane->handle()),
                                f.Location(),
                                BRep_Tool::Tolerance(f));
            }
        }
    }
    return touched;
}

bool TopoShape::getRotation(Base::Rotation& rot) const
{
    if (_Shape.IsNull())
        return false;

    int facecount = countSubShapes(TopAbs_FACE);
    if (facecount == 0) {
        int edgecount = countSubShapes(TopAbs_EDGE);
        if (edgecount == 0)
            return false;
        if (edgecount == 1 && isLinearEdge()) {
            if (std::unique_ptr<Geometry> geo = Geometry::fromShape(getSubShape(TopAbs_EDGE, 1))) {
                std::unique_ptr<GeomLine> gline(static_cast<GeomCurve*>(geo.get())->toLine());
                if (gline) {
                    rot = Base::Rotation(Base::Vector3d(0,0,1), gline->getDir());
                    return true;
                }
            }
        }
    } else if (facecount == 1) {
        if (std::unique_ptr<Geometry> geo = Geometry::fromShape(getSubShape(TopAbs_FACE, 1))) {
            if (geo->isDerivedFrom(GeomElementarySurface::getClassTypeId())) {
                auto dir = static_cast<GeomElementarySurface*>(geo.get())->getDir();
                auto xdir = static_cast<GeomElementarySurface*>(geo.get())->getXDir();
                rot = Base::Rotation(Base::Vector3d(0,0,1), dir);
                auto xd = rot.multVec(xdir);
                rot *= Base::Rotation(Base::Vector3d(1,0,0), xd);
                return true;
            }
        }
    }

    gp_Pln pln;
    if (!findPlane(pln))
        return false;

    auto dir = pln.Position().Direction();
    auto xdir = pln.Position().XDirection();
    rot = Base::Rotation(Base::Vector3d(0,0,1), Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
    auto xd = rot.multVec(Base::Vector3d(xdir.X(), xdir.Y(), xdir.Z()));
    rot *= Base::Rotation(Base::Vector3d(1,0,0), xd);
    return true;
}

Data::MappedName
TopoShape::renameDuplicateElement(int index,
                                  const Data::IndexedName & element, 
                                  const Data::IndexedName & element2, 
                                  const Data::MappedName & name,
                                  Data::ElementIDRefs &sids)
{
    // This function is overridden for debugging purpose only
    //
    // std::string tmp;
    // element2.toString(tmp);
    // Part::Feature::create(getSubTopoShape(
    //             shapeType(element2.getType()), element2.getIndex()), tmp.c_str());

    return Data::ComplexGeoData::renameDuplicateElement(index, element, element2, name, sids);
}

TopoDS_Shape &TopoShape::move(TopoDS_Shape &s, const TopLoc_Location &loc)
{
# if OCC_VERSION_HEX < 0x070600
    s.Move(loc);
#else
    s.Move(loc, false);
#endif
    return s;
}

TopoDS_Shape TopoShape::moved(const TopoDS_Shape &s, const TopLoc_Location &loc)
{
# if OCC_VERSION_HEX < 0x070600
    return s.Moved(loc);
#else
    return s.Moved(loc, false);
#endif
}

TopoDS_Shape &TopoShape::move(TopoDS_Shape &s, const gp_Trsf &trsf)
{
# if OCC_VERSION_HEX < 0x070600
    if (std::abs(trsf.ScaleFactor()) > 1e-14)
#else
    if (std::abs(trsf.ScaleFactor()) > TopLoc_Location::ScalePrec())
#endif
    {
        auto trsfCopy(trsf);
        trsfCopy.SetScaleFactor(1.0);
        s.Move(trsfCopy);
    }
    else
        s.Move(trsf);
    return s;
}

TopoDS_Shape TopoShape::moved(const TopoDS_Shape &s, const gp_Trsf &trsf)
{
    TopoDS_Shape sCopy(s);
    return move(sCopy, trsf);
}

TopoDS_Shape &TopoShape::locate(TopoDS_Shape &s, const TopLoc_Location &loc)
{
    s.Location(TopLoc_Location());
    return move(s, loc);
}

TopoDS_Shape TopoShape::located(const TopoDS_Shape &s, const TopLoc_Location &loc)
{
    auto sCopy(s);
    sCopy.Location(TopLoc_Location());
    return moved(sCopy, loc);
}

TopoDS_Shape &TopoShape::locate(TopoDS_Shape &s, const gp_Trsf &trsf)
{
    s.Location(TopLoc_Location());
    return move(s, trsf);
}

TopoDS_Shape TopoShape::located(const TopoDS_Shape &s, const gp_Trsf &trsf)
{
    auto sCopy(s);
    sCopy.Location(TopLoc_Location());
    return moved(sCopy, trsf);
}

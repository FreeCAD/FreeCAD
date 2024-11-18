// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2022 Zheng, Lei <realthunder.dev@gmail.com>              *
 *   Copyright (c) 2023 FreeCAD Project Association                         *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
#include <cmath>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_CompCurve.hxx>
#if OCC_VERSION_HEX < 0x070600
#include <BRepAdaptor_HCurve.hxx>
#include <BRepAdaptor_HCompCurve.hxx>
#endif

#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepFill.hxx>
#include <BRepFill_Generator.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Mod/Part/App/FCBRepAlgoAPI_BooleanOperation.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Common.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Section.h>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_FindPlane.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_NurbsConvert.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepLib.hxx>
#include <BRepOffsetAPI_DraftAngle.hxx>
#include <BRepOffsetAPI_MakeFilling.hxx>
#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepOffsetAPI_MakeEvolved.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepProj_Projection.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <GeomConvert.hxx>
#include <GeomFill_BezierCurves.hxx>
#include <GeomFill_BSplineCurves.hxx>
#include <Precision.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <ShapeBuild_ReShape.hxx>
#include <ShapeConstruct_Curve.hxx>
#include <ShapeUpgrade_ShellSewing.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <gp_Pln.hxx>

#include <utility>

#endif

#if OCC_VERSION_HEX >= 0x070500
#include <OSD_Parallel.hxx>
#endif

#include "modelRefine.h"
#include "CrossSection.h"
#include "TopoShape.h"
#include "TopoShapeOpCode.h"
#include "TopoShapeCache.h"
#include "TopoShapeMapper.h"
#include "FaceMaker.h"
#include "Geometry.h"
#include "BRepOffsetAPI_MakeOffsetFix.h"
#include "Base/Tools.h"
#include "Base/BoundBox.h"

#include <App/ElementMap.h>
#include <App/ElementNamingUtils.h>
#include <ShapeAnalysis_FreeBoundsProperties.hxx>
#include <BRepFeat_MakeRevol.hxx>

#include "Tools.h"

FC_LOG_LEVEL_INIT("TopoShape", true, true)  // NOLINT

#if OCC_VERSION_HEX >= 0x070600
using Adaptor3d_HCurve = Adaptor3d_Curve;
using BRepAdaptor_HCurve = BRepAdaptor_Curve;
using BRepAdaptor_HCompCurve = BRepAdaptor_CompCurve;
#endif

namespace Part
{

static void expandCompound(const TopoShape& shape, std::vector<TopoShape>& res)
{
    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null input shape");
    }
    if (shape.getShape().ShapeType() != TopAbs_COMPOUND) {
        res.push_back(shape);
        return;
    }
    for (auto& s : shape.getSubTopoShapes()) {
        expandCompound(s, res);
    }
}

void TopoShape::initCache(int reset) const
{
    if (reset > 0 || !_cache || _cache->isTouched(_Shape)) {
        if (_parentCache) {
            _parentCache.reset();
            _subLocation.Identity();
        }
        _cache = std::make_shared<TopoShapeCache>(_Shape);
    }
}

Data::ElementMapPtr TopoShape::resetElementMap(Data::ElementMapPtr elementMap)
{
    if (_cache && elementMap != this->elementMap(false)) {
        for (auto& info : _cache->shapeAncestryCache) {
            info.clear();
        }
    }
    else {
        initCache();
    }
    if (elementMap) {
        _cache->cachedElementMap = elementMap;
        _cache->subLocation.Identity();
        _subLocation.Identity();
        _parentCache.reset();
    }
    return Data::ComplexGeoData::resetElementMap(elementMap);
}

void TopoShape::flushElementMap() const
{
    initCache();
    if (!elementMap(false) && this->_cache) {
        if (this->_cache->cachedElementMap) {
            const_cast<TopoShape*>(this)->resetElementMap(this->_cache->cachedElementMap);
        }
        else if (this->_parentCache) {
            TopoShape parent(this->Tag, this->Hasher, this->_parentCache->shape);
            parent._cache = _parentCache;
            parent.flushElementMap();
            TopoShape self(this->Tag,
                           this->Hasher,
                           this->_Shape.Located(this->_subLocation * this->_cache->subLocation));
            self._cache = _cache;
            self.mapSubElement(parent);
            this->_parentCache.reset();
            this->_subLocation.Identity();
            const_cast<TopoShape*>(this)->resetElementMap(self.elementMap());
        }
    }
}

void TopoShape::setShape(const TopoDS_Shape& shape, bool resetElementMap)
{
    if (resetElementMap) {
        this->resetElementMap();
    }
    else if (_cache && _cache->isTouched(shape)) {
        this->flushElementMap();
    }
    //_Shape._Shape = shape; // TODO: Replace the next line with this once ShapeProtector is
    // available.
    _Shape = shape;
    if (_cache) {
        initCache();
    }
}


TopoDS_Shape& TopoShape::move(TopoDS_Shape& tds, const TopLoc_Location& location)
{
#if OCC_VERSION_HEX < 0x070600
    tds.Move(location);
#else
    tds.Move(location, false);
#endif
    return tds;
}

TopoDS_Shape TopoShape::moved(const TopoDS_Shape& tds, const TopLoc_Location& location)
{
#if OCC_VERSION_HEX < 0x070600
    return tds.Moved(location);
#else
    return tds.Moved(location, false);
#endif
}

TopoDS_Shape& TopoShape::move(TopoDS_Shape& tds, const gp_Trsf& transfer)
{
#if OCC_VERSION_HEX < 0x070600
    static constexpr double scalePrecision {1e-14};
    if (std::abs(transfer.ScaleFactor()) > scalePrecision)
#else
    if (std::abs(transfer.ScaleFactor()) > TopLoc_Location::ScalePrec())
#endif
    {
        auto transferCopy(transfer);
        transferCopy.SetScaleFactor(1.0);
        tds.Move(transferCopy);
    }
    else {
        tds.Move(transfer);
    }
    return tds;
}

TopoDS_Shape TopoShape::moved(const TopoDS_Shape& tds, const gp_Trsf& transfer)
{
    TopoDS_Shape sCopy(tds);
    return move(sCopy, transfer);
}

TopoDS_Shape& TopoShape::locate(TopoDS_Shape& tds, const TopLoc_Location& loc)
{
    tds.Location(TopLoc_Location());
    return move(tds, loc);
}

TopoDS_Shape TopoShape::located(const TopoDS_Shape& tds, const TopLoc_Location& loc)
{
    auto sCopy(tds);
    sCopy.Location(TopLoc_Location());
    return moved(sCopy, loc);
}

TopoDS_Shape& TopoShape::locate(TopoDS_Shape& tds, const gp_Trsf& transfer)
{
    tds.Location(TopLoc_Location());
    return move(tds, transfer);
}

TopoDS_Shape TopoShape::located(const TopoDS_Shape& tds, const gp_Trsf& transfer)
{
    auto sCopy(tds);
    sCopy.Location(TopLoc_Location());
    return moved(sCopy, transfer);
}

void TopoShape::operator=(const TopoShape& sh)
{
    if (this != &sh) {
        this->setShape(sh._Shape, true);
        this->Tag = sh.Tag;
        this->Hasher = sh.Hasher;
        this->_cache = sh._cache;
        this->_parentCache = sh._parentCache;
        this->_subLocation = sh._subLocation;
        resetElementMap(sh.elementMap(false));
    }
}

int TopoShape::findShape(const TopoDS_Shape& subshape) const
{
    initCache();
    return _cache->findShape(_Shape, subshape);
}


TopoDS_Shape TopoShape::findShape(const char* name) const
{
    if (!name) {
        return {};
    }

    Data::MappedElement res = getElementName(name);
    if (!res.index) {
        return {};
    }

    auto idx = shapeTypeAndIndex(name);
    if (idx.second == 0) {
        return {};
    }
    initCache();
    return _cache->findShape(_Shape, idx.first, idx.second);
}

TopoDS_Shape TopoShape::findShape(TopAbs_ShapeEnum type, int idx) const
{
    initCache();
    return _cache->findShape(_Shape, type, idx);
}

std::vector<TopoShape> TopoShape::findSubShapesWithSharedVertex(const TopoShape& subshape,
                                                                std::vector<std::string>* names,
                                                                Data::SearchOptions options,
                                                                double tol,
                                                                double atol) const
{
    bool checkGeometry = options.testFlag(Data::SearchOption::CheckGeometry);
    bool singleSearch = options.testFlag(Data::SearchOption::SingleResult);
    std::vector<TopoShape> res;
    if (subshape.isNull() || this->isNull()) {
        return res;
    }
    double tol2 = tol * tol;
    int index = 0;
    TopAbs_ShapeEnum shapeType = subshape.shapeType();

    // This is an intentionally recursive method, which will exit after looking through all
    // ancestors.
    auto searchCompositeShape = [&](TopAbs_ShapeEnum childType) {  // NOLINT (misc-no-recu2rsive)
        unsigned long count = subshape.countSubShapes(childType);
        if (!count) {
            return;
        }
        auto first = subshape.getSubTopoShape(childType, 1);
        for (const auto& child :
             findSubShapesWithSharedVertex(first, nullptr, options, tol, atol)) {
            for (int idx : findAncestors(child.getShape(), shapeType)) {
                auto shape = getSubTopoShape(shapeType, idx);
                if (shape.countSubShapes(childType) != count) {
                    continue;
                }
                bool found = true;
                for (unsigned long i = 2; i < count; ++i) {
                    if (shape
                            .findSubShapesWithSharedVertex(subshape.getSubTopoShape(childType, i),
                                                           nullptr,
                                                           options,
                                                           tol,
                                                           atol)
                            .empty()) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    res.push_back(shape);
                    if (names) {
                        names->push_back(shapeName(shapeType) + std::to_string(idx));
                    }
                    if (singleSearch) {
                        return;
                    }
                }
            }
        }
    };

    switch (shapeType) {
        case TopAbs_WIRE:
            searchCompositeShape(TopAbs_EDGE);
            break;
        case TopAbs_SHELL:
            searchCompositeShape(TopAbs_FACE);
            break;
        case TopAbs_SOLID:
            searchCompositeShape(TopAbs_SHELL);
            break;
        case TopAbs_COMPSOLID:
            searchCompositeShape(TopAbs_SOLID);
            break;
        case TopAbs_COMPOUND:
            // special treatment of single sub-shape compound, that is, search
            // its extracting the compound
            if (countSubShapes(TopAbs_SHAPE) == 1) {
                return findSubShapesWithSharedVertex(subshape.getSubTopoShape(TopAbs_SHAPE, 1),
                                                     names,
                                                     options,
                                                     tol,
                                                     atol);
            }
            else if (unsigned long count = countSubShapes(TopAbs_SHAPE)) {
                // For multi-sub-shape compound, only search for compound with the same
                // structure
                int idx = 0;
                for (const auto& compound : getSubTopoShapes(shapeType)) {
                    ++idx;
                    if (compound.countSubShapes(TopAbs_SHAPE) != count) {
                        continue;
                    }
                    int i = 0;
                    bool found = true;
                    for (const auto& s : compound.getSubTopoShapes(TopAbs_SHAPE)) {
                        ++i;
                        auto ss = subshape.getSubTopoShape(TopAbs_SHAPE, i);
                        if (ss.isNull() && s.isNull()) {
                            continue;
                        }
                        auto options2 = options;
                        options2.setFlag(Data::SearchOption::SingleResult);
                        if (ss.isNull() || s.isNull() || ss.shapeType() != s.shapeType()
                            || ss.findSubShapesWithSharedVertex(s, nullptr, options2, tol, atol)
                                   .empty()) {
                            found = false;
                            break;
                        }
                    }
                    if (found) {
                        if (names) {
                            names->push_back(shapeName(shapeType) + std::to_string(idx));
                        }
                        res.push_back(compound);
                        if (singleSearch) {
                            return res;
                        }
                    }
                }
            }
            break;
        case TopAbs_VERTEX:
            // Vertex search will do comparison with tolerance to account for
            // rounding error inccured through transformation.
            for (auto& shape : getSubTopoShapes(TopAbs_VERTEX)) {
                ++index;
                if (BRep_Tool::Pnt(TopoDS::Vertex(shape.getShape()))
                        .SquareDistance(BRep_Tool::Pnt(TopoDS::Vertex(subshape.getShape())))
                    <= tol2) {
                    if (names) {
                        names->push_back(std::string("Vertex") + std::to_string(index));
                    }
                    res.push_back(shape);
                    if (singleSearch) {
                        return res;
                    }
                }
            }
            break;
        case TopAbs_EDGE:
        case TopAbs_FACE: {
            std::unique_ptr<Geometry> geom;
            bool isLine = false;
            bool isPlane = false;

            std::vector<TopoDS_Shape> vertices;
            TopoShape wire;
            if (shapeType == TopAbs_FACE) {
                wire = subshape.splitWires();
                vertices = wire.getSubShapes(TopAbs_VERTEX);
            }
            else {
                vertices = subshape.getSubShapes(TopAbs_VERTEX);
            }

            if (vertices.empty() || checkGeometry) {
                geom = Geometry::fromShape(subshape.getShape(), true);
                if (!geom) {
                    return res;
                }
                if (shapeType == TopAbs_EDGE) {
                    isLine = (geom->isDerivedFrom(GeomLine::getClassTypeId())
                              || geom->isDerivedFrom(GeomLineSegment::getClassTypeId()));
                }
                else {
                    isPlane = geom->isDerivedFrom(GeomPlane::getClassTypeId());
                }
            }

            auto compareGeometry = [&](const TopoShape& s, bool strict) {
                std::unique_ptr<Geometry> g2(Geometry::fromShape(s.getShape(), true));
                if (!g2) {
                    return false;
                }
                if (isLine && !strict) {
                    // For lines, don't compare geometry, just check the
                    // vertices below instead, because the exact same edge
                    // may have different geometrical representation.
                    if (!g2->isDerivedFrom(GeomLine::getClassTypeId())
                        && !g2->isDerivedFrom(GeomLineSegment::getClassTypeId())) {
                        return false;
                    }
                }
                else if (isPlane && !strict) {
                    // For planes, don't compare geometry either, so that
                    // we don't need to worry about orientation and so on.
                    // Just check the edges.
                    if (!g2->isDerivedFrom(GeomPlane::getClassTypeId())) {
                        return false;
                    }
                }
                else if (!g2 || !g2->isSame(*geom, tol, atol)) {
                    return false;
                }
                return true;
            };

            if (vertices.empty()) {
                // Probably an infinite shape, so we have to search by geometry
                int idx = 0;
                for (auto& shape : getSubTopoShapes(shapeType)) {
                    ++idx;
                    if (!shape.countSubShapes(TopAbs_VERTEX) && compareGeometry(shape, true)) {
                        if (names) {
                            names->push_back(shapeName(shapeType) + std::to_string(idx));
                        }
                        res.push_back(shape);
                        if (singleSearch) {
                            return res;
                        }
                    }
                }
                break;
            }

            // The basic idea of shape search is about the same for both edge and face.
            // * Search the first vertex, which is done with tolerance.
            // * Find the ancestor shape of the found vertex
            // * Compare each vertex of the ancestor shape and the input shape
            // * Perform geometry comparison of the ancestor and input shape.
            //      * For face, perform addition geometry comparison of each edge.
            std::unordered_set<TopoShape, ShapeHasher, ShapeHasher> shapeSet;
            for (auto& vert :
                 findSubShapesWithSharedVertex(vertices[0], nullptr, options, tol, atol)) {
                for (auto idx : findAncestors(vert.getShape(), shapeType)) {
                    auto shape = getSubTopoShape(shapeType, idx);
                    if (!shapeSet.insert(shape).second) {
                        continue;
                    }
                    TopoShape otherWire;
                    std::vector<TopoDS_Shape> otherVertices;
                    if (shapeType == TopAbs_FACE) {
                        otherWire = shape.splitWires();
                        if (wire.countSubShapes(TopAbs_EDGE)
                            != otherWire.countSubShapes(TopAbs_EDGE)) {
                            continue;
                        }
                        otherVertices = otherWire.getSubShapes(TopAbs_VERTEX);
                    }
                    else {
                        otherVertices = shape.getSubShapes(TopAbs_VERTEX);
                    }
                    if (otherVertices.size() != vertices.size()) {
                        continue;
                    }
                    if (checkGeometry && !compareGeometry(shape, false)) {
                        continue;
                    }
                    unsigned ind = 0;
                    bool matched = true;
                    for (auto& vertex : vertices) {
                        bool found = false;
                        for (unsigned inner = 0; inner < otherVertices.size(); ++inner) {
                            auto& vertex1 = otherVertices[ind];
                            if (++ind == otherVertices.size()) {
                                ind = 0;
                            }
                            if (BRep_Tool::Pnt(TopoDS::Vertex(vertex))
                                    .SquareDistance(BRep_Tool::Pnt(TopoDS::Vertex(vertex1)))
                                <= tol2) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            matched = false;
                            break;
                        }
                    }
                    if (!matched) {
                        continue;
                    }

                    if (shapeType == TopAbs_FACE && checkGeometry) {
                        // Is it really necessary to check geometries of each edge of a face?
                        // Right now we only do outer wire check
                        auto otherEdges = otherWire.getSubShapes(TopAbs_EDGE);
                        std::vector<std::unique_ptr<Geometry>> geos;
                        geos.resize(otherEdges.size());
                        bool matched2 = true;
                        unsigned i = 0;
                        auto edges = wire.getSubShapes(TopAbs_EDGE);
                        for (auto& edge : edges) {
                            std::unique_ptr<Geometry> geom2(Geometry::fromShape(edge, true));
                            if (!geom2) {
                                matched2 = false;
                                break;
                            }
                            bool isLine2 = false;
                            gp_Pnt pt1, pt2;
                            if (geom2->isDerivedFrom(GeomLine::getClassTypeId())
                                || geom2->isDerivedFrom(GeomLineSegment::getClassTypeId())) {
                                pt1 = BRep_Tool::Pnt(TopExp::FirstVertex(TopoDS::Edge(edge)));
                                pt2 = BRep_Tool::Pnt(TopExp::LastVertex(TopoDS::Edge(edge)));
                                isLine2 = true;
                            }
                            // We will tolerate on edge reordering
                            bool found = false;
                            for (unsigned j = 0; j < otherEdges.size(); j++) {
                                auto& e1 = otherEdges[i];
                                auto& g1 = geos[i];
                                if (++i >= otherEdges.size()) {
                                    i = 0;
                                }
                                if (!g1) {
                                    g1 = Geometry::fromShape(e1, true);
                                    if (!g1) {
                                        break;
                                    }
                                }
                                if (isLine2) {
                                    if (g1->isDerivedFrom(GeomLine::getClassTypeId())
                                        || g1->isDerivedFrom(GeomLineSegment::getClassTypeId())) {
                                        auto p1 =
                                            BRep_Tool::Pnt(TopExp::FirstVertex(TopoDS::Edge(e1)));
                                        auto p2 =
                                            BRep_Tool::Pnt(TopExp::LastVertex(TopoDS::Edge(e1)));
                                        if ((p1.SquareDistance(pt1) <= tol2
                                             && p2.SquareDistance(pt2) <= tol2)
                                            || (p1.SquareDistance(pt2) <= tol2
                                                && p2.SquareDistance(pt1) <= tol2)) {
                                            found = true;
                                            break;
                                        }
                                    }
                                    continue;
                                }

                                if (g1->isSame(*geom2, tol, atol)) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                matched2 = false;
                                break;
                            }
                        }
                        if (!matched2) {
                            continue;
                        }
                    }
                    if (names) {
                        names->push_back(shapeName(shapeType) + std::to_string(idx));
                    }
                    res.push_back(shape);
                }
            }
            break;
        }
        default:
            break;
    }
    return res;
}

int TopoShape::findAncestor(const TopoDS_Shape& subshape, TopAbs_ShapeEnum type) const
{
    initCache();
    return _cache->findShape(_Shape, _cache->findAncestor(_Shape, subshape, type));
}

TopoDS_Shape TopoShape::findAncestorShape(const TopoDS_Shape& subshape, TopAbs_ShapeEnum type) const
{
    initCache();
    return _cache->findAncestor(_Shape, subshape, type);
}

std::vector<int> TopoShape::findAncestors(const TopoDS_Shape& subshape, TopAbs_ShapeEnum type) const
{
    const auto& shapes = findAncestorsShapes(subshape, type);
    std::vector<int> ret;
    ret.reserve(shapes.size());
    for (const auto& shape : shapes) {
        ret.push_back(findShape(shape));
    }
    return ret;
}

std::vector<TopoDS_Shape> TopoShape::findAncestorsShapes(const TopoDS_Shape& subshape,
                                                         TopAbs_ShapeEnum type) const
{
    initCache();
    std::vector<TopoDS_Shape> shapes;
    _cache->findAncestor(_Shape, subshape, type, &shapes);
    return shapes;
}

// The following lines should be used for now to replace the original macros (in the future we can
// refactor to use std::source_location and eliminate the use of the macros entirely).
//     FC_THROWM(NullShapeException, "Null shape");
//     FC_THROWM(NullShapeException, "Null input shape");
//     FC_WARN("Null input shape");  // NOLINT
//
// The original macros:
// #define HANDLE_NULL_SHAPE _HANDLE_NULL_SHAPE("Null shape",true)
// #define HANDLE_NULL_INPUT _HANDLE_NULL_SHAPE("Null input shape",true)
// #define WARN_NULL_INPUT _HANDLE_NULL_SHAPE("Null input shape",false)

bool TopoShape::hasPendingElementMap() const
{
    return !elementMap(false) && this->_cache
        && (this->_parentCache || this->_cache->cachedElementMap);
}

bool TopoShape::canMapElement(const TopoShape& other) const
{
    if (isNull() || other.isNull() || this == &other || other.Tag == -1 || Tag == -1) {
        return false;
    }
    if ((other.Tag == 0) && !other.elementMap(false) && !other.hasPendingElementMap()) {
        return false;
    }
    initCache();
    other.initCache();
    _cache->relations.clear();
    return true;
}

namespace
{
size_t checkSubshapeCount(const TopoShape& topoShape1,
                          const TopoShape& topoShape2,
                          TopAbs_ShapeEnum elementType)
{
    auto count = topoShape1.countSubShapes(elementType);
    auto other = topoShape2.countSubShapes(elementType);
    if (count != other) {
        FC_WARN("sub shape mismatch");  // NOLINT
        if (count > other) {
            count = other;
        }
    }
    return count;
}

}  // namespace

void TopoShape::setupChild(Data::ElementMap::MappedChildElements& child,
                           TopAbs_ShapeEnum elementType,
                           const TopoShape& topoShape,
                           size_t shapeCount,
                           const char* op)
{
    child.indexedName = Data::IndexedName::fromConst(TopoShape::shapeName(elementType).c_str(), 1);
    child.offset = 0;
    child.count = static_cast<int>(shapeCount);
    child.elementMap = topoShape.elementMap();
    if (this->Tag != topoShape.Tag) {
        child.tag = topoShape.Tag;
    }
    else {
        child.tag = 0;
    }
    if (op) {
        child.postfix = op;
    }
}

void TopoShape::copyElementMap(const TopoShape& topoShape, const char* op)
{
    if (topoShape.isNull() || isNull()) {
        return;
    }
    std::vector<Data::ElementMap::MappedChildElements> children;
    std::array<TopAbs_ShapeEnum, 3> elementTypes = {TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE};
    for (const auto elementType : elementTypes) {
        auto count = checkSubshapeCount(*this, topoShape, elementType);
        if (count == 0) {
            continue;
        }
        children.emplace_back();
        auto& child = children.back();
        setupChild(child, elementType, topoShape, count, op);
    }
    resetElementMap();
    if (!Hasher) {
        Hasher = topoShape.Hasher;
    }
    setMappedChildElements(children);
}


// TODO: Refactor mapSubElementTypeForShape to reduce complexity
void TopoShape::mapSubElementTypeForShape(const TopoShape& other,
                                          TopAbs_ShapeEnum type,
                                          const char* op,
                                          int count,
                                          bool forward,
                                          bool& warned)
{
    auto& shapeMap = _cache->getAncestry(type);
    auto& otherMap = other._cache->getAncestry(type);
    const char* shapeType = shapeName(type).c_str();

    // 1-indexed for readability (e.g. there is no "Edge0", we started at "Edge1", etc.)
    for (int outerCounter = 1; outerCounter <= count; ++outerCounter) {
        int innerCounter {0};
        int index {0};
        if (forward) {
            innerCounter = outerCounter;
            index = shapeMap.find(_Shape, otherMap.find(other._Shape, outerCounter));
            if (index == 0) {
                continue;
            }
        }
        else {
            index = outerCounter;
            innerCounter = otherMap.find(other._Shape, shapeMap.find(_Shape, outerCounter));
            if (innerCounter == 0) {
                continue;
            }
        }
        Data::IndexedName element = Data::IndexedName::fromConst(shapeType, index);
        for (auto& mappedName :
             other.getElementMappedNames(Data::IndexedName::fromConst(shapeType, innerCounter),
                                         true)) {
            auto& name = mappedName.first;
            auto& sids = mappedName.second;
            if (!sids.empty()) {
                if (!Hasher) {
                    Hasher = sids[0].getHasher();
                }
                else if (!sids[0].isFromSameHasher(Hasher)) {
                    if (!warned) {
                        warned = true;
                        FC_WARN("hasher mismatch");  // NOLINT
                    }
                    sids.clear();
                }
            }
            char elementType {shapeName(type)[0]};

            std::ostringstream ss;
            ensureElementMap()->encodeElementName(elementType, name, ss, &sids, Tag, op, other.Tag);
            elementMap()->setElementName(element, name, Tag, &sids);
        }
    }
}

void TopoShape::mapSubElementForShape(const TopoShape& other, const char* op)
{
    bool warned = false;
    static const std::array<TopAbs_ShapeEnum, 3> types = {TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE};

    for (auto type : types) {
        auto& shapeMap = _cache->getAncestry(type);
        auto& otherMap = other._cache->getAncestry(type);
        if ((shapeMap.count() == 0) || (otherMap.count() == 0)) {
            continue;
        }

        bool forward {false};
        int count {0};
        if (otherMap.count() <= shapeMap.count()) {
            forward = true;
            count = otherMap.count();
        }
        else {
            forward = false;
            count = shapeMap.count();
        }
        mapSubElementTypeForShape(other, type, op, count, forward, warned);
    }
}

void TopoShape::mapSubElement(const TopoShape& other, const char* op, bool forceHasher)
{
    if (!canMapElement(other)) {
        return;
    }

    if (!getElementMapSize(false) && this->_Shape.IsPartner(other._Shape)) {
        if (!this->Hasher) {
            this->Hasher = other.Hasher;
        }
        copyElementMap(other, op);
        return;
    }

    bool warned = false;
    static const std::array<TopAbs_ShapeEnum, 3> types = {TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE};

    auto checkHasher = [this](const TopoShape& other) {
        if (Hasher) {
            if (other.Hasher != Hasher) {
                if (!getElementMapSize(false)) {
                    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                        FC_WARN("hasher mismatch");
                    }
                }
                else {
                    // FC_THROWM(Base::RuntimeError, "hasher mismatch");
                    FC_ERR("hasher mismatch");
                }
                Hasher = other.Hasher;
            }
        }
        else {
            Hasher = other.Hasher;
        }
    };

    for (auto type : types) {
        auto& shapeMap = _cache->getAncestry(type);
        auto& otherMap = other._cache->getAncestry(type);
        if (!shapeMap.count() || !otherMap.count()) {
            continue;
        }
        if (!forceHasher && other.Hasher) {
            forceHasher = true;
            checkHasher(other);
        }
        const char* shapetype = shapeName(type).c_str();
        std::ostringstream ss;

        bool forward;
        int count;
        if (otherMap.count() <= shapeMap.count()) {
            forward = true;
            count = otherMap.count();
        }
        else {
            forward = false;
            count = shapeMap.count();
        }
        for (int k = 1; k <= count; ++k) {
            int i, idx;
            if (forward) {
                i = k;
                idx = shapeMap.find(_Shape, otherMap.find(other._Shape, k));
                if (!idx) {
                    continue;
                }
            }
            else {
                idx = k;
                i = otherMap.find(other._Shape, shapeMap.find(_Shape, k));
                if (!i) {
                    continue;
                }
            }
            Data::IndexedName element = Data::IndexedName::fromConst(shapetype, idx);
            for (auto& v :
                 other.getElementMappedNames(Data::IndexedName::fromConst(shapetype, i), true)) {
                auto& name = v.first;
                auto& sids = v.second;
                if (sids.size()) {
                    if (!Hasher) {
                        Hasher = sids[0].getHasher();
                    }
                    else if (!sids[0].isFromSameHasher(Hasher)) {
                        if (!warned) {
                            warned = true;
                            FC_WARN("hasher mismatch");
                        }
                        sids.clear();
                    }
                }
                ss.str("");

                ensureElementMap()->encodeElementName(shapetype[0], name, ss, &sids, Tag, op, other.Tag);
                elementMap()->setElementName(element, name, Tag, &sids);
            }
        }
    }
}

void TopoShape::mapSubElementsTo(std::vector<TopoShape>& shapes, const char* op) const
{
    for (auto& shape : shapes) {
        shape.mapSubElement(*this, op);
    }
}

std::vector<Data::ElementMap::MappedChildElements>
TopoShape::createChildMap(size_t count, const std::vector<TopoShape>& shapes, const char* op)
{
    std::vector<Data::ElementMap::MappedChildElements> children;
    children.reserve(count * (size_t)3);
    std::array<TopAbs_ShapeEnum, 3> types = {TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE};
    for (const auto topAbsType : types) {
        size_t offset = 0;
        for (auto& topoShape : shapes) {
            if (topoShape.isNull()) {
                continue;
            }
            auto subShapeCount = topoShape.countSubShapes(topAbsType);
            if (subShapeCount == 0) {
                continue;
            }
            children.emplace_back();
            auto& child = children.back();
            child.indexedName =
                Data::IndexedName::fromConst(TopoShape::shapeName(topAbsType).c_str(), 1);
            child.offset = static_cast<int>(offset);
            offset += subShapeCount;
            child.count = static_cast<int>(subShapeCount);
            child.elementMap = topoShape.elementMap();
            child.tag = topoShape.Tag;
            if (op) {
                child.postfix = op;
            }
        }
    }
    return children;
}

void TopoShape::mapCompoundSubElements(const std::vector<TopoShape>& shapes, const char* op)
{
    int count = 0;
    for (auto& topoShape : shapes) {
        if (topoShape.isNull()) {
            continue;
        }
        ++count;
        auto subshape = getSubShape(TopAbs_SHAPE, count, /*silent = */ true);
        if (!subshape.IsPartner(topoShape._Shape)) {
            return;  // Not a partner shape, don't do any mapping at all
        }
    }
    auto children {createChildMap(count, shapes, op)};
    setMappedChildElements(children);
}

void TopoShape::mapSubElement(const std::vector<TopoShape>& shapes, const char* op)
{
    if (shapes.empty()) {
        return;
    }

    if (shapeType(true) == TopAbs_COMPOUND) {
        int count = 0;
        for (auto& s : shapes) {
            if (s.isNull()) {
                continue;
            }
            if (!getSubShape(TopAbs_SHAPE, ++count, true).IsPartner(s._Shape)) {
                count = 0;
                break;
            }
        }
        if (count) {
            std::vector<Data::ElementMap::MappedChildElements> children;
            children.reserve(count * 3);
            TopAbs_ShapeEnum types[] = {TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE};
            for (unsigned i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
                int offset = 0;
                for (auto& s : shapes) {
                    if (s.isNull()) {
                        continue;
                    }
                    int count = s.countSubShapes(types[i]);
                    if (!count) {
                        continue;
                    }
                    children.emplace_back();
                    auto& child = children.back();
                    child.indexedName =
                        Data::IndexedName::fromConst(shapeName(types[i]).c_str(), 1);
                    child.offset = offset;
                    offset += count;
                    child.count = count;
                    child.elementMap = s.elementMap();
                    child.tag = s.Tag;
                    if (op) {
                        child.postfix = op;
                    }
                }
            }
            setMappedChildElements(children);
            return;
        }
    }

    for (auto& shape : shapes) {
        mapSubElement(shape, op);
    }
}

std::vector<TopoDS_Shape> TopoShape::getSubShapes(TopAbs_ShapeEnum type,
                                                  TopAbs_ShapeEnum avoid) const
{
    std::vector<TopoDS_Shape> ret;
    if (isNull()) {
        return ret;
    }
    if (avoid != TopAbs_SHAPE) {
        for (TopExp_Explorer exp(getShape(), type, avoid); exp.More(); exp.Next()) {
            ret.push_back(exp.Current());
        }
        return ret;
    }
    initCache();
    auto& ancestry = _cache->getAncestry(type);
    int count = ancestry.count();
    ret.reserve(count);
    for (int i = 1; i <= count; ++i) {
        ret.push_back(ancestry.find(_Shape, i));
    }
    return ret;
}

std::vector<TopoShape> TopoShape::getSubTopoShapes(TopAbs_ShapeEnum type,
                                                   TopAbs_ShapeEnum avoid) const
{
    if (isNull()) {
        return std::vector<TopoShape>();
    }
    initCache();

    auto res = _cache->getAncestry(type).getTopoShapes(*this);
    if (avoid != TopAbs_SHAPE && hasSubShape(avoid)) {
        for (auto it = res.begin(); it != res.end();) {
            if (_cache->findAncestor(_Shape, it->getShape(), avoid).IsNull()) {
                ++it;
            }
            else {
                it = res.erase(it);
            }
        }
    }
    return res;
}

std::vector<TopoShape> TopoShape::getOrderedEdges(MapElement mapElement) const
{
    if (isNull()) {
        return std::vector<TopoShape>();
    }

    std::vector<TopoShape> shapes;
    if (shapeType() == TopAbs_WIRE) {
        BRepTools_WireExplorer xp(TopoDS::Wire(getShape()));
        while (xp.More()) {
            shapes.push_back(TopoShape(xp.Current()));
            xp.Next();
        }
    }
    else {
        //        INIT_SHAPE_CACHE();
        initCache();
        for (const auto& w : getSubShapes(TopAbs_WIRE)) {
            BRepTools_WireExplorer xp(TopoDS::Wire(w));
            while (xp.More()) {
                shapes.push_back(TopoShape(xp.Current()));
                xp.Next();
            }
        }
    }
    if (mapElement == MapElement::map) {
        mapSubElementsTo(shapes);
    }
    return shapes;
}

std::vector<TopoShape> TopoShape::getOrderedVertexes(MapElement mapElement) const
{
    if (isNull()) {
        return std::vector<TopoShape>();
    }

    std::vector<TopoShape> shapes;

    auto collect = [&](const TopoDS_Shape& s) {
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

    if (shapeType() == TopAbs_WIRE) {
        collect(getShape());
    }
    else {
        //        INIT_SHAPE_CACHE();
        initCache();
        for (const auto& s : getSubShapes(TopAbs_WIRE)) {
            collect(s);
        }
    }
    if (mapElement == MapElement::map) {
        mapSubElementsTo(shapes);
    }
    return shapes;
}

struct ShapeInfo
{
    const TopoDS_Shape& shape;
    TopoShapeCache::Ancestry& cache;
    TopAbs_ShapeEnum type;
    const char* shapetype;

    ShapeInfo(const TopoDS_Shape& shape, TopAbs_ShapeEnum type, TopoShapeCache::Ancestry& cache)
        : shape(shape)
        , cache(cache)
        , type(type)
        , shapetype(TopoShape::shapeName(type).c_str())
    {}

    [[nodiscard]] int count() const
    {
        return cache.count();
    }

    TopoDS_Shape find(int index)
    {
        return cache.find(shape, index);
    }

    int find(const TopoDS_Shape& subshape)
    {
        return cache.find(shape, subshape);
    }
};

////////////////////////////////////////
// makESHAPE -> makeShapeWithElementMap
///////////////////////////////////////

struct NameKey
{
    Data::MappedName name;
    long tag = 0;
    int shapetype = 0;

    NameKey() = default;
    explicit NameKey(Data::MappedName n)
        : name(std::move(n))
    {}
    NameKey(int type, Data::MappedName n)
        : name(std::move(n))
    {
        // Order the shape type from vertex < edge < face < other.  We'll rely
        // on this for sorting when we name the geometry element.
        switch (type) {
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
    bool operator<(const NameKey& other) const
    {
        if (shapetype < other.shapetype) {
            return true;
        }
        if (shapetype > other.shapetype) {
            return false;
        }
        if (tag < other.tag) {
            return true;
        }
        if (tag > other.tag) {
            return false;
        }
        return name < other.name;
    }
};

struct NameInfo
{
    int index {};
    Data::ElementIDRefs sids;
    const char* shapetype {};
};


const std::string& modPostfix()
{
    static std::string postfix(Data::POSTFIX_MOD);
    return postfix;
}

const std::string& modgenPostfix()
{
    static std::string postfix(Data::POSTFIX_MODGEN);
    return postfix;
}

const std::string& genPostfix()
{
    static std::string postfix(Data::POSTFIX_GEN);
    return postfix;
}

const std::string& upperPostfix()
{
    static std::string postfix(Data::POSTFIX_UPPER);
    return postfix;
}

const std::string& lowerPostfix()
{
    static std::string postfix(Data::POSTFIX_LOWER);
    return postfix;
}

// TODO: Refactor checkForParallelOrCoplanar to reduce complexity
void checkForParallelOrCoplanar(const TopoDS_Shape& newShape,
                                const ShapeInfo& newInfo,
                                std::vector<TopoDS_Shape>& newShapes,
                                const gp_Pln& pln,
                                int& parallelFace,
                                int& coplanarFace,
                                int& checkParallel)
{
    for (TopExp_Explorer xp(newShape, newInfo.type); xp.More(); xp.Next()) {
        newShapes.push_back(xp.Current());

        if ((parallelFace < 0 || coplanarFace < 0) && checkParallel > 0) {
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
            if (TopoShape(newShapes.back()).findPlane(plnOther)) {
                if (pln.Axis().IsParallel(plnOther.Axis(), Precision::Angular())) {
                    if (coplanarFace < 0) {
                        gp_Vec vec(pln.Axis().Location(), plnOther.Axis().Location());
                        Standard_Real D1 = gp_Vec(pln.Axis().Direction()).Dot(vec);
                        if (D1 < 0) {
                            D1 = -D1;
                        }
                        Standard_Real D2 = gp_Vec(plnOther.Axis().Direction()).Dot(vec);
                        if (D2 < 0) {
                            D2 = -D2;
                        }
                        if (D1 <= Precision::Confusion() && D2 <= Precision::Confusion()) {
                            coplanarFace = (int)newShapes.size();
                            continue;
                        }
                    }
                    if (parallelFace < 0) {
                        parallelFace = (int)newShapes.size();
                    }
                }
            }
        }
    }
}

// TODO: Refactor makeShapeWithElementMap to reduce complexity
TopoShape& TopoShape::makeShapeWithElementMap(const TopoDS_Shape& shape,
                                              const Mapper& mapper,
                                              const std::vector<TopoShape>& shapes,
                                              const char* op)
{
    setShape(shape);
    if (shape.IsNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }

    if (shapes.empty()) {
        return *this;
    }

    size_t canMap = 0;
    for (auto& incomingShape : shapes) {
        if (canMapElement(incomingShape)) {
            ++canMap;
        }
    }
    if (canMap == 0U) {
        return *this;
    }
    if (canMap != shapes.size() && FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
        FC_WARN("Not all input shapes are mappable");  // NOLINT
    }

    if (!op) {
        op = Part::OpCodes::Maker;
    }
    std::string _op = op;
    _op += '_';

    initCache();
    ShapeInfo vertexInfo(_Shape, TopAbs_VERTEX, _cache->getAncestry(TopAbs_VERTEX));
    ShapeInfo edgeInfo(_Shape, TopAbs_EDGE, _cache->getAncestry(TopAbs_EDGE));
    ShapeInfo faceInfo(_Shape, TopAbs_FACE, _cache->getAncestry(TopAbs_FACE));
    mapSubElement(shapes);  // Intentionally leave the op off here

    std::array<ShapeInfo*, 3> infos = {&vertexInfo, &edgeInfo, &faceInfo};

    std::array<ShapeInfo*, TopAbs_SHAPE> infoMap {};
    infoMap[TopAbs_VERTEX] = &vertexInfo;
    infoMap[TopAbs_EDGE] = &edgeInfo;
    infoMap[TopAbs_WIRE] = &edgeInfo;
    infoMap[TopAbs_FACE] = &faceInfo;
    infoMap[TopAbs_SHELL] = &faceInfo;
    infoMap[TopAbs_SOLID] = &faceInfo;
    infoMap[TopAbs_COMPOUND] = &faceInfo;
    infoMap[TopAbs_COMPSOLID] = &faceInfo;

    std::ostringstream ss;
    std::string postfix;
    Data::MappedName newName;

    std::map<Data::IndexedName, std::map<NameKey, NameInfo>> newNames;

    // First, collect names from other shapes that generates or modifies the
    // new shape
    for (auto& pinfo : infos) {  // Walk Vertexes, then Edges, then Faces
        auto& info = *pinfo;
        for (const auto& incomingShape : shapes) {
            if (!canMapElement(incomingShape)) {
                continue;
            }
            auto& otherMap = incomingShape._cache->getAncestry(info.type);
            if (otherMap.count() == 0) {
                continue;
            }
            for (int i = 1; i <= otherMap.count(); i++) {
                const auto& otherElement = otherMap.find(incomingShape._Shape, i);
                // Find all new objects that are a modification of the old object
                Data::ElementIDRefs sids;
                NameKey key(
                    info.type,
                    incomingShape.getMappedName(Data::IndexedName::fromConst(info.shapetype, i),
                                                true,
                                                &sids));

                int newShapeCounter = 0;
                for (auto& newShape : mapper.modified(otherElement)) {
                    ++newShapeCounter;
                    if (newShape.ShapeType() >= TopAbs_SHAPE) {
                        // NOLINTNEXTLINE
                        FC_ERR("unknown modified shape type " << newShape.ShapeType() << " from "
                                                              << info.shapetype << i);
                        continue;
                    }
                    auto& newInfo = *infoMap.at(newShape.ShapeType());
                    if (newInfo.type != newShape.ShapeType()) {
                        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                            // TODO: it seems modified shape may report higher
                            // level shape type just like generated shape below.
                            // Maybe we shall do the same for name construction.
                            // NOLINTNEXTLINE
                            FC_WARN("modified shape type " << shapeName(newShape.ShapeType())
                                                           << " mismatch with " << info.shapetype
                                                           << i);
                        }
                        continue;
                    }
                    int newShapeIndex = newInfo.find(newShape);
                    if (newShapeIndex == 0) {
                        // This warning occurs in makeElementRevolve. It generates
                        // some shape from a vertex that never made into the
                        // final shape. There may be incomingShape cases there.
                        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                            // NOLINTNEXTLINE
                            FC_WARN("Cannot find " << op << " modified " << newInfo.shapetype
                                                   << " from " << info.shapetype << i);
                        }
                        continue;
                    }

                    Data::IndexedName element =
                        Data::IndexedName::fromConst(newInfo.shapetype, newShapeIndex);
                    if (getMappedName(element)) {
                        continue;
                    }

                    key.tag = incomingShape.Tag;
                    auto& name_info = newNames[element][key];
                    name_info.sids = sids;
                    name_info.index = newShapeCounter;
                    name_info.shapetype = info.shapetype;
                }

                int checkParallel = -1;
                gp_Pln pln;

                // Find all new objects that were generated from an old object
                // (e.g. a face generated from an edge)
                newShapeCounter = 0;
                for (auto& newShape : mapper.generated(otherElement)) {
                    if (newShape.ShapeType() >= TopAbs_SHAPE) {
                        // NOLINTNEXTLINE
                        FC_ERR("unknown generated shape type " << newShape.ShapeType() << " from "
                                                               << info.shapetype << i);
                        continue;
                    }

                    int parallelFace = -1;
                    int coplanarFace = -1;
                    auto& newInfo = *infoMap.at(newShape.ShapeType());
                    std::vector<TopoDS_Shape> newShapes;
                    int shapeOffset = 0;
                    if (newInfo.type == newShape.ShapeType()) {
                        newShapes.push_back(newShape);
                    }
                    else {
                        // It is possible for the maker to report generating a
                        // higher level shape, such as shell or solid. For
                        // example, when extruding, OCC will report the
                        // extruding face generating the entire solid. However,
                        // it will also report the edges of the extruding face
                        // generating the side faces. In this case, too much
                        // information is bad for us. We don't want the name of
                        // the side face (and its edges) to be coupled with
                        // incomingShape (unrelated) edges in the extruding face.
                        //
                        // shapeOffset below is used to make sure the higher
                        // level mapped names comes late after sorting. We'll
                        // ignore those names if there are more precise mapping
                        // available.
                        shapeOffset = 3;

                        if (info.type == TopAbs_FACE && checkParallel < 0) {
                            if (!TopoShape(otherElement).findPlane(pln)) {
                                checkParallel = 0;
                            }
                            else {
                                checkParallel = 1;
                            }
                        }
                        checkForParallelOrCoplanar(newShape,
                                                   newInfo,
                                                   newShapes,
                                                   pln,
                                                   parallelFace,
                                                   coplanarFace,
                                                   checkParallel);
                    }
                    key.shapetype += shapeOffset;
                    for (auto& workingShape : newShapes) {
                        ++newShapeCounter;
                        int workingShapeIndex = newInfo.find(workingShape);
                        if (workingShapeIndex == 0) {
                            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                                // NOLINTNEXTLINE
                                FC_WARN("Cannot find " << op << " generated " << newInfo.shapetype
                                                       << " from " << info.shapetype << i);
                            }
                            continue;
                        }

                        Data::IndexedName element =
                            Data::IndexedName::fromConst(newInfo.shapetype, workingShapeIndex);
                        auto mapped = getMappedName(element);
                        if (mapped) {
                            continue;
                        }

                        key.tag = incomingShape.Tag;
                        auto& name_info = newNames[element][key];
                        name_info.sids = sids;
                        if (newShapeCounter == parallelFace) {
                            name_info.index = std::numeric_limits<int>::min();
                        }
                        else if (newShapeCounter == coplanarFace) {
                            name_info.index = std::numeric_limits<int>::min() + 1;
                        }
                        else {
                            name_info.index = -newShapeCounter;
                        }
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

    while (true) {

        // Construct the names for modification/generation info collected in
        // the previous step
        for (auto itName = newNames.begin(), itNext = itName; itNext != newNames.end();
             itName = itNext) {
            // We treat the first modified/generated source shape name specially.
            // If case there are more than one source shape. We hash the first
            // source name separately, and then obtain the second string id by
            // hashing all the source names together.  We then use the second
            // string id as the postfix for our name.
            //
            // In this way, we can associate the same source that are modified by
            // multiple other shapes.

            ++itNext;

            auto& element = itName->first;
            auto& names = itName->second;
            const auto& first_key = names.begin()->first;
            auto& first_info = names.begin()->second;

            if (!delayed && first_key.shapetype >= 3 && first_info.index > INT_MIN + 1) {
                // This name is mapped from high level (shell, solid, etc.)
                // Delay till next round.
                //
                // index>INT_MAX+1 is for checking generated coplanar and
                // parallel face mapping, which has special fixed index to make
                // name stable.  These names are not delayed.
                continue;
            }
            if (!delayed && getMappedName(element)) {
                newNames.erase(itName);
                continue;
            }

            int name_type =
                first_info.index > 0 ? 1 : 2;  // index>0 means modified, or else generated
            Data::MappedName first_name = first_key.name;

            Data::ElementIDRefs sids(first_info.sids);

            postfix.clear();
            if (names.size() > 1) {
                ss.str("");
                ss << '(';
                bool first = true;
                auto it = names.begin();
                int count = 0;
                for (++it; it != names.end(); ++it) {
                    auto& other_key = it->first;
                    if (other_key.shapetype >= 3 && first_key.shapetype < 3) {
                        // shapetype>=3 means it's a high level mapping (e.g. a face
                        // generates a solid). We don't want that if there are more
                        // precise low level mapping available. See comments above
                        // for more details.
                        break;
                    }
                    if (first) {
                        first = false;
                    }
                    else {
                        ss << '|';
                    }
                    auto& other_info = it->second;
                    std::ostringstream ss2;
                    if (other_info.index != 1) {
                        // 'K' marks the additional source shape of this
                        // generate (or modified) shape.
                        ss2 << elementMapPrefix() << 'K';
                        if (other_info.index == INT_MIN) {
                            ss2 << '0';
                        }
                        else if (other_info.index == INT_MIN + 1) {
                            ss2 << "00";
                        }
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

                    ensureElementMap()->encodeElementName(*other_info.shapetype,
                                                    other_name,
                                                    ss2,
                                                    &sids,
                                                    Tag,
                                                    nullptr,
                                                    other_key.tag);
                    ss << other_name;
                    if ((name_type == 1 && other_info.index < 0)
                        || (name_type == 2 && other_info.index > 0)) {
                        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                            FC_WARN("element is both generated and modified");  // NOLINT
                        }
                        name_type = 0;
                    }
                    sids += other_info.sids;
                    // To avoid the name becoming to long, just put some limit here
                    if (++count == 4) {
                        break;
                    }
                }
                if (!first) {
                    ss << ')';
                    if (Hasher) {
                        sids.push_back(Hasher->getID(ss.str().c_str()));
                        ss.str("");
                        ss << sids.back().toString();
                    }
                    postfix = ss.str();
                }
            }

            ss.str("");
            if (name_type == 2) {
                ss << genPostfix();
            }
            else if (name_type == 1) {
                ss << modPostfix();
            }
            else {
                ss << modgenPostfix();
            }
            if (first_info.index == INT_MIN) {
                ss << '0';
            }
            else if (first_info.index == INT_MIN + 1) {
                ss << "00";
            }
            else if (abs(first_info.index) > 1) {
                ss << abs(first_info.index);
            }
            ss << postfix;

            ensureElementMap()
                ->encodeElementName(element[0], first_name, ss, &sids, Tag, op, first_key.tag);
            elementMap()->setElementName(element, first_name, Tag, &sids);
            if (!delayed && first_key.shapetype < 3) {
                newNames.erase(itName);
            }
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

        for (size_t infoIndex = infos.size() - 1; infoIndex != 0; --infoIndex) {
            std::map<Data::IndexedName,
                     std::map<Data::MappedName, NameInfo, Data::ElementNameComparator>>
                names;
            auto& info = *infos.at(infoIndex);
            auto& next = *infos.at(infoIndex - 1);
            int elementCounter = 1;
            auto it = newNames.end();
            if (delayed) {
                it = newNames.upper_bound(Data::IndexedName::fromConst(info.shapetype, 0));
            }
            for (;; ++elementCounter) {
                Data::IndexedName element;
                if (!delayed) {
                    if (elementCounter > info.count()) {
                        break;
                    }
                    element = Data::IndexedName::fromConst(info.shapetype, elementCounter);
                    if (newNames.count(element) != 0U) {
                        continue;
                    }
                }
                else if (it == newNames.end()
                         || !boost::starts_with(it->first.getType(), info.shapetype)) {
                    break;
                }
                else {
                    element = it->first;
                    ++it;
                    elementCounter = element.getIndex();
                    if (elementCounter == 0 || elementCounter > info.count()) {
                        continue;
                    }
                }
                Data::ElementIDRefs sids;
                Data::MappedName mapped = getMappedName(element, false, &sids);
                if (!mapped) {
                    continue;
                }

                TopTools_IndexedMapOfShape submap;
                TopExp::MapShapes(info.find(elementCounter), next.type, submap);
                for (int submapIndex = 1, infoCounter = 1; submapIndex <= submap.Extent();
                     ++submapIndex) {
                    ss.str("");
                    int elementIndex = next.find(submap(submapIndex));
                    assert(elementIndex);
                    Data::IndexedName indexedName =
                        Data::IndexedName::fromConst(next.shapetype, elementIndex);
                    if (getMappedName(indexedName)) {
                        continue;
                    }
                    auto& infoRef = names[indexedName][mapped];
                    infoRef.index = infoCounter++;
                    infoRef.sids = sids;
                }
            }
            // Assign the actual names
            for (auto& [indexedName, nameInfoMap] : names) {
                // Do we really want multiple names for an element in this case?
                // If not, we just pick the name in the first sorting order here.
                auto& nameInfoMapEntry = *nameInfoMap.begin();
                {
                    auto& nameInfo = nameInfoMapEntry.second;
                    auto& sids = nameInfo.sids;
                    newName = nameInfoMapEntry.first;
                    ss.str("");
                    ss << upperPostfix();
                    if (nameInfo.index > 1) {
                        ss << nameInfo.index;
                    }

                    ensureElementMap()->encodeElementName(indexedName[0], newName, ss, &sids, Tag, op);
                    elementMap()->setElementName(indexedName, newName, Tag, &sids);
               }
            }
        }

        // The forward pass. For any elements that are not named, try construct its
        // name from the lower elements
        bool hasUnnamed = false;
        for (size_t ifo = 1; ifo < infos.size(); ++ifo) {
            auto& info = *infos.at(ifo);
            auto& prev = *infos.at(ifo - 1);
            for (int i = 1; i <= info.count(); ++i) {
                Data::IndexedName element = Data::IndexedName::fromConst(info.shapetype, i);
                if (getMappedName(element)) {
                    continue;
                }

                Data::ElementIDRefs sids;
                std::map<Data::MappedName, Data::IndexedName, Data::ElementNameComparator> names;
                TopExp_Explorer xp;
                if (info.type == TopAbs_FACE) {
                    xp.Init(BRepTools::OuterWire(TopoDS::Face(info.find(i))), TopAbs_EDGE);
                }
                else {
                    xp.Init(info.find(i), prev.type);
                }
                for (; xp.More(); xp.Next()) {
                    int previousElementIndex = prev.find(xp.Current());
                    assert(previousElementIndex);
                    Data::IndexedName prevElement =
                        Data::IndexedName::fromConst(prev.shapetype, previousElementIndex);
                    if (!delayed && (newNames.count(prevElement) != 0U)) {
                        names.clear();
                        break;
                    }
                    Data::ElementIDRefs sid;
                    Data::MappedName name = getMappedName(prevElement, false, &sid);
                    if (!name) {
                        // only assign name if all lower elements are named
                        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                            FC_WARN("unnamed lower element " << prevElement);  // NOLINT
                        }
                        names.clear();
                        break;
                    }
                    auto res = names.emplace(name, prevElement);
                    if (res.second) {
                        sids += sid;
                    }
                    else if (prevElement != res.first->second) {
                        // The seam edge will appear twice, which is normal. We
                        // only warn if the mapped element names are different.
                        // NOLINTNEXTLINE
                        FC_WARN("lower element " << prevElement << " and " << res.first->second
                                                 << " has duplicated name " << name << " for "
                                                 << info.shapetype << i);
                    }
                }
                if (names.empty()) {
                    hasUnnamed = true;
                    continue;
                }
                auto it = names.begin();
                newName = it->first;
                if (names.size() == 1) {
                    ss << lowerPostfix();
                }
                else {
                    bool first = true;
                    ss.str("");
                    if (!Hasher) {
                        ss << lowerPostfix();
                    }
                    ss << '(';
                    int count = 0;
                    for (++it; it != names.end(); ++it) {
                        if (first) {
                            first = false;
                        }
                        else {
                            ss << '|';
                        }
                        ss << it->first;

                        // To avoid the name becoming to long, just put some limit here
                        if (++count == 4) {
                            break;
                        }
                    }
                    ss << ')';
                    if (Hasher) {
                        sids.push_back(Hasher->getID(ss.str().c_str()));
                        ss.str("");
                        ss << lowerPostfix() << sids.back().toString();
                    }
                }

                ensureElementMap()->encodeElementName(element[0], newName, ss, &sids, Tag, op);
                elementMap()->setElementName(element, newName, Tag, &sids);
            }
        }
        if (!hasUnnamed || delayed || newNames.empty()) {
            break;
        }
        delayed = true;
    }
    return *this;
}

namespace
{
void addShapesToBuilder(const std::vector<TopoShape>& shapes,
                        BRep_Builder& builder,
                        TopoDS_Compound& comp)
{
    int count = 0;
    for (auto& topoShape : shapes) {
        if (topoShape.isNull()) {
            FC_WARN("Null input shape");  // NOLINT
            continue;
        }
        builder.Add(comp, topoShape.getShape());
        ++count;
    }
    if (count == 0) {
        FC_THROWM(NullShapeException, "Null shape");
    }
}
}  // namespace

// TODO: Can this be consolidated with getSubShape()?  Empty Parm Logic is a little different.
TopoShape TopoShape::getSubTopoShape(const char* Type, bool silent) const
{
    if (!Type || !Type[0]) {
        switch (shapeType(true)) {
            case TopAbs_COMPOUND:
            case TopAbs_COMPSOLID:
                if (countSubShapes(TopAbs_SOLID) == 1) {
                    return getSubTopoShape(TopAbs_SOLID, 1);
                }
                if (countSubShapes(TopAbs_SHELL) == 1) {
                    return getSubTopoShape(TopAbs_SHELL, 1);
                }
                if (countSubShapes(TopAbs_FACE) == 1) {
                    return getSubTopoShape(TopAbs_FACE, 1);
                }
                if (countSubShapes(TopAbs_WIRE) == 1) {
                    return getSubTopoShape(TopAbs_WIRE, 1);
                }
                if (countSubShapes(TopAbs_EDGE) == 1) {
                    return getSubTopoShape(TopAbs_EDGE, 1);
                }
                if (countSubShapes(TopAbs_VERTEX) == 1) {
                    return getSubTopoShape(TopAbs_VERTEX, 1);
                }
                break;
            default:
                break;
        }
        return *this;
    }

    Data::MappedElement mapped = getElementName(Type);
    if (!mapped.index && boost::starts_with(Type, elementMapPrefix())) {
        if (!silent) {
            FC_THROWM(Base::CADKernelError, "Mapped element not found: " << Type);
        }
        return TopoShape();
    }
    auto res = shapeTypeAndIndex(mapped.index);
    if (res.second <= 0) {
        if (!silent) {
            FC_THROWM(Base::ValueError, "Invalid shape name " << (Type ? Type : ""));
        }
        return TopoShape();
    }
    return getSubTopoShape(res.first, res.second, silent);
}

// TODO: Can this be consolidated with getSubShape()?  We use ancestry; other uses current shape.
TopoShape TopoShape::getSubTopoShape(TopAbs_ShapeEnum type, int idx, bool silent) const
{
    if (isNull()) {
        if (!silent) {
            FC_THROWM(NullShapeException, "null shape");
        }
        return TopoShape();
    }
    if (idx <= 0) {
        if (!silent) {
            FC_THROWM(Base::ValueError, "Invalid shape index " << idx);
        }
        return TopoShape();
    }
    if (type < 0 || type > TopAbs_SHAPE) {
        if (!silent) {
            FC_THROWM(Base::ValueError, "Invalid shape type " << type);
        }
        return TopoShape();
    }
    initCache();
    auto& shapeMap = _cache->getAncestry(type);
    if (idx > shapeMap.count()) {
        if (!silent) {
            FC_THROWM(Base::IndexError,
                      "Shape index " << idx << " out of bound " << shapeMap.count());
        }
        return TopoShape();
    }

    return shapeMap.getTopoShape(*this, idx);
}

TopoShape& TopoShape::makeElementEvolve(const TopoShape& spine,
                                        const TopoShape& profile,
                                        JoinType join,
                                        CoordinateSystem axeProf,
                                        MakeSolid solid,
                                        Spine profOnSpine,
                                        double tol,
                                        const char* op)
{
    if (!op) {
        op = Part::OpCodes::Evolve;
    }
    if (tol == 0.0) {
        tol = 1e-6;
    }

    GeomAbs_JoinType joinType;
    switch (join) {
        case JoinType::tangent:
            joinType = GeomAbs_Tangent;
            break;
        case JoinType::intersection:
            joinType = GeomAbs_Intersection;
            break;
        default:
            joinType = GeomAbs_Arc;
            break;
    }

    TopoDS_Shape spineShape;
    if (spine.countSubShapes(TopAbs_FACE) > 0) {
        spineShape = spine.getSubShape(TopAbs_FACE, 1);
    }
    else if (spine.countSubShapes(TopAbs_WIRE) > 0) {
        spineShape = spine.getSubShape(TopAbs_WIRE, 1);
    }
    else if (spine.countSubShapes(TopAbs_EDGE) > 0) {
        spineShape =
            BRepBuilderAPI_MakeWire(TopoDS::Edge(spine.getSubShape(TopAbs_EDGE, 1))).Wire();
    }
    if (spineShape.IsNull() || !BRepBuilderAPI_FindPlane(spineShape).Found()) {
        FC_THROWM(Base::CADKernelError, "Expect the spine to be a planar wire or face");
    }

    TopoDS_Shape profileShape;
    if (profile.countSubShapes(TopAbs_FACE) > 0 || profile.countSubShapes(TopAbs_WIRE) > 0) {
        profileShape = profile.getSubShape(TopAbs_WIRE, 1);
    }
    else if (profile.countSubShapes(TopAbs_EDGE) > 0) {
        profileShape =
            BRepBuilderAPI_MakeWire(TopoDS::Edge(profile.getSubShape(TopAbs_EDGE, 1))).Wire();
    }
    if (profileShape.IsNull() || !BRepBuilderAPI_FindPlane(profileShape).Found()) {
        if (profileShape.IsNull() || profile.countSubShapes(TopAbs_EDGE) > 1
            || !profile.getSubTopoShape(TopAbs_EDGE, 1).isLinearEdge()) {
            FC_THROWM(Base::CADKernelError,
                      "Expect the the profile to be a planar wire or a face or a line");
        }
    }
    if (spineShape.ShapeType() == TopAbs_FACE) {
        BRepOffsetAPI_MakeEvolved maker(
            TopoDS::Face(spineShape),
            TopoDS::Wire(profileShape),
            joinType,
            axeProf == CoordinateSystem::global ? Standard_True : Standard_False,
            solid == MakeSolid::makeSolid ? Standard_True : Standard_False,
            profOnSpine == Spine::on ? Standard_True : Standard_False,
            tol);
        return makeElementShape(maker, {spine, profile}, op);
    }
    else {
        BRepOffsetAPI_MakeEvolved maker(
            TopoDS::Wire(spineShape),
            TopoDS::Wire(profileShape),
            joinType,
            axeProf == CoordinateSystem::global ? Standard_True : Standard_False,
            solid == MakeSolid::makeSolid ? Standard_True : Standard_False,
            profOnSpine == Spine::on ? Standard_True : Standard_False,
            tol);
        return makeElementShape(maker, {spine, profile}, op);
    }
}

TopoShape& TopoShape::makeElementRuledSurface(const std::vector<TopoShape>& shapes,
                                              int orientation,
                                              const char* op)
{
    if (!op) {
        op = Part::OpCodes::RuledSurface;
    }

    if (shapes.size() != 2) {
        FC_THROWM(Base::CADKernelError, "Wrong number of input shapes");
    }

    std::vector<TopoShape> curves(2);
    int i = 0;
    for (auto& s : shapes) {
        if (s.isNull()) {
            FC_THROWM(NullShapeException, "Null input shape");
        }
        auto type = s.shapeType();
        if (type == TopAbs_WIRE || type == TopAbs_EDGE) {
            curves[i++] = s;
            continue;
        }
        auto countOfWires = s.countSubShapes(TopAbs_WIRE);
        if (countOfWires > 1) {
            curves[i++] = s.getSubTopoShape(TopAbs_WIRE, 1);
            continue;
        }
        auto countOfEdges = s.countSubShapes(TopAbs_EDGE);
        if (countOfEdges == 0) {
            FC_THROWM(Base::CADKernelError, "Input shape has no edge");
        }
        if (countOfEdges == 1) {
            curves[i++] = s.getSubTopoShape(TopAbs_EDGE, 1);
            continue;
        }
        curves[i] = s.makeElementWires();
        if (curves[i].isNull()) {
            FC_THROWM(NullShapeException, "Null input shape");
        }
        if (curves[i].shapeType() != TopAbs_WIRE) {
            FC_THROWM(Base::CADKernelError, "Input shape forms more than one wire");
        }
        ++i;
    }

    if (curves[0].shapeType() != curves[1].shapeType()) {
        for (auto& curve : curves) {
            if (curve.shapeType() == TopAbs_EDGE) {
                curve = curve.makeElementWires();
            }
        }
    }

    auto& S1 = curves[0];
    auto& S2 = curves[1];
    bool isWire = S1.shapeType() == TopAbs_WIRE;

    // https://forum.freecadweb.org/viewtopic.php?f=8&t=24052
    //
    // if both shapes are sub-elements of one common shape then the fill
    // algorithm leads to problems if the shape has set a placement. The
    // workaround is to copy the sub-shape
    S1 = S1.makeElementCopy();
    S2 = S2.makeElementCopy();

    if (orientation == 0) {
        // Automatic
        Handle(Adaptor3d_HCurve) a1;
        Handle(Adaptor3d_HCurve) a2;
        if (!isWire) {
            BRepAdaptor_HCurve adapt1(TopoDS::Edge(S1.getShape()));
            BRepAdaptor_HCurve adapt2(TopoDS::Edge(S2.getShape()));
            a1 = new BRepAdaptor_HCurve(adapt1);
            a2 = new BRepAdaptor_HCurve(adapt2);
        }
        else {
            BRepAdaptor_HCompCurve adapt1(TopoDS::Wire(S1.getShape()));
            BRepAdaptor_HCompCurve adapt2(TopoDS::Wire(S2.getShape()));
            a1 = new BRepAdaptor_HCompCurve(adapt1);
            a2 = new BRepAdaptor_HCompCurve(adapt2);
        }

        if (!a1.IsNull() && !a2.IsNull()) {
            // get end points of 1st curve
            gp_Pnt p1 = a1->Value(0.9 * a1->FirstParameter() + 0.1 * a1->LastParameter());
            gp_Pnt p2 = a1->Value(0.1 * a1->FirstParameter() + 0.9 * a1->LastParameter());
            if (S1.getShape().Orientation() == TopAbs_REVERSED) {
                std::swap(p1, p2);
            }

            // get end points of 2nd curve
            gp_Pnt p3 = a2->Value(0.9 * a2->FirstParameter() + 0.1 * a2->LastParameter());
            gp_Pnt p4 = a2->Value(0.1 * a2->FirstParameter() + 0.9 * a2->LastParameter());
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
    for (const auto& c : curves) {
        for (const auto& e : c.getSubTopoShapes(TopAbs_EDGE)) {
            auto found = res.findSubShapesWithSharedVertex(e);
            if (found.size() > 0) {
                found.front().resetElementMap(e.elementMap());
                edges.push_back(found.front());
            }
        }
    }
    // Use empty mapper and let makeShapeWithElementMap name the created surface with lower
    // elements.
    return makeShapeWithElementMap(res.getShape(), Mapper(), edges, op);
}

TopoShape& TopoShape::makeElementCompound(const std::vector<TopoShape>& shapes,
                                          const char* op,
                                          SingleShapeCompoundCreationPolicy policy)
{
    if (policy == SingleShapeCompoundCreationPolicy::returnShape && shapes.size() == 1) {
        *this = shapes[0];
        return *this;
    }

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);

    if (shapes.empty()) {
        setShape(comp);
        return *this;
    }
    addShapesToBuilder(shapes, builder, comp);
    setShape(comp);
    initCache();

    mapSubElement(shapes, op);
    return *this;
}

static std::vector<TopoShape> prepareProfiles(const std::vector<TopoShape>& shapes,
                                              size_t offset = 0)
{
    std::vector<TopoShape> ret;
    for (size_t i = offset; i < shapes.size(); ++i) {
        auto shape = shapes[i];
        if (shape.isNull()) {
            FC_THROWM(NullShapeException, "Null input shape");
        }
        if (shape.countSubShapes(TopAbs_FACE) == 1) {
            shape = shape.getSubTopoShape(TopAbs_FACE, 1).splitWires();
        }
        else if (shape.countSubShapes(TopAbs_WIRE) == 0 && shape.countSubShapes(TopAbs_EDGE) > 0) {
            shape = shape.makeElementWires();
        }

        if (shape.countSubShapes(TopAbs_WIRE) == 1) {
            ret.push_back(shape.getSubTopoShape(TopAbs_WIRE, 1));
            continue;
        }
        else if (shape.countSubShapes(TopAbs_VERTEX) == 1) {
            ret.push_back(shape.getSubTopoShape(TopAbs_VERTEX, 1));
            continue;
        }

        FC_THROWM(Base::CADKernelError,
                  "Profile shape is not a single vertex, edge, wire nor face.");
    }
    if (ret.empty()) {
        FC_THROWM(Base::CADKernelError, "No profile");
    }
    return ret;
}

TopoShape& TopoShape::makeElementPipeShell(const std::vector<TopoShape>& shapes,
                                           const MakeSolid make_solid,
                                           const Standard_Boolean isFrenet,
                                           TransitionMode transition,
                                           const char* op,
                                           double tol3d,
                                           double tolBound,
                                           double tolAngular)
{
    if (!op) {
        op = Part::OpCodes::PipeShell;
    }

    if (shapes.size() < 2) {
        FC_THROWM(Base::CADKernelError, "Not enough input shapes");
    }

    auto spine = shapes.front().makeElementWires();
    if (spine.isNull()) {
        FC_THROWM(NullShapeException, "Null input shape");
    }
    if (spine.getShape().ShapeType() != TopAbs_WIRE) {
        FC_THROWM(Base::CADKernelError, "Spine shape cannot form a single wire");
    }

    BRepOffsetAPI_MakePipeShell mkPipeShell(TopoDS::Wire(spine.getShape()));
    BRepBuilderAPI_TransitionMode transMode;
    switch (transition) {
        case TransitionMode::RightCorner:
            transMode = BRepBuilderAPI_RightCorner;
            break;
        case TransitionMode::RoundCorner:
            transMode = BRepBuilderAPI_RoundCorner;
            break;
        default:
            transMode = BRepBuilderAPI_Transformed;
            break;
    }
    mkPipeShell.SetMode(isFrenet);
    mkPipeShell.SetTransitionMode(transMode);
    if (tol3d != 0.0 || tolBound != 0.0 || tolAngular != 0.0) {
        if (tol3d == 0.0) {
            tol3d = 1e-4;
        }
        if (tolBound == 0.0) {
            tolBound = 1e-4;
        }
        if (tolAngular == 0.0) {
            tolAngular = 1e-2;
        }
        mkPipeShell.SetTolerance(tol3d, tolBound, tolAngular);
    }

    for (auto& sh : prepareProfiles(shapes, 1)) {
        mkPipeShell.Add(sh.getShape());
    }

    if (!mkPipeShell.IsReady()) {
        FC_THROWM(Base::CADKernelError, "shape is not ready to build");
    }
    else {
        mkPipeShell.Build();
    }

    if (make_solid == MakeSolid::makeSolid) {
        mkPipeShell.MakeSolid();
    }

    return makeElementShape(mkPipeShell, shapes, op);
}

TopoShape& TopoShape::makeElementOffset(const TopoShape& shape,
                                        double offset,
                                        double tol,
                                        bool intersection,
                                        bool selfInter,
                                        short offsetMode,
                                        JoinType join,
                                        FillType fill,
                                        const char* op)
{
    if (!op) {
        op = Part::OpCodes::Offset;
    }

    BRepOffsetAPI_MakeOffsetShape mkOffset;
    mkOffset.PerformByJoin(shape.getShape(),
                           offset,
                           tol,
                           BRepOffset_Mode(offsetMode),
                           intersection ? Standard_True : Standard_False,
                           selfInter ? Standard_True : Standard_False,
                           GeomAbs_JoinType(join));

    if (!mkOffset.IsDone()) {
        FC_THROWM(Base::CADKernelError, "BRepOffsetAPI_MakeOffsetShape not done");
    }

    TopoShape res(Tag, Hasher);
    res.makeElementShape(mkOffset, shape, op);
    if (shape.hasSubShape(TopAbs_SOLID) && !res.hasSubShape(TopAbs_SOLID)) {
        try {
            res = res.makeElementSolid();
        }
        catch (Standard_Failure& e) {
            FC_WARN("failed to make solid: " << e.GetMessageString());
        }
    }
    if (fill == FillType::noFill) {
        *this = res;
        return *this;
    }

    // get perimeter wire of original shape.
    // Wires returned seem to have edges in connection order.
    ShapeAnalysis_FreeBoundsProperties freeCheck(shape.getShape());
    freeCheck.Perform();
    if (freeCheck.NbClosedFreeBounds() < 1) {
        FC_THROWM(Base::CADKernelError, "no closed bounds");
    }

    BRep_Builder builder;
    std::vector<TopoShape> shapes;
    for (int index = 1; index <= freeCheck.NbClosedFreeBounds(); ++index) {
        TopoShape originalWire(shape.Tag,
                               shape.Hasher,
                               freeCheck.ClosedFreeBound(index)->FreeBound());
        originalWire.mapSubElement(shape);
        const BRepAlgo_Image& img = mkOffset.MakeOffset().OffsetEdgesFromShapes();

        // build offset wire.
        TopoDS_Wire offsetWire;
        builder.MakeWire(offsetWire);
        for (const auto& s : originalWire.getSubShapes(TopAbs_EDGE)) {
            if (!img.HasImage(s)) {
                FC_THROWM(Base::CADKernelError, "no image for shape");
            }
            const TopTools_ListOfShape& currentImage = img.Image(s);
            TopTools_ListIteratorOfListOfShape listIt;
            int edgeCount(0);
            TopoDS_Edge mappedEdge;
            for (listIt.Initialize(currentImage); listIt.More(); listIt.Next()) {
                if (listIt.Value().ShapeType() != TopAbs_EDGE) {
                    continue;
                }
                edgeCount++;
                mappedEdge = TopoDS::Edge(listIt.Value());
            }

            if (edgeCount != 1) {
                std::ostringstream stream;
                stream << "wrong edge count: " << edgeCount << std::endl;
                FC_THROWM(Base::CADKernelError, stream.str().c_str());
            }
            builder.Add(offsetWire, mappedEdge);
        }
        std::vector<TopoShape> wires;
        wires.push_back(originalWire);
        wires.push_back(TopoShape(Tag, Hasher, offsetWire));
        wires.back().mapSubElement(res);

        // It would be nice if we could get thruSections to build planar faces
        // in all areas possible, so we could run through refine. I tried setting
        // ruled to standard_true, but that didn't have the desired affect.
        BRepOffsetAPI_ThruSections aGenerator;
        aGenerator.AddWire(TopoDS::Wire(originalWire.getShape()));
        aGenerator.AddWire(offsetWire);
        aGenerator.Build();
        if (!aGenerator.IsDone()) {
            FC_THROWM(Base::CADKernelError, "ThruSections failed");
        }

        shapes.push_back(TopoShape(Tag, Hasher).makeElementShape(aGenerator, wires));
    }

    TopoShape perimeterCompound(Tag, Hasher);
    perimeterCompound.makeElementCompound(shapes, op);

    // still had to sew. not using the passed in parameter for sew.
    // Sew has it's own default tolerance. Opinions?
    BRepBuilderAPI_Sewing sewTool;
    sewTool.Add(shape.getShape());
    sewTool.Add(perimeterCompound.getShape());
    sewTool.Add(res.getShape());
    sewTool.Perform();  // Perform Sewing

    TopoDS_Shape outputShape = sewTool.SewedShape();
    if ((outputShape.ShapeType() == TopAbs_SHELL) && (outputShape.Closed())) {
        BRepBuilderAPI_MakeSolid solidMaker(TopoDS::Shell(outputShape));
        if (solidMaker.IsDone()) {
            TopoDS_Solid temp = solidMaker.Solid();
            // contrary to the occ docs the return value OrientCloseSolid doesn't
            // indicate whether the shell was open or not. It returns true with an
            // open shell and we end up with an invalid solid.
            if (BRepLib::OrientClosedSolid(temp)) {
                outputShape = temp;
            }
        }
    }

    shapes.clear();
    shapes.push_back(shape);
    shapes.push_back(res);
    shapes.push_back(perimeterCompound);
    *this = TopoShape(Tag, Hasher)
                .makeShapeWithElementMap(outputShape, MapperSewing(sewTool), shapes, op);
    return *this;
}

TopoShape& TopoShape::makeElementOffsetFace(const TopoShape& shape,
                                            double offset,
                                            double innerOffset,
                                            JoinType joinType,
                                            JoinType innerJoinType,
                                            const char* op)
{
    if (std::abs(innerOffset) < Precision::Confusion()
        && std::abs(offset) < Precision::Confusion()) {
        *this = shape;
        return *this;
    }

    if (shape.isNull()) {
        FC_THROWM(Base::ValueError, "makeOffsetFace: input shape is null!");
    }
    if (!shape.hasSubShape(TopAbs_FACE)) {
        FC_THROWM(Base::ValueError, "makeOffsetFace: no face found");
    }

    std::vector<TopoShape> res;
    for (auto& face : shape.getSubTopoShapes(TopAbs_FACE)) {
        std::vector<TopoShape> wires;
        TopoShape outerWire = face.splitWires(&wires, ReorientForward);
        if (wires.empty()) {
            res.push_back(makeElementOffset2D(face,
                                              offset,
                                              joinType,
                                              FillType::noFill,
                                              OpenResult::noOpenResult,
                                              false,
                                              op));
            continue;
        }
        if (outerWire.isNull()) {
            FC_THROWM(Base::CADKernelError, "makeOffsetFace: missing outer wire!");
        }

        if (std::abs(offset) > Precision::Confusion()) {
            outerWire = outerWire.makeElementOffset2D(offset,
                                                      joinType,
                                                      FillType::noFill,
                                                      OpenResult::noOpenResult,
                                                      false,
                                                      op);
        }

        if (std::abs(innerOffset) > Precision::Confusion()) {
            TopoShape innerWires(0, Hasher);
            innerWires.makeElementCompound(wires,
                                           "",
                                           SingleShapeCompoundCreationPolicy::returnShape);
            innerWires = innerWires.makeElementOffset2D(innerOffset,
                                                        innerJoinType,
                                                        FillType::noFill,
                                                        OpenResult::noOpenResult,
                                                        true,
                                                        op);
            wires = innerWires.getSubTopoShapes(TopAbs_WIRE);
        }
        wires.push_back(outerWire);
        gp_Pln pln;
        res.push_back(TopoShape(0, Hasher).makeElementFace(wires,
                                                           nullptr,
                                                           nullptr,
                                                           face.findPlane(pln) ? &pln : nullptr));
    }
    return makeElementCompound(res, "", SingleShapeCompoundCreationPolicy::returnShape);
}

TopoShape& TopoShape::makeElementOffset2D(const TopoShape& shape,
                                          double offset,
                                          JoinType joinType,
                                          FillType fill,
                                          OpenResult allowOpenResult,
                                          bool intersection,
                                          const char* op)
{
    if (!op) {
        op = Part::OpCodes::Offset2D;
    }

    if (shape.isNull()) {
        FC_THROWM(Base::ValueError, "makeOffset2D: input shape is null!");
    }
    if (allowOpenResult == OpenResult::allowOpenResult && OCC_VERSION_HEX < 0x060900) {
        FC_THROWM(Base::AttributeError, "openResult argument is not supported on OCC < 6.9.0.");
    }

    // OUTLINE OF MAKEOFFSET2D
    // * Prepare shapes to process
    // ** if _Shape is a compound, recursively call this routine for all subcompounds
    // ** if intrsection, dump all non-compound children into shapes to process; otherwise call this
    // routine recursively for all children
    // ** if _shape isn't a compound, dump it straight to shapes to process
    // * Test for shape types, and convert them all to wires
    // * find plane
    // * OCC call (BRepBuilderAPI_MakeOffset)
    // * postprocessing (facemaking):
    // ** convert offset result back to faces, if inputs were faces
    // ** OR do offset filling:
    // *** for closed wires, simply feed source wires + offset wires to smart facemaker
    // *** for open wires, try to connect source anf offset result by creating new edges (incomplete
    // implementation)
    // ** actual call to FaceMakerBullseye, unified for all facemaking.

    std::vector<TopoShape> shapesToProcess;
    std::vector<TopoShape> shapesToReturn;
    SingleShapeCompoundCreationPolicy outputPolicy = SingleShapeCompoundCreationPolicy::returnShape;
    if (shape.getShape().ShapeType() == TopAbs_COMPOUND) {
        if (!intersection) {
            // simply recursively process the children, independently
            expandCompound(shape, shapesToProcess);
            outputPolicy = SingleShapeCompoundCreationPolicy::forceCompound;
        }
        else {
            // collect non-compounds from this compound for collective offset. Process other shapes
            // independently.
            for (auto& s : shape.getSubTopoShapes()) {
                if (s.getShape().ShapeType() == TopAbs_COMPOUND) {
                    // recursively process subcompounds
                    shapesToReturn.push_back(TopoShape(Tag, Hasher)
                                                 .makeElementOffset2D(s,
                                                                      offset,
                                                                      joinType,
                                                                      fill,
                                                                      allowOpenResult,
                                                                      intersection,
                                                                      op));
                    outputPolicy = SingleShapeCompoundCreationPolicy::forceCompound;
                }
                else {
                    shapesToProcess.push_back(s);
                }
            }
        }
    }
    else {
        shapesToProcess.push_back(shape);
    }

    if (shapesToProcess.size() > 0) {
        TopoShape res(Tag, Hasher);

        // although 2d offset supports offsetting a face directly, it seems there is
        // no way to do a collective offset of multiple faces. So, we are doing it
        // by getting all wires from the faces, and applying offsets to them, and
        // reassembling the faces later.
        std::vector<TopoShape> sourceWires;
        bool haveWires = false;
        bool haveFaces = false;
        for (auto& s : shapesToProcess) {
            const auto& sh = s.getShape();
            switch (sh.ShapeType()) {
                case TopAbs_EDGE:
                    sourceWires.push_back(s.makeElementWires());
                    haveWires = true;
                    break;
                case TopAbs_WIRE:
                    sourceWires.push_back(s);
                    haveWires = true;
                    break;
                case TopAbs_FACE: {
                    auto outerWire = s.splitWires(&sourceWires);
                    sourceWires.push_back(outerWire);
                    haveFaces = true;
                } break;
                default:
                    FC_THROWM(Base::TypeError,
                              "makeOffset2D: input shape is not an edge, wire or face or compound "
                              "of those.");
                    break;
            }
        }
        if (haveWires && haveFaces) {
            FC_THROWM(
                Base::TypeError,
                "makeOffset2D: collective offset of a mix of wires and faces is not supported");
        }
        if (haveFaces) {
            allowOpenResult = OpenResult::noOpenResult;
        }

        // find plane.
        gp_Pln workingPlane;
        if (!TopoShape()
                 .makeElementCompound(sourceWires,
                                      "",
                                      SingleShapeCompoundCreationPolicy::returnShape)
                 .findPlane(workingPlane)) {
            FC_THROWM(Base::CADKernelError, "makeOffset2D: wires are nonplanar or noncoplanar");
        }

        // do the offset..
        TopoShape offsetShape;
        if (fabs(offset) > Precision::Confusion()) {
            BRepOffsetAPI_MakeOffsetFix mkOffset(GeomAbs_JoinType(joinType),
                                                 allowOpenResult == OpenResult::allowOpenResult);
            for (auto& w : sourceWires) {
                mkOffset.AddWire(TopoDS::Wire(w.getShape()));
            }
            try {
#if defined(__GNUC__) && defined(FC_OS_LINUX)
                Base::SignalException se;
#endif
                mkOffset.Perform(offset);
            }
            catch (Standard_Failure&) {
                throw;
            }
            catch (...) {
                FC_THROWM(Base::CADKernelError,
                          "BRepOffsetAPI_MakeOffset has crashed! (Unknown exception caught)");
            }
            if (mkOffset.Shape().IsNull()) {
                FC_THROWM(NullShapeException, "makeOffset2D: result of offsetting is null!");
            }

            // Copying shape to fix strange orientation behavior, OCC7.0.0. See bug #2699
            //  http://www.freecadweb.org/tracker/view.php?id=2699
            offsetShape = shape.makeElementShape(mkOffset, op).makeElementCopy();
        }
        else {
            offsetShape = TopoShape(Tag, Hasher)
                              .makeElementCompound(sourceWires,
                                                   0,
                                                   SingleShapeCompoundCreationPolicy::returnShape);
        }

        std::vector<TopoShape> offsetWires;
        // interestingly, if wires are removed, empty compounds are returned by MakeOffset (as of
        // OCC 7.0.0) so, we just extract all nesting
        expandCompound(offsetShape, offsetWires);
        if (offsetWires.empty()) {
            FC_THROWM(Base::CADKernelError, "makeOffset2D: offset result has no wires.");
        }

        std::vector<TopoShape> wiresForMakingFaces;
        if (fill == FillType::noFill) {
            if (haveFaces) {
                wiresForMakingFaces.insert(wiresForMakingFaces.end(),
                                           offsetWires.begin(),
                                           offsetWires.end());
            }
            else {
                shapesToReturn.insert(shapesToReturn.end(), offsetWires.begin(), offsetWires.end());
            }
        }
        else {
            // fill offset
            if (fabs(offset) < Precision::Confusion()) {
                FC_THROWM(Base::ValueError,
                          "makeOffset2D: offset distance is zero. Can't fill offset.");
            }

            // filling offset. There are three major cases to consider:
            //  1. source wires and result wires are closed (simplest) -> make face
            //  from source wire + offset wire
            //
            //  2. source wire is open, but offset wire is closed (if not
            //  allowOpenResult). -> throw away source wire and make face right from
            //  offset result.
            //
            //  3. source and offset wire are both open (note that there may be
            //  closed islands in offset result) -> need connecting offset result to
            //  source wire with new edges

            // first, lets split apart closed and open wires.
            std::vector<TopoShape> closedWires;
            std::vector<TopoShape> openWires;
            for (auto& w : sourceWires) {
                if (BRep_Tool::IsClosed(TopoDS::Wire(w.getShape()))) {
                    closedWires.push_back(w);
                }
                else {
                    openWires.push_back(w);
                }
            }
            for (auto& w : offsetWires) {
                if (BRep_Tool::IsClosed(TopoDS::Wire(w.getShape()))) {
                    closedWires.push_back(w);
                }
                else {
                    openWires.push_back(w);
                }
            }

            wiresForMakingFaces.insert(wiresForMakingFaces.end(),
                                       closedWires.begin(),
                                       closedWires.end());
            if (allowOpenResult == OpenResult::noOpenResult || openWires.size() == 0) {
                // just ignore all open wires
            }
            else {
                // We need to connect open wires to form closed wires.

                // for now, only support offsetting one open wire -> there should be exactly two
                // open wires for connecting
                if (openWires.size() != 2) {
                    FC_THROWM(Base::CADKernelError,
                              "makeOffset2D: collective offset with filling of multiple wires is "
                              "not supported yet.");
                }

                TopoShape openWire1 = openWires.front();
                TopoShape openWire2 = openWires.back();

                // find open vertices
                BRepTools_WireExplorer xp;
                xp.Init(TopoDS::Wire(openWire1.getShape()));
                TopoDS_Vertex v1 = xp.CurrentVertex();
                for (; xp.More(); xp.Next()) {};
                TopoDS_Vertex v2 = xp.CurrentVertex();

                // find open vertices
                xp.Init(TopoDS::Wire(openWire2.getShape()));
                TopoDS_Vertex v3 = xp.CurrentVertex();
                for (; xp.More(); xp.Next()) {};
                TopoDS_Vertex v4 = xp.CurrentVertex();

                // check
                if (v1.IsNull()) {
                    FC_THROWM(NullShapeException, "v1 is null");
                }
                if (v2.IsNull()) {
                    FC_THROWM(NullShapeException, "v2 is null");
                }
                if (v3.IsNull()) {
                    FC_THROWM(NullShapeException, "v3 is null");
                }
                if (v4.IsNull()) {
                    FC_THROWM(NullShapeException, "v4 is null");
                }

                // assemble new wire

                // we want the connection order to be
                // v1 -> openWire1 -> v2 -> (new edge) -> v4 -> openWire2(rev) -> v3 -> (new edge)
                // -> v1 let's check if it's the case. If not, we reverse one wire and swap its
                // endpoints.

                if (fabs(gp_Vec(BRep_Tool::Pnt(v2), BRep_Tool::Pnt(v3)).Magnitude() - fabs(offset))
                    <= BRep_Tool::Tolerance(v2) + BRep_Tool::Tolerance(v3)) {
                    openWire2._Shape.Reverse();
                    std::swap(v3, v4);
                    v3.Reverse();
                    v4.Reverse();
                }
                else if ((fabs(gp_Vec(BRep_Tool::Pnt(v2), BRep_Tool::Pnt(v4)).Magnitude()
                               - fabs(offset))
                          <= BRep_Tool::Tolerance(v2) + BRep_Tool::Tolerance(v4))) {
                    // orientation is as expected, nothing to do
                }
                else {
                    FC_THROWM(
                        Base::CADKernelError,
                        "makeOffset2D: fill offset: failed to establish open vertex relationship.");
                }

                // now directions of open wires are aligned. Finally. make new wire!
                BRepBuilderAPI_MakeWire mkWire;
                // add openWire1
                BRepTools_WireExplorer it;
                for (it.Init(TopoDS::Wire(openWire1.getShape())); it.More(); it.Next()) {
                    mkWire.Add(it.Current());
                }
                // add first joining edge
                mkWire.Add(BRepBuilderAPI_MakeEdge(v2, v4).Edge());
                // add openWire2, in reverse order
                openWire2._Shape.Reverse();
                for (it.Init(TopoDS::Wire(openWire2.getShape())); it.More(); it.Next()) {
                    mkWire.Add(it.Current());
                }
                // add final joining edge
                mkWire.Add(BRepBuilderAPI_MakeEdge(v3, v1).Edge());

                mkWire.Build();

                wiresForMakingFaces.push_back(
                    TopoShape(Tag, Hasher).makeElementShape(mkWire, openWires, op));
            }
        }

        // make faces
        if (wiresForMakingFaces.size() > 0) {
            TopoShape face(0, Hasher);
            face.makeElementFace(wiresForMakingFaces, nullptr, nullptr, &workingPlane);
            expandCompound(face, shapesToReturn);
        }
    }

    return makeElementCompound(shapesToReturn, op, outputPolicy);
}

TopoShape& TopoShape::makeElementThickSolid(const TopoShape& shape,
                                            const std::vector<TopoShape>& faces,
                                            double offset,
                                            double tol,
                                            bool intersection,
                                            bool selfInter,
                                            short offsetMode,
                                            JoinType join,
                                            const char* op)
{
    if (!op) {
        op = Part::OpCodes::Thicken;
    }

    // we do not offer tangent join type
    switch (join) {
        case JoinType::arc:
        case JoinType::intersection:
            break;
        default:
            join = JoinType::intersection;
    }

    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }

    if (faces.empty()) {
        FC_THROWM(NullShapeException, "Null input shape");
    }

    if (fabs(offset) <= 2 * tol) {
        *this = shape;
        return *this;
    }

    TopTools_ListOfShape remFace;
    for (auto& face : faces) {
        if (face.isNull()) {
            FC_THROWM(NullShapeException, "Null input shape");
        }
        if (!shape.findShape(face.getShape())) {
            FC_THROWM(Base::CADKernelError, "face does not belong to the shape");
        }
        remFace.Append(face.getShape());
    }
    BRepOffsetAPI_MakeThickSolid mkThick;
    mkThick.MakeThickSolidByJoin(shape.getShape(),
                                 remFace,
                                 offset,
                                 tol,
                                 BRepOffset_Mode(offsetMode),
                                 intersection ? Standard_True : Standard_False,
                                 selfInter ? Standard_True : Standard_False,
                                 GeomAbs_JoinType(join));
    return makeElementShape(mkThick, shape, op);
}


TopoShape& TopoShape::makeElementWires(const TopoShape& shape,
                                       const char* op,
                                       double tol,
                                       ConnectionPolicy policy,
                                       TopoShapeMap* output)
{
    return makeElementWires(std::vector<TopoShape>{shape}, op , tol, policy, output);
}


TopoShape& TopoShape::makeElementWires(const std::vector<TopoShape>& shapes,
                                       const char* op,
                                       double tol,
                                       ConnectionPolicy policy,
                                       TopoShapeMap* output)
{
    if (!op) {
        op = Part::OpCodes::Wire;
    }
    if (tol < Precision::Confusion()) {
        tol = Precision::Confusion();
    }

    if (policy == ConnectionPolicy::requireSharedVertex) {
        // Can't use ShapeAnalysis_FreeBounds if not shared. It seems the output
        // edges are modified somehow, and it is not obvious how to map the
        // resulting edges.
        Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
        Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
        for (const auto &shape : shapes) {
            for (const auto &edge : shape.getSubShapes(TopAbs_EDGE)) {
                hEdges->Append(edge);
            }
        }
        if (hEdges->Length() == 0) {
            FC_THROWM(NullShapeException, "Null shape");
        }
        ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges, tol, Standard_True, hWires);
        if (hWires->Length() == 0) {
            FC_THROWM(NullShapeException, "Null shape");
        }

        std::vector<TopoShape> wires;
        for (int i = 1; i <= hWires->Length(); i++) {
            auto wire = hWires->Value(i);
            wires.emplace_back(Tag,Hasher,wire);
            wires.back().mapSubElement(shapes, op);
        }
        return makeElementCompound(wires, "", SingleShapeCompoundCreationPolicy::returnShape);
    }

    std::vector<TopoShape> wires;
    std::list<TopoShape> edgeList;

    for (const auto &shape : shapes) {
        for(const auto &e : shape.getSubTopoShapes(TopAbs_EDGE)) {
            edgeList.emplace_back(e);
        }
    }

    std::vector<TopoShape> edges;
    edges.reserve(edgeList.size());
    wires.reserve(edgeList.size());

    // sort them together to wires
    while (!edgeList.empty()) {
        BRepBuilderAPI_MakeWire mkWire;
        // add and erase first edge
        edges.clear();
        edges.push_back(edgeList.front());
        mkWire.Add(TopoDS::Edge(edges.back().getShape()));
        edges.back().setShape(mkWire.Edge(), false);
        if (output) {
            (*output)[edges.back()] = edgeList.front();
        }
        edgeList.pop_front();

        TopoDS_Wire new_wire = mkWire.Wire();  // current new wire

        // try to connect each edge to the wire, the wire is complete if no more edges are
        // connectible
        bool found = true;
        while (found) {
            found = false;
            for (auto it = edgeList.begin(); it != edgeList.end(); ++it) {
                mkWire.Add(TopoDS::Edge(it->getShape()));
                if (mkWire.Error() != BRepBuilderAPI_DisconnectedWire) {
                    // edge added ==> remove it from list
                    found = true;
                    edges.push_back(*it);
                    // MakeWire will replace vertex of connected edge, which
                    // effectively creat a new edge. So we need to update the
                    // shape in order to preserve element mapping.
                    edges.back().setShape(mkWire.Edge(), false);
                    if (output) {
                        (*output)[edges.back()] = *it;
                    }
                    edgeList.erase(it);
                    new_wire = mkWire.Wire();
                    break;
                }
            }
        }

        wires.emplace_back(new_wire);
        wires.back().mapSubElement(edges, op);
        wires.back().fix();
    }
    return makeElementCompound(wires, nullptr, SingleShapeCompoundCreationPolicy::returnShape);
}


struct EdgePoints
{
    gp_Pnt v1, v2;
    std::list<TopoShape>::iterator it;
    const TopoShape* edge;
    bool closed {false};

    EdgePoints(std::list<TopoShape>::iterator it, double tol)
        : it(it)
        , edge(&*it)
    {
        TopExp_Explorer xp(it->getShape(), TopAbs_VERTEX);
        v1 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        xp.Next();
        if (xp.More()) {
            v2 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
            closed = (v2.SquareDistance(v1) <= tol);
        }
        else {
            v2 = v1;
            closed = true;
        }
    }
};

TopoShape TopoShape::reverseEdge(const TopoShape& edge)
{
    Standard_Real first = NAN;
    Standard_Real last = NAN;
    const Handle(Geom_Curve)& curve = BRep_Tool::Curve(TopoDS::Edge(edge.getShape()), first, last);
    first = curve->ReversedParameter(first);
    last = curve->ReversedParameter(last);
    TopoShape res(BRepBuilderAPI_MakeEdge(curve->Reversed(), last, first));
    auto edgeName = Data::IndexedName::fromConst("Edge", 1);
    if (auto mapped = edge.getMappedName(edgeName)) {
        res.elementMap()->setElementName(edgeName, mapped, res.Tag);
    }
    auto v1Name = Data::IndexedName::fromConst("Vertex", 1);
    auto v2Name = Data::IndexedName::fromConst("Vertex", 2);
    auto v1 = edge.getMappedName(v1Name);
    auto v2 = edge.getMappedName(v2Name);
    if (v1 && v2) {
        res.elementMap()->setElementName(v1Name, v2, res.Tag);
        res.elementMap()->setElementName(v2Name, v1, res.Tag);
    }
    else if (v1 && edge.countSubShapes(TopAbs_EDGE) == 1) {
        // It's possible an edge has only one vertex, so no need to reverse
        // the name
        res.elementMap()->setElementName(v1Name, v1, res.Tag);
    }
    else if (v1) {
        res.elementMap()->setElementName(v2Name, v1, res.Tag);
    }
    else if (v2) {
        res.elementMap()->setElementName(v1Name, v2, res.Tag);
    }
    return res;
};

std::deque<TopoShape> TopoShape::sortEdges(std::list<TopoShape>& edges, bool keepOrder, double tol)
{
    if (tol < Precision::Confusion()) {
        tol = Precision::Confusion();
    }
    double tol3d = tol * tol;

    std::list<EdgePoints> edgePoints;
    for (auto it = edges.begin(); it != edges.end(); ++it) {
        edgePoints.emplace_back(it, tol3d);
    }

    std::deque<TopoShape> sorted;
    if (edgePoints.empty()) {
        return sorted;
    }

    gp_Pnt first;
    gp_Pnt last;
    first = edgePoints.front().v1;
    last = edgePoints.front().v2;

    sorted.push_back(*edgePoints.front().edge);
    edges.erase(edgePoints.front().it);
    if (edgePoints.front().closed) {
        return sorted;
    }

    edgePoints.erase(edgePoints.begin());

    while (!edgePoints.empty()) {
        // search for adjacent edge
        std::list<EdgePoints>::iterator pEI;
        for (pEI = edgePoints.begin(); pEI != edgePoints.end(); ++pEI) {
            if (pEI->closed) {
                continue;
            }

            if (keepOrder && sorted.size() == 1) {
                if (pEI->v2.SquareDistance(first) <= tol3d
                    || pEI->v1.SquareDistance(first) <= tol3d) {
                    sorted[0] = reverseEdge(sorted[0]);
                    std::swap(first, last);
                }
            }

            if (pEI->v1.SquareDistance(last) <= tol3d) {
                last = pEI->v2;
                sorted.push_back(*pEI->edge);
                edges.erase(pEI->it);
                edgePoints.erase(pEI);
                pEI = edgePoints.begin();
                break;
            }
            if (pEI->v2.SquareDistance(first) <= tol3d) {
                sorted.push_front(*pEI->edge);
                first = pEI->v1;
                edges.erase(pEI->it);
                edgePoints.erase(pEI);
                pEI = edgePoints.begin();
                break;
            }
            if (pEI->v2.SquareDistance(last) <= tol3d) {
                last = pEI->v1;
                sorted.push_back(reverseEdge(*pEI->edge));
                edges.erase(pEI->it);
                edgePoints.erase(pEI);
                pEI = edgePoints.begin();
                break;
            }
            if (pEI->v1.SquareDistance(first) <= tol3d) {
                first = pEI->v2;
                sorted.push_front(reverseEdge(*pEI->edge));
                edges.erase(pEI->it);
                edgePoints.erase(pEI);
                pEI = edgePoints.begin();
                break;
            }
        }

        if ((pEI == edgePoints.end()) || (last.SquareDistance(first) <= tol3d)) {
            // no adjacent edge found or polyline is closed
            return sorted;
        }
    }

    return sorted;
}

TopoShape& TopoShape::makeElementOrderedWires(const std::vector<TopoShape>& shapes,
                                              const char* op,
                                              double tol,
                                              TopoShapeMap* output)
{
    if (!op) {
        op = Part::OpCodes::Wire;
    }
    if (tol < Precision::Confusion()) {
        tol = Precision::Confusion();
    }

    std::vector<TopoShape> wires;
    std::list<TopoShape> edgeList;

    auto shape =
        TopoShape().makeElementCompound(shapes, "", SingleShapeCompoundCreationPolicy::returnShape);
    for (auto& edge : shape.getSubTopoShapes(TopAbs_EDGE)) {
        edgeList.push_back(edge);
    }

    while (!edgeList.empty()) {
        BRepBuilderAPI_MakeWire mkWire;
        std::vector<TopoShape> edges;
        for (auto& edge : sortEdges(edgeList, true, tol)) {
            edges.push_back(edge);
            mkWire.Add(TopoDS::Edge(edge.getShape()));
            // MakeWire will replace vertex of connected edge, which
            // effectively creat a new edge. So we need to update the shape
            // in order to preserve element mapping.
            edges.back().setShape(mkWire.Edge(), false);
            if (output) {
                (*output)[edges.back()] = edge;
            }
        }
        wires.emplace_back(mkWire.Wire());
        wires.back().mapSubElement(edges, op);
    }
    return makeElementCompound(wires, nullptr, SingleShapeCompoundCreationPolicy::returnShape);
}

bool TopoShape::_makeElementTransform(const TopoShape& shape,
                                      const Base::Matrix4D& mat,
                                      const char* op,
                                      CheckScale checkScale,
                                      CopyType copy)
{
    if (checkScale == CheckScale::checkScale) {
        auto scaleType = mat.hasScale();
        if (scaleType != Base::ScaleType::NoScaling && scaleType != Base::ScaleType::Uniform) {
            makeElementGTransform(shape, mat, op, copy);
            return true;
        }
    }
    makeElementTransform(shape, convert(mat), op, copy);
    return false;
}

TopoShape& TopoShape::makeElementTransform(const TopoShape& shape,
                                           const gp_Trsf& trsf,
                                           const char* op,
                                           CopyType copy)
{
    if (copy == CopyType::noCopy) {
        // OCCT checks the ScaleFactor against gp::Resolution() which is DBL_MIN!!!
        copy = trsf.ScaleFactor() * trsf.HVectorialPart().Determinant() < 0.
                || Abs(Abs(trsf.ScaleFactor()) - 1) > Precision::Confusion()
            ? CopyType::copy
            : CopyType::noCopy;
    }
    TopoShape tmp(shape);
    if (copy == CopyType::copy) {
        if (shape.isNull()) {
            FC_THROWM(NullShapeException, "Null input shape");
        }

        BRepBuilderAPI_Transform mkTrf(shape.getShape(), trsf, Standard_True);
        // TODO: calling Moved() is to make sure the shape has some Location,
        // which is necessary for STEP export to work. However, if we reach
        // here, it probably means BRepBuilderAPI_Transform has modified
        // underlying shapes (because of scaling), it will break compound child
        // parent relationship anyway. In short, STEP import/export will most
        // likely break badly if there is any scaling involved
        tmp.setShape(mkTrf.Shape().Moved(gp_Trsf()), false);
    }
    else {
        tmp.move(trsf);
    }

    if (op || (shape.Tag && shape.Tag != Tag)) {
        setShape(tmp._Shape);
        initCache();
        if (!Hasher) {
            Hasher = tmp.Hasher;
        }
        copyElementMap(tmp, op);
    }
    else {
        *this = tmp;
    }
    return *this;
}

TopoShape& TopoShape::makeElementGTransform(const TopoShape& shape,
                                            const Base::Matrix4D& mat,
                                            const char* op,
                                            CopyType copy)
{
    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null input shape");
    }

    // if(!op) op = Part::OpCodes::Gtransform;
    gp_GTrsf matrix;
    matrix.SetValue(1, 1, mat[0][0]);
    matrix.SetValue(2, 1, mat[1][0]);
    matrix.SetValue(3, 1, mat[2][0]);
    matrix.SetValue(1, 2, mat[0][1]);
    matrix.SetValue(2, 2, mat[1][1]);
    matrix.SetValue(3, 2, mat[2][1]);
    matrix.SetValue(1, 3, mat[0][2]);
    matrix.SetValue(2, 3, mat[1][2]);
    matrix.SetValue(3, 3, mat[2][2]);
    matrix.SetValue(1, 4, mat[0][3]);
    matrix.SetValue(2, 4, mat[1][3]);
    matrix.SetValue(3, 4, mat[2][3]);

    // geometric transformation
    TopoShape tmp(shape);
    BRepBuilderAPI_GTransform mkTrf(shape.getShape(), matrix, copy == CopyType::copy);
    tmp.setShape(mkTrf.Shape(), false);
    if (op || (shape.Tag && shape.Tag != Tag)) {
        setShape(tmp._Shape);
        initCache();
        if (!Hasher) {
            Hasher = tmp.Hasher;
        }
        copyElementMap(tmp, op);
    }
    else {
        *this = tmp;
    }
    return *this;
}

TopoShape&
TopoShape::makeElementCopy(const TopoShape& shape, const char* op, bool copyGeom, bool copyMesh)
{
    if (shape.isNull()) {
        return *this;
    }

    TopoShape tmp(shape);
    tmp.setShape(BRepBuilderAPI_Copy(shape.getShape(), copyGeom, copyMesh).Shape(), false);
    tmp.setTransform(shape.getTransform());
    if (op || (shape.Tag && shape.Tag != Tag)) {
        setShape(tmp._Shape);
        initCache();
        if (!Hasher) {
            Hasher = tmp.Hasher;
        }
        copyElementMap(tmp, op);
    }
    else {
        *this = tmp;
    }
    return *this;
}

const std::vector<TopoDS_Shape>& MapperSewing::modified(const TopoDS_Shape& s) const
{
    _res.clear();
    try {
        const auto& shape = maker.Modified(s);
        if (!shape.IsNull() && !shape.IsSame(s)) {
            _res.push_back(shape);
        }
        else {
            const auto& sshape = maker.ModifiedSubShape(s);
            if (!sshape.IsNull() && !sshape.IsSame(s)) {
                _res.push_back(sshape);
            }
        }
    }
    catch (const Standard_Failure& e) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("Exception on shape mapper: " << e.GetMessageString());
        }
    }
    return _res;
}

struct MapperThruSections: MapperMaker
{
    TopoShape firstProfile;
    TopoShape lastProfile;

    MapperThruSections(BRepOffsetAPI_ThruSections& tmaker, const std::vector<TopoShape>& profiles)
        : MapperMaker(tmaker)
    {
        if (!tmaker.FirstShape().IsNull()) {
            firstProfile = profiles.front();
        }
        if (!tmaker.LastShape().IsNull()) {
            lastProfile = profiles.back();
        }
    }
    const std::vector<TopoDS_Shape>& generated(const TopoDS_Shape& s) const override
    {
        MapperMaker::generated(s);
        if (!_res.empty()) {
            return _res;
        }
        try {
            auto& tmaker = dynamic_cast<BRepOffsetAPI_ThruSections&>(maker);
            auto shape = tmaker.GeneratedFace(s);
            if (!shape.IsNull()) {
                _res.push_back(shape);
            }
            if (firstProfile.getShape().IsSame(s) || firstProfile.findShape(s)) {
                _res.push_back(tmaker.FirstShape());
            }
            else if (lastProfile.getShape().IsSame(s) || lastProfile.findShape(s)) {
                _res.push_back(tmaker.LastShape());
            }
        }
        catch (const Standard_Failure& e) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                FC_WARN("Exception on shape mapper: " << e.GetMessageString());
            }
        }
        return _res;
    }
};

struct MapperPrism: MapperMaker
{
    std::unordered_map<TopoDS_Shape, TopoDS_Shape, ShapeHasher, ShapeHasher> vertexMap;
    ShapeMapper::ShapeMap edgeMap;

    MapperPrism(BRepFeat_MakePrism& maker, const TopoShape& upTo)
        : MapperMaker(maker)
    {
        (void)upTo;

        std::vector<TopoShape> shapes;
        for (TopTools_ListIteratorOfListOfShape it(maker.FirstShape()); it.More(); it.Next()) {
            shapes.push_back(it.Value());
        }

        if (shapes.size()) {
            // It seems that BRepFeat_MakePrism::newEdges() does not return
            // edges generated by extruding the profile vertices. The following
            // code assumes BRepFeat_MakePrism::myFShape is the profile, and
            // FirstShape() returns the corresponding faces in the new shape,
            // i.e. the bottom profile, and add all edges that shares a
            // vertex with the profiles as new edges.

            std::unordered_set<TopoDS_Shape, ShapeHasher, ShapeHasher> edgeSet;
            TopoShape bottom;
            bottom.makeElementCompound(shapes,
                                       nullptr,
                                       TopoShape::SingleShapeCompoundCreationPolicy::returnShape);
            TopoShape shape(maker.Shape());
            for (auto& vertex : bottom.getSubShapes(TopAbs_VERTEX)) {
                for (auto& e : shape.findAncestorsShapes(vertex, TopAbs_EDGE)) {
                    // Make sure to not visit the the same edge twice.
                    // And check only edge that are not found in the bottom profile
                    if (!edgeSet.insert(e).second && !bottom.findShape(e)) {
                        auto otherVertex = TopExp::FirstVertex(TopoDS::Edge(e));
                        if (otherVertex.IsSame(vertex)) {
                            otherVertex = TopExp::LastVertex(TopoDS::Edge(e));
                        }
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
            for (auto& edge : bottom.getSubShapes(TopAbs_EDGE)) {
                std::vector<int> indices;
                auto first = TopExp::FirstVertex(TopoDS::Edge(edge));
                auto last = TopExp::LastVertex(TopoDS::Edge(edge));
                auto itFirst = vertexMap.find(first);
                auto itLast = vertexMap.find(last);
                if (itFirst == vertexMap.end() || itLast == vertexMap.end()) {
                    continue;
                }
                std::vector<TopoShape> faces;
                for (int idx : shape.findAncestors(edge, TopAbs_FACE)) {
                    faces.push_back(shape.getSubTopoShape(TopAbs_FACE, idx));
                }
                if (faces.empty()) {
                    continue;
                }
                for (int idx : shape.findAncestors(itFirst->second, TopAbs_EDGE)) {
                    auto e = shape.getSubTopoShape(TopAbs_EDGE, idx);
                    if (!e.findShape(itLast->second)) {
                        continue;
                    }
                    for (auto& face : faces) {
                        if (!face.findShape(e.getShape())) {
                            continue;
                        }
                        auto& entry = edgeMap[edge];
                        if (entry.shapeSet.insert(e.getShape()).second) {
                            entry.shapes.push_back(e.getShape());
                        }
                    }
                }
            }
        }
    }
    const std::vector<TopoDS_Shape>& generated(const TopoDS_Shape& s) const override
    {
        _res.clear();
        switch (s.ShapeType()) {
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
                if (it != edgeMap.end()) {
                    return it->second.shapes;
                }
                break;
            }
            default:
                break;
        }
        MapperMaker::generated(s);
        return _res;
    }
};

TopoShape& TopoShape::makeElementFilledFace(const std::vector<TopoShape>& _shapes,
                                            const BRepFillingParams& params,
                                            const char* op)
{
    if (!op) {
        op = Part::OpCodes::FilledFace;
    }
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

    if (!params.surface.isNull() && params.surface.getShape().ShapeType() == TopAbs_FACE) {
        maker.LoadInitSurface(TopoDS::Face(params.surface.getShape()));
    }

    std::vector<TopoShape> shapes;
    for (auto& s : _shapes) {
        expandCompound(s, shapes);
    }

    TopoShapeMap output;
    auto getOrder = [&](const TopoDS_Shape& s) {
        auto it = params.orders.find(s);
        if (it == params.orders.end()) {
            auto iter = output.find(s);
            if (iter != output.end()) {
                it = params.orders.find(iter->second.getShape());
            }
        }
        if (it != params.orders.end()) {
            return static_cast<GeomAbs_Shape>(it->second);
        }
        return GeomAbs_C0;
    };

    auto getSupport = [&](const TopoDS_Shape& s) {
        TopoDS_Face support;
        auto it = params.supports.find(s);
        if (it == params.supports.end()) {
            auto iter = output.find(s);
            if (iter != output.end()) {
                it = params.supports.find(iter->second.getShape());
            }
        }
        if (it != params.supports.end()) {
            if (!it->second.IsNull() && it->second.ShapeType() == TopAbs_FACE) {
                support = TopoDS::Face(it->second);
            }
        }
        return support;
    };

    auto findBoundary = [](std::vector<TopoShape>& shapes) -> TopoShape {
        // Find a wire (preferably a closed one) to be used as the boundary.
        int i = -1;
        int boundIdx = -1;
        for (auto& s : shapes) {
            ++i;
            if (s.isNull() || !s.hasSubShape(TopAbs_EDGE) || s.shapeType() != TopAbs_WIRE) {
                continue;
            }
            if (BRep_Tool::IsClosed(TopoDS::Wire(s.getShape()))) {
                boundIdx = i;
                break;
            }
            else if (boundIdx < 0) {
                boundIdx = i;
            }
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
    if (params.boundary_begin >= 0 && params.boundary_end > params.boundary_begin
        && params.boundary_end <= (int)shapes.size()) {
        if (params.boundary_end - 1 != params.boundary_begin
            || shapes[params.boundary_begin].shapeType() != TopAbs_WIRE) {
            std::vector<TopoShape> edges;
            edges.insert(edges.end(),
                         shapes.begin() + params.boundary_begin,
                         shapes.begin() + params.boundary_end);
            wires = TopoShape(0, Hasher)
                        .makeElementWires(edges,
                                          "",
                                          0.0,
                                          ConnectionPolicy::requireSharedVertex,
                                          &output)
                        .getSubTopoShapes(TopAbs_WIRE);
            shapes.erase(shapes.begin() + params.boundary_begin,
                         shapes.begin() + params.boundary_end);
        }
    }
    else {
        bound = findBoundary(shapes);
        if (bound.isNull()) {
            // If no boundary is found, then try to build one.
            std::vector<TopoShape> edges;
            for (auto it = shapes.begin(); it != shapes.end();) {
                if (it->shapeType(true) == TopAbs_EDGE) {
                    edges.push_back(*it);
                    it = shapes.erase(it);
                }
                else {
                    ++it;
                }
            }
            if (edges.size()) {
                wires = TopoShape(0, Hasher)
                            .makeElementWires(edges,
                                              "",
                                              0.0,
                                              ConnectionPolicy::requireSharedVertex,
                                              &output)
                            .getSubTopoShapes(TopAbs_WIRE);
            }
        }
    }

    if (bound.isNull()) {
        bound = findBoundary(wires);
    }

    if (bound.isNull()) {
        FC_THROWM(Base::CADKernelError, "No boundary wire");
    }

    // Since we've only selected one wire for boundary, return all the
    // other edges in shapes to be added as non boundary constraints
    shapes.insert(shapes.end(), wires.begin(), wires.end());

    // Must fix wire connection to avoid OCC crash in BRepFill_Filling.cxx WireFromList()
    // https://github.com/Open-Cascade-SAS/OCCT/blob/1c96596ae7ba120a678021db882857e289c73947/src/BRepFill/BRepFill_Filling.cxx#L133
    // The reason of crash is because the wire connection tolerance is too big.
    // The crash can be fixed by simply checking itl.More() before calling Remove().
    bound.fix(Precision::Confusion(), Precision::Confusion(), Precision::Confusion());

    for (const auto& e : bound.getOrderedEdges()) {
        maker.Add(TopoDS::Edge(e.getShape()),
                  getSupport(e.getShape()),
                  getOrder(e.getShape()),
                  /*IsBound*/ Standard_True);
    }

    for (const auto& s : shapes) {
        if (s.isNull()) {
            continue;
        }
        const auto& sh = s.getShape();
        if (sh.ShapeType() == TopAbs_WIRE) {
            for (const auto& e : s.getSubShapes(TopAbs_EDGE)) {
                maker.Add(TopoDS::Edge(e),
                          getSupport(e),
                          getOrder(e),
                          /*IsBound*/ Standard_False);
            }
        }
        else if (sh.ShapeType() == TopAbs_EDGE) {
            maker.Add(TopoDS::Edge(sh),
                      getSupport(sh),
                      getOrder(sh),
                      /*IsBound*/ Standard_False);
        }
        else if (sh.ShapeType() == TopAbs_FACE) {
            maker.Add(TopoDS::Face(sh), getOrder(sh));
        }
        else if (sh.ShapeType() == TopAbs_VERTEX) {
            maker.Add(BRep_Tool::Pnt(TopoDS::Vertex(sh)));
        }
    }

    maker.Build();
    if (!maker.IsDone()) {
        FC_THROWM(Base::CADKernelError, "Failed to created face by filling edges");
    }
    return makeElementShape(maker, _shapes, op);
}

// TODO:  This method does not appear to ever be called in the codebase, and it is probably
// broken, because using TopoShape() with no parameters means the result will not have an
// element Map.
// TopoShape& TopoShape::makeElementSolid(const std::vector<TopoShape>& shapes, const char* op)
//{
//    return makeElementSolid(TopoShape().makeElementCompound(shapes), op);
//}

TopoShape& TopoShape::makeElementSolid(const TopoShape& shape, const char* op)
{
    if (!op) {
        op = Part::OpCodes::Solid;
    }

    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }

    // first, if we were given a compsolid, try making a solid out of it
    TopoDS_CompSolid compsolid;
    int count = 0;
    for (const auto& s : shape.getSubShapes(TopAbs_COMPSOLID)) {
        ++count;
        compsolid = TopoDS::CompSolid(s);
        if (count > 1) {
            break;
        }
    }
    if (count == 0) {
        // no compsolids. Get shells...
        BRepBuilderAPI_MakeSolid mkSolid;
        count = 0;
        for (const auto& s : shape.getSubShapes(TopAbs_SHELL)) {
            ++count;
            mkSolid.Add(TopoDS::Shell(s));
        }

        if (count == 0) {  // no shells?
            FC_THROWM(Base::CADKernelError, "No shells or compsolids found in shape");
        }

        makeElementShape(mkSolid, shape, op);

        TopoDS_Solid solid = TopoDS::Solid(_Shape);
        BRepLib::OrientClosedSolid(solid);
        setShape(solid, false);
    }
    else if (count == 1) {
        BRepBuilderAPI_MakeSolid mkSolid(compsolid);
        makeElementShape(mkSolid, shape, op);
    }
    else {  // if (count > 1)
        FC_THROWM(Base::CADKernelError,
                  "Only one compsolid can be accepted. "
                  "Provided shape has more than one compsolid.");
    }
    return *this;
}

TopoShape& TopoShape::makeElementMirror(const TopoShape& shape, const gp_Ax2& ax2, const char* op)
{
    if (!op) {
        op = Part::OpCodes::Mirror;
    }

    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }
    gp_Trsf mat;
    mat.SetMirror(ax2);
    TopLoc_Location loc = shape.getShape().Location();
    gp_Trsf placement = loc.Transformation();
    mat = placement * mat;
    BRepBuilderAPI_Transform mkTrf(shape.getShape(), mat);
    return makeElementShape(mkTrf, shape, op);
}

TopoShape& TopoShape::makeElementSlice(const TopoShape& shape,
                                       const Base::Vector3d& dir,
                                       double distance,
                                       const char* op)
{
    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }
    TopoCrossSection cs(dir.x, dir.y, dir.z, shape, op);
    TopoShape res = cs.slice(1, distance);
    setShape(res._Shape);
    Hasher = res.Hasher;
    resetElementMap(res.elementMap());
    return *this;
}

TopoShape& TopoShape::makeElementSlices(const TopoShape& shape,
                                        const Base::Vector3d& dir,
                                        const std::vector<double>& distances,
                                        const char* op)
{
    std::vector<TopoShape> wires;
    TopoCrossSection cs(dir.x, dir.y, dir.z, shape, op);
    int index = 0;
    for (auto& distance : distances) {
        cs.slice(++index, distance, wires);
    }
    return makeElementCompound(wires, op, SingleShapeCompoundCreationPolicy::returnShape);
}

TopoShape& TopoShape::replaceElementShape(const TopoShape& shape,
                                          const std::vector<std::pair<TopoShape, TopoShape>>& s)
{
    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }
    BRepTools_ReShape reshape;
    std::vector<TopoShape> shapes;
    shapes.reserve(s.size() + 1);
    for (auto& v : s) {
        if (v.first.isNull() || v.second.isNull()) {
            FC_THROWM(NullShapeException, "Null input shape");
        }
        reshape.Replace(v.first.getShape(), v.second.getShape());
        shapes.push_back(v.second);
    }
    // TODO:  This does not work when replacing a shape in a compound.  Should we replace with
    // something else?
    //  Note that remove works with a compound.
    shapes.push_back(shape);
    setShape(reshape.Apply(shape.getShape(), TopAbs_SHAPE));
    mapSubElement(shapes);
    return *this;
}

TopoShape& TopoShape::removeElementShape(const TopoShape& shape, const std::vector<TopoShape>& s)
{
    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }
    BRepTools_ReShape reshape;
    for (auto& sh : s) {
        if (sh.isNull()) {
            FC_THROWM(NullShapeException, "Null input shape");
        }
        reshape.Remove(sh.getShape());
    }
    setShape(reshape.Apply(shape.getShape(), TopAbs_SHAPE));
    mapSubElement(shape);
    return *this;
}

TopoShape& TopoShape::makeElementFillet(const TopoShape& shape,
                                        const std::vector<TopoShape>& edges,
                                        double radius1,
                                        double radius2,
                                        const char* op)
{
    if (!op) {
        op = Part::OpCodes::Fillet;
    }
    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }

    if (edges.empty()) {
        FC_THROWM(NullShapeException, "Null input shape");
    }
    BRepFilletAPI_MakeFillet mkFillet(shape.getShape());
    for (auto& e : edges) {
        if (e.isNull()) {
            FC_THROWM(NullShapeException, "Null input shape");
        }
        const auto& edge = e.getShape();
        if (!shape.findShape(edge)) {
            FC_THROWM(Base::CADKernelError, "edge does not belong to the shape");
        }
        mkFillet.Add(radius1, radius2, TopoDS::Edge(edge));
    }
    return makeElementShape(mkFillet, shape, op);
}

TopoShape& TopoShape::makeElementChamfer(const TopoShape& shape,
                                         const std::vector<TopoShape>& edges,
                                         ChamferType chamferType,
                                         double radius1,
                                         double radius2,
                                         const char* op,
                                         Flip flipDirection)
{
    if (!op) {
        op = Part::OpCodes::Chamfer;
    }
    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }
    if (edges.empty()) {
        FC_THROWM(NullShapeException, "Null input shape");
    }
    BRepFilletAPI_MakeChamfer mkChamfer(shape.getShape());
    for (auto& e : edges) {
        const auto& edge = e.getShape();
        if (e.isNull()) {
            FC_THROWM(NullShapeException, "Null input shape");
        }
        if (!shape.findShape(edge)) {
            FC_THROWM(Base::CADKernelError, "edge does not belong to the shape");
        }
        // Add edge to fillet algorithm
        TopoDS_Shape face;
        if (flipDirection == Flip::flip) {
            face = shape.findAncestorsShapes(edge, TopAbs_FACE).back();
        }
        else {
            face = shape.findAncestorShape(edge, TopAbs_FACE);
        }
        switch (chamferType) {
            case ChamferType::equalDistance:  // Equal distance
                mkChamfer.Add(radius1, radius1, TopoDS::Edge(edge), TopoDS::Face(face));
                break;
            case ChamferType::twoDistances:  // Two distances
                mkChamfer.Add(radius1, radius2, TopoDS::Edge(edge), TopoDS::Face(face));
                break;
            case ChamferType::distanceAngle:  // Distance and angle
                mkChamfer.AddDA(radius1,
                                Base::toRadians(radius2),
                                TopoDS::Edge(edge),
                                TopoDS::Face(face));
                break;
        }
    }
    return makeElementShape(mkChamfer, shape, op);
}

TopoShape& TopoShape::makeElementGeneralFuse(const std::vector<TopoShape>& _shapes,
                                             std::vector<std::vector<TopoShape>>& modifies,
                                             double tol,
                                             const char* op)
{
    if (!op) {
        op = Part::OpCodes::GeneralFuse;
    }

    if (_shapes.empty()) {
        FC_THROWM(NullShapeException, "Null input shape");
    }

    std::vector<TopoShape> shapes(_shapes);

    BRepAlgoAPI_BuilderAlgo mkGFA;
    mkGFA.SetRunParallel(true);
    TopTools_ListOfShape GFAArguments;
    for (auto& shape : shapes) {
        if (shape.isNull()) {
            FC_THROWM(NullShapeException, "Null input shape");
        }
        GFAArguments.Append(shape.getShape());
    }
    mkGFA.SetArguments(GFAArguments);
    if (tol > 0.0) {
        mkGFA.SetFuzzyValue(tol);
    } else if (tol < 0.0) {
        FCBRepAlgoAPIHelper::setAutoFuzzy(&mkGFA);
    }
    mkGFA.SetNonDestructive(Standard_True);
    mkGFA.Build();
    if (!mkGFA.IsDone()) {
        FC_THROWM(Base::CADKernelError, "GeneralFuse failed");
    }
    makeElementShape(mkGFA, shapes, op);
    modifies.resize(shapes.size());
    int index = 0;
    for (auto& shape : shapes) {
        auto& mod = modifies[index++];
        for (TopTools_ListIteratorOfListOfShape it(mkGFA.Modified(shape.getShape())); it.More();
             it.Next()) {
            TopoShape res(Tag);
            res.setShape(it.Value());
            mod.push_back(res);
        }
        mapSubElementsTo(mod);
    }
    return *this;
}

TopoShape&
TopoShape::makeElementFuse(const std::vector<TopoShape>& shapes, const char* op, double tol)
{
    return makeElementBoolean(Part::OpCodes::Fuse, shapes, op, tol);
}

TopoShape&
TopoShape::makeElementCut(const std::vector<TopoShape>& shapes, const char* op, double tol)
{
    return makeElementBoolean(Part::OpCodes::Cut, shapes, op, tol);
}


TopoShape& TopoShape::makeElementShape(BRepBuilderAPI_MakeShape& mkShape,
                                       const TopoShape& source,
                                       const char* op)
{
    std::vector<TopoShape> sources(1, source);
    return makeElementShape(mkShape, sources, op);
}

TopoShape& TopoShape::makeElementShape(BRepBuilderAPI_MakeShape& mkShape,
                                       const std::vector<TopoShape>& shapes,
                                       const char* op)
{
    TopoDS_Shape shape;
    // OCCT 7.3.x requires calling Solid() and not Shape() to function correctly
    if (typeid(mkShape) == typeid(BRepPrimAPI_MakeHalfSpace)) {
        shape = static_cast<BRepPrimAPI_MakeHalfSpace&>(mkShape).Solid();
    }
    else {
        shape = mkShape.Shape();
    }
    return makeShapeWithElementMap(shape, MapperMaker(mkShape), shapes, op);
}

TopoShape& TopoShape::makeElementShape(BRepFeat_MakePrism& mkShape,
                                       const std::vector<TopoShape>& sources,
                                       const TopoShape& upTo,
                                       const char* op)
{
    if (!op) {
        op = Part::OpCodes::Prism;
    }
    MapperPrism mapper(mkShape, upTo);
    makeShapeWithElementMap(mkShape.Shape(), mapper, sources, op);
    return *this;
}


TopoShape& TopoShape::makeElementLoft(const std::vector<TopoShape>& shapes,
                                      IsSolid isSolid,
                                      IsRuled isRuled,
                                      IsClosed isClosed,
                                      Standard_Integer maxDegree,
                                      const char* op)
{
    if (!op) {
        op = Part::OpCodes::Loft;
    }

    // http://opencascade.blogspot.com/2010/01/surface-modeling-part5.html
    BRepOffsetAPI_ThruSections aGenerator(isSolid == IsSolid::solid, isRuled == IsRuled::ruled);
    aGenerator.SetMaxDegree(maxDegree);

    auto profiles = prepareProfiles(shapes);
    if (shapes.size() < 2) {
        FC_THROWM(Base::CADKernelError,
                  "Need at least two vertices, edges or wires to create loft face");
    }

    int i=0;
    Base::Vector3d center1,center2;
    for (auto& sh : profiles) {
        if (i>0) {
            if (sh.getCenterOfGravity(center1) && profiles[i-1].getCenterOfGravity(center2) && center1.IsEqual(center2,Precision::Confusion())) {
                FC_THROWM(Base::CADKernelError,
                          "Segments of a Loft/Pad do not have sufficient separation");
            }
        }
        const auto& shape = sh.getShape();
        if (shape.ShapeType() == TopAbs_VERTEX) {
            aGenerator.AddVertex(TopoDS::Vertex(shape));
        }
        else {
            aGenerator.AddWire(TopoDS::Wire(shape));
        }
        i++;
    }
    // close loft by duplicating initial profile as last profile.  not perfect.
    if (isClosed == IsClosed::closed) {
        /* can only close loft in certain combinations of Vertex/Wire(Edge):
            - V1-W1-W2-W3-V2  ==> V1-W1-W2-W3-V2-V1  invalid closed
            - V1-W1-W2-W3     ==> V1-W1-W2-W3-V1     valid closed
            - W1-W2-W3-V1     ==> W1-W2-W3-V1-W1     invalid closed
            - W1-W2-W3        ==> W1-W2-W3-W1        valid closed*/
        if (profiles.back().getShape().ShapeType() == TopAbs_VERTEX) {
            Base::Console().Message("TopoShape::makeLoft: can't close Loft with Vertex as last "
                                    "profile. 'Closed' ignored.\n");
        }
        else {
            // repeat Add logic above for first profile
            const TopoDS_Shape& firstProfile = profiles.front().getShape();
            if (firstProfile.ShapeType() == TopAbs_VERTEX) {
                aGenerator.AddVertex(TopoDS::Vertex(firstProfile));
            }
            else if (firstProfile.ShapeType() == TopAbs_EDGE) {
                aGenerator.AddWire(BRepBuilderAPI_MakeWire(TopoDS::Edge(firstProfile)).Wire());
            }
            else if (firstProfile.ShapeType() == TopAbs_WIRE) {
                aGenerator.AddWire(TopoDS::Wire(firstProfile));
            }
        }
    }

    Standard_Boolean anIsCheck = Standard_True;
    aGenerator.CheckCompatibility(anIsCheck);  // use BRepFill_CompatibleWires on profiles. force
                                               // #edges, orientation, "origin" to match.

    aGenerator.Build();
    return makeShapeWithElementMap(aGenerator.Shape(),
                                   MapperThruSections(aGenerator, profiles),
                                   shapes,
                                   op);
}

TopoShape& TopoShape::makeElementPrism(const TopoShape& base, const gp_Vec& vec, const char* op)
{
    if (!op) {
        op = Part::OpCodes::Extrude;
    }
    if (base.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }
    BRepPrimAPI_MakePrism mkPrism(base.getShape(), vec);
    return makeElementShape(mkPrism, base, op);
}

TopoShape& TopoShape::makeElementPrismUntil(const TopoShape& _base,
                                            const TopoShape& profile,
                                            const TopoShape& supportFace,
                                            const TopoShape& __uptoface,
                                            const gp_Dir& direction,
                                            PrismMode Mode,
                                            Standard_Boolean checkLimits,
                                            const char* op)
{
    if (!op) {
        op = Part::OpCodes::Prism;
    }

    BRepFeat_MakePrism PrismMaker;

    // don't remove limits of concave face
    if (checkLimits && __uptoface.shapeType(true) == TopAbs_FACE){
        Base::Vector3d vCog;
        profile.getCenterOfGravity(vCog);
        gp_Pnt pCog(vCog.x, vCog.y, vCog.z);
        checkLimits = ! Part::Tools::isConcave(TopoDS::Face(__uptoface.getShape()), pCog , direction);
    }

    TopoShape _uptoface(__uptoface);
    if (checkLimits && _uptoface.shapeType(true) == TopAbs_FACE
        && !BRep_Tool::NaturalRestriction(TopoDS::Face(_uptoface.getShape()))) {
        // When using the face with BRepFeat_MakePrism::Perform(const TopoDS_Shape& Until)
        // then the algorithm expects that the 'NaturalRestriction' flag is set in order
        // to work as expected.
        BRep_Builder builder;
        _uptoface = _uptoface.makeElementCopy();
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
    if (checkLimits && uptoface.shapeType(true) == TopAbs_FACE
        && uptoface.hasSubShape(TopAbs_WIRE)) {
        TopoDS_Face face = TopoDS::Face(uptoface.getShape());
        bool remove_limits = false;
        // Remove the limits of the upToFace so that the extrusion works even if profile is larger
        // than the upToFace
        for (auto& sketchface : profile.getSubTopoShapes(TopAbs_FACE)) {
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
            for (auto& w : wires) {
                BRepProj_Projection proj(TopoDS::Wire(w.getShape()),
                                         profile.getShape(),
                                         -direction);
                if (proj.More()) {
                    remove_limits = true;
                    break;
                }
            }
        }

        if (remove_limits) {
            // Note: Using an unlimited face every time gives unnecessary failures for concave
            // faces
            TopLoc_Location loc = face.Location();
            BRepAdaptor_Surface adapt(face, Standard_False);
            // use the placement of the adapter, not of the upToFace
            loc = TopLoc_Location(adapt.Trsf());
            BRepBuilderAPI_MakeFace mkFace(adapt.Surface().Surface(), Precision::Confusion());
            if (mkFace.IsDone()) {
                uptoface.setShape(located(mkFace.Shape(), loc), false);
            }
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
        if ((!_base.isNull() && base.isSame(_base)) || (_base.isNull() && base.isSame(profile))) {
            // It is unclear under exactly what condition extrude up to face
            // can fail. Either the support face or the up to face must be part
            // of the base, or maybe some thing else.
            //
            // To deal with it, we retry again by disregard the supplied base,
            // and use up to face to extrude our own base. Later on, use the
            // supplied base (i.e. _base) to calculate the final shape if the
            // mode is FuseWithBase or CutWithBase.
            checkBase = true;
            uptoface = uptofaceCopy;
            base.makeElementPrism(_uptoface, direction);
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

            for (auto& face : profile.getSubTopoShapes(
                     profile.hasSubShape(TopAbs_FACE) ? TopAbs_FACE : TopAbs_WIRE)) {
                srcShapes.clear();
                if (!profile.isNull() && !result.findShape(profile.getShape())) {
                    srcShapes.push_back(profile);
                }
                if (!supportFace.isNull() && !result.findShape(supportFace.getShape())) {
                    srcShapes.push_back(supportFace);
                }

                // DO NOT include uptoface for element mapping. Because OCCT
                // BRepFeat_MakePrism will report all top extruded face being
                // modified by the uptoface. If there are more than one face in
                // the profile, this will cause unnecessary duplicated element
                // mapped name. And will also disrupte element history tracing
                // back to the profile sketch.
                //
                // if (!uptoface.isNull() && !this->findShape(uptoface.getShape()))
                //     srcShapes.push_back(uptoface);

                srcShapes.push_back(result);

                if (result.isInfinite()){
                    result = face;
                }

                PrismMaker.Init(result.getShape(),
                                face.getShape(),
                                TopoDS::Face(supportFace.getShape()),
                                direction,
                                mode,
                                Standard_False);
                mode = PrismMode::FuseWithBase;

                PrismMaker.Perform(uptoface.getShape());

                if (!PrismMaker.IsDone() || PrismMaker.Shape().IsNull()) {
                    FC_THROWM(Base::CADKernelError, "BRepFeat_MakePrism: extrusion failed");
                }

                result.makeElementShape(PrismMaker, srcShapes, uptoface, op);
            }
            break;
        }
        catch (Base::Exception&) {
            if (!retry()) {
                throw;
            }
        }
        catch (Standard_Failure&) {
            if (!retry()) {
                throw;
            }
        }
    }

    if (!_base.isNull() && Mode != PrismMode::None) {
        if (Mode == PrismMode::FuseWithBase) {
            result.makeElementFuse({_base, result});
        }
        else {
            result.makeElementCut({_base, result});
        }
    }

    *this = result;
    return *this;
}

TopoShape& TopoShape::makeElementRevolve(const TopoShape& _base,
                                         const gp_Ax1& axis,
                                         double d,
                                         const char* face_maker,
                                         const char* op)
{
    if (!op) {
        op = Part::OpCodes::Revolve;
    }

    TopoShape base(_base);
    if (base.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }
    if (face_maker && !base.hasSubShape(TopAbs_FACE)) {
        if (!base.hasSubShape(TopAbs_WIRE)) {
            base = base.makeElementWires();
        }
        base = base.makeElementFace(nullptr, face_maker, nullptr);
    }
    BRepPrimAPI_MakeRevol mkRevol(base.getShape(), axis, d);
    return makeElementShape(mkRevol, base, op);
}

TopoShape& TopoShape::makeElementRevolution(const TopoShape& _base,
                                            const TopoDS_Shape& profile,
                                            const gp_Ax1& axis,
                                            const TopoDS_Face& supportface,
                                            const TopoDS_Face& uptoface,
                                            const char* face_maker,
                                            RevolMode Mode,
                                            Standard_Boolean Modify,
                                            const char* op)
{
    if (!op) {
        op = Part::OpCodes::Revolve;
    }
    if (Mode == RevolMode::None) {
        Mode = RevolMode::FuseWithBase;
    }
    TopoShape base(_base);
    if (base.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }
    if (face_maker && !base.hasSubShape(TopAbs_FACE)) {
        if (!base.hasSubShape(TopAbs_WIRE)) {
            base = base.makeElementWires();
        }
        base = base.makeElementFace(nullptr, face_maker, nullptr);
    }

    BRepFeat_MakeRevol mkRevol;
    for (TopExp_Explorer xp(profile, TopAbs_FACE); xp.More(); xp.Next()) {
        mkRevol.Init(base.getShape(),
                     xp.Current(),
                     supportface,
                     axis,
                     static_cast<int>(Mode),
                     Modify);
        mkRevol.Perform(uptoface);
        if (!mkRevol.IsDone()) {
            throw Base::RuntimeError("Revolution: Up to face: Could not revolve the sketch!");
        }
        base = mkRevol.Shape();
    }
    return makeElementShape(mkRevol, base, op);
}

TopoShape& TopoShape::makeElementDraft(const TopoShape& shape,
                                       const std::vector<TopoShape>& _faces,
                                       const gp_Dir& pullDirection,
                                       double angle,
                                       const gp_Pln& neutralPlane,
                                       bool retry,
                                       const char* op)
{
    if (!op) {
        op = Part::OpCodes::Draft;
    }

    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }

    std::vector<TopoShape> faces(_faces);
    bool done = true;
    BRepOffsetAPI_DraftAngle mkDraft;
    do {
        if (faces.empty()) {
            FC_THROWM(Base::CADKernelError, "no faces can be used");
        }

        mkDraft.Init(shape.getShape());
        done = true;
        for (auto it = faces.begin(); it != faces.end(); ++it) {
            // TODO: What is the flag for?
            mkDraft.Add(TopoDS::Face(it->getShape()), pullDirection, angle, neutralPlane);
            if (!mkDraft.AddDone()) {
                // Note: the function ProblematicShape returns the face on which the error occurred
                // Note: mkDraft.Remove() stumbles on a bug in Draft_Modification::Remove() and is
                //       therefore unusable. See
                //       http://forum.freecadweb.org/viewtopic.php?f=10&t=3209&start=10#p25341 The
                //       only solution is to discard mkDraft and start over without the current face
                // mkDraft.Remove(face);
                FC_ERR("Failed to add some face for drafting, skip");
                done = false;
                faces.erase(it);
                break;
            }
        }
    } while (retry && !done);

    mkDraft.Build();
    return makeElementShape(mkDraft, shape, op);
}

TopoShape& TopoShape::makeElementFace(const TopoShape& shape,
                                      const char* op,
                                      const char* maker,
                                      const gp_Pln* plane)
{
    std::vector<TopoShape> shapes;
    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }
    if (shape.getShape().ShapeType() == TopAbs_COMPOUND) {
        shapes = shape.getSubTopoShapes();
    }
    else {
        shapes.push_back(shape);
    }
    return makeElementFace(shapes, op, maker, plane);
}

TopoShape& TopoShape::makeElementFace(const std::vector<TopoShape>& shapes,
                                      const char* op,
                                      const char* maker,
                                      const gp_Pln* plane)
{
    if (!maker || !maker[0]) {
        maker = "Part::FaceMakerBullseye";
    }
    std::unique_ptr<FaceMaker> mkFace = FaceMaker::ConstructFromType(maker);
    mkFace->MyHasher = Hasher;
    mkFace->MyOp = op;
    if (plane) {
        mkFace->setPlane(*plane);
    }

    for (auto& shape : shapes) {
        if (shape.getShape().ShapeType() == TopAbs_COMPOUND) {
            mkFace->useTopoCompound(shape);
        }
        else {
            mkFace->addTopoShape(shape);
        }
    }
    mkFace->Build();

    const auto& ret = mkFace->getTopoShape();
    setShape(ret._Shape);
    Hasher = ret.Hasher;
    resetElementMap(ret.elementMap());
    if (!isValid()) {
        ShapeFix_ShapeTolerance aSFT;
        aSFT.LimitTolerance(getShape(),
                            Precision::Confusion(),
                            Precision::Confusion(),
                            TopAbs_SHAPE);

        // In some cases, the OCC reports the returned shape having invalid
        // tolerance. Not sure about the real cause.
        //
        // Update: one of the cause is related to OCC bug in
        // BRepBuilder_FindPlane, A possible call sequence is,
        //
        //      makeElementOffset2D() -> TopoShape::findPlane() -> BRepLib_FindSurface
        //
        // See code comments in findPlane() for the description of the bug and
        // work around.

        ShapeFix_Shape fixer(getShape());
        fixer.Perform();
        setShape(fixer.Shape(), false);

        if (!isValid()) {
            FC_WARN("makeElementFace: resulting face is invalid");
        }
    }
    return *this;
}

class MyRefineMaker: public BRepBuilderAPI_RefineModel
{
public:
    explicit MyRefineMaker(const TopoDS_Shape& s)
        : BRepBuilderAPI_RefineModel(s)
    {}

    void populate(ShapeMapper& mapper)
    {
        for (TopTools_DataMapIteratorOfDataMapOfShapeListOfShape it(this->myModified); it.More();
             it.Next()) {
            if (it.Key().IsNull()) {
                continue;
            }
            mapper.populate(MappingStatus::Generated, it.Key(), it.Value());
        }
    }
};

TopoShape& TopoShape::makeElementRefine(const TopoShape& shape, const char* op, RefineFail no_fail)
{
    if (shape.isNull()) {
        if (no_fail == RefineFail::throwException) {
            FC_THROWM(NullShapeException, "Null shape");
        }
        _Shape.Nullify();
        return *this;
    }
    if (!op) {
        op = Part::OpCodes::Refine;
    }
    bool closed = shape.isClosed();
    try {
        MyRefineMaker mkRefine(shape.getShape());
        GenericShapeMapper mapper;
        mkRefine.populate(mapper);
        mapper.init(shape, mkRefine.Shape());
        makeShapeWithElementMap(mkRefine.Shape(), mapper, {shape}, op);
        // For some reason, refine operation may reverse the solid
        fixSolidOrientation();
        if (isClosed() == closed) {
            return *this;
        }
    }
    catch (Standard_Failure&) {
        if (no_fail == RefineFail::throwException) {
            throw;
        }
    }
    *this = shape;
    return *this;
}

    std::vector<Data::IndexedName>
    TopoShape::getHigherElements(const char *element, bool silent) const
    {
        TopoShape shape = getSubTopoShape(element, silent);
        if(shape.isNull())
            return {};

        std::vector<Data::IndexedName> res;

        for (int type = shape.shapeType() - 1; type >= 0; type--) {
            const char* shapetype = shapeName((TopAbs_ShapeEnum)type).c_str();
            for (int idx : findAncestors(shape.getShape(), (TopAbs_ShapeEnum)type))
                res.emplace_back(shapetype, idx);
        }
        return res;
    }

TopoShape& TopoShape::makeElementBSplineFace(const TopoShape& shape,
                                             FillingStyle style,
                                             bool keepBezier,
                                             const char* op)
{
    std::vector<TopoShape> input(1, shape);
    return makeElementBSplineFace(input, style, keepBezier, op);
}

TopoShape& TopoShape::makeElementBSplineFace(const std::vector<TopoShape>& input,
                                             FillingStyle style,
                                             bool keepBezier,
                                             const char* op)
{
    std::vector<TopoShape> edges;
    for (auto& s : input) {
        auto e = s.getSubTopoShapes(TopAbs_EDGE);
        edges.insert(edges.end(), e.begin(), e.end());
    }

    if (edges.size() == 1 && edges[0].isClosed()) {
        auto edge = edges[0].getSubShape(TopAbs_EDGE, 1);
        auto e = TopoDS::Edge(edge);
        auto v = TopExp::FirstVertex(e);
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(e, first, last);

        BRepBuilderAPI_MakeEdge mk1, mk2, mk3, mk4;
        Handle(Geom_BSplineCurve) bspline = Handle(Geom_BSplineCurve)::DownCast(curve);
        if (bspline.IsNull()) {
            ShapeConstruct_Curve scc;
            bspline = scc.ConvertToBSpline(curve, first, last, Precision::Confusion());
            if (bspline.IsNull()) {
                FC_THROWM(Base::CADKernelError, "Failed to convert edge to bspline");
            }
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

        if (!mk1.IsDone() || !mk2.IsDone() || !mk3.IsDone() || !mk4.IsDone()) {
            FC_THROWM(Base::CADKernelError, "Failed to split edge");
        }

        auto e1 = mk1.Edge();
        auto e2 = mk2.Edge();
        auto e3 = mk3.Edge();
        auto e4 = mk4.Edge();

        ShapeMapper mapper;
        mapper.populate(MappingStatus::Modified, e, {e1, e2, e3, e4});
        mapper.populate(MappingStatus::Generated, v, {TopExp::FirstVertex(e1)});
        mapper.populate(MappingStatus::Generated, v, {TopExp::LastVertex(e4)});

        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);
        builder.Add(comp, e1);
        builder.Add(comp, e2);
        builder.Add(comp, e3);
        builder.Add(comp, e4);

        TopoShape s;
        s.makeShapeWithElementMap(comp, mapper, edges, Part::OpCodes::Split);
        return makeElementBSplineFace(s, style, op);
    }

    if (edges.size() < 2 || edges.size() > 4) {
        FC_THROWM(Base::CADKernelError, "Require minimum two, maximum four edges");
    }

    GeomFill_FillingStyle fstyle;
    switch (style) {
        case coons:
            fstyle = GeomFill_CoonsStyle;
            break;
        case curved:
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
        for (const auto& e : edges) {
            const TopoDS_Edge& edge = TopoDS::Edge(e.getShape());
            TopLoc_Location heloc;  // this will be output
            Handle(Geom_Curve) c_geom = BRep_Tool::Curve(edge, heloc, u1, u2);
            Handle(Geom_BezierCurve) curve = Handle(Geom_BezierCurve)::DownCast(c_geom);
            if (!curve) {
                break;
            }
            curve->Transform(
                heloc.Transformation());  // apply original transformation to control points
            curves.push_back(curve);
        }
        if (curves.size() == edges.size()) {
            GeomFill_BezierCurves aSurfBuilder;  // Create Surface Builder

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
        for (const auto& e : edges) {
            const TopoDS_Edge& edge = TopoDS::Edge(e.getShape());
            TopLoc_Location heloc;  // this will be output
            Handle(Geom_Curve) c_geom =
                BRep_Tool::Curve(edge, heloc, u1, u2);  // The geometric curve
            Handle(Geom_BSplineCurve) bspline =
                Handle(Geom_BSplineCurve)::DownCast(c_geom);  // Try to get BSpline curve
            if (!bspline.IsNull()) {
                gp_Trsf transf = heloc.Transformation();
                bspline->Transform(transf);  // apply original transformation to control points
                // Store Underlying Geometry
                curves.push_back(bspline);
            }
            else {
                // try to convert it into a B-spline
                BRepBuilderAPI_NurbsConvert mkNurbs(edge);
                TopoDS_Edge nurbs = TopoDS::Edge(mkNurbs.Shape());
                // avoid copying
                TopLoc_Location heloc2;  // this will be output
                Handle(Geom_Curve) c_geom2 =
                    BRep_Tool::Curve(nurbs, heloc2, u1, u2);  // The geometric curve
                Handle(Geom_BSplineCurve) bspline2 =
                    Handle(Geom_BSplineCurve)::DownCast(c_geom2);  // Try to get BSpline curve

                if (!bspline2.IsNull()) {
                    gp_Trsf transf = heloc2.Transformation();
                    bspline2->Transform(transf);  // apply original transformation to control points
                    // Store Underlying Geometry
                    curves.push_back(bspline2);
                }
                else {
                    // BRepBuilderAPI_NurbsConvert failed, try ShapeConstruct_Curve now
                    ShapeConstruct_Curve scc;
                    Handle(Geom_BSplineCurve) spline =
                        scc.ConvertToBSpline(c_geom, u1, u2, Precision::Confusion());
                    if (spline.IsNull()) {
                        Standard_Failure::Raise(
                            "A curve was not a B-spline and could not be converted into one.");
                    }
                    gp_Trsf transf = heloc2.Transformation();
                    spline->Transform(transf);  // apply original transformation to control points
                    curves.push_back(spline);
                }
            }
        }

        GeomFill_BSplineCurves aSurfBuilder;  // Create Surface Builder

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

    // TODO:  Is this correct?  makeElementBSplineFace is new (there is no corresponding non element
    // version of this operation).  It appears to be reasonable for the BRepBuilderAPI_MakeFace to
    // return more edges than we sent in.  The correspondence between old and  edges is assumed here
    // in resetting the element maps.
    auto newEdges = aFace.getSubTopoShapes(TopAbs_EDGE);
    if (newEdges.size() != edges.size()) {
        FC_WARN("Face edge count mismatch");
    }
    unsigned ind = 0;
    for (auto& edge : newEdges) {
        if (ind < edges.size()) {
            edge.resetElementMap(edges[ind++].elementMap());
        }
    }
    aFace.mapSubElement(newEdges);

    Data::ElementIDRefs sids;
    Data::MappedName edgeName =
        aFace.getMappedName(Data::IndexedName::fromConst("Edge", 1), true, &sids);
    aFace.setElementComboName(Data::IndexedName::fromConst("Face", 1),
                              {edgeName},
                              Part::OpCodes::BSplineFace,
                              op,
                              &sids);
    *this = aFace;
    return *this;
}

/**
 *  Encode and set an element name in the elementMap.  If a hasher is defined, apply it to the name.
 *
 * @param element   The element name(type) that provides 1 one character suffix to the name IF
 * <conditions>.
 * @param names     The subnames to build the name from.  If empty, return the TopoShape MappedName.
 * @param marker    The elementMap name or suffix to start the name with.  If null, use the
 *                  elementMapPrefix.
 * @param op        The op text passed to the element name encoder along with the TopoShape Tag
 * @param _sids     If defined, records the sub ids processed.
 *
 * @return          The encoded, possibly hashed name.
 */
Data::MappedName TopoShape::setElementComboName(const Data::IndexedName& element,
                                                const std::vector<Data::MappedName>& names,
                                                const char* marker,
                                                const char* op,
                                                const Data::ElementIDRefs* _sids)
{
    if (names.empty()) {
        return Data::MappedName {};
    }
    std::string _marker;
    if (!marker) {
        marker = elementMapPrefix().c_str();
    }
    else if (!boost::starts_with(marker, elementMapPrefix())) {
        _marker = elementMapPrefix() + marker;
        marker = _marker.c_str();
    }
    auto it = names.begin();
    Data::MappedName newName = *it;
    std::ostringstream ss;
    Data::ElementIDRefs sids;
    if (_sids) {
        sids = *_sids;
    }
    if (names.size() == 1) {
        ss << marker;
    }
    else {
        bool first = true;
        ss.str("");
        if (!Hasher) {
            ss << marker;
        }
        ss << '(';
        for (++it; it != names.end(); ++it) {
            if (first) {
                first = false;
            }
            else {
                ss << '|';
            }
            ss << *it;
        }
        ss << ')';
        if (Hasher) {
            sids.push_back(Hasher->getID(ss.str().c_str()));
            ss.str("");
            ss << marker << sids.back().toString();
        }
    }

    ensureElementMap()->encodeElementName(element[0], newName, ss, &sids, Tag, op);
    return elementMap()->setElementName(element, newName, Tag, &sids);
}

std::vector<Data::MappedName> TopoShape::decodeElementComboName(const Data::IndexedName& element,
                                                                const Data::MappedName& name,
                                                                const char* marker,
                                                                std::string* postfix) const
{
    std::vector<Data::MappedName> names;
    if (!element) {
        return names;
    }
    if (!marker) {
        marker = "";
    }
    int plen = (int)elementMapPrefix().size();
    int markerLen = strlen(marker);
    int len;
    int pos = name.findTagInElementName(nullptr, &len);
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
        }
        else {
            pos = name.find(")");
            if (pos < 0) {
                // non closing bracket?
                return {};
            }
            ++pos;
        }
        if (len <= (int)markerLen) {
            return {};
        }
        len -= markerLen + plen;
    }

    if (name.find(elementMapPrefix(), len) != len || name.find(marker, len + plen) != len + plen) {
        return {};
    }

    names.emplace_back(name, 0, len);

    std::string text;
    len += plen + markerLen;
    name.appendToBuffer(text, len, pos - len);

    if (this->Hasher) {
        if (auto id = App::StringID::fromString(names.back().toRawBytes())) {
            if (App::StringIDRef sid = this->Hasher->getID(id)) {
                names.pop_back();
                names.emplace_back(sid);
            }
            else {
                return names;
            }
        }
        if (auto id = App::StringID::fromString(text.c_str())) {
            if (App::StringIDRef sid = this->Hasher->getID(id)) {
                text = sid.dataToText();
            }
            else {
                return names;
            }
        }
    }
    if (text.empty() || text[0] != '(') {
        return names;
    }
    auto endPos = text.rfind(')');
    if (endPos == std::string::npos) {
        return names;
    }

    if (postfix) {
        *postfix = text.substr(endPos + 1);
    }

    text.resize(endPos);
    std::istringstream iss(text.c_str() + 1);
    std::string token;
    while (std::getline(iss, token, '|')) {
        names.emplace_back(token);
    }
    return names;
}

/**
 * Reorient the outer and inner wires of the TopoShape
 *
 * @param inner If this is not a nullptr, then any inner wires processed will be returned in this
 * vector.
 * @param reorient  One of NoReorient, Reorient ( Outer forward, inner reversed ),
 *                  ReorientForward ( all forward ), or ReorientReversed ( all reversed )
 * @return The outer wire, or an empty TopoShape if this isn't a Face, has no Face subShapes, or the
 *         outer wire isn't found.
 */
TopoShape TopoShape::splitWires(std::vector<TopoShape>* inner, SplitWireReorient reorient) const
{
    // ShapeAnalysis::OuterWire() is un-reliable for some reason. OCC source
    // code shows it works by creating face using each wire, and then test using
    // BRepTopAdaptor_FClass2d::PerformInfinitePoint() to check if it is an
    // outbound wire. And practice shows it sometimes returns the incorrect
    // result. Need more investigation. Note that this may be related to
    // unreliable solid face orientation
    // (https://forum.freecadweb.org/viewtopic.php?p=446006#p445674)
    //
    // Use BrepTools::OuterWire() instead. OCC source code shows it is
    // implemented using simple bound box checking. This should be a
    // reliable method, especially so for a planar face.

    TopoDS_Shape tmp;
    if (shapeType(true) == TopAbs_FACE) {
        tmp = BRepTools::OuterWire(TopoDS::Face(_Shape));
    }
    else if (countSubShapes(TopAbs_FACE) == 1) {
        tmp = BRepTools::OuterWire(TopoDS::Face(getSubShape(TopAbs_FACE, 1)));
    }
    if (tmp.IsNull()) {
        return TopoShape {};
    }
    const auto& wires = getSubTopoShapes(TopAbs_WIRE);
    auto it = wires.begin();

    TopAbs_Orientation orientOuter, orientInner;
    switch (reorient) {
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

    auto doReorient = [](TopoShape& s, TopAbs_Orientation orient) {
        // Special case of single edge wire. Make sure the edge is in the
        // required orientation. This is necessary because BRepFill_OffsetWire
        // has special handling of circular edge offset, which seem to only
        // respect the edge orientation and disregard the wire orientation. The
        // orientation is used to determine whether to shrink or expand.
        if (s.countSubShapes(TopAbs_EDGE) == 1) {
            TopoDS_Shape e = s.getSubShape(TopAbs_EDGE, 1);
            if (e.Orientation() == orient) {
                if (s._Shape.Orientation() == orient) {
                    return;
                }
            }
            else {
                e = e.Oriented(orient);
            }
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(e));
            s.setShape(mkWire.Shape(), false);
        }
        else if (s._Shape.Orientation() != orient) {
            s.setShape(s._Shape.Oriented(orient), false);
        }
    };

    for (; it != wires.end(); ++it) {
        auto& wire = *it;
        if (wire.getShape().IsSame(tmp)) {
            if (inner) {
                for (++it; it != wires.end(); ++it) {
                    inner->push_back(*it);
                    if (reorient) {
                        doReorient(inner->back(), orientInner);
                    }
                }
            }
            auto res = wire;
            if (reorient) {
                doReorient(res, orientOuter);
            }
            return res;
        }
        if (inner) {
            inner->push_back(wire);
            if (reorient) {
                doReorient(inner->back(), orientInner);
            }
        }
    }
    return TopoShape {};
}

bool TopoShape::isLinearEdge(Base::Vector3d* dir, Base::Vector3d* base) const
{
    if (isNull() || getShape().ShapeType() != TopAbs_EDGE) {
        return false;
    }

    if (!GeomCurve::isLinear(BRepAdaptor_Curve(TopoDS::Edge(getShape())).Curve().Curve(),
                             dir,
                             base)) {
        return false;
    }

    // BRep_Tool::Curve() will transform the returned geometry, so no need to
    // check the shape's placement.
    return true;
}

bool TopoShape::isPlanarFace(double tol) const
{
    if (isNull() || getShape().ShapeType() != TopAbs_FACE) {
        return false;
    }

    return GeomSurface::isPlanar(BRepAdaptor_Surface(TopoDS::Face(getShape())).Surface().Surface(),
                                 nullptr,
                                 tol);
}

// TODO:  Refactor this into two methods.  Totally separate concerns here.
bool TopoShape::linearize(LinearizeFace do_face, LinearizeEdge do_edge)
{
    bool touched = false;
    BRep_Builder builder;
    // Note: changing edge geometry seems to mess up with face (or shell, or solid)
    // Probably need to do some fix afterwards.
    if (do_edge == LinearizeEdge::linearizeEdges) {
        for (auto& edge : getSubTopoShapes(TopAbs_EDGE)) {
            TopoDS_Edge e = TopoDS::Edge(edge.getShape());
            BRepAdaptor_Curve curve(e);
            if (curve.GetType() == GeomAbs_Line || !edge.isLinearEdge()) {
                continue;
            }
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
    if (do_face == LinearizeFace::linearizeFaces) {
        for (auto& face : getSubTopoShapes(TopAbs_FACE)) {
            TopoDS_Face f = TopoDS::Face(face.getShape());
            BRepAdaptor_Surface surf(f);
            if (surf.GetType() == GeomAbs_Plane || !face.isPlanarFace()) {
                continue;
            }
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

struct MapperFill: Part::TopoShape::Mapper
{
    BRepFill_Generator& maker;
    explicit MapperFill(BRepFill_Generator& maker)
        : maker(maker)
    {}
    const std::vector<TopoDS_Shape>& generated(const TopoDS_Shape& s) const override
    {
        _res.clear();
        try {
            TopTools_ListIteratorOfListOfShape it;
            for (it.Initialize(maker.GeneratedShapes(s)); it.More(); it.Next()) {
                _res.push_back(it.Value());
            }
        }
        catch (const Standard_Failure& e) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                FC_WARN("Exception on shape mapper: " << e.GetMessageString());
            }
        }
        return _res;
    }
};

const std::vector<TopoDS_Shape>& MapperMaker::modified(const TopoDS_Shape& s) const
{
    _res.clear();
    try {
        TopTools_ListIteratorOfListOfShape it;
        for (it.Initialize(maker.Modified(s)); it.More(); it.Next()) {
            _res.push_back(it.Value());
        }
    }
    catch (const Standard_Failure& e) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("Exception on shape mapper: " << e.GetMessageString());
        }
    }
    return _res;
}

const std::vector<TopoDS_Shape>& MapperMaker::generated(const TopoDS_Shape& s) const
{
    _res.clear();
    try {
        TopTools_ListIteratorOfListOfShape it;
        for (it.Initialize(maker.Generated(s)); it.More(); it.Next()) {
            _res.push_back(it.Value());
        }
    }
    catch (const Standard_Failure& e) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("Exception on shape mapper: " << e.GetMessageString());
        }
    }
    return _res;
}

MapperHistory::MapperHistory(const Handle(BRepTools_History) & history)
    : history(history)
{}

MapperHistory::MapperHistory(const Handle(BRepTools_ReShape) & reshape)
{
    if (reshape) {
        history = reshape->History();
    }
}

MapperHistory::MapperHistory(ShapeFix_Root& fix)
{
    if (fix.Context()) {
        history = fix.Context()->History();
    }
}

const std::vector<TopoDS_Shape>& MapperHistory::modified(const TopoDS_Shape& s) const
{
    _res.clear();
    try {
        if (history) {
            TopTools_ListIteratorOfListOfShape it;
            for (it.Initialize(history->Modified(s)); it.More(); it.Next()) {
                _res.push_back(it.Value());
            }
        }
    }
    catch (const Standard_Failure& e) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("Exception on shape mapper: " << e.GetMessageString());
        }
    }
    return _res;
}

const std::vector<TopoDS_Shape>& MapperHistory::generated(const TopoDS_Shape& s) const
{
    _res.clear();
    try {
        if (history) {
            TopTools_ListIteratorOfListOfShape it;
            for (it.Initialize(history->Generated(s)); it.More(); it.Next()) {
                _res.push_back(it.Value());
            }
        }
    }
    catch (const Standard_Failure& e) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("Exception on shape mapper: " << e.GetMessageString());
        }
    }
    return _res;
}

// topo naming counterpart of TopoShape::makeShell()
TopoShape& TopoShape::makeElementShell(bool silent, const char* op)
{
    if (silent) {
        if (isNull()) {
            return *this;
        }

        if (shapeType(true) != TopAbs_COMPOUND) {
            return *this;
        }

        // we need a compound that consists of only faces
        TopExp_Explorer it;
        // no shells
        if (hasSubShape(TopAbs_SHELL)) {
            return *this;
        }

        // no wires outside a face
        it.Init(_Shape, TopAbs_WIRE, TopAbs_FACE);
        if (it.More()) {
            return *this;
        }

        // no edges outside a wire
        it.Init(_Shape, TopAbs_EDGE, TopAbs_WIRE);
        if (it.More()) {
            return *this;
        }

        // no vertexes outside an edge
        it.Init(_Shape, TopAbs_VERTEX, TopAbs_EDGE);
        if (it.More()) {
            return *this;
        }
    }
    else if (!hasSubShape(TopAbs_FACE)) {
        FC_THROWM(Base::CADKernelError, "Cannot make shell without face");
    }

    BRep_Builder builder;
    TopoDS_Shape shape;
    TopoDS_Shell shell;
    builder.MakeShell(shell);

    try {
        for (const auto& face : getSubShapes(TopAbs_FACE)) {
            builder.Add(shell, face);
        }

        TopoShape tmp(Tag, Hasher, shell);
        tmp.resetElementMap();
        tmp.mapSubElement(*this, op);

        shape = shell;
        BRepCheck_Analyzer check(shell);
        if (!check.IsValid()) {
            ShapeUpgrade_ShellSewing sewShell;
            shape = sewShell.ApplySewing(shell);
            // TODO confirm the above won't change OCCT topological naming
        }

        if (shape.IsNull()) {
            if (silent) {
                return *this;
            }
            FC_THROWM(NullShapeException, "Failed to make shell");
        }

        if (shape.ShapeType() != TopAbs_SHELL) {
            if (silent) {
                return *this;
            }
            FC_THROWM(Base::CADKernelError,
                      "Failed to make shell: unexpected output shape type "
                          << shapeType(shape.ShapeType(), true));
        }

        setShape(shape);
        resetElementMap(tmp.elementMap());
    }
    catch (Standard_Failure& e) {
        if (!silent) {
            FC_THROWM(Base::CADKernelError, "Failed to make shell: " << e.GetMessageString());
        }
    }

    return *this;
}

TopoShape& TopoShape::makeElementShellFromWires(const std::vector<TopoShape>& wires,
                                                bool silent,
                                                const char* op)
{
    BRepFill_Generator maker;
    for (auto& w : wires) {
        if (w.shapeType(silent) == TopAbs_WIRE) {
            maker.AddWire(TopoDS::Wire(w.getShape()));
        }
    }
    if (wires.empty()) {
        if (silent) {
            _Shape.Nullify();
            return *this;
        }
        FC_THROWM(NullShapeException, "No input shapes");
    }
    maker.Perform();
    this->makeShapeWithElementMap(maker.Shell(), MapperFill(maker), wires, op);
    return *this;
}

bool TopoShape::fixSolidOrientation()
{
    if (isNull()) {
        return false;
    }

    if (shapeType() == TopAbs_SOLID) {
        TopoDS_Solid solid = TopoDS::Solid(_Shape);
        BRepLib::OrientClosedSolid(solid);
        if (solid.IsEqual(_Shape)) {
            return false;
        }
        setShape(solid, false);
        return true;
    }

    if (shapeType() == TopAbs_COMPOUND || shapeType() == TopAbs_COMPSOLID) {
        auto shapes = getSubTopoShapes();
        bool touched = false;
        for (auto& s : shapes) {
            if (s.fixSolidOrientation()) {
                touched = true;
            }
        }
        if (!touched) {
            return false;
        }

        BRep_Builder builder;
        if (shapeType() == TopAbs_COMPOUND) {
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            for (auto& s : shapes) {
                if (!s.isNull()) {
                    builder.Add(comp, s.getShape());
                }
            }
            setShape(comp, false);
        }
        else {
            TopoDS_CompSolid comp;
            builder.MakeCompSolid(comp);
            for (auto& s : shapes) {
                if (!s.isNull()) {
                    builder.Add(comp, s.getShape());
                }
            }
            setShape(comp, false);
        }
        return true;
    }

    return false;
}

TopoShape& TopoShape::makeElementBoolean(const char* maker,
                                         const TopoShape& shape,
                                         const char* op,
                                         double tolerance)
{
    return makeElementBoolean(maker, std::vector<TopoShape>(1, shape), op, tolerance);
}


// TODO: Refactor this so that each OpCode type is a separate method to reduce size
TopoShape& TopoShape::makeElementBoolean(const char* maker,
                                         const std::vector<TopoShape>& shapes,
                                         const char* op,
                                         double tolerance)
{
    if (!maker) {
        FC_THROWM(Base::CADKernelError, "no maker");
    }

    if (!op) {
        op = maker;
    }

    if (shapes.empty()) {
        FC_THROWM(NullShapeException, "Null shape");
    }

    if (strcmp(maker, Part::OpCodes::Compound) == 0) {
        return makeElementCompound(shapes, op, SingleShapeCompoundCreationPolicy::returnShape);
    }
    else if (boost::starts_with(maker, Part::OpCodes::Face)) {
        std::string prefix(Part::OpCodes::Face);
        prefix += '.';
        const char* face_maker = 0;
        if (boost::starts_with(maker, prefix)) {
            face_maker = maker + prefix.size();
        }
        return makeElementFace(shapes, op, face_maker);
    }
    else if (strcmp(maker, Part::OpCodes::Wire) == 0) {
        return makeElementWires(shapes, op);
    }
    else if (strcmp(maker, Part::OpCodes::Compsolid) == 0) {
        BRep_Builder builder;
        TopoDS_CompSolid Comp;
        builder.MakeCompSolid(Comp);
        for (auto& s : shapes) {
            if (!s.isNull()) {
                builder.Add(Comp, s.getShape());
            }
        }
        setShape(Comp);
        mapSubElement(shapes, op);
        return *this;
    }

    if (strcmp(maker, Part::OpCodes::Pipe) == 0) {
        if (shapes.size() != 2) {
            FC_THROWM(Base::CADKernelError, "Sweep needs a spine and a shape");
        }
        if (shapes[0].isNull() || shapes[1].isNull()) {
            FC_THROWM(Base::CADKernelError, "Cannot sweep with empty spine or empty shape");
        }
        if (shapes[0].getShape().ShapeType() != TopAbs_WIRE) {
            FC_THROWM(Base::CADKernelError, "Spine shape is not a wire");
        }
        BRepOffsetAPI_MakePipe mkPipe(TopoDS::Wire(shapes[0].getShape()), shapes[1].getShape());
        return makeElementShape(mkPipe, shapes, op);
    }

    if (strcmp(maker, Part::OpCodes::Shell) == 0) {
        BRep_Builder builder;
        TopoDS_Shell shell;
        builder.MakeShell(shell);
        for (auto& s : shapes) {
            builder.Add(shell, s.getShape());
        }
        setShape(shell);
        mapSubElement(shapes, op);
        BRepCheck_Analyzer check(shell);
        if (!check.IsValid()) {
            ShapeUpgrade_ShellSewing sewShell;
            setShape(sewShell.ApplySewing(shell), false);
            // TODO: confirm the above won't change OCCT topological naming
        }
        return *this;
    }

    bool buildShell = true;

    std::vector<TopoShape> _shapes;
    if (strcmp(maker, Part::OpCodes::Fuse) == 0) {
        for (auto it = shapes.begin(); it != shapes.end(); ++it) {
            auto& s = *it;
            if (s.isNull()) {
                if ( it == shapes.begin() ) {
                    return *this; // Compatible with pre-TNP allowing <null shape>.fuse() behavior
                }
                FC_THROWM(NullShapeException, "Null input shape");
            }
            if (s.shapeType() == TopAbs_COMPOUND) {
                if (_shapes.empty()) {
                    _shapes.insert(_shapes.end(), shapes.begin(), it);
                }
                expandCompound(s, _shapes);
            }
            else if (_shapes.size()) {
                _shapes.push_back(s);
            }
        }
    }
    else if (strcmp(maker, Part::OpCodes::Cut) == 0) {
        for (unsigned i = 1; i < shapes.size(); ++i) {
            auto& s = shapes[i];
            if (s.isNull()) {
                FC_THROWM(NullShapeException, "Null input shape");
            }
            if (s.shapeType() == TopAbs_COMPOUND) {
                if (_shapes.empty()) {
                    _shapes.insert(_shapes.end(), shapes.begin(), shapes.begin() + i);
                }
                expandCompound(s, _shapes);
            }
            else if (_shapes.size()) {
                _shapes.push_back(s);
            }
        }
    }

    if (tolerance != 0.0 && _shapes.empty()) {
        _shapes = shapes;
    }

    const auto& inputs = _shapes.size() ? _shapes : shapes;
    if (inputs.empty()) {
        FC_THROWM(NullShapeException, "Null input shape");
    }
    if (inputs.size() == 1) {
        *this = inputs[0];
        if (shapes.size() == 1) {
            // _shapes has fewer items than shapes due to compound expansion.
            // Only warn if the caller passes one shape.
            FC_WARN("Boolean operation with only one shape input");
        }
        return *this;
    }

    std::unique_ptr<BRepAlgoAPI_BooleanOperation> mk;
    if (strcmp(maker, Part::OpCodes::Fuse) == 0) {
        mk.reset(new FCBRepAlgoAPI_Fuse);
    }
    else if (strcmp(maker, Part::OpCodes::Cut) == 0) {
        mk.reset(new FCBRepAlgoAPI_Cut);
    }
    else if (strcmp(maker, Part::OpCodes::Common) == 0) {
        mk.reset(new FCBRepAlgoAPI_Common);
    }
    else if (strcmp(maker, Part::OpCodes::Section) == 0) {
        mk.reset(new FCBRepAlgoAPI_Section);
        buildShell = false;
    }
    else {
        FC_THROWM(Base::CADKernelError, "Unknown maker");
    }

    TopTools_ListOfShape shapeArguments, shapeTools;

    int i = -1;
    for (const auto& shape : inputs) {
        if (shape.isNull()) {
            FC_THROWM(NullShapeException, "Null input shape");
        }
        if (++i == 0) {
            shapeArguments.Append(shape.getShape());
        }
        else {
            shapeTools.Append(shape.getShape());
        }
    }

#if OCC_VERSION_HEX >= 0x070500
    // -1/22/2024 Removing the parameter.
    // if (PartParams::getParallelRunThreshold() > 0) {
    mk->SetRunParallel(Standard_True);
    OSD_Parallel::SetUseOcctThreads(Standard_True);
    // }
#else
    // 01/22/2024 This will be an extremely rare case, since we don't
    // build against OCCT versions this old.  Removing the parameter.
    mk->SetRunParallel(true);
#endif

    mk->SetArguments(shapeArguments);
    mk->SetTools(shapeTools);
    if (tolerance > 0.0) {
        mk->SetFuzzyValue(tolerance);
    } else if (tolerance < 0.0) {
        FCBRepAlgoAPIHelper::setAutoFuzzy(mk.get());
    }
    mk->Build();
    makeElementShape(*mk, inputs, op);

    if (buildShell) {
        makeElementShell();
    }
    return *this;
}

bool TopoShape::isSame(const Data::ComplexGeoData& _other) const
{
    if (!_other.isDerivedFrom(TopoShape::getClassTypeId())) {
        return false;
    }

    const auto& other = static_cast<const TopoShape&>(_other);
    return Tag == other.Tag && Hasher == other.Hasher && _Shape.IsEqual(other._Shape);
}

long TopoShape::isElementGenerated(const Data::MappedName& _name, int depth) const
{
    long res = 0;
    long tag = 0;
    traceElement(_name, [&](const Data::MappedName& name, int offset, long tag2, long) {
        (void)offset;
        if (tag2 < 0) {
            tag2 = -tag2;
        }
        if (tag && tag2 != tag) {
            if (--depth < 1) {
                return true;
            }
        }
        tag = tag2;
        if (depth == 1 && name.startsWith(genPostfix(), offset)) {
            res = tag;
            return true;
        }
        return false;
    });

    return res;
}

void TopoShape::reTagElementMap(long tag, App::StringHasherRef hasher, const char* postfix)
{
    if (!tag) {
        FC_WARN("invalid shape tag for re-tagging");
        return;
    }

    if (_Shape.IsNull())
        return;

    TopoShape tmp(*this);
    initCache(1);
    Hasher = hasher;
    Tag = tag;
    resetElementMap();
    copyElementMap(tmp, postfix);
}

void TopoShape::cacheRelatedElements(const Data::MappedName& name,
                                     HistoryTraceType sameType,
                                     const QVector<Data::MappedElement>& names) const
{
    initCache();
    _cache->insertRelation(ShapeRelationKey(name, sameType), names);
}

bool TopoShape::getRelatedElementsCached(const Data::MappedName& name,
                                         HistoryTraceType sameType,
                                         QVector<Data::MappedElement>& names) const
{
    if (!_cache) {
        return false;
    }
    auto it = _cache->relations.find(ShapeRelationKey(name, sameType));
    if (it == _cache->relations.end()) {
        return false;
    }
    names = it->second;
    return true;
}

}  // namespace Part

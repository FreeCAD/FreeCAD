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

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <Precision.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#endif

#include "TopoShape.h"
#include "TopoShapeCache.h"
#include "TopoShapeOpCode.h"


FC_LOG_LEVEL_INIT("TopoShape", true, true)  // NOLINT

namespace Part
{

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

namespace
{
void warnIfLogging()
{
    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
        FC_WARN("hasher mismatch");  // NOLINT
    }
};

void hasherMismatchError()
{
    FC_ERR("hasher mismatch");  // NOLINT
}


void checkAndMatchHasher(TopoShape& topoShape1, const TopoShape& topoShape2)
{
    if (topoShape1.Hasher) {
        if (topoShape2.Hasher != topoShape1.Hasher) {
            if (topoShape1.getElementMapSize(false) == 0U) {
                warnIfLogging();
            }
            else {
                hasherMismatchError();
            }
            topoShape1.Hasher = topoShape2.Hasher;
        }
    }
    else {
        topoShape1.Hasher = topoShape2.Hasher;
    }
}
}  // namespace

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
            std::ostringstream ss;
            char elementType {shapeName(type)[0]};
            elementMap()->encodeElementName(elementType, name, ss, &sids, Tag, op, other.Tag);
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

    if ((getElementMapSize(false) == 0U) && this->_Shape.IsPartner(other._Shape)) {
        if (!this->Hasher) {
            this->Hasher = other.Hasher;
        }
        copyElementMap(other, op);
        return;
    }

    if (!forceHasher && other.Hasher) {
        checkAndMatchHasher(*this, other);
    }

    mapSubElementForShape(other, op);
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
        mapCompoundSubElements(shapes, op);
    }
    else {
        for (auto& shape : shapes) {
            mapSubElement(shape, op);
        }
    }
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

TopoShape&
TopoShape::makeElementCompound(const std::vector<TopoShape>& shapes,
                               const char* op,
                               SingleShapeCompoundCreationPolicy policy)
{
    if (policy == SingleShapeCompoundCreationPolicy::RETURN_SHAPE && shapes.size() == 1) {
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

TopoShape& TopoShape::makeElementWires(const std::vector<TopoShape>& shapes,
                                       const char* op,
                                       double tol,
                                       ConnectionPolicy policy,
                                       TopoShapeMap* output)
{
    if (shapes.empty()) {
        FC_THROWM(NullShapeException, "Null shape");
    }
    if (shapes.size() == 1) {
        return makeElementWires(shapes[0], op, tol, policy, output);
    }
    return makeElementWires(TopoShape(Tag).makeElementCompound(shapes), op, tol, policy, output);
}


TopoShape& TopoShape::makeElementWires(const TopoShape& shape,
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

    if (policy == ConnectionPolicy::REQUIRE_SHARED_VERTEX) {
        // Can't use ShapeAnalysis_FreeBounds if not shared. It seems the output
        // edges are modified somehow, and it is not obvious how to map the
        // resulting edges.
        Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
        Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
        for (TopExp_Explorer xp(shape.getShape(), TopAbs_EDGE); xp.More(); xp.Next()) {
            hEdges->Append(xp.Current());
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
            wires.emplace_back(Tag, Hasher, wire);
        }
        shape.mapSubElementsTo(wires, op);
        return makeElementCompound(wires, "", SingleShapeCompoundCreationPolicy::RETURN_SHAPE);
    }

    std::vector<TopoShape> wires;
    std::list<TopoShape> edgeList;

    for (auto& edge : shape.getSubTopoShapes(TopAbs_EDGE)) {
        edgeList.emplace_back(edge);
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
    return makeElementCompound(wires, nullptr, SingleShapeCompoundCreationPolicy::RETURN_SHAPE);
}


struct EdgePoints
{
    gp_Pnt v1, v2;
    std::list<TopoShape>::iterator it;
    const TopoShape* edge;
    bool closed{false};

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

TopoShape TopoShape::reverseEdge (const TopoShape& edge) {
    Standard_Real first = NAN;
    Standard_Real last = NAN;
    const Handle(Geom_Curve)& curve =
        BRep_Tool::Curve(TopoDS::Edge(edge.getShape()), first, last);
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

    auto shape = TopoShape().makeElementCompound(shapes, "", SingleShapeCompoundCreationPolicy::RETURN_SHAPE);
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
    return makeElementCompound(wires, nullptr, SingleShapeCompoundCreationPolicy::RETURN_SHAPE);
}


TopoShape &TopoShape::makeElementCopy(const TopoShape &shape, const char *op, bool copyGeom, bool copyMesh)
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
        initCache();
        if (!Hasher)
            Hasher = tmp.Hasher;
        copyElementMap(tmp, op);
    }else
        *this = tmp;
    return *this;
}

}  // namespace Part

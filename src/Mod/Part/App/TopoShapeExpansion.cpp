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
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>

#include <ShapeFix_Shape.hxx>
#include <ShapeFix_ShapeTolerance.hxx>

#endif

#include "TopoShape.h"
#include "TopoShapeCache.h"
#include "FaceMaker.h"


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
            if ( ! elementMap() ) {
                FC_THROWM(NullShapeException, "No element map");
            }
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
TopoShape::makeElementCompound(const std::vector<TopoShape>& shapes, const char* op, bool force)
{
    if (!force && shapes.size() == 1) {
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
        //      makEOffset2D() -> TopoShape::findPlane() -> BRepLib_FindSurface
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

/**
 *  Encode and set an element name in the elementMap.  If a hasher is defined, apply it to the name.
 *
 * @param element   The element name(type) that provides 1 one character suffix to the name IF <conditions>.
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
        return Data::MappedName();
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
    elementMap()->encodeElementName(element[0], newName, ss, &sids, Tag, op);
    return elementMap()->setElementName(element, newName, Tag, &sids);
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
    // BRepTopAdaptor_FClass2d::PerformInfinitePoint() to check if it is an out
    // bound wire. And practice shows it sometimes returns the incorrect
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
        return TopoShape();
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
    return TopoShape();
}


}  // namespace Part
